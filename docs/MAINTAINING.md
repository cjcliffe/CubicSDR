# Maintaining CubicSDR Documentation

This document describes how to maintain and extend the documentation in `docs/`.

**Last Updated:** 2026-07-23

## Documentation Structure

```
docs/
  RECOMMENDATIONS.md        Project evaluation, strengths, weaknesses, recommendations
  PLAN.md                   Master index of improvement plans with timeline
  MAINTAINING.md            This file — how to maintain docs
  design/
    README.md               Architecture overview and key class reference
    signal-flow.md          Data path from SDR hardware to audio output
    threading.md            Thread inventory, synchronization, lifecycle
    modem-system.md         Modem plugin architecture and available types
  plans/
    add-unit-tests.md       Test infrastructure and initial coverage
    add-ci-test-execution.md  CI test pipeline
    fix-gitignore.md        Repository hygiene patterns
    modernize-cmake.md      CMake 2.8 → 3.14+ modernization
    smart-pointers.md       Replace raw new/delete with std::unique_ptr
    split-appframe.md       Split AppFrame.cpp into 5 files
    remove-c4996-suppression.md  Address unsafe CRT function usage
    replace-reinterpret-cast.md  Fix undefined behavior in DataTree
    update-vendored-deps.md Update or replace third-party libraries
    resolve-todos.md        Address open TODO/FIXME markers
```

## Update Conventions

### When to Update

- **After implementing a plan:** Update the plan file to mark completed steps, add notes about actual results, and link to the implementing commits/PRs.
- **After architectural changes:** Update `design/` documents to reflect new classes, modified data flows, or changed threading patterns.
- **After adding/removing dependencies:** Update `update-vendored-deps.md` current state table.
- **After resolving TODOs:** Update `resolve-todos.md` to mark resolved items.

### Timestamps

Every file includes a `Last Updated` date at the top. Update this when making significant changes (not for minor formatting fixes).

### Cross-Links

Plans link back to `RECOMMENDATIONS.md` and `PLAN.md` via relative paths:
```markdown
See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)
```

Architecture documents link to each other and to `PLAN.md`:
```markdown
See also: [Signal Flow](signal-flow.md) | [Threading Model](threading.md)
```

### File Naming

- Plans: lowercase with hyphens (`add-unit-tests.md`)
- Design docs: lowercase with hyphens (`signal-flow.md`)
- Top-level: UPPERCASE (`RECOMMENDATIONS.md`, `PLAN.md`, `MAINTAINING.md`)

## Adding New Documentation

### New Improvement Plan

1. Create `docs/plans/your-plan.md` with the standard header:
   ```markdown
   # Plan: Title
   
   Description.
   
   See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)
   
   **Last Updated:** YYYY-MM-DD
   ```
2. Add a row to the plans table in `PLAN.md`
3. Add to the recommended execution order if applicable
4. Update the projected timeline in `PLAN.md`

### New Architecture Document

1. Create `docs/design/your-topic.md`
2. Add a row to the table in `docs/design/README.md`
3. Add a reference in `PLAN.md` under "Architecture Documentation"

## Review Process

Documentation should be reviewed for:

1. **Accuracy** — Do file paths, line numbers, and class names match the current codebase?
2. **Completeness** — Are all affected files/modules covered?
3. **Consistency** — Do cross-links work? Are naming conventions followed?
4. **Actionability** — Can a developer follow the plan without referring to the source code?

## Archiving Completed Plans

When all plans in a category are implemented, consider:

1. Moving completed plans to a `docs/plans/completed/` subdirectory
2. Updating `PLAN.md` to reflect completion status
3. Keeping the plans for historical reference (they document design decisions)
