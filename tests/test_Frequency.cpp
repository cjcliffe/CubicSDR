// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "Frequency.h"

#include <cmath>
#include <string>

TEST_SUITE("Frequency") {
    TEST_CASE("formatting thresholds and SI kHz casing") {
        CHECK(frequencyToStr(0) == "0");
        CHECK(frequencyToStr(999) == "999");
        CHECK(frequencyToStr(1000) == "1kHz");
        CHECK(frequencyToStr(1500) == "1.5kHz");
        CHECK(frequencyToStr(475500) == "475.5kHz");
        CHECK(frequencyToStr(999500) == "999.5kHz");
        CHECK(frequencyToStr(999999) == "1000kHz");
        CHECK(frequencyToStr(1000000) == "1MHz");
        CHECK(frequencyToStr(7100000) == "7.1MHz");
        CHECK(frequencyToStr(146520000) == "146.52MHz");
        CHECK(frequencyToStr(999999999) == "1000MHz");
        CHECK(frequencyToStr(1000000000) == "1GHz");
        CHECK(frequencyToStr(1296000000) == "1.296GHz");
    }

    TEST_CASE("formatting leaves negative values as plain Hz") {
        CHECK(frequencyToStr(-100) == "-100");
        CHECK(frequencyToStr(-1500) == "-1500");
    }

    TEST_CASE("round trip holds within formatter precision") {
        const long long freqs[] = { 123456, 475500, 1800000, 3750000, 7100000, 14175000, 14230000, 28500000,
            50125000, 123456789, 144390000, 432100000, 999999999, 1296000000 };
        for (long long freq : freqs) {
            long long parsed = strToFrequency(frequencyToStr(freq));
            // Significant-digits formatting means exact round-trip does not hold at all magnitudes.
            long double tolerance = (freq > 0 ? (long double)freq : 1.0L) * 1e-3L + 1.0L;
            CHECK(std::fabs((long double)parsed - (long double)freq) <= tolerance);
        }
    }

    TEST_CASE("parsing applies the implicit megahertz rule to bare input") {
        CHECK(strToFrequency("14.175", FrequencyUnit::Auto) == 14175000);
        CHECK(strToFrequency("0.5", FrequencyUnit::Auto) == 500000);
        CHECK(strToFrequency("500", FrequencyUnit::Auto) == 500000000);
        CHECK(strToFrequency("1000", FrequencyUnit::Auto) == 1000000000);
        CHECK(strToFrequency("3001", FrequencyUnit::Auto) == 3001);
        CHECK(strToFrequency("146520000", FrequencyUnit::Auto) == 146520000);
        CHECK(strToFrequency("0", FrequencyUnit::Auto) == 0);
    }

    TEST_CASE("parsing handles suffix casing, whitespace, and invalid input") {
        CHECK(strToFrequency("1kHz") == 1000);
        CHECK(strToFrequency("1KHz") == 1000);
        CHECK(strToFrequency("1KHZ") == 1000);
        CHECK(strToFrequency("1MHz") == 1000000);
        CHECK(strToFrequency("1mhz") == 1000000);
        CHECK(strToFrequency("1M") == 1000000);
        CHECK(strToFrequency("1MHZ") == 1000000);
        CHECK(strToFrequency("1GHz") == 1000000000);
        CHECK(strToFrequency("1G") == 1000000000);
        CHECK(strToFrequency("500Hz") == 500);
        CHECK(strToFrequency("  7.1 MHz ") == 7100000);
        CHECK(strToFrequency("") == 0);
        CHECK(strToFrequency("abc") == 0);
    }

    TEST_CASE("parsing strips a leading dash so negative input does not round-trip") {
        CHECK(strToFrequency("-100") == 100000000);
        CHECK(strToFrequency(frequencyToStr(-100)) != -100);
    }

    TEST_CASE("filterChars filters in place against the allow-list") {
        std::string s = "a1.5MHz!";
        CHECK(&filterChars(s, "0123456789.MKGHmkgh") == &s);
        CHECK(s == "1.5MH");
    }

    TEST_CASE("bare input defaults to megahertz unless a unit is given") {
        CHECK(strToFrequency("5000") == 5000000000);
        CHECK(strToFrequency("5", FrequencyUnit::Hz) == 5);
        CHECK(strToFrequency("5", FrequencyUnit::kHz) == 5000);
        CHECK(strToFrequency("5", FrequencyUnit::MHz) == 5000000);
        CHECK(strToFrequency("5", FrequencyUnit::GHz) == 5000000000);
        CHECK(strToFrequency("5MHz", FrequencyUnit::Hz) == 5000000);
        CHECK(strToFrequency("5000", FrequencyUnit::Auto) == 5000);
    }
}
