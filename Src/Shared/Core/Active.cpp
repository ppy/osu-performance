#include <Shared.h>

#include "Active.h"

CActive::~CActive()
{
	if(!_isDone)
	{
		Send(std::bind(&CActive::DoDone, this), false);

		if(_thread.joinable())
		{
			_thread.join();
		}
	}
}

void CActive::Send(std::function<void()> callback)
{
	Send(callback, true);
}

std::unique_ptr<CActive> CActive::Create()
{
	auto pActive = std::unique_ptr<CActive>{ new CActive{} };
	pActive->_thread = std::thread(&CActive::Run, pActive.get());
	return pActive;
}

size_t CActive::AmountPending() const
{
	CheckForAndThrowException();
	return _tasks.Size();
}

bool CActive::IsBusy() const
{
	return AmountPending() > 0;
}


CActive::CActive()
{
	// This is required here, because atomic doesn't allow
	// copy construction.
	_isDone = false;
}

void CActive::Run()
{
	try
	{
		while(!_isDone)
		{
			// Wait for a task, pop it and execute it. Hence the double ()()
			_tasks.WaitAndPop()();
		}
	}
	catch(...)
	{
		_activeException = std::current_exception();
		_isDone = true;
	}

#ifdef __WIN32
	// ExitThread is required to prevent a deadlock with joining in the case, that main is returning at the same time
	// Remove when fixed in VC - not an issue in gcc / clang
	ExitThread(nullptr);
#endif
}

void CActive::Send(std::function<void()> callback, bool checkForException)
{
	if(checkForException)
	{
		CheckForAndThrowException();
	}

	_tasks.Push(callback);
}

void CActive::DoDone()
{
	_isDone = true;
}

void CActive::CheckForAndThrowException() const
{
	if(_isDone)
	{
		if (std::exception_ptr exception = _activeException)
		{
			_activeException = nullptr;
			std::rethrow_exception(exception);
		}
		else
		{
			throw CActiveException{SRC_POS, "Active object is already done."};
		}
	}
}
