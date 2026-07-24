# Plan: Remove MSVC C4996 Suppression

CubicSDR is a cross-platform Software-Defined Radio application (C++14, wxWidgets, OpenGL). This plan covers removing the global MSVC C4996 warning suppression and addressing the underlying unsafe CRT function usage.

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

**Last Updated:** 2026-07-23

## Current State

Line 712 of `CMakeLists.txt`:
```cmake
ADD_DEFINITIONS(/wd"4996")
```
This globally suppresses all "This function or variable may be unsafe" warnings from MSVC CRT. This hides potential buffer overflow risks from functions like `sprintf`, `strcpy`, `strcat`, etc.

## Implementation Plan

1. Remove the `/wd4996` suppression from `CMakeLists.txt`.
2. Build the project and catalog all C4996 warnings (file, line, function).
3. For each occurrence, replace with the safe variant:
   - `sprintf` → `snprintf`
   - `strcpy` → `strncpy` or `std::string` operations
   - `strcat` → `strncat` or `std::string` operations
   - `sscanf` → use bounds-checked alternatives
4. If any third-party/vendored code triggers C4996, suppress it locally with `#pragma warning(disable: 4996)` around just that code, not globally.
5. Re-enable the warning globally and verify a clean build.

## Files to Modify

| File | Action |
|------|--------|
| `CMakeLists.txt` | Remove `/wd4996` |
| Various source files | Replace unsafe CRT functions (exact count TBD after unsuppressing) |
