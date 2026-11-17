## Goal

Make the AltGr chords that produce `[`/`]` nudge on AZERTY on
Windows, Linux, and macOS, resolving the conflict where the
bracket position also triggers digit frequency entry (the
remaining half of GH#629).

## Root Cause

Translated characters only exist on `CHAR` events: key down/up
codes are explicitly untranslated (modifier-independent), so no
matching there can ever see an AltGr-produced `[`. The global
handlers match key codes only, and the "universal" raw-code block
carries macOS virtual-key values on every port. Raw codes are
platform-specific (positional on macOS, VK codes on Windows,
keysyms on GTK), so the block only ever worked on macOS. Worse,
the global filter suppresses every unmatched key, which also
suppresses its `CHAR` event — translated brackets and digits can
never arrive while that stands.

## Approach

- *`CHAR` matcher:* a new `OnGlobalChar` matches translated
  brackets (`[`/`]` full snap, `{`/`}` half snap) and digits,
  identically on all ports; unhandled characters pass through.
  Both raw-code blocks are deleted.
- *Key-down passthrough:* bracket and main-row digit cases leave
  the key-down handler (numpad stays: layout-independent, still
  single-fire), the Ctrl swallow narrows to plain Ctrl, and
  unmatched keys pass instead of filtering so their `CHAR`
  generates. Canvas redispatch runs on key-down only, never on
  the hook pass, so shortcuts like `B` fire exactly once.
  Letters, arrows, space, tab, and Ctrl+B/D are untouched.
- *Mode-key scheme:* one key per mode (`F` FM/FMS cycle, `N`
  NBFM, `A` AM, `W` CW, `L`/`U` LSB/USB, `D` DSB/LSB/USB cycle,
  `I` I/Q) with outside-family fallback, fixing `F` going dead
  after `A`. `D` advances its cycle only when pressed in streak,
  otherwise it homes straight to DSB. Delete moves to
  DEL/Backspace preferring the hovered demod with fallback to
  the selection (key-up, hold-safe); the canvas delete case is
  removed.
- *`CHAR` filtering:* unhandled characters filter instead of
  passing, since reaching native focus unhandled beeps on
  macOS. Safe because every text entry is modal or guarded.
- *No-demod key pre-sets:* `M`/`V` with no selection flip the
  mute/delta buttons' last-state directly, matching clicks;
  the solo sync moved into a `handleSoloButton` helper that
  runs with and without a selection so the button tracks the
  `S` key.
- *Waterfall tips:* hover instructions say `DEL` delete instead
  of `D`, and the idle tip reads `Shift-R/M record/mute all`.
- *Untouched:* `FrequencyDialog`/`DemodLabelDialog` text entry,
  arrow-key snap (future separate snap work).

## Validation Plan

- Full C++ suite and `test_build.py` green, Windows build with no
  new warnings. Key handling needs the running app, so behavior
  is verified live on all three ports × QWERTY/AZERTY: AltGr
  brackets nudge, direct brackets nudge on QWERTY, Shift halves
  snap, digits open entry on both layouts including numpad,
  space opens entry, unshifted letters still trigger, `!` is
  inert on GTK, stray Ctrl+Alt+letter chords do nothing. Also
  confirm hold-to-nudge repeats, menu keyboard nav still works,
  typing in every text field is unaffected, and tuning-strip
  space fires its dialog exactly once. Mode keys: each selects
  its mode, `F` cycles FM/FMS and recovers after `A`, delete
  keys remove the hovered demod or the selection when nothing
  is hovered (hold-safe), and Backspace stays safe in dialogs
  and text fields. `B` toggles dB once per press without
  beeping; `E` and tuning-strip space open a single dialog.
  With no selection, `M`/`V`/`S` flip their buttons and the
  next created demod inherits the pre-set.

## Risks / Open Questions

- If wxGTK does not translate AltGr to Unicode `[`, the GTK side
  needs a native fallback; the unified matcher still covers
  Windows/macOS. Resolved by the evidence step before coding.
- No automated coverage is possible for wx key handling; the
  live matrix is the gate.

## Outcome

Implemented as planned. Retire this synopsis on merge.

## Packaging Note

CI bundles are parked for a macOS-x64-only test round; revert
`.circleci/config.yml` before merge. The `dmg-test3` tree fails
`hdiutil convert` with a corrupt `temp.dmg` deterministically
while `dmg-test2` passes on the same hosts; every packaging
sub-step exit code is checked, so `create` is exiting 0 while
writing a bad image, keyed off the staging geometry. The next
test tag carries failure forensics plus one `cpack` retry and
moves the image filesystem from deprecated HFS+ to APFS
(mountable since 10.13; the deployment floor is 11.0).
