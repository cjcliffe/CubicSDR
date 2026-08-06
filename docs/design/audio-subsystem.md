# Audio Subsystem

This document describes CubicSDR's audio output architecture, the controller/bound mixing pattern, recording pipeline, and device management.

## Overview

The audio subsystem handles real-time audio output from demodulators to hardware devices. It uses a **controller/bound** pattern where one `AudioThread` per physical output device owns the RtAudio stream, and other `AudioThread` instances (one per demodulator) bind to it for mixing.

```
DemodulatorThread (N)
    | pushes to "AudioDataOutput" queue
    v
AudioThreadInputQueue (per-demod, max 100 items)
    | retrieved as "AudioDataInput" on AudioThread
    v
AudioThread (per-demod, "bound") ---bindThread()---> AudioThread (controller)
                                                             |
                                                     audioCallback (real-time)
                                                             |
                                                     RtAudio hardware output
```

## Class Hierarchy

| Class | File | Role |
|-------|------|------|
| `AudioThread` | `src/audio/AudioThread.h` | Per-device audio output; serves as both controller and bound thread |
| `AudioSinkThread` | `src/audio/AudioSinkThread.h` | Abstract base for audio consumers (recording) |
| `AudioSinkFileThread` | `src/audio/AudioSinkFileThread.h` | WAV file recording sink |
| `AudioFile` | `src/audio/AudioFile.h` | Abstract file output handler |
| `AudioFileWAV` | `src/audio/AudioFileWAV.h` | WAV file writer with multi-part support |
| `AudioThreadInput` | `src/audio/AudioThread.h` | Audio data packet (float samples, metadata) |
| `AudioThreadCommand` | `src/audio/AudioThread.h` | Control commands (set device, set sample rate) |

## Controller/Bound Pattern

### Static State

`AudioThread` maintains two static maps protected by `m_device_mutex`:

| Map | Type | Purpose |
|-----|------|---------|
| `deviceController` | `map<int, AudioThread*>` | Maps output device ID to its controller thread |
| `deviceSampleRate` | `map<int, int>` | Maps output device ID to configured sample rate |

### Thread Roles

**Controller thread:**
- Created on first `setupDevice()` call for a given device ID
- Owns the RtAudio stream and `audioCallback`
- Manages the `boundThreads` list
- Runs an infinite loop processing `AudioThreadCommand` messages via a timed pop (`HEARTBEAT_CHECK_PERIOD_MICROS` = 50 ms), which also serves as the termination check interval

**Bound thread:**
- Object allocated in `DemodulatorInstance` constructor; thread started in `DemodulatorInstance::run()`
- Pushes `AudioThreadInput` packets to its `inputQueue`
- Its `inputQueue` is consumed by the controller's `audioCallback`, which mixes data from all bound threads

### Device Setup Flow

1. `AudioThread::run()` calls `setupDevice(deviceId)` (under `m_device_mutex`)
2. If a controller already existed for the previous device, `this` removes itself from that controller's `boundThreads` (under the old controller's mutex)
3. If no controller exists for `deviceId`:
   - A new `AudioThread` is created as controller
   - The controller is registered in `deviceController[deviceId]`
   - The controller binds the current (calling) thread to its `boundThreads`
   - `attachControllerThread()` stores the controller's `std::thread*` for lifecycle management
4. If a controller already exists and `this` is the controller:
   - The RtAudio stream is opened directly with `audioCallback`
5. If a controller already exists and `this` is a bound thread:
   - The current thread binds itself to the existing controller (under the controller's mutex)
6. The `active` flag is set to `true`

### Audio Mixing (Real-Time)

The `audioCallback` function runs in the RtAudio real-time thread:

1. Zero the output buffer
2. Lock the controller mutex
3. For each bound thread:
   - Lock the bound thread's mutex
   - Skip if terminated, inactive, queue missing, or queue empty
   - If `currentInput` is null, pop a packet from the queue; on success, skip to the next thread (the newly popped packet is not mixed until the next callback invocation)
   - If `currentInput` sample rate doesn't match the controller's, pop and discard packets until a match is found or the queue is exhausted; skip the thread if no match
   - If `currentInput` has zero channels or empty data and the queue has more items, pop the next packet; otherwise skip the thread
   - Mix samples from `currentInput` into the output buffer (mono: duplicate to L+R; stereo: direct mix), advancing `audioQueuePtr` and popping the next packet from the queue as needed when the current packet is exhausted mid-mixing
   - Apply per-thread gain
4. If total peak > 1.0, normalize the output buffer
5. Return 0 on success; return 1 if the controller is terminated, which instructs RtAudio to stop the stream

Key properties:
- **Buffer size:** The RtAudio buffer is 1024 frames by default (`nBufferFrames`), which at 48 kHz yields ~21 ms latency per buffer
- **Sample rate matching:** Whenever a new `currentInput` is popped (either on first access or when the current packet is exhausted mid-mixing), the callback checks if its sample rate matches the controller's. If not, it pops and discards packets until it finds a matching one or the queue is exhausted. If no matching packet is found, `currentInput` is left as `nullptr` and the thread is skipped
- **First-packet latency:** When a new packet is popped from a queue, the callback immediately continues to the next thread without mixing it. This introduces a one-callback-cycle delay before a newly queued packet produces output, avoiding partial consumption of a fresh packet
- **Underflow handling:** If a bound thread runs out of data, the callback continues with the next thread. RtAudio buffer underflows (reported via `status` flag) are counted in the controller's `underflowCount` field
- **Gain staging:** Per-thread `gain` (0.0–2.0, default 1.0) is applied before mixing; global normalization prevents clipping

### Real-Time Design Constraints

The `audioCallback` runs in a RtAudio real-time thread (potentially with `SCHED_FIFO` priority). To meet real-time guarantees:

- **Non-blocking queue access:** All queue operations use `try_pop()` and `try_push()`
- **Two-stage packet consumption:** The callback maintains `currentInput` and `audioQueuePtr` per bound thread across invocations. A packet is only popped from the queue when `currentInput` is null. Samples are consumed from the current packet across multiple callback invocations until exhausted, then the next packet is popped
- **Pre-zeroed output buffer:** The output buffer is zeroed at the start of every callback
- **Lock ordering:** The callback acquires locks in a strict order: controller mutex first, then each bound thread's mutex in sequence
- **No allocation:** The callback never allocates or deallocates memory. All buffers are pre-allocated; `std::vector` operations are confined to the producer side (DemodulatorThread)

### Thread Lifecycle

**Startup** (across `DemodulatorInstance` constructor and `run()`):
1. `AudioThread` created in the `DemodulatorInstance` constructor
2. The audio pipe queue is registered on the `AudioThread` as `"AudioDataInput"` and on the `DemodulatorThread` as `"AudioDataOutput"`
3. `setInitOutputDevice()` called in `DemodulatorInstance::setOutputDevice()` (when the demodulator is not yet active) to store the device ID and sample rate
4. `AudioThread::run()` starts in a new thread (via `IOThread::threadMain`) when `DemodulatorInstance::run()` is called
5. `setupDevice()` called inside `AudioThread::run()` — either creates a new controller thread (with its own `std::thread` via `attachControllerThread()`) or binds to an existing controller

**Shutdown** (in `DemodulatorInstance::terminate()`):
1. `AudioThread::terminate()` sets `stopping = true`
2. The `run()` loop exits (after the next `HEARTBEAT_CHECK_PERIOD_MICROS` timeout), flushes the input queue, and nullifies `currentInput`
3. Cleanup in `run()` after the loop:
   - For bound threads: removes itself from the controller's `boundThreads` (under the controller's mutex)
   - For controller threads: stops and closes the RtAudio stream
4. The `AudioThread` destructor handles the controller's `std::thread` lifecycle: terminates, joins, and deletes it

**Device cleanup** (`AudioThread::deviceCleanup()`):
- Called during application shutdown
- Deletes all controller threads from `deviceController`

**Active state management** (`AudioThread::setActive()` / `isActive()`):
- Allows dynamically enabling or disabling audio output without destroying the AudioThread
- Transitioning inactive → active: binds the thread to the controller's `boundThreads`
- Transitioning active → inactive: removes the thread from the controller's `boundThreads`
- Flushes the input queue (when non-null) to discard stale data
- The `active` flag is also checked by the `audioCallback` — inactive threads are skipped during mixing

## AudioThreadInput

Data packet passed from demodulator to audio output:

| Field | Type | Description |
|-------|------|-------------|
| `frequency` | `long long` | Demodulated signal frequency |
| `inputRate` | `int` | Input sample rate before demodulation |
| `sampleRate` | `int` | Audio output sample rate |
| `channels` | `int` | 1 (mono) or 2 (stereo) |
| `peak` | `float` | Peak signal level (for normalization) |
| `type` | `int` | Audio type: 0 = mono waveform, 1 = stereo, 2 = IQ/XY (visualization only) |
| `is_squelch_active` | `bool` | Whether squelch is currently active |
| `data` | `vector<float>` | Interleaved float samples |

## Audio Commands

`AudioThreadCommand` is sent via the command queue:

| Command | Effect |
|---------|--------|
| `AUDIO_THREAD_CMD_SET_DEVICE` | Calls `setupDevice()` to switch output device |
| `AUDIO_THREAD_CMD_SET_SAMPLE_RATE` | Calls `setSampleRate()` — stops/restarts RtAudio stream, updates all bound threads and active demodulators |

## Recording Pipeline

### AudioSinkThread

Abstract base class for audio consumers that run in their own thread:

- Owns an `AudioThreadInputQueue` with max 1000 items, registered as `"input"` in the IOThread queue map
- Pops input packets in a loop, calling `sink()` for each
- Detects input property changes (channels, frequency, inputRate, sample rate; notably not `peak` or `type`) and calls `inputChanged()`
- On termination, flushes the input queue to discard in-flight data
- Subclasses implement `sink()` and `inputChanged()`

### AudioSinkFileThread

Concrete recording implementation:

- `inputChanged()` closes the current WAV file and resets the duration timer, causing a new file to be started on the next write

**Squelch options:**
| Option | Behavior |
|--------|----------|
| `SQUELCH_RECORD_SILENCE` | Record silence (zeros) when squelch is active |
| `SQUELCH_SKIP_SILENCE` | Skip recording entirely when squelch is active |
| `SQUELCH_RECORD_ALWAYS` | Record audio regardless of squelch state |

**File time limiting:**
- When `fileTimeLimit > 0`, the sink tracks recording duration via a `Timer`
- When duration exceeds the limit, the current file is closed and a new one is opened
- New files are named with a timestamp suffix: `{baseName}_{YYYY-MM-DD_HH-MM-SS}.wav`

**File naming:**
- Base name is set by the user (demodulator label or default)
- Invalid filename characters (`<>:"/\|?*`) are replaced with `_`
- Sequence numbers (`_NNN`, zero-padded to 3 digits) are appended to the base name when a file exceeds the 2GB limit and a new part is started. The first part has no sequence suffix
- Time-limited recording changes the base name to include a timestamp (`{baseName}_{YYYY-MM-DD_HH-MM-SS}`), resetting the sequence number to 0
- If the final filename already exists, a `-N` suffix is added to avoid overwrites (e.g., `base_001-1.wav`)
- Files are placed in the configured recording path

### AudioFile / AudioFileWAV

`AudioFile` is the abstract file handler; `AudioFileWAV` is the concrete implementation.

**WAV file writing:**
- Standard PCM format: 16-bit signed integer samples
- Float samples are scaled to int16 range: when peak >= 1.0, samples are divided by peak (normalizing down); when peak < 1.0, samples are multiplied by 32767 without amplification, preserving headroom
- File size is limited to ~2GB (`MAX_WAV_FILE_SIZE = 0x7FFFFFFF - 1024`) for compatibility
- When the limit is reached, the file is closed and a new part is opened with an incremented sequence number

**Multi-part WAV handling:**
- When a file exceeds the 2GB limit, it is closed and a new file is opened with `currentSequenceNumber` incremented. The sequence number `_NNN` (zero-padded to 3 digits) is inserted into the base name before the extension, but only when > 0 (so the first file has no sequence suffix)
- If the resulting filename already exists on disk (e.g., from a previous session), a `-N` suffix is appended to the candidate name until a unique name is found. This collision check runs after the sequence number is applied
- Header is written on file open; data chunk size is patched on close

**File path resolution:**
- Recording path comes from `AppConfig::getRecordingPath()`
- Filename is: `{recordingPath}/{baseName}.{ext}`
- File is opened in binary mode

## Device Enumeration

`AudioThread::enumerateDevices()` queries RtAudio for all available output devices:

- Creates a temporary `RtAudio` instance
- Iterates all devices, printing: name, default status, channel counts, native formats, supported sample rates
- Results are stored in the provided `vector<RtAudio::DeviceInfo>`

## Thread Safety

| Mechanism | Scope | Purpose |
|-----------|-------|---------|
| `m_device_mutex` (static) | Global | Protects `deviceController`, `deviceSampleRate` |
| `m_mutex` (per-thread) | Per AudioThread | Protects `boundThreads`, `sampleRate` |
| `audioCallback` locking | Real-time | Locks controller mutex, then each bound thread mutex in sequence |

Design constraints:
- `audioCallback` must not allocate memory
- `audioCallback` uses `try_pop()` (non-blocking) for all queue access
- Mutex lock order: controller → bound thread (never reversed)
- Queue overflow: `DemodulatorThread` uses `try_push()` to push audio data. If the playback queue (max 100 items) is full, the frame is dropped with a warning message
- `deviceCleanup()` skips `m_device_mutex` locking and is only called during application shutdown

## Platform-Specific Notes

**macOS:**
- Audio thread priority set to `sched_get_priority_max(SCHED_RR) - 1` via `pthread_setschedparam`
- `AudioSinkThread` (recording) uses the same `SCHED_RR` priority on macOS

**Linux:**
- Default thread priorities used

**Non-Windows (macOS + Linux):**
- RtAudio stream options include `SCHED_FIFO` priority (guarded by `#ifndef _MSC_VER`)

**Windows:**
- Default thread priorities used
- RtAudio configured with `RTAUDIO_SCHEDULE_REALTIME`

## Buffer Management

`DemodulatorThread` uses `ReBuffer<AudioThreadInput>` (defined in `IOThread.h`) to pool audio output buffers. For the full pool mechanics (reuse logic, GC thresholds, warning thresholds), see [signal-flow.md](signal-flow.md) "Buffer Management".

## Muting

`DemodulatorThread` checks the `muted` flag, solo mode, and squelch state before pushing `AudioThreadInput` to the playback queue (`audioOutputQueue`). A demodulator only pushes to playback when it is not muted, not squelched, and either solo mode is off or this demodulator is the current modem. Demodulators excluded by any of these conditions do not push data, so their bound thread's queue remains empty.

The recording sink queue (`audioSinkOutputQueue`) is pushed independently: it receives audio whenever `ati` is non-null, regardless of squelch, mute, or solo state. The squelch flag is attached to `ati` before the push, and the recording sink handles squelch through the `is_squelch_active` field. This ensures recording captures the raw demodulated signal even when playback is silenced by mute or solo mode.

## Digital Modem Audio

`ModemDigital` subclasses produce two separate `AudioThreadInput` objects. The playback buffer `ati` is allocated with an empty `data` vector (populated by `demodulate()` for analog modems but left empty for digital). When the visualization block runs, `ati` is set to `nullptr` for digital modems (with a TODO comment about future audio output support), so it is never pushed to either the playback or recording queues. A separate `ati_vis` is populated with interleaved I/Q sample data (`channels=2`, `type=2`) and pushed to the visualization queue `audioVisOutputQueue`. The visualization path (ScopeVisualProcessor) consumes `ati_vis` for constellation/scope display. No audio data reaches the mixing or recording paths for digital modems.

## Audio Data Flow Summary

```
DemodulatorThread
    | calls Modem::demodulate() -> fills AudioThreadInput
    | pushes to "AudioDataOutput" queue via try_push()
    v
AudioThreadInputQueue (per-demod, max 100 items)
    | retrieved as "AudioDataInput" on AudioThread
    v
AudioThread (bound)
    | holds: inputQueue, currentInput, audioQueuePtr
    | (consumed by controller's audioCallback)
    v
audioCallback (controller, real-time)
    | for each bound thread:
    |   pops from bound.inputQueue via try_pop()
    |   maintains currentInput across invocations
    |   mixes with per-thread gain
    v
Normalization (if peak > 1.0)
    |
    v
RtAudio output buffer -> speakers/headphones

DemodulatorThread
    | also pushes to AudioSinkFileThread input queue
    v
AudioSinkFileThread
    | calls AudioFileWAV::writeToFile()
    v
WAV file on disk
```

Note: The recording pipeline's `AudioSinkThread` input queue has a capacity of 1000 items, 10x larger than the audio playback path's 100-item queue, giving the recording path more headroom to absorb scheduling jitter.
