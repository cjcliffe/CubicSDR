# Plan: Use Git Submodules for Vendored Dependencies

CubicSDR is a cross-platform Software-Defined Radio application (C++14, wxWidgets, OpenGL). This plan covers updating and potentially converting vendored third-party libraries to git submodules for better version tracking and upstream updates.

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

**Last Updated:** 2026-07-23

## Current State

The `external/` directory contains full copies of 8 third-party libraries with no version tracking:

| Dependency | Vendored Version | Latest Upstream | Status |
|-----------|-----------------|-----------------|--------|
| lodepng | 20180819 | 20240326+ | Outdated ~6 years |
| TinyXML | 2.6.2 | N/A (abandoned) | Replace with TinyXML-2 |
| RtAudio | 5.2.0 | 6.0.1+ | 2 major versions behind |
| CubicVR2 Math | ~2013 | N/A | Unmaintained |
| RS-232 | 0.21 | 0.21 | Unmaintained since 2015 |
| wglext | ~2013 | Registry-generated | Self-declared obsolete |
| liquid-dsp | 1.5.0 | 1.7.0+ | 2 versions behind |
| hamlib | 4.x (early) | 4.6+ | Outdated |

## Implementation Plan

This is a large, risky change. Recommend doing it incrementally:

### Phase 1: Update actively-maintained dependencies

1. **lodepng**: Update to latest (20240326+). This is a drop-in update — same API, just newer version.
2. **liquid-dsp**: Update to 1.7.0+. Requires testing DSP behavior changes.
3. **RtAudio**: Evaluate upgrading to 6.0.1+ (major API changes) or staying on 5.x latest.
4. **hamlib**: Update pre-built DLLs to latest 4.6+ release.

### Phase 2: Replace abandoned dependencies

5. **TinyXML → TinyXML-2**: Different API. Requires updating all XML serialization code in `DataTree.cpp` and `AppConfig.cpp`. This is a significant refactor.
6. **wglext**: Replace with headers generated from the Khronos XML registry, or remove if not actually used.

### Phase 3: Convert to git submodules (optional)

7. For each dependency that has an active GitHub repo, convert to a git submodule:
   ```bash
   git submodule add https://github.com/lvandeve/lodepng.git external/lodepng
   ```
8. Document the pinned commit/tag for each submodule.
9. Update CI to initialize submodules: `git submodule update --init --recursive`.

### Phase 4: Evaluate removing unmaintained deps

10. **CubicVR2 Math**: If only used for basic vec2/vec3/mat4 operations, consider replacing with `glm` (widely used, actively maintained, header-only).
11. **RS-232**: Evaluate `asio` or platform-native serial APIs as alternatives.

## Files to Modify

| File | Action |
|------|--------|
| `external/` directory | Update/replace libraries |
| `src/util/DataTree.cpp` | Update if migrating TinyXML → TinyXML-2 |
| `src/AppConfig.cpp` | Update if migrating TinyXML → TinyXML-2 |
| `CMakeLists.txt` | Update include paths, link targets |
| `.gitmodules` | Create if using submodules |

## Rollback Strategy

Each dependency update should be done in a separate commit to enable granular rollback:

1. **Commit per dependency** — Update one library at a time (e.g., "Update lodepng to 20240326"). If a build breaks or DSP behavior changes, `git revert` that single commit without affecting other updates.
2. **Pin before updating** — Record the current vendored version in the commit message before replacing. Example: "Update liquid-dsp from 1.5.0 to 1.7.0" makes the diff in `git log` self-documenting.
3. **Build and smoke-test after each update** — Compile on all three platforms (or at least the primary dev platform) and run a quick manual smoke test (open a device, verify demodulation) before moving to the next dependency.
4. **TinyXML-2 migration is the highest-risk step** — The API change affects `DataTree.cpp` and `AppConfig.cpp`. Consider doing this in a feature branch with CI builds on all platforms before merging. If the migration fails, revert the branch merge.
5. **Submodule conversion (Phase 3) should be a single atomic commit** — Converting multiple libraries to submodules in separate commits creates confusing history. Do it all at once after all updates are stable.
