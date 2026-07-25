# Audio Subsystem

This document describes CubicSDR's audio output architecture, the controller/bound mixing pattern, recording pipeline, and device management.

## Overview

The audio subsystem handles real-time audio output from demodulators to hardware devices. It uses a **controller/bound** pattern where one `AudioThread` per physical output device owns the RtAudio stream, and other `AudioThread` instances (one per demodulator) bind to it for mixing.

```
DemodulatorThread (N)
    |
    +-- AudioThreadInputQueue --> AudioThread (per-demod, "bound")
    |                                 |
    |                          bindThread()
    |                                 |
    +-- AudioThread (controller) <----+
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

`AudioThread` maintains three static maps protected by `m_device_mutex`:

| Map | Type | Purpose |
|-----|------|---------|
| `deviceController` | `map<int, AudioThread*>` | Maps output device ID to its controller thread |
| `deviceSampleRate` | `map<int, int>` | Maps output device ID to configured sample rate |

### Thread Roles

**Controller thread:**
- Created on first `setupDevice()` call for a given device ID
- Owns the RtAudio stream and `audioCallback`
- Manages the `boundThreads` list
- Runs an infinite loop processing `AudioThreadCommand` messages

**Bound thread:**
- Created per-demodulator via `DemodulatorInstance::run()`
- Pushes `AudioThreadInput` packets to its `inputQueue`
- The controller's `audioCallback` pops from all bound threads and mixes

### Device Setup Flow

1. `AudioThread::run()` calls `setupDevice(deviceId)`
2. If no controller exists for `deviceId`:
   - A new `AudioThread` is created as controller
   - Its own `run()` is started in a new `std::thread`
   - The current thread binds itself to the controller
3. If a controller already exists:
   - The current thread binds itself to the existing controller
4. The controller opens the RtAudio stream with `audioCallback`

### Audio Mixing (Real-Time)

The `audioCallback` function runs in the RtAudio real-time thread:

1. Zero the output buffer
2. Lock the controller mutex
3. For each bound thread:
   - Lock the bound thread's mutex
   - Skip if terminated, inactive, or queue empty
   - Pop `AudioThreadInput` packets from the bound thread's queue
   - Mix samples into the output buffer (mono: duplicate to L+R; stereo: direct mix)
   - Apply per-thread gain
4. If total peak > 1.0, normalize the output buffer
5. Return

Key properties:
- **Sample rate matching:** If a bound thread's current input has a different sample rate than the controller, the callback drains old packets until it finds a matching one
- **Underflow handling:** If a bound thread runs out of data, the callback continues with the next thread (no silence insertion — the output buffer was pre-zeroed)
- **Gain staging:** Per-thread `gain` (0.0–2.0) is applied before mixing; global normalization prevents clipping

### Thread Lifecycle

**Startup** (in `DemodulatorInstance::run()`):
1. `AudioThread` created and started
2. `setupDevice()` called → binds to controller or creates one

**Shutdown** (in `DemodulatorInstance::terminate()`):
1. `AudioThread::terminate()` sets `stopping = true`
2. The `run()` loop exits, drains the input queue
3. Bound thread removes itself from controller's `boundThreads`
4. For controller threads: RtAudio stream is stopped and closed
5. Controller thread joins its `std::thread` and deletes it

**Device cleanup** (`AudioThread::deviceCleanup()`):
- Called during application shutdown
- Deletes all controller threads from `deviceController`

## AudioThreadInput

Data packet passed from demodulator to audio output:

| Field | Type | Description |
|-------|------|-------------|
| `frequency` | `long long` | Demodulated signal frequency |
| `inputRate` | `int` | Input sample rate before demodulation |
| `sampleRate` | `int` | Audio output sample rate |
| `channels` | `int` | 1 (mono) or 2 (stereo) |
| `peak` | `float` | Peak signal level (for normalization) |
| `type` | `int` | Audio type identifier |
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

- Owns an `AudioThreadInputQueue` with max 1000 items
- Pops input packets in a loop, calling `sink()` for each
- Detects input property changes (channels, frequency, sample rate) and calls `inputChanged()`
- Subclasses implement `sink()` and `inputChanged()`

### AudioSinkFileThread

Concrete recording implementation:

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
- Sequence numbers are appended for multi-part WAV files
- Files are placed in the configured recording path

### AudioFile / AudioFileWAV

`AudioFile` is the abstract file handler; `AudioFileWAV` is the concrete implementation.

**WAV file writing:**
- Standard PCM format: 16-bit signed integer samples
- Float samples are scaled to int16 range using peak-based normalization
- File size is limited to ~2GB (`MAX_WAV_FILE_SIZE = 0x7FFFFFFF - 1024`) for compatibility
- When the limit is reached, the file is closed and a new part is opened with an incremented sequence number

**Multi-part WAV handling:**
- `getOutputFileName()` appends `_NNN` for sequence numbers > 0
- If the filename already exists, a `-N` suffix is added to avoid overwrites
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
| `m_mutex` (per-thread) | Per AudioThread | Protects `boundThreads`, `sampleRate`, `active` |
| `audioCallback` locking | Real-time | Locks controller mutex, then each bound thread mutex in sequence |

Design constraints:
- `audioCallback` must not allocate memory
- `audioCallback` uses `try_pop()` (non-blocking) for all queue access
- Mutex lock order: controller → bound thread (never reversed)

## Platform-Specific Notes

**macOS:**
- Audio thread priority set to `sched_get_priority_max(SCHED_RR) - 1` via `pthread_setschedparam`
- RtAudio stream options include `SCHED_FIFO` priority

**Windows/Linux:**
- Default thread priorities used
- RtAudio configured with `RTAUDIO_SCHEDULE_REALTIME`

## Audio Data Flow Summary

```
DemodulatorThread
    | calls Modem::demodulate() -> fills AudioThreadInput
    v
AudioThreadInputQueue (per-demod, max 100 items)
    |
    v
AudioThread (bound) --populates--> currentInput
    |
    v
audioCallback (controller, real-time)
    | pops from all boundThreads
    | mixes with gain + normalization
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
