// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <vector>
#include <memory>
#include "ThreadBlockingQueue.h"

// One decoded audio buffer travelling from a demodulator thread to the mixer.
// Kept free of audio-device headers so the headless mixer model and its tests
// can use the buffer and queue types without RtAudio or wxWidgets.
class AudioThreadInput {
public:
    long long frequency{};
    int inputRate{};
    int sampleRate{};
    int channels{};
    float peak{};
    int type{};
    bool is_squelch_active{};
    long long producedMs{};

    std::vector<float> data;

    AudioThreadInput() :
        frequency(0), inputRate(0), sampleRate(0), channels(0), peak(0), type(0), is_squelch_active(false), producedMs(0) {

    }


    explicit AudioThreadInput(AudioThreadInput *copyFrom) {
        copy(copyFrom);
    }

    void copy(AudioThreadInput *copyFrom) {
        frequency = copyFrom->frequency;
        inputRate = copyFrom->inputRate;
        sampleRate = copyFrom->sampleRate;
        channels = copyFrom->channels;
        peak = copyFrom->peak;
        type = copyFrom->type;
        is_squelch_active = copyFrom->is_squelch_active;
        producedMs = copyFrom->producedMs;
        data.assign(copyFrom->data.begin(), copyFrom->data.end());
    }


    virtual ~AudioThreadInput() = default;
};

typedef std::shared_ptr<AudioThreadInput> AudioThreadInputPtr;

typedef ThreadBlockingQueue<AudioThreadInputPtr> DemodulatorThreadOutputQueue;

typedef std::shared_ptr<DemodulatorThreadOutputQueue> DemodulatorThreadOutputQueuePtr;

typedef ThreadBlockingQueue<AudioThreadInputPtr> AudioThreadInputQueue;

typedef std::shared_ptr<AudioThreadInputQueue> AudioThreadInputQueuePtr;
