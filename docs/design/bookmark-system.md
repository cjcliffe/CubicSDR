# Bookmark System

This document describes CubicSDR's bookmark management system, data model, persistence, and the bookmark view UI.

## Overview

The bookmark system provides persistent storage for frequently visited frequencies and demodulator configurations. It includes four categories: active demodulators, bookmarked entries, recent entries, and frequency band ranges.

```
BookmarkMgr
    |
    +-- bmData (map<group_name, BookmarkList>)  -- user-defined bookmark groups
    +-- recents (BookmarkList)                   -- recently used demodulators
    +-- ranges (BookmarkRangeList)               -- frequency band ranges
    +-- expandState (map<name, bool>)            -- UI tree expand/collapse state
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

The `node` field stores the complete demodulator configuration including modem settings, gain, squelch, output device, and other parameters. This allows exact restoration when a bookmark is loaded.

### BookmarkRangeEntry (`src/BookmarkMgr.h`)

Represents a named frequency band:

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

## BookmarkMgr (`src/BookmarkMgr.h`)

Singleton managed by `CubicSDR`. Thread-safe via `recursive_mutex`.

### Bookmark Operations

| Method | Description |
|--------|-------------|
| `addBookmark(group, demod)` | Create bookmark from live demodulator |
| `addBookmark(group, entry)` | Add pre-built bookmark entry |
| `removeBookmark(group, entry)` | Remove from specific group |
| `removeBookmark(entry)` | Remove from all groups |
| `moveBookmark(entry, group)` | Move entry between groups |
| `getBookmarks(group)` | Get independent copy of group's bookmarks |

### Group Operations

| Method | Description |
|--------|-------------|
| `addGroup(name)` | Create empty group |
| `removeGroup(name)` | Delete group and all its bookmarks |
| `renameGroup(old, new)` | Rename group |
| `getGroups(arr)` | Get list of group names |

### Recent Entries

| Method | Description |
|--------|-------------|
| `addRecent(demod)` | Add demodulator to recents |
| `addRecent(entry)` | Add bookmark to recents |
| `removeRecent(entry)` | Remove from recents |
| `getRecents()` | Get independent copy of recents list |
| `clearRecents()` | Empty recents list |

Recents are capped at `BOOKMARK_RECENTS_MAX` (25 entries). Older entries are trimmed automatically.

### Range Operations

| Method | Description |
|--------|-------------|
| `addRange(entry)` | Add frequency band range |
| `removeRange(entry)` | Remove range |
| `getRanges()` | Get independent copy of ranges list |
| `clearRanges()` | Empty ranges list |

### Default Ranges

On first run (no bookmark file exists), `loadDefaultRanges()` populates standard amateur radio bands:

| Band | Frequency Range |
|------|----------------|
| 2200 Meters | 135.7–137.8 kHz |
| 630 Meters | 472–479 kHz |
| 160 Meters | 1.8–2 MHz |
| 80 Meters | 3.5–4.0 MHz |
| 60 Meters | 5.332–5.405 MHz |
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

> **Note:** The 17 meters band range (17.044–19.092 MHz) in the source is incorrect per international allocations. The ITU 17m band is 18.068–18.168 MHz. The source value spans ~2 MHz and bleeds into adjacent bands. This is a bug in `BookmarkMgr.cpp`, not a documentation error.

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
    <group>
      <@name>My Bookmarks</@name>
      <@expanded>true</@expanded>
      <modem>
        <type>NBFM</type>
        <label>Local Repeater</label>
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
3. Write branch expand states (active, range, bookmark, recent)
4. Write ranges
5. Write bookmark groups: for each entry, check if a matching live demodulator exists and save its current state instead of the stale bookmark data
6. Write current demodulators + recent entries to `recent_modems`
7. Create backup of existing file before overwriting

### Load Flow (`BookmarkMgr::loadFromFile()`)

1. If no bookmark file exists, load default ranges and return
2. Load `DataTree` from file, validate root node name
3. Parse branch expand states
4. Parse ranges into `ranges` list
5. Parse modem groups into `bmData` map
6. Parse recent modems into `recents` list
7. On success: copy file to `.lastloaded`
8. On failure: copy file to `.failedload`

### Backup Strategy

| File | Purpose |
|------|---------|
| `bookmarks.xml` | Active bookmark file |
| `bookmarks.xml.backup` | Previous version (before last save) |
| `bookmarks.xml.lastloaded` | Last successfully loaded version |
| `bookmarks.xml.failedload` | Last failed load attempt (for debugging) |

## UI Integration

### BookmarkView

The bookmark panel (`src/forms/Bookmark/BookmarkView.h`) displays bookmarks in a tree structure:

- **Active** — currently running demodulators
- **Ranges** — frequency band definitions
- **Bookmarks** — user-defined groups with saved entries
- **Recent** — recently used demodulators

### Expand State

`BookmarkMgr::setExpandState(name, bool)` and `getExpandState(name)` track which tree branches are expanded. Persisted in the `branches` node of the bookmark file.

### Demodulator Interaction

- **Add bookmark:** Right-click demodulator → "Bookmark" → select group
- **Load bookmark:** Double-click bookmark entry → creates and runs new demodulator
- **Delete bookmark:** Right-click → "Remove"
- **Move between groups:** Drag and drop in the bookmark view
