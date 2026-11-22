# Raylib Prep: Decouple Core from wxWidgets

## Goal

Decouple the portable core (SDR, demod, DSP, audio, bookmarks, config) from wxWidgets behind stable seams, so the future Raylib/ImGui port replaces only the UI shell. Work lands as step-branches merged back to the `raylib-prep` epic branch. No behavior change; no UI rewrite; property-grid replacement is explicitly out of scope (an ImGui-native design happens at port time, not a grid duplication).

## Success Criteria

- No direct `wx/` includes remain in the core layers (`src/sdr/`, `src/demod/`, `src/process/`, `src/audio/`, `src/rig/`), verified by include audit. Transitive wx headers via `AppConfig.h` persist until the step 2 header diet.
- Bookmark, session, and config logic is wx-free and reads/writes byte-compatible `config.xml` / `bookmarks.xml` files; the new portable helpers are unit-tested and manager round-trips are covered live.
- Every shortcut/menu action is invocable without a `wxEvent`, and the frame tick runs as plain `update(dt)` / `render()` entry points.
- File dialogs, clipboard, and message popups each go through one wrapper function.
- Full build is green, the unit suite passes, and behavior is confirmed live on Windows with a device attached.

## Approach

Seams first, behavior frozen. Each step introduces a non-wx interface (or removes a wx dependency) while the wx shell keeps working exactly as before — the wx side becomes just one implementation of seams the Raylib side will later reimplement. Prior research established the seam points: core touches wx only via `wxGetApp()` plus a small number of `wxString` conversions; `BookmarkMgr` needs only a `wxArrayString` swap; `AppConfig` needs only path-helper and popup isolation; `_()` i18n is confined to `AppFrame.cpp`.

## Steps

0. **Epic setup.** Save this plan to `docs/plans/raylib-prep.md` (unindexed fix-synopsis; retired at merge) and cut epic branch `raylib-prep` from master.
1. **`wxGetApp()` seam.** Define a plain `AppServices` interface (config, demod/bookmark managers, frequency/rate accessors, thread-notify calls, view/window queries); the wx app object implements it; core call sites switch to the interface. Thread→UI notifies stay as direct calls (verified: they already update state directly rather than posting wx events) and keep their exact bodies behind the interface. Convert the remaining core `wxString` uses to std UTF-8 helpers. Step-branch, merged to epic.
2. **Data-layer portability.** `BookmarkMgr` and `SessionMgr`: drop wx types, route view callbacks and manager access through `AppServices`. `AppConfig`: plain-int window geometry, path resolution behind one seam call, message popup behind one seam call; wx-free header and implementation; schema frozen (files unchanged). A new portable `FileUtil` carries the file operations with unit tests; manager-level load/save tests stay out of the headless suite (the demod stack needs DSP/SDR libraries) and are covered live instead. Step-branch, merged to epic.
3. **Actions and tick.** Centralize the shortcut→action map as data; action bodies take no `wxEvent`; frame tick exposed as `update(dt)` / `render()` with no `wxTimerEvent` types leaking into canvas code. Step-branch, merged to epic.
4. **Native-service wrappers.** One helper each for file dialog, clipboard, and message popup; migrate all call sites (recording path, console copy, config errors). Step-branch, merged to epic.
5. **Verification sweep and docs.** Include audit across core, full module build, whole suite, live device check (attach/stream/detach, enumeration messages, recording-path flow); update affected design docs; retire the plan file at merge.

## Validation Plan

- Per step: `python build.py` (fast module set), `cubicsdr_tests.exe` plus any new focused suites, and a live smoke check of the touched surface.
- Final: include audit proving zero `wx/` includes in core; full-module build; whole suite green; live checklist (device enumeration messages, stream start/stop, bookmark load/save, shortcut actions, recording-path prompt).
- Highest-risk check: thread-notify parity — device attach/detach and enumeration progress must behave exactly as before, since the accessor swap touches every device-event call site; the notify bodies themselves are preserved verbatim.

## Risks

- Risk: accessor-swap typos or dropped device events. Mitigation: the interface preserves existing call order and payloads; live device testing per step 1.
- Risk: scope creep into rewriting UI files. Guard: UI/shell files change only at seam call sites; forms, sizers, `GLFont`, and canvas GL code are untouched.
