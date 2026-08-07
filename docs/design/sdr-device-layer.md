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

The static `enumerate_devices(remoteAddr, noInit)` method performs the actual work. When `noInit` is `true`, it returns cached results from `devs[remoteAddr]` (or `nullptr` if empty) without re-initializing SoapySDR — used by the UI to retrieve already-enumerated devices. When `noInit` is `false` (default), it performs full initialization and enumeration if the cache is empty.

The `run()` method drives enumeration in two phases: local then remote. Manual devices are enumerated within each phase, not separately.

1. **SoapySDR initialization** (once, in `enumerate_devices()`):
   - Load SoapySDR modules (system, bundled, or user-specified path)
   - Discover available factory functions (driver names)
   - Detect if `remote` factory is available

2. **Local enumeration** (`enumerate_devices("")`):
   - Call `SoapySDR::Device::enumerate()` with no arguments
   - Append manual device results to the same results vector (see step 4)
   - For each result, create `SDRDeviceInfo` and populate driver/name
   - Attempt `SoapySDR::Device::make()` to query hardware info
   - Apply saved device settings from `DeviceConfig`
   - Store all devices (local + manual) in `devs[""]` (empty string = local)

3. **Remote enumeration** (`enumerate_devices(remoteAddr)` for each remote):
   - Call `SoapySDR::Device::enumerate()` with `driver=remote,remote={addr}`
   - Append manual device results to the same results vector (see step 4)
   - Same device creation and info querying as local
   - Store all devices (remote + manual) in `devs[remoteAddr]`

4. **Manual device enumeration** (inside each `enumerate_devices()` call):
   - For each entry in `manuals` list
   - Call `SoapySDR::Device::enumerate()` with `driver={factory},{params}`
   - If enumeration fails, create device entry marked as unavailable with label "Not Found ({factory})"
   - Results are appended to the current enumeration's results vector, so manual devices appear in both `devs[""]` and `devs[remoteAddr]`

### Static State

| Map/Vector | Type | Purpose |
|------------|------|---------|
| `devs` | `map<string, vector<SDRDeviceInfo*>>` | Devices keyed by remote address ("" = local) |
| `factories` | `vector<string>` | Available SoapySDR driver names |
| `modules` | `vector<string>` | Loaded SoapySDR module paths |
| `remotes` | `vector<string>` | Configured remote server addresses |
| `manuals` | `vector<SDRManualDef>` | Manual device definitions |
| `soapy_initialized` | `bool` | Whether SoapySDR modules have been loaded |
| `has_remote` | `bool` | Whether the `remote` driver factory is available |

The `reset()` method clears `soapy_initialized`, `factories`, `modules`, and `devs` (unmaking each device's SoapySDR pointer first), but does **not** clear `remotes` or `manuals` — these persist across re-enumerations.

### Notification

`SDREnumerator` sends notifications to the UI via `wxGetApp().sdrEnumThreadNotify()`:

| State | Meaning |
|-------|---------|
| `SDR_ENUM_MESSAGE` | Status message (displayed in UI) |
| `SDR_ENUM_DEVICES_READY` | Enumeration complete, devices available |
| `SDR_ENUM_FAILED` | No modules available (factory list contains exactly one entry and it is `null`). Sent during `enumerate_devices()` initialization, not after — enumeration continues and `SDR_ENUM_DEVICES_READY` is still sent at the end of `run()` |
| `SDR_ENUM_TERMINATED` | Thread terminated (defined but not currently sent) |

## SDRDeviceInfo (`src/sdr/SDRDeviceInfo.h`)

Represents a discovered or manually defined SDR device.

### Identity

| Property | Type | Description |
|----------|------|-------------|
| `name` | `string` | Display name (from SoapySDR `label` or `device` field); set during enumeration |
| `serial` | `string` | Device serial number (setter exists but is never called) |
| `driver` | `string` | SoapySDR driver name; set during enumeration |
| `hardware` | `string` | Hardware revision (set during enumeration, populated from `getHardwareInfo()`) |
| `tuner` | `string` | Tuner chip type (setter exists but is never called) |
| `manufacturer` | `string` | Device manufacturer (setter exists but is never called) |
| `product` | `string` | Product name (setter exists but is never called) |

### Device ID

`getDeviceId()` returns the device's display name (`getName()`).

### State

| Property | Type | Description |
|----------|------|-------------|
| `available` | `bool` | Device was successfully queried |
| `active` | `atomic_bool` | Device is currently in use |
| `remote` | `bool` | Device is on a remote server |
| `manual` | `bool` | Device was manually defined |
| `manual_params` | `string` | Raw parameter string from `SDRManualDef` (used to match devices during manual removal) |
| `timestamps` | `bool` | Device supports timestamped streaming (setter exists but is never called) |

### Hardware Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getIndex()` | `int` | Device enumeration index |
| `getSampleRates(direction, channel)` | `vector<long>` | Available sample rates |
| `getSampleRateNear(direction, channel, rate)` | `long` | Nearest supported sample rate |
| `getAntennaNames(direction, channel)` | `vector<string>` | Available antenna ports |
| `getAntennaName(direction, channel)` | `string` | Currently selected antenna |
| `getGains(direction, channel)` | `SDRRangeMap` | Per-stage gain ranges |
| `getCurrentGain(direction, channel, name)` | `double` | Current gain value |
| `hasCORR(direction, channel)` | `bool` | Supports DC offset correction (deprecated) |

### Device and Stream Arguments

| Method | Returns | Description |
|--------|---------|-------------|
| `getDeviceArgs()` | `SoapySDR::Kwargs` | Device identification arguments |
| `setDeviceArgs(kwargs)` | `void` | Set device identification arguments |
| `getStreamArgs()` | `SoapySDR::Kwargs` | Stream configuration arguments |
| `setStreamArgs(kwargs)` | `void` | Set stream configuration arguments |

### SoapySDR Integration

`SDRDeviceInfo` holds a `SoapySDR::Device*` pointer for direct hardware access:
- `getSoapyDevice()` lazily creates the device via `SoapySDR::Device::make()` on first call, caching the pointer
- `setSoapyDevice()` replaces or clears the pointer (unmaking any existing device first)
- Used to query capabilities (sample rates, gains, antennas)
- Released via `SoapySDR::Device::unmake()` in the destructor

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
`SDRThread` carries a `SoapySDR::Kwargs streamArgs` member that stores and applies driver-specific stream configuration (the set of possible arguments is declared by the device's `getStreamArgsInfo()`, not by `SDRThread`). When a device is selected in the UI, `SDRDevicesDialog::OnUseSelected()` queries `getStreamArgsInfo(SOAPY_SDR_RX, 0)`, populates the `streamArgs` map from the edited property values, persists them via `DeviceConfig::setStreamOpts()`, and pushes them through `CubicSDR::setStreamArgs()` into `SDRThread::setStreamArgs()`. `SDRThread` itself does not add keys; it applies the selections received from the dialog.

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
- Remote device add (via `CubicSDR::addRemote`); no UI exists to remove a remote device — `removeRemote` is declared but never invoked
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
   - If `BUNDLED_MODS_ONLY` is defined, load only bundled modules from `modules/` subdirectory next to the executable
   - Otherwise, load bundled modules and optionally system modules based on the `getUseLocalMod()` runtime preference
3. **System modules** (default `SoapySDR::loadModules()`)

Module discovery is controlled by `SoapySDR::listModules()` and `SoapySDR::loadModule()`.
