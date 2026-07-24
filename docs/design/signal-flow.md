# Signal Flow

This document describes the complete data path from SDR hardware input to audio output.

**Last Updated:** 2026-07-23

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
    |---[pipeIQVisualData]-----> SpectrumVisualProcessor (main spectrum FFT)
    |---[pipeWaterfallIQVisualData]--> FFTDataDistributor (waterfall)
    |---[pipeDemodIQVisualData]--> SpectrumVisualProcessor (demod spectrum)
    |---[per-demod pipeIQInputData]--> DemodulatorPreThread
    v
[Stage 3] DemodulatorPreThread
    | DemodulatorThreadPostInputQueue (max 100 items)
    v
[Stage 4] DemodulatorThread
    |---[pipeAudioData]---------> AudioThread (speaker output)
    |---[audioVisOutputQueue]---> ScopeVisualProcessor (scope display)
    |---[audioSinkOutputQueue]--> AudioSinkThread (WAV recording)
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

Receives raw IQ from SDRThread and performs:
1. **DC blocking** via IIR filter (`iirfilt_crcf`)
2. **Polyphase filterbank channelization** (`firpfbch_crcf` or `firpfbch2_crcf`) — splits wideband IQ into N sub-channels
3. **Distribution** — pushes channelized data to each active demodulator's input queue

Also pushes full-rate IQ to visual processing queues for the main spectrum and waterfall displays.

### Stage 3: Demodulator Pre-Processing

**Class:** `DemodulatorPreThread` (`src/demod/DemodulatorPreThread.h`)

Per-demodulator preprocessing:
1. **Frequency shifting** — centers the desired signal using `nco_crcf`
2. **IQ resampling** — decimates to the modem's bandwidth using `msresamp_crcf`
3. **Modem attachment** — bundles `Modem*` and `ModemKit*` pointers with each IQ packet

When the modem type changes, a `DemodulatorWorkerThread` builds the new modem and filters on a background thread to avoid blocking the data flow.

### Stage 4: Demodulation

**Class:** `DemodulatorThread` (`src/demod/DemodulatorThread.h`)

Core demodulation execution:
1. Calls `Modem::demodulate(kit, input, audioOut)` — the virtual dispatch point
2. Computes signal levels (peak, RMS)
3. Applies squelch logic
4. Routes audio to output queues and visualization

### Stage 5: Audio Output

**Class:** `AudioThread` (`src/audio/AudioThread.h`)

Manages RtAudio hardware output using a **controller/bound** pattern:
- One AudioThread per physical output device acts as **controller** (owns the RtAudio stream)
- Other AudioThreads (one per demodulator) are **bound** to the controller
- The `audioCallback` (real-time audio thread) mixes audio from all bound threads

## Queue Wiring

### Global Queues (set up in `CubicSDR::OnInit()`)

| Queue Name | Type | Producer | Consumer | Max |
|------------|------|----------|----------|-----|
| `pipeSDRIQData` | `SDRThreadIQDataQueue` | SDRThread | SDRPostThread | 100 |
| `pipeIQVisualData` | `DemodulatorThreadInputQueue` | SDRPostThread | Main spectrum FFT | 1 |
| `pipeWaterfallIQVisualData` | `DemodulatorThreadInputQueue` | SDRPostThread | Waterfall FFT | 128 |
| `pipeDemodIQVisualData` | `DemodulatorThreadInputQueue` | SDRPostThread | Demod spectrum FFT | 1 |
| `pipeAudioVisualData` | `DemodulatorThreadOutputQueue` | DemodulatorThread | Scope visual | 1 |

### Per-Demodulator Queues (set up in `DemodulatorInstance` constructor)

| Queue Name | Type | Producer | Consumer | Max |
|------------|------|----------|----------|-----|
| `pipeIQInputData` | `DemodulatorThreadInputQueue` | SDRPostThread | DemodulatorPreThread | 100 |
| `pipeIQDemodData` | `DemodulatorThreadPostInputQueue` | DemodulatorPreThread | DemodulatorThread | 100 |
| `pipeAudioData` | `AudioThreadInputQueue` | DemodulatorThread | AudioThread | 100 |

## Buffer Management

### ReBuffer Pool

To avoid heap allocation on every frame, CubicSDR uses `ReBuffer<T>` (`src/IOThread.h`) — a `shared_ptr`-based buffer pool:

- `getBuffer()` returns a buffer whose `use_count` is 1 (not in transit), or allocates a new one
- When the last consumer releases a buffer, it returns to the pool
- Old buffers age out after `REBUFFER_GC_LIMIT` (100) idle cycles

Used by: SDRThread, SDRPostThread, DemodulatorPreThread, DemodulatorThread, SpectrumVisualProcessor, ScopeVisualProcessor.

### Data Types

| Type | File | Contents |
|------|------|----------|
| `SDRThreadIQData` | `src/sdr/SoapySDRThread.h` | Raw IQ samples, frequency, sample rate, DC correction flag |
| `DemodulatorThreadIQData` | `src/demod/DemodDefs.h` | Channelized IQ samples, frequency, sample rate |
| `DemodulatorThreadPostIQData` | `src/demod/DemodDefs.h` | Resampled IQ + Modem* + ModemKit* pointers |
| `AudioThreadInput` | `src/audio/AudioThread.h` | Float audio data, sample rate, channels, peak level, squelch flag |
| `SpectrumVisualData` | `src/process/SpectrumVisualProcessor.h` | FFT spectrum points, floor/ceiling, center frequency, bandwidth |
| `ScopeRenderData` | `src/process/ScopeVisualProcessor.h` | Waveform points, scope mode, FFT parameters |

## Visual Processing Pipeline

A parallel path handles display data:

```
SDRPostThread
    |
    +--[pipeIQVisualData]--> SpectrumVisualProcessor --> WaterfallCanvas (UI thread)
    +--[pipeWaterfallIQVisualData]--> FFTVisualDataThread --> SpectrumVisualProcessor --> WaterfallCanvas
    +--[pipeDemodIQVisualData]--> SpectrumVisualProcessor --> Demod spectrum display

DemodulatorThread
    |
    +--[audioVisOutputQueue]--> ScopeVisualProcessor --> ScopeCanvas (UI thread)
```

The UI thread is entirely **pull-based** — canvases poll their input queues via `try_pop()` in their `OnIdle()` handlers. Worker threads never push data to the UI.
