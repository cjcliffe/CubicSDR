# Bookmark Active-List Throttle

## Problem

With the bookmark panel open, dragging a demodulator on the
waterfall tracked its frequency and label in the Active list
at full frame rate, hammering the tree with synchronous
repaints and making the whole UI lag until the drag ended.

## Root Cause

Each drag mouse-move calls `DemodulatorInstance::setFrequency()`
and `updateLabel()`, and both raised the immediate structural
refresh flag. Bookmark updates ride the frame tick in
`AppFrame::tickFrame()`, so every frame ran the active-list
refresh plus forced synchronous repaints of the tree, props,
and button panels. Tuning-slider drags and rig-follow took
the same path through `updateLabel()`.

## Fix

Split the dirty flag in two:

- Drag-rate producers (`setFrequency()`, `updateLabel()`) raise
  a lightweight label-only flag via
  `BookmarkMgr::updateActiveLabels()`.
- `BookmarkView::pollUpdates()` promotes the label flag to a
  real refresh at most every 500 ms, through the existing
  in-place label path. A pending structural change still
  clears both flags and refreshes immediately.
- Discrete events (selection, add/remove, recording flips,
  session load, dialog edits) keep the immediate structural
  path unchanged.

The throttle itself is a new header-only `RateGate` helper in
`src/util/`, covered by focused doctest cases.

## Files

- `src/util/RateGate.h` (new)
- `tests/test_RateGate.cpp` (new), registered in
  `tests/CMakeLists.txt`
- `src/forms/Bookmark/BookmarkView.cpp`, `.h`
- `src/BookmarkMgr.cpp`, `.h`
- `src/demod/DemodulatorInstance.cpp`

## Verification

- Build through `python build.py` stays green.
- Full doctest suite stays green, including the new
  `RateGate` cases.
- Live check on Windows: drag a demodulator on the
  waterfall with the bookmark panel open; the Active
  labels should step a few times a second with no UI
  lag, and settle on the final value when the drag ends.
- The 500 ms interval is the
  `BOOKMARK_ACTIVE_LABEL_INTERVAL_MS` constant at the top
  of `BookmarkView.cpp` if it ever needs retuning.

## Rollback

Revert the branch. The only behavioral surface is the
bookmark refresh cadence; drag-rate producers fall back to
the immediate structural flag.
