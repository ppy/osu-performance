#pragma once

#include "SharedQueue.h"

DEFINE_LOGGED_EXCEPTION(CActiveException);

class CActive
{
public:
	CActive(const CActive&) = delete;
	CActive& operator=(const CActive&) = delete;

	virtual ~CActive();

	void Send(std::function<void()> callback);
	static std::unique_ptr<CActive> Create();

	size_t NumPending() const;
	bool IsBusy() const;

private:
	// See Create
	CActive();
	void run();

	void send(std::function<void()> callback, bool checkForException);

	CSharedQueue<std::function<void()>> _tasks;

	std::thread _thread;
	std::atomic<bool> _isDone;

	mutable std::exception_ptr _activeException = nullptr;

	void doDone();
	void checkForAndThrowException() const;
};
