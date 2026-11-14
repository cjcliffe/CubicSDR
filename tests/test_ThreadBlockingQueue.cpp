// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

#include "doctest.h"
#include "ThreadBlockingQueue.h"

TEST_SUITE("ThreadBlockingQueue") {
    TEST_CASE("push and pop preserve order") {
        ThreadBlockingQueue<int> q;
        q.set_max_num_items(10);
        for (int i = 0; i < 5; i++) {
            CHECK(q.push(i));
        }
        CHECK(q.size() == 5);
        CHECK_FALSE(q.empty());
        for (int i = 0; i < 5; i++) {
            int value = -1;
            CHECK(q.pop(value));
            CHECK(value == i);
        }
        CHECK(q.empty());
    }

    TEST_CASE("full and flush states") {
        ThreadBlockingQueue<int> q; // default capacity is a single item
        CHECK(q.empty());
        CHECK_FALSE(q.full());
        CHECK(q.push(1));
        CHECK(q.full());
        CHECK_FALSE(q.try_push(2));
        q.flush();
        CHECK(q.empty());
        CHECK_FALSE(q.full());
    }

    TEST_CASE("try_pop fails on an empty queue") {
        ThreadBlockingQueue<int> q;
        int value = -1;
        CHECK_FALSE(q.try_pop(value));
        CHECK(value == -1);
    }

    TEST_CASE("timed operations fail without blocking forever") {
        ThreadBlockingQueue<int> q;
        q.set_max_num_items(1);
        CHECK(q.push(1));

        // Short timeouts take the immediate-fail path when blocked.
        CHECK_FALSE(q.push(2, 50));
        int value = -1;
        CHECK(q.pop(value));
        CHECK(value == 1);
        value = -1;
        CHECK_FALSE(q.pop(value, 50));
        CHECK(value == -1);

        // A longer timeout exercises the real wait before failing.
        CHECK(q.push(1));
        CHECK_FALSE(q.push(2, 5000));
        CHECK(q.pop(value));
    }

    TEST_CASE("capacity only grows") {
        ThreadBlockingQueue<int> q;
        q.set_max_num_items(5);
        q.set_max_num_items(2); // shrinking is a no-op
        for (int i = 0; i < 5; i++) {
            CHECK(q.try_push(i));
        }
        CHECK(q.full());
    }

    TEST_CASE("a blocked pop wakes when the producer pushes") {
        ThreadBlockingQueue<int> q; // default capacity is a single item
        int received = -1;
        // The sleep lets the worker park in its wait; join plus the
        // final asserts prove the wakeup, not timing.
        bool got = false;

        std::thread consumer([&]() {
            got = q.pop(received);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        CHECK(q.push(42));
        consumer.join();

        CHECK(got);
        CHECK(received == 42);
    }

    TEST_CASE("a blocked push wakes when the consumer pops") {
        ThreadBlockingQueue<int> q; // default capacity is a single item
        CHECK(q.push(1));

        // The sleep lets the worker park in its wait; join plus the
        // final asserts prove the wakeup, not timing.
        bool pushed = false;
        std::thread producer([&]() {
            pushed = q.push(2);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        int value = -1;
        CHECK(q.pop(value));
        CHECK(value == 1);
        producer.join();
        CHECK(pushed);

        CHECK(q.pop(value));
        CHECK(value == 2);
    }

    TEST_CASE("producer and consumer transfer every item in order") {
        ThreadBlockingQueue<int> q;
        // Default timeouts block indefinitely, so this also covers the blocking waits.
        q.set_max_num_items(16);
        constexpr int count = 200;
        std::vector<int> received;
        received.reserve(count);

        std::thread producer([&]() {
            for (int i = 0; i < count; i++) {
                q.push(i);
            }
        });
        std::thread consumer([&]() {
            for (int i = 0; i < count; i++) {
                int value = -1;
                q.pop(value);
                received.push_back(value);
            }
        });
        producer.join();
        consumer.join();

        REQUIRE(received.size() == count);
        for (int i = 0; i < count; i++) {
            CHECK(received[i] == i);
        }
    }
}
