# Agent Instructions

This is a C++ Software-Defined Radio application (CubicSDR) built with wxWidgets, OpenGL, liquid-dsp, and SoapySDR.

## Project Documentation

- `docs/RECOMMENDATIONS.md` — Project evaluation, strengths, weaknesses, and priority-ranked improvement recommendations
- `docs/PLAN.md` — Index of implementation plans with risk/effort estimates and execution order
- `docs/design/` — Architecture documentation (signal flow, threading model, modem system)
- `docs/plans/` — Individual implementation plans for each recommendation:
  - `add-unit-tests.md` — Test infrastructure and initial test coverage
  - `fix-gitignore.md` — Comprehensive .gitignore patterns
  - `modernize-cmake.md` — CMake 2.8 → 3.14+ modernization
  - `smart-pointers.md` — Replace raw new/delete with std::unique_ptr
  - `split-appframe.md` — Split monolithic AppFrame.cpp into multiple files
  - `remove-c4996-suppression.md` — Address unsafe CRT function usage
  - `replace-reinterpret-cast.md` — Fix undefined behavior in DataTree
  - `update-vendored-deps.md` — Update or replace third-party libraries
  - `add-ci-test-execution.md` — Run tests in CI pipeline
  - `resolve-todos.md` — Address open TODO/FIXME markers

## Build

CMake-based. See `CMakeLists.txt` for build configuration. The project targets C++14 on Windows (MSVC), macOS, and Linux.

## Key Directories

- `src/` — Application source (sdr/, demod/, audio/, visual/, modules/modem/, util/)
- `external/` — Vendored third-party libraries (lodepng, tinyxml, rtaudio, liquid-dsp, hamlib, cubicvr2)
- `.circleci/` — CI configuration (builds only, no test execution)
- `font/` — Bitmap fonts
- `cmake/` — CMake helper modules

## Conventions

- Follow existing code style in each file
- Do not add comments unless asked
- SPDX license headers on source files: `// Copyright (c) Charles J. Cliffe // SPDX-License-Identifier: GPL-2.0+`

## Documentation Guidelines

When creating or updating documentation in `docs/`, avoid brittle content that will drift from the codebase:

- **No specific line numbers** — Reference functions/classes, not line numbers (e.g., "In `CubicSDR.cpp`" not "on line 390")
- **No exact counts** — Avoid "197 source files" or "18 TODOs" — use qualitative terms like "multiple" or "several"
- **No "Last Updated" timestamps** — They create maintenance burden with zero value
- **No redundant preambles** — Don't repeat "CubicSDR is a cross-platform..." in every doc when PLAN.md already links everything
- **No theoretical discussions** — Keep design considerations in code comments, not implementation plans
- **No editorial commentary** — Describe what the code does, not what it doesn't do. Avoid negative framing ("X is not handled by Y") and notes about what was corrected during editing. The spec should reflect the current state of the code, not the reverse-engineering process
- **Reference functions, not lines** — "In the `OnInit()` method" is durable; "on line 390" is not
- **Keep actionable content only** — File lists, verification criteria, and rollback strategies are durable; inventories of current state go stale

## Session Logging

Log your work to `AGENT-LOG.md` in the project root. For each session, append a section with:
- Date and model name
- Numbered list of actions taken
- Tables of files created/modified with brief descriptions

### Log Maintenance

- **Capture outcomes, not process.** If something is added then later removed in the same session, just note the removal and why — don't keep both.
- **Compact iterative fixes.** Multiple passes over the same files (fix errors → fix different errors → fix original errors) should compress into a single summary, not three separate entries.
- **Drop files that no longer exist.** If a file was created then deleted, remove it from the file table or note it as "(deleted)" — don't leave stale entries.
- **Keep the log scannable.** A log entry should answer "what changed and why" in a few bullet points. If it takes more than that, it's too detailed.
