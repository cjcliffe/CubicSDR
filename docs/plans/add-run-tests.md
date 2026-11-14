# Plan: Add --run-tests Entry to build.py

See also: [PLAN.md](../PLAN.md)

## Current State

`build.py` drives dependency, app, and packaging builds through per-platform builders dispatched from `main`, reusing the `run()` helper. It has no test entry, so running the suite requires raw `cmake` / `ctest` invocations even though project convention is to build through `python build.py`.

The top-level CMake exposes a `BUILD_TESTING` option gating the `tests` subdirectory, which defines the wx-free `cubicsdr_tests` target. CI builds and runs tests in a separate non-preset build directory followed by `ctest --output-on-failure`.

## Implementation Plan

1. Accept `--run-tests` in `parse_args`.
2. Add a test-phase helper reusing the existing `run()` helper: configure a separate non-preset build directory with `BUILD_TESTING=ON`, build only the `cubicsdr_tests` target, and run `ctest` against that directory with `--output-on-failure`. Prefer a separate directory over reusing an app preset directory, so no preset or build layout changes and no `cmake --preset` plus extra `-D` override questions arise.
3. Hook the helper into `main` after the platform build dispatch, so all platforms share one code path instead of duplicating it per builder. Pass the matching build config to `ctest` on multi-config generators such as Windows.
4. Define flag interactions: `--dry-run` flows through `run()` as usual; `--clean` keeps its existing behavior; `--skip-app`, `--skip-deps`, `--modules`, and `--package` do not block the test phase. Like CI, the entry assumes dependencies are available rather than bootstrapping them: top-level configure still requires wxWidgets and SoapySDR even though the test target itself links neither.
5. Keep the committed test target wx-free, so no dependency plumbing changes are needed.

## Verification Criteria

- `python build.py --run-tests --dry-run` previews the configure, build, and `ctest` commands without running anything.
- `python build.py --run-tests` configures, builds, and runs the suite green on the touched platforms with dependencies available.
- Existing app builds are unchanged when the flag is absent.
- Reruns without a rebuild stay available through `ctest` against the test directory or the test binary directly.

## Rollback Strategy

This is a localized change to the build driver plus an index row. If regressions appear, `git revert` the commit to restore the prior behavior.

## Files to Modify

| File | Action |
|------|--------|
| `build.py` | Add `--run-tests` flag, test-phase helper, and `main` hookup |
| `docs/PLAN.md` | Index the new plan |
