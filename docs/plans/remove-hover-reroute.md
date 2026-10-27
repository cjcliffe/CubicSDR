# Plan: Remove Hover Re-route of Demodulator Visuals

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md) | [Threading Model](../design/threading.md)

## Goal

Remove the behavior where hovering a demodulator temporarily re-routes visuals and GUI context to it. After this change, only the explicitly selected demodulator drives the side panels, dialogs, keyboard shortcuts, rig follow logic, and the audio-visual queue. Hover may still affect cursor, status text, and a local highlight, but it must not change any manager state.

## Current State

In `DemodulatorMgr`:

- Three related members exist: `activeContextModem` (hover-or-selection), `currentModem` (selection), and `activeVisualDemodulator` (visual-queue target).
- `setActiveDemodulator()` takes a `temporary` flag. Temporary calls (the default) update `activeContextModem` and re-point the visual output queue. Non-temporary calls additionally promote context state into `currentModem`, refresh last-known modem settings, and notify bookmarks and rig control.
- `getActiveContextModem()` returns the hover target with fallback to the selection. `getCurrentModem()` returns the selection.
- `setActiveDemodulatorByRawPointer()` exists for internal callers that only hold a raw pointer.

Hover producers:

- `WaterfallCanvas::updateHoverState()` clears the context modem on every mouse move and re-sets it to the nearest demodulator under the cursor. `OnMouseLeftWindow()` clears it. `OnMouseDown()` commits the hovered target as the selection. `OnMouseReleased()` creates or moves demodulators and commits selection.
- `TuningCanvas` hover handling sets the context modem to the current selection while hovering frequency or bandwidth digits, and clears it otherwise. `OnMouseLeftWindow()` also clears it.
- Related null-reset and commit call pairs exist in `BookmarkView`, `BookmarkMgr::removeActive()`, `SessionMgr`, `WaterfallCanvas` mouse handlers, and `AppFrame` TAB cycling.

Hover consumers (things that change just because the mouse moved):

- Waterfall and spectrum rendering branches in `WaterfallCanvas` and `SpectrumCanvas`, including which demodulator gets the hover color and when the new-demod frequency selector appears.
- `PrimaryGLContext::DrawFreqSelector()`, which sizes the selector from the selection but is gated by hover state in callers.
- `SDRPostThread::runDemodChannels()`, which picks the visual channel from the selection today; hover currently perturbs this indirectly through queue re-routing in the manager.
- `CubicSDR::showFrequencyInput()` and `showLabelInput()`, which pick dialog titles and targets from the context modem.
- `AppFrame::OnGlobalKeyUp()`, `toggleActiveDemodRecording()`, waterfall `OnKeyDown()` delete and center handlers, and `RigThread` follow logic, which prefer the context modem over the selection.
- Meanwhile `AppFrame::OnIdle()` panel handlers, `DemodulatorInstance::setFrequency()`, `DemodulatorThread` audio gating, and `PrimaryGLContext` solo highlighting already use `getCurrentModem()`, so panels and hover can disagree today.

The observable confusion: moving the mouse across the waterfall silently switches which demodulator the meters, dialogs, and keyboard shortcuts act on, and `AppFrame::isUserDemodBusy()` has to treat context/selection disagreement as a special busy state.

## Desired State

- `DemodulatorMgr` holds a single selection, exposed as `getCurrentModem()` and mutated by `setActiveDemodulator()` with no temporary flag.
- The visual output queue always follows the selection. Hover never touches the manager.
- Each canvas keeps its own transient hover reference as a local variable or member, used only for cursor shape, status text, and highlight tint.
- All former `getActiveContextModem()` readers use `getCurrentModem()`, except canvas render code which uses selection plus its local hover reference.

## Key Decisions

- Remove the `temporary` parameter outright rather than deprecating it, since this branch owns the cleanup and every call site is being edited anyway.
- Remove `getActiveContextModem()`, `setActiveDemodulatorByRawPointer()`, `activeContextModem`, and `activeVisualDemodulator` outright. Keep `getCurrentModem()` as the single accessor so the intent at each reader is unambiguous.
- Preserve a hover affordance (cursor plus highlight) but implement it without manager state. Recommended because deleting all hover feedback would make bandwidth-edge and frequency dragging undiscoverable; the confusion comes from re-routing, not from highlighting.
- Treat solo-mode squelch-break selection stealing in `DemodulatorThread` as an explicit selection change, not hover. It keeps its behavior but goes through the single-arg setter.

## Implementation Plan

### Phase 1: Collapse `DemodulatorMgr` to a single selection

In `src/demod/DemodulatorMgr.h` and `src/demod/DemodulatorMgr.cpp`:

- Change the setter signature to `setActiveDemodulator(const DemodulatorInstancePtr& demod)` and delete the `temporary` parameter, `getActiveContextModem()`, and `setActiveDemodulatorByRawPointer()`. Callers that only hold a raw pointer (`DemodulatorThread` solo-mode squelch break) do an inline lookup over `getDemodulators()` matching `.get()` instead of using a manager raw-pointer helper.
- Delete the `activeContextModem` and `activeVisualDemodulator` members. Keep `currentModem` and the existing last-state fields (`lastBandwidth`, `lastDemodType`, gain, squelch, mute, delta lock, modem settings).
- Rewrite the setter as an explicit selection commit: detach the visual queue from the previous selection via `setVisualOutputQueue(nullptr)`, assign `currentModem`, attach the queue to the new selection when non-null and leave it detached when null, then run the existing commit side effects (`updateLastState()`, bookmark active-list refresh, rig follow update). Passing `nullptr` therefore means deselect, replacing the old temporary-clear idiom whose null case silently fell back to the selection.
- Simplify `updateLastState()` so it only validates and snapshots `currentModem`; remove the context-to-current promotion branch.
- Simplify `deleteThread()` to clear `currentModem` when the deleted instance is selected; remove context and visual-member cleanup.
- Update `getPreviousDemodulator()` and `getNextDemodulator()` guards to reference the single selection.

### Phase 2: Stop hover from writing manager state

- Add a `hoverDemod` member to `WaterfallCanvas` (it currently holds no demodulator members, only drag state and cursors). `updateHoverState()` computes the nearest demodulator under the cursor into `hoverDemod` and uses it only for drag-state selection (`WF_DRAG_FREQUENCY`, `WF_DRAG_BANDWIDTH_LEFT/RIGHT`, `WF_DRAG_RANGE`, `WF_DRAG_NONE`), cursor shape, and status text; it never touches the manager. A member (rather than a recomputed local) keeps render, `OnMouseDown`, `OnMouseMoved`, and `OnMouseReleased` consistent within a frame.
- In `WaterfallCanvas::updateHoverState()`: delete the unconditional clear call at entry and the re-set call on hover match.
- In `WaterfallCanvas::OnMouseLeftWindow()`: clear `hoverDemod`, reset cursor, and return; do not call the manager. In `TuningCanvas::OnMouseLeftWindow()`: reset `hoverIndex`/`hoverState` and cursor only.
- In `TuningCanvas::OnMouseMoved()`: delete the trailing block that sets the context modem to the selection or null based on `hoverState`; `hoverState` and `hoverIndex` already drive status text and digit highlighting, and frequency/bandwidth readouts already use `getCurrentModem()`.
- In `WaterfallCanvas::OnMouseDown()`: call `updateHoverState()` first as today, then commit once with `setActiveDemodulator(hoverDemod ? hoverDemod : getCurrentModem())` when a drag state is entered, replacing the re-commit of the context modem. This preserves current click behavior where pressing on a non-selected demodulator selects it before any drag.
- In `WaterfallCanvas::OnMouseMoved()` with the mouse down: operate drag edits on the selection (`getCurrentModem()`), which `OnMouseDown` just committed; the hover member is ignored while dragging. With no button down: refresh `hoverDemod` only.
- In `WaterfallCanvas::OnMouseReleased()`: replace the dual `demod`/`activeDemod` locals (selection plus context) with selection plus `hoverDemod`; click-move, new-demod creation, and range-drag paths each end with one explicit `setActiveDemodulator(demod)` call.
- Collapse the redundant clear-then-set pairs into single selection commits: `BookmarkView` active-select, double-click frequency/bandwidth, and tree-activate handlers call `setActiveDemodulator(target)` once; `BookmarkMgr::removeActive()` calls `setActiveDemodulator(nullptr)` once before removal (with `deleteThread()` also clearing selection as backstop); `AppFrame` TAB cycling calls `setActiveDemodulator(next)` once; `SessionMgr` load calls `setActiveDemodulator(nullptr)` once to deselect before `terminateAll()` and `setActiveDemodulator(restored)` once after.

### Phase 3: Point former context readers at the selection

Replace `getActiveContextModem()` with `getCurrentModem()` in:

- `WaterfallCanvas` key handling (delete, center) and mouse-drag frequency and bandwidth edits, which then operate on the selection committed at mouse-down.
- `SpectrumCanvas` render gating for the new-demod frequency indicator.
- `CubicSDR::showFrequencyInput()` dialog-title choice and `showLabelInput()` target. Note the intended UX change: with a selection present, the dialog always targets that demodulator even when the cursor sits over empty spectrum; the center-frequency title appears only when selection is null.
- `AppFrame::OnGlobalKeyUp()` delta-lock, mute, and record toggles; `toggleActiveDemodRecording()`; `isUserDemodBusy()`, where the context-vs-selection disagreement clause is deleted and only the mouse-down clauses remain.
- `RigThread` follow logic, which reads both the context modem and the selection today. Rewrite the center-lock comparison so it no longer compares selection against itself: when modem following is on, the rig loop acts purely on the selection, and the app-frequency sync condition drops the context side.
- `DemodulatorThread` solo-mode squelch-break stealing, routed through the single-arg setter after an inline `getDemodulators()` lookup matching the raw instance pointer. The static squelch-lock pointer itself is unchanged.

No change needed inside `PrimaryGLContext::DrawFreqSelector()` or `SDRPostThread::runDemodChannels()`: both already read `getCurrentModem()`. The related fixes are at their edges: the waterfall and spectrum callers stop gating the frequency selector on hover state, and the `SDRPostThread` empty-selection bootstrap (which currently calls the setter with the temporary default and therefore only sets hover state) becomes an explicit `setActiveDemodulator(firstInRange)` selection.

No change needed in `TuningCanvas` readouts, `DemodulatorInstance::setFrequency()` rig notification, `DemodulatorThread` audio gating, or solo highlighting: all already use the selection.

### Phase 4: Rework canvas rendering around selection plus local hover

- In `WaterfallCanvas` rendering: draw the selection with the highlight wash when steady or when hovering empty spectrum with an active selection (the click would move it, so it takes the destroy color as a move indicator); while aiming at another demodulator or SHIFT-aiming a new spot, the selection falls through to the regular outline-only draw with no wash, since that click would select or create rather than move it. Leaving the window clears `hoverDemod` and zeroes the wash alpha immediately so the selection does not flash back to full brightness while fading out. Draw `hoverDemod` with the hover color only when it differs from the selection, and draw remaining demodulators normally. When the cursor sits over empty spectrum (`hoverDemod` null, mouse in view), draw the ghost frequency selector at the cursor alongside the selection highlight, so the pending click target can be aligned against waterfall history before clicking. Delete the hover-dependent branches: the `mouseInView || context` gate, the separate context-vs-selection highlight paths, and the selector suppression that hides the selector whenever any context modem exists. The `isNew` coloring keeps the shift-key and inactive-selection conditions.
- In `SpectrumCanvas` rendering: draw the selection in `DrawDemodInfo()` with the hover color (the same yellow the ghost uses) so the active demodulator is unmistakable there at all times, reserving the destroy red for the waterfall's move indicator; show the ghost bandwidth indicator whenever the waterfall mouse is in view over empty spectrum, regardless of whether a selection exists.
- Keep `getLastBandwidth()` and `getLastDemodulatorType()` as the source for the ghost indicator dimensions.

### Phase 5: Clean up naming and dead state

- In `AppFrame`, rename the `activeDemodulator` raw-pointer cache used by `handleCurrentModem()` change detection to `lastHandledDemod` (or equivalent) so it is not confused with the removed context modem; it remains a change-detection cache only.
- Search for and delete any remaining references to `activeContextModem`, `activeVisualDemodulator`, `getActiveContextModem`, `ByRawPointer`, and the two-arg setter form.

## Validation Plan

- Build through the project driver: `python build.py --help` followed by the normal `python build.py` build; do not invoke `cmake` or `make` directly.
- Text search confirms no remaining `getActiveContextModem`, `activeContextModem`, `activeVisualDemodulator`, `setActiveDemodulatorByRawPointer`, or two-argument `setActiveDemodulator` calls.
- Manual pass with multiple demodulators: hover without clicking leaves side panels, meters, modem properties, and demod scope unchanged; hovering empty spectrum shows the ghost preview at the cursor alongside the selection highlight; click selects or moves the selection to the ghost position; dragging edges changes bandwidth of the selection; clicking empty spectrum with SHIFT creates and selects; TAB cycles selection; `M`, `V`, `E`, `C`, `D`, and SPACE act on the selection regardless of cursor position; deleting the selection clears panels and releases the visual queue.
- Solo-mode squelch-break check: with solo mode on, a squelching demodulator that breaks squelch still becomes the explicit selection.

## Risks and Rollback

- Solo-mode squelch stealing currently piggybacks on the context setter; converting it to an explicit selection commit preserves behavior but should be tested since it changes selection without a click.
- Rig follow logic currently compares context against selection; after simplification it acts purely on selection, which removes a source of disagreement but changes behavior when the cursor sits over a non-selected demodulator while follow-modem is on. Verify rig follow with and without modem following.
- Bookmark and session flows that issue clear-then-set pairs may briefly clear selection mid-refresh; collapsing them into single commits avoids flicker but each handler needs retesting.
- Rollback is to restore the plan branch to its base since the change touches the manager API and every call site at once; splitting it across commits would leave intermediate states unbuildable.

## Open Questions

- Null-selection visual queue: detach the queue on deselect (recommended; the demod scope then goes quiet with no selection) versus leaving the last selection attached. Detaching matches deselect meaning and the empty-selection panel state.
- Hover highlight: keep a local-only hover tint distinct from the selection highlight (recommended; preserves bandwidth-edge and frequency-drag affordances) versus selection-only highlighting with hover affecting cursor and status text alone.
- Solo-mode squelch-break stealing: keep the automatic selection steal on squelch break in solo mode, routed through the single-arg setter (recommended; preserves current solo behavior while making the selection explicit), versus removing the steal so squelch break only flashes the visual cue and never changes selection.
- Empty-spectrum hover with a selection present: decided — draw the ghost preview alongside the selection highlight, since it shows where the selection is about to move and lets the hotspot be aligned against waterfall history before clicking.
