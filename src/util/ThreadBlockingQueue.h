// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once 

#include <vector>
#include <mutex>
#include <thread>
#include <cstdint>
#include <stddef.h>
#include <condition_variable>
#include <typeinfo>
#include <iostream>

#define MIN_ITEM_NB (1)

//use this timeout constant in either pop() or push() calls to indicate 
// a non-blocking operation, so respectively equivalent to try_pop() and try_push() 
#define NON_BLOCKING_TIMEOUT (100)

//use this timeout constant in either pop() or push() calls to indicate 
//an indefnite timeout duration. 
#define BLOCKING_INFINITE_TIMEOUT (0)

class ThreadQueueBase {
};

/** A thread-safe asynchronous blocking queue */
template<typename T>
class ThreadBlockingQueue : public ThreadQueueBase {

    typedef typename std::vector<T>::value_type value_type;
    typedef typename std::vector<T>::size_type size_type;

public:

    /*! Create safe blocking queue. */
    ThreadBlockingQueue() {
        //at least 1 (== Exchanger)
		m_circular_buffer.resize(MIN_ITEM_NB + 1); //there is one slot more than the size for internal management.
    };
    
    //Copy constructor
    ThreadBlockingQueue(const ThreadBlockingQueue& sq) {
        std::lock_guard < std::mutex > lock(sq.m_mutex);
        m_circular_buffer = sq.m_circular_buffer;
		m_head = sq.m_head;
		m_tail = sq.m_tail;
    }

    /*! Destroy safe queue. */
    ~ThreadBlockingQueue() {
        std::lock_guard < std::mutex > lock(m_mutex);
    }

    /**
     * Sets the maximum number of items in the queue. Real value is clamped
     * to 1 on the lower bound. 
     * \param[in] nb max of items
     */
    void set_max_num_items(unsigned int max_num_items) {
        std::lock_guard < std::mutex > lock(m_mutex);

        if (max_num_items > (unsigned int)privateMaxNumElements()) {
            //Only raise the existing max size, never reduce it
            //for simplification sake at runtime.
			m_circular_buffer.resize(max_num_items + 1); // there is 1 extra allocated slot.
            //m_head and m_tail stays valid.
            m_cond_not_full.notify_all();
        }
    }

    /**
     * Pushes the item into the queue. If the queue is full, waits until room
     * is available, for at most timeout microseconds.
     * \param[in] item An item.
     * \param[in] timeout a max waiting timeout in microseconds for an item to be pushed. 
     * by default, = 0 means indefinite wait.
     * \param[in] errorMessage an error message written on std::cout in case of the timeout wait
     * \return true if an item was pushed into the queue, else a timeout has occured.
     */
    bool push(const value_type& item, std::uint64_t timeout = BLOCKING_INFINITE_TIMEOUT,const char* errorMessage = "") {
        std::unique_lock < std::mutex > lock(m_mutex);

        if (timeout == BLOCKING_INFINITE_TIMEOUT) {
            m_cond_not_full.wait(lock, [this]() // Lambda funct
            {
                return privateSize() < privateMaxNumElements();
            });
        } else if (timeout <= NON_BLOCKING_TIMEOUT && privateSize() >= privateMaxNumElements()) {
            // if the value is below a threshold, consider it is a try_push()
            return false;
        }
        else if (false == m_cond_not_full.wait_for(lock, std::chrono::microseconds(timeout),
           [this]() { return privateSize() < privateMaxNumElements(); })) {
            std::thread::id currentThreadId = std::this_thread::get_id();
            std::cout << "WARNING: Thread 0x" << std::hex << currentThreadId << std::dec <<
                " (" << currentThreadId << ") executing {" << typeid(*this).name() << "}.push() has failed with timeout > " <<
                (timeout * 0.001) << " ms, message: " << errorMessage << std::endl;
           return false;
        }

		//m_tail is already the next valid place an item can be put
		m_circular_buffer[m_tail] = item;
		m_tail = nextIndex(m_tail, (int)m_circular_buffer.size());
        
		m_cond_not_empty.notify_all();
        return true;
    }

    /**
    * Try to pushes the item into the queue, immediatly, without waiting. If the queue is full, the item
    * is not inserted and the function returns false. 
    * \param[in] item An item.
    */
    bool try_push(const value_type& item) {
        std::lock_guard < std::mutex > lock(m_mutex);

        if (privateSize() >= privateMaxNumElements()) {
            return false;
        }

		//m_tail is already the next valid place an item can be put
		m_circular_buffer[m_tail] = item;
		m_tail = nextIndex(m_tail, (int)m_circular_buffer.size());

        m_cond_not_empty.notify_all();
        return true;
    }

    /**
     * Pops item from the queue. If the queue is empty, blocks for timeout microseconds, or until item becomes available.
     * \param[in] timeout The number of microseconds to wait. O (default) means indefinite wait.
     * \param[in] errorMessage an error message written on std::cout in case of the timeout wait
     * \return true if get an item from the queue, false if no item is received before the timeout.
     */
    bool pop(value_type& item, std::uint64_t timeout = BLOCKING_INFINITE_TIMEOUT, const char* errorMessage = "") {
        std::unique_lock < std::mutex > lock(m_mutex);

        if (timeout == BLOCKING_INFINITE_TIMEOUT) {
            m_cond_not_empty.wait(lock, [this]() // Lambda funct
            {
                return privateSize() > 0;
            });
        } else if (timeout <= NON_BLOCKING_TIMEOUT && privateSize() == 0) {
            // if the value is below a threshold, consider it is try_pop()
            return false;
        }
        else if (false == m_cond_not_empty.wait_for(lock, std::chrono::microseconds(timeout),
            [this]() { return privateSize() > 0; })) {
            std::thread::id currentThreadId = std::this_thread::get_id();
            std::cout << "WARNING: Thread 0x" << std::hex << currentThreadId << std::dec <<
                " (" << currentThreadId << ") executing {" << typeid(*this).name() << "}.pop() has failed with timeout > " <<
                (timeout * 0.001) << " ms, message: " << errorMessage << std::endl;
            return false;
        }

        item = m_circular_buffer[m_head];
		m_head = nextIndex(m_head, (int)m_circular_buffer.size());
      
        m_cond_not_full.notify_all();
        return true;
    }

    /**
     *  Tries to pop item from the queue.
     * \param[out] item The item.
     * \return False is returned if no item is available.
     */
    bool try_pop(value_type& item) {
        std::lock_guard < std::mutex > lock(m_mutex);

        if (privateSize() == 0) {
            return false;
        }

		item = m_circular_buffer[m_head];
		m_head = nextIndex(m_head, (int)m_circular_buffer.size());

        m_cond_not_full.notify_all();
        return true;
    }


    /**
     *  Gets the number of items in the queue.
     * \return Number of items in the queue.
     */
    size_type size() const {
        std::lock_guard < std::mutex > lock(m_mutex);
        return privateSize();
    }

    /**
     *  Check if the queue is empty.
     * \return true if queue is empty.
     */
    bool empty() const {
        std::lock_guard < std::mutex > lock(m_mutex);
        return privateSize() == 0;
    }

    /**
     *  Check if the queue is full.
     * \return true if queue is full.
     */
    bool full() const {
        std::lock_guard < std::mutex > lock(m_mutex);
        return (privateSize() >= privateMaxNumElements());
    }

    /**
     *  Remove any items in the queue.
     */
    void flush() {
        std::lock_guard < std::mutex > lock(m_mutex);
		m_head = 0;
		m_tail = 0;

        m_cond_not_full.notify_all();
    }

    /**
     *  Swaps the contents.
     * \param[out] sq The ThreadBlockingQueue to swap with 'this'.
     */
    void swap(ThreadBlockingQueue& sq) {
        if (this != &sq) {
            std::lock_guard < std::mutex > lock1(m_mutex);
            std::lock_guard < std::mutex > lock2(sq.m_mutex);
            m_circular_buffer.swap(sq.m_circular_buffer);

            std::swap(m_head, sq.m_head);
			std::swap(m_tail, sq.m_tail);

            if (privateSize() > 0) {
                m_cond_not_empty.notify_all();
            }

            if (sq.privateSize() > 0) {
                sq.m_cond_not_empty.notify_all();
            }

            if (privateSize() < privateMaxNumElements()) {
                m_cond_not_full.notify_all();
            }

            if (sq.privateSize() < sq.privateMaxNumElements()) {
                sq.m_cond_not_full.notify_all();
            }
        }
    }

    /*! The copy assignment operator */
    ThreadBlockingQueue& operator=(const ThreadBlockingQueue& sq) {
        if (this != &sq) {
            std::lock_guard < std::mutex > lock1(m_mutex);
            std::lock_guard < std::mutex > lock2(sq.m_mutex);
  
			m_circular_buffer = sq.m_circular_buffer;

			m_head = sq.m_head;
			m_tail = sq.m_tail;

            if (privateSize() > 0) {
                m_cond_not_empty.notify_all();
            }

            if (privateSize() < privateMaxNumElements()) {
                m_cond_not_full.notify_all();
            }
        }
        return *this;
    }

private:
    /// use a circular buffer structure to prevent allocations / reallocations (fixed array + modulo)
    std::vector<T> m_circular_buffer;

	/**
	* The 'head' index of the element at the head of the deque, 'tail'
	* the next (valid !) index at which an element can be pushed.
	* m_head == m_tail means empty.
	*/
	int m_head = 0, m_tail = 0;

	//
	inline int nextIndex(int index, int modulus) const {
		return (index + 1 == modulus) ? 0 : index + 1;
	}

	//
	inline int privateSize() const {
		if (m_head <= m_tail) {
			return m_tail - m_head;
		}

		return (m_tail - m_head + (int)m_circular_buffer.size());
	}

	//
	inline int privateMaxNumElements() const {
		return (int)m_circular_buffer.size() - 1;
	}

    mutable std::mutex m_mutex;
    std::condition_variable m_cond_not_empty;
    std::condition_variable m_cond_not_full;
};

/*! Swaps the contents of two ThreadBlockingQueue objects. (external operator) */
template<typename T>
void swap(ThreadBlockingQueue<T>& q1, ThreadBlockingQueue<T>& q2) {
    q1.swap(q2);
}
