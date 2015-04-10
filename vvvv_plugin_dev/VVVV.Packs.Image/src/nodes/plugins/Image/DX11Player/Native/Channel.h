#pragma once

#include "blockingconcurrentqueue.h"

template<typename Data>
class Channel
{
private:
	moodycamel::BlockingConcurrentQueue<Data> m_queue;
	bool closed;
public:
	Channel()
	:closed(false)
	{
	}

    bool send(const Data & data)
    {
		if(closed)
		{
			return false;
		}
        m_queue.enqueue(data);
		return true;
    }

    bool empty() const
    {
        return m_queue.size_approx()==0;
    }

    size_t size() const
    {
		return  m_queue.size_approx();
    }

    bool try_recv(Data& out_value)
    {
        if(empty() || closed)
        {
            return false;
        }
        
        return m_queue.try_dequeue(out_value);
    }

    bool recv(Data& popped_value)
    {
        if(closed)
		{
			return false;
		}
		m_queue.wait_dequeue(popped_value);
		return true;
    }

	void close()
	{
		closed = true;
		Data data;
		while (m_queue.try_dequeue(data));
	}
};