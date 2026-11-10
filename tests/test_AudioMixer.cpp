// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "audio/AudioMixer.h"

namespace {

// Small frame count keeps the tests fast; the mixer treats it opaquely.
constexpr unsigned int NFRAMES = 8;
constexpr int DEVICE_RATE = 48000;

AudioThreadInputPtr makeBuffer(int channels, int sampleRate, long long producedMs, std::vector<float> data,
                               float peak = 0.5f) {
    auto buffer = std::make_shared<AudioThreadInput>();
    buffer->channels = channels;
    buffer->sampleRate = sampleRate;
    buffer->producedMs = producedMs;
    buffer->data = std::move(data);
    buffer->peak = peak;
    return buffer;
}

std::vector<float> ramp(size_t count, float start = 0.0f, float step = 0.01f) {
    std::vector<float> out(count);
    for (size_t i = 0; i < count; i++) {
        out[i] = start + step * (float)i;
    }
    return out;
}

struct Harness {
    MixerVoice voice;
    float out[NFRAMES * 2]{};
    long long nowMs = 1000000;

    Harness() {
        voice.inputQueue = std::make_shared<AudioThreadInputQueue>();
        voice.inputQueue->set_max_num_items(100);
        voice.prebufferDone = true;
    }

    void push(AudioThreadInputPtr buffer) {
        CHECK(voice.inputQueue->try_push(buffer));
    }

    double pass() {
        std::fill(std::begin(out), std::end(out), 0.0f);
        return AudioMixer::mixVoice(voice, out, NFRAMES, DEVICE_RATE, nowMs);
    }

    // Mono sample i of the mixed output (left channel carries the mix).
    float mono(size_t i) const {
        return out[i * 2];
    }
};

TEST_SUITE("AudioMixer") {
    TEST_CASE("steady mono play is FIFO-exact") {
        Harness h;
        for (int b = 0; b < 3; b++) {
            h.push(makeBuffer(1, DEVICE_RATE, h.nowMs, ramp(NFRAMES, 0.1f * (float)b, 0.01f), 0.2f));
        }

        for (int b = 0; b < 3; b++) {
            double peak = h.pass();
            CHECK(peak == doctest::Approx(0.2));
            for (size_t i = 0; i < NFRAMES; i++) {
                float want = 0.1f * (float)b + 0.01f * (float)i;
                CHECK(h.mono(i) == doctest::Approx(want));
                CHECK(h.out[i * 2 + 1] == doctest::Approx(want));
            }
        }
        CHECK(h.voice.inputQueue->empty());
    }

    TEST_CASE("stereo mixes direct without duplication") {
        Harness h;
        std::vector<float> data;
        for (int i = 0; i < (int)NFRAMES * 2; i++) {
            data.push_back(0.05f * (float)i);
        }
        h.push(makeBuffer(2, DEVICE_RATE, h.nowMs, data, 0.4f));

        double peak = h.pass();
        CHECK(peak == doctest::Approx(0.4));
        for (int i = 0; i < (int)NFRAMES * 2; i++) {
            CHECK(h.out[i] == doctest::Approx(0.05f * (float)i));
        }
    }

    TEST_CASE("gain scales samples and peak") {
        Harness h;
        h.voice.gain = 0.5f;
        h.push(makeBuffer(1, DEVICE_RATE, h.nowMs, std::vector<float>(NFRAMES, 0.4f), 0.4f));

        double peak = h.pass();
        CHECK(peak == doctest::Approx(0.2));
        for (size_t i = 0; i < NFRAMES; i++) {
            CHECK(h.mono(i) == doctest::Approx(0.2f));
        }
    }

    TEST_CASE("prebuffer holds silent until the lead accumulates") {
        Harness h;
        h.voice.prebufferDone = false;
        h.push(makeBuffer(1, DEVICE_RATE, h.nowMs, ramp(NFRAMES, 0.3f), 0.3f));

        // First observation latches the clock; the backlog must accumulate,
        // not drain, while held.
        CHECK(h.pass() == doctest::Approx(0.0));
        CHECK(h.voice.prebufferStartMs == h.nowMs);
        CHECK(h.voice.inputQueue->size() == 1);
        for (size_t i = 0; i < NFRAMES; i++) {
            CHECK(h.mono(i) == doctest::Approx(0.0f));
        }

        h.nowMs += AUDIOTHREAD_PREBUFFER_MS - 1;
        CHECK(h.pass() == doctest::Approx(0.0));

        h.nowMs += 1;
        CHECK(h.pass() == doctest::Approx(0.3));
        CHECK(h.voice.prebufferDone);
        CHECK(h.mono(0) == doctest::Approx(0.3f));
    }

    TEST_CASE("deep stale backlog drains to the keep window") {
        Harness h;
        // Ten buffers spanning 900ms: only the newest keep-window worth is live.
        for (int b = 0; b < 10; b++) {
            h.push(makeBuffer(1, DEVICE_RATE, h.nowMs - 900 + 100 * b, std::vector<float>(NFRAMES, 0.01f * (float)b),
                              0.1f));
        }

        h.pass();
        // Keeps buffers within 200ms behind the newest (the span test is
        // strict); the first mix plays the oldest kept buffer.
        CHECK(h.mono(0) == doctest::Approx(0.07f));
        CHECK(h.voice.inputQueue->size() == 2);
        AudioThreadInputPtr rest;
        CHECK(h.voice.inputQueue->try_pop(rest));
        CHECK(rest->producedMs == h.nowMs - 100);
        CHECK(h.voice.inputQueue->try_pop(rest));
        CHECK(rest->producedMs == h.nowMs);
    }

    TEST_CASE("stall resumes immediately with no added silence") {
        Harness h;
        h.push(makeBuffer(1, DEVICE_RATE, h.nowMs, std::vector<float>(NFRAMES, 0.25f), 0.25f));
        CHECK(h.pass() == doctest::Approx(0.25));

        // Starve mid-stream: the next pass finds nothing to pop, stays silent,
        // and contributes no peak.
        CHECK(h.pass() == doctest::Approx(0.0));
        CHECK(!h.voice.currentInput);
        for (size_t i = 0; i < NFRAMES; i++) {
            CHECK(h.mono(i) == doctest::Approx(0.0f));
        }

        // Fresh shallow backlog plays in the same pass it is picked up.
        h.push(makeBuffer(1, DEVICE_RATE, h.nowMs, std::vector<float>(NFRAMES, 0.35f), 0.35f));
        CHECK(h.pass() == doctest::Approx(0.35));
        CHECK(h.mono(0) == doctest::Approx(0.35f));
    }

    TEST_CASE("loaded current plays through an empty queue") {
        Harness h;
        h.push(makeBuffer(1, DEVICE_RATE, h.nowMs, ramp(NFRAMES * 2, 0.1f), 0.2f));
        h.pass();
        CHECK(h.voice.inputQueue->empty());

        // An empty queue alone must never skip playable samples.
        CHECK(h.pass() == doctest::Approx(0.2));
        for (size_t i = 0; i < NFRAMES; i++) {
            CHECK(h.mono(i) == doctest::Approx(0.1f + 0.01f * (float)(NFRAMES + i)));
        }
    }

    TEST_CASE("rate mismatch discards until a match, then mixes it") {
        Harness h;
        h.push(makeBuffer(1, 44100, h.nowMs, std::vector<float>(NFRAMES, 0.1f), 0.1f));
        h.push(makeBuffer(1, 44100, h.nowMs, std::vector<float>(NFRAMES, 0.2f), 0.2f));
        h.push(makeBuffer(1, DEVICE_RATE, h.nowMs, std::vector<float>(NFRAMES, 0.3f), 0.3f));

        // Resume pops the stale buffer, the rate filter drops it and its stale
        // sibling, and the matching buffer mixes in the same pass.
        CHECK(h.pass() == doctest::Approx(0.3));
        CHECK(h.mono(0) == doctest::Approx(0.3f));
        CHECK(h.voice.inputQueue->empty());

        // With no match anywhere the filter unloads the stale backlog and the
        // voice stays silent.
        h.push(makeBuffer(1, 44100, h.nowMs, std::vector<float>(NFRAMES, 0.4f), 0.4f));
        h.push(makeBuffer(1, 44100, h.nowMs, std::vector<float>(NFRAMES, 0.5f), 0.5f));
        h.voice.currentInput = nullptr;
        h.voice.audioQueuePtr = 0;
        CHECK(h.pass() == doctest::Approx(0.0));
        CHECK(!h.voice.currentInput);
        CHECK(h.voice.inputQueue->empty());

        // Stale current with nothing queued unloads instead of emitting a
        // wrong-rate pass.
        h.voice.currentInput = makeBuffer(1, 44100, h.nowMs, std::vector<float>(NFRAMES, 0.6f), 0.6f);
        CHECK(h.pass() == doctest::Approx(0.0));
        CHECK(!h.voice.currentInput);
        CHECK(h.mono(0) == doctest::Approx(0.0f));
    }

    TEST_CASE("empty-data current advances on the next pass") {
        Harness h;
        auto d0 = makeBuffer(1, DEVICE_RATE, h.nowMs, std::vector<float>(), 0.0f);
        auto d1 = makeBuffer(1, DEVICE_RATE, h.nowMs, std::vector<float>(NFRAMES, 0.5f), 0.5f);
        h.push(d0);
        h.push(d1);

        // The empty buffer is replaced but never mixed: this pass stays silent.
        CHECK(h.pass() == doctest::Approx(0.0));
        CHECK(h.voice.currentInput == d1);
        CHECK(h.mono(0) == doctest::Approx(0.0f));

        CHECK(h.pass() == doctest::Approx(0.5));
        CHECK(h.mono(0) == doctest::Approx(0.5f));
    }

    TEST_CASE("normalize scales only hot mixes") {
        float hot[NFRAMES * 2];
        std::fill(std::begin(hot), std::end(hot), 0.6f);
        AudioMixer::normalize(hot, NFRAMES, 1.2);
        for (float v : hot) {
            CHECK(v == doctest::Approx(0.5f));
        }

        float cool[NFRAMES * 2];
        std::fill(std::begin(cool), std::end(cool), 0.4f);
        AudioMixer::normalize(cool, NFRAMES, 0.8);
        for (float v : cool) {
            CHECK(v == doctest::Approx(0.4f));
        }
    }

    TEST_CASE("voices sum into one buffer") {
        Harness h;
        MixerVoice second;
        second.inputQueue = std::make_shared<AudioThreadInputQueue>();
        second.inputQueue->set_max_num_items(100);
        second.prebufferDone = true;
        h.push(makeBuffer(1, DEVICE_RATE, h.nowMs, std::vector<float>(NFRAMES, 0.2f), 0.2f));
        CHECK(second.inputQueue->try_push(
            makeBuffer(1, DEVICE_RATE, h.nowMs, std::vector<float>(NFRAMES, 0.3f), 0.3f)));

        double peak = 0.0;
        std::fill(std::begin(h.out), std::end(h.out), 0.0f);
        peak += AudioMixer::mixVoice(h.voice, h.out, NFRAMES, DEVICE_RATE, h.nowMs);
        peak += AudioMixer::mixVoice(second, h.out, NFRAMES, DEVICE_RATE, h.nowMs);
        CHECK(peak == doctest::Approx(0.5));
        for (size_t i = 0; i < NFRAMES; i++) {
            CHECK(h.mono(i) == doctest::Approx(0.5f));
        }
    }

    TEST_CASE("reset drops held input and restarts the hold") {
        Harness h;
        h.voice.currentInput = makeBuffer(1, DEVICE_RATE, h.nowMs, ramp(NFRAMES, 0.9f), 0.9f);
        h.voice.audioQueuePtr = 3;
        h.voice.prebufferDone = true;
        h.voice.prebufferStartMs = h.nowMs;

        h.voice.reset();

        CHECK(h.voice.currentInput == nullptr);
        CHECK(h.voice.audioQueuePtr == 0);
        CHECK(!h.voice.prebufferDone);
        CHECK(h.voice.prebufferStartMs == 0);
    }

    TEST_CASE("hold stays silent with a loaded current input") {
        Harness h;
        h.voice.prebufferDone = false;
        h.voice.prebufferStartMs = 0;
        h.voice.currentInput = makeBuffer(1, DEVICE_RATE, h.nowMs, ramp(NFRAMES, 0.9f), 0.9f);
        h.push(makeBuffer(1, DEVICE_RATE, h.nowMs, ramp(NFRAMES, 0.1f), 0.1f));

        CHECK(h.pass() == 0.0);
        CHECK(h.mono(0) == 0.0f);
        CHECK(h.voice.currentInput != nullptr);
    }
}

}  // namespace
