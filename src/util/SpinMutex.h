// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once 
#include <atomic>

// A non-recursive Mutex implemented as a spin-lock, implementing the Lockable requirement
class SpinMutex {

public:
    SpinMutex() = default;

    SpinMutex(const SpinMutex&) = delete;

    SpinMutex& operator=(const SpinMutex&) = delete;

    ~SpinMutex() { lock_state.clear(std::memory_order_release); }

    void lock() { while (lock_state.test_and_set(std::memory_order_acquire)); }

    bool try_lock() {return !lock_state.test_and_set(std::memory_order_acquire); }

    void unlock() { lock_state.clear(std::memory_order_release); }

private:
    std::atomic_flag lock_state = ATOMIC_FLAG_INIT;
};
