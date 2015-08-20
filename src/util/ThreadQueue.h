#pragma once 

/* Credit to Alfredo Pons / https://plus.google.com/109903449837592676231
 * Code from http://gnodebian.blogspot.com.es/2013/07/a-thread-safe-asynchronous-queue-in-c11.html
 *
 * Changes:
 *   Charles J. Nov-19-2014
 *     - Renamed SafeQueue -> ThreadQueue 
 */

#include <queue>
#include <list>
#include <mutex>
#include <thread>
#include <cstdint>
#include <condition_variable>

class ThreadQueueBase {
    
};

/** A thread-safe asynchronous queue */
template<class T, class Container = std::list<T>>
class ThreadQueue : public ThreadQueueBase {

    typedef typename Container::value_type value_type;
    typedef typename Container::size_type size_type;
    typedef Container container_type;

public:

    /*! Create safe queue. */
    ThreadQueue() = default;
    ThreadQueue(ThreadQueue&& sq) {
        m_queue = std::move(sq.m_queue);
    }
    ThreadQueue(const ThreadQueue& sq) {
        std::lock_guard < std::mutex > lock(sq.m_mutex);
        m_queue = sq.m_queue;
    }

    /*! Destroy safe queue. */
    ~ThreadQueue() {
        std::lock_guard < std::mutex > lock(m_mutex);
    }

    /**
     * Sets the maximum number of items in the queue. Defaults is 0: No limit
     * \param[in] item An item.
     */
    void set_max_num_items(unsigned int max_num_items) {
        std::lock_guard < std::mutex > lock(m_mutex);

        m_max_num_items = max_num_items;
    }

    /**
     *  Pushes the item into the queue.
     * \param[in] item An item.
     * \return true if an item was pushed into the queue
     */
    bool push(const value_type& item) {
        std::lock_guard < std::mutex > lock(m_mutex);

        if (m_max_num_items > 0 && m_queue.size() > m_max_num_items)
            return false;

        m_queue.push(item);
        m_condition.notify_one();
        return true;
    }

    /**
     *  Pushes the item into the queue.
     * \param[in] item An item.
     * \return true if an item was pushed into the queue
     */
    bool push(const value_type&& item) {
        std::lock_guard < std::mutex > lock(m_mutex);

        if (m_max_num_items > 0 && m_queue.size() > m_max_num_items)
            return false;

        m_queue.push(item);
        m_condition.notify_one();
        return true;
    }

    /**
     *  Pops item from the queue. If queue is empty, this function blocks until item becomes available.
     * \param[out] item The item.
     */
    void pop(value_type& item) {
        std::unique_lock < std::mutex > lock(m_mutex);
        m_condition.wait(lock, [this]() // Lambda funct
                {
                    return !m_queue.empty();
                });
        item = m_queue.front();
        m_queue.pop();
    }

    /**
     *  Pops item from the queue using the contained type's move assignment operator, if it has one..
     *  This method is identical to the pop() method if that type has no move assignment operator.
     *  If queue is empty, this function blocks until item becomes available.
     * \param[out] item The item.
     */
    void move_pop(value_type& item) {
        std::unique_lock < std::mutex > lock(m_mutex);
        m_condition.wait(lock, [this]() // Lambda funct
                {
                    return !m_queue.empty();
                });
        item = std::move(m_queue.front());
        m_queue.pop();
    }

    /**
     *  Tries to pop item from the queue.
     * \param[out] item The item.
     * \return False is returned if no item is available.
     */
    bool try_pop(value_type& item) {
        std::unique_lock < std::mutex > lock(m_mutex);

        if (m_queue.empty())
            return false;

        item = m_queue.front();
        m_queue.pop();
        return true;
    }

    /**
     *  Tries to pop item from the queue using the contained type's move assignment operator, if it has one..
     *  This method is identical to the try_pop() method if that type has no move assignment operator.
     * \param[out] item The item.
     * \return False is returned if no item is available.
     */
    bool try_move_pop(value_type& item) {
        std::unique_lock < std::mutex > lock(m_mutex);

        if (m_queue.empty())
            return false;

        item = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    /**
     *  Pops item from the queue. If the queue is empty, blocks for timeout microseconds, or until item becomes available.
     * \param[out] t An item.
     * \param[in] timeout The number of microseconds to wait.
     * \return true if get an item from the queue, false if no item is received before the timeout.
     */
    bool timeout_pop(value_type& item, std::uint64_t timeout) {
        std::unique_lock < std::mutex > lock(m_mutex);

        if (m_queue.empty()) {
            if (timeout == 0)
                return false;

            if (m_condition.wait_for(lock, std::chrono::microseconds(timeout)) == std::cv_status::timeout)
                return false;
        }

        item = m_queue.front();
        m_queue.pop();
        return true;
    }

    /**
     *  Pops item from the queue using the contained type's move assignment operator, if it has one..
     *  If the queue is empty, blocks for timeout microseconds, or until item becomes available.
     *  This method is identical to the try_pop() method if that type has no move assignment operator.
     * \param[out] t An item.
     * \param[in] timeout The number of microseconds to wait.
     * \return true if get an item from the queue, false if no item is received before the timeout.
     */
    bool timeout_move_pop(value_type& item, std::uint64_t timeout) {
        std::unique_lock < std::mutex > lock(m_mutex);

        if (m_queue.empty()) {
            if (timeout == 0)
                return false;

            if (m_condition.wait_for(lock, std::chrono::microseconds(timeout)) == std::cv_status::timeout)
                return false;
        }

        item = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    /**
     *  Gets the number of items in the queue.
     * \return Number of items in the queue.
     */
    size_type size() const {
        std::lock_guard < std::mutex > lock(m_mutex);
        return m_queue.size();
    }

    /**
     *  Check if the queue is empty.
     * \return true if queue is empty.
     */
    bool empty() const {
        std::lock_guard < std::mutex > lock(m_mutex);
        return m_queue.empty();
    }

    /**
     *  Check if the queue is full.
     * \return true if queue is full.
     */
    bool full() const {
        std::lock_guard < std::mutex > lock(m_mutex);
        return (m_max_num_items != 0) && (m_queue.size() >= m_max_num_items);
    }

    /**
     *  Remove any items in the queue.
     */
    void flush() const {
        std::lock_guard < std::mutex > lock(m_mutex);
        std::queue<T, Container> emptyQueue;
        std::swap(m_queue, emptyQueue);
    }

    /**
     *  Swaps the contents.
     * \param[out] sq The ThreadQueue to swap with 'this'.
     */
    void swap(ThreadQueue& sq) {
        if (this != &sq) {
            std::lock_guard < std::mutex > lock1(m_mutex);
            std::lock_guard < std::mutex > lock2(sq.m_mutex);
            m_queue.swap(sq.m_queue);

            if (!m_queue.empty())
                m_condition.notify_all();

            if (!sq.m_queue.empty())
                sq.m_condition.notify_all();
        }
    }

    /*! The copy assignment operator */
    ThreadQueue& operator=(const ThreadQueue& sq) {
        if (this != &sq) {
            std::lock_guard < std::mutex > lock1(m_mutex);
            std::lock_guard < std::mutex > lock2(sq.m_mutex);
            std::queue<T, Container> temp { sq.m_queue };
            m_queue.swap(temp);

            if (!m_queue.empty())
                m_condition.notify_all();
        }

        return *this;
    }

    /*! The move assignment operator */
    ThreadQueue& operator=(ThreadQueue && sq) {
        std::lock_guard < std::mutex > lock(m_mutex);
        m_queue = std::move(sq.m_queue);

        if (!m_queue.empty())
            m_condition.notify_all();

        return *this;
    }

private:

    std::queue<T, Container> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    unsigned int m_max_num_items = 0;
};

/*! Swaps the contents of two ThreadQueue objects. */
template<class T, class Container>
void swap(ThreadQueue<T, Container>& q1, ThreadQueue<T, Container>& q2) {
    q1.swap(q2);
}
