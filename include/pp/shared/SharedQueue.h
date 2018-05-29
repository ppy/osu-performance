#pragma once

#include <pp/Common.h>

#include <deque>
#include <mutex>
#include <condition_variable>

PP_NAMESPACE_BEGIN

template <typename T>
class SharedQueue
{
public:
	bool Empty() const
	{
		std::lock_guard<std::mutex> lock{ _mutex };
		return _rawQueue.empty();
	}

	size_t Size() const
	{
		std::lock_guard<std::mutex> lock{ _mutex };
		return _rawQueue.size();
	}

	void Push(T& NewElem)
	{
		std::lock_guard<std::mutex> lock{ _mutex };
		_rawQueue.push_back(NewElem);
		_dataCondition.notify_one();
	}

	T WaitAndPop()
	{
		std::unique_lock<std::mutex> lock{ _mutex };

		while (_rawQueue.empty())
		{
			_dataCondition.wait(lock);
		}

		T result = std::move(_rawQueue.front());
		_rawQueue.pop_front();

		return result;
	}

private:
	std::deque<T> _rawQueue;
	mutable std::mutex _mutex;
	std::condition_variable _dataCondition;
};

PP_NAMESPACE_END
