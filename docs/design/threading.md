# Threading Model

This document describes CubicSDR's threading architecture, synchronization mechanisms, and thread lifecycle management.

## Thread Infrastructure

### IOThread Base Class

**File:** `src/IOThread.h`

All worker threads inherit from `IOThread`, which provides:
- **Lifecycle management:** `stopping` (atomic bool) for async termination; `terminated` (atomic bool) for completion
- **Named queue bindings:** `setInputQueue(name, queue)` / `setOutputQueue(name, queue)` with string-keyed maps
- **Thread entry:** `threadMain()` wraps `run()` in try/catch, sets both `terminated` and `stopping` to `true` on exit
- **Spin-wait sleep:** `isTerminated(timeout)` busy-waits with 5ms sleep (`SPIN_WAIT_SLEEP_MS`) between checks

### Thread Creation Pattern

CubicSDR uses `std::thread` exclusively (no wxThread):

```cpp
threadObject = new ThreadClass(...);
t_stdThread = new std::thread(&ThreadClass::threadMain, threadObject);
```

On macOS, `DemodulatorPreThread` and `DemodulatorThread` use `pthread_create` with ~2MB stack sizes (2048000 bytes).

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
| Audio Sink (N) | `AudioSinkThread` | `src/audio/AudioSinkThread.h` | Base audio sink for demodulator output |
| Audio Sink File (N) | `AudioSinkFileThread` | `src/audio/AudioSinkFileThread.h` | WAV file recording |
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
| `std::atomic_bool` | SDRThread, DemodulatorPreThread, DemodulatorThread, DemodulatorInstance, CubicSDR | Lock-free parameter change signaling |
| `std::recursive_mutex` | AudioThread, AudioSinkThread, DemodulatorInstance, DemodulatorMgr, BookmarkMgr | Protecting shared mutable state with re-entrant access |
| `std::mutex` | IOThread queue bindings, SDRThread settings/gains, VisualProcessor, SpectrumVisualProcessor, AppConfig, WaterfallCanvas, DigitalConsole, DemodulatorThread (squelch lock) | Protecting infrequent mutations and visualization state |

### SpinMutex

**File:** `src/util/SpinMutex.h`

Non-recursive spinlock using `std::atomic_flag` with acquire/release memory ordering. Used as the internal lock for `ThreadBlockingQueue` and `ReBuffer` due to its low overhead for short critical sections.

### ReBuffer Pooling

**File:** `src/IOThread.h`

`ReBuffer<BufferType>` is a buffer pool with reference-counted recycling, used to minimize allocation overhead in hot data paths (audio output, visualization). `getBuffer()` checks each pooled `shared_ptr` — if `use_count() == 1`, the buffer is available for reuse. The first available buffer is selected (age reset to 1); other available buffers have their age decremented. Buffers that go unused age and are garbage-collected when `age < -REBUFFER_GC_LIMIT` (i.e. below -100). Used by `DemodulatorThread` (audio output buffers) and `VisualDataReDistributor` (visualization buffers).

### VisualProcessor Pipeline

**File:** `src/process/VisualProcessor.h`

`VisualProcessor<Input, Output>` is a template base class for the visualization pipeline. Each processor has one input queue and N output queues. `process()` is called by the owning thread's main loop; `distribute()` pushes results to all attached outputs. Two concrete subclasses handle different distribution strategies:
- `VisualDataDistributor` — zero-copy shared pointer forwarding
- `VisualDataReDistributor` — deep-copy distribution via `ReBuffer` pooling

Protected by `std::mutex busy_update` for queue list mutations.

### SDREnumerator One-Shot Spawning

`SDREnumerator` threads are spawned as needed (device refresh, remote add, re-enumeration) without joining the previous instance. Each call to `threadMain` performs a single enumeration pass and exits. The old thread pointer is overwritten without cleanup — a deliberate fire-and-forget pattern.

## Thread Lifecycle

### Startup Sequence

In `CubicSDR::OnInit()` (`src/CubicSDR.cpp`):

1. Queue plumbing — all global queues created and wired
2. `SpectrumVisualDataThread` started
3. `DemodVisualDataThread` started (if enabled)
4. `SDRPostThread` started
5. `SDREnumerator` created
6. `AppFrame` created (wxWidgets main window)
7. `SDREnumerator` thread started
8. Device selection triggers `SDRThread` start (in `CubicSDR::setDevice()`)

### Per-Demodulator Startup

When `DemodulatorInstance::run()` is called:

1. `AudioThread` started
2. `DemodulatorPreThread` started
3. `DemodulatorThread` started

### Shutdown Sequence

In `CubicSDR::OnExit()`:

1. `RigThread::terminate()` — stops hamlib rig control (if active)
2. `SDRThread::terminate()` — stops producing IQ data (waited up to 3s)
3. `SDRPostThread::terminate()` — stops channelizing (waited up to 3s)
4. `DemodulatorMgr::terminateAll()` — terminates all demodulator instances (queues flushed inside each `DemodulatorInstance::terminate()`)
5. Visual processor threads terminated (waited up to 1s each)
6. All threads joined

If any termination step times out, the application calls `::exit()` with a platform-specific error code rather than risk hanging indefinitely.

### Per-Demodulator Shutdown

`DemodulatorInstance::terminate()`:

1. `AudioThread::terminate()` — stops consuming audio
2. `DemodulatorThread::terminate()` — stops demodulating
3. `DemodulatorPreThread::terminate()` — stops resampling (also terminates worker thread)
4. All queues flushed to unblock pending pushes

### Known Issues

In `DemodulatorInstance::isTerminated()`, the macOS cleanup path for the audio thread calls `pthread_join(t_PreDemod, NULL)` instead of `pthread_join(t_Audio, NULL)`. At that point `t_PreDemod` has already been joined and set to `nullptr`, so this is a call to `pthread_join(NULL, ...)` which is undefined behavior per POSIX. The non-macOS path (`t_Audio->join()`) is correct. This is a copy-paste bug in `src/demod/DemodulatorInstance.cpp`.

## Thread Priorities (macOS)

On macOS, threads are assigned scheduling priorities:

| Thread | Policy | Priority |
|--------|--------|----------|
| SDR Post-Processing | `SCHED_FIFO` | `sched_get_priority_max` |
| Demodulator Pre-Thread | `SCHED_FIFO` | max - 1 |
| Demodulator Thread | `SCHED_FIFO` | max - 1 |
| Audio Thread (controller) | `SCHED_RR` | max - 1 |
| Audio Sink Thread | `SCHED_RR` | max - 1 |

Note: SDR Thread has `SCHED_FIFO` priority code but it is currently commented out.

Windows and Linux use default thread priorities.

## wxWidgets Integration

The UI thread is **entirely pull-based**:

1. `AppFrame::OnIdle()` is called continuously by the wx event loop and handles device params, modem properties, and UI state
2. Each canvas registers its own `EVT_IDLE` handler independently (e.g., `WaterfallCanvas::OnIdle`, `SpectrumCanvas::OnIdle`, `ScopeCanvas::OnIdle`) — these call `processInputQueue()` or `try_pop()` directly (non-blocking)
3. Worker threads never push data to the UI
4. Shared state uses `std::atomic` variables (frequency, signal levels, mute state)
5. UI-initiated changes go through atomic variables and flags, not wx events
