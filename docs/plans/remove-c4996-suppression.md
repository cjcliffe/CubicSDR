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

## Verification Criteria

- `/wd4996` is removed from `CMakeLists.txt`.
- MSVC build produces zero C4996 warnings in project source code.
- Third-party/vendored code uses local `#pragma warning(disable: 4996)` if needed.
- No buffer overflow or unsafe function regressions introduced.

## Rollback Strategy

If the C4996 fixes introduce behavioral changes (e.g., `snprintf` vs `sprintf` edge cases):
1. `git revert` the commit.
2. Re-add `/wd4996` to `CMakeLists.txt` to restore the suppressed build.
3. Consider fixing warnings incrementally (one file at a time) rather than all at once.

## Files to Modify

| File | Action |
|------|--------|
| `CMakeLists.txt` | Remove `/wd4996` |
| Various source files | Replace unsafe CRT functions (exact count TBD after unsuppressing) |
