#pragma once

#include <mutex>
#include <atomic>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <iostream>

#include "ThreadQueue.h"
#include "Timer.h"

struct map_string_less : public std::binary_function<std::string,std::string,bool>
{
    bool operator()(const std::string& a,const std::string& b) const
    {
        return a.compare(b) < 0;
    }
};


class ReferenceCounter {

public:
    
    void setIndex(int idx) {
        std::lock_guard < std::recursive_mutex > lock(m_mutex);
        index = idx;
    }

    int getIndex() {
        std::lock_guard < std::recursive_mutex > lock(m_mutex);
        return index;
    }

    void setRefCount(int rc) {
        std::lock_guard < std::recursive_mutex > lock(m_mutex);
        refCount = rc;
    }
    
    void decRefCount() {
        std::lock_guard < std::recursive_mutex > lock(m_mutex);
        refCount--;
    }
    
    int getRefCount() {
        std::lock_guard < std::recursive_mutex > lock(m_mutex);
        return refCount;
    }

    // Access to the own mutex protecting the ReferenceCounter, i.e the monitor of the class
     std::recursive_mutex& getMonitor() const {
        return m_mutex;
    }

protected:
    //this is a basic mutex for all ReferenceCounter derivatives operations INCLUDING the counter itself for consistency !
   mutable std::recursive_mutex m_mutex;

private:
   int index, refCount;
};


#define REBUFFER_GC_LIMIT 100

class ReBufferGC {
public:
    static void garbageCollect() {
        std::lock_guard < std::mutex > lock(g_mutex);
        
        std::deque<ReferenceCounter *> garbageRemoval;
        for (typename std::set<ReferenceCounter *>::iterator i = garbage.begin(); i != garbage.end(); i++) {
            if ((*i)->getRefCount() <= 0) {
                garbageRemoval.push_back(*i);
            }
            else {
                std::cout << "Garbage in queue buffer idx #" << (*i)->getIndex() << ", " << (*i)->getRefCount() << " usage(s)" << std::endl;
            }
        }
        if ( garbageRemoval.size() ) {
            std::cout << "Garbage collecting " << garbageRemoval.size() << " ReBuffer(s)" << std::endl;
            while (!garbageRemoval.empty()) {
                ReferenceCounter *ref = garbageRemoval.back();
                garbageRemoval.pop_back();
                garbage.erase(ref);
                delete ref;
            }
        }
    }
    
    static void addGarbage(ReferenceCounter *ref) {
        std::lock_guard < std::mutex > lock(g_mutex);
        garbage.insert(ref);
    }
    
private:
    static std::mutex g_mutex;
    static std::set<ReferenceCounter *> garbage;
};


template<class BufferType = ReferenceCounter>
class ReBuffer {
    
public:
    ReBuffer(std::string bufferId) : bufferId(bufferId) {
        indexCounter.store(0);
    }
    
    BufferType *getBuffer() {
        std::lock_guard < std::mutex > lock(m_mutex);

        BufferType* buf = nullptr;
        for (outputBuffersI = outputBuffers.begin(); outputBuffersI != outputBuffers.end(); outputBuffersI++) {
            if (buf == nullptr && (*outputBuffersI)->getRefCount() <= 0) {
                buf = (*outputBuffersI);
                buf->setRefCount(1);
            } else if ((*outputBuffersI)->getRefCount() <= 0) {
                (*outputBuffersI)->decRefCount();
            }
        }
        
        if (buf != nullptr) {
            if (outputBuffers.back()->getRefCount() < -REBUFFER_GC_LIMIT) {
                BufferType *ref = outputBuffers.back();
                outputBuffers.pop_back();
                delete ref;
            }
            buf->setIndex(indexCounter++);
            return buf;
        }
        
#define REBUFFER_WARNING_THRESHOLD 100
        if (outputBuffers.size() > REBUFFER_WARNING_THRESHOLD) {
            std::cout << "Warning: ReBuffer '" << bufferId << "' count '" << outputBuffers.size() << "' exceeds threshold of '" << REBUFFER_WARNING_THRESHOLD << "'" << std::endl;
        }

        buf = new BufferType();
        buf->setRefCount(1);
        buf->setIndex(indexCounter++);
        outputBuffers.push_back(buf);
        
        return buf;
    }
    
    void purge() {
        std::lock_guard < std::mutex > lock(m_mutex);
//        if (bufferId == "DemodulatorThreadBuffers") {
//            std::cout << "'" << bufferId << "' purging.. total indexes: " << indexCounter.load() << std::endl;
//        }
        while (!outputBuffers.empty()) {
            BufferType *ref = outputBuffers.front();
            outputBuffers.pop_front();
            if (ref->getRefCount() <= 0) {
                delete ref;
            } else {
                // Something isn't done with it yet; throw it on the pile.. keep this as a bug indicator for now..
                std::cout << "'" << bufferId << "' pushed garbage.." << std::endl;
                ReBufferGC::addGarbage(ref);
            }
        }
    }

   private:
    std::string bufferId;
    std::deque<BufferType*> outputBuffers;
    typename std::deque<BufferType*>::iterator outputBuffersI;
    mutable std::mutex m_mutex;
    std::atomic_int indexCounter;
};


class IOThread {
public:
    IOThread();
    virtual ~IOThread();

    static void *pthread_helper(void *context);

#ifdef __APPLE__
    virtual void *threadMain();
#else
    virtual void threadMain();
#endif

    virtual void setup();
    virtual void run();
    virtual void terminate();
    bool isTerminated();
    virtual void onBindOutput(std::string name, ThreadQueueBase* threadQueue);
    virtual void onBindInput(std::string name, ThreadQueueBase* threadQueue);

    void setInputQueue(std::string qname, ThreadQueueBase *threadQueue);
    ThreadQueueBase *getInputQueue(std::string qname);
    void setOutputQueue(std::string qname, ThreadQueueBase *threadQueue);
    ThreadQueueBase *getOutputQueue(std::string qname);
    
protected:
    std::map<std::string, ThreadQueueBase *, map_string_less> input_queues;
    std::map<std::string, ThreadQueueBase *, map_string_less> output_queues;
    std::atomic_bool terminated;
    Timer gTimer;
};
