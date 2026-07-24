# CubicSDR Architecture

This directory contains architectural documentation for CubicSDR. These documents are designed to be expanded as the codebase evolves.

**Last Updated:** 2026-07-23

## Documents

| Document | Description |
|----------|-------------|
| [Signal Flow](signal-flow.md) | Complete data path from SDR hardware to audio output, including queue topology and buffer management |
| [Threading Model](threading.md) | Thread inventory, lifecycle management, synchronization mechanisms, and producer-consumer patterns |
| [Modem System](modem-system.md) | Modem plugin architecture, factory registration, data processing pipeline, and available modem types |

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
```

### Key Classes

| Class | File | Role |
|-------|------|------|
| `CubicSDR` | `src/CubicSDR.h` | Application singleton, owns all global threads |
| `AppFrame` | `src/AppFrame.h` | Main wxWidgets window, UI event loop |
| `SDRThread` | `src/sdr/SoapySDRThread.h` | Reads IQ samples from hardware |
| `SDRPostThread` | `src/sdr/SDRPostThread.h` | Channelizes and distributes IQ data |
| `DemodulatorInstance` | `src/demod/DemodulatorInstance.h` | Orchestrates one demodulator's thread chain |
| `DemodulatorMgr` | `src/demod/DemodulatorMgr.h` | Manages all demodulator instances |
| `Modem` | `src/modules/modem/Modem.h` | Abstract modem base class with factory pattern |
| `AudioThread` | `src/audio/AudioThread.h` | Manages RtAudio output device |
| `IOThread` | `src/IOThread.h` | Base class for all worker threads |
| `ThreadBlockingQueue` | `src/util/ThreadBlockingQueue.h` | Primary inter-thread communication mechanism |
