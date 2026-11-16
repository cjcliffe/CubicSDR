## Goal

A round of keyboard-handling fixes in the global key path
(`OnGlobalKeyDown`): the GH#629 delta-lock nudge fight, nudge
scope, V/M/R fallback, and Shift+M/Shift+R toggle-all parity and
scope. Plus a delta-lock offset sign fix on the demod-create path
found while verifying drag behavior.

## Root Cause (GH#629)

`gkNudge` sets the demod frequency and label directly. Every
post-thread turn enforces `freq == center + ofs` for locked demods,
clobbering the still-pending nudge before the DSP thread ever applies
it — then correcting the label back. Each keypress therefore jumps and
reverts, firing two bookmark-list refreshes along the way.

## Approach

- *Locked nudge (#629):* nudge the center by the nudge amount
  instead of touching the demod. The post thread advances the locked
  demod and its label exactly once to the new `center + ofs` — no
  fight, no flicker. Visually the spectrum slides under a fixed
  marker; the device clamp applies so nudging past the device edge
  stops cleanly. Unchanged: the normal path (demod moves, center
  jumps to keep it in data) and the stale-diff device-follow
  backstop.
- *Nudge scope:* the `OnGlobalKeyDown` dispatch passes the selection
  directly instead of preferring the hovered demodulator, so `[`/`]`
  work the same with the cursor anywhere. Hovering to nudge a
  non-selected demod is dropped as not useful.
- *V/M/R fallback:* pin, mute and record prefer the hovered
  demodulator but fall back to the selection when nothing is
  hovered, instead of doing nothing with the cursor in the
  waterfall.
- *Toggle-alls:* Shift+M mutes/unmutes and Shift+R records across
  in-range demods only, with checkbox semantics (a partial set
  completes, all-set clears). This adds mute-all alongside
  record-all and drops record-all's old arm-everything behavior,
  whose any-set polarity also flipped on distant demods.
- *Create-path lock offset:* the demod-create paths wrote the
  lock offset with a flipped sign (`center - freq` instead of
  `freq - center`), so a new locked demod snapped to the mirrored
  position on the next post-thread pass instead of landing where
  placed. All offset writers now share a `deltaLockOffset`
  helper pinning the post thread's `freq == center + ofs`
  convention.
- *Delta button with no selection:* the mute button writes through
  to the last-state when nothing is selected (`handleMuteButton`),
  but the delta button had no such path — enabling it and creating
  a demod silently created an unlocked one and cleared the button.
  A matching `handleDeltaLockButton` carries the button state into
  the next created demod.
- *Range-move on locked demods:* the range-release tail set the
  frequency without re-pinning the lock offset, so the post thread
  snapped it straight back and only the bandwidth stuck. It now
  re-pins like every other move path.

## Validation Plan

- Full C++ suite and `test_build.py` green, Windows build with no new
  warnings. The key path needs the running app, so behavior is
  verified live: lock a demod and step both directions (marker holds
  relative position while the spectrum slides, tuner follows);
  nudge/V/M/R with the cursor in and out of the waterfall;
  Shift+M/Shift+R over partial and full in-range sets; create a
  locked demod by range-drag and confirm it lands where placed;
  enable the delta button with no selection and confirm the next
  created demod is locked; range-drag an existing locked demod
  and confirm it moves with the new bandwidth.

## Risks / Open Questions

- Mouse-drag on a locked demod re-pins the lock offset as it moves
  (waterfall drag path), so it never had the snap-back fight; no
  change needed.
- Rapid presses converge (the nudge never touches the demod, so there
  is nothing to clobber), but this relies on the post thread's
  existing correction, which is unchanged.
- AZERTY/non-QWERTY layout handling is out of scope here; follow-up
  branch.

## Outcome

Implemented as planned. Retire this synopsis on merge.
