// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "doctest.h"
#include "SpinMutex.h"

TEST_SUITE("SpinMutex") {
    TEST_CASE("lock makes try_lock fail until unlock") {
        SpinMutex m;
        m.lock();
        CHECK_FALSE(m.try_lock());
        m.unlock();
        CHECK(m.try_lock());
        m.unlock();
    }

    TEST_CASE("works with lock_guard and unique_lock") {
        SpinMutex m;
        {
            std::lock_guard<SpinMutex> guard(m);
            CHECK_FALSE(m.try_lock());
        }
        {
            std::unique_lock<SpinMutex> lock(m);
            CHECK_FALSE(m.try_lock());
        }
        CHECK(m.try_lock());
        m.unlock();
    }

    TEST_CASE("protects a shared counter across threads") {
        SpinMutex m;
        int counter = 0;
        constexpr int per_thread = 1000;
        constexpr int thread_count = 4;
        std::vector<std::thread> workers;
        for (int t = 0; t < thread_count; t++) {
            workers.emplace_back([&]() {
                for (int i = 0; i < per_thread; i++) {
                    std::lock_guard<SpinMutex> guard(m);
                    counter++;
                }
            });
        }
        for (auto &worker : workers) {
            worker.join();
        }
        CHECK(counter == per_thread * thread_count);
    }

    TEST_CASE("is not copyable") {
        static_assert(!std::is_copy_constructible<SpinMutex>::value, "SpinMutex must not be copy constructible");
        static_assert(!std::is_copy_assignable<SpinMutex>::value, "SpinMutex must not be copy assignable");
        CHECK(true);
    }
}
