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
| Linux | `~/.cubicsdr` (or `~/.config/CubicSDR` on XDG-compliant builds) |

The directory is created automatically if it doesn't exist.

### Config Files

| File | Purpose |
|------|---------|
| `config.xml` | Default configuration file |
| `config-{name}.xml` | Named configuration (for `-c` command line option) |
| `bookmarks.xml` | Bookmark data |
| `bookmarks.xml.backup` | Bookmark backup |
| `bookmarks.xml.lastloaded` | Last successfully loaded bookmarks |
| `bookmarks.xml.failedload` | Created when a bookmark file fails to load |

Bookmark file loading uses a dialog-interactive recovery sequence across `.backup` and `.lastloaded` fallbacks. The exact behavior depends on the `backup` flag passed to `BookmarkMgr::loadFromFile()` and is detailed in the [Bookmark System](bookmark-system.md) document.

## AppConfig (`src/AppConfig.h`)

Global application configuration. Singleton accessed via `wxGetApp().getConfig()`.

### Window Settings

| Property | Type | Config Key | Default | Description |
|----------|------|------------|---------|-------------|
| `winX`, `winY` | `int` | `window/x`, `window/y` | 0 | Window position |
| `winW`, `winH` | `int` | `window/w`, `window/h` | 0 | Window size |
| `winMax` | `bool` | `window/max` | false | Maximized state |
| `showTips` | `bool` | `window/tips` | true | Show tooltips |
| `modemPropsCollapsed` | `bool` | `window/modemprops_collapsed` | false | Modem properties panel collapsed |
| `perfMode` | `PerfModeEnum` | `window/perf_mode` | 1 (normal) | Performance mode (0=low, 1=normal, 2=high) |

### Display Settings

| Property | Type | Config Key | Default | Description |
|----------|------|------------|---------|-------------|
| `themeId` | `int` | `window/theme` | 0 | Color theme index |
| `fontScale` | `int` | `window/font_scale` | 0 | Font scale (0=normal, 1=medium, 2=large) |
| `snap` | `long long` | `window/snap` | 1 | Frequency snap value in Hz |
| `centerFreq` | `long long` | `window/center_freq` | 100000000 | Center frequency |
| `waterfallLinesPerSec` | `int` | `window/waterfall_lps` | 30 | Waterfall refresh rate |
| `spectrumAvgSpeed` | `float` | `window/spectrum_avg` | 0.65 | Spectrum averaging speed |
| `dbOffset` | `int` | `window/db_offset` | 0 | dB display offset |

### Layout Settings

| Property | Type | Config Key | Default | Description |
|----------|------|------------|---------|-------------|
| `mainSplit` | `float` | `window/main_split` | -1 | Main splitter position |
| `visSplit` | `float` | `window/vis_split` | -1 | Visualization splitter position |
| `bookmarkSplit` | `float` | `window/bookmark_split` | 200 | Bookmark panel splitter position |
| `bookmarksVisible` | `bool` | `window/bookmark_visible` | true* | Bookmark panel visibility |

\* Default is `false` when built with `CUBICSDR_DEFAULT_HIDE_BOOKMARKS` defined.

### Recording Settings

| Property | Type | Config Key | Default | Description |
|----------|------|------------|---------|-------------|
| `recordingPath` | `string` | `recording/path` | *(empty)* | Output directory for recordings |
| `recordingSquelchOption` | `int` | `recording/squelch` | 0 | Squelch handling (0=silence, 1=skip, 2=always) |
| `recordingFileTimeLimitSeconds` | `int` | `recording/file_time_limit` | 0 | Max file duration in seconds (0=unlimited) |

`verifyRecordingPath()` returns `false` (and shows a dialog) if the path is empty, does not exist, or is not writable.

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

The constructor explicitly initializes `rigEnabled` (false), `rigModel` (1), `rigRate` (57600), `rigPort` ("/dev/ttyUSB0"), `rigControlMode` (true), and `rigFollowMode` (true). The remaining fields (`rigCenterLock`, `rigFollowModem`) rely on implicit zero-initialization from `std::atomic`. On load, a zero-valued `model` or `rate` is replaced with the constructor default (1 and 57600 respectively).

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
| `deviceName` | `string` | `name` | Human-readable device name (falls back to `deviceId` if empty) |
| `ppm` | `int` | `ppm` | Parts per million frequency correction |
| `offset` | `long long` | `offset` | Frequency offset in Hz |
| `sampleRate` | `long` | `sample_rate` | Configured sample rate |
| `agcMode` | `bool` | `agc_mode` | AGC enabled (defaults to `true` in the constructor) |
| `antennaName` | `string` | `antenna` | Selected antenna port |
| `streamOpts` | `map<string,string>` | `streamOpts/*` | SoapySDR stream options |
| `settings` | `map<string,string>` | `settings/*` | Driver-specific settings |
| `gains` | `map<string,float>` | `gains/gain/*` | Per-stage gain values |
| `rigIF` | `map<int,long long>` | `rig_ifs/rig_if/*` | Rig IF frequency per model |

Note: `DeviceConfig::load()` does **not** read the `id` node — that is done by `AppConfig::load()`, which extracts the device ID, creates/retrieves the `DeviceConfig` via `getDevice(deviceId)`, then calls `DeviceConfig::load()` on the resulting object.

Note: `DeviceConfig::save()` writes the `antenna`, `streamOpts`, `settings`, `rig_ifs`, and `gains` sections only when the corresponding container is non-empty. On load, `gains` entries whose value is `NaN` are skipped.

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
2. Populate window, recording, device, manual device, and rig nodes. The `window` node and all its children are only written when both `winW` and `winH` are non-zero (a zero-sized window produces no window config).
3. Get config file path from `getConfigFileName()`
4. Call `DataTree::SaveToFileXML()`; `save()` returns `false` and logs an error if the file cannot be written.

**Load** (`AppConfig::load()`):
1. Determine config file path
2. If the named config doesn't exist and a default `config.xml` exists, copy from it. If neither file exists (or the copy fails), `load()` returns `true` early, silently leaving defaults instead of loading.
3. Call `DataTree::LoadFromFileXML()`; if the config file exists but is not readable, `load()` returns `false`.
4. Parse each section: window, recording, devices, manual_devices, rig
5. Missing sections use defaults
6. `perfMode` is unconditionally reset to `PERF_NORMAL` before checking the XML value, so a missing `perf_mode` node always results in `PERF_NORMAL` regardless of any prior value. A present `perf_mode` value is only applied if it equals `PERF_LOW` or `PERF_HIGH`; any other (unrecognized) value leaves it as `PERF_NORMAL`.

`AppConfig::reset()` is declared but is a no-op: it returns `true` without applying or reloading any state, and is not currently invoked.

### Named Configurations

When launched with `-c {name}`, CubicSDR uses `config-{name}.xml` instead of `config.xml`. This allows multiple independent configurations.

## Session Management (`src/SessionMgr.h`)

Sessions capture the complete demodulator state for save/restore.

### Session File Format

```xml
<cubicsdr_session>
  <header>
    <!-- version is percent-encoded, e.g. "0.2.8" ~> "%30%2e%32%2e%38" -->
    <version>%30%2e%32%2e%38</version>
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

Note: The `<version>` element is stored internally as a percent-encoded `wstring` (not plain text), so the earlier example shows its encoded form. The `<view_state>` section is only written when the waterfall canvas view state is active; it is absent when no view state is saved.

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

`DataTree` has two constructors: a default constructor (root node unnamed, set via `setName()`) and `DataTree(const char *name_in)` which names the root node directly. `SessionMgr` uses the latter.

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
| `getParentNode()` / `setParentNode()` | Parent node navigation |
| `newChild(name)` | Create and append a child node |
| `newChild(name, otherNode)` | Append an existing child node (takes ownership, does not clone) |
| `newChildCloneFrom(name, cloneFrom)` | Deep-clone a child node tree from another DataNode |
| `child(index)` | Get child by index |
| `child(name, index=0)` | Get child by name (with optional index) |
| `numChildren()` | Number of children |
| `numChildren(name)` | Number of children matching a specific name |
| `hasAnother(name)` | Check if another child with name exists (iterator) |
| `hasAnother()` | Check if any next child exists (iterator) |
| `getNext(name)` | Get next child with name (iterator) |
| `getNext()` | Get next child (iterator) |
| `rewind(name)` / `rewind()` | Reset iterator to beginning (by name or generic) |
| `rewindAll()` | Reset all iterators |
| `element()` | Get the `DataElement` value |
| `findAll(name, list)` | Recursively find all descendants matching name |

DataNode also provides operator overloads that are the idiomatic access pattern throughout the codebase:
- Cast operators (`operator string`, `operator const char*`, `operator int`, `operator float`, etc.) for reading values. `operator const char*` returns `nullptr` if the type is not `DATA_STRING`.
- `operator[]` maps to `getNext(name)` or `child(idx)`
- `operator()` maps to `hasAnother(name)`
- `operator^` maps to `newChild(name)`

### DataElement Types

Supported scalar types:
- `char`, `unsigned char`, `int`, `unsigned int`, `long`, `unsigned long`, `long long`
- `float`, `double`
- `std::string`, `std::wstring`

Supported vector types:
- `std::vector<char>`, `std::vector<unsigned char>`
- `std::vector<int>`, `std::vector<unsigned int>`
- `std::vector<long>`, `std::vector<unsigned long>`, `std::vector<long long>`
- `std::vector<float>`, `std::vector<double>`
- `std::vector<std::string>`

Supported set types:
- `std::set<std::string>`

Additionally, `DataElement` supports raw byte buffers via `set(const char*, long)` (stored as `DATA_VOID`).

### XML Serialization

`DataTree::SaveToFileXML()` and `DataTree::LoadFromFileXML()` use TinyXML for XML I/O. The XML structure maps directly to the DataNode tree:
- Element names → `DataNode::getName()`
- Element text content → `DataElement` value
- Child elements → child `DataNode`s
- Element attributes → `DataNode`s whose name is prefixed with `@` (BadgerFish convention). A `DataNode` named e.g. `@name` is written as (and read back from) the XML attribute `name`.
- `std::vector<std::string>` values serialize as a sequence of child `<str>` elements, and are deserialized back into a string vector when an element's children are all named `str`.
- A `DataNode` with an empty name is written using the element name `node`.

`LoadFromFileXML` accepts a `DT_FloatingPointPolicy` parameter (`USE_FLOAT` or `USE_DOUBLE`) that controls whether floating-point XML text values are parsed as `float` or `double`. The default is `USE_FLOAT`.

`DataTree` also provides `printXML()` for debugging, which outputs the tree structure to stdout.

### Thread Safety

`DataTree` and `DataNode` are **not thread-safe**. Config save/load happens on the main thread during UI events.

Both `AppConfig` and `DeviceConfig` use `std::atomic` for scalar fields to allow safe reads/writes from different threads without locking. `DeviceConfig` additionally uses `busy_lock` (a `std::mutex`) to protect the `deviceId` and `deviceName` strings, and the `save()`/`load()` methods. However, individual accessors for `antennaName`, `streamOpts`, `gains`, `settings`, and `rigIF` do **not** acquire the mutex — they are only safe when accessed from the same thread that calls `save()`/`load()`. `AppConfig` does not use a mutex for any of its non-atomic fields (`recordingPath`, `rigPort`, `configName`, `manualDevices`).

`BookmarkMgr` uses a `std::recursive_mutex` (`busy_lock`) to protect all bookmark data access. The `saveToFile()` and `loadFromFile()` methods acquire this lock, as do all bookmark add/remove/query operations.

## Config Save Triggers

- **On window close:** `AppFrame::OnClose()` snapshots selected UI state (window geometry, theme, font scale, snap, center frequency, spectrum/waterfall speeds, manual devices, modem properties collapsed, splitter positions, bookmarks visibility, and hamlib settings) into `AppConfig`, calls `config->save()`, then saves bookmarks via `BookmarkMgr::saveToFile()`. If the `saveDisabled` flag is set, the handler returns early without saving. Recording settings are not re-snapshotted here; they are already current in `AppConfig` from earlier menu handler calls.
- **On setting change:** various UI actions update the in-memory config immediately (e.g., PPM changes in `TuningCanvas` → `CubicSDR::setPPM()` → `SDRThread::setPPM()` → `DeviceConfig::setPPM()`). The config file is only written to disk on window close, or explicitly via `CubicSDR::saveConfig()` (e.g., `TuningCanvas::OnMouseLeftWindow` saves if PPM changed).
- **Session save:** explicit user action via File menu
