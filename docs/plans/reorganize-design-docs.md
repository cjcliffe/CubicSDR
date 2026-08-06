# Plan: Reorganize Design Documentation (Owner-per-Module + Visual Split)

See also: [PLAN.md](../PLAN.md) | [Architecture Overview](../design/README.md)

## Motivation

The design docs under `docs/design/` contain duplicated content across documents. The same concepts
(ReBuffer pool, VisualProcessor pipeline, canvas pull behavior) are described from multiple angles,
so the copies can drift apart as the code changes. `visual-architecture.md` is also the largest
deep-dive doc (roughly 2-5x the other docs) and mixes rendering content with data-pipeline content.

This plan establishes a single **owner-per-module** model and splits `visual-architecture.md` at the
render/data boundary. Once complete, each concept appears in exactly one doc; all others cross-reference
it rather than restating it.

## Target End State

| Document | Owner (describes this) |
|----------|------------------------|
| `signal-flow.md` | End-to-end data path, queues, buffer management, and the `ReBuffer` pool |
| `threading.md` | Thread lifecycle, synchronization mechanisms, producer-consumer **patterns** (the threading *model*, not component internals) |
| `audio-subsystem.md` | Controller/bound audio mixing `audioCallback`, and audio-specific buffer usage |
| `visual-rendering.md` (new) | Rendering/UI only: canvas hierarchy, GL contexts, `GLPanel`, `GLFont`, `ColorTheme`, rendering flow, mouse/key interaction |
| `visual-data-pipeline.md` (new) | Visual data processing: `VisualProcessor`, distributors, `FFTDataDistributor`, `SpectrumVisualProcessor`, `ScopeVisualProcessor`, `FFTVisualDataThread` |

`visual-architecture.md` is deleted and replaced by the two new visual files.

## Duplication Audit (must all resolve to one owner)

Findings verified in the current docs (search each term to confirm before editing):

| Topic | Where it currently appears (dup) | Decided owner |
|-------|----------------------------------|---------------|
| `ReBuffer` pool mechanics | `signal-flow.md` "Buffer Management"; `threading.md` "ReBuffer Pooling"; `audio-subsystem.md` "Buffer Management" | `signal-flow.md` |
| `VisualProcessor` / distributors / SpectrumVisualProcessor / ScopeVisualProcessor internals | `threading.md` Pattern 6, "VisualProcessor Pipeline", "busy_run"; `visual-architecture.md` "Visual Data Processing Pipeline"; `signal-flow.md` "Visual Processing Pipeline" (FFTDataDistributor rate-limiting) | new `visual-data-pipeline.md` |
| Thread-safety of visual pipeline (`busy_update` / `busy_run` locks) | `threading.md` "Synchronization Mechanisms" table; `visual-architecture.md` "Thread Safety Summary" | `threading.md` (lock inventory lives there; visual/threading may cross-link) |
| Canvas pull model (`OnIdle`/`OnPaint`) | `threading.md` "wxWidgets Integration"; `visual-architecture.md` "OnIdle Processing" + "Rendering Flow"; `signal-flow.md` "Visual Processing Pipeline" | `visual-rendering.md` |
| `ScopeVisualProcessor` | `threading.md` Pattern 7 (UI-thread); `visual-architecture.md` "ScopeVisualProcessor"; referenced as a consumer in `signal-flow.md`, `audio-subsystem.md` | `visual-data-pipeline.md` (internals); threading keeps only the threading aspect |

`audio-subsystem.md` references to visual processors are legitimate (it appears as a producer of
`ati_vis` data pushed to `audioVisOutputQueue`, consumed by `ScopeVisualProcessor`) and are NOT
duplication; leave it as-is.
`signal-flow.md`'s "Data Types" table also lists `SpectrumVisualData` and `ScopeRenderData` — these
stay in `signal-flow.md` because they are data-path artifacts (produced/consumed along the pipeline);
`visual-data-pipeline.md` should reference them by name rather than re-documenting their fields.
`signal-flow.md`'s "Visual Processing Pipeline" section, however, restates the visual topology, the
`FFTDataDistributor` rate-limiting behavior, and the canvas pull (`OnIdle` vs `OnPaint`) model — trim it
to cross-links, so those topics are owned by `visual-data-pipeline.md` / `visual-rendering.md`. See
"Update Indexes".

## Step 1: Decide owner boundaries

Before editing, confirm:

1. **ReBuffer** stays fully in `signal-flow.md` ("Buffer Management"), including GC threshold and
   `REBUFFER_WARNING_THRESHOLD`. `threading.md` "ReBuffer Pooling" is reduced to a short pointer that
   keeps only the thread-specific note already present in its "Synchronization Mechanisms" table
   (`SpinMutex` is listed as the lock used by `ReBuffer`) and links to `signal-flow.md` for the pool
   mechanics. `audio-subsystem.md` "Buffer Management" is reduced to the audio-specific use
   (`ReBuffer<AudioThreadInput>` by `DemodulatorThread`) and links to `signal-flow.md`.
   The `SpinMutex`/ReBuffer-pool allocation row from `visual-architecture.md`'s "Thread Safety Summary"
   is absorbed here (in `signal-flow.md`'s ReBuffer section), **not** into `visual-data-pipeline.md`.
   `threading.md`'s "Synchronization Mechanisms" table remains the canonical lock inventory (its
   `SpinMutex` → ReBuffer row is retained); `signal-flow.md` should mention SpinMutex only as a
   one-line part of the pool mechanics, not as a restated lock table.
2. **Visual processor internals** live only in `visual-data-pipeline.md`. `threading.md` keeps the
   threading-model aspect (Patterns 6 and 7: dedicated-thread vs UI-thread distribution, the data path
   `DemodulatorThread → pipeAudioVisualData → ScopeVisualProcessor::process() → ScopeCanvas`) but not the
   per-component processing detail. The `busy_run`/`busy_update` lock entries stay in threading's
   "Synchronization Mechanisms" table (canonical); `visual-data-pipeline.md` references them by name.
   Concretely, `threading.md`'s "VisualProcessor Pipeline" and "SpectrumVisualProcessor busy_run"
   subsections are trimmed to a threading-model statement plus the lock-inventory pointer; the
   two-phase-lock algorithm narrative (`process()` internals) moves to `visual-data-pipeline.md`.
3. **Canvas pull behavior** lives in `visual-rendering.md` (OnIdle Processing, Rendering Flow, Data
   Flow Timing, per-canvas paint/OnIdle tables). `threading.md` "wxWidgets Integration" keeps only the
   UI-thread-poll statement and links to rendering; it does **not** enumerate which canvases poll in
   OnIdle vs OnPaint, since that per-canvas table is owned by `visual-rendering.md` "OnIdle Processing".
4. The two generic rows of `visual-architecture.md`'s "Thread Safety Summary" — `ThreadBlockingQueue`
   and `std::shared_ptr` — are not restated after the split: both are already owned by `threading.md`
   (Pattern 1, "Queue-Based Data Flow"), so the remaining visual docs reference them only by name.

## Step 2: Split `visual-architecture.md`

Split `visual-architecture.md` into two files by section, then delete the original:

### `visual-rendering.md` (rendering/UI half)

- Overview **prose**: the pull-based model (canvases poll their queues), canvas consumer roles, and how the canvases fit the data path. The pipeline-topology diagram is **not** reproduced here — it is owned by `visual-data-pipeline.md` (its intro diagram); this doc links to it
- Canvas Class Hierarchy + InteractiveCanvas
- WaterfallCanvas, SpectrumCanvas, ScopeCanvas (behavior + drag/key interactions)
- GLPanel System (fill types, coord systems, transform pipeline, hit-testing)
- PrimaryGLContext (drawing primitives, GL state, blend modes)
- ScopeContext, MeterContext, TuningContext, ModeSelectorContext, UITestContext
- GLFont system (font sizes, scaling, drawer, string caching, thread safety; the internal `cache_busy`
  SpinMutex stays here as component-internal detail — `threading.md`'s lock inventory is not extended)
- ColorTheme system and ThemeMgr
- Hover Alpha (hover indicator transparency, `hoverAlpha` animation)
- Rendering Flow (per-frame OnPaint, OnIdle Processing, paint flows; Data Flow Timing cross-links `visual-data-pipeline.md` for pipeline internals rather than restating them)
- Mouse Interaction (MouseTracker, event propagation, waterfall drag/zoom state machines, keyboard nav)
- Key Constants for canvases

### `visual-data-pipeline.md` (processing)
- Intro diagram: the top-level visual topology diagram from the former Overview (SDRPostThread/
  DemodulatorThread → queues → SpectrumVisualDataThread/FFTVisualDataThread/ScopeVisualProcessor →
  canvases) — **owned here** as the single canonical home of the pipeline topology, so it is not
  restated in `visual-rendering.md`
- VisualProcessor template (`setInput`/`attachOutput`/`run`/`distribute`/backpressure semantics)
- VisualDataDistributor vs VisualDataReDistributor (zero-copy vs deep-copy)
- FFTDataDistributor (rate limiting, batching, non-blocking distribute)
- SpectrumVisualProcessor full per-frame pipeline (resampling, FFT, smoothing formulas, floor/ceiling, constants)
- FFTVisualDataThread (glue thread loop)
- ScopeVisualProcessor (display modes, non-blocking try_pop)
- Thread Safety summary limited to the pipeline's own locks (`busy_update`, `busy_run`); cross-link
  `threading.md` for the canonical lock table and `signal-flow.md` for the `ReBuffer`/`SpinMutex` pool
  entry (not restated here)

### Source-to-Destination Map (for the split)

Every section of the current `visual-architecture.md` (by its heading in that file) maps to exactly
one destination. Move each section verbatim, then excise only the cross-doc duplication noted in
"Step 1" and the inline edits called out below:

| `visual-architecture.md` section | Destination |
|----------------------------------|-------------|
| `Overview` — prose (pull-based model, canvas consumer roles, how canvases fit the data path) | `visual-rendering.md` |
| `Overview` — pipeline topology diagram (SDRPostThread/DemodulatorThread → queues → threads → canvases) | `visual-data-pipeline.md` (intro) |
| `Canvas Class Hierarchy` + `InteractiveCanvas` / `WaterfallCanvas` / `SpectrumCanvas` / `ScopeCanvas` | `visual-rendering.md` |
| `GLPanel System` (Base Class Properties, Fill Types, Coordinate Systems, Transform Pipeline `calcTransform()`+`draw()`, Hit-Testing) | `visual-rendering.md` |
| `PrimaryGLContext` (Drawing Methods, GL State Management, Buffer Strategy) | `visual-rendering.md` |
| `ScopeContext Extensions`, `MeterContext`, `TuningContext`, `ModeSelectorContext`, `UITestContext` | `visual-rendering.md` |
| `Hover Alpha` | `visual-rendering.md` |
| `GLFont System` (Font Sizes, Font Scaling, Drawer Proxy, String Caching, Thread Safety) | `visual-rendering.md` |
| `ColorTheme System` (Theme Structure, Available Themes, ThemeMgr) | `visual-rendering.md` |
| `Visual Data Processing Pipeline` — `VisualProcessor Template`, `Distribution Modes`, `VisualDataDistributor`/`ReDistributor`, `FFTDataDistributor`, `SpectrumVisualProcessor`, `FFTVisualDataThread`, `ScopeVisualProcessor` | `visual-data-pipeline.md` |
| `Thread Safety Summary` — `busy_update` / `busy_run` rows | `visual-data-pipeline.md` (pipeline lock summary) |
| `Thread Safety Summary` — `SpinMutex` → ReBuffer row | `signal-flow.md` (absorbed into the ReBuffer section) |
| `Thread Safety Summary` — `ThreadBlockingQueue` and `std::shared_ptr` rows | drop (already owned by `threading.md`) |
| `Rendering Flow` (Per-Frame Rendering, OnIdle Processing, WaterfallCanvas / ScopeCanvas Paint Flow, Data Flow Timing) | `visual-rendering.md` |
| `Mouse Interaction` (MouseTracker, Event Propagation, Drag State Machine, Zoom State Machine, Keyboard Navigation, Spectrum/Scope interactions) | `visual-rendering.md` |
| `Key Constants` | `visual-rendering.md` |

During the mechanical split, reconcile these plan-level decisions inline:

- In `visual-rendering.md`, trim the "Rendering Flow" contents so per-frame "Data Flow Timing" cross-links
  `visual-data-pipeline.md` for pipeline internals instead of restating them.
- In `visual-data-pipeline.md`, set `ScopeVisualProcessor`'s threading aspect (UI-thread vs dedicated
  thread) as a cross-link to `threading.md` Pattern 7; keep only the processing detail here.
- In `visual-data-pipeline.md`, the `Thread Safety summary` holds only `busy_update`/`busy_run`;
  do not restate the canonical lock table (owned by `threading.md`) or the ReBuffer/SpinMutex pool
  (owned by `signal-flow.md`).

Both new files follow the documentation guidelines in AGENTS.md (spec headings = the ones the file
describes). The design docs do not currently use a "See also" header; add cross-links inline wherever
content is owned by another doc, consistent with the owner-per-module model.

## Update Indexes

- `docs/design/README.md`: replace the single "Visual Architecture" entry with two entries
  (`visual-rendering.md` and `visual-data-pipeline.md`).
- `docs/PLAN.md`: replace the "Visual Architecture" architecture listing with the same two entries.
  (This plan is already listed in the "Plans" table and the "Recommended Execution Order", so no
  further index change is needed there.)
- `signal-flow.md`: trim the "Visual Processing Pipeline" section to cross-links instead of restating
  content — point to `visual-data-pipeline.md` for the topology diagram and `FFTDataDistributor`
  internals, and to `visual-rendering.md` for the canvas pull (`OnIdle` vs `OnPaint`) behavior.

## Out of Scope (do not do here)

- Moving "setter exists but is never called" notes / bug-object findings out of the design docs —
  separate task.
- Renaming remaining files to a uniform `-system` suffix.
- Changing any product code.

## Verification Criteria

- `grep -n "ReBuffer"` shows ReBuffer described fully in `signal-flow.md` only; other references are
  cross-links or mentions, not restatements.
- `grep -n "VisualProcessor"` shows component internals only in `visual-data-pipeline.md`; the other
  files only link or describe threading behavior.
- `visual-rendering.md` and `visual-data-pipeline.md` together cover everything formerly in
  `visual-architecture.md`; neither restates content owned by another doc. Note: `visual-rendering.md`
  will remain the largest deep-dive (the bulk of the source file is rendering/UI content) — this is
  expected and acceptable, as the goal is single ownership, not equal file sizes.
- `docs/design/README.md` and `docs/PLAN.md` list both new files; no link is dead.
- No links to the deleted `design/visual-architecture.md` remain anywhere in `docs/`. Note:
  `AGENT-LOG.md` (project root) references the old filename as historical record — leave it untouched.

## Files

| File | Action |
|------|--------|
| `docs/design/visual-rendering.md` | Create (rendering/UI half of `visual-architecture.md`) |
| `docs/design/visual-data-pipeline.md` | Create (processing half of `visual-architecture.md`) |
| `docs/design/visual-architecture.md` | Delete (after moving both halves) |
| `docs/design/signal-flow.md` | Keep ReBuffer as owner; trim "Visual Processing Pipeline" to cross-links (topology/distributor internals → `visual-data-pipeline.md`, canvas pull → `visual-rendering.md`) |
| `docs/design/threading.md` | Remove ReBuffer restatement; reduce Pattern 6 / "VisualProcessor Pipeline" / "SpectrumVisualProcessor busy_run" subsections to the threading model and lock-inventory pointer (two-phase-lock narrative moves to `visual-data-pipeline.md`); keep sync table canonical; drop the per-canvas OnIdle/OnPaint enumeration from "wxWidgets Integration" (owned by `visual-rendering.md`) |
| `docs/design/audio-subsystem.md` | Trim ReBuffer restatement, keep audio-specific usage |
| `docs/design/README.md` | Reindex the two split visual docs |
| `docs/PLAN.md` | Replace the single visual entry with the two new docs |