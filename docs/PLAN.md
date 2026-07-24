# CubicSDR Improvement Plans

Detailed implementation plans for each recommendation. See [RECOMMENDATIONS.md](RECOMMENDATIONS.md) for the full evaluation and summary.

**Last Updated:** 2026-07-23

## Plans

| Plan | Risk | Effort | Dependencies |
|------|------|--------|-------------|
| [Fix .gitignore](plans/fix-gitignore.md) | None | 5 min | None |
| [Add Unit Tests](plans/add-unit-tests.md) | Low | 2-3 days | None |
| [Replace reinterpret_cast Type Punning](plans/replace-reinterpret-cast.md) | Low | 1 day | None |
| [Modernize CMake](plans/modernize-cmake.md) | Low-Medium | 1-2 days | None |
| [Split AppFrame.cpp](plans/split-appframe.md) | Low | 1 day | None |
| [Resolve Open TODOs](plans/resolve-todos.md) | Low | 1 day | split-appframe, replace-reinterpret-cast |
| [Replace Raw new/delete with Smart Pointers](plans/smart-pointers.md) | Medium | 1 day | None |
| [Remove MSVC C4996 Suppression](plans/remove-c4996-suppression.md) | Medium | 1-2 days | None |
| [Add CI Test Execution](plans/add-ci-test-execution.md) | Low | 2 hours | add-unit-tests |
| [Update Vendored Dependencies](plans/update-vendored-deps.md) | High | 3-5 days | None |

## Recommended Execution Order

Execute in this order to minimize risk and satisfy dependencies:

1. **Fix .gitignore** — Zero risk, immediate value, unblocks clean builds
2. **Add Unit Tests** — Foundational; enables test-driven work on subsequent plans
3. **Add CI Test Execution** — Locks in test infrastructure before code changes
4. **Replace reinterpret_cast** — Low risk, standalone, improves correctness
5. **Modernize CMake** — Low-medium risk, standalone, enables better build practices
6. **Split AppFrame.cpp** — Low risk, standalone, reduces cognitive load for later work
7. **Resolve TODOs** — Depends on AppFrame split and reinterpret_cast replacement
8. **Replace Raw new/delete** — Medium risk; do after tests exist to catch regressions
9. **Remove C4996 Suppression** — Medium risk; requires touching many files
10. **Update Vendored Dependencies** — Highest risk; do last, requires extensive testing
