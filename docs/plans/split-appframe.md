# Plan: Split AppFrame.cpp

CubicSDR is a cross-platform Software-Defined Radio application (C++14, wxWidgets, OpenGL). This plan covers splitting the monolithic `AppFrame.cpp` (3,202 lines) into multiple compilation units.

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

**Last Updated:** 2026-07-23

## Current State

- `AppFrame.cpp`: 3,202 lines in a single file
- `AppFrame.h`: 389 lines
- Handles menus, keyboard, device management, UI layout, hamlib, sessions, idle handlers, and accessors

## Implementation Plan

Split into 5 compilation units (same class, multiple `.cpp` files — no header changes needed):

| New File | Content | ~Lines |
|----------|---------|--------|
| `AppFrame.cpp` (kept) | Constructor, destructor, init*, make* factory methods, OnClose, OnNewWindow, splitter events, accessors, utilities | ~1,030 |
| `AppFrame_Menus.cpp` | `OnMenu`, all 19 `actionOnMenu*` methods, `makeFileMenu`, `makeDisplayMenu`, `makeAudioSampleRateMenu`, `makeRecordingMenu`, `updateRecordingMenu`, `getSettingsLabel` | ~900 |
| `AppFrame_Handlers.cpp` | `OnIdle`, all 12 `handle*` methods, `handleUpdateDeviceParams` | ~710 |
| `AppFrame_Keyboard.cpp` | `OnGlobalKeyDown`, `OnGlobalKeyUp`, `gkNudge`, `toggleActiveDemodRecording`, `toggleAllActiveDemodRecording` | ~334 |
| `AppFrame_Hamlib.cpp` | All `#ifdef USE_HAMLIB` methods: `makeRigMenu`, `enableRig`, `disableRig`, `setRigControlPort`, `dismissRigControlPortDialog`, `actionOnMenuRig`, `handleRigMenu` | ~301 |

### Steps

1. Create the 4 new `.cpp` files, each including `AppFrame.h`.
2. Move the method implementations (not declarations) to the new files.
3. Update `CMakeLists.txt` to add the new source files to `cubicsdr_sources`.
4. Build and verify no regressions.
5. This is a low-risk change — the header stays the same, only the implementation is split across compilation units.

## Files to Create/Modify

| File | Action |
|------|--------|
| `src/AppFrame_Menus.cpp` | Create |
| `src/AppFrame_Handlers.cpp` | Create |
| `src/AppFrame_Keyboard.cpp` | Create |
| `src/AppFrame_Hamlib.cpp` | Create |
| `src/AppFrame.cpp` | Remove moved methods |
| `CMakeLists.txt` | Add new source files |
