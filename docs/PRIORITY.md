# Priority Next Steps

Recommended actions after architecture documentation review. Items are ordered by priority and dependency.

## Source Code Fixes

| # | Action | Risk | Effort | Notes |
|---|--------|------|--------|-------|
| 1 | Fix 17m band range in `BookmarkMgr.cpp` | Low | 5 min | Change to ITU 18.068-18.168 MHz. Check if adjacent bands need adjustment. |
| 2 | Fix CMakeLists.txt source/header mismatches | None | 10 min | 5 entries in wrong category. Doesn't break build but misrepresents structure. |

## Documentation Improvements

| # | Action | Risk | Effort | Notes |
|---|--------|------|--------|-------|
| 3 | Add CONTRIBUTING.md | None | 1 hr | Build instructions, code style, PR process. Currently missing. |
| 4 | Add CHANGELOG.md | None | 1 hr | Version history. Currently missing. |
| 5 | Add Doxygen config | None | 2 hr | API docs generation. Currently missing. |
| 6 | Improve README.md | None | 30 min | Add inline build instructions (currently only on external wiki). |

## Code Quality (from RECOMMENDATIONS.md)

Execute in this order per `docs/PLAN.md`:

| # | Plan | Risk | Effort | Dependencies |
|---|------|------|--------|-------------|
| 7 | Fix .gitignore | None | 5 min | None |
| 8 | Add unit tests | Low | 2-3 days | None |
| 9 | Add CI test execution | Low | 2 hours | #8 |
| 10 | Replace reinterpret_cast type punning | Low | 1 day | None |
| 11 | Modernize CMake | Low-Med | 1-2 days | None |
| 12 | Split AppFrame.cpp | Low | 1 day | None |
| 13 | Resolve open TODOs | Low | 1 day | #11, #12 |
| 14 | Replace raw new/delete | Medium | 1 day | #8 |
| 15 | Remove MSVC C4996 suppression | Medium | 1-2 days | None |
| 16 | Update vendored dependencies | High | 3-5 days | None |
