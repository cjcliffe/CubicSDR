## Goal

Repaint static GL controls on interaction or state change only, instead of
every frame tick.

## Success Criteria

- Every control passes the interaction matrix below with no stale or laggy
  rendering; idle CPU drops; C++ and build suites stay green.

## Approach

Keep calling every `frameUpdate` (the polls are trivial) but make each
`Refresh()` conditional, per canvas:

- *MeterCanvas:* setters already self-refresh, covering the per-tick live
  signal/gain updates; add `Refresh()` to mouse-move/wheel/drag handlers,
  which today rely on the pump for the hover indicator and slider motion.
- *TuningCanvas:* `OnPaint` reads live app state, so external tunes have no
  event path — replace the unconditional `Refresh()` with change-detect over
  the painted values while keeping the drag stepping, and add `Refresh()` to
  mouse-move for hover.
- *GainCanvas:* drop the leading unconditional `Refresh()`; the change-gated
  refresh and gain polling stay as-is; confirm slider drags repaint via their
  own handlers.
- *ModeSelectorCanvas:* already gated to pulse-or-hover; add `Refresh()` to
  mouse-move and drop the hover branch so hovered buttons repaint at event
  rate instead of tick rate.
- Animated canvases (waterfall, spectrum, scope, demod views) and live-meter
  setter traffic: untouched.

## Steps

1. Branch per the Fix Synopsis convention; convert per canvas in the order
   meters, tuner, gains, mode selector; build.
2. Write the synopsis; commit, merge, and retire on request.

## Validation Plan

- Full build plus C++ suite and `test_build.py` green (wx/GL-bound, no unit test).
- Live interaction matrix: hover, click, drag, and wheel every button, meter,
  tuner digit, and gain slider; external tune via nudge keys, bookmark goto,
  and demod switch (tuner must follow); record button must breathe while
  recording and stop after; signal meter must dance with live levels. Any
  stale control fails the unit.
- CPU spot-check at idle and while streaming versus the pre-change build
  (expect idle to drop; streaming to hold or drop).

## Risks / Open Questions

- Risk: a missed refresh path leaves a control stale — contained by the
  interaction matrix, and every change is a revertible few lines.
- Open questions: none.

## Outcome

Implemented as planned (+45/−6 across six files). Suites green. A brace
dropped during the hover-branch removal was caught by the build and repaired
before verification. Thorough live interaction matrix passed with no stale
controls.
