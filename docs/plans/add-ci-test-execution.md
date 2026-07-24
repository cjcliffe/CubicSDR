# Plan: Add CI Test Execution

CubicSDR is a cross-platform Software-Defined Radio application (C++14, wxWidgets, OpenGL). This plan covers adding test execution to the CI pipeline.

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

**Last Updated:** 2026-07-23

## Current State

No CI configuration exists in the repository (no `.circleci/`, `.github/workflows/`, or `.travis.yml`).

## Implementation Plan

Depends on: [Add Unit Tests](add-unit-tests.md). Once tests exist:

1. Add CI configuration (e.g., GitHub Actions) with a test step after build:
   ```yaml
   - run:
       name: Run tests
       command: |
         cd build
         ctest --output-on-failure
   ```
2. Consider adding a matrix of test configurations (Release/Debug).
3. Add test result upload for CI visibility.
4. Consider adding a separate "test" workflow that depends on "build".

## Verification Criteria

- CI pipeline compiles the project and then runs `ctest`.
- Test results are visible in the CI job output.
- A test failure causes the CI job to fail (non-zero exit code).

## Files to Modify

| File | Action |
|------|--------|
| CI config (`.github/workflows/` or similar) | Create CI pipeline with test execution |

## Dependencies

This plan requires the test infrastructure from [Add Unit Tests](add-unit-tests.md) to be completed first.
