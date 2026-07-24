# Contributing to CubicSDR

## Getting Started

1. Read `docs/RECOMMENDATIONS.md` for a project overview, known issues, and improvement areas.
2. Read `docs/PLAN.md` for the full list of improvement plans with risk/effort estimates and a recommended execution order.
3. Pick a plan from `docs/plans/` that matches your interest and skill level.

## Picking Up a Plan

Each plan in `docs/plans/` includes:
- **Current State** — what exists today
- **Implementation Plan** — step-by-step instructions
- **Files to Create/Modify** — exact files affected

Plans are ordered by dependency. Check the "Dependencies" column in `docs/PLAN.md` before starting — some plans require others to be completed first.

## Code Conventions

- Follow existing code style in each file (indentation, naming, braces).
- Do not add comments unless asked.
- SPDX license headers on all source files:
  ```
  // Copyright (c) Charles J. Cliffe // SPDX-License-Identifier: GPL-2.0+
  ```

## Build and Test

CubicSDR uses CMake. See the project README and wiki for build instructions.

Once the test infrastructure is in place (see `docs/plans/add-unit-tests.md`):
```bash
cmake -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Submitting Changes

1. Fork the repository and create a feature branch.
2. Implement the plan, following the steps in the plan file.
3. Verify the build compiles cleanly and tests pass.
4. Submit a pull request referencing the plan (e.g., "Implements docs/plans/smart-pointers.md").

## Reporting Issues

The project has 14 open TODO/FIXME markers tracked in `docs/plans/resolve-todos.md`. If you encounter a bug or have a feature request, open an issue on the GitHub repository.
