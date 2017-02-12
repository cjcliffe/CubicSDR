// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once 

/* Credit to Alfredo Pons / https://plus.google.com/109903449837592676231
 * Code from http://gnodebian.blogspot.com.es/2013/07/a-thread-safe-asynchronous-queue-in-c11.html
 *
 * Changes:
 *   Charles J. Nov-19-2014
 *     - Renamed SafeQueue -> ThreadQueue 
 *   Sonnier.V Feb-10-2017
 *     - Simplified, various fixes
 */

#include <deque>
#include <list>
#include <mutex>
#include <thread>
#include <cstdint>
#include <condition_variable>

class ThreadQueueBase {   
};

/** A thread-safe asynchronous queue */
template<typename T>
class ThreadQueue : public ThreadQueueBase {

    typedef typename std::deque<T>::value_type value_type;
    typedef typename std::deque<T>::size_type size_type;

public:

    /*! Create safe queue. */
    ThreadQueue() {
        m_max_num_items = 0;
    };
    ThreadQueue(ThreadQueue&& sq) {
        m_queue = std::move(sq.m_queue);
        m_max_num_items = sq.m_max_num_items;
    }
    ThreadQueue(const ThreadQueue& sq) {
        std::lock_guard < std::mutex > lock(sq.m_mutex);
        m_queue = sq.m_queue;
        m_max_num_items = sq.m_max_num_items;
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

        if (m_max_num_items > 0 && m_queue.size() > m_max_num_items) {
            return false;
        }

        m_queue.push_back(item);
        m_cond_not_empty.notify_all();
        return true;
    }

    /**
     *  Pushes the item into the queue.
     * \param[in] item An item.
     * \return true if an item was pushed into the queue
     */
    bool push(const value_type&& item) {
        std::lock_guard < std::mutex > lock(m_mutex);

        if (m_max_num_items > 0 && m_queue.size() > m_max_num_items) {
            return false;
        }

        m_queue.push_back(item);
        m_cond_not_empty.notify_all();
        return true;
    }

    /**
     *  Pops item from the queue. If queue is empty, this function blocks until item becomes available.
     * \param[out] item The item.
     */
    void pop(value_type& item) {
        std::unique_lock < std::mutex > lock(m_mutex);
        m_cond_not_empty.wait(lock, [this]() // Lambda funct
                {
                    return !m_queue.empty();
                });
        item = m_queue.front();
        m_queue.pop_front();
    }

    /**
     *  Pops item from the queue using the contained type's move assignment operator, if it has one..
     *  This method is identical to the pop() method if that type has no move assignment operator.
     *  If queue is empty, this function blocks until item becomes available.
     * \param[out] item The item.
     */
    void move_pop(value_type& item) {
        std::unique_lock < std::mutex > lock(m_mutex);
        m_cond_not_empty.wait(lock, [this]() // Lambda funct
                {
                    return !m_queue.empty();
                });
        item = std::move(m_queue.front());
        m_queue.pop_front();
    }

    /**
     *  Tries to pop item from the queue.
     * \param[out] item The item.
     * \return False is returned if no item is available.
     */
    bool try_pop(value_type& item) {
        std::lock_guard < std::mutex > lock(m_mutex);

        if (m_queue.empty())
            return false;

        item = m_queue.front();
        m_queue.pop_front();
        return true;
    }

    /**
     *  Tries to pop item from the queue using the contained type's move assignment operator, if it has one..
     *  This method is identical to the try_pop() method if that type has no move assignment operator.
     * \param[out] item The item.
     * \return False is returned if no item is available.
     */
    bool try_move_pop(value_type& item) {
        std::lock_guard < std::mutex > lock(m_mutex);

        if (m_queue.empty())
            return false;

        item = std::move(m_queue.front());
        m_queue.pop_front();
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

            if (m_cond_not_empty.wait_for(lock, std::chrono::microseconds(timeout)) == std::cv_status::timeout)
                return false;
        }

        item = m_queue.front();
        m_queue.pop_front();
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

            if (m_cond_not_empty.wait_for(lock, std::chrono::microseconds(timeout)) == std::cv_status::timeout)
                return false;
        }

        item = std::move(m_queue.front());
        m_queue.pop_front();
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
    void flush() {
        std::lock_guard < std::mutex > lock(m_mutex);
        m_queue.clear();
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
            std::swap(m_max_num_items, sq.m_max_num_items);

            if (!m_queue.empty())
                m_cond_not_empty.notify_all();
   

            if (!sq.m_queue.empty())
                sq.m_cond_not_empty.notify_all();
        }
    }

    /*! The copy assignment operator */
    ThreadQueue& operator=(const ThreadQueue& sq) {
        if (this != &sq) {
            std::lock_guard < std::mutex > lock1(m_mutex);
            std::lock_guard < std::mutex > lock2(sq.m_mutex);
   
            m_queue = sq.m_queue;
            m_max_num_items = sq.m_max_num_items;

            if (!m_queue.empty()) 
                m_cond_not_empty.notify_all();
        }

        return *this;
    }

    /*! The move assignment operator */
    ThreadQueue& operator=(ThreadQueue && sq) {
        std::lock_guard < std::mutex > lock(m_mutex);
        m_queue = std::move(sq.m_queue);
        m_max_num_items = sq.m_max_num_items;

        if (!m_queue.empty()) 
            m_cond_not_empty.notify_all();
      
        return *this;
    }

private:

    std::deque<T> m_queue;

    mutable std::mutex m_mutex;
    std::condition_variable m_cond_not_empty;
    size_t m_max_num_items;
};

/*! Swaps the contents of two ThreadQueue objects. */
template<typename T>
void swap(ThreadQueue<T>& q1, ThreadQueue<T>& q2) {
    q1.swap(q2);
}
