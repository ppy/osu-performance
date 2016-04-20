#include <Shared.h>
#include "Threading.h"


void CPriorityMutex::LockLowPrio()
{
	L.lock();
	N.lock();
	M.lock();
	N.unlock();
}

void CPriorityMutex::UnlockLowPrio()
{
	M.unlock();
	L.unlock();
}


void CPriorityMutex::LockHighPrio()
{
	N.lock();
	M.lock();
	N.unlock();
}

void CPriorityMutex::UnlockHighPrio()
{
	M.unlock();
}


CPriorityLock::CPriorityLock(CPriorityMutex* pPriorityMutex, bool isHighPriority)
	: _pPriorityMutex{pPriorityMutex}, _isHighPriority{isHighPriority}, _isLocked{false}
{
	Lock();
}

CPriorityLock::~CPriorityLock()
{
	Unlock();
}

void CPriorityLock::Lock()
{
	if (_isLocked)
	{
		return;
	}

	if(_isHighPriority)
	{
		_pPriorityMutex->LockHighPrio();
	}
	else
	{
		_pPriorityMutex->LockLowPrio();
	}

	_isLocked = true;
}

void CPriorityLock::Unlock()
{
	if(!_isLocked)
	{
		return;
	}

	if(_isHighPriority)
	{
		_pPriorityMutex->UnlockHighPrio();
	}
	else
	{
		_pPriorityMutex->UnlockLowPrio();
	}

	_isLocked = false;
}


CRWLock::CRWLock(CRWMutex* pRWMutex, bool isWriter)
	: _pRWMutex{pRWMutex}, _isWriter{isWriter}, _isLocked{false}
{
	Lock();
}

CRWLock::~CRWLock()
{
	Unlock();
}

void CRWLock::Lock()
{
	if(_isLocked)
	{
		return;
	}

	if(_isWriter)
	{
		_pRWMutex->WriterLock();
	}
	else
	{
		_pRWMutex->ReaderLock();
	}

	_isLocked = true;
}

void CRWLock::Unlock()
{
	if(!_isLocked)
	{
		return;
	}

	if(_isWriter)
	{
		_pRWMutex->WriterUnlock();
	}
	else
	{
		_pRWMutex->ReaderUnlock();
	}

	_isLocked = false;
}


CThreadPool::CThreadPool()
	: CThreadPool{0}
{

}

CThreadPool::CThreadPool(const u32 NumThreads)
{
	StartThreads(NumThreads);
}

CThreadPool::~CThreadPool()
{
	ShutdownThreads(m_Threads.size());

	Log(CLog::EType::Threads, "All threads terminated.");
}


void CThreadPool::StartThreads(const u32 Amount)
{
	m_NumThreads += Amount;
	for(u32 i = m_Threads.size(); i < m_NumThreads; ++i)
	{
		m_Threads.emplace_back(
			[this, i]
		{
			Log(CLog::Threads, StrFormat("Worker thread {0} started.", i));

			while(true)
			{
				std::unique_lock<std::mutex> lock{this->m_TaskQueueMutex};

				// look for a work item
				while(i < this->m_NumThreads &&
					this->m_TaskQueue.empty())
				{
					//__LOG_DBG(MSG_THREADS, "Thread {0} is waiting for a task.", i);

					// if there are none wait for notification
					this->m_WorkerCondition.wait(lock);
				}

				if(i >= this->m_NumThreads)
				{
					break;
				}

				std::function<void()> task{std::move(this->m_TaskQueue.front())};
				this->m_TaskQueue.pop_front();

				// Unlock the lock, so we can process the task without blocking other threads
				lock.unlock();

				try
				{
					task();
				}
				catch (CLoggedException& e)
				{
					e.Log();
				}

				this->m_NumTasksInSystem--;

				{
					std::unique_lock<std::mutex> lock{this->m_SystemBusyMutex};

					if(m_NumTasksInSystem == 0)
					{
						this->m_SystemBusyCondition.notify_all();
					}
				}
			}

			Log(CLog::Threads, StrFormat("Worker thread {0} stopped.", i));
		}
		);
	}
}


void CThreadPool::ShutdownThreads(const u32 Amount)
{
	auto AmountToClose = std::min(Amount, m_NumThreads);

	{
		std::lock_guard<std::mutex> lock{m_TaskQueueMutex};

		m_NumThreads -= AmountToClose;
	}

	// Wake up all the threads to have them quit
	m_WorkerCondition.notify_all();

	for(auto i = 0u; i < AmountToClose; ++i)
	{
		// Join them
		m_Threads.back().join();
		m_Threads.pop_back();
	}
}


void CThreadPool::WaitUntilFinished()
{
	std::unique_lock<std::mutex> lock{m_SystemBusyMutex};

	if(m_NumTasksInSystem == 0)
	{
		return;
	}

	m_SystemBusyCondition.wait(lock);
}


void CThreadPool::WaitUntilFinishedFor(const std::chrono::microseconds Duration)
{
	std::unique_lock<std::mutex> lock{m_SystemBusyMutex};

	if(m_NumTasksInSystem == 0)
	{
		return;
	}

	m_SystemBusyCondition.wait_for(lock, Duration);
}


void CThreadPool::FlushQueue()
{
	std::lock_guard<std::mutex> lock{m_TaskQueueMutex};

	m_NumTasksInSystem -= m_TaskQueue.size();

	// Clear the task queue
	m_TaskQueue.clear();

}
