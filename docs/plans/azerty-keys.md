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
  generates. Letters, arrows, space, tab, and Ctrl+B/D are
  untouched.
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
  space fires its dialog exactly once.

## Risks / Open Questions

- If wxGTK does not translate AltGr to Unicode `[`, the GTK side
  needs a native fallback; the unified matcher still covers
  Windows/macOS. Resolved by the evidence step before coding.
- No automated coverage is possible for wx key handling; the
  live matrix is the gate.

## Outcome

Implemented as planned. Retire this synopsis on merge.
