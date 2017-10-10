// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AudioSinkThread.h"

#define HEARTBEAT_CHECK_PERIOD_MICROS (50 * 1000) 

AudioSinkThread::AudioSinkThread() {
    inputQueuePtr = std::make_shared<AudioThreadInputQueue>();
    setInputQueue("input", inputQueuePtr);
}

AudioSinkThread::~AudioSinkThread() {

}

void AudioSinkThread::run() {
#ifdef __APPLE__
    pthread_t tID = pthread_self();	 // ID of this thread
    int priority = sched_get_priority_max(SCHED_RR) - 1;
    sched_param prio = { priority }; // scheduling priority of thread
    pthread_setschedparam(tID, SCHED_RR, &prio);
#endif

    AudioThreadInputPtr inp;
    AudioThreadInput inputRef;

    while (!stopping) {
        if (!inputQueuePtr->pop(inp, HEARTBEAT_CHECK_PERIOD_MICROS)) {
            continue;
        }

        if (inputRef.channels != inp->channels || 
                inputRef.frequency != inp->frequency ||
                inputRef.inputRate != inp->inputRate ||
                inputRef.sampleRate != inp->sampleRate) {

            inputChanged(inputRef, inp);

            inputRef.channels = inp->channels;
            inputRef.frequency = inp->frequency;
            inputRef.inputRate = inp->inputRate;
            inputRef.sampleRate = inp->sampleRate;
        }

        sink(inp);
    }
}

void AudioSinkThread::terminate() {
    IOThread::terminate();
    inputQueuePtr->flush();
}
