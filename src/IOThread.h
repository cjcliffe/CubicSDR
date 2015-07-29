#pragma once

#include <mutex>
#include <atomic>
#include <deque>
#include <map>
#include <string>


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


template<class BufferType = ReferenceCounter>
class ReBuffer {
    
public:
    BufferType *getBuffer() {
        BufferType* buf = NULL;
        for (outputBuffersI = outputBuffers.begin(); outputBuffersI != outputBuffers.end(); outputBuffersI++) {
            if ((*outputBuffersI)->getRefCount() <= 0) {
                return (*outputBuffersI);
            }
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
    virtual void setup() {
        
    };
    
    virtual void init() {
        
    };
    
    virtual void onBindOutput(std::string name, void* threadQueue) {
        
    };
    
    virtual void onBindInput(std::string name, void* threadQueue) {
        
    };
    
#ifdef __APPLE__
    virtual void *threadMain() {
        return 0;
    };
    
    static void *pthread_helper(void *context) {
        return ((IOThread *) context)->threadMain();
    };
    
#else
    virtual void threadMain() {
        
    };
#endif
    
    virtual void terminate() {
        
    };
    
    void setInputQueue(std::string qname, void *threadQueue) {
        input_queues[qname] = threadQueue;
        this->onBindInput(qname, threadQueue);
    };
    
    void *getInputQueue(std::string qname) {
        return input_queues[qname];
    };
    
    void setOutputQueue(std::string qname, void *threadQueue) {
        output_queues[qname] = threadQueue;
        this->onBindOutput(qname, threadQueue);
    };
    
    void *getOutputQueue(std::string qname) {
        return output_queues[qname];
    };
    
    
protected:
    std::map<std::string, void *, map_string_less> input_queues;
    std::map<std::string, void *, map_string_less> output_queues;
};
