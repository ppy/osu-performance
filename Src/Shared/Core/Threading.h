#pragma once

class CPriorityMutex
{
public:
	void LockLowPrio();
	void UnlockLowPrio();

	void LockHighPrio();
	void UnlockHighPrio();

private:
	std::mutex M;
	std::mutex N;
	std::mutex L;
};

class CPriorityLock
{
public:
	CPriorityLock(CPriorityMutex* pPriorityMutex, bool isHighPriority);
	~CPriorityLock();

	void Lock();
	void Unlock();

private:
	CPriorityMutex* _pPriorityMutex;
	bool _isHighPriority;
	bool _isLocked;
};

class CRWMutex
{
public:
	CRWMutex()
	{
		readers = writers = read_waiters = write_waiters = 0;
	}

	void ReaderLock()
	{
		std::unique_lock<std::mutex> lock(mutex);

		if(writers || write_waiters)
		{
			read_waiters++;
			do
			{
				read.wait(lock);
			} while(writers || write_waiters);
			read_waiters--;
		}
		readers++;
	}

	void ReaderUnlock()
	{
		std::unique_lock<std::mutex> lock(mutex);

		readers--;
		if(write_waiters)
			write.notify_all();
	}

	void WriterLock()
	{
		std::unique_lock<std::mutex> lock(mutex);

		if(readers || writers)
		{
			write_waiters++;
			do
			{
				write.wait(lock);
			} while(readers || writers);
			write_waiters--;
		}
		writers = 1;
	}

	void WriterUnlock()
	{
		std::unique_lock<std::mutex> lock(mutex);

		writers = 0;
		if(write_waiters)
			write.notify_all();
		else if(read_waiters)
			read.notify_all();
	}

private:
	std::mutex mutex;
	std::condition_variable read, write;
	unsigned readers, writers, read_waiters, write_waiters;
};

class CRWLock
{
public:
	CRWLock(CRWMutex* pRWMutex, bool isWriter);
	~CRWLock();

	void Lock();
	void Unlock();

private:
	CRWMutex* _pRWMutex;
	bool _isWriter;
	bool _isLocked;
};

class CThreadPool
{
public:
	CThreadPool();
	CThreadPool(const u32 NumThreads);
	~CThreadPool();

	template<class F, class... Args>
	std::future<typename std::result_of<F(Args...)>::type> EnqueueTask(F && f, Args && ... args)
	{
		typedef typename std::result_of<F(Args...)>::type return_type;

		++m_NumTasksInSystem;

		auto task = std::make_shared< std::packaged_task<return_type()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);

		auto res = task->get_future();

		{
			std::lock_guard<std::mutex> lock{m_TaskQueueMutex};

			// add the task
			m_TaskQueue.emplace_back([task]() { (*task)(); });
		}

		m_WorkerCondition.notify_one();

		return res;
	}

	void StartThreads(const u32 Amount);
	void ShutdownThreads(const u32 Amount);

	u32 GetNumTasksInSystem() const
	{
		return m_NumTasksInSystem;
	}

	void WaitUntilFinished();
	void WaitUntilFinishedFor(const std::chrono::microseconds Duration);
	void FlushQueue();

private:
	u32 m_NumThreads = 0; // We don't have any threads running on startup
	std::vector<std::thread> m_Threads;

	std::deque< std::function<void()> > m_TaskQueue;
	std::mutex m_TaskQueueMutex;
	std::condition_variable m_WorkerCondition;

	std::atomic<u32> m_NumTasksInSystem{0}; // We have no tasks in the system on startup
	std::mutex m_SystemBusyMutex;
	std::condition_variable m_SystemBusyCondition;
};
