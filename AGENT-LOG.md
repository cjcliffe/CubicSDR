# Agent Session Log

## Logging Format

Log your work by appending a section for each session. For each session, include:
- Date and model name
- Numbered list of actions taken
- Tables of files created/modified with brief descriptions

Keep entries scannable — capture **outcomes**, not process. If something is added then later removed, note the removal and why. Don't keep both.

---

## Session 1: Project Evaluation and Planning

**Date:** 2026-07-23
**Model:** opencode/big-pickle

- Explored codebase structure, technology stack, key directories
- Analyzed code quality (TODO/FIXME, tests, memory safety, build, docs, git, deps)
- Created 17 documentation files: RECOMMENDATIONS.md, PLAN.md, 4 architecture docs under docs/design/, 10 plan files under docs/plans/, AGENTS.md, AGENT-LOG.md

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

---

## Sessions 2-4: Documentation Refinement and Debrittling

**Date:** 2026-07-23
**Model:** opencode/big-pickle

Multiple passes over all documentation to fix errors, improve quality, and remove brittle content:
- Fixed broken cross-references, added verification criteria and rollback strategies to plan files
- Verified ~50+ factual claims, fixed 13 errors
- Removed "Last Updated" timestamps from all 17 docs
- Removed redundant preambles, subjective sections, exact counts, brittle line numbers, editorial commentary
- Added Documentation Guidelines to AGENTS.md

### Files Modified

| File | Action |
|------|--------|
| All 17 docs under `docs/` | Removed timestamps, preambles, editorial commentary |
| `AGENTS.md` | Added Documentation Guidelines section |

---

## Session 5: Documentation Accuracy Review

**Date:** 2026-07-24
**Model:** opencode/mimo-v2.5-free

- Verified all factual claims in docs/ against source code
- Fixed 10 factual errors across 5 files (missing data type fields, incorrect pipeline wiring, wrong return types, file path errors)

---

## Session 6: Subsystem Deep Dive Documentation

**Date:** 2026-07-24
**Model:** opencode/mimo-v2.5-free

- Created 5 new subsystem deep dive documents under `docs/design/`
- Updated README.md and PLAN.md to link new documents

### Files Created

| File | Description |
|------|-------------|
| `docs/design/audio-subsystem.md` | AudioThread controller/bound pattern, WAV recording pipeline, device management |
| `docs/design/visual-architecture.md` | Canvas hierarchy, GLPanel system, GLFont, ColorTheme, visual data processing |
| `docs/design/configuration-system.md` | AppConfig/DeviceConfig persistence, DataTree serialization, session management |
| `docs/design/bookmark-system.md` | BookmarkMgr data model, groups/ranges/recents, XML persistence |
| `docs/design/sdr-device-layer.md` | SDREnumerator discovery, SDRDeviceInfo capabilities, SoapySDR module loading |

---

## Session 7: Documentation Verification and Priority Planning

**Date:** 2026-07-27
**Model:** opencode/mimo-v2.5-free

- Verified 59 claims across 9 design docs (97% accuracy, 2 errors found)
- Fixed MIN_BANDWIDTH value and pthread stack size
- Noted 17m band source bug in BookmarkMgr.cpp
- Created `docs/PRIORITY.md` with ordered next steps

---

## Sessions 8-18: Visual Architecture Documentation Review

**Date:** 2026-07-27 to 2026-07-28
**Model:** opencode/mimo-v2.5-free

Extensive iterative review and expansion of `docs/design/visual-architecture.md`:

**Session 8 — Expansion:**
- Expanded 6 sections with source-level detail: GLPanel, VisualProcessor, Mouse Interaction, GLFont, PrimaryGLContext, Rendering Flow

**Sessions 9-18 — Verification and fixes (grouped):**
- Multiple verification passes using parallel agent tasks (up to 14 tasks per pass)
- Fixed ~70+ issues total across all passes, including:
  - Corrected data flow diagrams (pipe ownership, producer attribution)
  - Fixed GLPanel constants, member access levels, coordinate systems
  - Corrected GLFont GC trigger semantics, cache eviction timing
  - Fixed mouse interaction details (hit-test usage, drag states, hover distance, keyboard navigation)
  - Fixed ScopeCanvas, MeterContext, TuningContext details
  - Corrected VisualProcessor pipeline descriptions (two-phase locking, smoothing formulas, distribution modes)
  - Fixed SpectrumVisualProcessor rate limiting source, IQ packet sizing, FFTDataDistributor documentation
  - Documented 4 GLContext subclasses (MeterContext, TuningContext, ModeSelectorContext, UITestContext)

### Files Modified

| File | Action |
|------|--------|
| `docs/design/visual-architecture.md` | Expanded 6 sections; fixed ~70+ issues across ~10 verification passes |

---

## Sessions 19-22: Signal Flow Documentation Review

**Date:** 2026-07-28
**Model:** opencode/mimo-v2.5-free

Iterative review and fixes to `docs/design/signal-flow.md`:

- Verified queue names/types/max sizes, thread creation/termination orders, data type members, DC blocking, channelizer, frequency shifting, resampling, modem dispatch, audio pattern, ReBuffer, visual pipeline
- Fixed waterfall thread class name, signal level computation description
- Fixed DC blocking scope (channel-0 only in multi-channel mode)
- Corrected push semantics (push-pull hybrid, not purely pull-based)
- Added missing queue documentation, blocking vs non-blocking asymmetry
- Fixed canvas polling locations (only WaterfallCanvas uses OnIdle)
- Documented FFTVisualDataThread internal sub-pipeline, type aliasing, ReBuffer GC threshold wording

### Files Modified

| File | Action |
|------|--------|
| `docs/design/signal-flow.md` | Fixed ~17 issues across 4 verification passes |

---

## Sessions 23-29: Audio Subsystem Documentation Review

**Date:** 2026-07-28 to 2026-07-30
**Model:** opencode/mimo-v2.5-free

Iterative review and fixes to `docs/design/audio-subsystem.md`:

- Verified all 10 audio files plus DemodulatorInstance, DemodulatorThread, IOThread
- Fixed static map count, AudioSinkThread input detection, WAV normalization description
- Fixed device setup flow (missing step, binding direction), thread lifecycle (setInitOutputDevice call site)
- Added Real-Time Design Constraints, Buffer Management, Muting, Digital Modem Audio sections
- Fixed ReBuffer age management, muting solo mode condition, setActive() flush behavior
- Fixed audioCallback first-packet behavior, empty-data retry logic
- Fixed digital modem audio (nullification vs discard), recording push squelch guard
- Cleaned all editorial commentary

### Files Modified

| File | Action |
|------|--------|
| `docs/design/audio-subsystem.md` | Fixed ~26 issues, added 4 sections across 7 verification passes |
| `AGENTS.md` | Added "No editorial commentary" guideline |
| `docs/design/README.md` | Removed editorial commentary |

---

## Sessions 30-31: Modem System Documentation Review

**Date:** 2026-07-30
**Model:** opencode/mimo-v2.5-free

Review and fixes to `docs/design/modem-system.md`:

- Fixed pipeline diagram output type, added DefaultRatesList typedef
- Expanded interface table (virtual/non-virtual/static methods)
- Expanded ModemKit hierarchy (added ModemKitCW, ModemKitFMStereo)
- Added Supporting Types section, ENABLE_DIGITAL_LAB clarification
- Documented checkSampleRate, rebuildKit, useSignalOutput mechanisms
- Added stereo output note, DSP approach summary, Modem Settings section
- Fixed getDemodulatorLock return type

### Files Modified

| File | Action |
|------|--------|
| `docs/design/modem-system.md` | Fixed 13 issues across 2 passes |

---

## Sessions 32-43: Threading Documentation Review

**Date:** 2026-07-31 to 2026-08-02
**Model:** opencode/mimo-v2.5-free

Extensive iterative review and fixes to `docs/design/threading.md`:

- Verified IOThread base class, all 15 thread classes, all communication patterns
- Fixed startup order (SDRPostThread before AppFrame), added hamlib to shutdown
- Fixed terminate() mutex claim (no lock, unlike run/isTerminated)
- Corrected Known Issues (pthread_join bug: wrong variable name + wrong API)
- Added SDREnumerator exit leak and waterfall thread memory leak to Known Issues
- Expanded synchronization table (added missing mutexes, SpinMutex locations, atomic flags)
- Added missing communication patterns (callback notification, VisualProcessor pipeline)
- Fixed wxWidgets integration claim (pull-based with exception, 9 EVT_IDLE handlers)
- Added thread priority details, m_thread_control_mutex to lifecycle sections
- Fixed ReBuffer GC condition, SpectrumVisualProcessor two-phase locking description

### Files Modified

| File | Action |
|------|--------|
| `docs/design/threading.md` | Fixed ~60+ issues, added 3 communication patterns, expanded sync table across 12 passes |

---

## Session 44: Agent Log Compaction

**Date:** 2026-08-02
**Model:** opencode/mimo-v2.5-free

- Compacted AGENT-LOG.md from 1267 lines to ~234 lines by grouping 44 sessions into 8 task-based entries
- Added standard logging format note at the top
- Verified compacted content against original; corrected issue count estimates for Sessions 8-18 (~50+ → ~70+), Sessions 23-29 (~25 → ~26), and Sessions 32-43 (~50+ → ~60+)

---

## Session 45: Threading Documentation Final Review

**Date:** 2026-08-02
**Model:** opencode/mimo-v2.5-free

- Full verification of threading.md against source code using 7 parallel agent tasks
- Verified all claims: IOThread, SDRThread, demodulator threads, audio threads, CubicSDR startup/shutdown, visual processors, wxWidgets integration
- Found 2 discrepancies: UITestCanvas (class doesn't exist) and SCHED_FIFO wording (block is fully commented out, not just on macOS)
- Applied 2 fixes

### Files Modified

| File | Action |
|------|--------|
| `docs/design/threading.md` | Removed non-existent UITestCanvas from canvas list; clarified SCHED_FIFO is commented out on all platforms |

---

## Session 46: Threading Documentation Completeness Review

**Date:** 2026-08-02
**Model:** opencode/mimo-v2.5-free

- Full verification of threading.md against source code (re-verified all claims from Session 45)
- Confirmed document is accurate — no factual errors found
- Added 4 minor omissions for completeness:
  1. Pattern 7: UI-Thread Processing — documented ScopeVisualProcessor runs on wxWidgets main thread (no dedicated thread), with non-blocking try_pop data path
  2. Heartbeat Period subsection — documented the consistent 50ms HEARTBEAT_CHECK_PERIOD_MICROS used across all IOThread subclasses (~20Hz shutdown check)
  3. AudioSinkThread description — clarified as abstract base with its own IOThread run loop (not just "base audio sink")
  4. VisualProcessor Pipeline threading models — distinguished dedicated-thread (SpectrumVisualProcessor, FFTVisualDataThread) from UI-thread (ScopeVisualProcessor) usage

### Files Modified

| File | Action |
|------|--------|
| `docs/design/threading.md` | Added Pattern 7, Heartbeat Period section, AudioSinkThread description, VisualProcessor threading models |

---

## Session 47: Threading Documentation Final Accuracy Review

**Date:** 2026-08-02
**Model:** opencode/mimo-v2.5-free

- Full verification of threading.md against source code using 9 parallel agent tasks covering IOThread, SDRThread, demodulator threads, audio threads, visual processors, SpinMutex/ReBuffer, CubicSDR notifications, ThreadBlockingQueue, SDREnumerator/shutdown, and wxWidgets integration
- All claims verified accurate — document is in excellent shape
- Found 1 genuine error: AppFrame entry in thread inventory table had wrong class name (`wxApp/wxFrame` → `AppFrame`) and wrong file path (`AppFrame.cpp` → `src/AppFrame.h`)
- Two other items investigated (shutdown sequence wording, pthread_create attribution) were confirmed accurate on closer inspection — no changes needed

### Files Modified

| File | Action |
|------|--------|
| `docs/design/threading.md` | Fixed AppFrame class name and file path in thread inventory table |

---

## Session 48: Threading Documentation Accuracy and Completeness Review

**Date:** 2026-08-02
**Model:** opencode/mimo-v2.5-free

- Full verification of threading.md against source code using 18 parallel agent tasks (IOThread, SpinMutex, SDRThread, SDRPostThread, SDREnumerator, DemodulatorThread, DemodulatorPreThread, DemodulatorWorkerThread, DemodulatorInstance, AudioThread, AudioSinkThread, VisualProcessor, SpectrumVisualProcessor, SpectrumVisualDataThread, FFTVisualDataThread, CubicSDR/AppFrame, RigThread)
- Found 1 inaccuracy: SDREnumerator `std::terminate()` claim — the overwrite of `t_SDREnum` pointer does not invoke the destructor (no `delete` is called), so `std::terminate()` is never triggered; actual consequences are a memory leak and orphaned thread
- Found 15 omissions of additional atomics, mutexes, and behavioral details
- Applied 1 fix and 7 additions:
  1. Fixed SDREnumerator `std::terminate()` inaccuracy
  2. Added ~20 missing atomic flags to Pattern 2 (SDRThread state atomics, DemodulatorInstance numeric/pointer atomics, DemodulatorThread signal atomics, SDRPostThread chanMode, CubicSDR additional flags, AudioThread active/outputDevice, FFTVisualDataThread linesPerSecond, RigThread control flags, AppFrame deviceChanged/modemPropertiesUpdated)
  3. Added DemodulatorThread SpinMutex, RigThread atomics, named mutexes, and static AudioThread m_device_mutex to synchronization table
  4. Added SpectrumVisualProcessor back-pressure detail (isOutputEmpty frame drop)
  5. Added DemodulatorPreThread worker thread termination timeout (5000ms)
  6. Added AudioThread static m_device_mutex and sample rate change reconfiguration to Pattern 3
  7. Added AudioSinkThread inputChanged() virtual, input queue max, m_mutex; IOThread destructor behavior, isTerminated edge cases, gTimer

### Files Modified

| File | Action |
|------|--------|
| `docs/design/threading.md` | Fixed 1 inaccuracy, added 7 completeness improvements across Pattern 2, sync table, Pattern 3, SpectrumVisualProcessor, Per-Demodulator Shutdown, AudioSinkThread, IOThread |

---

## Session 49: SDR Device Layer Documentation Review

**Date:** 2026-08-02
**Model:** opencode/mimo-v2.5-free

- Reviewed `docs/design/sdr-device-layer.md` against source code (SDRDeviceInfo.h/cpp, SDREnumerator.h/cpp, SoapySDRThread.h/cpp)
- Found 3 inaccuracies, 4 omissions, 1 minor issue
- Applied 9 fixes:
  1. Fixed `getDeviceId()` description (returns `getName()`, not driver+serial/remote/factory)
  2. Fixed `getSoapyDevice()` description (lazily creates device on demand, not set via `setSoapyDevice()`)
  3. Corrected module loading order (`BUNDLED_MODS_ONLY` vs `getUseLocalMod()` distinction)
  4. Noted `SDR_ENUM_TERMINATED` as defined but unused
  5. Added `getIndex()` to Hardware Queries table
  6. Added Device and Stream Arguments section (`get/setDeviceArgs`, `get/setStreamArgs`)
  7. Marked `hasCORR()` as deprecated
  8. Added `soapy_initialized` and `has_remote` to Static State table
  9. Clarified `SDR_ENUM_FAILED` condition (only `null` factory found)

### Files Modified

| File | Action |
|------|--------|
| `docs/design/sdr-device-layer.md` | Fixed 3 inaccuracies, added 6 completeness improvements |

## Session 50: SDR Device Layer Documentation Verification

**Date:** 2026-08-02
**Model:** opencode/mimo-v2.5-free

- Re-reviewed `docs/design/sdr-device-layer.md` against SDREnumerator.cpp, SDRDeviceInfo.h/cpp, SoapySDRThread.h, SDRDevices.h
- Found 3 issues in enumeration flow description and notification table
- Applied 2 edits:
  1. Rewrote Enumeration Flow to reflect that manual devices are enumerated within `enumerate_devices()` (not as a separate step), and stored in both `devs[""]` and `devs[remoteAddr]`
  2. Corrected `SDR_ENUM_FAILED` condition: factory list must have exactly one entry AND it must be `null`

### Files Modified

| File | Action |
|------|--------|
| `docs/design/sdr-device-layer.md` | Fixed enumeration flow accuracy, corrected SDR_ENUM_FAILED description |

---

## Session 51: SDR Device Layer Documentation Accuracy and Completeness Review

**Date:** 2026-08-03
**Model:** opencode/mimo-v2.5-free

- Full verification of `docs/design/sdr-device-layer.md` against source code (SDRDeviceInfo.h/cpp, SDREnumerator.h/cpp, SoapySDRThread.h, SDRDevices.h, threading.md, signal-flow.md)
- Cross-referenced all claims in enumeration flow, static state, notifications, identity/state tables, device arguments, and module loading
- Found 6 issues: 2 completeness gaps, 3 inaccuracies, 1 omission
- Applied 6 fixes:

### Files Modified

| File | Action |
|------|--------|
| `docs/design/sdr-device-layer.md` | Added 6 fixes for accuracy and completeness |

| Fix | Section | Description |
|-----|---------|-------------|
| 1 | Enumeration Flow | Documented `noInit` parameter — returns cached results without re-initializing SoapySDR |
| 2 | Notification table | Clarified `SDR_ENUM_FAILED` fires during `enumerate_devices()` initialization, not after; `SDR_ENUM_DEVICES_READY` is still sent |
| 3 | Static State | Noted `reset()` clears `devs`/`factories`/`modules` but not `remotes` or `manuals` |
| 4 | Identity/State tables | Annotated `serial`, `tuner`, `manufacturer`, `product`, `timestamps` as never populated (setters exist but are never called) |
| 5 | Stream arguments | Replaced incorrect `buflen`/`bufflen`/`remote` table with note that CubicSDR does not populate specific stream arg keys |
| 6 | State table | Added `manual_params` property used during manual device removal |

---

## Session 52: SDR Device Layer Documentation Re-verification

**Date:** 2026-08-03
**Model:** opencode/mimo-v2.5-free

- Full re-verification of `docs/design/sdr-device-layer.md` against source code (SDRDeviceInfo.h/cpp, SDREnumerator.h/cpp, SoapySDRThread.h, SDRDevices.h)
- Cross-referenced all claims in enumeration flow, static state, notifications, identity/state tables, device arguments, and module loading
- Evaluated 4 potential corrections (noInit description, manual device location, sample rate decimation, stream args)
- After careful review, determined all 4 items are accurate as written — no changes needed

### Files Modified

| File | Action |
|------|--------|
| (none) | No changes required |

---

## Session 53: Bookmark System Documentation Review

**Date:** 2026-08-03
**Model:** opencode/mimo-v2.5-free

- Reviewed `docs/design/bookmark-system.md` against source code (BookmarkMgr.h/cpp, BookmarkView.h/cpp, DemodulatorMgr.cpp)
- Found 12 issues: 4 high, 5 medium, 3 low priority
- Applied 13 fixes:

### Files Modified

| File | Action |
|------|--------|
| `docs/design/bookmark-system.md` | Fixed XML field name, expand state architecture, added missing type aliases/API methods, clarified sorting/trim/backup behavior, documented UI features and thread safety |

| # | Priority | Fix | Section |
|---|----------|-----|---------|
| 1 | High | XML field name `<label>` → `<user_label>` | Bookmark File Format |
| 2 | High | Rewrote expand state to document two-system architecture (BookmarkView for branches, BookmarkMgr for groups) | Expand State |
| 3 | High | Added `BookmarkMapSorted`, `BookmarkNames`, `BookmarkExpandState` type aliases | Type Aliases |
| 4 | High | Added 9 missing API methods (`resetBookmarks`, `hasLastLoad`, `hasBackup`, `saveToFile`, `loadFromFile`, `removeActive`, `updateActiveList`, `updateBookmarks` ×2) | Bookmark Operations |
| 5 | Medium | `getGroups()` shows both `BookmarkNames&` and `wxArrayString&` overloads | Group Operations |
| 6 | Medium | Load flow clarifies `backup` flag + `.lastloaded`/`.backup` conditions | Load Flow |
| 7 | Medium | `getBookmarks()`/`getRanges()` note frequency sorting | Bookmark/Range Operations |
| 8 | Medium | `trimRecents` soft limit clarified (one entry removed per call) | Recent Entries |
| 9 | Medium | Added `bmDataSorted` and `rangesSorted` to architecture diagram | Overview |
| 10 | Low | Thread safety notes exceptions where lock is not held | BookmarkMgr |
| 11 | Low | Added search, range mgmt, recording, tooltips, inline editing | Additional UI Features |
| 12 | Low | Default expand states documented (active/bookmark/recent=true, range=false) | Expand State |
| 13 | High | Fixed DataTree `@` notation: `<group><@name>..</@name>` → `<group name=".." expanded="true">` (Badgerfish attribute convention) | Bookmark File Format |

---

## Session 54: Bookmark System Documentation Review

**Date:** 2026-08-03
**Model:** opencode/mimo-v2.5-free

- Reviewed `docs/design/bookmark-system.md` against source code (BookmarkMgr.h/cpp, BookmarkView.h/cpp, CubicSDR.cpp)
- Verified data model, API tables, persistence flows, expand state, default ranges, UI integration
- Found 1 inaccuracy (`removeActive()` listed as missing but was already in table), 4 omissions, 1 minor issue
- Applied 5 fixes:

### Files Modified

| File | Action |
|------|--------|
| `docs/design/bookmark-system.md` | Fixed 2 method descriptions, improved save flow accuracy, added error recovery section, noted layering concern |

| # | Fix | Section |
|---|-----|---------|
| 1 | `addGroup()` — added "(no-op if group already exists)" | Group Operations |
| 2 | `renameGroup()` — added merge behavior note (appends entries when target exists) | Group Operations |
| 3 | Save flow step 3 — noted model-to-view dependency (`BookmarkView::getExpandState()` called from `saveToFile()`) | Save Flow |
| 4 | Save flow step 5 — replaced "stale bookmark data" with neutral "bookmark's stored state"; added match criteria (type, label, frequency, bandwidth) | Save Flow |
| 5 | Added Error Recovery subsection documenting cascading recovery dialog chain (`ActionDialogBookmarkLoadFailed` → `ActionDialogBookmarkBackupLoadFailed` → `ActionDialogBookmarkCatastophe`) | Backup Strategy |

## Session 55: Bookmark System Documentation Review

**Date:** 2026-08-03
**Model:** opencode/mimo-v2.5-free

- Reviewed `docs/design/bookmark-system.md` against source code (BookmarkMgr.h/cpp, BookmarkView.h/cpp)
- Cross-referenced all data model fields, method signatures, persistence flows, expand state, default ranges, and UI interactions
- Found 6 issues: 2 misleading descriptions, 3 missing details, 1 inaccurate description
- Applied 6 fixes:

### Files Modified

| File | Action |
|------|--------|
| `docs/design/bookmark-system.md` | Fixed addRecent descriptions, added trimRecents locking note, corrected Demodulator Interaction section, added getExpandState/setExpandState API docs, clarified save flow matching behavior |

| # | Fix | Section |
|---|-----|---------|
| 1 | `addRecent(demod)` — clarified it serializes demod state; `addRecent(entry)` — clarified it takes a `BookmarkEntryPtr` | Recent Entries |
| 2 | Added note that `trimRecents()` is lock-free and always called from locked context (`addRecent`) | Recent Entries |
| 3 | Rewrote "Add bookmark" to describe properties panel dropdown (not right-click); rewrote "Delete" to cover DELETE key + Remove button for all item types | Demodulator Interaction |
| 4 | Added paragraph documenting `getExpandState()`/`setExpandState()` public accessors on both BookmarkView and BookmarkMgr | Expand State |
| 5 | Save flow step 5 now names `getLastDemodulatorWith()` and notes last-match behavior for duplicate demodulators | Save Flow |

---

## Session 56: Bookmark System Design Doc Review

**Date:** 2026-08-03
**Model:** opencode/mimo-v2.5-free

- Reviewed `docs/design/bookmark-system.md` against all source files (`BookmarkMgr.h`, `BookmarkMgr.cpp`, `BookmarkView.h`, `BookmarkView.cpp`, `CubicSDR.cpp`)
- Verified data model, type aliases, method signatures, persistence format, save/load flows, backup strategy, UI integration, expand state, and default ranges
- Found 3 accuracy/completeness issues; applied fixes

### Files Modified

| File | Action |
|------|--------|
| `docs/design/bookmark-system.md` | Fixed thread-safety list, getBookmarks side effect note, error recovery branching |

| # | Fix | Section |
|---|-----|---------|
| 1 | Added `getBookmarkEntryDisplayName()` and `getActiveDisplayName()` to non-locked methods list | BookmarkMgr overview |
| 2 | Added "(sorts internal list as a side effect)" to `getBookmarks()` description | Bookmark Operations |
| 3 | Rewrote Error Recovery to reflect actual branching logic (initial check selects dialog by file existence; backup failure triggers secondary chain) | Error Recovery |

---

## Session 57: Configuration System Documentation Review

**Date:** 2026-08-05
**Model:** opencode/mimo-v2.5-free

- Reviewed `docs/design/configuration-system.md` against source code (AppConfig.h/cpp, SessionMgr.h/cpp, DataTree.h/cpp, AppFrame.h/cpp, BookmarkMgr.cpp)
- Found 3 inaccuracies and 9 omissions
- Applied 6 fixes:

### Files Modified

| File | Action |
|------|--------|
| `docs/design/configuration-system.md` | Added missing bookmark file, conditional window write, expanded thread safety, save triggers, DataNode operators, DataTree constructor, floating-point policy |

| # | Fix | Section |
|---|-----|---------|
| 1 | Added `bookmarks.xml.failedload` to config files table | Config Files |
| 2 | Documented window node conditional write (only when winW and winH are non-zero) | Save/Load Lifecycle |
| 3 | Expanded thread safety to cover AppConfig atomics and note missing mutex on string fields | Thread Safety |
| 4 | Added `saveDisabled` early return and bookmark save on close to save triggers | Config Save Triggers |
| 5 | Added DataNode operator overloads and `findAll()` to operations section | DataNode Operations |
| 6 | Added DataTree named constructor and `DT_FloatingPointPolicy` to DataTree section | Architecture, XML Serialization |

---

## Session 58: Configuration System Documentation Re-review

**Date:** 2026-08-05
**Model:** opencode/mimo-v2.5-free

- Full re-review of `docs/design/configuration-system.md` against source code (AppConfig.h/cpp, SessionMgr.h/cpp, DataTree.h/cpp, BookmarkMgr.cpp)
- Found 4 inaccuracies, 9 completeness gaps; applied 8 fixes

### Files Modified

| File | Action |
|------|--------|
| `docs/design/configuration-system.md` | Fixed thread safety coverage, config save triggers, DataNode operations table, DataElement types, DeviceConfig id parsing, bookmark recovery logic, printXML note |

| # | Fix | Section |
|---|-----|---------|
| 1 | Corrected thread safety: mutex only protects deviceId/deviceName/save()/load(); antennaName and map fields are unprotected | Thread Safety |
| 2 | Clarified config save triggers: in-memory updates happen immediately, file write only on window close or explicit saveConfig() | Config Save Triggers |
| 3 | Fixed manualDevices listed as "string field" — it's a vector | Thread Safety |
| 4 | Expanded DataNode operations table with missing methods (hasAnother(), getNext(), rewind*, parent nav, numChildren(name), newChild overloads) | DataNode Operations |
| 5 | Added std::set<string> and DATA_VOID raw buffer to DataElement types | DataElement Types |
| 6 | Added note that DeviceConfig::load() doesn't read the id node | DeviceConfig |
| 7 | Added bookmark file recovery sequence description | Config Files |
| 8 | Added printXML() debugging utility note | XML Serialization |

---

## Session 59: Configuration System Documentation Verification

**Date:** 2026-08-05
**Model:** opencode/deepseek-v4-flash-free

- Verified `docs/design/configuration-system.md` against source (AppConfig, SessionMgr, DataTree, AppFrame::OnClose, TuningCanvas, CubicSDR CLI parsing)
- Confirmed the document was largely accurate; applied 4 documentation corrections

### Files Modified

| File | Action |
|------|--------|
| `docs/design/configuration-system.md` | Session version encoding example, perf_mode load note, recording defaults, reset() no-op note |

| # | Fix | Section |
|---|-----|---------|
| 1 | Showed `<version>` in its percent-encoded form (`%30%2e%32%2e%38`) matching the DATA_WSTRING wsEncode output; updated the explanatory note | Session File Format |
| 2 | Clarified that any unrecognized `perf_mode` value (not just a missing node) results in `PERF_NORMAL` | Save/Load Lifecycle |
| 3 | Added Default column for recording settings (`recordingSquelchOption`=0, `recordingFileTimeLimitSeconds`=0) | Recording Settings |
| 4 | Noted `AppConfig::reset()` is a no-op stub that is not currently invoked | Save/Load Lifecycle |

## Session 60: Configuration System Documentation Corrections

**Date:** 2026-08-05
**Model:** opencode/deepseek-v4-flash-free

- Re-verified `docs/design/configuration-system.md` against source (AppConfig, BookmarkMgr, CubicSDR.cpp, TuningCanvas, SoapySDRThread)
- Confirmed no substantive errors; applied 3 documentation corrections

### Files Modified

| File | Action |
|------|--------|
| `docs/design/configuration-system.md` | Bookmark failedload nuance, full PPM chain, named-config silent early return |

| # | Fix | Section |
|---|-----|---------|
| 1 | Clarified `.failedload`/`.lastloaded` copies only occur on the initial `backup=true` load, and `.failedload` only for entry-parse (not XML-load) failures | Config Files |
| 2 | Completed PPM chain: `TuningCanvas` → `CubicSDR::setPPM()` → `SDRThread::setPPM()` → `DeviceConfig::setPPM()` | Config Save Triggers |
| 3 | Documented that `load()` returns `true` (leaving defaults) when neither named nor base config exists, or the copy fails | Save/Load Lifecycle |

## Session: Configuration System Doc Review

Model: deepseek-v4-flash-free, 2026-08-05

Verified `docs/design/configuration-system.md` line-by-line against `AppConfig.h`/`.cpp`, `SessionMgr.cpp`, and `DataTree.h`/`.cpp`. Confirmed the document was accurate (all defaults, config keys, atomic types, XML structures, and DataTree type list matched). Applied four documentation corrections:

| # | Fix | Section |
|---|-----|---------|
| 1 | Trimmed bookmark-recovery paragraph to a cross-reference, removing duplication already covered in `bookmark-system.md` | Config Files |
| 2 | Noted `save()` returns `false` and logs an error when the file cannot be written | Save Lifecycle |
| 3 | Noted `load()` returns `false` when the config file exists but is not readable | Load Lifecycle |
| 4 | Documented conditional device-node writes (non-empty containers only), the NaN gain skip on load, and the zero-to-default fallback for `rigModel`/`rigRate` | DeviceConfig / Hamlib |

## Session: Configuration System Doc Corrections Round 2

**Date:** 2026-08-05
**Model:** opencode/deepseek-v4-flash-free

- Re-verified the whole document against source; confirmed accuracy of all prior claims
- No factual errors; applied additional completeness notes to the XML Serialization section

### Files Modified

| File | Action |
|------|--------|
| `docs/design/configuration-system.md` | Documented the `@`-attribute (BadgerFish) convention, string-vector `<str>` serialization, and empty-name→`node` element fallback in `DataTree` |

---

## Session: Design Doc Reorganization — PRIORITY Merge and Reorg Plan

**Date:** 2026-08-05
**Model:** opencode/deepseek-v4-flash-free

Reviewed all 9 design docs plus PLAN/PRIORITY/RECOMMENDATIONS. Found `PRIORITY.md` duplicating `PLAN.md` (code-quality table + execution order), the CMake-mismatch table duplicated between `design/README.md` and `PRIORITY.md`, and cross-doc duplication (ReBuffer in signal-flow+threading, VisualProcessor pipeline in threading+visual-architecture, canvas OnIdle in both). Per user approval, executed items #1 and #2 and wrote a reorg plan for #3A + the visual split.

### Files Modified

| File | Action |
|------|--------|
| `docs/PLAN.md` | Added "Source Fixes & Documentation" section (items 1-6 from PRIORITY), registered reorganize-design-docs plan in the plans table |
| `docs/PRIORITY.md` | Deleted (items folded into PLAN; no remaining references) |
| `docs/design/README.md` | Removed CMakeLists source/header mismatch table (now single-sourced in PLAN) |
| `docs/plans/reorganize-design-docs.md` | Created: owner-per-module consolidation (#3A) and visual-architecture render/data split |

## Session: Design Doc Reorganization — Plan Self-Containment Check

**Date:** 2026-08-05
**Model:** opencode/deepseek-v4-flash-free

Re-audited the reorg plan against the source docs via grep. Found it was incomplete for a future
agent: ReBuffer actually appears in three docs (signal-flow, threading, audio-subsystem), not two;
VisualProcessor internals are restated across threading Pattern 6/7 and visual-architecture; and the
`busy_run`/`busy_update` thread-safety and OnIdle pull model were duplicated with no owner decision.
Rewrote `docs/plans/reorganize-design-docs.md` to be fully self-contained with a verified duplication
audit table, explicit owner boundaries, a section-by-section split map for both new visual files,
index-update steps, out-of-scope items, and grep-based verification criteria.

### Files Modified

| File | Action |
|------|--------|
| `docs/plans/reorganize-design-docs.md` | Rewrote with duplication audit, owner boundaries, section-level split map, verification criteria |

## Session: Reorg Plan — Accuracy Review and Softening

**Date:** 2026-08-05
**Model:** opencode/deepseek-v4-flash-free

Re-verified the reorganize-design-docs plan against every design doc. Confirmed accuracy (ReBuffer in
signal-flow+threading+audio; VisualProcessor in threading+visual-architecture; canvas pull in
threading+visual-architecture; only README.md and PLAN.md link the to-be-deleted file). Applied
corrections to the plan: resolved the Overview/data-path diagram to single ownership
(visual-data-pipeline.md owns the topology diagram; visual-rendering.md keeps overview prose and links);
assigned the `ReBuffer`/`SpinMutex` thread-safety row to `signal-flow.md` rather than the pipeline doc;
clarified the "SpinMutex / atomically-just" wording; fixed a duplicate "GL state" typo; and added the
plan to PLAN.md's Recommended Execution Order (it was listed among plans but missing from the order).

### Files Modified

| File | Action |
|------|--------|
| `docs/plans/reorganize-design-docs.md` | Clarified diagram and SpinMutex row ownership, fixed wording/typo, noted PLAN.md execution-order gap |
| `docs/PLAN.md` | Added "Reorganize Design Docs" as step 7 in Recommended Execution Order (renumbered 7-10 to 8-11) |

## Session: Reorg Plan — Verification and Corrections

**Date:** 2026-08-05
**Model:** opencode/deepseek-v4-flash-free

Re-verified the reorganize-design-docs plan against the actual docs. Confirmed the audit table rows,
section split coverage, and size claim; found two issues and applied corrections: (1) the "Update
Indexes" step claimed the plan was missing from PLAN.md's execution order, but PLAN.md already lists it
in the plans table and as step 7 — removed that stale instruction; (2) the audit under-counted
`signal-flow.md`, whose "Visual Processing Pipeline" restates the visual topology diagram,
`FFTDataDistributor` rate-limiting, and the canvas pull model — added it as a duplicate source (rows 2
and 3) and upgraded its handling from "add a cross-link" to "trim to cross-links" in the Update Indexes
and Files sections.

### Files Modified

| File | Action |
|------|--------|
| `docs/plans/reorganize-design-docs.md` | Corrected stale PLAN.md execution-order claim; added signal-flow.md as duplicate source and changed its handling to a trim-to-cross-links step |

---

## Session: Reorg Plan — Correction of Remaining Owner-Boundary Inconsistencies

**Date:** 2026-08-06
**Model:** opencode/deepseek-v4-flash-free

Re-verified the reorganize-design-docs plan against every design doc (audit rows, section split map,
index claims, dead-link check). Confirmed the plan was accurate and complete, then applied three
owner-boundary clarifications: (1) removed the "lists which canvases poll in OnIdle vs OnPaint" step
from threading.md's "wxWidgets Integration", which contradicted visual-rendering.md's single ownership
of that per-canvas table; (2) noted that the two generic Thread Safety Summary rows
(`ThreadBlockingQueue`, `std::shared_ptr`) are already owned by threading Pattern 1 and are not
restated after the split; (3) clarified that threading's Synchronization table remains the canonical
lock inventory for `SpinMutex` → ReBuffer, with signal-flow mentioning SpinMutex only as a one-line
pool-mechanics note.

### Files Modified

| File | Action |
|------|--------|
| `docs/plans/reorganize-design-docs.md` | Step 1: threading drops the per-canvas OnIdle/OnPaint enumeration; added note on generic Thread Safety rows; clarified `SpinMutex`/ReBuffer canonical ownership. Files table: threading row updated to match |

---

## Session: Reorg Plan — Disambiguation Edits

**Date:** 2026-08-06
**Model:** opencode/deepseek-v4-flash-free

Re-verified the reorganize-design-docs plan against all design docs and source (ReBuffer constants,
SpinMutex lock, audit rows, section split, index claims, dead-link check) and applied four small edits
to the plan doc: (1) size claim "roughly 2-5x" instead of "2-4x"; (2) noted `cache_busy` SpinMutex stays
in visual-rendering.md as component-internal detail (threading lock inventory unchanged); (3) recorded
that `SpectrumVisualData`/`ScopeRenderData` in signal-flow.md's Data Types table stay there as
data-path artifacts (visual-data-pipeline.md references by name); (4) verification note that
`AGENT-LOG.md` references the old filename as history and should be left untouched.

### Files Modified

| File | Action |
|------|--------|
| `docs/plans/reorganize-design-docs.md` | Wording/disambiguation: size claim, GL concurrency lock note, Data Types ownership note, AGENT-LOG.md exclusion note |

## Session: Reorg Plan — Implementation Readiness Prep

Independent verification of the `reorganize-design-docs.md` plan against all current design docs
(signal-flow, threading, visual-architecture, audio-subsystem, design/README, PLAN). Confirmed every
audit row, the section-split boundaries, the index/dead-link claims (only `design/README.md` and
`docs/PLAN.md` link the to-be-deleted `visual-architecture.md`; AGENT-LOG.md references are historical),
and the audio/ReBuffer ownership notes. Because a separate agent session will perform the final
implementation, added a verbatim **Source-to-Destination Map** section to the plan mapping every
`visual-architecture.md` heading to its destination file (rendering vs data-pipeline, plus the
Thread-Safety-rows reassignment), so the split is mechanical and nothing is dropped or misassigned.
Also disambiguated the `signal-flow.md` SpinMutex phrasing (it is an end-state instruction, not current
content). No changes were made to any design document.

### Files Modified

| File | Action |
|------|--------|
| `docs/plans/reorganize-design-docs.md` | Added "Source-to-Destination Mapping" table and inline split reconciliation notes; clarified SpinMutex end-state phrasing |
