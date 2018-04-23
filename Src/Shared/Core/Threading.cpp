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
		return;

	if (_isHighPriority)
		_pPriorityMutex->LockHighPrio();
	else
		_pPriorityMutex->LockLowPrio();

	_isLocked = true;
}

void CPriorityLock::Unlock()
{
	if (!_isLocked)
		return;

	if (_isHighPriority)
		_pPriorityMutex->UnlockHighPrio();
	else
		_pPriorityMutex->UnlockLowPrio();

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
	if (_isLocked)
		return;

	if (_isWriter)
		_pRWMutex->WriterLock();
	else
		_pRWMutex->ReaderLock();

	_isLocked = true;
}

void CRWLock::Unlock()
{
	if (!_isLocked)
		return;

	if (_isWriter)
		_pRWMutex->WriterUnlock();
	else
		_pRWMutex->ReaderUnlock();

	_isLocked = false;
}

CThreadPool::CThreadPool()
: CThreadPool{0}
{
}

CThreadPool::CThreadPool(const u32 numThreads)
{
	StartThreads(numThreads);
}

CThreadPool::~CThreadPool()
{
	ShutdownThreads((u32)_threads.size());
	Log(CLog::EType::Threads, "All threads terminated.");
}

void CThreadPool::StartThreads(const u32 num)
{
	_numThreads += num;
	for (u32 i = (u32)_threads.size(); i < _numThreads; ++i)
	{
		_threads.emplace_back([this, i]
		{
			Log(CLog::Threads, StrFormat("Worker thread {0} started.", i));

			while (true)
			{
				std::unique_lock<std::mutex> lock{_taskQueueMutex};

				// look for a work item
				while (i < _numThreads && _taskQueue.empty())
				{
					//__LOG_DBG(MSG_THREADS, "Thread {0} is waiting for a task.", i);

					// if there are none wait for notification
					_workerCondition.wait(lock);
				}

				if (i >= _numThreads)
					break;

				std::function<void()> task{std::move(_taskQueue.front())};
				_taskQueue.pop_front();

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

				_numTasksInSystem--;

				{
					std::unique_lock<std::mutex> lock{_systemBusyMutex};

					if (_numTasksInSystem == 0)
						_systemBusyCondition.notify_all();
				}
			}

			Log(CLog::Threads, StrFormat("Worker thread {0} stopped.", i));
		});
	}
}

void CThreadPool::ShutdownThreads(const u32 num)
{
	auto numToClose = std::min(num, _numThreads);

	{
		std::lock_guard<std::mutex> lock{_taskQueueMutex};

		_numThreads -= numToClose;
	}

	// Wake up all the threads to have them quit
	_workerCondition.notify_all();

	for (auto i = 0u; i < numToClose; ++i)
	{
		// Join them
		_threads.back().join();
		_threads.pop_back();
	}
}

void CThreadPool::WaitUntilFinished()
{
	std::unique_lock<std::mutex> lock{_systemBusyMutex};

	if (_numTasksInSystem == 0)
		return;

	_systemBusyCondition.wait(lock);
}

void CThreadPool::WaitUntilFinishedFor(const std::chrono::microseconds Duration)
{
	std::unique_lock<std::mutex> lock{_systemBusyMutex};

	if (_numTasksInSystem == 0)
		return;

	_systemBusyCondition.wait_for(lock, Duration);
}

void CThreadPool::FlushQueue()
{
	std::lock_guard<std::mutex> lock{_taskQueueMutex};

	_numTasksInSystem -= (u32)_taskQueue.size();

	// Clear the task queue
	_taskQueue.clear();
}
