# Plan: Remove MSVC C4996 Suppression

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

## Current State

MSVC deprecation warning C4996 is disabled for the application target in the MSVC block of `CMakeLists.txt` (`target_compile_options(CubicSDR PRIVATE /wd4996)`). The suppression is per-target rather than global, but because vendored sources (tinyxml, RtAudio, lodepng, cubicvr2 math, plus rs232 when `USE_HAMLIB` is on) are compiled as part of that same target, it also quiets warnings coming from third-party code.

First-party call sites that C4996 flags, all under `src/`:

| File | Location | Call |
|------|----------|------|
| `AudioFile.cpp` | `AudioFile::getOutputFileName` | `fopen` in the unique-filename probe loop |
| `AudioFileWAV.cpp` | `AudioFileWAV::getOutputFileName` | Same `fopen` probe pattern |
| `AudioSinkFileThread.cpp` | `AudioSinkFileThread::sink`, `AudioSinkFileThread::setAudioFileHandler` | `std::localtime`, which is also thread-unsafe |
| `RigThread.cpp` | `RigThread::run` | `strncpy` into the hamlib rigport pathname; a truncated copy leaves the buffer unterminated. Only compiled when `USE_HAMLIB` is on (off by default) |
| `DataTree.cpp` | `DataTree::wsEncode`, `DataTree::wsDecode`, `DataElement::toString` | `wcstombs` / `mbstowcs`, with return values ignored |
| `DataTree.h` | `DataElement::set` (wstring), `DataElement::get` (wstring) | `wcstombs` / `mbstowcs` |

No `sprintf` / `strcpy` / `strcat` / `sscanf` calls remain in `src/`. In vendored code the picture is mixed: tinyxml already handles MSVC itself (`TiXmlFOpen` uses `fopen_s`, and `TIXML_SNPRINTF` / `TIXML_SSCANF` map to the `_s` variants under its `TIXML_SAFE` block, so its raw `sprintf` fallbacks in `#else` branches never compile on MSVC), lodepng carries its own disabling pragma, and the RtAudio flagged calls (`strcpy` / `sprintf`) sit in `__LINUX_ALSA__` regions that do not compile on Windows. Not in scope: the `strncpy` in the hamlib `rig_debug` macro is inside `#ifndef __cplusplus` / `#ifdef __GNUC__` and hamlib itself ships as a prebuilt library, and `external/doctest` is only compiled into the `cubicsdr_tests` target, never into `CubicSDR`. The one vendored source that does add warnings to a Windows build is `external/rs232/rs232-win.cpp` (`sprintf` in `comGetPortName` / `comGetInternalName`), compiled only when `USE_HAMLIB` is on — so the scoped suppression in step 1 must cover it too. (The `freopen` in `CubicSDR.cpp` needs no action: it sits in a disabled `#if (0)` block.)

## Implementation Plan

1. Confine the existing suppression to vendored sources first, using `set_source_files_properties` with `COMPILE_OPTIONS /wd4996` (or `COMPILE_FLAGS`) on the `external/` files compiled into the target (lodepng, tinyxml, cubicvr2 math, RtAudio, plus the rs232 source when `USE_HAMLIB` is on). Scoping lodepng is redundant but harmless given its own pragma. This keeps the build green while first-party code is fixed, and avoids editing third-party files that a future dependency update would overwrite.

2. Replace each first-party call with a portable equivalent:

   | Call | Replacement |
   |------|-------------|
   | `fopen` | `fopen_s` under `_MSC_VER`, plain `fopen` elsewhere (`fopen_s` returns `errno_t` and takes a `FILE**` out-param, so each probe loop needs restructuring, not a drop-in swap) |
   | `std::localtime` | `localtime_s` under `_MSC_VER`, `localtime_r` on POSIX (also fixes the thread-safety hazard; note the argument order is reversed: `localtime_s(&out, &t)` vs `localtime_r(&t, &out)`) |
   | `strncpy` into the rigport pathname | `snprintf` with the destination size, which always NUL-terminates on C99-conformant toolchains (VS2015+; otherwise prefer `snprintf_s` under `_MSC_VER`) |
   | `wcstombs` / `mbstowcs` | `wcstombs_s` / `mbstowcs_s` under `_MSC_VER`, unchanged elsewhere; the `_s` variants take a converted-count out-param plus an explicit destination size, so reproduce the current truncate-on-failure semantics deliberately rather than relying on the pre-zeroed buffers alone |

   Constraints on the replacements: `strncpy` and `strncat` are themselves C4996-flagged, so they are not valid fixes — prefer `snprintf` or `std::string` operations. The `_s`-suffixed functions are MSVC-only, so every use needs an `_MSC_VER` guard with a portable fallback; the project builds on Windows, macOS, and Linux.

3. Remove `/wd4996` from the CubicSDR target once `src/` is clean.

4. Rebuild on Windows through `python build.py` (never raw `cmake` or `msbuild`) and confirm the build log shows no C4996 entries for files under `src/`.

## Verification Criteria

- `/wd4996` is no longer applied to the CubicSDR target; any remaining suppression is scoped to `external/` sources only.
- The MSVC build log contains no C4996 entries for files under `src/`.
- The conditional `RigThread.cpp` / rs232 paths are also proven C4996-clean: the `windows` preset used by `python build.py` sets `USE_HAMLIB=ON`, so the standard preset build already covers them (a raw configure without the preset would skip them, since the CMake option defaults to `OFF`).
- No behavioral regressions: recording filename sequencing, timed file splits, and rig path setup still work (manual smoke test); DataTree wide-string round-trips are covered by `cubicsdr_tests` (`tests/test_DataTree.cpp`, configured with `BUILD_TESTING=ON`).
- The Windows build via `python build.py` succeeds.

## Rollback Strategy

- Re-add the single `target_compile_options` line to restore the suppressed build.
- Land incrementally (vendored scoping first, then one commit per file group) so any revert stays narrow.

## Files to Modify

| File | Action |
|------|--------|
| `CMakeLists.txt` | Scope suppression to `external/` sources, then drop the target-wide `/wd4996` |
| `src/audio/AudioFile.cpp` | Portable `fopen` handling in `getOutputFileName` |
| `src/audio/AudioFileWAV.cpp` | Portable `fopen` handling in `getOutputFileName` |
| `src/audio/AudioSinkFileThread.cpp` | Thread-safe `localtime` handling |
| `src/rig/RigThread.cpp` | NUL-safe pathname copy (only built with `USE_HAMLIB`) |
| `src/util/DataTree.cpp` | Portable `wcstombs` / `mbstowcs` handling |
| `src/util/DataTree.h` | Portable `wcstombs` / `mbstowcs` handling |

## Related Plans

- [Update Vendored Dependencies](update-vendored-deps.md) — the reason vendored code gets scoped suppression instead of hand edits
