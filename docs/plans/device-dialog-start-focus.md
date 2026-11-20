# Device Dialog Start Focus

## Problem

On first start the SDR Devices dialog auto-selects the last
used device, but keyboard focus stays elsewhere, so there is
no way to press Enter to start streaming.

## Fix

- When the enumeration timer auto-selects the last-used (or
  active) device, the Start button takes focus — but only
  when that device is not already streaming, so reopening
  mid-stream never arms Stop under Enter.
- The dialog key hook routes Enter and numpad Enter to the
  existing Start/Stop mouse handler via a synthetic click
  carrying the key timestamp, reusing its double-click
  debounce. Only presses with focus on the button act;
  property-grid and tree keys pass through untouched.
- A native button binding covers Enter presses the key hook
  never sees: wx translates a focused button's Enter
  straight to a native click pre-dispatch. Those events
  carry no timestamp, so the handler stamps them with the
  triggering input message's time — without that, the
  debounce reads every press as a repeat and swallows it.
- Space is deliberately excluded: the key-up lands on the
  main window after the dialog closes and would open the
  frequency dialog. Enter has no such stray.

## Files

- `src/forms/SDRDevices/SDRDevices.cpp`, `.h`

## Verification

- Build through `python build.py` stays green; full doctest
  suite stays green.
- Live check on Windows: fresh launch, wait for the device
  to auto-select, press Enter without touching the mouse;
  the device starts and the dialog closes. Mouse Start/Stop
  still fires exactly once per click.

## Rollback

Revert the branch. Enter in the dialog returns to doing
nothing; mouse behavior is unchanged.
