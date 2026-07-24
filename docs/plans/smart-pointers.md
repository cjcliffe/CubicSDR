# Plan: Replace Raw new/delete with Smart Pointers

See also: [RECOMMENDATIONS.md](../RECOMMENDATIONS.md) | [PLAN.md](../PLAN.md) | [Threading Model](../design/threading.md)

## Current State

In `src/CubicSDR.cpp`:
- Multiple raw `new std::thread(...)` and `new WorkerThread()` allocations
- Multiple raw `delete` operations for thread and worker objects
- Confirmed memory leaks:
  - `m_glContextAttributes` — never deleted
  - `confName` wxString — never freed
  - `modPath` wxString — never freed
  - `appframe` — never explicitly deleted
- `t_SDREnum` overwritten without joining/deleting the old thread

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

## Verification Criteria

- Zero raw `new std::thread(...)` or `new WorkerThread(...)` allocations remain in `CubicSDR.cpp`.
- Zero manual `delete` calls for thread or worker objects remain.
- No memory leaks detected under repeated startup/shutdown cycles (verify with AddressSanitizer or Valgrind if available).
- All 4 confirmed memory leaks are fixed.
- `t_SDREnum` overwrite is guarded by a join check.
- Build succeeds and application starts/closes cleanly.

## Rollback Strategy

If smart pointer changes cause crashes (e.g., a thread joined twice, or a use-after-free):
1. `git revert` the commit.
2. The original raw `new`/`delete` code is functional — the revert is safe.
3. If only specific phases cause issues, revert to the last working phase (e.g., keep Phase 1 declarations but revert Phase 2 allocations).

## Files to Modify

| File | Action |
|------|--------|
| `src/CubicSDR.h` | Change pointer declarations to smart pointers |
| `src/CubicSDR.cpp` | Update allocations, remove deletes, fix leaks |
