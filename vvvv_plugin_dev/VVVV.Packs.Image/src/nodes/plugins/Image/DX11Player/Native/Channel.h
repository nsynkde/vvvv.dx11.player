#pragma once

#include "blockingconcurrentqueue.h"

template<typename Data>
class Channel
{
private:
	moodycamel::BlockingConcurrentQueue<Data> m_queue;
public:
	Channel()
	{
	}

    bool send(const Data & data)
    {
        return m_queue.enqueue(data);
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
        if(empty())
        {
            return false;
        }
        
        return m_queue.try_dequeue(out_value);
    }

    bool recv(Data& popped_value)
    {
		return m_queue.wait_dequeue(popped_value);
    }

	void close()
	{
		m_queue.close();
	}
};