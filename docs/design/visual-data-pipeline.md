# Visual Data Pipeline

This document describes CubicSDR's visual data processing pipeline: the VisualProcessor template, distribution modes, FFT/scope processing, and the thread safety model for visual data. For the rendering/UI layer (canvases, GLPanel, fonts, themes), see [visual-rendering.md](visual-rendering.md).

## Pipeline Topology

```
SDRPostThread (produces IQ data into pipe queues owned by CubicSDR)
    |
    +--[pipeIQVisualData]--------> SpectrumVisualDataThread --> SpectrumCanvas
    +--[pipeWaterfallIQVisualData]-> FFTVisualDataThread --> WaterfallCanvas
    +--[pipeDemodIQVisualData]----> SpectrumVisualDataThread (demodVisualThread) --> DemodSpectrumCanvas + DemodWaterfallCanvas

DemodulatorThread
    |
    +--[audioVisOutputQueue]------> ScopeVisualProcessor --> ScopeCanvas
```

`SpectrumVisualDataThread` (`src/process/SpectrumVisualDataThread.h`) is a dedicated IOThread that wraps a `SpectrumVisualProcessor` and runs it in its own thread. Two separate instances exist: one for the main spectrum (`spectrumVisualThread`) and one for the demod spectrum (`demodVisualThread`). `FFTVisualDataThread` wraps its own processing pipeline (containing an `FFTDataDistributor` and a `SpectrumVisualProcessor`) for waterfall data.

For the queue wiring (which queues connect which threads, queue types, and capacity limits), see [signal-flow.md](signal-flow.md) "Queue Wiring".

## VisualProcessor Template (`src/process/VisualProcessor.h`)

Base template for all visual data processors. Implements a generic 1:N pipeline with thread-safe queue attachment:

```
InputQueue → process() → distribute() → OutputQueues[]
```

**Core API:**
- `setInput(queue)` — attach input queue (protected by `busy_update` mutex)
- `attachOutput(queue)` / `removeOutput(queue)` — manage output queue list
- `run()` — captures a local copy of input, calls `process()` if input is non-empty
- `process()` — pure virtual, implemented by subclasses
- `distribute(item, timeout, errorMessage)` — pushes output to all attached output queues (locked iteration)
- `flushQueues()` — flushes both input and all outputs
- `isInputEmpty()` / `isOutputEmpty()` / `isAnyOutputEmpty()` — query queue states. Note: `isOutputEmpty()` returns true only when **all** output queues have room for more data (i.e., all are not full). `isAnyOutputEmpty()` returns true when **any** single output has room. Processors use `isOutputEmpty()` to skip processing when any consumer is backed up (backpressure).

**Synchronization:** `busy_update` (std::mutex) protects the `input` and `outputs` vectors. Processors call `isOutputEmpty()` before processing to avoid redundant work when consumers are backed up.

## Distribution Modes

Two distribution strategies handle different multicast patterns:

**`VisualDataDistributor<T>`** — Zero-copy shared dispatch. Pops each input item and pushes the same `shared_ptr` to all output queues. Stops pushing when all outputs are full (backpressure). Used when consumers only read the data.

**`VisualDataReDistributor<T>`** — Deep-copy dispatch via `ReBuffer` pool. Each output gets its own copy of the data, allocated from a pre-pooled buffer to avoid per-frame allocation. Used when consumers modify or consume the data independently.

Both `VisualDataDistributor` and `VisualDataReDistributor` are defined inline in `VisualProcessor.h`.

## FFTDataDistributor (`src/process/FFTDataDistributor.h`)

Specialized rate-limited distributor for IQ-to-FFT batching. Inherits from `VisualProcessor` and implements:
- Rate limiting via `lineRateAccum` / `linesPerSecond` to control FFT execution pace
- Internal buffering with `bufferMax`, `bufferOffset`, `bufferedItems` to batch IQ packets into FFT-sized chunks
- Uses non-blocking `distribute()` push (unlike the blocking push in base `VisualProcessor`)
- This is the class used by `FFTVisualDataThread` (as `fftDistrib`), not `VisualDataDistributor`

## SpectrumVisualProcessor — Full Per-Frame Pipeline

The most complex processor. Converts raw IQ samples into display-ready spectrum points.

**Input:** `DemodulatorThreadIQData` (IQ samples with sample rate, center frequency metadata)
**Output:** `SpectrumVisualData` (interleaved [x,y] spectrum points, floor/ceiling, metadata)

**Processing sequence:**

1. **Guard check** — Skip if any output queue is full (backpressure from consumers) or input is empty
2. **Pop IQ data** — Blocking pop with 50ms timeout (`HEARTBEAT_CHECK_PERIOD_MICROS`)
3. **View mode resampling** — If viewing a sub-band:
   - Compute resample ratio from center frequency offset
   - Frequency-shift using NCO (`nco_crcf_mix_block_up/down`)
   - Resample to FFT input size (`msresamp_crcf_execute`)
4. **FFT execution** — `fft_execute(fftPlan)` (liquid-dsp FFT, internal size = `DEFAULT_FFT_SIZE * SPECTRUM_VZM` = 4096 points; user-facing display resolution = 2048 points)
5. **Magnitude computation** — `sqrt(real² + imag²)` with FFT shift (swap halves to center DC)
6. **Smoothing** — Double exponential moving average with NaN guards (note: `maa` is updated first, using the *previous* frame's `ma` value, then `ma` is updated from raw input):
    - `if (fft_result_maa != fft_result_maa) fft_result_maa = fft_result` (NaN guard)
    - `fft_result_maa += (fft_result_ma - fft_result_maa) * fft_average_rate` (uses old `ma`)
    - `if (fft_result_ma != fft_result_ma) fft_result_ma = fft_result` (NaN guard)
    - `fft_result_ma += (fft_result - fft_result_ma) * fft_average_rate`
   - Initial `fft_average_rate = 0.65f`
   
   **Note:** `ScopeVisualProcessor` uses a different update order within each iteration — `ma` is updated first from raw input, then `maa` is updated using the *new* `ma`. In `SpectrumVisualProcessor`, the order is reversed: `maa` is updated first using the *old* `ma`, then `ma` is updated from raw input.
7. **Floor/ceiling tracking** — Slow-moving averages (0.05 rate) with NaN guards; peak hold if enabled
8. **Log-scale normalization** — Maps FFT bins to [0,1] spectrum points using floor/ceiling
9. **DC spike removal** — If `hideDC` enabled, interpolates over ±2kHz around DC
10. **Distribute** — Push `SpectrumVisualData` to all attached output queues

**Key constants:**

| Constant | Value | Purpose |
|----------|-------|---------|
| `HEARTBEAT_CHECK_PERIOD_MICROS` | 50,000 (50ms) | Input pop timeout |
| `DEFAULT_FFT_SIZE` | 2048 | User-facing FFT bin count |
| `SPECTRUM_VZM` | 2 | Internal FFT multiplier (actual FFT = `DEFAULT_FFT_SIZE * SPECTRUM_VZM` = 4096 points) |
| `PEAK_RESET_COUNT` | 30 | Frames before peak hold resets |
| `fft_average_rate` | 0.65f | Smoothing factor (higher = more responsive) |

## FFTVisualDataThread — The Glue Thread

A dedicated thread that bridges IQ data to the waterfall display:

```
pipeIQDataIn → fftDistrib → fftQueue → wproc → pipeFFTDataOut
```

The thread loop:
1. Sleep ~10ms between iterations
2. `fftDistrib.run()` — packages IQ data into FFT-ready batches (rate-limited by `FFT_DISTRIBUTOR_BUFFER_IN_SECONDS = 0.250s`)
3. `wproc.run()` — executes FFT processing in a tight loop until input is drained (one FFT per iteration)

This thread bridges IQ data to the waterfall display by running the FFT distributor and processor in a tight loop. The FFT distributor batches incoming IQ packets into FFT-sized chunks, and the processor executes one FFT per batch. Note: while `FFTDataDistributor` supports multiple outputs, `FFTVisualDataThread` attaches only one (`fftQueue`). The multi-consumer distribution (to both waterfall and spectrum) happens at the `SDRPostThread` level, which attaches separate queues to each consumer.

## ScopeVisualProcessor

Processes demodulated audio for scope/spectrum display.

**Input:** `AudioThreadInput` (audio samples with sample rate)
**Output:** `ScopeRenderData` (waveform points or FFT spectrum)

**Display modes:**
- `SCOPE_MODE_Y` — Single-channel time waveform
- `SCOPE_MODE_2Y` — Dual-channel overlaid waveforms
- `SCOPE_MODE_XY` — Lissajous figure (phase display)
- Spectrum mode — FFT of demodulated audio (default 1024 points); controlled by a separate boolean flag (`renderData->spectrum`) rather than a fourth `ScopeMode` enum value

Uses `try_pop` (non-blocking) instead of blocking pop, since audio data arrives at a fixed rate and stale data should be dropped.

`ScopeVisualProcessor` runs on the wxWidgets main thread via `AppFrame::handleScopeProcessor()` (see [threading.md](threading.md) Pattern 7), not on a dedicated IOThread.

## Thread Safety Summary

The visual data pipeline uses two synchronization mechanisms for processor-internal state:

| Mechanism | Protects | Used By |
|-----------|----------|---------|
| `busy_update` (std::mutex) | Input/output queue pointers | VisualProcessor base |
| `busy_run` (std::mutex) | All processor state (FFT buffers, settings) | SpectrumVisualProcessor |

For the canonical lock inventory (including `SpinMutex`, `ThreadBlockingQueue`, `std::shared_ptr`, and all other synchronization mechanisms in the codebase), see [threading.md](threading.md) "Synchronization Mechanisms".

For the `ReBuffer` pool mechanics (used by `VisualDataReDistributor` for deep-copy distribution), see [signal-flow.md](signal-flow.md) "Buffer Management".
