# Plan: Replace Raw new/delete with Smart Pointers

CubicSDR is a cross-platform Software-Defined Radio application (C++14, wxWidgets, OpenGL). This plan covers replacing raw `new`/`delete` allocations with `std::unique_ptr` and fixing memory leaks in `src/CubicSDR.cpp`.

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md)

**Last Updated:** 2026-07-23

## Current State

In `src/CubicSDR.cpp`:
- 8 raw `new std::thread(...)` allocations (lines 390, 393, 397, 405, 617, 794, 843, 1164)
- 6 raw `new WorkerThread()` allocations (lines 340, 355, 358, 374, 400, 1157)
- 15 raw `delete` operations (lines 506-530, 775, 804, 1148, 1153, 1188, 1194)
- 4 confirmed memory leaks:
  - `m_glContextAttributes` (line 225) — never deleted
  - `confName` wxString (line 562) — never freed
  - `modPath` wxString (line 579) — never freed
  - `appframe` (line 404) — never explicitly deleted
- `t_SDREnum` overwritten (lines 617, 794) without joining/deleting the old thread

## Implementation Plan

### Phase 1: Declare smart pointer members

In `src/CubicSDR.h`, change declarations from:
```cpp
std::thread *t_SDR;
SDRThread *sdrThread;
```
to:
```cpp
std::unique_ptr<std::thread> t_SDR;
std::unique_ptr<SDRThread> sdrThread;
```

Apply to all thread and worker object pointers:
- `t_SDR`, `t_PostSDR`, `t_SpectrumVisual`, `t_DemodVisual`, `t_SDREnum`, `t_Rig`
- `sdrThread`, `sdrPostThread`, `spectrumVisualThread`, `demodVisualThread`, `sdrEnum`, `rigThread`
- `m_glContext` (PrimaryGLContext*)

### Phase 2: Update allocations

Replace all `new` calls with `std::make_unique`:
```cpp
// Before:
t_SDR = new std::thread(&SDRThread::threadMain, sdrThread);
// After:
t_SDR = std::make_unique<std::thread>(&SDRThread::threadMain, sdrThread);
```

### Phase 3: Remove all manual `delete` calls

Smart pointers handle cleanup automatically. Remove all 15 `delete` operations.

### Phase 4: Fix memory leaks

- `m_glContextAttributes`: wrap in `std::unique_ptr<wxGLContextAttrs>` or delete in `OnExit()`.
- `confName`/`modPath` (lines 562, 579): change from raw `new wxString` to stack-allocated `wxString` or `std::unique_ptr`.

### Phase 5: Fix t_SDREnum overwrite

Before overwriting `t_SDREnum`, join the existing thread:
```cpp
if (t_SDREnum && t_SDREnum->joinable()) {
    t_SDREnum->join();
}
t_SDREnum = std::make_unique<std::thread>(...);
```

### Design Considerations: Join vs. Detach

When replacing raw `new`/`delete` with smart pointers, each thread must be evaluated for correct shutdown semantics:

- **Join before delete** — Use when the thread's work must complete before destruction (e.g., `t_SDR`, `t_DemodVisual`). The current code joins before deleting, so `std::unique_ptr` with explicit `join()` preserves existing behavior.
- **Detach** — Use when the thread is a background worker that should outlive the owning scope. If any thread is detached, it must not be held by a `unique_ptr` that joins on destruction — use a detached raw pointer or `std::thread` stored separately.
- **Exception safety** — `std::unique_ptr` destructor calls `delete` (not `join`). For threads that must be joined, always call `join()` explicitly before the `unique_ptr` goes out of scope. Consider a RAII wrapper that joins in the destructor if this pattern is needed in multiple places.

Review each thread allocation in `CubicSDR.cpp` and document the chosen semantics (join or detach) in a code comment at the declaration site.

## Files to Modify

| File | Action |
|------|--------|
| `src/CubicSDR.h` | Change pointer declarations to smart pointers |
| `src/CubicSDR.cpp` | Update allocations, remove deletes, fix leaks |
