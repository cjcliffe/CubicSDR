// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "sdr/SDRPacerThread.h"

#include <chrono>
#include <iostream>
#include <thread>

//50 ms
#define HEARTBEAT_CHECK_PERIOD_MICROS (50 * 1000)

// Recheck input at least this often instead of sleeping a full period,
// so arriving blocks, disable toggles, and flush requests are noticed
// promptly; overshoot only banks pacing credit instead of losing time.
#define PACER_MAX_SLEEP_MICROS (2 * 1000)

SDRPacerThread::SDRPacerThread() : pacingEnabled(true), flushRequested(false) {
}

void SDRPacerThread::run() {
    SDRThreadIQDataQueuePtr iqDataInQueue =
        std::static_pointer_cast<SDRThreadIQDataQueue>(getInputQueue("IQDataInput"));
    SDRThreadIQDataQueuePtr iqDataOutQueue =
        std::static_pointer_cast<SDRThreadIQDataQueue>(getOutputQueue("IQDataOutput"));

    if (iqDataInQueue == nullptr || iqDataOutQueue == nullptr) {
        return;
    }

    long droppedWindow = 0;
    size_t maxLevelWindow = 0;
    auto lastLog = std::chrono::steady_clock::now();
    auto lastTick = lastLog;

    // Stream sentry: a block from a new stream discards the old one
    // everywhere (backlog plus output queue), adopts the new identity,
    // then re-drains the input keeping only new-stream siblings, so no
    // stale frame survives a rate or channel change.
    auto noteBlock = [&](SDRThreadIQDataPtr block) {
        if (pacer.noteBlock(block->sampleRate, block->numChannels)) {
            droppedWindow += backlog.size();
            backlog.clear();
            droppedWindow += iqDataOutQueue->size();
            iqDataOutQueue->flush();
            pacer.reset();
            pacer.noteBlock(block->sampleRate, block->numChannels);
            backlog.push_back(block);
            SDRThreadIQDataPtr queued;
            while (iqDataInQueue->try_pop(queued)) {
                if (!pacer.noteBlock(queued->sampleRate, queued->numChannels)) {
                    backlog.push_back(queued);
                } else {
                    droppedWindow++;
                }
            }
            return;
        }
        backlog.push_back(block);
    };

    while (!stopping) {
        if (flushRequested.exchange(false)) {
            backlog.clear();
            iqDataInQueue->flush();
            iqDataOutQueue->flush();
            pacer.reset();
        }

        // Dry: sleep on input instead of spinning; the heartbeat timeout
        // keeps the loop responsive to stop and flush requests.
        if (backlog.empty()) {
            SDRThreadIQDataPtr block;
            if (!iqDataInQueue->pop(block, HEARTBEAT_CHECK_PERIOD_MICROS)) {
                continue;
            }
            noteBlock(block);
        }

        // Then drain whatever arrived so each tick schedules the backlog.
        SDRThreadIQDataPtr block;
        while (iqDataInQueue->try_pop(block)) {
            noteBlock(block);
        }

        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> dt = now - lastTick;
        lastTick = now;

        pacer.setBypass(!pacingEnabled.load());
        SDRPacer::Plan plan = pacer.plan(dt.count(), backlog.size());

        for (int i = 0; i < plan.drop && !backlog.empty(); i++) {
            backlog.pop_front();
            droppedWindow++;
        }
        for (int i = 0; i < plan.emit && !backlog.empty() && !iqDataOutQueue->full(); i++) {
            SDRThreadIQDataPtr out = backlog.front();
            backlog.pop_front();
            // A refused emit goes back to the front: the pacer already
            // decided this frame is due, so it waits instead of dropping.
            if (!iqDataOutQueue->try_push(out)) {
                backlog.push_front(out);
                break;
            }
        }

        if (backlog.size() > maxLevelWindow) {
            maxLevelWindow = backlog.size();
        }
        // Drop accounting is summarized every few seconds, never per frame.
        if (droppedWindow > 0 && now - lastLog > std::chrono::seconds(5)) {
            std::cout << "SDRPacerThread: dropped " << droppedWindow << " blocks, max level "
                      << maxLevelWindow << " frames" << std::endl;
            droppedWindow = 0;
            maxLevelWindow = 0;
            lastLog = now;
        }

        if (plan.waitSeconds > 0.0) {
            long sleepMicros = (long)(plan.waitSeconds * 1e6);
            if (sleepMicros > PACER_MAX_SLEEP_MICROS) {
                sleepMicros = PACER_MAX_SLEEP_MICROS;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(sleepMicros));
        }
    }

    backlog.clear();
}

void SDRPacerThread::terminate() {
    IOThread::terminate();
    SDRThreadIQDataQueuePtr iqDataInQueue =
        std::static_pointer_cast<SDRThreadIQDataQueue>(getInputQueue("IQDataInput"));
    SDRThreadIQDataQueuePtr iqDataOutQueue =
        std::static_pointer_cast<SDRThreadIQDataQueue>(getOutputQueue("IQDataOutput"));
    if (iqDataInQueue != nullptr) {
        iqDataInQueue->flush();
    }
    if (iqDataOutQueue != nullptr) {
        iqDataOutQueue->flush();
    }
}

void SDRPacerThread::setPacingEnabled(bool enabled) {
    pacingEnabled.store(enabled);
}

bool SDRPacerThread::isPacingEnabled() const {
    return pacingEnabled.load();
}

void SDRPacerThread::requestFlush() {
    flushRequested.store(true);
}
