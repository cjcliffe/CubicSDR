# CubicSDR Architecture

This directory contains architectural documentation for CubicSDR. These documents are designed to be expanded as the codebase evolves.

## Documents

### Core Architecture

| Document | Description |
|----------|-------------|
| [Signal Flow](signal-flow.md) | Complete data path from SDR hardware to audio output, including queue topology and buffer management |
| [Threading Model](threading.md) | Thread inventory, lifecycle management, synchronization mechanisms, and producer-consumer patterns |
| [Modem System](modem-system.md) | Modem plugin architecture, factory registration, data processing pipeline, and available modem types |

### Subsystem Deep Dives

| Document | Description |
|----------|-------------|
| [Audio Subsystem](audio-subsystem.md) | Controller/bound mixing pattern, WAV recording pipeline, device management, and real-time audio callback |
| [Visual Architecture](visual-architecture.md) | Canvas hierarchy, GLPanel system, GLFont rendering, ColorTheme system, and visual data processing pipeline |
| [Configuration System](configuration-system.md) | AppConfig/DeviceConfig persistence, DataTree serialization, session management, and file locations |
| [Bookmark System](bookmark-system.md) | BookmarkMgr data model, groups/ranges/recents, XML persistence, and default frequency bands |
| [SDR Device Layer](sdr-device-layer.md) | SDREnumerator discovery, SDRDeviceInfo capabilities, manual devices, and SoapySDR module loading |

## Quick Reference

### Source Directory Layout

```
src/
  sdr/           SDR hardware interaction (SoapySDR wrapper)
  demod/         Demodulator lifecycle and processing
  audio/         Audio output (RtAudio) and recording
  visual/        OpenGL canvas rendering
  modules/modem/ Modem plugins (analog and digital)
  process/       FFT and visual data processing
  util/          Core utilities (queues, timers, data trees)
  rig/           Hamlib CAT control (optional)
  forms/         UI forms (device selection, bookmarks, dialogs)
  panel/         Display panels (scope, spectrum, waterfall, meter)
```

### Key Classes

| Class | File | Role |
|-------|------|------|
| `CubicSDR` | `src/CubicSDR.h` | Application singleton, owns all global threads |
| `AppFrame` | `src/AppFrame.h` | Main wxWidgets window, UI event loop |
| `AppConfig` | `src/AppConfig.h` | Application configuration persistence |
| `SDRThread` | `src/sdr/SoapySDRThread.h` | Reads IQ samples from hardware |
| `SDRPostThread` | `src/sdr/SDRPostThread.h` | Channelizes and distributes IQ data |
| `SDREnumerator` | `src/sdr/SDREnumerator.h` | Background device discovery |
| `DemodulatorInstance` | `src/demod/DemodulatorInstance.h` | Orchestrates one demodulator's thread chain |
| `DemodulatorMgr` | `src/demod/DemodulatorMgr.h` | Manages all demodulator instances |
| `DemodulatorWorkerThread` | `src/demod/DemodulatorWorkerThread.h` | Offloaded modem/filter creation |
| `Modem` | `src/modules/modem/Modem.h` | Abstract modem base class with factory pattern |
| `AudioThread` | `src/audio/AudioThread.h` | Manages RtAudio output device |
| `AudioSinkThread` | `src/audio/AudioSinkThread.h` | Per-demod audio sink (base class) |
| `AudioSinkFileThread` | `src/audio/AudioSinkFileThread.h` | WAV file recording |
| `IOThread` | `src/IOThread.h` | Base class for all worker threads |
| `ThreadBlockingQueue` | `src/util/ThreadBlockingQueue.h` | Primary inter-thread communication mechanism |
