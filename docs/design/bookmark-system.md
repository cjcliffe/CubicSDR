# Bookmark System

This document describes CubicSDR's bookmark management system, data model, persistence, and the bookmark view UI.

## Overview

The bookmark system provides persistent storage for frequently visited frequencies and demodulator configurations. It includes four categories: active demodulators, bookmarked entries, recent entries, and frequency band ranges.

```
BookmarkMgr
    |
    +-- bmData (map<group_name, BookmarkList>)  -- user-defined bookmark groups
    +-- bmDataSorted (map<group_name, bool>)    -- per-group sort cache
    +-- recents (BookmarkList)                   -- recently used demodulators
    +-- ranges (BookmarkRangeList)               -- frequency band ranges
    +-- rangesSorted (bool)                     -- ranges sort cache
    +-- expandState (map<name, bool>)            -- bookmark group expand/collapse state
```

## Data Model

### BookmarkEntry (`src/BookmarkMgr.h`)

Represents a saved demodulator configuration:

| Field | Type | Description |
|-------|------|-------------|
| `type` | `string` | Demodulator type (e.g., "FM", "NBFM", "USB") |
| `label` | `wstring` | User-assigned label |
| `frequency` | `long long` | Center frequency in Hz |
| `bandwidth` | `int` | Demodulator bandwidth in Hz |
| `node` | `DataNode*` | Full demodulator state (serialized via `DemodulatorMgr::saveInstance()`) |

The `node` field stores the complete demodulator configuration including modem settings, gain, squelch, output device, and other parameters. This allows exact restoration when a bookmark is loaded. The class has no constructor, so `frequency`, `bandwidth`, and `node` are all uninitialized when a `BookmarkEntry` is default-constructed. The destructor calls `delete node` on this raw pointer, which is undefined behavior if `node` was never assigned. In practice, all construction paths (`demodToBookmarkEntry()` and `nodeToBookmark()`) assign `node`.

### BookmarkRangeEntry (`src/BookmarkMgr.h`)

Represents a named frequency band. Unlike `BookmarkEntry`, this class has constructors that initialize all numeric fields to zero:

| Field | Type | Description |
|-------|------|-------------|
| `label` | `wstring` | Band name (e.g., "2 Meters (144-148 MHz)") |
| `freq` | `long long` | Center frequency in Hz |
| `startFreq` | `long long` | Band start frequency in Hz |
| `endFreq` | `long long` | Band end frequency in Hz |

### Type Aliases

| Alias | Definition |
|-------|------------|
| `BookmarkEntryPtr` | `shared_ptr<BookmarkEntry>` |
| `BookmarkRangeEntryPtr` | `shared_ptr<BookmarkRangeEntry>` |
| `BookmarkList` | `vector<BookmarkEntryPtr>` |
| `BookmarkRangeList` | `vector<BookmarkRangeEntryPtr>` |
| `BookmarkMap` | `map<string, BookmarkList>` |
| `BookmarkMapSorted` | `map<string, bool>` — per-group sort cache |
| `BookmarkNames` | `vector<string>` — used by `getGroups()` |
| `BookmarkExpandState` | `map<string, bool>` — bookmark group expand states |

## BookmarkMgr (`src/BookmarkMgr.h`)

Singleton managed by `CubicSDR`. Most public methods are thread-safe via `recursive_mutex` (`busy_lock`). Exceptions include `updateActiveList()`, `updateBookmarks()`, `hasLastLoad()`, `hasBackup()`, `removeActive()`, and the static methods `getBookmarkEntryDisplayName()` and `getActiveDisplayName()` which do not acquire the lock.

### Bookmark Operations

| Method | Description |
|--------|-------------|
| `addBookmark(group, demod)` | Create bookmark from live demodulator |
| `addBookmark(group, entry)` | Add pre-built bookmark entry |
| `removeBookmark(group, entry)` | Remove from specific group |
| `removeBookmark(entry)` | Remove from all groups |
| `moveBookmark(entry, group)` | Move entry between groups |
| `getBookmarks(group)` | Get independent copy of group's bookmarks (sorted by frequency; sorts internal list as a side effect) |
| `removeActive(demod)` | Deactivate and destroy a live demodulator instance (clears active context, removes from demod list, deletes thread) |
| `resetBookmarks()` | Clear all data and reload default ranges |
| `hasLastLoad(bookmarkFn)` | Check if `.lastloaded` backup exists (always checks in config dir) |
| `hasBackup(bookmarkFn)` | Check if `.backup` file exists (always checks in config dir) |
| `saveToFile(bookmarkFn, backup, useFullpath)` | Save bookmarks to XML file |
| `loadFromFile(bookmarkFn, backup, useFullpath)` | Load bookmarks from XML file |
| `updateActiveList()` | Trigger async UI refresh of active demodulators |
| `updateBookmarks()` | Trigger async UI refresh of all bookmark groups |
| `updateBookmarks(group)` | Trigger async UI refresh of a specific bookmark group |

### Group Operations

| Method | Description |
|--------|-------------|
| `addGroup(name)` | Create empty group (no-op if group already exists). Does not set an expand state entry; `getExpandState()` returns `true` for unknown keys, so new groups default to expanded |
| `removeGroup(name)` | Delete group and all its bookmarks |
| `renameGroup(old, new)` | Rename group (if target already exists, merges entries from old group into target) |
| `getGroups(arr)` | Append group names to array (accepts `BookmarkNames&` or `wxArrayString&`; does not clear the array first). Names are returned in map-sorted (alphabetical) order, which determines the display order in the tree |

> **Note:** `removeGroup()` does not clean up the corresponding entry in `expandState`, and `renameGroup()` does not migrate the old group's expand state to the new name. Both leave stale entries in `BookmarkMgr::expandState`. This has no visible effect because `getExpandState()` returns `true` for unknown keys, but the stale entries accumulate over time.

### Recent Entries

| Method | Description |
|--------|-------------|
| `addRecent(demod)` | Create recent entry from live demodulator (serializes state) |
| `addRecent(entry)` | Add pre-built bookmark entry to recents |
| `removeRecent(entry)` | Remove from recents |
| `getRecents()` | Get independent copy of recents list |
| `clearRecents()` | Empty recents list |

Recents are capped at `BOOKMARK_RECENTS_MAX` (25 entries). The oldest entry is removed each time a new entry is added when at capacity (soft limit enforced one-at-a-time). The internal `trimRecents()` method does not acquire the lock; it is always called from within a locked context (`addRecent`). Deleting an active demodulator (via `DemodulatorMgr::deleteThread()`, reached from `removeActive()` or the delete key) also adds it to recents before the instance is terminated.

### Range Operations

| Method | Description |
|--------|-------------|
| `addRange(entry)` | Add frequency band range |
| `removeRange(entry)` | Remove range |
| `getRanges()` | Get independent copy of ranges list (sorted by center frequency) |
| `clearRanges()` | Empty ranges list |

### Default Ranges

On first run (no bookmark file exists), `loadDefaultRanges()` populates standard amateur radio bands:

| Band | Frequency Range |
|------|----------------|
| 2200 Meters | 135.7–137.8 kHz |
| 630 Meters | 472–479 kHz |
| 160 Meters | 1.8–2 MHz |
| 80 Meters | 3.5–4.0 MHz |
| 60 Meters | 5.332–5.405 MHz (source label: "5.332-5.405Mhz") |
| 40 Meters | 7.0–7.3 MHz |
| 30 Meters | 10.1–10.15 MHz |
| 20 Meters | 14.0–14.35 MHz |
| 17 Meters | 17.044–19.092 MHz |
| 15 Meters | 21–21.45 MHz |
| 12 Meters | 24.89–24.99 MHz |
| 10 Meters | 28–29.7 MHz |
| 6 Meters | 50–54 MHz |
| 4 Meters | 70–70.5 MHz |
| 2 Meters | 144–148 MHz |
| 1.25 Meters | 219–225 MHz |
| 70 cm | 420–450 MHz |
| 33 cm | 902–928 MHz |
| 23 cm | 1240–1300 MHz |
| 13 cm lower | 2300–2310 MHz |
| 13 cm upper | 2390–2450 MHz |

> **Note:** The 17 meters band range (17.044–19.092 MHz) in the source is incorrect per international allocations. The ITU 17m band is 18.068–18.168 MHz. The source value spans ~2 MHz and bleeds into adjacent bands.

## Persistence

### Bookmark File Format

File: `bookmarks.xml` in the config directory.

```xml
<cubicsdr_bookmarks>
  <header>
    <version>0.2.8</version>
  </header>
  <branches>
    <active>1</active>
    <range>0</range>
    <bookmark>1</bookmark>
    <recent>1</recent>
  </branches>
  <ranges>
    <range>
      <label>2 Meters (144-148 MHz)</label>
      <freq>146000000</freq>
      <start>144000000</start>
      <end>148000000</end>
    </range>
    <!-- more ranges -->
  </ranges>
  <modems>
    <group name="My Bookmarks" expanded="true">
      <modem>
        <type>NBFM</type>
        <user_label>Local Repeater</user_label>
        <frequency>146940000</frequency>
        <bandwidth>12500</bandwidth>
        <!-- full demodulator state -->
      </modem>
    </group>
  </modems>
  <recent_modems>
    <modem>
      <!-- recent demodulator entries -->
    </modem>
  </recent_modems>
</cubicsdr_bookmarks>
```

### Save Flow (`BookmarkMgr::saveToFile()`)

1. Create `DataTree` with root `cubicsdr_bookmarks`
2. Write header with version
3. Write branch expand states (active, range, bookmark, recent) — read from `BookmarkView::getExpandState()`, creating a model-to-view dependency in `saveToFile()`
4. Write ranges
5. Write bookmark groups: for each entry, check if a matching live demodulator exists (same type, label, frequency, and bandwidth) via `getLastDemodulatorWith()` and save its current state instead of the bookmark's stored state. If multiple live demodulators share the same parameters, the last match is used.
6. Write current demodulators + recent entries to `recent_modems`
7. Create backup of existing file before overwriting (only if `backup` is true, the save file exists, and the backup file does not exist or is writable)

> **Note:** The entire save operation is guarded by `saveFile.IsDirWritable()`. If the config directory is not writable, the save is silently skipped — no error is raised and no backup is created.

### Load Flow (`BookmarkMgr::loadFromFile()`)

0. Returns `false` early if the bookmark file exists but is not readable
1. If `backup` is true and no bookmark file, `.lastloaded`, or `.backup` files exist, load default ranges and return
2. Load `DataTree` from file, validate root node name
3. Parse branch expand states
4. Parse ranges into `ranges` list
5. Parse modem groups into `bmData` map
6. Parse recent modems into `recents` list
7. On success: copy file to `.lastloaded` (if `backup` is true)
8. On per-entry parse failure: copy file to `.failedload` (if `backup` is true). Hard failures (unreadable file, XML parse failure, or wrong root node name) return early and do not create `.failedload`.

### Backup Strategy

| File | Purpose |
|------|---------|
| `bookmarks.xml` | Active bookmark file |
| `bookmarks.xml.backup` | Previous version (before last save) |
| `bookmarks.xml.lastloaded` | Last successfully loaded version |
| `bookmarks.xml.failedload` | Last failed load attempt (for debugging) |

### Error Recovery

On startup, `CubicSDR.cpp` implements a cascading recovery chain when bookmark loading fails. The initial check selects which dialog to show based on which backup files exist:

1. **Initial load fails, backup exists** → `ActionDialogBookmarkLoadFailed` offers to load the `.backup` file
2. **Initial load fails, no backup** → checks for last-loaded file: `ActionDialogBookmarkBackupLoadFailed` offers to load the `.lastloaded` file, or `ActionDialogBookmarkCatastophe` if neither exists
3. **`ActionDialogBookmarkLoadFailed` OK click, backup load succeeds** → recovery complete
4. **`ActionDialogBookmarkLoadFailed` OK click, backup load fails** → cascades to `ActionDialogBookmarkBackupLoadFailed` (if `.lastloaded` exists) or `ActionDialogBookmarkCatastophe`
5. **`ActionDialogBookmarkBackupLoadFailed` OK click, last-loaded load succeeds** → recovery complete
6. **`ActionDialogBookmarkBackupLoadFailed` OK click, last-loaded load fails** → `ActionDialogBookmarkCatastophe`

Recovery dialogs call `loadFromFile` with `backup=false`, so no `.lastloaded` or `.failedload` files are created during recovery attempts.

None of the three bookmark dialog subclasses override `doClickCancel()`. The base `ActionDialog` class defines `doClickCancel()` as a no-op, so clicking Cancel simply closes the dialog. Clicking Cancel in `ActionDialogBookmarkLoadFailed` or `ActionDialogBookmarkBackupLoadFailed` causes the app to continue without loading the offered backup; bookmarks may already be partially populated if the failed load aborted mid-parse. Clicking Cancel in `ActionDialogBookmarkCatastophe` causes the app to continue without exiting. `ActionDialogBookmarkCatastophe`'s OK action calls `disableSave(true)`, which prevents **all** saves on close — not just bookmarks, but also `AppConfig`.

> **Note:** `ActionDialogBookmarkBackupLoadFailed` loads the `.lastloaded` file. `ActionDialogBookmarkCatastophe` offers to exit without saving to preserve files for manual recovery.

## UI Integration

### BookmarkView

The bookmark panel (`src/forms/Bookmark/BookmarkView.h`) displays bookmarks in a tree structure:

- **Active** — currently running demodulators
- **View Ranges** — frequency band definitions
- **Bookmarks** — user-defined groups with saved entries
- **Recents** — recently used demodulators

### Expand State

Expand state is tracked by two separate systems:

1. **Top-level branch states** (active, range, bookmark, recent): Stored in `BookmarkView::expandState`. Defaults: active=true, range=false, bookmark=true, recent=true. Persisted in the `<branches>` node of the bookmark file as integers.

2. **Bookmark group states**: Stored in `BookmarkMgr::expandState` (type `BookmarkExpandState`). Controls expand/collapse of individual bookmark groups within the "Bookmarks" branch. Defaults to `true` when a group name is not found. Persisted as `expanded` attributes on `<group>` nodes (e.g., `<group name="My Bookmarks" expanded="true">`).

When a tree item is collapsed/expanded, the appropriate handler writes to the matching system: top-level branch events write to `BookmarkView::expandState`, while group events call `BookmarkMgr::setExpandState()`.

Both systems expose public accessors: `BookmarkView::getExpandState()`/`setExpandState()` for branch states, and `BookmarkMgr::getExpandState()`/`setExpandState()` for group states. Note that `BookmarkView::getExpandState()` uses `std::map::operator[]` which default-inserts `false` for missing keys, while `BookmarkMgr::getExpandState()` returns `true` for missing keys. In practice this difference is harmless because the branch states are seeded directly in the constructor and refreshed from the `<branches>` node during load. The `saveToFile()` method reads branch states via `BookmarkView::getExpandState()`, creating a model-to-view dependency.

During search, expand states are overridden: ranges are forced collapsed, while recents and bookmark groups are forced expanded. Expand/collapse events are suppressed during search to prevent user actions from conflicting with the forced states.

`loadFromFile()` does not clear `BookmarkMgr::expandState` before repopulating it. Old group expand states persist across reloads for groups that no longer exist in the loaded file. `bmData`, `recents`, `ranges`, and `bmDataSorted` are all cleared at the start of `loadFromFile()`, but `expandState` is not. This has no visible effect because `getExpandState()` returns `true` for unknown keys.

### Demodulator Interaction

- **Add bookmark:** Select an active demodulator → use the bookmark choice dropdown in the properties panel → select group (or "New Group..")
- **Load bookmark:** Double-click bookmark entry → activates a matching live demodulator if one exists (same type, label, frequency, and bandwidth), otherwise creates and runs a new one
- **Delete item:** Select item → press DELETE key, or click the Remove button in the properties panel. Works for active, recent, bookmark, range, and group items.
- **Move between groups:** Drag and drop in the bookmark view. Only ACTIVE, RECENT, and BOOKMARK items can be dragged; RANGE and GROUP items are not draggable. Dropping an ACTIVE item on a group creates a new bookmark. Dropping a RECENT item on a group bookmarks it and removes it from recents. Dropping a BOOKMARK item on a group moves it between groups. Dropping any draggable item on the "Bookmarks" branch root creates an implicit "Unnamed" group.

### Additional UI Features

- **Search/Filter:** Keyword search filters bookmark tree in real-time
- **Range management:** Add, remove, and update frequency band ranges; ranges are renamed by editing the label field in the properties panel
- **Recording controls:** Start/stop audio recording from active demodulators
- **Status bar hint:** A static hint ("Drag & Drop to create / move bookmarks, Group and arrange bookmarks, quick Search by keywords.") is shown in the status bar when the mouse enters the bookmark panel
- **Frequency/bandwidth editing:** Double-click the frequency or bandwidth fields in the properties panel to open a FrequencyDialog (active demodulators only)
