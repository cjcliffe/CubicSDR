# Remove Hover-Digit Frequency Entry

## Problem

Pressing a number key while hovering a canvas opened the
frequency dialog with the digit prefilled. Besides being
easy to trigger by accident, the path caused keyboard bugs
on Linux, up to and including the desktop losing click
interactivity after CubicSDR closed.

## Fix

- Removed both digit-entry cases: numpad digits in the
  global key-down handler and translated row digits in the
  global char handler. Digits while hovering are now inert
  (no canvas handles them; the char filter swallows them).
- Removed the now-dead prefill plumbing: the init-string
  parameter of `showFrequencyInput`, the matching
  `FrequencyDialog` constructor parameter and member, the
  `OnShow` handler and its event-table entry, the width
  calculation, and the prefill branches.
- Updated the two key-routing comments and the tuner status
  tips to drop the numeric-key mention.
- Kept: hover plus Space still opens the dialog everywhere,
  and typing (including numpad and paste) inside the dialog
  itself is untouched.

## Files

- `src/AppFrame.cpp`
- `src/CubicSDR.cpp`, `.h`
- `src/FrequencyDialog.cpp`, `.h`
- `src/visual/TuningCanvas.cpp`

## Verification

- Build through `python build.py` stays green; full doctest
  suite stays green.
- Live check: hovering canvases and pressing digits does
  nothing; Space still opens the frequency dialog; the
  dialog still accepts typed and pasted values.

## Rollback

Revert the branch. Digit entry returns as before.
