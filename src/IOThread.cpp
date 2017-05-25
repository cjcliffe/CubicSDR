// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "IOThread.h"
#include <typeinfo>

#define SPIN_WAIT_SLEEP_MS 5

IOThread::IOThread() {
    terminated.store(false);
    stopping.store(false);
}

IOThread::~IOThread() {
    terminated.store(true);
    stopping.store(true);
}

#ifdef __APPLE__
void *IOThread::threadMain() {
    terminated.store(false);
    stopping.store(false);
    try {
        run();
    }
    catch (...) {
        terminated.store(true);
        stopping.store(true);
        throw;
    }

    terminated.store(true);
    stopping.store(true);
    return this;
};

void *IOThread::pthread_helper(void *context) {
    return ((IOThread *) context)->threadMain();
};
#else
void IOThread::threadMain() {
    terminated.store(false);
    stopping.store(false);
    try {
        run();
    }
    catch (...) {
        terminated.store(true);
        stopping.store(true);
        throw;
    }
  
    terminated.store(true);
    stopping.store(true);
};
#endif

void IOThread::setup() {
    //redefined in subclasses
};

void IOThread::run() {
    //redefined in subclasses
};


void IOThread::terminate() {
    stopping.store(true);
};

void IOThread::onBindOutput(std::string /* name */, ThreadQueueBase* /* threadQueue */) {
   
};

void IOThread::onBindInput(std::string /* name */, ThreadQueueBase* /* threadQueue */) {
    
};

void IOThread::setInputQueue(std::string qname, ThreadQueueBase *threadQueue) {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    input_queues[qname] = threadQueue;
    this->onBindInput(qname, threadQueue);
};

ThreadQueueBase *IOThread::getInputQueue(std::string qname) {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    return input_queues[qname];
};

void IOThread::setOutputQueue(std::string qname, ThreadQueueBase *threadQueue) {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    output_queues[qname] = threadQueue;
    this->onBindOutput(qname, threadQueue);
};

ThreadQueueBase *IOThread::getOutputQueue(std::string qname) {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    return output_queues[qname];
};

bool IOThread::isTerminated(int waitMs) {

    if (terminated.load()) {
        return true;
    }
    else if (waitMs == 0) {
        return false;
    }

    //this is a stupid busy plus sleep loop
    int nbCyclesToWait = 0;

    if (waitMs < 0) {
        nbCyclesToWait = std::numeric_limits<int>::max();
    }
    else {

        nbCyclesToWait = (waitMs / SPIN_WAIT_SLEEP_MS) + 1;
    }

    for ( int i = 0; i < nbCyclesToWait; i++) {

        std::this_thread::sleep_for(std::chrono::milliseconds(SPIN_WAIT_SLEEP_MS));

        if (terminated.load()) {
            return true;
        }
    }

    std::cout << "ERROR: thread '" << typeid(*this).name() << "' has not terminated in time ! (> " << waitMs << " ms)" << std::endl << std::flush;

    return terminated.load();
}
