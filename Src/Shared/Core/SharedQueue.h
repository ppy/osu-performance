#pragma once


#include <deque>
#include <mutex>
#include <condition_variable>


template <typename T>
class CSharedQueue
{
public:

	bool Empty() const
	{
		std::lock_guard<std::mutex> lock{ m_Mutex };
		return m_RawQueue.empty();
	}

	size_t Size() const
	{
		std::lock_guard<std::mutex> lock{ m_Mutex };
		return m_RawQueue.size();
	}

	void Push(T& NewElem)
	{
		std::lock_guard<std::mutex> lock{ m_Mutex };
		m_RawQueue.push_back(NewElem);
		m_DataCondition.notify_one();
	}

	T WaitAndPop()
	{
		std::unique_lock<std::mutex> lock{ m_Mutex };

		while (m_RawQueue.empty())
		{
			m_DataCondition.wait(lock);
		}

		T Result = std::move(m_RawQueue.front());
		m_RawQueue.pop_front();

		return Result;
	}



private:

	std::deque<T> m_RawQueue;
	mutable std::mutex m_Mutex;
	std::condition_variable m_DataCondition;
};
