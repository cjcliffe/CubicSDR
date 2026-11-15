## Goal

Fix GH#842 (session files fail to restore the audio output device, always
falling back to the default) for devices whose RtAudio names contain runs
of spaces or leading/trailing space.

## Root Cause

The session file is correct: save writes the live RtAudio device name raw.
But load parses through TinyXML with whitespace condensing on, so the name
comes back with runs collapsed and ends trimmed. `loadInstance` then
compares the condensed loaded string with `==` against the spaced live
enumeration, misses, and falls back to the first device. Reproduced with a
failing-first DataTree save/load round-trip test using the reporter's exact
device string before changing the fix.

## Approach

- *New `AudioDeviceMatch.h`:* `collapseAudioDeviceName()` folds whitespace
  runs to single spaces and trims ends (mirroring the XML layer's condense,
  leaving non-ASCII bytes untouched); `audioDeviceNameMatches()` accepts
  exact or condensed equality. Header-only, following the `ResampleGuard.h`
  precedent, covered by `test_AudioDeviceMatch.cpp`.
- *`loadInstance`:* the output-device lookup uses the whitespace-insensitive
  matcher. Loaded names are always condensed (nothing disables it), so a
  single comparison covers both plain and spaced names.
- *Fallback guard:* the first-device fallback now skips when the output map
  is empty instead of dereferencing `begin()` on an empty container.

## Validation Plan

- New matcher suite green (unit cases plus a DataTree save/load/match
  end-to-end with the reporter's string), full C++ suite and
  `test_build.py` green, Windows build with no new warnings.
- Live session-restore check needs an audio device with odd spacing in its
  name; the end-to-end test drives the same save/load/match code path.

## Risks / Open Questions

- Two devices whose names differ solely by whitespace would collide under
  the condensed comparison; no ordering can break that tie since the
  distinction is destroyed on load. Not observed in practice.
- Old session files keep working unchanged: the file format is untouched,
  only the comparison is more forgiving.

## Outcome

Implemented as planned. Retire this synopsis on merge.
