# Plan: Resolve Open TODOs

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

## Current State

Multiple TODO/FIXME markers in project source code:

| File | Comment | Action |
|------|---------|--------|
| `CubicSDRDefs.h` | `TODO: Make the waterfall resolutions an option.` | Convert to GitHub issue |
| `AppFrame.cpp` | `TODO: refactor these..` | Address during AppFrame split |
| `AppFrame.cpp` | `TODO: Move the stuff from there to here` | Address during AppFrame split |
| `AppFrame.cpp` | `TODO: Catch key-ups outside of original target` | Convert to GitHub issue |
| `DataTree.h` | `TODO: smarter way with templates?` (x2) | Convert to GitHub issue |
| `DataTree.cpp` | `TODO: forced cast in (char*) beware...` | Address during reinterpret_cast fix |
| `DataTree.cpp` | `TODO: stack recursion optimization` (x2) | Convert to GitHub issue |
| `DemodulatorThread.cpp` | `TODO: handle digital modems with audio output` | Convert to GitHub issue |
| `DemodulatorMgr.cpp` | `TODO: This is probably unnecessary and confusing` | Investigate and either fix or remove |
| `SoapySDRThread.cpp` | Various TODOs about timing and bandwidth (x3) | Convert to GitHub issues |
| `GainCanvas.cpp` | `TODO: if it not desirable, do not update in AGC mode` | Convert to GitHub issue |
| `ScopeCanvas.cpp` | `TODO: find out why frontbuffer drawing has stopped working in wx 3.1.0?` | Investigate; may be fixed in newer wxWidgets |
| `PrimaryGLContext.cpp` | `TODO: Better recording indicator...` | Convert to GitHub issue |
| `BookmarkView.cpp` | `TODO: keys for other actions?` | Convert to GitHub issue |

## Implementation Plan

1. For TODOs that will be addressed by other plans, resolve them during those tasks:
   - `AppFrame.cpp` refactor TODOs → resolve during [Split AppFrame.cpp](split-appframe.md)
   - `DataTree.cpp` forced cast TODO → resolve during [Replace reinterpret_cast](replace-reinterpret-cast.md)

2. For the remaining TODOs, create GitHub issues for each with:
   - The original TODO text
   - File/line reference
   - Description of the feature request or bug

3. Remove the TODO comments from the code, replacing with issue references:
   ```cpp
   // See: https://github.com/cjcliffe/CubicSDR/issues/XXX
   ```

4. For `DemodulatorMgr.cpp` ("probably unnecessary and confusing"), investigate the code and either fix the issue or remove the dead code.

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
