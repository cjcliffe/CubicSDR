# Threading Model

This document describes CubicSDR's threading architecture, synchronization mechanisms, and thread lifecycle management.

## Thread Infrastructure

### IOThread Base Class

**File:** `src/IOThread.h`

All worker threads inherit from `IOThread`, which provides:
- **Lifecycle management:** `stopping` (atomic bool, `protected`) for async termination; `terminated` (atomic bool, `private`) for completion
- **Named queue bindings:** `setInputQueue(name, queue)` / `setOutputQueue(name, queue)` with string-keyed maps
- **Thread entry:** `threadMain()` resets both flags to `false` at entry, wraps `run()` in try/catch, sets both `terminated` and `stopping` to `true` on exit (and re-throws exceptions after setting flags)
- **Spin-wait sleep:** `isTerminated(timeout)` busy-waits with 5ms sleep (`SPIN_WAIT_SLEEP_MS`) between checks

### Thread Creation Pattern

CubicSDR does not use wxThread. The standard pattern is `std::thread`:

```cpp
threadObject = new ThreadClass(...);
t_stdThread = new std::thread(&ThreadClass::threadMain, threadObject);
```

Exception: on macOS, `DemodulatorPreThread` and `DemodulatorThread` use `pthread_create` with ~2MB stack sizes (2048000 bytes) to control the stack size directly.

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
| Audio Sink File (N) | `AudioSinkFileThread` | `src/audio/AudioSinkFileThread.h` | Audio file recording |
| Rig Control (optional) | `RigThread` | `src/rig/RigThread.h` | Hamlib CAT control |

## Communication Patterns

### Pattern 1: Queue-Based Data Flow (Primary)

All data-carrying threads communicate via `ThreadBlockingQueue<T>`:
- **Blocking push/pop** for critical data (demod pipeline, worker commands)
- **Non-blocking try_push/try_pop** for visualization (data loss acceptable)
- Most queue items are `std::shared_ptr<T>` (IQ data, audio data); worker command/result queues use value types

### Pattern 2: Atomic Flags for Control

`std::atomic_bool` flags signal parameter changes between UI and worker threads:
- **SDRThread:** `freq_changed`, `rate_changed`, `offset_changed`, `antenna_changed`, `ppm_changed`, `device_changed`, `agc_mode_changed`, `gain_value_changed`, `setting_value_changed`, `frequency_locked`, `frequency_lock_init`, `iq_swap`, `hasPPM`, `hasHardwareDC`, `agc_mode`
- **DemodulatorPreThread:** `frequencyChanged`, `bandwidthChanged`, `sampleRateChanged`, `audioSampleRateChanged`, `demodTypeChanged`, `modemSettingsChanged`, `initialized`
- **DemodulatorInstance:** `active`, `muted`, `deltaLock`, `recording`, `follow`, `tracking`
- **SDRPostThread:** `doRefresh` — signals the channelizer to re-initialize on the next processing loop
- **CubicSDR:** `devicesReady`, `devicesFailed`, `soloMode`, `shuttingDown`
- The polling thread checks flags each iteration and applies changes

### Pattern 3: Controller/Bound Audio Mixing

- One AudioThread per physical output device owns the RtAudio stream (**controller**)
- Other AudioThreads bind to the controller via `bindThread()`
- The `audioCallback` (real-time context) iterates bound threads, pops audio via `try_pop()`, and mixes into the output buffer
- The controller `AudioThread` destructor calls `controllerThread->join()` **without** acquiring `m_mutex` — intentional to avoid deadlocks; safe because it only runs after all bound threads have detached
- The `audioCallback` acquires `std::recursive_mutex` (the controller's `m_mutex` and each bound thread's `m_mutex`) — this is a potential priority inversion risk in the real-time audio callback

### Pattern 4: Worker Thread for Expensive Operations

- `DemodulatorWorkerThread` handles modem creation and filter building
- PreThread sends commands via bounded queue (max 2 items)
- Worker responds with results via separate queue (max 100 items)
- Prevents expensive liquid-dsp initialization from blocking the data path

### Pattern 5: Callback Notification (Worker → UI)

- SDRThread and SDREnumerator call `sdrThreadNotify()`/`sdrEnumThreadNotify()` on `CubicSDR` from worker threads. SDRThread calls both methods for different purposes: it uses `sdrEnumThreadNotify` for progress messages (e.g. "Initializing device") and `sdrThreadNotify` for final states (`SDR_THREAD_INITIALIZED`, `SDR_THREAD_FAILED`). SDREnumerator calls only `sdrEnumThreadNotify`.
- These methods store messages in `std::string notifyMessage` protected by `std::mutex notify_busy`
- `sdrEnumThreadNotify` sets `std::atomic_bool` flags (`devicesReady` on `SDR_ENUM_DEVICES_READY`, `devicesFailed` on `SDR_ENUM_FAILED`); `sdrThreadNotify` stores messages but does not set these flags
- The UI polls these in `SDRDevices` dialog via `getNotification()`
- Note: `sdrThreadNotify` with `SDR_THREAD_INITIALIZED` also calls `appframe->initDeviceParams()` directly — a cross-thread write to internal AppFrame state (pointer + `deviceChanged` atomic flag, no UI widget manipulation)
- Worker threads also set atomic flags for UI updates: `DemodulatorWorkerThread` calls `notifyUpdateModemProperties()` which sets `AppFrame::modemPropertiesUpdated`; UI polls this in `OnIdle()`

### Pattern 6: VisualProcessor Pipeline (Threaded Distribution)

- `VisualProcessor<Input, Output>` bridges FFT threads to UI canvases
- Each processor has one input queue and N output queues
- Two distribution strategies:
  - `VisualDataDistributor` — zero-copy shared pointer forwarding
  - `VisualDataReDistributor` — deep-copy via `ReBuffer` pooling
- Protected by `std::mutex busy_update` for queue list mutations

## Synchronization Mechanisms

| Mechanism | Location | Purpose |
|-----------|----------|---------|
| `ThreadBlockingQueue<T>` | Throughout | Primary inter-thread data transfer |
| `std::condition_variable_any` | `ThreadBlockingQueue` | Enables blocking push/pop with `SpinMutex` |
| `SpinMutex` | `ThreadBlockingQueue`, `ReBuffer`, `DemodulatorThread`, `GLFont` | Lightweight lock for high-frequency queue operations and dynamic rebinding |
| `std::atomic<T>` | SDRThread, DemodulatorPreThread, DemodulatorThread, DemodulatorInstance, CubicSDR, IOThread, AudioThread, AppFrame, FFTVisualDataThread, WaterfallCanvas | Lock-free parameter change signaling and state management |
| `std::recursive_mutex` | AudioThread, AudioSinkThread, DemodulatorInstance, DemodulatorMgr, BookmarkMgr | Protecting shared mutable state with re-entrant access |
| `std::mutex` | IOThread (`m_queue_bindings_mutex`), SDRThread (`setting_busy`, `gain_busy`), VisualProcessor (`busy_update`), SpectrumVisualProcessor (`busy_run`), DeviceConfig (`busy_lock`), WaterfallCanvas (`tex_update`), DigitalConsole (`stream_busy`), ModemDigitalOutputConsole (`stream_busy`), DemodulatorThread (static `squelchLockMutex`), CubicSDR (`notify_busy`) | Protecting infrequent mutations and visualization state |

### SpinMutex

**File:** `src/util/SpinMutex.h`

Non-recursive spinlock using `std::atomic_flag` with acquire/release memory ordering. Used as the internal lock for `ThreadBlockingQueue` and `ReBuffer` due to its low overhead for short critical sections.

### ReBuffer Pooling

**File:** `src/IOThread.h`

`ReBuffer<BufferType>` is a buffer pool with reference-counted recycling, used to minimize allocation overhead in hot data paths (audio output, visualization). `getBuffer()` iterates all pooled `shared_ptr` entries — if `use_count() == 1`, the buffer is available for reuse. The first available buffer is selected (age reset to 1); other available buffers have their age decremented. GC only checks the last element in the deque: if its age drops below `-REBUFFER_GC_LIMIT` (i.e. below -100), it is popped. A `use_count() == 0` (dangling pointer) is treated as a bug and erased with a warning. Used by `DemodulatorThread` (audio output buffers) and `VisualDataReDistributor` (visualization buffers).

### VisualProcessor Pipeline

**File:** `src/process/VisualProcessor.h`

`VisualProcessor<Input, Output>` is a template base class for the visualization pipeline. Each processor has one input queue and N output queues. `process()` is called by the owning thread's main loop; `distribute()` pushes results to all attached outputs. Two concrete subclasses handle different distribution strategies:
- `VisualDataDistributor` — zero-copy shared pointer forwarding
- `VisualDataReDistributor` — deep-copy distribution via `ReBuffer` pooling

Protected by `std::mutex busy_update` for queue list mutations.

### SpectrumVisualProcessor busy_run

**File:** `src/process/SpectrumVisualProcessor.h`

`spectrumVisualProcessor` uses `std::mutex busy_run` to serialize FFT computation against parameter changes. The mutex protects all internal state: FFT plan, buffers, averaging accumulators, resampler, frequency shifter, and configuration fields. All setter/getter methods (called from the UI thread) acquire this mutex. `process()` uses a two-phase locking pattern: a short-lived scoped lock checks and clears `fftSizeChanged` (then releases before calling `setup()` outside the lock), followed by an `input->pop()` also outside the lock, then re-acquires `busy_run` for the remainder of the FFT computation (lines 245 onward). This means UI parameter changes block until the current FFT completes, and vice versa, but input polling and setup are not held up by the computation mutex.

### SDREnumerator One-Shot Spawning

`SDREnumerator` threads are spawned as needed (device refresh, remote add, re-enumeration) without joining the previous instance. Each call to `threadMain` performs a single enumeration pass and exits. The old thread pointer is overwritten without cleanup — a deliberate fire-and-forget pattern. This leaks the `std::thread` object; if the old thread is still running when overwritten, the `std::thread` destructor calls `std::terminate()` per the C++ standard. In practice the old thread has usually completed before a new one is spawned, but the race is not guaranteed. Additionally, `OnExit()` does not join or delete `t_SDREnum`/`sdrEnum`, unlike every other thread pair in the shutdown sequence.

## Thread Lifecycle

### Startup Sequence

In `CubicSDR::OnInit()` (`src/CubicSDR.cpp`):

1. Queue plumbing — all global queues created and wired
2. `SpectrumVisualDataThread` started
3. `DemodVisualDataThread` started (if enabled)
4. `SDRPostThread` started
5. `SDREnumerator` object created (but thread not yet started)
6. `AppFrame` created (wxWidgets main window) — its constructor creates a `FFTVisualDataThread` for the waterfall display and starts it immediately
7. `SDREnumerator` thread started
8. Device selection triggers `SDRThread` start (in `CubicSDR::setDevice()`)

### Per-Demodulator Startup

When `DemodulatorInstance::run()` is called (protected by `m_thread_control_mutex`):

1. `AudioThread` started
2. `DemodulatorPreThread` started
3. `DemodulatorThread` started

An `AudioSinkFileThread` may also be started on-demand when recording is activated (`startRecording()`). It is not part of the standard startup — it is created, joined, and deleted entirely within the `startRecording()`/`stopRecording()` pair.

### Shutdown Sequence

In `CubicSDR::OnExit()`:

1. `stopRig()` — calls `RigThread::terminate()` (sets atomic flag) then `isTerminated(1000)` to join (if rig is active)
2. `SDRThread::terminate()` — stops producing IQ data (waited up to 3s)
3. `SDRPostThread::terminate()` — stops channelizing (waited up to 3s)
4. `DemodulatorMgr::terminateAll()` — terminates all demodulator instances (queues flushed inside each `DemodulatorInstance::terminate()`)
5. Visual processor threads terminated (spectrum and demod, waited up to 1s each)
6. All `std::thread` objects joined and deleted (`t_SDR`, `t_PostSDR`, `t_DemodVisual`, `t_SpectrumVisual`); corresponding thread objects deleted
7. `AudioThread::deviceCleanup()` — deletes controller AudioThreads for all devices

The waterfall `FFTVisualDataThread` (created inside `AppFrame`) is terminated and joined in `AppFrame::~AppFrame()`, which runs when wxWidgets destroys the frame after `OnExit()` returns.

If any termination step times out, the application calls `::exit()` with a step-specific error code rather than risk hanging indefinitely (11 = SDR thread, 12 = SDR post-thread, 13 = visual processor threads).

Note: `t_SDREnum` and `sdrEnum` are not joined or deleted in `OnExit()`. They rely on process exit for cleanup.

### Per-Demodulator Shutdown

`DemodulatorInstance::terminate()` (notably **not** protected by `m_thread_control_mutex`, unlike `run()` and `isTerminated()`):

1. `AudioThread::terminate()` — stops consuming audio
2. `DemodulatorThread::terminate()` — stops demodulating
3. `DemodulatorPreThread::terminate()` — stops resampling (also terminates worker thread)
4. If recording is active, `stopRecording()` — detaches the `AudioSinkFileThread` output queue, joins and deletes the sink thread
5. All queues flushed (`pipeIQInputData`, `pipeAudioData`, `pipeIQDemodData`) to unblock pending pushes

The actual thread join/cleanup happens in `isTerminated()`, which is called from the destructor with an infinite wait. `isTerminated()` acquires `m_thread_control_mutex` and holds it while iterating through all thread cleanup.

### Known Issues

**macOS audio thread join bug:** In `DemodulatorInstance::isTerminated()`, the macOS cleanup path for the audio thread (`DemodulatorInstance.cpp`) calls `pthread_join(t_PreDemod, NULL)` — a copy-paste error where `t_PreDemod` was used instead of `t_Audio`. At that point `t_PreDemod` has already been joined and set to `nullptr`, so this calls `pthread_join(NULL, ...)` which returns `ESRCH` (no thread found). The result is that the audio thread's `std::thread` object is leaked. On macOS, `t_PreDemod` and `t_Demod` are `pthread_t` (not pointers), while `t_Audio` is `std::thread*` on all platforms — so even with the correct variable name, `pthread_join` would be the wrong API. The non-macOS path (`t_Audio->join()` / `delete t_Audio`) is correct.

**SDREnumerator not cleaned up on exit:** `t_SDREnum` and `sdrEnum` are never joined or deleted in `OnExit()` or anywhere else in the codebase. They rely on process exit for cleanup.

**Waterfall thread not deleted in AppFrame destructor:** `AppFrame::~AppFrame()` calls `waterfallDataThread->terminate()` and `t_FFTData->join()`, but neither the `std::thread*` object nor the `FFTVisualDataThread*` are deleted — a memory leak on shutdown.

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

The UI thread is **primarily pull-based**:

1. `AppFrame::OnIdle()` is called continuously by the wx event loop and handles device params, modem properties, and UI state
2. Each canvas registers its own `EVT_IDLE` handler independently (`AppFrame::OnIdle`, `WaterfallCanvas::OnIdle`, `SpectrumCanvas::OnIdle`, `ScopeCanvas::OnIdle`, `TuningCanvas::OnIdle`, `ModeSelectorCanvas::OnIdle`, `MeterCanvas::OnIdle`, `GainCanvas::OnIdle`, `UITestCanvas::OnIdle`) — `WaterfallCanvas::OnIdle` calls `processInputQueue()` (which internally `try_pop()`s); other canvases typically just call `Refresh()` and defer queue pops to `OnPaint()`
3. Visual data flows are pull-based: worker threads push to queues, UI canvases pull via `try_pop()` in `OnIdle()` (WaterfallCanvas) or `OnPaint()` (SpectrumCanvas, ScopeCanvas)
4. Shared state uses `std::atomic` variables (frequency, signal levels, mute state)
5. UI-initiated changes go through atomic variables and flags, not wx events
6. **Exception:** `SDRThread` triggers `refreshGainUI()` from the worker thread via `notifyMainUIOfDeviceChange()`, which rebuilds `GainCanvas` panels and triggers `Refresh()` — a cross-thread UI mutation outside the pull-based pattern
