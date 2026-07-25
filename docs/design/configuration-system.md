# Configuration System

This document describes CubicSDR's configuration persistence, the `AppConfig`/`DeviceConfig` classes, session management, and the DataTree serialization format.

## Overview

CubicSDR persists its configuration using XML files stored in the platform's standard user data directory. The configuration system is built on `DataTree`, a hierarchical data structure backed by TinyXML.

```
AppConfig (global settings)
    |
    +-- DeviceConfig (per-device settings, keyed by device ID)
    |
    +-- Manual device definitions
    +-- Hamlib rig settings (optional)

SessionMgr (session state)
    |
    +-- Demodulator instances
    +-- View state
```

## File Locations

### Config Directory

`AppConfig::getConfigDir()` returns the platform-specific user data directory via `wxStandardPaths::Get().GetUserDataDir()`:

| Platform | Path |
|----------|------|
| Windows | `%APPDATA%/CubicSDR` |
| macOS | `~/Library/Application Support/CubicSDR` |
| Linux | `~/.cubicsdr` |

The directory is created automatically if it doesn't exist.

### Config Files

| File | Purpose |
|------|---------|
| `config.xml` | Default configuration file |
| `config-{name}.xml` | Named configuration (for `-c` command line option) |
| `bookmarks.xml` | Bookmark data |
| `bookmarks.xml.backup` | Bookmark backup |
| `bookmarks.xml.lastloaded` | Last successfully loaded bookmarks |

## AppConfig (`src/AppConfig.h`)

Global application configuration. Singleton accessed via `wxGetApp().getConfig()`.

### Window Settings

| Property | Type | Config Key | Description |
|----------|------|------------|-------------|
| `winX`, `winY` | `int` | `window/x`, `window/y` | Window position |
| `winW`, `winH` | `int` | `window/w`, `window/h` | Window size |
| `winMax` | `bool` | `window/max` | Maximized state |
| `showTips` | `bool` | `window/tips` | Show tooltips |
| `modemPropsCollapsed` | `bool` | `window/modemprops_collapsed` | Modem properties panel collapsed |
| `perfMode` | `PerfModeEnum` | `window/perf_mode` | Performance mode (0=low, 1=normal, 2=high) |

### Display Settings

| Property | Type | Config Key | Default | Description |
|----------|------|------------|---------|-------------|
| `themeId` | `int` | `window/theme` | 0 | Color theme index |
| `fontScale` | `int` | `window/font_scale` | 0 | Font scale (0=normal, 1=medium, 2=large) |
| `snap` | `long long` | `window/snap` | 0 | Frequency snap value in Hz |
| `centerFreq` | `long long` | `window/center_freq` | 0 | Center frequency |
| `waterfallLinesPerSec` | `int` | `window/waterfall_lps` | 30 | Waterfall refresh rate |
| `spectrumAvgSpeed` | `float` | `window/spectrum_avg` | — | Spectrum averaging speed |
| `dbOffset` | `int` | `window/db_offset` | 0 | dB display offset |

### Layout Settings

| Property | Type | Config Key | Description |
|----------|------|------------|-------------|
| `mainSplit` | `float` | `window/main_split` | Main splitter position |
| `visSplit` | `float` | `window/vis_split` | Visualization splitter position |
| `bookmarkSplit` | `float` | `window/bookmark_split` | Bookmark panel splitter position |
| `bookmarksVisible` | `bool` | `window/bookmark_visible` | Bookmark panel visibility |

### Recording Settings

| Property | Type | Config Key | Description |
|----------|------|------------|-------------|
| `recordingPath` | `string` | `recording/path` | Output directory for recordings |
| `recordingSquelchOption` | `int` | `recording/squelch` | Squelch handling (0=silence, 1=skip, 2=always) |
| `recordingFileTimeLimitSeconds` | `int` | `recording/file_time_limit` | Max file duration in seconds (0=unlimited) |

### Hamlib Settings (conditional on `USE_HAMLIB`)

| Property | Type | Config Key | Description |
|----------|------|------------|-------------|
| `rigEnabled` | `bool` | `rig/enabled` | Hamlib enabled |
| `rigModel` | `int` | `rig/model` | Hamlib rig model ID |
| `rigRate` | `int` | `rig/rate` | Serial baud rate |
| `rigPort` | `string` | `rig/port` | Serial port path |
| `rigControlMode` | `bool` | `rig/control` | Control rig frequency |
| `rigFollowMode` | `bool` | `rig/follow` | Follow rig frequency |
| `rigCenterLock` | `bool` | `rig/center_lock` | Lock center frequency to rig |
| `rigFollowModem` | `bool` | `rig/follow_modem` | Follow active modem frequency |

### Manual Devices

Stored as a list under `manual_devices/device`:
```xml
<manual_devices>
  <device>
    <factory>remote</factory>
    <params>driver=rtlsdr</params>
  </device>
</manual_devices>
```

## DeviceConfig (`src/AppConfig.h`)

Per-device configuration, keyed by device ID string. Stored under `devices/device` in the config XML.

### Properties

| Property | Type | Config Key | Description |
|----------|------|------------|-------------|
| `deviceId` | `string` | `id` | Device identifier |
| `deviceName` | `string` | `name` | Human-readable device name |
| `ppm` | `int` | `ppm` | Parts per million frequency correction |
| `offset` | `long long` | `offset` | Frequency offset in Hz |
| `sampleRate` | `long` | `sample_rate` | Configured sample rate |
| `agcMode` | `bool` | `agc_mode` | AGC enabled |
| `antennaName` | `string` | `antenna` | Selected antenna port |
| `streamOpts` | `map<string,string>` | `streamOpts/*` | SoapySDR stream options |
| `settings` | `map<string,string>` | `settings/*` | Driver-specific settings |
| `gains` | `map<string,float>` | `gains/gain/*` | Per-stage gain values |
| `rigIF` | `map<int,long long>` | `rig_ifs/rig_if/*` | Rig IF frequency per model |

### Device Config XML Structure

```xml
<device>
  <id>remote=0:driver=rtlsdr</id>
  <name>RTL-SDR</name>
  <ppm>0</ppm>
  <offset>0</offset>
  <sample_rate>2400000</sample_rate>
  <agc_mode>1</agc_mode>
  <antenna>antenna0</antenna>
  <streamOpts>
    <buflen>16384</buflen>
  </streamOpts>
  <settings>
    <bias_tee>0</bias_tee>
  </settings>
  <gains>
    <gain>
      <id>LNA</id>
      <value>40.0</value>
    </gain>
  </gains>
  <rig_ifs>
    <rig_if>
      <model>1</model>
      <sdr_if>145000000</sdr_if>
    </rig_if>
  </rig_ifs>
</device>
```

## Config File Format

### Root Structure

```xml
<cubicsdr_config>
  <window>...</window>
  <recording>...</recording>
  <devices>...</devices>
  <manual_devices>...</manual_devices>
  <rig>...</rig>
</cubicsdr_config>
```

### Save/Load Lifecycle

**Save** (`AppConfig::save()`):
1. Create a `DataTree` with root node named `cubicsdr_config`
2. Populate window, recording, device, manual device, and rig nodes
3. Get config file path from `getConfigFileName()`
4. Call `DataTree::SaveToFileXML()`

**Load** (`AppConfig::load()`):
1. Determine config file path
2. If named config doesn't exist, copy from default `config.xml`
3. Call `DataTree::LoadFromFileXML()`
4. Parse each section: window, recording, devices, manual_devices, rig
5. Missing sections use defaults

### Named Configurations

When launched with `-c {name}`, CubicSDR uses `config-{name}.xml` instead of `config.xml`. This allows multiple independent configurations.

## Session Management (`src/SessionMgr.h`)

Sessions capture the complete demodulator state for save/restore.

### Session File Format

```xml
<cubicsdr_session>
  <header>
    <version>0.2.8</version>
    <center_freq>145000000</center_freq>
    <sample_rate>2400000</sample_rate>
    <solo_mode>0</solo_mode>
    <view_state>
      <center_freq>145000000</center_freq>
      <bandwidth>2400000</bandwidth>
    </view_state>
  </header>
  <demodulators>
    <demodulator>
      <frequency>145300000</frequency>
      <bandwidth>12500</bandwidth>
      <type>NBFM</type>
      ...
    </demodulator>
    <!-- more demodulators -->
  </demodulators>
</cubicsdr_session>
```

### Save Flow (`SessionMgr::saveSession()`)

1. Create `DataTree` with root `cubicsdr_session`
2. Write header: version, center frequency, sample rate, solo mode, view state
3. Iterate all demodulator instances, call `DemodulatorMgr::saveInstance()` for each
4. Ensure filename ends in `.xml`
5. Save to file

### Load Flow (`SessionMgr::loadSession()`)

1. Load `DataTree` from file
2. Validate root node name is `cubicsdr_session`
3. Terminate all existing demodulators
4. Parse header: version, sample rate (clamped to device limits), solo mode
5. Parse demodulators: create each via `DemodulatorMgr::loadInstance()`, call `run()`, set active
6. Restore center frequency and view state
7. Set active demodulator

## DataTree (`src/util/DataTree.h`)

The serialization framework underlying all configuration and session files.

### Architecture

```
DataTree
  |
  +-- rootNode: DataNode (named, has children)
        |
        +-- DataElement (typed value: char, int, float, double, string, wstring, vectors)
        |
        +-- child DataNodes (recursive tree)
```

### DataNode Operations

| Method | Purpose |
|--------|---------|
| `setName()` / `getName()` | Node name (maps to XML element name) |
| `newChild(name)` | Create and append a child node |
| `child(index)` | Get child by index |
| `child(name)` | Get first child by name |
| `numChildren()` | Number of children |
| `hasAnother(name)` | Check if another child with name exists (iterator) |
| `getNext(name)` | Get next child with name (iterator) |
| `element()` | Get the `DataElement` value |

### DataElement Types

Supported scalar types:
- `char`, `short`, `int`, `long`, `long long`
- `float`, `double`
- `std::string`, `std::wstring`

Supported vector types:
- `std::vector<int>`, `std::vector<float>`, `std::vector<double>`

### XML Serialization

`DataTree::SaveToFileXML()` and `DataTree::LoadFromFileXML()` use TinyXML for XML I/O. The XML structure maps directly to the DataNode tree:
- Element names → `DataNode::getName()`
- Element text content → `DataElement` value
- Child elements → child `DataNode`s

### Thread Safety

`DataTree` and `DataNode` are **not thread-safe**. Config save/load happens on the main thread during UI events. The `DeviceConfig` class uses `busy_lock` (a `std::mutex`) to protect concurrent access to individual device config data.

## Config Save Triggers

- **On exit:** `CubicSDR::OnExit()` calls `config->save()`
- **On window move/resize:** debounced save of window geometry
- **On setting change:** various UI actions update config immediately
- **Session save:** explicit user action via File menu
