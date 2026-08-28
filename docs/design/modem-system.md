# Modem System

This document describes CubicSDR's modem plugin architecture, registration mechanism, and available modem types.

## Architecture Overview

CubicSDR uses a **static factory registration** pattern for modems. All modems are compiled in and registered at startup — there is no dynamic plugin discovery.

```
ModemBase (empty base)
  |
  +-- Modem (abstract, factory methods + pure virtual interface)
        |
        +-- ModemAnalog (analog base, float audio output)
        |     +-- ModemFM, ModemNBFM, ModemAM, ModemCW
        |     +-- ModemLSB, ModemUSB, ModemDSB
        |
        +-- ModemDigital (digital base, symbol output)
        |     +-- ModemAPSK, ModemASK, ModemBPSK, ModemDPSK
        |     +-- ModemFSK, ModemGMSK, ModemOOK, ModemPSK
        |     +-- ModemQAM, ModemQPSK, ModemSQAM, ModemST
        |
        +-- ModemFMStereo (direct Modem subclass, analog but not ModemAnalog)
        +-- ModemIQ (direct Modem subclass, analog but not ModemAnalog)
```

## Registration

### Factory Pattern

**File:** `src/modules/modem/Modem.h`

```cpp
typedef ModemBase *(*ModemFactoryFn)();
typedef std::map<std::string, ModemFactoryFn> ModemFactoryList;
typedef std::map<std::string, int> DefaultRatesList;

class Modem {
    static ModemFactoryList modemFactories;
    static DefaultRatesList modemDefaultRates;
    
    static void addModemFactory(ModemFactoryFn fn, std::string name, int defaultRate);
    static Modem *makeModem(std::string name);
    static ModemFactoryList getFactories();
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

Digital modems are conditionally compiled with `ENABLE_DIGITAL_LAB` (the CMake option, defaulting to OFF). Factory registration uses `#ifdef ENABLE_DIGITAL_LAB` (`CubicSDR.cpp`), while `ModemDigital.h` and `ModemDigital.cpp` use `#if ENABLE_DIGITAL_LAB` for member and method guards.

## Modem Interface

### Core Virtual Methods

**File:** `src/modules/modem/Modem.h`

| Method | Purpose |
|--------|---------|
| `getType()` | Returns `"analog"` or `"digital"` |
| `getName()` | Returns modem name (e.g., `"FM"`, `"PSK"`) |
| `checkSampleRate(long long, int)` | Returns the modem's desired bandwidth; `DemodulatorWorkerThread` uses this to compute the IQ resample ratio so the modem receives pre-resampled IQ at its requested rate |
| `getDefaultSampleRate()` | Returns modem-specific default rate (base returns 200000) |
| `buildKit(long long sampleRate, int audioSampleRate)` | Creates per-session state (ModemKit) |
| `disposeKit(ModemKit *kit)` | Destroys a kit |
| `demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut)` | Core DSP method |
| `getSettings()` | Returns configurable parameters as `ModemArgInfoList` |
| `writeSetting(string, string)` / `readSetting(string)` | Get/set individual parameters |
| `writeSettings(ModemSettings)` / `readSettings()` | Batch get/set parameters |

### Non-Virtual Public Methods

| Method | Purpose |
|--------|---------|
| `shouldRebuildKit()` / `rebuildKit()` / `clearRebuildKit()` | Kit refresh lifecycle; `writeSetting()` calls `rebuildKit()` when a changed parameter requires recreating the kit (used by ModemCW, ModemFMStereo, ModemFSK, ModemGMSK). `DemodulatorPreThread` detects the flag and sends `DEMOD_WORKER_THREAD_CMD_BUILD_FILTERS` to the worker thread |
| `useSignalOutput()` / `useSignalOutput(bool)` | When true, `DemodulatorThread` computes signal level from demodulated audio output (for envelope-based modems). When false, computes from raw IQ input. Enabled by: AM, CW, DSB, LSB, USB |

### Static Methods

| Method | Purpose |
|--------|---------|
| `addModemFactory(ModemFactoryFn, string, int)` | Registers a modem factory with name and default rate |
| `makeModem(string)` | Creates a modem by registered name |
| `getFactories()` | Returns the factory map |
| `getModemDefaultSampleRate(string)` | Looks up default rate by registered name |

### ModemKit Hierarchy

| Class | File | Contents |
|-------|------|----------|
| `ModemKit` | `Modem.h` | Base: `sampleRate`, `audioSampleRate` |
| `ModemKitAnalog` | `ModemAnalog.h` | `msresamp_rrrf audioResampler`, `audioResampleRatio` |
| `ModemKitCW` | `ModemCW.h` | Extends `ModemKitAnalog` with `msresamp_cccf mInputResampler` |
| `ModemKitFMStereo` | `ModemFMStereo.h` | Extends `ModemKit` directly; `audioResampler`, `stereoResampler`, `firStereoLeft/Right`, `iirStereoPilot`, `firStereoR2C`/`firStereoC2R` (hilbert transforms), `iirDemphL`/`iirDemphR` (de-emphasis), `demph` mode, `nco_crcf stereoPilot` |
| `ModemKitDigital` | `ModemDigital.h` | Empty base; each digital modem defines its own subclass (e.g., `ModemKitFSK`, `ModemKitGMSK`) with liquid-dsp objects |

### ModemDigital Interface

`ModemDigital` adds these virtual methods beyond the `Modem` interface (defined in `ModemDigital.h`):

| Method | Purpose |
|--------|---------|
| `digitalStart(ModemKitDigital*, modemcf, ModemIQData*)` | Called before demodulation; resizes `demodOutputDataDigital` to match input |
| `digitalFinish(ModemKitDigital*, modemcf)` | Called after demodulation; flushes `outStream` to `ModemDigitalOutput` (when `ENABLE_DIGITAL_LAB` is defined) |
| `setDemodulatorLock(bool)` / `getDemodulatorLock()` | Set/get the demodulator lock state (atomic bool); `getDemodulatorLock()` returns `int` |
| `updateDemodulatorLock(modemcf, float)` | Updates lock state based on EVM threshold from liquid-dsp modem |

## Data Processing

### Demodulation Pipeline

```
DemodulatorPreThread
    | (attaches Modem* and ModemKit* to each IQ packet)
    v
DemodulatorThread
    | calls: cModem->demodulate(cModemKit, &modemData, ati.get())
    v
AudioThreadInput (filled for analog; empty data buffer for digital)
```

Squelch is computed entirely within `DemodulatorThread` — no modem code participates. Signal level is computed per buffer using the source selected by `useSignalOutput()`, with asymmetric attack/decay smoothing. The squelch state is attached to the audio buffer via `ati->is_squelch_active`.

### Analog Modems

- Fill `audioOut->data` with demodulated float audio (mono or stereo)
- `ModemAnalog` base provides `initOutputBuffers()` and `buildAudioOutput()`
- Audio resampling from demod rate to audio rate via `msresamp_rrrf`
- Only ModemFMStereo and ModemIQ produce stereo output (`channels = 2`); all others are mono
- DSP approaches vary significantly: FM uses `freqdem_demodulate_block`, AM uses manual envelope detection (`sqrt(I*I+Q*Q)` + DC blocker, not `ampmodem`), DSB uses `ampmodem`, SSB uses NCO frequency shift + IIR lowpass + Hilbert transform, CW uses complex input resampling + NCO + Hilbert, FM Stereo has a full PLL-based MPX stereo decoder

### Digital Modems

- Fill `demodOutputDataDigital` with symbol indices (except FSK and GMSK, which stream decoded symbols directly to the Digital Lab `outStream` instead; the buffer contents are not consumed downstream)
- Produce no audio directly
- `DemodulatorThread` forwards IQ data to visualization instead

## Modem Selection

1. User selects demod type via UI
2. `DemodulatorInstance::setDemodulatorType(string)` called
3. `DemodulatorPreThread` detects `demodTypeChanged`, sends `DEMOD_WORKER_THREAD_CMD_MAKE_DEMOD` to worker
4. `DemodulatorWorkerThread` calls `Modem::makeModem(type)` and `modem->buildKit()`
5. New Modem* and ModemKit* are bundled with IQ packets for DemodulatorThread

Runtime modem switching: PreThread nulls out modem/kit immediately (packets dropped until worker finishes), then adopts new modem/kit when worker responds.

## Modem Settings

Most modems expose no configurable parameters. The following override `getSettings()`:

**Analog:** ModemCW (offset, auto gain, gain), ModemFMStereo (de-emphasis time constant).

**Digital:** ModemAPSK, ModemASK, ModemDPSK, ModemPSK, ModemQAM, ModemSQAM (constellation size); ModemFSK (bits/symbol, symbols/second, bandwidth); ModemGMSK (filter delay, samples/symbol, excess bandwidth).

Settings that change the liquid-dsp constellation size take effect in place via `updateDemodulatorCons()` without rebuilding the kit. Settings that change signal processing parameters (ModemCW offset, ModemFMStereo de-emphasis, ModemFSK/GMSK symbol parameters) call `rebuildKit()` to recreate the kit with new filter state.

## Available Modems

### Analog (9: 7 ModemAnalog subclasses + 2 direct Modem subclasses)

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

Note: `ModemFMStereo` and `ModemIQ` inherit directly from `Modem`, not from `ModemAnalog`. They are listed here because they produce analog audio output, but they do not use `ModemAnalog`'s resampling infrastructure. Both return `"analog"` from `getType()`, which is how `DemodulatorThread` dispatches them as analog modems despite the non-standard inheritance. `ModemFMStereo` registers under the factory key `"FMS"` (and `getName()` returns `"FMS"`); the mode selector button displays this key directly (the label `"FM Stereo"` appears only in the modem's setting descriptions, e.g. the de-emphasis option).

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

## Supporting Types

**File:** `src/modules/modem/Modem.h`

| Type | Definition | Purpose |
|------|-----------|---------|
| `ModemIQData` | `vector<liquid_float_complex> data` + `long long sampleRate` | Input buffer to `demodulate()` |
| `ModemSettings` | `map<string, string>` | Key-value store for modem parameters |
| `ModemRange` | Double min/max pair | Numeric range for setting constraints |
| `ModemArgInfo` | Struct with key, value, name, description, units, type, range, options, optionNames. Type enum: `BOOL`, `INT`, `FLOAT`, `STRING`, `PATH_DIR`, `PATH_FILE`, `COLOR` | Setting descriptor returned by `getSettings()` |
| `ModemArgInfoList` | `vector<ModemArgInfo>` | Collection of setting descriptors |

**File:** `src/modules/modem/ModemDigital.h`

| Type | Purpose |
|------|---------|
| `ModemDigitalOutput` | Abstract interface for Digital Lab console output (`write()`, `Show()`, `Hide()`, `Close()`). The class is always compiled; its integration with `ModemDigital` (`setOutput()`, `digitalOut`/`outStream` members) is guarded by `#if ENABLE_DIGITAL_LAB`. |

## Adding a New Modem

To add a new modem type:

1. Create `src/modules/modem/analog/ModemXXX.h` and `.cpp` (or `digital/`)
2. Inherit from `ModemAnalog` or `ModemDigital`
3. Implement the pure virtual methods: `getType()`, `getName()`, `checkSampleRate()`, `buildKit()`, `disposeKit()`, `demodulate()`
4. If the modem needs custom kit state, define a `ModemKitXXX` subclass in the header (inherit from `ModemKitAnalog`, `ModemKitDigital`, or `ModemKit` as appropriate)
5. Add a static `factory()` method
6. Register in `CubicSDR::OnInit()` in `src/CubicSDR.cpp` (inside `#ifdef ENABLE_DIGITAL_LAB` for digital modems)
7. Add to `CMakeLists.txt` source list (inside `IF(ENABLE_DIGITAL_LAB)` for digital modems)
