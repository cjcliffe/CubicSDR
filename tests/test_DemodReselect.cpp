// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "demod/DemodReselect.h"

namespace {

std::vector<char> allActive(size_t n) {
    return std::vector<char>(n, 1);
}

TEST_SUITE("DemodReselect") {
    TEST_CASE("empty list selects nothing") {
        CHECK(DemodReselect::pick({}, {}, -1, 1000, 100) == -1);
        CHECK(DemodReselect::pick({}, {}, 0, 1000, 100) == -1);
    }

    TEST_CASE("previous in view wins over lower frequencies") {
        std::vector<long long> freqs = { 900, 1000, 1100 };
        CHECK(DemodReselect::pick(freqs, allActive(3), 1, 1000, 100) == 1);
    }

    TEST_CASE("previous out of view falls back to first in view") {
        std::vector<long long> freqs = { 900, 1000, 2000 };
        CHECK(DemodReselect::pick(freqs, allActive(3), 2, 1000, 100) == 1);
    }

    TEST_CASE("no previous selects first in view") {
        std::vector<long long> freqs = { 2000, 1010, 990 };
        CHECK(DemodReselect::pick(freqs, allActive(3), -1, 1000, 100) == 2);
    }

    TEST_CASE("out-of-range previous is ignored") {
        std::vector<long long> freqs = { 2000, 1010, 990 };
        CHECK(DemodReselect::pick(freqs, allActive(3), 7, 1000, 100) == 2);
    }

    TEST_CASE("nothing in view selects nothing") {
        std::vector<long long> freqs = { 5000, 6000 };
        CHECK(DemodReselect::pick(freqs, allActive(2), 1, 1000, 100) == -1);
        CHECK(DemodReselect::pick(freqs, allActive(2), -1, 1000, 100) == -1);
    }

    TEST_CASE("non-positive span treats everything as in view") {
        std::vector<long long> freqs = { 900, 1000, 2000 };
        CHECK(DemodReselect::pick(freqs, allActive(3), 2, 1000, 0) == 2);
        CHECK(DemodReselect::pick(freqs, allActive(3), -1, 1000, 0) == 0);
    }

    TEST_CASE("view edges are inclusive") {
        std::vector<long long> freqs = { 949, 950, 1050, 1051 };
        CHECK(DemodReselect::pick(freqs, allActive(4), -1, 1000, 100) == 1);
        CHECK(DemodReselect::pick({ 949, 1050 }, allActive(2), -1, 1000, 100) == 1);
        CHECK(DemodReselect::pick({ 950, 949 }, allActive(2), -1, 1000, 100) == 0);
        std::vector<long long> outside = { 949, 1051 };
        CHECK(DemodReselect::pick(outside, allActive(2), -1, 1000, 100) == -1);
    }

    TEST_CASE("frequency ties break by lowest index") {
        std::vector<long long> freqs = { 1000, 1000 };
        CHECK(DemodReselect::pick(freqs, allActive(2), -1, 1000, 100) == 0);
    }

    TEST_CASE("single candidate selects it only in view") {
        CHECK(DemodReselect::pick({ 1000 }, allActive(1), -1, 1000, 100) == 0);
        CHECK(DemodReselect::pick({ 5000 }, allActive(1), -1, 1000, 100) == -1);
        CHECK(DemodReselect::pick({ 5000 }, allActive(1), 0, 1000, 100) == -1);
    }

    TEST_CASE("inactive previous falls back to first active in view") {
        std::vector<long long> freqs = { 900, 1000, 1010 };
        std::vector<char> active = { 1, 0, 1 };
        CHECK(DemodReselect::pick(freqs, active, 1, 1000, 100) == 2);
    }

    TEST_CASE("first in view skips inactive entries") {
        std::vector<long long> freqs = { 990, 1000, 1010 };
        std::vector<char> active = { 0, 1, 1 };
        CHECK(DemodReselect::pick(freqs, active, -1, 1000, 100) == 1);
    }

    TEST_CASE("all inactive selects nothing") {
        std::vector<long long> freqs = { 990, 1000 };
        std::vector<char> active = { 0, 0 };
        CHECK(DemodReselect::pick(freqs, active, 1, 1000, 100) == -1);
        CHECK(DemodReselect::pick(freqs, active, -1, 1000, 100) == -1);
    }

    TEST_CASE("missing activity flags count as inactive") {
        std::vector<long long> freqs = { 990, 1000 };
        CHECK(DemodReselect::pick(freqs, { 1 }, -1, 1000, 100) == 0);
        CHECK(DemodReselect::pick(freqs, {}, -1, 1000, 100) == -1);
    }
}

}
