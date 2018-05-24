#pragma once

#include <pp/common.h>
#include <pp/shared/Threading.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

PP_NAMESPACE_BEGIN

class PriorityMutex
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

class PriorityLock
{
public:
	PriorityLock(PriorityMutex* pPriorityMutex, bool isHighPriority);
	~PriorityLock();

	void Lock();
	void Unlock();

private:
	PriorityMutex* _pPriorityMutex;
	bool _isHighPriority;
	bool _isLocked;
};

class RWMutex
{
public:
	RWMutex()
	{
		readers = writers = read_waiters = write_waiters = 0;
	}

	void ReaderLock()
	{
		std::unique_lock<std::mutex> lock(mutex);

		if (writers || write_waiters)
		{
			read_waiters++;
			do
			{
				read.wait(lock);
			} while (writers || write_waiters);
			read_waiters--;
		}
		readers++;
	}

	void ReaderUnlock()
	{
		std::unique_lock<std::mutex> lock(mutex);

		readers--;
		if (write_waiters)
			write.notify_all();
	}

	void WriterLock()
	{
		std::unique_lock<std::mutex> lock(mutex);

		if (readers || writers)
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
		if (write_waiters)
			write.notify_all();
		else if (read_waiters)
			read.notify_all();
	}

private:
	std::mutex mutex;
	std::condition_variable read, write;
	unsigned readers, writers, read_waiters, write_waiters;
};

class RWLock
{
public:
	RWLock(RWMutex* pRWMutex, bool isWriter);
	~RWLock();

	void Lock();
	void Unlock();

private:
	RWMutex* _pRWMutex;
	bool _isWriter;
	bool _isLocked;
};

class ThreadPool
{
public:
	ThreadPool();
	ThreadPool(const u32 numThreads);
	~ThreadPool();

	template<class F, class... Args>
	std::future<typename std::result_of<F(Args...)>::type> EnqueueTask(F && f, Args && ... args)
	{
		typedef typename std::result_of<F(Args...)>::type return_type;

		++_numTasksInSystem;

		auto task = std::make_shared< std::packaged_task<return_type()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

		auto res = task->get_future();

		{
			std::lock_guard<std::mutex> lock{_taskQueueMutex};

			// add the task
			_taskQueue.emplace_back([task]() { (*task)(); });
		}

		_workerCondition.notify_one();

		return res;
	}

	void StartThreads(const u32 num);
	void ShutdownThreads(const u32 num);

	u32 GetNumTasksInSystem() const
	{
		return _numTasksInSystem;
	}

	void WaitUntilFinished();
	void WaitUntilFinishedFor(const std::chrono::microseconds Duration);
	void FlushQueue();

private:
	u32 _numThreads = 0; // We don't have any threads running on startup
	std::vector<std::thread> _threads;

	std::deque<std::function<void()>> _taskQueue;
	std::mutex _taskQueueMutex;
	std::condition_variable _workerCondition;

	std::atomic<u32> _numTasksInSystem{0}; // We have no tasks in the system on startup
	std::mutex _systemBusyMutex;
	std::condition_variable _systemBusyCondition;
};

PP_NAMESPACE_END
