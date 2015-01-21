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
public:
    void send(const Data & data)
    {
		std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(data);
		lock.unlock();
        m_condition.notify_one();
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
        if(m_queue.empty())
        {
            return false;
        }
        
        out_value=m_queue.front();
        m_queue.pop();
        return true;
    }

    void recv(Data& popped_value)
    {
		std::unique_lock<std::mutex> lock(m_mutex);
        while(m_queue.empty())
        {
            m_condition.wait(lock);
        }
        
        popped_value=m_queue.front();
        m_queue.pop();
    }

};