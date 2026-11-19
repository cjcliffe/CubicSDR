## Goal

Make squelch behave like a proper rig: meaningful thresholds on
every mode, no chattering, no chopping on pauses, no
noise-burst crash-through, click-free transitions.

## Root Cause

- *Meaningless measurement:* AM/DSB/LSB/USB/CW measured
  post-AGC demod audio, whose peaks normalize to 0.5 whether
  signal or noise is present — signal and noise read within
  ~1-2 dB, so no threshold separates them. (The S-meter on
  those modes read the same mush.)
- *Bare comparator:* a single threshold with no hysteresis, no
  hang time, and no open delay — chattering, chopping on
  pauses, crash-through on noise bursts.
- *Hard buffer-drop mute:* squelched buffers were dropped
  entirely, clicking on every transition.

## Approach

- *RF measurement:* squelch and meter read input IQ level for
  all modes; the `useSignalOutput` branch is gone. Saved SSB
  thresholds set against the old mush need re-setting once.
- *`SquelchState` helper:* opens when level exceeds the
  threshold continuously for 25 ms, closes after 1 s
  retriggerable hang below threshold minus adaptive
  hysteresis (2..6 dB from measured level spread: tight on
  quiet channels for weak signals, wide on noisy ones).
  Hovering in the band drains the hang timer at quarter
  rate, so straddling noise trends closed instead of riding.
  Starts closed; enable and threshold changes reset it for
  prompt feedback. Liquid/wx-free in `SquelchState.h` with
  doctest coverage. (A 250 ms hang and fixed band closed too eagerly
  once the threshold sat at proper headroom.)
- *`SquelchFade` helper:* 5 ms raised-cosine mute ramp
  replacing the hard drop, reversing cleanly mid-ramp;
  fully-silent blocks are still dropped to save CPU.
  Recordings ride the same faded buffers for free.
- *Solo focus rule:* a new break steals focus only when the
  active demodulator is silent or absent, never from a live
  stream; on close, focus waits for the next break or manual
  selection. The first-breaker lock is gone.
- *Solo Tab jump:* Tab jumps to the most recent still-breaking
  demodulator other than the current one and consumes the
  break history, so the next Tab cycles normally until a new
  break lands; with no other breaker, or outside solo mode,
  Tab cycles as before.
- *`MeterBallistics` helper:* the needle uses asymmetric
  ballistics (0.1 s attack, 0.5 s release) instead of
  slamming to each block peak. Display only; the squelch
  decision reads the raw per-block level.
- *`MeterRange` helper:* the meter scale auto-ranges by
  gliding (2 dB/s to expand, 2 dB/s to contract) instead
  of jumping to each block extreme, with a 20 dB minimum
  span, the threshold kept visible, and the slew frozen
  while the slider is dragged. Needle and marker glyphs
  peg at the scale ends instead of vanishing.
- *Right-click snap:* right-clicking the meter places the
  threshold above the visible level by the current
  hysteresis width (clamped to the scale), so the close
  edge reaches back down to the clicked noise floor.

## Validation Plan

- Full C++ suite and `test_build.py` green, Windows build with no
  new warnings. Behavior needs ears and RF: thresholds bite at
  sensible RF levels on every mode, marginal signals hold
  without chattering, pauses do not chop, noise bursts do not
  crash through, open/close transitions do not click.

## Risks / Open Questions

- Hang (1 s), open delay (25 ms), adaptive hysteresis
  (2..6 dB from spread), and fade (5 ms) are set from live
  feel; retune if pauses chop or tails drag.
- FM noise-operated squelch would be the next step for
  proper-rig FM feel; CTCSS/DCS remain out of scope.

## Outcome

Implemented as planned, pending live verification. Retire this
synopsis on merge.
