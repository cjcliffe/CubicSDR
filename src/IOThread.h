// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <mutex>
#include <atomic>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <thread>
#include <memory>
#include <climits>
#include "ThreadBlockingQueue.h"
#include "Timer.h"

struct map_string_less : public std::binary_function<std::string,std::string,bool>
{
    bool operator()(const std::string& a,const std::string& b) const
    {
        return a.compare(b) < 0;
    }
};

template <typename PtrType>
class ReBufferAge {
public:
    PtrType ptr;
    int age;

    virtual ~ReBufferAge() {};
};

#define REBUFFER_GC_LIMIT 100
#define REBUFFER_WARNING_THRESHOLD 150

template<typename BufferType>
class ReBuffer {
    
    typedef typename std::shared_ptr<BufferType> ReBufferPtr;
   
public:
    ReBuffer(std::string bufferId) : bufferId(bufferId) {
    }
    
    /// Return a new ReBuffer_ptr usable by the application.
    ReBufferPtr getBuffer() {

        std::lock_guard < std::mutex > lock(m_mutex);

        // iterate the ReBuffer_ptr list: if the std::shared_ptr count == 1, it means 
        //it is only referenced in outputBuffers itself, so available for re-use.
        //else if the std::shared_ptr count <= 1, make it age.
        //else the ReBuffer_ptr is in use, don't use it.

        ReBufferPtr buf = nullptr;

        outputBuffersI it = outputBuffers.begin();

       while (it != outputBuffers.end()) {

           long use = it->ptr.use_count();
            //1. If we encounter a shared_ptr with a use count of 0, this
            //is a bug since it is supposed to be at least 1, because it is referenced here.
            //in this case, purge it from here and trace.
            if (use == 0) {
                it = outputBuffers.erase(it);
                std::cout << "Warning: in ReBuffer '" << bufferId << "' count '" << outputBuffers.size() << "', found 1 dangling buffer !" << std::endl << std::flush;
            } 
            else if (use == 1) {
                if (buf == nullptr) {
                    it->age = 1;  //select this one.
                    buf = it->ptr;
                    //std::cout << "**" << std::flush;
                    it++;
                }
                else {
                    //make the other unused buffers age
                    it->age--;
                    it++;
                }
            }
            else {
                it++;
            }
        } //end while

       //2.1 Garbage collect the oldest (last element) if it aged too much, and return the buffer
       if (buf != nullptr) {
            
           if (outputBuffers.back().age < -REBUFFER_GC_LIMIT) {
                //by the nature of the shared_ptr, memory will ne deallocated automatically.           
                outputBuffers.pop_back();
                //std::cout << "--" << std::flush;
            }
            return buf;
        }

        if (outputBuffers.size() > REBUFFER_WARNING_THRESHOLD) {
            std::cout << "Warning: ReBuffer '" << bufferId << "' count '" << outputBuffers.size() << "' exceeds threshold of '" << REBUFFER_WARNING_THRESHOLD << "'" << std::endl;
        }
        
        //3.We need to allocate a new buffer. 
        ReBufferAge < ReBufferPtr > newBuffer;

        //careful here: newBuffer.ptr is already constructed, so we need to set "in place" its
        //ownership to a (new BufferType()).
        newBuffer.ptr.reset(new BufferType());
        newBuffer.age = 1;

        outputBuffers.push_back(newBuffer);

        //std::cout << "++" << std::flush;
        
        return newBuffer.ptr;
    }
    
    /// Purge the cache.
    void purge() {
        std::lock_guard < std::mutex > lock(m_mutex);

        // since outputBuffers are full std::shared_ptr,
        //purging if will effectively loose the local reference,
        // so the std::shared_ptr will naturally be deallocated
        //when their time comes.  
        outputBuffers.clear();
    }

private:

    //name of the buffer cache kind
    std::string bufferId;
    
    //the ReBuffer cache
    std::vector< ReBufferAge < ReBufferPtr > > outputBuffers;

    typedef typename std::vector< ReBufferAge < ReBufferPtr > >::iterator outputBuffersI;

    //mutex protecting access to outputBuffers.
    mutable std::mutex m_mutex;
};


class IOThread {
public:
    IOThread();
    virtual ~IOThread();

    static void *pthread_helper(void *context);

#ifdef __APPLE__
    virtual void *threadMain();
#else

    //the thread Main call back itself
    virtual void threadMain();
#endif

    virtual void setup();
    virtual void run();

    //Request for termination (asynchronous)
    virtual void terminate();

    //Returns true if the thread is indeed terminated, i.e the run() method
    //has returned. 
    //If wait > 0 ms, the call is blocking at most 'waitMs' milliseconds for the thread to die, then returns.
    //If wait < 0, the wait in infinite until the thread dies.
    bool isTerminated(int waitMs = 0);
    
    virtual void onBindOutput(std::string name, ThreadQueueBase* threadQueue);
    virtual void onBindInput(std::string name, ThreadQueueBase* threadQueue);

    void setInputQueue(std::string qname, ThreadQueueBase *threadQueue);
    ThreadQueueBase *getInputQueue(std::string qname);
    void setOutputQueue(std::string qname, ThreadQueueBase *threadQueue);
    ThreadQueueBase *getOutputQueue(std::string qname);
    
protected:
    std::map<std::string, ThreadQueueBase *, map_string_less> input_queues;
    std::map<std::string, ThreadQueueBase *, map_string_less> output_queues;

    //this protects against concurrent changes in input/output bindings: get/set/Input/OutPutQueue
    mutable std::mutex m_queue_bindings_mutex;

    //true when a termination is ordered
    std::atomic_bool stopping;
    Timer gTimer;

private:
    //true when the thread has really ended, i.e run() from threadMain() has returned.
    std::atomic_bool terminated;

   
};
