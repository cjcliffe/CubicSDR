# Plan: Modernize CMake

CubicSDR is a cross-platform Software-Defined Radio application (C++14, wxWidgets, OpenGL). This plan covers modernizing the CMake build system from CMake 2.8 patterns to modern CMake 3.14+.

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

**Last Updated:** 2026-07-23

## Current State

- `cmake_minimum_required(VERSION 2.8.12)` — from 2013
- Uses `-std=c++0x` (draft C++11) instead of proper standard setting
- Global `include_directories()`, `ADD_DEFINITIONS()`, `link_libraries()` instead of target-specific
- Deprecated `CMAKE_CREATE_WIN32_EXE`, `LINK_FLAGS`, `SOURCE_GROUP REGULAR_EXPRESSION`
- Stray comma in `CMAKE_OSX_DEPLOYMENT_TARGET` assignment (line 303)
- Uppercase CMake commands throughout

## Implementation Plan

### Phase 1: Minimum viable modernization

1. Bump `cmake_minimum_required(VERSION 3.14...3.28)` — enables modern policies while allowing newer CMake.
2. Replace `ADD_DEFINITIONS( -std=c++0x -pthread )` (line 166) with:
   ```cmake
   target_compile_features(CubicSDR PRIVATE cxx_std_14)
   find_package(Threads REQUIRED)
   target_link_libraries(CubicSDR PRIVATE Threads::Threads)
   ```
3. Fix the stray comma on line 303: `SET(CMAKE_OSX_DEPLOYMENT_TARGET, "10.9")` → `set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")`.
4. Replace `set(CMAKE_CREATE_WIN32_EXE ...)` (line 708) with:
   ```cmake
   set_target_properties(CubicSDR PROPERTIES WIN32_EXECUTABLE TRUE)
   target_link_options(CubicSDR PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:"mainCRTStartup")
   ```

### Phase 2: Target-specific commands

5. Replace all `include_directories(...)` with `target_include_directories(CubicSDR PRIVATE ...)`.
6. Replace all `ADD_DEFINITIONS(...)` with `target_compile_definitions(CubicSDR PRIVATE ...)`.
7. Replace `link_libraries(${HAMLIB_LIBRARY})` with `target_link_libraries(CubicSDR PRIVATE ${HAMLIB_LIBRARY})`.
8. Replace `set_target_properties(... LINK_FLAGS ...)` with `target_link_options()`.

### Phase 3: Style normalization

9. Convert all uppercase CMake commands to lowercase (`SET` → `set`, `IF` → `if`, etc.).
10. Remove empty `endif()` arguments: `endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )` → `endif()`.

### Phase 4: Advanced modernization (optional, future)

11. Replace `include(${wxWidgets_USE_FILE})` with imported targets (requires wxWidgets 3.2+).
12. Replace `SOURCE_GROUP(... REGULAR_EXPRESSION ...)` with `source_group(TREE ...)`.
13. Consider bumping minimum to CMake 3.16+ for broader modern feature access.

## Files to Modify

| File | Action |
|------|--------|
| `CMakeLists.txt` | All phases |
