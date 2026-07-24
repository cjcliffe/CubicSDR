# Threading Model

This document describes CubicSDR's threading architecture, synchronization mechanisms, and thread lifecycle management.

**Last Updated:** 2026-07-23

## Thread Infrastructure

### IOThread Base Class

**File:** `src/IOThread.h`

All worker threads inherit from `IOThread`, which provides:
- **Lifecycle management:** `stopping` (atomic bool) for async termination; `terminated` (atomic bool) for completion
- **Named queue bindings:** `setInputQueue(name, queue)` / `setOutputQueue(name, queue)` with string-keyed maps
- **Thread entry:** `threadMain()` wraps `run()` in try/catch, sets `terminated=true` on exit
- **Heartbeat:** 50ms pop timeout (`HEARTBEAT_CHECK_PERIOD_MICROS`) allows periodic `stopping` flag checks

### Thread Creation Pattern

CubicSDR uses `std::thread` exclusively (no wxThread):

```cpp
threadObject = new ThreadClass(...);
t_stdThread = new std::thread(&ThreadClass::threadMain, threadObject);
```

On macOS, `DemodulatorPreThread` and `DemodulatorThread` use `pthread_create` with 2MB stack sizes.

## Thread Inventory

| Thread | Class | File | Purpose |
|--------|-------|------|---------|
| Main (wxWidgets UI) | wxApp/wxFrame | `AppFrame.cpp` | Event loop, OpenGL rendering, UI interaction |
| SDR Thread | `SDRThread` | `src/sdr/SoapySDRThread.h` | Reads IQ samples from SoapySDR hardware |
| SDR Post-Processing | `SDRPostThread` | `src/sdr/SDRPostThread.h` | DC blocking, channelization, IQ distribution |
| SDR Enumerator | `SDREnumerator` | `src/sdr/SDREnumerator.h` | Background device discovery |
| Spectrum Visual | `SpectrumVisualDataThread` | `src/process/SpectrumVisualDataThread.h` | FFT processing for main spectrum display |
| Demod Visual | `SpectrumVisualDataThread` | `src/process/SpectrumVisualDataThread.h` | FFT processing for demod spectrum display |
| Waterfall FFT | `FFTVisualDataThread` | `src/process/FFTVisualDataThread.h` | Rate-limited waterfall FFT data |
| Demodulator Pre-Thread (N) | `DemodulatorPreThread` | `src/demod/DemodulatorPreThread.h` | Frequency shifting, resampling, modem switching |
| Demodulator Worker (N) | `DemodulatorWorkerThread` | `src/demod/DemodulatorWorkerThread.h` | Offloaded modem/filter creation |
| Demodulator Thread (N) | `DemodulatorThread` | `src/demod/DemodulatorThread.h` | Core demodulation, squelch, signal levels |
| Audio Thread (N) | `AudioThread` | `src/audio/AudioThread.h` | Per-demod audio processing and device binding |
| Audio Controller | `AudioThread` | `src/audio/AudioThread.h` | Per-device RtAudio stream owner and mixer |
| Audio Sink (N) | `AudioSinkThread` | `src/audio/AudioSinkThread.h` | WAV file recording |
| Rig Control (optional) | `RigThread` | `src/rig/RigThread.h` | Hamlib CAT control |

## Communication Patterns

### Pattern 1: Queue-Based Data Flow (Primary)

All data-carrying threads communicate via `ThreadBlockingQueue<T>`:
- **Blocking push/pop** for critical data (demod pipeline, worker commands)
- **Non-blocking try_push/try_pop** for visualization (data loss acceptable)
- All data passed as `std::shared_ptr<T>`

### Pattern 2: Atomic Flags for Control

`std::atomic_bool` flags signal parameter changes between UI and worker threads:
- `freq_changed`, `rate_changed`, `bandwidthChanged`
- `demodTypeChanged`, `modemSettingsChanged`
- The polling thread checks flags each iteration and applies changes

### Pattern 3: Controller/Bound Audio Mixing

- One AudioThread per physical output device owns the RtAudio stream (**controller**)
- Other AudioThreads bind to the controller via `bindThread()`
- The `audioCallback` (real-time context) iterates bound threads, pops audio via `try_pop()`, and mixes into the output buffer

### Pattern 4: Worker Thread for Expensive Operations

- `DemodulatorWorkerThread` handles modem creation and filter building
- PreThread sends commands via bounded queue (max 2 items)
- Worker responds with results via separate queue (max 100 items)
- Prevents expensive liquid-dsp initialization from blocking the data path

## Synchronization Mechanisms

| Mechanism | Location | Purpose |
|-----------|----------|---------|
| `ThreadBlockingQueue<T>` | Throughout | Primary inter-thread data transfer |
| `SpinMutex` | `ThreadBlockingQueue`, `ReBuffer` | Lightweight lock for high-frequency queue operations |
| `std::atomic_bool` | SDRThread, DemodulatorPreThread, DemodulatorThread | Lock-free parameter change signaling |
| `std::recursive_mutex` | AudioThread, DemodulatorInstance, DemodulatorMgr | Protecting shared mutable state |
| `std::mutex` | IOThread queue bindings, SDRThread settings | Protecting infrequent mutations |

### SpinMutex

**File:** `src/util/SpinMutex.h`

Non-recursive spinlock using `std::atomic_flag` with acquire/release memory ordering. Used as the internal lock for `ThreadBlockingQueue` and `ReBuffer` due to its low overhead for short critical sections.

## Thread Lifecycle

### Startup Sequence

In `CubicSDR::OnInit()` (`src/CubicSDR.cpp`):

1. Queue plumbing — all global queues created and wired
2. `SpectrumVisualDataThread` started
3. `DemodVisualDataThread` started (if enabled)
4. `SDRPostThread` started
5. `AppFrame` created (wxWidgets main window)
6. `SDREnumerator` started
7. Device selection triggers `SDRThread` start (in `CubicSDR::setDevice()`)

### Per-Demodulator Startup

When `DemodulatorInstance::run()` is called:

1. `AudioThread` started
2. `DemodulatorPreThread` started
3. `DemodulatorThread` started

### Shutdown Sequence

In `CubicSDR::OnExit()`:

1. `SDRThread::terminate()` — stops producing IQ data (waited up to 3s)
2. `SDRPostThread::terminate()` — stops channelizing (waited up to 3s)
3. `DemodulatorMgr::terminateAll()` — terminates all demodulator instances
4. Visual processor threads terminated
5. All threads joined
6. All queues flushed to unblock pending operations

### Per-Demodulator Shutdown

`DemodulatorInstance::terminate()`:

1. `AudioThread::terminate()` — stops consuming audio
2. `DemodulatorThread::terminate()` — stops demodulating
3. `DemodulatorPreThread::terminate()` — stops resampling (also terminates worker thread)
4. All queues flushed to unblock pending pushes

## Thread Priorities (macOS)

On macOS, threads are assigned scheduling priorities:

| Thread | Policy | Priority |
|--------|--------|----------|
| SDR Thread | `SCHED_FIFO` | `sched_get_priority_max` |
| SDR Post-Processing | `SCHED_FIFO` | max |
| Demodulator Pre-Thread | `SCHED_FIFO` | max - 1 |
| Demodulator Thread | `SCHED_FIFO` | max - 1 |
| Audio Thread (controller) | `SCHED_RR` | max - 1 |

Windows and Linux use default thread priorities.

## wxWidgets Integration

The UI thread is **entirely pull-based**:

1. `AppFrame::OnIdle()` is called continuously by the wx event loop
2. Each canvas calls `processInputQueue()` which uses `try_pop()` (non-blocking)
3. Worker threads never push data to the UI
4. Shared state uses `std::atomic` variables (frequency, signal levels, mute state)
5. UI-initiated changes go through atomic variables and flags, not wx events
