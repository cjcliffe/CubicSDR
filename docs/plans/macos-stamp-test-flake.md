# macOS stamp test arch flake — synopsis

## Symptom

Linux CI failed one `test_build.py` case, `test_covers_arch_host_and_floor`,
with `None != 'x86_64-apple-darwin'`. The same suite passed on Windows and on
other CI runners.

## Root cause

`macos_build_flags` returns `host=None` for native builds, where native means
the requested target equals the runner's own `platform.machine()`. The test
requested target `x86_64` with a fixed triplet expectation but pinned nothing
about nativeness, so on x86_64 runners the input was a native config (host
`None`, fail) while everywhere else it was a cross config (host set, pass).
The helper behaved as documented; the test's expectation was incomplete. The
C++ application build was never implicated.

## Investigation

Read the failing assertion, the stamp helper, and the flag helper; classified
every `macos_build_flags` caller in the suite. Only the one test asserted a
literal host for a fixed target. Sibling tests in the same file already pick
whichever arch is non-native, and the fix follows that established pattern
instead of changing production code.

## Fix

The cross-path test now targets whichever arch is non-native on the runner and
asserts the matching triplet, and a companion test locks the native branch
(`native` target, host `None`, floor recorded). Test-only change; `build.py`
is untouched.

## Verification

- Full `test_build.py`: 89 tests pass (one skip, as before).
- Arch matrix: the macOS flag/stamp/restamp cases pass with `platform.machine`
  mocked to x86_64, AMD64, arm64, and aarch64 — including the previously
  failing x86_64 shape.
- Sensitivity check: the same mock reproduces the original failure mode
  (`None` host on x86_64, triplet elsewhere) against the unpinned call.

## Follow-ups

None. If the macOS stamp format changes, both branches now have deterministic
tests to update together.
