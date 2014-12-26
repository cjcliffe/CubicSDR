#pragma once

#ifdef __APPLE__
#define BUF_SIZE (16384*2)
#define SRATE 2000000
#else
#define BUF_SIZE (16384*4)
#define SRATE 2500000
#endif
#define FFT_SIZE 2048

#define DEFAULT_FREQ 98900000
#define AUDIO_FREQUENCY 44100

#include <mutex>
#include <atomic>

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
    std::atomic<int> refCount;
};
