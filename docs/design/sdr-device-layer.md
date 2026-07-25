# SDR Device Layer

This document describes CubicSDR's SDR hardware abstraction layer, device discovery, device information model, and manual device support.

## Overview

CubicSDR uses SoapySDR as its hardware abstraction layer. The device layer handles:

- **Device discovery:** Enumerating local and remote SDR hardware
- **Device information:** Querying capabilities (sample rates, gains, antennas)
- **Manual devices:** Adding devices not auto-discovered
- **Device configuration:** Per-device settings persistence

```
SDREnumerator (background thread)
    |
    +-- SoapySDR::Device::enumerate()
    |
    v
SDRDeviceInfo (per-device capabilities)
    |
    v
SDRDevicesDialog (UI selection)
    |
    v
SDRThread (active device usage)
```

## SDREnumerator (`src/sdr/SDREnumerator.h`)

Background `IOThread` that discovers available SDR devices. Results are cached in a static map.

### Enumeration Flow

1. **SoapySDR initialization** (once):
   - Load SoapySDR modules (system, bundled, or user-specified path)
   - Discover available factory functions (driver names)
   - Detect if `remote` factory is available

2. **Local enumeration:**
   - Call `SoapySDR::Device::enumerate()` with no arguments
   - For each result, create `SDRDeviceInfo` and populate driver/name
   - Attempt `SoapySDR::Device::make()` to query hardware info
   - Apply saved device settings from `DeviceConfig`
   - Store in `devs[""]` (empty string = local)

3. **Remote enumeration:**
   - For each address in `remotes` list
   - Call `SoapySDR::Device::enumerate()` with `driver=remote,remote={addr}`
   - Same device creation and info querying as local
   - Store in `devs[remoteAddr]`

4. **Manual device enumeration:**
   - For each entry in `manuals` list
   - Call `SoapySDR::Device::enumerate()` with `driver={factory},{params}`
   - If enumeration fails, create device entry marked as unavailable with label "Not Found ({factory})"
   - Store in `devs[""]` (same as local)

### Static State

| Map/Vector | Type | Purpose |
|------------|------|---------|
| `devs` | `map<string, vector<SDRDeviceInfo*>>` | Devices keyed by remote address ("" = local) |
| `factories` | `vector<string>` | Available SoapySDR driver names |
| `modules` | `vector<string>` | Loaded SoapySDR module paths |
| `remotes` | `vector<string>` | Configured remote server addresses |
| `manuals` | `vector<SDRManualDef>` | Manual device definitions |

### Notification

`SDREnumerator` sends notifications to the UI via `wxGetApp().sdrEnumThreadNotify()`:

| State | Meaning |
|-------|---------|
| `SDR_ENUM_MESSAGE` | Status message (displayed in UI) |
| `SDR_ENUM_DEVICES_READY` | Enumeration complete, devices available |
| `SDR_ENUM_FAILED` | No modules available |
| `SDR_ENUM_TERMINATED` | Thread terminated |

## SDRDeviceInfo (`src/sdr/SDRDeviceInfo.h`)

Represents a discovered or manually defined SDR device.

### Identity

| Property | Type | Description |
|----------|------|-------------|
| `name` | `string` | Display name (from SoapySDR `label` or `device` field) |
| `serial` | `string` | Device serial number |
| `driver` | `string` | SoapySDR driver name |
| `hardware` | `string` | Hardware revision |
| `tuner` | `string` | Tuner chip type |
| `manufacturer` | `string` | Device manufacturer |
| `product` | `string` | Product name |

### Device ID

`getDeviceId()` returns a unique identifier string. For local devices, this is typically the driver name and serial number. For remote devices, it includes the remote address. For manual devices, it includes the factory and parameters.

### State

| Property | Type | Description |
|----------|------|-------------|
| `available` | `bool` | Device was successfully queried |
| `active` | `atomic_bool` | Device is currently in use |
| `remote` | `bool` | Device is on a remote server |
| `manual` | `bool` | Device was manually defined |
| `timestamps` | `bool` | Device supports timestamped streaming |

### Hardware Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getSampleRates(direction, channel)` | `vector<long>` | Available sample rates |
| `getSampleRateNear(direction, channel, rate)` | `long` | Nearest supported sample rate |
| `getAntennaNames(direction, channel)` | `vector<string>` | Available antenna ports |
| `getAntennaName(direction, channel)` | `string` | Currently selected antenna |
| `getGains(direction, channel)` | `SDRRangeMap` | Per-stage gain ranges |
| `getCurrentGain(direction, channel, name)` | `double` | Current gain value |
| `hasCORR(direction, channel)` | `bool` | Supports DC offset correction |

### SoapySDR Integration

`SDRDeviceInfo` holds a `SoapySDR::Device*` pointer for direct hardware access:
- Set via `setSoapyDevice()` after `SoapySDR::Device::make()`
- Used to query capabilities (sample rates, gains, antennas)
- Released via `SoapySDR::Device::unmake()` after enumeration

## Device Arguments

SoapySDR devices are configured via key-value argument strings:

| Argument | Purpose |
|----------|---------|
| `driver` | SoapySDR driver module name |
| `device` | Device identifier |
| `serial` | Device serial number |
| `remote` | Remote server address |
| `label` | Human-readable device label |

**Stream arguments** (stored in `streamArgs`):
| Argument | Purpose |
|----------|---------|
| `buflen` | Buffer length |
| `bufflen` | Alternative buffer length |
| `remote` | Remote streaming address |

**Device settings** (stored in `DeviceConfig::settings`):
- Driver-specific settings (e.g., `bias_tee` for RTL-SDR)
- Applied during device enumeration and on device open

## Manual Device Definition

`SDRManualDef` allows users to add devices not auto-discovered:

```cpp
struct SDRManualDef {
    std::string factory;  // SoapySDR driver name
    std::string params;   // Comma-separated key=value pairs
};
```

**Example:**
- Factory: `rtlsdr`
- Params: `driver=rtlsdr,bias_tee=1`

Manual devices are:
1. Stored in `AppConfig` under `manual_devices`
2. Loaded into `SDREnumerator::manuals` at startup
3. Enumerated alongside local devices
4. If not found, shown as unavailable with "Not Found" label

## Device Selection UI

`SDRDevicesDialog` (`src/forms/SDRDevices/`) provides:

- List of all discovered devices (local, remote, manual)
- Device properties display (sample rates, gains, antennas)
- Manual device add/remove
- Remote device add/remove
- Device activation (triggers `SDRThread` creation)

## Device Configuration

Per-device settings are persisted in `DeviceConfig` (see [Configuration System](configuration-system.md)):

- Sample rate, frequency offset, PPM correction
- AGC mode, antenna selection
- Per-gain-stage values
- Stream options
- Driver-specific settings
- Rig IF frequencies

## Module Loading

SoapySDR modules are loaded in this order:

1. **User-specified path** (`-m` command line option)
2. **Bundled modules** (if `BUNDLE_SOAPY_MODS` is defined):
   - Check `modules/` subdirectory next to the executable
   - Optionally load system modules first (if `BUNDLED_MODS_ONLY` is not defined)
3. **System modules** (default `SoapySDR::loadModules()`)

Module discovery is controlled by `SoapySDR::listModules()` and `SoapySDR::loadModule()`.
