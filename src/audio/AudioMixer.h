// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cstddef>
#include "AudioThreadInput.h"

// Milliseconds of production each voice accumulates before its first mix:
// the resulting lead absorbs scheduling stalls without losing samples, at the
// cost of this much live-monitoring latency. Recording and I/Q feeds use
// separate queues and are unaffected.
#define AUDIOTHREAD_PREBUFFER_MS (150)

// Milliseconds of audio kept behind the newest buffer when resuming onto an
// empty current input: older buildup is stale startup backlog and is dropped,
// while everything inside the window is live and plays in order. Must exceed
// AUDIOTHREAD_PREBUFFER_MS so the drain never truncates the lead.
#define AUDIOTHREAD_RESUME_KEEP_MS (200)

// Per-stream mix state. Lives in AudioThread for the realtime callback and
// stands alone in tests; the mixer advances it through the queue.
struct MixerVoice {
    AudioThreadInputQueuePtr inputQueue;
    AudioThreadInputPtr currentInput;
    size_t audioQueuePtr{};
    float gain{1.0f};
    bool prebufferDone{};
    long long prebufferStartMs{};
};

// Headless audio mixer: sums bound demodulator voices into one output buffer.
// Pure per-voice logic over caller-provided state: no locks, no I/O, no RtAudio
// and no wall-clock reads, so every behavior (prebuffer hold, stale drain,
// resume, rate match, mixing, normalization) is unit-testable. The realtime
// callback holds the voice locks and supplies the clock; tests drive the same
// entry points directly.
class AudioMixer {
public:
    // Mix one voice into out (stereo float, nFrames per channel), advancing the
    // voice. deviceRate selects playable buffers; nowMs is steady-clock
    // milliseconds driving prebuffer timing. Returns the voice peak
    // contribution for the normalization pass (0 when the voice stays silent).
    static double mixVoice(MixerVoice &voice, float *out, unsigned int nFrames, int deviceRate, long long nowMs);

    // Scale the mixed buffer when the summed peak exceeds full scale.
    static void normalize(float *out, unsigned int nFrames, double peak);
};
