#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <condition_variable>
#include <queue>
#include <mutex>

template <typename DATATYPE>
class ThreadSafeQueue
{
private:
    std::queue<DATATYPE> m_Queue;
    mutable std::mutex m_Mutex;
    std::condition_variable m_Condition;
    bool b_ShuttingDown = false;

public:
    // Default Constructor
    ThreadSafeQueue() = default;
    // Destructor
    ~ThreadSafeQueue() {
        Shutdown();
    }

    // Disable Copy and Move, Assignment and Constructor
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue(ThreadSafeQueue&&) = delete; // Move Constructor
    ThreadSafeQueue& operator=(ThreadSafeQueue&&) = delete; // Move assignment

    // Utitlity Methods
    void push(DATATYPE value);

    bool try_pop(DATATYPE& value); // Non-blocking pop
    bool pop(DATATYPE& value);                    // Blocking pop
    // Pop with timeout
    template <class Rep, class Period>
    bool pop_for(DATATYPE& value, const std::chrono::duration<Rep, Period>& timeout);

    bool empty() const;
    std::size_t size() const;
    bool is_shutting_down() const;

    void Shutdown();
};


template <typename DATATYPE>
void ThreadSafeQueue<DATATYPE>::push(DATATYPE data) {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if(b_ShuttingDown)
        {
            return;
        }
        m_Queue.push(data);
    }
    m_Condition.notify_one();
}

// [NON BLOCKING]
template <typename DATATYPE>
bool ThreadSafeQueue<DATATYPE>::try_pop(DATATYPE& value) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if(m_Queue.empty() || b_ShuttingDown)
    {
        return false;
    }
    value = std::move(m_Queue.front());
    m_Queue.pop();
    return true;
}

// [BLOCKING]
template <typename DATATYPE>
bool ThreadSafeQueue<DATATYPE>::pop(DATATYPE& value) {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_Condition.wait(lock, [this] { return !m_Queue.empty() || b_ShuttingDown; });
    if(b_ShuttingDown)
    {
        return false;
    }
    value = std::move(m_Queue.front());
    m_Queue.pop();
    return true;
}

// [BLOCKING] with timer
template <typename DATATYPE>
template <class Rep, class Period>
bool ThreadSafeQueue<DATATYPE>::pop_for(DATATYPE& value, const std::chrono::duration<Rep, Period>& timeout) {
    std::unique_lock<std::mutex> lock(m_Mutex);
    if (!m_Condition.wait_for(lock, timeout, [this] { return !m_Queue.empty() || b_ShuttingDown; })) {
        // Timeout occurred
        return false;
    }
    if (b_ShuttingDown || m_Queue.empty()) {
        return false;
    }
    value = std::move(m_Queue.front());
    m_Queue.pop();
    return true;
}

template <typename DATATYPE>
bool ThreadSafeQueue<DATATYPE>::empty() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Queue.empty();
}

template <typename DATATYPE>
std::size_t ThreadSafeQueue<DATATYPE>::size() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Queue.size();
}

template <typename DATATYPE>
void ThreadSafeQueue<DATATYPE>::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        b_ShuttingDown = true;
    }

    m_Condition.notify_all();
}

template <typename DATATYPE>
bool ThreadSafeQueue<DATATYPE>::is_shutting_down() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return b_ShuttingDown;
}


#endif // THREAD_SAFE_QUEUE_HPP
