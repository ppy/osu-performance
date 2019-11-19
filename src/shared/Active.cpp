#include <pp/Common.h>
#include <pp/shared/Active.h>

PP_NAMESPACE_BEGIN

Active::~Active()
{
	if (!_isDone)
		send(std::bind(&Active::doDone, this), false);

	if (_thread.joinable())
		_thread.join();
}

void Active::Send(std::function<void()> callback)
{
	send(callback, true);
}

std::unique_ptr<Active> Active::Create()
{
	auto pActive = std::unique_ptr<Active>{ new Active{} };
	pActive->_thread = std::thread(&Active::run, pActive.get());
	return pActive;
}

size_t Active::NumPending() const
{
	checkForAndThrowException();
	return _tasks.Size();
}

bool Active::IsBusy() const
{
	return NumPending() > 0;
}

Active::Active()
{
	// This is required here, because atomic doesn't allow
	// copy construction.
	_isDone = false;
}

void Active::run()
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

void Active::send(std::function<void()> callback, bool checkForException)
{
	if (checkForException)
		checkForAndThrowException();

	_tasks.Push(callback);
}

void Active::doDone()
{
	_isDone = true;
}

void Active::checkForAndThrowException() const
{
	if (_isDone)
	{
		if (std::exception_ptr exception = _activeException)
		{
			_activeException = nullptr;
			std::rethrow_exception(exception);
		}
		else
			throw ActiveException{SRC_POS, "Active object is already done."};
	}
}

PP_NAMESPACE_END
