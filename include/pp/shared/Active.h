#pragma once

#include <pp/Common.h>
#include <pp/shared/SharedQueue.h>

#include <atomic>
#include <thread>

PP_NAMESPACE_BEGIN

DEFINE_LOGGED_EXCEPTION(ActiveException);

class Active
{
public:
	Active(const Active&) = delete;
	Active& operator=(const Active&) = delete;

	virtual ~Active();

	void Send(std::function<void()> callback);
	static std::unique_ptr<Active> Create();

	size_t NumPending() const;
	bool IsBusy() const;

private:
	// See Create
	Active();
	void run();

	void send(std::function<void()> callback, bool checkForException);

	SharedQueue<std::function<void()>> _tasks;

	std::thread _thread;
	std::atomic<bool> _isDone;

	mutable std::exception_ptr _activeException = nullptr;

	void doDone();
	void checkForAndThrowException() const;
};

PP_NAMESPACE_END
