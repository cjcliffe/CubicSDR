# Agent Instructions

This is a C++ Software-Defined Radio application (CubicSDR) built with wxWidgets, OpenGL, liquid-dsp, and SoapySDR.

## Project Documentation

- `docs/RECOMMENDATIONS.md` — Project evaluation, strengths, weaknesses, and priority-ranked improvement recommendations
- `docs/PLAN.md` — Index of implementation plans with risk/effort estimates and execution order
- `docs/plans/` — Individual implementation plans for each recommendation:
  - `add-unit-tests.md` — Test infrastructure and initial test coverage
  - `fix-gitignore.md` — Comprehensive .gitignore patterns
  - `modernize-cmake.md` — CMake 2.8 → 3.14+ modernization
  - `smart-pointers.md` — Replace raw new/delete with std::unique_ptr
  - `split-appframe.md` — Split 3,200-line AppFrame.cpp into 5 files
  - `remove-c4996-suppression.md` — Address unsafe CRT function usage
  - `replace-reinterpret-cast.md` — Fix undefined behavior in DataTree
  - `update-vendored-deps.md` — Update or replace third-party libraries
  - `add-ci-test-execution.md` — Run tests in CI pipeline
  - `resolve-todos.md` — Address 14 open TODO/FIXME markers

## Build

CMake-based. See `CMakeLists.txt` for build configuration. The project targets C++14 on Windows (MSVC), macOS, and Linux.

## Key Directories

- `src/` — Application source (sdr/, demod/, audio/, visual/, modules/modem/, util/)
- `external/` — Vendored third-party libraries (lodepng, tinyxml, rtaudio, liquid-dsp, hamlib, cubicvr2)
- `font/` — Bitmap fonts
- `cmake/` — CMake helper modules

## Conventions

- Follow existing code style in each file
- Do not add comments unless asked
- SPDX license headers on source files: `// Copyright (c) Charles J. Cliffe // SPDX-License-Identifier: GPL-2.0+`

## Session Logging

Log your work to `AGENT-LOG.md` in the project root. For each session, append a section with:
- Date and model name
- Numbered list of actions taken
- Tables of files created/modified with brief descriptions

This creates a history of agent contributions to the project.
