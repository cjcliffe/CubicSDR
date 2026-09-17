# Plan: Modernize CMake

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

## Current State

- `cmake_minimum_required(VERSION 2.8.12)` — from 2013
- Uses `-std=c++0x` (draft C++11) instead of proper standard setting
- Global `include_directories()`, `ADD_DEFINITIONS()`, `link_libraries()` instead of target-specific
- Deprecated `CMAKE_CREATE_WIN32_EXE`, `LINK_FLAGS`, `SOURCE_GROUP REGULAR_EXPRESSION`
- Stray comma in `CMAKE_OSX_DEPLOYMENT_TARGET` assignment

## Implementation Plan

### Phase 1: Minimum viable modernization

1. Bump `cmake_minimum_required(VERSION 3.14...3.28)` — enables modern policies while allowing newer CMake.
2. Replace `ADD_DEFINITIONS( -std=c++0x -pthread )` with:
   ```cmake
   target_compile_features(CubicSDR PRIVATE cxx_std_14)
   find_package(Threads REQUIRED)
   target_link_libraries(CubicSDR PRIVATE Threads::Threads)
   ```
3. Fix the stray comma on the `CMAKE_OSX_DEPLOYMENT_TARGET` line: `SET(CMAKE_OSX_DEPLOYMENT_TARGET, "10.9")` → `set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")`.
4. Replace `set(CMAKE_CREATE_WIN32_EXE ...)` with:
   ```cmake
   set_target_properties(CubicSDR PROPERTIES WIN32_EXECUTABLE TRUE)
   target_link_options(CubicSDR PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:"mainCRTStartup")
   ```

### Phase 2: Target-specific commands

5. Replace all `include_directories(...)` with `target_include_directories(CubicSDR PRIVATE ...)`.
6. Replace all `ADD_DEFINITIONS(...)` with `target_compile_definitions(CubicSDR PRIVATE ...)`.
7. Replace `link_libraries(${HAMLIB_LIBRARY})` with `target_link_libraries(CubicSDR PRIVATE ${HAMLIB_LIBRARY})`.
8. Replace `set_target_properties(... LINK_FLAGS ...)` with `target_link_options()`.

### Phase 3: Advanced modernization (optional, future)

9. Replace `include(${wxWidgets_USE_FILE})` with imported targets (requires wxWidgets 3.2+).
10. Replace `SOURCE_GROUP(... REGULAR_EXPRESSION ...)` with `source_group(TREE ...)`.
11. Consider bumping minimum to CMake 3.16+ for broader modern feature access.

## Verification Criteria

- `cmake_minimum_required` version is 3.14 or higher.
- `cmake -B build` configures without errors on MSVC, GCC, and Clang.
- Build produces a working executable with no regressions.
- No global `include_directories()`, `ADD_DEFINITIONS()`, or `link_libraries()` remain (all are target-specific).
- The stray comma on the `CMAKE_OSX_DEPLOYMENT_TARGET` line is fixed.

## Rollback Strategy

This is a high-touch change to a single file. If the build breaks:
1. `git revert` the commit to restore the original `CMakeLists.txt`.
2. The original CMake 2.8 patterns are functional on all platforms — the revert is safe.
3. If only Phase 2 causes issues (e.g., a missed `include_directories`), consider doing Phase 1 alone first and verifying before proceeding.

## Files to Modify

| File | Action |
|------|--------|
| `CMakeLists.txt` | All phases |
