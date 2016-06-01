#include "IOThread.h"

IOThread::IOThread() {
    terminated.store(false);
}

IOThread::~IOThread() {

}

#ifdef __APPLE__
void *IOThread::threadMain() {
    terminated.store(false);
    run();
    return this;
};

void *IOThread::pthread_helper(void *context) {
    return ((IOThread *) context)->threadMain();
};
#else
void IOThread::threadMain() {
    terminated.store(false);
    run();
};
#endif

void IOThread::setup() {
    
};

void IOThread::run() {
    
};

void IOThread::terminate() {
    terminated.store(true);
};

void IOThread::onBindOutput(std::string /* name */, ThreadQueueBase* /* threadQueue */) {
    
};

void IOThread::onBindInput(std::string /* name */, ThreadQueueBase* /* threadQueue */) {
    
};

void IOThread::setInputQueue(std::string qname, ThreadQueueBase *threadQueue) {
    input_queues[qname] = threadQueue;
    this->onBindInput(qname, threadQueue);
};

ThreadQueueBase *IOThread::getInputQueue(std::string qname) {
    return input_queues[qname];
};

void IOThread::setOutputQueue(std::string qname, ThreadQueueBase *threadQueue) {
    output_queues[qname] = threadQueue;
    this->onBindOutput(qname, threadQueue);
};

ThreadQueueBase *IOThread::getOutputQueue(std::string qname) {
    return output_queues[qname];
};

bool IOThread::isTerminated() {
    return terminated.load();
}
