#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename Data>
class Channel
{
private:
    std::queue<Data> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
	bool closed;
public:
	Channel()
	:closed(false)
	{
	}

    bool send(const Data & data)
    {
		std::lock_guard<std::mutex> lock(m_mutex);
		if(closed)
		{
			return false;
		}
        m_queue.push(data);
        m_condition.notify_one();
		return true;
    }

    bool empty() const
    {
		std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    size_t size() const
    {
		std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    bool try_recv(Data& out_value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_queue.empty() || closed)
        {
            return false;
        }
        
        out_value=m_queue.front();
        m_queue.pop();
        return true;
    }

    bool recv(Data& popped_value)
    {
		std::unique_lock<std::mutex> lock(m_mutex);
        if(closed)
		{
			return false;
		}
        while(m_queue.empty())
        {
            m_condition.wait(lock);
			if(closed)
			{
				return false;
			}
        }
        popped_value=m_queue.front();
        m_queue.pop();
		return true;
    }

	void close()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		closed = true;
		while(!m_queue.empty()) m_queue.pop();
        m_condition.notify_one();
	}
};