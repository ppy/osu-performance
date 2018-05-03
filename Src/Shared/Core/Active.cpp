#include <Shared.h>

#include "Active.h"

CActive::~CActive()
{
	if (!_isDone)
	{
		send(std::bind(&CActive::doDone, this), false);
		if (_thread.joinable())
			_thread.join();
	}
}

void CActive::Send(std::function<void()> callback)
{
	send(callback, true);
}

std::unique_ptr<CActive> CActive::Create()
{
	auto pActive = std::unique_ptr<CActive>{ new CActive{} };
	pActive->_thread = std::thread(&CActive::run, pActive.get());
	return pActive;
}

size_t CActive::NumPending() const
{
	checkForAndThrowException();
	return _tasks.Size();
}

bool CActive::IsBusy() const
{
	return NumPending() > 0;
}

CActive::CActive()
{
	// This is required here, because atomic doesn't allow
	// copy construction.
	_isDone = false;
}

void CActive::run()
{
	try
	{
		while (!_isDone)
			// Wait for a task, pop it and execute it. Hence the double ()()
			_tasks.WaitAndPop()();
	}
	catch (...)
	{
		_activeException = std::current_exception();
		_isDone = true;
	}
}

void CActive::send(std::function<void()> callback, bool checkForException)
{
	if (checkForException)
		checkForAndThrowException();

	_tasks.Push(callback);
}

void CActive::doDone()
{
	_isDone = true;
}

void CActive::checkForAndThrowException() const
{
	if (_isDone)
	{
		if (std::exception_ptr exception = _activeException)
		{
			_activeException = nullptr;
			std::rethrow_exception(exception);
		}
		else
			throw CActiveException{SRC_POS, "Active object is already done."};
	}
}
