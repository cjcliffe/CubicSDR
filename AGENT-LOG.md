# Agent Session Log

## Session 1: Project Evaluation and Planning

**Date:** 2026-07-23
**Model:** opencode/big-pickle

### Actions

1. Explored the CubicSDR codebase structure, technology stack, and key directories
2. Analyzed code quality: TODO/FIXME markers, test coverage, memory safety, build system, documentation, git hygiene, vendored dependencies
3. Generated `RECOMMENDATIONS.md` with evaluation summary and 10 priority-ranked recommendations
4. Generated `PLAN.md` with detailed implementation plans for each recommendation
5. Created `docs/design/` directory with 4 architecture documents (README.md, signal-flow.md, threading.md, modem-system.md)
6. Split `PLAN.md` into 10 individual plan files under `docs/plans/`
7. Created `AGENTS.md` and `AGENT-LOG.md`

### Files Created

| File | Description |
|------|-------------|
| `docs/RECOMMENDATIONS.md` | Project evaluation and priority-ranked recommendations |
| `docs/PLAN.md` | Index of implementation plans with risk/effort estimates |
| `docs/design/README.md` | Architecture overview, source layout, key class reference |
| `docs/design/signal-flow.md` | Data path from SDR hardware to audio output |
| `docs/design/threading.md` | Thread inventory, synchronization, lifecycle |
| `docs/design/modem-system.md` | Factory registration, modem hierarchy, available modems |
| `docs/plans/add-unit-tests.md` | Test infrastructure and initial test coverage |
| `docs/plans/fix-gitignore.md` | Comprehensive .gitignore patterns |
| `docs/plans/modernize-cmake.md` | CMake 2.8 → 3.14+ modernization |
| `docs/plans/smart-pointers.md` | Replace raw new/delete with std::unique_ptr |
| `docs/plans/split-appframe.md` | Split monolithic AppFrame.cpp into multiple files |
| `docs/plans/remove-c4996-suppression.md` | Address unsafe CRT function usage |
| `docs/plans/replace-reinterpret-cast.md` | Fix undefined behavior in DataTree |
| `docs/plans/update-vendored-deps.md` | Update or replace third-party libraries |
| `docs/plans/add-ci-test-execution.md` | Run tests in CI pipeline |
| `docs/plans/resolve-todos.md` | Address open TODO/FIXME markers |
| `AGENTS.md` | Agent instructions and project documentation index |

## Session 2: Documentation Refinement (Sessions 2-4 Combined)

**Date:** 2026-07-23
**Model:** opencode/big-pickle

Multiple iterative passes over the documentation to fix errors, add missing content, and improve quality:

- Fixed broken cross-references in plan files (numeric prefix remnants)
- Added verification criteria and rollback strategies to all plan files
- Added cross-references between plan files and architecture docs
- Verified ~50+ factual claims against source code, fixed 13 errors (source counts, reinterpret_cast counts, TODO counts, threading claims, CI status, dependency versions)
- Created then deleted `CONTRIBUTING.md` and `docs/MAINTAINING.md` (not actionable)
- Expanded `update-vendored-deps.md` with detailed API migration tables (TinyXML→TinyXML-2, CubicVR2→glm)
- Expanded `add-unit-tests.md` with Phase 3 test modules
- Expanded `smart-pointers.md` with thread lifecycle analysis

## Session 3: Documentation Debrittling

**Date:** 2026-07-23
**Model:** opencode/big-pickle

### Actions

1. Evaluated all docs files for brittle/noise content that would drift from the codebase during iterative development
2. Removed "Last Updated" timestamps from all 17 documentation files
3. Removed redundant "CubicSDR is a cross-platform..." preambles from all 10 plan files
4. Removed "Key Strengths" section from `RECOMMENDATIONS.md` (subjective, doesn't drive action)
5. Removed TODO/FIXME inventory table from `RECOMMENDATIONS.md` (redundant with `resolve-todos.md`)
6. Removed exact counts from `RECOMMENDATIONS.md` ("~197 source files", "18 TODOs", "~177 of 197")
7. Removed brittle line numbers from `smart-pointers.md` and `modernize-cmake.md`
8. Removed Testability Assessment table from `add-unit-tests.md`
9. Removed theoretical "Design Considerations: Join vs. Detach" section from `smart-pointers.md`
10. Added Documentation Guidelines section to `AGENTS.md` prohibiting brittle content in future docs

### Files Modified

| File | Action |
|------|--------|
| `docs/RECOMMENDATIONS.md` | Removed strengths section, TODO table, exact counts, timestamp |
| `docs/PLAN.md` | Removed timestamp |
| `docs/plans/add-unit-tests.md` | Removed preamble, timestamp, Testability Assessment table |
| `docs/plans/fix-gitignore.md` | Removed preamble, timestamp |
| `docs/plans/modernize-cmake.md` | Removed preamble, timestamp, brittle line numbers |
| `docs/plans/smart-pointers.md` | Removed preamble, timestamp, brittle line numbers, theoretical section |
| `docs/plans/split-appframe.md` | Removed preamble, timestamp |
| `docs/plans/remove-c4996-suppression.md` | Removed preamble, timestamp |
| `docs/plans/replace-reinterpret-cast.md` | Removed preamble, timestamp |
| `docs/plans/update-vendored-deps.md` | Removed preamble, timestamp |
| `docs/plans/add-ci-test-execution.md` | Removed preamble, timestamp |
| `docs/plans/resolve-todos.md` | Removed preamble, timestamp |
| `docs/design/README.md` | Removed timestamp |
| `docs/design/signal-flow.md` | Removed timestamp |
| `docs/design/threading.md` | Removed timestamp |
| `docs/design/modem-system.md` | Removed timestamp |
| `AGENTS.md` | Added Documentation Guidelines section |

## Session 4: Log Compaction

**Date:** 2026-07-23
**Model:** opencode/big-pickle

### Problem

Sessions 2-4 were iterative fixes to each other's work: adding timestamps → removing them, adding sections → deleting them, fixing counts → re-fixing them. This created confusion about the final state and made the log harder to use as a reference.

### Solution

Compressed sessions 2-4 into a single summary paragraph. The final state is what matters; the back-and-forth process of arriving at it is noise.

### Lesson Learned

Logs should capture **outcomes**, not process. If something is added then later removed in the same session, just note the removal and why — don't keep the full history of adding it. A log entry that says "added timestamps to all files" followed later by "removed timestamps from all files" leaves the reader unsure which state is current.

## Session 5: Documentation Accuracy Review

**Date:** 2026-07-24
**Model:** opencode/mimo-v2.5-free

### Actions

1. Systematically verified all factual claims in docs/ against source code using parallel agent tasks
2. Verified design docs (signal-flow.md, threading.md, modem-system.md, README.md) against actual code
3. Verified all 10 plan files against actual codebase state
4. Verified RECOMMENDATIONS.md claims against source code
5. Fixed 10 factual errors across 5 documentation files

### Errors Fixed

| File | Error | Fix |
|------|-------|-----|
| `docs/design/signal-flow.md` | Data types table missing fields (numChannels, sampleRate, modemName, etc.) | Added missing fields to all 5 data type entries |
| `docs/design/signal-flow.md` | Visual pipeline: `pipeIQVisualData` → WaterfallCanvas | Corrected to → SpectrumCanvas |
| `docs/design/threading.md` | `threadMain()` sets only `terminated=true` | Corrected to both `terminated` and `stopping` |
| `docs/design/threading.md` | Shutdown sequence claims queue flush at end of OnExit() | Removed; flushing happens inside DemodulatorInstance::terminate() |
| `docs/design/threading.md` | wxWidgets section implies AppFrame::OnIdle() dispatches to canvases | Clarified each canvas has independent EVT_IDLE handler |
| `docs/design/modem-system.md` | `getFactories()` returns `std::vector<std::string>` | Corrected to `ModemFactoryList` (std::map) |
| `docs/plans/resolve-todos.md` | `src/SoapySDRThread.cpp` path | Corrected to `src/sdr/SoapySDRThread.cpp` |
| `docs/plans/update-vendored-deps.md` | `src/forms Dialog/PortSelectorDialog.cpp` (space) | Corrected to `src/forms/Dialog/PortSelectorDialog.cpp` |

### Files Modified

| File | Action |
|------|--------|
| `docs/design/signal-flow.md` | Fixed data types table (5 entries), fixed visual pipeline wiring |
| `docs/design/threading.md` | Fixed threadMain() behavior, shutdown sequence, wxWidgets integration |
| `docs/design/modem-system.md` | Fixed getFactories() return type and ModemFactoryList typedef |
| `docs/plans/resolve-todos.md` | Fixed SoapySDRThread.cpp file path |
| `docs/plans/update-vendored-deps.md` | Fixed PortSelectorDialog.cpp path typo |

## Session 6: Subsystem Deep Dive Documentation

**Date:** 2026-07-24
**Model:** opencode/mimo-v2.5-free

### Actions

1. Identified missing subsystem documentation by comparing existing design docs against full codebase structure
2. Read source files for audio, visual, configuration, bookmark, and SDR device subsystems
3. Created 5 new subsystem deep dive documents under `docs/design/`
4. Updated `docs/design/README.md` and `docs/PLAN.md` to link new documents

### Files Created

| File | Description |
|------|-------------|
| `docs/design/audio-subsystem.md` | AudioThread controller/bound pattern, WAV recording pipeline, device management, real-time mixing |
| `docs/design/visual-architecture.md` | Canvas hierarchy, GLPanel system, GLFont bitmap rendering, ColorTheme, visual data processing |
| `docs/design/configuration-system.md` | AppConfig/DeviceConfig persistence, DataTree serialization, session management, file locations |
| `docs/design/bookmark-system.md` | BookmarkMgr data model, groups/ranges/recents, XML persistence, default amateur radio bands |
| `docs/design/sdr-device-layer.md` | SDREnumerator discovery, SDRDeviceInfo capabilities, manual devices, SoapySDR module loading |

### Files Modified

| File | Action |
|------|--------|
| `docs/design/README.md` | Added "Subsystem Deep Dives" section linking new documents |
| `docs/PLAN.md` | Added "Subsystem Deep Dives" section linking new documents |

## Session 7: Documentation Verification and Priority Planning

**Date:** 2026-07-27
**Model:** opencode/mimo-v2.5-free

### Actions

1. Verified 59 factual claims across all 9 design docs against source code (97% accuracy, 2 errors found)
2. Verified CMakeLists.txt source/header mismatch table (5 entries, all accurate)
3. Fixed `MIN_BANDWIDTH` value in `visual-architecture.md` (was 30000, actual 500)
4. Fixed pthread stack size in `threading.md` (was 2,048,000, actual 2048000)
5. Added note about 17m band source bug in `bookmark-system.md` (range 17.044-19.092 MHz is incorrect per ITU; should be 18.068-18.168 MHz)
6. Created `docs/PRIORITY.md` with ordered list of recommended next steps

### Errors Found

| File | Claim | Actual |
|------|-------|--------|
| `docs/design/visual-architecture.md` | `MIN_BANDWIDTH` = 30000 | 500 (`Modem.h:13`) |
| `docs/design/threading.md` | pthread stack size 2,048,000 bytes | 2048000 bytes (`DemodulatorInstance.cpp:136`) |

### Source Bug Identified

The 17 meters band default range in `BookmarkMgr.cpp` (17.044-19.092 MHz) is incorrect per international allocations. The ITU 17m band is 18.068-18.168 MHz. Documented in `bookmark-system.md` with a note.

### Files Modified

| File | Action |
|------|--------|
| `docs/design/visual-architecture.md` | Fixed MIN_BANDWIDTH constant value |
| `docs/design/threading.md` | Fixed pthread stack size |
| `docs/design/bookmark-system.md` | Added note about 17m band source bug |
| `docs/PRIORITY.md` | Created with ordered next steps |

## Session 8: Visual Architecture Design Document Expansion

**Date:** 2026-07-27
**Model:** opencode/mimo-v2.5-free

### Actions

1. Evaluated all 22 design/plan documents for expansion potential; selected `visual-architecture.md` as highest value (most complex subsystem, most room for deeper documentation)
2. Explored full visual subsystem source code (GLPanel, VisualProcessor, MouseTracker, WaterfallCanvas, SpectrumCanvas, ScopeCanvas, GLFont, PrimaryGLContext)
3. Expanded GLPanel System section: coordinate systems, fill types, transform pipeline (calcTransform + draw), hit-testing, parent-child composition
4. Expanded Visual Data Processing Pipeline: VisualProcessor template internals, distribution modes, SpectrumVisualProcessor full per-frame pipeline (10-step sequence), FFTVisualDataThread glue thread, ScopeVisualProcessor modes, thread safety summary table
5. Expanded Mouse Interaction section: MouseTracker state, event propagation, WaterfallCanvas drag state machine (5 states with transition diagrams), frequency zoom state machine, keyboard navigation table, SpectrumCanvas interactions, ScopeCanvas spring animation
6. Expanded GLFont section: font loading/search order, Drawer proxy selection algorithm, string cache entry structure, eviction policy (GC_DRAW_COUNT_PERIOD/LIMIT), thread safety mechanisms
7. Expanded PrimaryGLContext section: GL state management, blend modes table, buffer strategy, ScopeContext extensions
8. Expanded Rendering Flow section: per-canvas OnPaint patterns (WaterfallCanvas and ScopeCanvas detailed flows), data flow timing diagram

### Files Modified

| File | Action |
|------|--------|
| `docs/design/visual-architecture.md` | Expanded 6 sections with source-level detail: GLPanel, VisualProcessor, Mouse Interaction, GLFont, PrimaryGLContext, Rendering Flow |

## Session 9: Visual Architecture Verification and Fix

**Date:** 2026-07-27
**Model:** opencode/mimo-v2.5-free

### Actions

1. Systematically verified all factual claims in `visual-architecture.md` against source code using 8 parallel agent tasks
2. Verified canvas class hierarchy, GLPanel system, GLFont system, SpectrumVisualProcessor pipeline, ColorTheme/ThemeMgr, PrimaryGLContext drawing, threading/data flow, and mouse interaction details
3. Found 2 factual errors and 4 minor omissions across the document (~95% accuracy)
4. Fixed all 4 issues

### Errors Fixed

| File | Claim | Actual | Fix |
|------|-------|--------|-----|
| `docs/design/visual-architecture.md` | Double EMA: `ma` updated first, then `maa` uses fresh `ma` | Code updates `maa` first (using old `ma`), then `ma` | Reversed formula order, added note explaining the distinction |
| `docs/design/visual-architecture.md` | `shiftDown` controls frequency-vs-bandwidth selection | `shiftDown` controls create-new vs modify-existing; frequency-vs-bandwidth is distance-based (`|freqDiff| > bandwidth/3`) | Corrected hover state flowchart to show shiftDown's actual role |
| `docs/design/visual-architecture.md` | Pipe queues owned by `SDRPostThread` | Pipes (`pipeIQVisualData`, etc.) are `CubicSDR` class members bound via `setOutputQueue()` | Rewrote data flow diagram to show CubicSDR as pipe owner |
| `docs/design/visual-architecture.md` | Canvas hierarchy lists 7 canvas classes | `UITestCanvas` (src/ui/) also inherits from InteractiveCanvas | Added UITestCanvas to hierarchy listing |

## Session 10: Visual Architecture Document Review and Fix

**Date:** 2026-07-27
**Model:** opencode/mimo-v2.5-free

### Actions

1. Reviewed `docs/design/visual-architecture.md` by verifying all claims against source code using a detailed agent task
2. Found 5 issues: 1 missing theme property, 1 incomplete data flow diagram, 1 imprecise enum description, 4 undocumented GLContext subclasses, 1 ambiguous API description
3. Fixed all 5 issues in `visual-architecture.md`
4. Documented MeterContext, TuningContext, ModeSelectorContext, and UITestContext subclasses with full method signatures, GL state, and drawing primitives

### Issues Fixed

| File | Issue | Fix |
|------|-------|-----|
| `docs/design/visual-architecture.md` | `wfHighlight` theme property missing from table | Added to theme property table |
| `docs/design/visual-architecture.md` | Data flow diagram omits `SpectrumVisualDataThread` intermediary | Added `SpectrumVisualDataThread` to diagram with explanation |
| `docs/design/visual-architecture.md` | Scope spectrum mode listed alongside enum values | Clarified it's a boolean flag (`renderData->spectrum`), not a fourth `ScopeMode` value |
| `docs/design/visual-architecture.md` | MeterContext, TuningContext, ModeSelectorContext, UITestContext undocumented | Added full documentation for all 4 subclasses |
| `docs/design/visual-architecture.md` | `isOutputEmpty()` semantics ambiguous | Clarified: returns true when all outputs have room (backpressure), not when queue is empty |

### Files Modified

| File | Action |
|------|--------|
| `docs/design/visual-architecture.md` | Fixed 5 issues: added wfHighlight, fixed data flow diagram, clarified scope mode, documented 4 GLContext subclasses, clarified isOutputEmpty semantics |

## Session 11: Visual Architecture Comprehensive Verification

**Date:** 2026-07-27
**Model:** opencode/mimo-v2.5-free

### Actions

1. Comprehensive verification of `docs/design/visual-architecture.md` against source code using 8 parallel agent tasks covering: canvas hierarchy, GLPanel system, PrimaryGLContext drawing, GLFont system, ColorTheme system, visual data processors, pipe queue topology, and mouse/keyboard interactions
2. Found 18 discrepancies: 3 medium-severity, 15 minor/cosmetic (~95% accuracy)
3. Fixed all 18 issues

### Issues Fixed

| # | Severity | Issue | Fix |
|---|----------|-------|-----|
| 1 | Medium | PrimaryGLContext drawing methods table omits all parameters | Added full parameter signatures to all 5 methods |
| 2 | Medium | VisualDataDistributor backpressure: "stops when any output is full" | Corrected to "all outputs are full" |
| 3 | Medium | UITestContext describes child panels as active (commented out in code) | Noted that only testMeter is parented; child panels are instantiated but not added to tree |
| 4 | Minor | GLPanel `borderColor`/`borderPx` comment conflates the two | Clarified: border color (RGBA4f) and per-edge border widths |
| 5 | Minor | GLPanel tree omits private `glPoints`/`glColors` arrays | Added to member tree |
| 6 | Minor | GLTextPanel omits `useNativeFont` member | Added note about scale factor bypass |
| 7 | Minor | calcTransform step 4 says "unit corners" imprecisely | Clarified: corners depend on coordinate system (-1/+1 or 0/+1) |
| 8 | Minor | `DrawTunerDigitBox` color param documented as functional (ignored in code) | Added note that color is ignored, always red |
| 9 | Minor | `DrawTunerBarIndexed` alpha param documented as functional (ignored in code) | Added note that alpha is ignored, hardcoded 0.6 |
| 10 | Minor | FFTVisualDataThread loop order incorrect (sleep listed last) | Corrected: sleep first, then distribute, then tight-loop process |
| 11 | Minor | SpectrumVisualProcessor smoothing formula omits NaN guards | Added `x != x` NaN checks to formula |
| 12 | Minor | FFTDataDistributor class undocumented | Added full documentation with rate limiting, buffering, non-blocking push |
| 13 | Minor | Distributor file locations implied as separate files | Noted both are defined inline in `VisualProcessor.h` |
| 14 | Minor | Hover distance stated as fixed 15 kHz | Clarified: dynamic buffer `halfBw + 10kHz * (currentBw / globalBw)` |
| 15 | Minor | Non-view arrow key Shift modifier undocumented | Added 10x bandwidth jump with Shift |
| 16 | Minor | Right-click scale reset described as instant | Corrected: animated exponential interpolation |
| 17 | Minor | `ColorTheme::name` missing from property table | Added to table |
| 18 | Minor | `ThemeMgr::getTheme()` omitted | Added to ThemeMgr description |

### Files Modified

| File | Action |
|------|--------|
| `docs/design/visual-architecture.md` | Fixed 18 discrepancies across PrimaryGLContext, VisualDataDistributor, UITestContext, GLPanel, GLTextPanel, calcTransform, TuningContext, FFTVisualDataThread, SpectrumVisualProcessor, FFTDataDistributor, hover state, keyboard nav, SpectrumCanvas, ColorTheme, ThemeMgr |

## Session 12: Visual Architecture Accuracy Review

**Date:** 2026-07-28
**Model:** opencode/mimo-v2.5-free

### Actions

1. Reviewed `docs/design/visual-architecture.md` by verifying all claims against source code
2. Verified canvas hierarchy (8 canvases), GLPanel system, PrimaryGLContext, derived contexts (Scope, Meter, Tuning, ModeSelector, UITest), GLFont system, ColorTheme, VisualProcessor, SpectrumVisualProcessor, FFTDataDistributor, MouseTracker, and mouse/keyboard interaction details
3. Found 3 minor issues (95%+ accuracy)
4. Applied all 3 fixes

### Issues Fixed

| File | Issue | Fix |
|------|-------|-----|
| `docs/design/visual-architecture.md` | `CHANNELIZER_RATE_MAX` listed as "varies" | Corrected to 500000 (actual value in CubicSDRDefs.h) |
| `docs/design/visual-architecture.md` | GLPanel margin description imprecise | Clarified: shrink transform proportionally by `marginPx * 2 * pvec` in each axis |
| `docs/design/visual-architecture.md` | ScopeCanvas panel interval incomplete | Added total interval = `panelWidth * 2.0 + panelSpacing` |

### Files Modified

| File | Action |
|------|--------|
| `docs/design/visual-architecture.md` | Fixed CHANNELIZER_RATE_MAX, margin scaling description, ScopeCanvas panel interval |

## Session 13: Visual Architecture Post-Review Fix

**Date:** 2026-07-28
**Model:** opencode/mimo-v2.5-free

### Actions

1. Reviewed `docs/design/visual-architecture.md` against source code as a final accuracy check
2. Verified all 20 specific claims (GLPanel members, enums, drag states, drawing methods, constants, formulas, keyboard/mouse interactions)
3. Found 2 minor discrepancies (~97% accuracy)
4. Applied both fixes

### Issues Fixed

| File | Issue | Fix |
|------|-------|-----|
| `docs/design/visual-architecture.md` | GLPanel member tree shows `glPoints`/`glColors` at public level without noting access | Changed to `(private) glPoints, glColors` |
| `docs/design/visual-architecture.md` | WaterfallCanvas section says "Scale factor: Shift+Up/Down adjusts visual gain" — ambiguous naming | Changed to "Visual gain: Shift+Up/Down adjusts `scaleMove` (drives visual gain animation toward target scale factor)" |

### Files Modified

| File | Action |
|------|--------|
| `docs/design/visual-architecture.md` | Fixed GLPanel private member notation and WaterfallCanvas scaleMove naming |

## Session 14: Visual Architecture Accuracy Review

**Date:** 2026-07-28
**Model:** opencode/mimo-v2.5-free

### Actions

1. Reviewed `docs/design/visual-architecture.md` by reading all source headers and implementations against every section of the document
2. Verified canvas class hierarchy (8 canvases), InteractiveCanvas, WaterfallCanvas, SpectrumCanvas, ScopeCanvas, MeterCanvas, TuningCanvas, ModeSelectorCanvas, GainCanvas, UITestCanvas
3. Verified GLPanel system (properties, fill types, coordinate systems, transform pipeline, hit-testing), PrimaryGLContext, all 5 derived contexts, GLFont system, ColorTheme system
4. Verified VisualProcessor template, VisualDataDistributor, VisualDataReDistributor, FFTDataDistributor, SpectrumVisualProcessor (10-step pipeline), ScopeVisualProcessor
5. Verified rendering flows (WaterfallCanvas, ScopeCanvas, SpectrumCanvas OnPaint), mouse interaction, drag state machine, keyboard navigation, key constants
6. Found 8 inaccuracies (drawn functions, misleading claims, imprecise wording)
7. Applied all 7 fixes (one fix addressed two issues)

### Issues Fixed

| File | Issue | Fix |
|------|-------|-----|
| `docs/design/visual-architecture.md` | Generic rendering flow lists `DrawFreqBwInfo` (not called in WaterfallCanvas) | Removed from generic flow, added note that it's SpectrumCanvas-specific |
| `docs/design/visual-architecture.md` | ScopeCanvas paint flow lists `DrawDivider` (never called in OnPaint) | Removed, replaced with actual DrawDeviceName + DrawTunerTitles |
| `docs/design/visual-architecture.md` | ScopeCanvas paint flow omits `bgPanel.draw()` | Added bgPanel.draw() before 3D perspective setup |
| `docs/design/visual-architecture.md` | FFTVisualDataThread claims "redistributes to multiple consumers" | Corrected: single output; multi-consumer distribution happens at SDRPostThread |
| `docs/design/visual-architecture.md` | SpectrumVisualProcessor guard check says "non-empty" | Corrected to "full" (matches isOutputEmpty() semantics) |
| `docs/design/visual-architecture.md` | No note about ScopeVisualProcessor using different smoothing order | Added note: ma updated first, then maa (opposite of SpectrumVisualProcessor) |
| `docs/design/visual-architecture.md` | minBandwidth stated as fixed "30000 Hz" | Changed to "default 30000 Hz" (configurable via setMinBandwidth()) |

### Files Modified

| File | Action |
|------|--------|
| `docs/design/visual-architecture.md` | Fixed 7 issues: removed DrawFreqBwInfo from generic flow, removed DrawDivider from ScopeCanvas, added bgPanel to ScopeCanvas, corrected FFTVisualDataThread redistribution claim, fixed guard check wording, added ScopeVisualProcessor smoothing note, corrected minBandwidth description |

## Session 15: Visual Architecture Targeted Review

**Date:** 2026-07-28
**Model:** opencode/mimo-v2.5-free

### Actions

1. Reviewed `docs/design/visual-architecture.md` by verifying 10 specific claims against source code
2. Verified overview data flow diagram, demod spectrum wiring, GLPanel hit-test usage, MeterContext Draw gradient, ScopeCanvas mouse wheel, and frequency snap
3. Found 5 errors and 1 minor detail (1 claim withdrawn after verification)
4. Applied 4 fixes

### Issues Fixed

| File | Issue | Fix |
|------|-------|-----|
| `docs/design/visual-architecture.md` | Overview diagram shows data flowing from CubicSDR to SDRPostThread (backwards — SDRPostThread is the producer) | Restructured diagram to show SDRPostThread as producer; clarified demod path uses separate `SpectrumVisualDataThread` instance |
| `docs/design/visual-architecture.md` | Hit-test claim: "is not used by the canvas mouse event system" | Corrected: `MeterPanel` uses it for click-to-set-level in `MeterCanvas` |
| `docs/design/visual-architecture.md` | MeterContext gradient described as "center y=0" (vertical) | Corrected to "center x=0" (horizontal); noted left/right asymmetry (right dims alpha too) |
| `docs/design/visual-architecture.md` | ScopeCanvas mouse wheel: "No action (empty handler)" | Corrected: `EVT_MOUSEWHEEL` is absent from the event table entirely |

### Claim Withdrawn

- **Frequency snap** (line 738): "Default is 1 Hz (no snapping)" — verified accurate. All `if (snap > 1)` guards skip rounding when snap=1, so the user-visible behavior is no snapping. No change needed.

### Files Modified

| File | Action |
|------|--------|
| `docs/design/visual-architecture.md` | Fixed 4 issues: restructured overview diagram, corrected hit-test usage claim, fixed MeterContext gradient direction, corrected ScopeCanvas mouse wheel description |

## Session 16: Visual Architecture Document Review

**Date:** 2026-07-28
**Model:** opencode/mimo-v2.5-free

### Actions

1. Reviewed `docs/design/visual-architecture.md` by verifying all claims against source code using a parallel agent task
2. Verified canvas class hierarchy (8 canvases), InteractiveCanvas, all 5 GLContext subclasses, GLPanel system, GLFont system, ColorTheme, VisualProcessor pipeline, MouseTracker, rendering flows, and keyboard/mouse interactions
3. Found 2 issues (~98% accuracy): 1 incorrect attribution, 1 minor omission
4. Applied both fixes

### Issues Fixed

| File | Issue | Fix |
|------|-------|-----|
| `docs/design/visual-architecture.md` | Hit-testing note attributes `isMeterHit()`/`getMeterHitValue()` usage to `MeterCanvas` | Corrected to `GainCanvas` — MeterCanvas uses `mouseTracker.getMouseY()` directly |
| `docs/design/visual-architecture.md` | `MeterContext::DrawBegin()` omits `glDisable(GL_CULL_FACE)` and `glDisable(GL_DEPTH_TEST)` | Added both state changes to match implementation and consistency with UITestContext |

### Files Modified

| File | Action |
|------|--------|
| `docs/design/visual-architecture.md` | Fixed hit-testing attribution (MeterCanvas → GainCanvas) and added missing GL state disables to MeterContext::DrawBegin |

## Session 17: Visual Architecture Final Review

**Date:** 2026-07-28
**Model:** opencode/mimo-v2.5-free

### Actions

1. Comprehensive re-verification of `docs/design/visual-architecture.md` against source code using 11 parallel agent tasks
2. Verified all major sections: canvas hierarchy, GLPanel system, PrimaryGLContext, all 5 GLContext extensions, GLFont system, ColorTheme system, VisualProcessor pipeline, SpectrumVisualProcessor, ScopeVisualProcessor, rendering flows, mouse interaction, drag state machines, constants
3. Found 2 factual errors, 4 misleading descriptions, and 5 minor omissions (~97% accuracy)
4. Applied 8 fixes covering errors and misleading descriptions; minor omissions were acceptable as-is

### Issues Fixed

| File | Issue | Fix |
|------|-------|-----|
| `docs/design/visual-architecture.md` | Data flow timing: IQ packet size stated as 64 samples/packet at ~37,500/sec | Corrected to 40,000 samples/packet at 60/sec (`sampleRate / TARGET_DISPLAY_FPS`); producer corrected from SDRPostThread to SDRThread |
| `docs/design/visual-architecture.md` | SpectrumVisualProcessor described as rate-limited internally | Corrected: rate limiting is in upstream `FFTDataDistributor`; processor drains whatever is available |
| `docs/design/visual-architecture.md` | `GLPANEL_FILL_NONE` vertex count listed as 0 | Corrected to 8 (allocated in `genArrays()` but skipped in `draw()`) |
| `docs/design/visual-architecture.md` | `DrawSelector` parameter names `padx`/`pady` | Corrected to `px`/`py` (matching code) |
| `docs/design/visual-architecture.md` | GLFont cache eviction stated as "every 50 calls" | Corrected: fires when `gcCounter > 50` (51st call) |
| `docs/design/visual-architecture.md` | Hover state `shiftDown` branch implies it selects a different drag state | Corrected: `!shiftDown` gates entry to demod-hover path; shift held yields `WF_DRAG_NONE` |
| `docs/design/visual-architecture.md` | SpectrumCanvas right-click described as unconditional | Corrected: only triggers when `originDeltaMouseY == 0` (pure click, no vertical drag) |
| `docs/design/visual-architecture.md` | `DrawTunerDigitBox` and `DrawTunerBarIndexed` ignored parameters | Already documented in original; no change needed (verified present at lines 266, 268) |

### Files Modified

| File | Action |
|------|--------|
| `docs/design/visual-architecture.md` | Fixed 8 issues: corrected IQ packet sizing, SDRThread producer attribution, SpectrumVisualProcessor rate limiting source, GLPANEL_FILL_NONE vertices, DrawSelector parameter names, GLFont GC trigger, hover state shiftDown behavior, SpectrumCanvas right-click condition |

## Session 18: Visual Architecture Verification

**Date:** 2026-07-28
**Model:** opencode/mimo-v2.5-free

### Actions

1. Reviewed `docs/design/visual-architecture.md` by verifying all claims against source code
2. Read 25+ source files (headers and implementations) covering every section of the document
3. Verified canvas hierarchy (8 canvases), GLPanel system, PrimaryGLContext, all 5 GLContext extensions, GLFont system, ColorTheme system, VisualProcessor pipeline, SpectrumVisualProcessor, ScopeVisualProcessor, FFTDataDistributor, FFTVisualDataThread, rendering flows, mouse/keyboard interaction, drag state machines, and key constants
4. Found 14 issues; re-verified each against source before deciding which to fix
5. Confirmed 3 issues were false positives (MeterContext texturing disable, TuningContext texturing disable, ScopeCanvas mouse wheel description all accurate as written)
6. Applied 2 fixes

### Issues Fixed

| File | Issue | Fix |
|------|-------|-----|
| `docs/design/visual-architecture.md` | ScopeVisualProcessor smoothing order note says "opposite of SpectrumVisualProcessor" which could be misread as the loop body order being reversed | Clarified both processors explicitly: SpectrumVisualProcessor updates `maa` first (using old `ma`), then `ma`; ScopeVisualProcessor updates `ma` first (from raw input), then `maa` (using new `ma`) |
| `docs/design/visual-architecture.md` | TuningContext `DrawTuner` font size thresholds listed as "32→24→18→16→12px" (single linear cascade) | Replaced with accurate dual-path description: width-based (≥500→32, ≥300→24, else→18) and height-based (≥28→18, ≥24→16, else→12) |

### False Positives (No Fix Needed)

| Claim | Code Verification |
|-------|-------------------|
| MeterContext::DrawBegin "disables texturing" | `MeterContext.cpp:22`: `glDisable(GL_TEXTURE_2D)` IS called |
| TuningContext::DrawBegin "disables texturing" | `TuningContext.cpp:41`: `glDisable(GL_TEXTURE_2D)` IS called |
| ScopeCanvas mouse wheel "handler exists but EVT_MOUSEWHEEL is absent from event table" | `ScopeCanvas.cpp:21-30`: no EVT_MOUSEWHEEL in table; handler at line 244 is empty `{}` — description accurate |

### Files Modified

| File | Action |
|------|--------|
| `docs/design/visual-architecture.md` | Fixed ScopeVisualProcessor smoothing order note and TuningContext font size thresholds |

## Session 19: Signal Flow Design Document Review

**Date:** 2026-07-28
**Model:** opencode/mimo-v2.5-free

### Actions

1. Reviewed `docs/design/signal-flow.md` by verifying all claims against source code using parallel agent tasks
2. Verified queue names, types, max sizes (8 queues), thread creation/termination orders, data type member variables (6 types), DC blocking filter, channelizer types, frequency shifting, resampling, modem dispatch, controller/bound audio pattern, ReBuffer pool behavior, and visual processing pipeline
3. Found 2 factual errors, 2 minor inaccuracies, and noted several omissions as judgment calls (~97% accuracy)
4. Applied 2 fixes for factual errors; left 2 minor inaccuracies (off-by-one in ReBuffer GC, imprecise signal level wording) as acceptable

### Errors Fixed

| File | Issue | Fix |
|------|-------|-----|
| `docs/design/signal-flow.md` | Pipeline diagram references `FFTDataDistributor (waterfall)` | Corrected to `FFTVisualDataThread (waterfall)` — `FFTDataDistributor` is an internal helper member, not the thread class |
| `docs/design/signal-flow.md` | Stage 4 says "Computes signal levels (peak, RMS)" | Corrected to "Computes signal level (mean magnitude, dB)" — actual code computes `mean(sqrt(I²+Q²))` then `20*log10()` |

### Issues Noted But Not Fixed

| Issue | Rationale |
|-------|-----------|
| ReBuffer GC described as "100 idle cycles" (actually 101+) | Off-by-one; close enough for practical purposes |
| `DemodulatorMgr` not mentioned in document | Document focuses on data path, not management layer |
| `DemodulatorWorkerThread` absent from pipeline diagram | Described accurately in Stage 3 text; diagram would be cluttered |
| Recording path absent from pipeline diagram | Described in Stage 4 text; diagram addition optional |
| No error/disconnect handling discussion | Document scope is happy-path signal flow |

### Files Modified

| File | Action |
|------|--------|
| `docs/design/signal-flow.md` | Fixed waterfall thread class name and signal level computation description |

## Session 20: Signal Flow Design Document Verification and Fix

**Date:** 2026-07-28
**Model:** opencode/mimo-v2.5-free

### Actions

1. Reviewed `docs/design/signal-flow.md` by verifying all claims against source code using a detailed agent task
2. Re-verified each finding directly against source before applying fixes
3. Found 11 discrepancies: 2 critical, 4 moderate, 5 minor
4. Applied all fixes

### Issues Fixed

| # | Severity | Issue | Fix |
|---|----------|-------|-----|
| 1 | Critical | DC blocking described as applied to all data before channelization; actually channel-0 only in multi-channel mode | Split Stage 2 into multi-channel vs single-channel paths; noted DC blocking is `if (i == 0)` in `runDemodChannels()` |
| 2 | Critical | "Entirely pull-based" claim — "Worker threads never push data to the UI" | Corrected to push-pull hybrid: producers use `try_push()`, consumers use `try_pop()` in `OnIdle()` |
| 3 | Moderate | `audioVisOutputQueue` listed in Global Queues table | Moved to Per-Demodulator Queues table with note about runtime binding to global `pipeAudioVisualData` |
| 4 | Moderate | `audioSinkOutputQueue` missing from queue tables | Added to Per-Demodulator Queues table with note about dynamic binding on recording start |
| 5 | Moderate | Blocking vs non-blocking push asymmetry undocumented | Documented: DemodulatorPreThread uses blocking `push()`, DemodulatorThread uses non-blocking `try_push()` |
| 6 | Moderate | Single-channel mode not described | Added single-channel path: DC blocking + full bandwidth push, no channelization |
| 7 | Minor | Pipeline diagram used wrong consumer names ("Main spectrum FFT", "Waterfall FFT") | Updated to `SpectrumVisualDataThread`, `FFTVisualDataThread` |
| 8 | Minor | Demod spectrum only mentioned feeding demod spectrum display | Noted it feeds both `demodSpectrumCanvas` and `demodWaterfallCanvas` |
| 9 | Minor | `audioCallback` described as method | Corrected to file-scope static function |
| 10 | Minor | ReBuffer GC description imprecise; `REBUFFER_WARNING_THRESHOLD` omitted | Clarified age-decrement-per-selection mechanism; added warning threshold (2000) |
| 11 | Minor | DemodulatorWorkerThread queue sizes undocumented | Added command queue (max 2) and result queue (max 100) to Stage 3 description |

### Files Modified

| File | Action |
|------|--------|
| `docs/design/signal-flow.md` | Fixed 11 issues across pipeline diagram, stage descriptions, queue tables, ReBuffer description, visual pipeline, and push semantics |
