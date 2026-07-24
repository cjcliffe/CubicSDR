# Plan: Resolve Open TODOs

CubicSDR is a cross-platform Software-Defined Radio application (C++14, wxWidgets, OpenGL). This plan covers resolving the 15 TODO/FIXME markers in the project source code.

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

**Last Updated:** 2026-07-23

## Current State

15 TODO/FIXME markers in project source code:

| File | Line | Comment | Action |
|------|------|---------|--------|
| `CubicSDRDefs.h` | 49 | `TODO: Make the waterfall resolutions an option.` | Convert to GitHub issue |
| `AppFrame.cpp` | 289 | `TODO: refactor these..` | Address during AppFrame split |
| `AppFrame.cpp` | 2868 | `TODO: Move the stuff from there to here` | Address during AppFrame split |
| `AppFrame.cpp` | 3084 | `TODO: Catch key-ups outside of original target` | Convert to GitHub issue |
| `DataTree.h` | 304, 363 | `TODO: smarter way with templates?` | Convert to GitHub issue |
| `DataTree.cpp` | 143 | `TODO: forced cast in (char*) beware...` | Address during reinterpret_cast fix |
| `DataTree.cpp` | 171, 225 | `TODO: stack recursion optimization` | Convert to GitHub issue |
| `DemodulatorThread.cpp` | 257 | `TODO: handle digital modems with audio output` | Convert to GitHub issue |
| `DemodulatorMgr.cpp` | 233 | `TODO: This is probably unnecessary and confusing` | Investigate and either fix or remove |
| `SoapySDRThread.cpp` | 203, 215, 486 | Various TODOs about timing and bandwidth | Convert to GitHub issues |
| `GainCanvas.cpp` | 291 | `TODO: if not desirable, do not update in AGC mode` | Convert to GitHub issue |
| `ScopeCanvas.cpp` | 132 | `TODO: find out why frontbuffer drawing stopped in wx 3.1.0?` | Investigate; may be fixed in newer wxWidgets |
| `PrimaryGLContext.cpp` | 120 | `TODO: Better recording indicator...` | Convert to GitHub issue |
| `BookmarkView.cpp` | 553 | `TODO: keys for other actions?` | Convert to GitHub issue |

## Implementation Plan

1. For TODOs that will be addressed by other plans, resolve them during those tasks:
   - `AppFrame.cpp:289,2868` → resolve during [Split AppFrame.cpp](split-appframe.md)
   - `DataTree.cpp:143` → resolve during [Replace reinterpret_cast](replace-reinterpret-cast.md)

2. For the remaining 10 TODOs, create GitHub issues for each with:
   - The original TODO text
   - File/line reference
   - Description of the feature request or bug

3. Remove the TODO comments from the code, replacing with issue references:
   ```cpp
   // See: https://github.com/cjcliffe/CubicSDR/issues/XXX
   ```

4. For `DemodulatorMgr.cpp:233` ("probably unnecessary and confusing"), investigate the code and either fix the issue or remove the dead code.

## Verification Criteria

- Zero TODO/FIXME comments remain in `src/` (verified by `grep -r "TODO\|FIXME" src/`).
- Each removed TODO either has a corresponding GitHub issue or was resolved by another plan.
- Build succeeds and no functional regressions introduced.

## Files to Modify

| File | Action |
|------|--------|
| `src/CubicSDRDefs.h` | Remove TODO, add issue reference |
| `src/AppFrame.cpp` | Remove resolved TODOs during split |
| `src/DataTree.h` | Remove TODOs, add issue references |
| `src/DataTree.cpp` | Remove TODOs, add issue references |
| `src/DemodulatorThread.cpp` | Remove TODO, add issue reference |
| `src/DemodulatorMgr.cpp` | Investigate and resolve |
| `src/SoapySDRThread.cpp` | Remove TODOs, add issue references |
| `src/GainCanvas.cpp` | Remove TODO, add issue reference |
| `src/ScopeCanvas.cpp` | Remove TODO, add issue reference |
| `src/PrimaryGLContext.cpp` | Remove TODO, add issue reference |
| `src/BookmarkView.cpp` | Remove TODO, add issue reference |

## Related Plans

- [Split AppFrame.cpp](split-appframe.md) — resolves 2 AppFrame TODOs
- [Replace reinterpret_cast](replace-reinterpret-cast.md) — resolves 1 DataTree TODO
