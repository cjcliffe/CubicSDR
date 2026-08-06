# Signal Flow

This document describes the complete data path from SDR hardware input to audio output.

## Overview

CubicSDR processes radio signals through a pipeline of thread-connected stages. Each stage runs in its own thread and communicates with adjacent stages via `ThreadBlockingQueue<T>` instances.

## Signal Pipeline

```
SDR Hardware (SoapySDR)
    |
    v
[Stage 1] SDRThread
    | SDRThreadIQDataQueue (max 100 items)
    v
[Stage 2] SDRPostThread
    |---[pipeIQVisualData]-----> SpectrumVisualDataThread (main spectrum)
    |---[pipeWaterfallIQVisualData]--> FFTVisualDataThread (waterfall)
    |---[pipeDemodIQVisualData]--> SpectrumVisualDataThread (demod spectrum + demod waterfall)
    |---[per-demod pipeIQInputData]--> DemodulatorPreThread
    v
[Stage 3] DemodulatorPreThread
    | DemodulatorThreadPostInputQueue (max 100 items)
    v
[Stage 4] DemodulatorThread
    |---[pipeAudioData]---------> AudioThread (speaker output)
    |---[audioVisOutputQueue]---> ScopeVisualProcessor (scope display)
    |---[audioSinkOutputQueue]--> AudioSinkFileThread (WAV recording)
    v
[Stage 5] AudioThread -> RtAudio hardware callback -> speakers/headphones
```

## Stage Details

### Stage 1: SDR Hardware Input

**Class:** `SDRThread` (`src/sdr/SoapySDRThread.h`)

Reads IQ samples from SoapySDR hardware in a tight loop. Samples are `liquid_float_complex` (float I/Q pairs). The thread manages:
- Device open/close
- Sample rate and frequency configuration
- Gain settings (per-channel)
- DC offset correction
- Streaming start/stop

Output: `SDRThreadIQData` containing raw IQ samples, frequency, sample rate, and DC correction status.

### Stage 2: Post-Processing / Channelization

**Class:** `SDRPostThread` (`src/sdr/SDRPostThread.h`)

Receives raw IQ from SDRThread. Behavior depends on channel count:

**Multi-channel mode** (`numChannels > 1`):
1. **Polyphase filterbank channelization** (`firpfbch_crcf` or `firpfbch2_crcf`) — splits wideband IQ into N sub-channels
2. **DC blocking** on channel 0 only via IIR filter (`iirfilt_crcf`) — other channels pass through unfiltered
3. **Distribution** — non-blocking `try_push()` each channel's data to its assigned demodulator's input queue

**Single-channel mode** (`numChannels <= 1`):
1. **DC blocking** on the entire bandwidth via IIR filter (`iirfilt_crcf`)
2. **Distribution** — non-blocking `try_push()` the full bandwidth to every active demodulator (no channelization)

In both modes, also pushes full-rate IQ (DC-corrected in single-channel, raw in multi-channel) to visual processing queues for the main spectrum and waterfall displays. **All output pushes use non-blocking `try_push()`** — both visual data and per-demodulator data can be silently dropped when queues are full. SDRPostThread never blocks on any consumer.

### Stage 3: Demodulator Pre-Processing

**Class:** `DemodulatorPreThread` (`src/demod/DemodulatorPreThread.h`)

Per-demodulator preprocessing:
1. **Frequency shifting** — centers the desired signal using `nco_crcf`
2. **IQ resampling** — decimates to the modem's bandwidth using `msresamp_crcf`
3. **Modem attachment** — bundles `Modem*` and `ModemKit*` pointers with each IQ packet

When the modem type changes, a `DemodulatorWorkerThread` builds the new modem and filters on a background thread to avoid blocking the data flow. The worker thread communicates via its own command queue (max 2) and result queue (max 100).

Output uses **blocking** `push()` — if the downstream queue is full, the pre-thread stalls until space is available.

### Stage 4: Demodulation

**Class:** `DemodulatorThread` (`src/demod/DemodulatorThread.h`)

Core demodulation execution:
1. Calls `Modem::demodulate(kit, input, audioOut)` — the virtual dispatch point
2. Computes signal level (mean magnitude, dB)
3. Applies squelch logic
4. Routes audio to output queues and visualization

All output pushes use **non-blocking** `try_push()` — if any output queue is full, data is dropped with a warning. This prevents a slow consumer (e.g. audio device) from stalling the demodulator.

### Stage 5: Audio Output

**Class:** `AudioThread` (`src/audio/AudioThread.h`)

Manages RtAudio hardware output using a **controller/bound** pattern:
- One AudioThread per physical output device acts as **controller** (owns the RtAudio stream)
- Other AudioThreads (one per demodulator) are **bound** to the controller
- A file-scope static `audioCallback` function (real-time audio thread) receives the controller AudioThread as `userData` and mixes audio from all bound threads

## Queue Wiring

### Global Queues (set up in `CubicSDR::OnInit()`)

| Queue Name | Type | Producer | Consumer | Max |
|------------|------|----------|----------|-----|
| `pipeSDRIQData` | `SDRThreadIQDataQueue` | SDRThread | SDRPostThread | 100 |
| `pipeIQVisualData` | `DemodulatorThreadInputQueue` | SDRPostThread | SpectrumVisualDataThread (main spectrum) | 1 |
| `pipeWaterfallIQVisualData` | `DemodulatorThreadInputQueue` | SDRPostThread | FFTVisualDataThread (waterfall) | 128 |
| `pipeDemodIQVisualData` | `DemodulatorThreadInputQueue` | SDRPostThread | SpectrumVisualDataThread (demod spectrum) | 1 |
| `pipeAudioVisualData` | `DemodulatorThreadOutputQueue` | DemodulatorThread | ScopeVisualProcessor | 1 |

### Per-Demodulator Queues (set up in `DemodulatorInstance` constructor)

| Queue Name | Type | Producer | Consumer | Max |
|------------|------|----------|----------|-----|
| `pipeIQInputData` | `DemodulatorThreadInputQueue` | SDRPostThread | DemodulatorPreThread | 100 |
| `pipeIQDemodData` | `DemodulatorThreadPostInputQueue` | DemodulatorPreThread | DemodulatorThread | 100 |
| `pipeAudioData` | `AudioThreadInputQueue` | DemodulatorThread | AudioThread | 100 |
| `audioVisOutputQueue` | `DemodulatorThreadOutputQueue` | DemodulatorThread | ScopeVisualProcessor | default |
| `audioSinkOutputQueue` | `DemodulatorThreadOutputQueue` | DemodulatorThread | AudioSinkFileThread | default |

Note: `audioVisOutputQueue` is a per-DemodulatorThread member that is bound at runtime to the global `pipeAudioVisualData` queue via `setOutputQueue("AudioVisualOutput", ...)`. `audioSinkOutputQueue` is bound dynamically only when WAV recording starts.

Note: `DemodulatorThreadOutputQueue` and `AudioThreadInputQueue` are both aliases for `ThreadBlockingQueue<AudioThreadInputPtr>` (defined in `src/audio/AudioThread.h`). They carry the same data type; the distinct names reflect the queue's role in the pipeline.

## Buffer Management

### ReBuffer Pool

To avoid heap allocation on every frame, CubicSDR uses `ReBuffer<T>` (`src/IOThread.h`) — a `shared_ptr`-based buffer pool:

- `getBuffer()` scans for a buffer with `use_count == 1` (not in transit), resets its age, and returns it; if none available, allocates a new one
- When the last consumer releases a buffer, its `use_count` drops to 1 and it becomes available for reuse
- Idle buffers age out: each time a buffer is selected, other unused buffers have their age decremented. When the oldest buffer's age drops below `-REBUFFER_GC_LIMIT` (i.e. age < -100), it is garbage collected
- A warning is emitted if the pool exceeds `REBUFFER_WARNING_THRESHOLD` (2000) buffers

Used by: SDRThread, SDRPostThread, DemodulatorPreThread, DemodulatorThread, SpectrumVisualProcessor, ScopeVisualProcessor.

### Data Types

| Type | File | Contents |
|------|------|----------|
| `SDRThreadIQData` | `src/sdr/SoapySDRThread.h` | Raw IQ samples, frequency, sample rate, DC correction flag, channel count |
| `DemodulatorThreadIQData` | `src/demod/DemodDefs.h` | Channelized IQ samples, frequency, sample rate |
| `DemodulatorThreadPostIQData` | `src/demod/DemodDefs.h` | Resampled IQ, sample rate, modem name/type, Modem* + ModemKit* pointers |
| `AudioThreadInput` | `src/audio/AudioThread.h` | Float audio data, frequency, input rate, sample rate, channels, peak level, type, squelch flag |
| `SpectrumVisualData` | `src/process/SpectrumVisualProcessor.h` | FFT spectrum points, hold points, floor/ceiling, center frequency, bandwidth |
| `ScopeRenderData` | `src/process/ScopeVisualProcessor.h` | Waveform points, scope mode, input/sample rate, channels, spectrum flag, FFT size/floor/ceiling |

## Visual Processing Pipeline

A parallel path handles display data:

```
SDRPostThread
    |
    +--[pipeIQVisualData]--> SpectrumVisualDataThread --> SpectrumCanvas (UI thread)
    +--[pipeWaterfallIQVisualData]--> FFTVisualDataThread --> WaterfallCanvas
    +--[pipeDemodIQVisualData]--> SpectrumVisualDataThread --> demodSpectrumCanvas + demodWaterfallCanvas

DemodulatorThread
    |
    +--[audioVisOutputQueue]--> ScopeVisualProcessor --> ScopeCanvas (UI thread)
```

For the visual data processing pipeline internals (VisualProcessor, distributors, FFT processing, ScopeVisualProcessor), see [visual-data-pipeline.md](visual-data-pipeline.md). For the canvas pull model and rendering flow (OnIdle vs OnPaint), see [visual-rendering.md](visual-rendering.md).
