# Plan: Use Git Submodules for Vendored Dependencies

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md) | [Modem System](../design/modem-system.md) | [Signal Flow](../design/signal-flow.md)

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

1. **lodepng** (`external/lodepng/`): Update to latest (20240326+). Drop-in update — same API.
   - Files: `lodepng.h`, `lodepng.cpp`
   - No application code changes needed

2. **liquid-dsp** (`external/liquid-dsp/`): Update to 1.7.0+.
   - Used extensively: `SDRPostThread` (channelizer), `DemodulatorPreThread` (resampler), `DemodulatorThread` (squelch), all modems (demodulation)
   - Requires testing DSP behavior changes — run full demodulation smoke test after update
   - Check liquid-dsp changelog for API breaks in `firpfbch`, `msresamp`, `nco`, `modemcf`

3. **RtAudio** (`external/rtaudio/`): Evaluate upgrading to 6.0.1+ (major API changes) or staying on 5.x latest.
   - Used by `AudioThread` (`src/audio/AudioThread.cpp`) — `RtAudio` class, `openStream()`, `startStream()`, `stopStream()`, `closeStream()`, callback-based API
   - RtAudio 6.x has breaking API changes (callback signature, device enumeration)
   - If upgrading: update `AudioThread::audioCallback()` and device enumeration in `AudioThread::enumerateDevices()`
   - If staying on 5.x: update to latest 5.x patch release (minimal risk)

4. **hamlib** (`external/hamlib/`): Update pre-built DLLs to latest 4.6+ release.
   - Used by `RigThread` (`src/rig/RigThread.h`) for CAT control
   - Pre-built DLLs in `external/hamlib/` — replace with new builds

### Phase 2: Replace abandoned dependencies

5. **TinyXML → TinyXML-2**: Different API. Requires updating `DataTree.cpp` XML serialization.

   **Current TinyXML usage** (all in `src/util/DataTree.cpp`):
   - `TiXmlDocument` — load/save XML files
   - `TiXmlNode` — traverse child nodes, switch on node types
   - `TiXmlElement` — create elements, read/write attributes
   - `TiXmlText` — create text content nodes
   - `TiXmlAttribute` — iterate element attributes
   - `TiXmlDeclaration` — XML declaration header

   **TinyXML-2 API mapping:**

   | TinyXML | TinyXML-2 |
   |---------|-----------|
   | `TiXmlDocument` | `tinyxml2::XMLDocument` |
   | `TiXmlNode` | `tinyxml2::XMLNode` |
   | `TiXmlElement` | `tinyxml2::XMLElement` |
   | `TiXmlText` | `tinyxml2::XMLText` (via `XMLElement::SetText()`) |
   | `TiXmlAttribute` | `tinyxml2::XMLAttribute` |
   | `TiXmlDeclaration` | `tinyxml2::XMLDeclaration` (or `XMLDocument` constructor) |
   | `element->SetAttribute(name, val)` | `element->SetAttribute(name, val)` (same) |
   | `element->Attribute(name)` | `element->Attribute(name)` (returns `const char*`) |
   | `node->ToText()->Value()` | `node->ToText()->Value()` (similar) |
   | `doc.LoadFile(filename)` | `doc.LoadFile(filename)` (returns `XMLError`) |
   | `doc.SaveFile(filename)` | `doc.SaveFile(filename)` (returns `XMLError`) |
   | `element->LinkEndChild(child)` | `element->InsertEndChild(child)` |

   **Files to modify:**
   - `src/util/DataTree.h` — change `#include "tinyxml.h"` to `#include <tinyxml2.h>`
   - `src/util/DataTree.cpp` — update all TinyXML calls to TinyXML-2 equivalents
   - `CMakeLists.txt` — change tinyxml include path to tinyxml2

   **Indirect consumers** (use DataTree API, no direct changes needed):
   - `src/AppConfig.cpp` — save/load application config
   - `src/BookmarkMgr.cpp` — save/load bookmarks
   - `src/demod/DemodulatorMgr.cpp` — serialize demodulator instances

6. **wglext** (`external/wglext/`): Replace with regenerated header or inline definitions.

   **Current usage** (narrowly scoped):
   - `src/util/GLExt.h` includes `wglext.h` (line 11-13)
   - `src/util/GLExt.cpp` defines three function pointers:
     - `wglGetExtensionsStringEXT` (declared but unused — `GLExtSupported()` uses `glGetString()` instead)
     - `wglSwapIntervalEXT` (loaded via `wglGetProcAddress`, used for VSync)
     - `wglGetSwapIntervalEXT` (loaded via `wglGetProcAddress`, used for VSync query)
   - Only used on Windows; MinGW uses system `<gl/wglext.h>`

   **Options:**
   - Replace with a minimal header defining only the 3 needed typedefs (~10 lines)
   - Or regenerate from Khronos XML registry if more WGL extensions are needed later

### Phase 3: Convert to git submodules (optional)

7. For each dependency that has an active GitHub repo, convert to a git submodule:
   ```bash
   git submodule add https://github.com/lvandeve/lodepng.git external/lodepng
   ```
8. Document the pinned commit/tag for each submodule.
9. Update CI to initialize submodules: `git submodule update --init --recursive`.

### Phase 4: Evaluate removing unmaintained deps

10. **CubicVR2 Math** (`external/cubicvr2/math/`): Replace with `glm` (widely used, actively maintained, header-only).

    **Current CubicVR2 usage** (types and functions used by application code):

    | Type | Methods Used | Used In |
    |------|-------------|---------|
    | `vec2` | Constructor, `+`, `-`, `*` operators, `.x`, `.y` | `GLPanel.h/.cpp`, `MouseTracker.h`, `GainCanvas.cpp`, `MeterPanel.h/.cpp` |
    | `vec3` | Constructor, `[]` operator | `GLFont.cpp` |
    | `vec4` | Constructor, `.x`, `.y`, `.z`, `[]` operator | `GLPanel.cpp`, `MeterPanel.cpp` |
    | `mat4` | `identity()`, `translate()`, `scale()`, `rotate()`, `perspective()`, `lookat()`, `inverse()`, `multiply()`, `vec4_multiply()`, `to_ptr()` | `GLPanel.cpp`, `GLFont.cpp`, `ScopeCanvas.cpp`, `SpectrumPanel.cpp` |

    **Files to modify:**
    - `src/ui/GLPanel.h` — change `CubicVR::mat4` → `glm::mat4`, `CubicVR::vec2` → `glm::vec2`
    - `src/ui/GLPanel.cpp` — update method calls (`mat4::identity()` → `glm::mat4(1.0f)`, etc.)
    - `src/util/MouseTracker.h` — change `CubicVR::vec2` → `glm::vec2`
    - `src/util/GLFont.cpp` — update transform calls
    - `src/visual/ScopeCanvas.cpp` — update `perspective()`/`lookat()` calls
    - `src/visual/SpectrumCanvas.cpp`, `WaterfallCanvas.cpp` — update `identity()` calls
    - `src/visual/GainCanvas.cpp` — update `vec2` usage
    - `src/panel/MeterPanel.h/.cpp` — update `vec2`/`vec4`/`mat4` usage
    - `src/panel/SpectrumPanel.cpp` — update transform calls
    - `CMakeLists.txt` — add glm include path, remove cubicvr2

    **glm API mapping:**

    | CubicVR2 | glm |
    |----------|-----|
    | `mat4::identity()` | `glm::mat4(1.0f)` |
    | `mat4::translate(x,y,z)` | `glm::translate(glm::mat4(1.0f), glm::vec3(x,y,z))` |
    | `mat4::scale(x,y,z)` | `glm::scale(glm::mat4(1.0f), glm::vec3(x,y,z))` |
    | `mat4::rotate(angle, x,y,z)` | `glm::rotate(glm::mat4(1.0f), angle, glm::vec3(x,y,z))` |
    | `mat4::perspective(fov, aspect, near, far)` | `glm::perspective(fov, aspect, near, far)` |
    | `mat4::lookat(eye, center, up)` | `glm::lookAt(eye, center, up)` |
    | `mat4::inverse(m)` | `glm::inverse(m)` |
    | `mat4::multiply(m, v)` | `m * v` |
    | `mat4::to_ptr()` | `glm::value_ptr(m)` (from `<glm/gtc/type_ptr.hpp>`) |

11. **RS-232** (`external/rs232/`): Evaluate alternatives.

    **Current usage:**
    - `src/forms Dialog/PortSelectorDialog.cpp` — uses `comEnumerate()`, `comGetNoPorts()`, `comGetPortName()`, `comGetInternalName()`, `comTerminate()` for serial port enumeration only
    - Actual serial I/O (`comOpen`/`comRead`/`comWrite`/`comClose`) may be unused or used via rig control

    **Options:**
    - Keep as-is (stable, works, minimal maintenance needed for enumeration-only usage)
    - Replace with platform-native APIs (`QueryDosDevice` on Windows, `/dev/tty*` glob on Linux/macOS)
    - Replace with `asio::serial_port` if Boost/Asio is already a dependency

## Files to Modify

| File | Action |
|------|--------|
| `external/` directory | Update/replace libraries |
| `src/util/DataTree.h` | Update tinyxml include |
| `src/util/DataTree.cpp` | Migrate TinyXML → TinyXML-2 API calls |
| `src/util/GLExt.h` | Replace wglext.h with minimal header |
| `src/util/GLExt.cpp` | No changes needed (function pointer types stay the same) |
| `src/ui/GLPanel.h` | Replace CubicVR types with glm |
| `src/ui/GLPanel.cpp` | Update matrix/vector operations |
| `src/util/MouseTracker.h` | Replace CubicVR::vec2 with glm::vec2 |
| `src/util/GLFont.cpp` | Update transform calls |
| `src/visual/*.cpp` | Update matrix operations |
| `src/panel/*.cpp` | Update vector/matrix usage |
| `CMakeLists.txt` | Update include paths, link targets |
| `.gitmodules` | Create if using submodules |

## Rollback Strategy

Each dependency update should be done in a separate commit to enable granular rollback:

1. **Commit per dependency** — Update one library at a time (e.g., "Update lodepng to 20240326"). If a build breaks or DSP behavior changes, `git revert` that single commit without affecting other updates.
2. **Pin before updating** — Record the current vendored version in the commit message before replacing. Example: "Update liquid-dsp from 1.5.0 to 1.7.0" makes the diff in `git log` self-documenting.
3. **Build and smoke-test after each update** — Compile on all three platforms (or at least the primary dev platform) and run a quick manual smoke test (open a device, verify demodulation) before moving to the next dependency.
4. **TinyXML-2 migration is the highest-risk step** — The API change affects `DataTree.cpp` and `AppConfig.cpp`. Consider doing this in a feature branch with CI builds on all platforms before merging. If the migration fails, revert the branch merge.
5. **Submodule conversion (Phase 3) should be a single atomic commit** — Converting multiple libraries to submodules in separate commits creates confusing history. Do it all at once after all updates are stable.

## Verification Criteria

- Each dependency update compiles cleanly on all three platforms.
- lodepng: PNG save/load round-trip produces identical output.
- liquid-dsp: Demodulation of a known signal produces expected output (manual smoke test).
- RtAudio: Audio output works on at least one device.
- TinyXML-2: Config and bookmark files load and save correctly; round-trip produces identical XML.
- CubicVR2→glm: All OpenGL rendering (spectrum, waterfall, scope, meters) displays correctly.
- No new compiler warnings from updated dependencies.
