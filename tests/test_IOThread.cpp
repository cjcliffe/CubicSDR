// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "doctest.h"
#include "IOThread.h"

namespace {

class TestThread : public IOThread {
public:
    bool ran = false;

    void run() override {
        ran = true;
    }

    bool stoppingObserved() const {
        return stopping.load();
    }
};

} // namespace

TEST_SUITE("IOThread") {
    TEST_CASE("queues bind and rebind by name") {
        TestThread t;
        ThreadQueueBasePtr inQueue = std::make_shared<ThreadBlockingQueue<int>>();
        ThreadQueueBasePtr outQueue = std::make_shared<ThreadBlockingQueue<int>>();

        t.setInputQueue("in", inQueue);
        CHECK(t.getInputQueue("in") == inQueue);

        t.setOutputQueue("out", outQueue);
        CHECK(t.getOutputQueue("out") == outQueue);

        // Rebinding a name replaces the previous queue.
        t.setInputQueue("in", outQueue);
        CHECK(t.getInputQueue("in") == outQueue);
    }

    TEST_CASE("terminate sets the stopping flag") {
        TestThread t;
        CHECK_FALSE(t.stoppingObserved());
        t.terminate();
        CHECK(t.stoppingObserved());
    }

    TEST_CASE("threadMain runs run and then reports terminated") {
        TestThread t;
        CHECK_FALSE(t.isTerminated(0));

        // Runs synchronously: no thread is spawned, so this is deterministic.
        t.threadMain();

        CHECK(t.ran);
        CHECK(t.isTerminated(0));
        CHECK(t.isTerminated(100));
    }

    TEST_CASE("isTerminated waits out the timeout when still running") {
        TestThread t;
        // Never started: not terminated. The console line below is expected;
        // it is the timeout diagnostic, not a test failure.
        CHECK_FALSE(t.isTerminated(50));
    }
}

TEST_SUITE("ReBuffer") {
    TEST_CASE("getBuffer allocates new buffers while any are in use") {
        ReBuffer<int> rb("test");
        auto first = rb.getBuffer();
        auto second = rb.getBuffer();
        auto third = rb.getBuffer();

        REQUIRE(first != nullptr);
        REQUIRE(second != nullptr);
        REQUIRE(third != nullptr);
        CHECK(first.get() != second.get());
        CHECK(first.get() != third.get());
        CHECK(second.get() != third.get());

        *first = 42;
        CHECK(*first == 42);
    }

    TEST_CASE("getBuffer reuses a released buffer") {
        ReBuffer<int> rb("test");
        auto held = rb.getBuffer();
        void *address = held.get();
        held.reset();

        auto reused = rb.getBuffer();
        REQUIRE(reused != nullptr);
        CHECK(reused.get() == address);
    }

    TEST_CASE("age-out keeps the pool usable over many cycles") {
        ReBuffer<int> rb("test");
        auto first = rb.getBuffer();
        auto second = rb.getBuffer();
        first.reset();
        second.reset();

        // Far more reuse cycles than REBUFFER_GC_LIMIT, so the unused
        // entry ages out mid-loop; the pool must stay consistent.
        for (int i = 0; i < 150; i++) {
            auto tmp = rb.getBuffer();
            REQUIRE(tmp != nullptr);
            *tmp = i;
            CHECK(*tmp == i);
        }
    }

    TEST_CASE("purge drops pooled buffers") {
        ReBuffer<int> rb("test");
        auto held = rb.getBuffer();
        void *address = held.get();

        rb.purge();

        // The pool is empty, so this allocates; the held address is
        // still live, so the new block cannot coincide with it.
        auto fresh = rb.getBuffer();
        REQUIRE(fresh != nullptr);
        CHECK(fresh.get() != address);
        held.reset();
    }

    TEST_CASE("concurrent getBuffer calls all succeed") {
        ReBuffer<int> rb("test");
        constexpr int per_thread = 50;
        constexpr int thread_count = 4;
        std::vector<std::vector<std::shared_ptr<int>>> results(thread_count);

        std::vector<std::thread> workers;
        for (int t = 0; t < thread_count; t++) {
            workers.emplace_back([&, t]() {
                for (int i = 0; i < per_thread; i++) {
                    results[t].push_back(rb.getBuffer());
                }
            });
        }
        for (auto &worker : workers) {
            worker.join();
        }

        for (const auto &per_thread_results : results) {
            REQUIRE(per_thread_results.size() == per_thread);
            for (const auto &buffer : per_thread_results) {
                CHECK(buffer != nullptr);
            }
        }
    }
}
