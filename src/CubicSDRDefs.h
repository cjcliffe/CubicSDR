#pragma once

#define CUBICSDR_TITLE "CubicSDR " CUBICSDR_VERSION " by Charles J. Cliffe (@ccliffe)"

#ifndef __BYTE_ORDER
    #ifdef _WIN32
        #define ATTRIBUTE
        #define __LITTLE_ENDIAN 1234
        #define __BIG_ENDIAN    4321
        #define __PDP_ENDIAN    3412
        #define __BYTE_ORDER __LITTLE_ENDIAN
    #else
        #ifdef __APPLE__
            #include <machine/endian.h>
        #else
            #include <endian.h>
        #endif
    #endif
#endif

const char filePathSeparator =
#ifdef _WIN32
                            '\\';
#else
                            '/';
#endif

#ifdef __APPLE__
#define BUF_SIZE (16384*6)
#define DEFAULT_SAMPLE_RATE 2000000
#endif
#ifdef __linux__
#define BUF_SIZE (16384*6)
#define DEFAULT_SAMPLE_RATE 2000000
#endif

#ifndef BUF_SIZE
#define BUF_SIZE (16384*5)
#define DEFAULT_SAMPLE_RATE 2500000
#endif

#define DEFAULT_FFT_SIZE 2048

#define DEFAULT_FREQ 100000000
#define DEFAULT_DEMOD_TYPE 1
#define DEFAULT_DEMOD_BW 200000

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
