#pragma once

#include <mutex>
#include <atomic>
#include <deque>
#include <map>
#include <string>

#include "ThreadQueue.h"

struct map_string_less : public std::binary_function<std::string,std::string,bool>
{
    bool operator()(const std::string& a,const std::string& b) const
    {
        return a.compare(b) < 0;
    }
};


class ReferenceCounter {
public:
    mutable std::mutex m_mutex;
    
    void setRefCount(int rc) {
        refCount.store(rc);
    }
    
    void decRefCount() {
        refCount.store(refCount.load()-1);
    }
    
    int getRefCount() {
        return refCount.load();
    }
protected:
    std::atomic_int refCount;
};


#define REBUFFER_GC_LIMIT 100

template<class BufferType = ReferenceCounter>
class ReBuffer {
    
public:
    BufferType *getBuffer() {
        BufferType* buf = NULL;
        for (outputBuffersI = outputBuffers.begin(); outputBuffersI != outputBuffers.end(); outputBuffersI++) {
            if (!buf && (*outputBuffersI)->getRefCount() <= 0) {
                buf = (*outputBuffersI);
                (*outputBuffersI)->setRefCount(0);
            } else if ((*outputBuffersI)->getRefCount() <= 0) {
                (*outputBuffersI)->decRefCount();
            }
        }
        
        if (buf) {
            if (outputBuffers.back()->getRefCount() < -REBUFFER_GC_LIMIT) {
                BufferType *ref = outputBuffers.back();
                outputBuffers.pop_back();
                delete ref;
            }
            return buf;
        }
        
        buf = new BufferType();
        outputBuffers.push_back(buf);
        
        return buf;
    }
    
    void purge() {
        while (!outputBuffers.empty()) {
            BufferType *ref = outputBuffers.front();
            outputBuffers.pop_front();
            delete ref;
        }
    }
private:
    std::deque<BufferType*> outputBuffers;
    typename std::deque<BufferType*>::iterator outputBuffersI;
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
    void *getInputQueue(std::string qname);
    void setOutputQueue(std::string qname, ThreadQueueBase *threadQueue);
    void *getOutputQueue(std::string qname);
    
    
protected:
    std::map<std::string, ThreadQueueBase *, map_string_less> input_queues;
    std::map<std::string, ThreadQueueBase *, map_string_less> output_queues;
    std::atomic_bool terminated;
};
