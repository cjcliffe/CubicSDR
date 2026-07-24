# Modem System

This document describes CubicSDR's modem plugin architecture, registration mechanism, and available modem types.

**Last Updated:** 2026-07-23

## Architecture Overview

CubicSDR uses a **static factory registration** pattern for modems. All modems are compiled in and registered at startup — there is no dynamic plugin discovery.

```
ModemBase (empty base)
  |
  +-- Modem (abstract, factory methods + pure virtual interface)
        |
        +-- ModemAnalog (analog base, float audio output)
        |     +-- ModemFM, ModemNBFM, ModemFMStereo, ModemAM, ModemCW
        |     +-- ModemLSB, ModemUSB, ModemDSB, ModemIQ
        |
        +-- ModemDigital (digital base, symbol output)
              +-- ModemAPSK, ModemASK, ModemBPSK, ModemDPSK
              +-- ModemFSK, ModemGMSK, ModemOOK, ModemPSK
              +-- ModemQAM, ModemQPSK, ModemSQAM, ModemST
```

## Registration

### Factory Pattern

**File:** `src/modules/modem/Modem.h`

```cpp
typedef ModemBase *(*ModemFactoryFn)();

class Modem {
    static std::map<std::string, ModemFactoryFn> modemFactories;
    static std::map<std::string, int> modemDefaultRates;
    
    static void addModemFactory(ModemFactoryFn fn, std::string name, int defaultRate);
    static Modem *makeModem(std::string name);
    static std::vector<std::string> getFactories();
};
```

Each concrete modem provides a static factory method:
```cpp
static ModemBase *factory() { return new ModemFM; }
```

### Registration Site

**File:** `src/CubicSDR.cpp` (in `CubicSDR::OnInit()`)

All modems are registered at startup:
```cpp
Modem::addModemFactory(ModemFM::factory, "FM", 200000);
Modem::addModemFactory(ModemNBFM::factory, "NBFM", 12500);
Modem::addModemFactory(ModemFMStereo::factory, "FMS", 200000);
// ... etc
```

Digital modems are conditionally compiled with `ENABLE_DIGITAL_LAB`.

## Modem Interface

### Core Virtual Methods

**File:** `src/modules/modem/Modem.h`

| Method | Purpose |
|--------|---------|
| `getType()` | Returns `"analog"` or `"digital"` |
| `getName()` | Returns modem name (e.g., `"FM"`, `"PSK"`) |
| `checkSampleRate(long long, int)` | Validates/adjusts a given sample rate |
| `buildKit(long long sampleRate, int audioSampleRate)` | Creates per-session state (ModemKit) |
| `disposeKit(ModemKit *kit)` | Destroys a kit |
| `demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut)` | Core DSP method |
| `getSettings()` | Returns configurable parameters |
| `writeSetting(string, string)` / `readSetting(string)` | Get/set parameters |

### ModemKit Hierarchy

| Class | File | Contents |
|-------|------|----------|
| `ModemKit` | `Modem.h` | Base: `sampleRate`, `audioSampleRate` |
| `ModemKitAnalog` | `ModemAnalog.h` | `msresamp_rrrf audioResampler`, `audioResampleRatio` |
| `ModemKitDigital` | `ModemDigital.h` | Empty base; subclasses add liquid-dsp objects |

## Data Processing

### Demodulation Pipeline

```
DemodulatorPreThread
    | (attaches Modem* and ModemKit* to each IQ packet)
    v
DemodulatorThread
    | calls: cModem->demodulate(cModemKit, &modemData, ati.get())
    v
AudioThreadInput (analog) or DemodulatorThreadOutput (digital)
```

### Analog Modems

- Fill `audioOut->data` with demodulated float audio (mono or stereo)
- `ModemAnalog` base provides `initOutputBuffers()` and `buildAudioOutput()`
- Audio resampling from demod rate to audio rate via `msresamp_rrrf`

### Digital Modems

- Fill `demodOutputDataDigital` with symbol indices
- Produce no audio directly
- `DemodulatorThread` forwards IQ data to visualization instead

## Modem Selection

1. User selects demod type via UI
2. `DemodulatorInstance::setDemodulatorType(string)` called
3. `DemodulatorPreThread` detects `demodTypeChanged`, sends `DEMOD_WORKER_THREAD_CMD_MAKE_DEMOD` to worker
4. `DemodulatorWorkerThread` calls `Modem::makeModem(type)` and `modem->buildKit()`
5. New Modem* and ModemKit* are bundled with IQ packets for DemodulatorThread

Runtime modem switching: PreThread nulls out modem/kit immediately (packets dropped until worker finishes), then adopts new modem/kit when worker responds.

## Available Modems

### Analog (9)

| Name | Class | File | Default Rate |
|------|-------|------|-------------|
| FM | `ModemFM` | `src/modules/modem/analog/ModemFM.cpp` | 200000 |
| NBFM | `ModemNBFM` | `src/modules/modem/analog/ModemNBFM.cpp` | 12500 |
| FM Stereo | `ModemFMStereo` | `src/modules/modem/analog/ModemFMStereo.cpp` | 200000 |
| AM | `ModemAM` | `src/modules/modem/analog/ModemAM.cpp` | 6000 |
| CW | `ModemCW` | `src/modules/modem/analog/ModemCW.cpp` | 500 |
| LSB | `ModemLSB` | `src/modules/modem/analog/ModemLSB.cpp` | 5400 |
| USB | `ModemUSB` | `src/modules/modem/analog/ModemUSB.cpp` | 5400 |
| DSB | `ModemDSB` | `src/modules/modem/analog/ModemDSB.cpp` | 5400 |
| I/Q | `ModemIQ` | `src/modules/modem/analog/ModemIQ.cpp` | 48000 |

### Digital (12, conditional on `ENABLE_DIGITAL_LAB`)

| Name | Class | File | Default Rate |
|------|-------|------|-------------|
| APSK | `ModemAPSK` | `src/modules/modem/digital/ModemAPSK.cpp` | 200000 |
| ASK | `ModemASK` | `src/modules/modem/digital/ModemASK.cpp` | 200000 |
| BPSK | `ModemBPSK` | `src/modules/modem/digital/ModemBPSK.cpp` | 200000 |
| DPSK | `ModemDPSK` | `src/modules/modem/digital/ModemDPSK.cpp` | 200000 |
| FSK | `ModemFSK` | `src/modules/modem/digital/ModemFSK.cpp` | 19200 |
| GMSK | `ModemGMSK` | `src/modules/modem/digital/ModemGMSK.cpp` | 19200 |
| OOK | `ModemOOK` | `src/modules/modem/digital/ModemOOK.cpp` | 200000 |
| PSK | `ModemPSK` | `src/modules/modem/digital/ModemPSK.cpp` | 200000 |
| QAM | `ModemQAM` | `src/modules/modem/digital/ModemQAM.cpp` | 200000 |
| QPSK | `ModemQPSK` | `src/modules/modem/digital/ModemQPSK.cpp` | 200000 |
| SQAM | `ModemSQAM` | `src/modules/modem/digital/ModemSQAM.cpp` | 200000 |
| ST | `ModemST` | `src/modules/modem/digital/ModemST.cpp` | 200000 |

## Adding a New Modem

To add a new modem type:

1. Create `src/modules/modem/analog/ModemXXX.h` and `.cpp` (or `digital/`)
2. Inherit from `ModemAnalog` or `ModemDigital`
3. Implement the pure virtual methods: `getType()`, `getName()`, `checkSampleRate()`, `buildKit()`, `disposeKit()`, `demodulate()`
4. Add a static `factory()` method
5. Register in `CubicSDR::OnInit()` in `src/CubicSDR.cpp`
6. Add to `CMakeLists.txt` source list
