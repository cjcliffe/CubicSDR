# CubicSDR Improvement Plans

Detailed implementation plans for each recommendation. See [RECOMMENDATIONS.md](RECOMMENDATIONS.md) for the full evaluation and summary.

## Architecture Documentation

Design documents covering the system architecture are in [docs/design/](design/):

### Core Architecture

| Document | Description |
|----------|-------------|
| [Architecture Overview](design/README.md) | Directory layout, key classes, quick reference |
| [Signal Flow](design/signal-flow.md) | Data path from SDR hardware to audio output |
| [Threading Model](design/threading.md) | Thread inventory, synchronization, lifecycle |
| [Modem System](design/modem-system.md) | Plugin architecture, factory pattern, available modems |

### Subsystem Deep Dives

| Document | Description |
|----------|-------------|
| [Audio Subsystem](design/audio-subsystem.md) | Controller/bound mixing, WAV recording, device management |
| [Visual Rendering](design/visual-rendering.md) | Canvas hierarchy, GLFont, ColorTheme, rendering flow |
| [Visual Data Pipeline](design/visual-data-pipeline.md) | VisualProcessor, distribution modes, FFT/scope processing |
| [Configuration System](design/configuration-system.md) | AppConfig/DeviceConfig, DataTree serialization, sessions |
| [Bookmark System](design/bookmark-system.md) | BookmarkMgr, groups/ranges/recents, persistence |
| [SDR Device Layer](design/sdr-device-layer.md) | SDREnumerator, SDRDeviceInfo, manual devices |

## Plans

| Plan | Risk | Effort | Dependencies |
|------|------|--------|-------------|
| [Replace reinterpret_cast Type Punning](plans/replace-reinterpret-cast.md) | Low | 1 day | None |
| [Split AppFrame.cpp](plans/split-appframe.md) | Low | 1 day | None |
| [Resolve Open TODOs](plans/resolve-todos.md) | Low | 1 day | split-appframe, replace-reinterpret-cast |
| [Replace Raw new/delete with Smart Pointers](plans/smart-pointers.md) | Medium | 1 day | None |
| [Remove MSVC C4996 Suppression](plans/remove-c4996-suppression.md) | Medium | 1-2 days | None |
| [Update Vendored Dependencies](plans/update-vendored-deps.md) | High | 3-5 days | None |
| [Add --run-tests to build.py](plans/add-run-tests.md) | Low | Half day | None |

## Source Fixes & Documentation

Discrete tasks not covered by the implementation plans above, tracked here as the single source for their next-step list.

### Source Code Fixes

| # | Action | Risk | Effort | Notes |
|---|--------|------|--------|-------|
| 1 | Fix 17m band range in `BookmarkMgr.cpp` | Low | 5 min | Change to ITU 18.068-18.168 MHz. Check if adjacent bands need adjustment. |
| 2 | Fix CMakeLists.txt source/header mismatches | None | 10 min | 5 entries in wrong category. Doesn't break build but misrepresents structure. |

### Documentation Improvements

| # | Action | Risk | Effort | Notes |
|---|--------|------|--------|-------|
| 3 | Add CONTRIBUTING.md | None | 1 hr | Build instructions, code style, PR process. Currently missing. |
| 4 | Add CHANGELOG.md | None | 1 hr | Version history. Currently missing. |
| 5 | Add Doxygen config | None | 2 hr | API docs generation. Currently missing. |
| 6 | Improve README.md | None | 30 min | Add inline build instructions (currently only on external wiki). |

## Recommended Execution Order

Execute in this order to minimize risk and satisfy dependencies:

1. **Replace reinterpret_cast** — Low risk, standalone, improves correctness
2. **Split AppFrame.cpp** — Low risk, standalone, reduces cognitive load for later work
3. **Resolve TODOs** — Depends on AppFrame split and reinterpret_cast replacement
4. **Replace Raw new/delete** — Medium risk; do after tests exist to catch regressions
5. **Remove C4996 Suppression** — Medium risk; requires touching many files
6. **Update Vendored Dependencies** — Highest risk; do last, requires extensive testing
