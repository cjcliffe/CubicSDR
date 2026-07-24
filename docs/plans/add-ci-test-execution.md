# Plan: Add CI Test Execution

CubicSDR is a cross-platform Software-Defined Radio application (C++14, wxWidgets, OpenGL). This plan covers adding test execution to the CircleCI pipeline.

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

**Last Updated:** 2026-07-23

## Current State

CircleCI config (`.circleci/config.yml`) only compiles the project. No tests are run.

## Implementation Plan

Depends on: [Add Unit Tests](add-unit-tests.md). Once tests exist:

1. Update `.circleci/config.yml` to add a test step after build:
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
| `.circleci/config.yml` | Add test execution step |

## Dependencies

This plan requires the test infrastructure from [Add Unit Tests](add-unit-tests.md) to be completed first.
