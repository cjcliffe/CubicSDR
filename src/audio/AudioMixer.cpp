// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AudioMixer.h"

double AudioMixer::mixVoice(MixerVoice &voice, float *out, unsigned int nFrames, int deviceRate, long long nowMs) {
    // An empty queue alone never skips: currentInput may still hold playable
    // samples, and every pop site below already handles an empty queue.
    if (!voice.prebufferDone) {
        // Hold silent until the queue holds a full prebuffer of production;
        // without this lead every scheduling stall would gap live audio.
        // The backlog accumulates instead of draining, and the first mix
        // below still drops buildup older than the keep window.
        if (voice.prebufferStartMs == 0) {
            voice.prebufferStartMs = nowMs;
        }
        if (nowMs - voice.prebufferStartMs < AUDIOTHREAD_PREBUFFER_MS) {
            return 0.0;
        }
        voice.prebufferDone = true;
    }

    if (!voice.currentInput) {
        voice.audioQueuePtr = 0;

        // Drop stale startup buildup by age: anything older than the keep
        // window behind the newest buffer is gone, while everything live
        // plays through in order. Playback starts immediately below:
        // skipping this callback would add a full buffer of silence to
        // every resume.
        AudioThreadInputPtr oldest, newest, drop;
        while (voice.inputQueue->try_front(oldest) && voice.inputQueue->try_back(newest)
               && oldest && newest
               && newest->producedMs - oldest->producedMs > AUDIOTHREAD_RESUME_KEEP_MS) {
            if (!voice.inputQueue->try_pop(drop)) {
                break;
            }
        }

        if (!voice.inputQueue->try_pop(voice.currentInput)) {
            return 0.0;
        }
    }

    if (voice.currentInput->sampleRate != deviceRate) {

        // Without a match the stale buffer unloads instead of mixing: playing
        // it would emit wrong-rate audio for a pass.
        bool matched = false;
        while (voice.inputQueue->try_pop(voice.currentInput)) {

            if (voice.currentInput && voice.currentInput->sampleRate == deviceRate) {
                matched = true;
                break;
            }
            voice.currentInput = nullptr;
        } //end while

        voice.audioQueuePtr = 0;

        if (!matched) {
            voice.currentInput = nullptr;
            return 0.0;
        }
    }


    if (voice.currentInput->channels == 0 || voice.currentInput->data.empty()) {
        if (!voice.inputQueue->empty()) {
            voice.audioQueuePtr = 0;
            if (voice.currentInput) {

                voice.currentInput = nullptr;
            }

            if (!voice.inputQueue->try_pop(voice.currentInput)) {
                return 0.0;
            }
        }
        return 0.0;
    }

    double mixPeak = voice.currentInput->peak * voice.gain;
    bool mixed = false;

    if (voice.currentInput->channels == 1) {

        for (unsigned int i = 0; i < nFrames; i++) {

            if (voice.audioQueuePtr >= voice.currentInput->data.size()) {
                voice.audioQueuePtr = 0;
                if (voice.currentInput) {

                    voice.currentInput = nullptr;
                }

                if (!voice.inputQueue->try_pop(voice.currentInput)) {
                    break;
                }


                double srcPeak = voice.currentInput->peak * voice.gain;
                if (mixPeak < srcPeak) {
                    mixPeak = srcPeak;
                }
            }
            if (voice.currentInput && !voice.currentInput->data.empty()) {
                mixed = true;
                float v = voice.currentInput->data[voice.audioQueuePtr] * voice.gain;
                out[i * 2] += v;
                out[i * 2 + 1] += v;
            }
            voice.audioQueuePtr++;
        }
    }
    else {
        for (unsigned int i = 0, iMax = voice.currentInput->channels * nFrames; i < iMax; i++) {

            if (voice.audioQueuePtr >= voice.currentInput->data.size()) {
                voice.audioQueuePtr = 0;
                if (voice.currentInput) {

                    voice.currentInput = nullptr;
                }

                if (!voice.inputQueue->try_pop(voice.currentInput)) {
                    break;
                }

                double srcPeak = voice.currentInput->peak * voice.gain;
                if (mixPeak < srcPeak) {
                    mixPeak = srcPeak;
                }
            }
            if (voice.currentInput && !voice.currentInput->data.empty()) {
                mixed = true;

                out[i] = out[i] + voice.currentInput->data[voice.audioQueuePtr] * voice.gain;
            }
            voice.audioQueuePtr++;
        }
    }

    // A pass that mixed nothing contributes no peak; returning the latched
    // peak would spuriously scale the other voices down in normalize.
    return mixed ? mixPeak : 0.0;
}

void AudioMixer::normalize(float *out, unsigned int nFrames, double peak) {
    if (peak > 1.0) {
        float invPeak = (float)(1.0 / peak);

        for (unsigned int i = 0; i < nFrames * 2; i++) {
            out[i] *= invPeak;
        }
    }
}
