---
title: "P08 — preemption PATCH 2 runtime service validation"
description: "Hybrid F' PATCH 2: epoch/service/intmask, T01-E repair, T02–T10/T12, cumulative patch artifact, feature OFF for games."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch2, n64modernruntime, harness]
---

# P08 — PREEMPTION PATCH 2 VALIDATION

## Meta

```text
MODE:                 IMPLEMENT PATCH 2 ONLY
MODEL:                Grok 4.6 HIGH
ARCHITECTURE:         Hybrid F' (APPROVED)
TOOIE_BASE_HEAD:      0dd3b8020fcd2a90d780118f0a5b598f90139921
RUNTIME_BASE_HEAD:    ca568b6ad79b9029d14077f0c3ffa757727c5559
IMPLEMENTATION_READY: PATCH 2 landed; PATCH 3 not started
PRODUCTION_PREEMPTION_ACTIVE: NO
```

DeepSeek Hybrid F remains `CORRECTED/SUPERSEDED`.

---

## Baseline before writes

```text
BASELINE_T01:    PASS  (pass=15 fail=0 skipped=11, exit 0)
BASELINE_BUILD:  PASS  (ninja no work on TooieRecompiled)
```

---

## Protected dirty files

```text
threadqueue.cpp  1c61c91985dddfc8919f41afb54aa4ce1c74d5d9e0c4476e51dd4c34a60dbeae
threads.cpp      d12dd1335a79748711e0bf0763f9c638a424fe7b9f0701c3b1a2565970340f99
N64Recomp/main.cpp        fb0ae67cfaf17902867d69b7b3b7aa499e4adabf0d7da921ce6a1a5fcd4035dd
N64Recomp/recompilation.cpp db8b443142b2513e3a5d00c2b0c453d387d70e95840b47b6de284b556e2eeede
```

Before and after identical. **PROVEN.**

```text
DIRTY_FILES_PRESERVED: YES
THREADQUEUE_SHA_UNCHANGED: YES
THREADS_SHA_UNCHANGED: YES
N64RECOMP_JAL_SHA_UNCHANGED: YES
GENERATOR_CHANGED: NO
GENERATED_CODE_CHANGED: NO
TOOIE_GAME_CODE_CHANGED: NO
```

---

## ABI / recomp_context slicing

```text
ABI_STRATEGY:
  Keep extern "C" void ultramodern_service_preemption(uint8_t*, void*).
  Do not declare recomp_context* in ultramodern.hpp.

RECOMP_CONTEXT_DEPENDENCY_SOLUTION:
  ultramodern interrupt APIs take uint32_t* status_reg / uint32_t.
  librecomp translations pass &ctx->status_reg (they already include recomp.h).
  Harness mask tests use the C++ API with an explicit status word.
  C ABI remains opaque; feature-OFF is a no-op. PATCH 3 may wrap C ABI
  in librecomp to extract status_reg — not done here.
```

This is an implementation-slicing correction, not an F' architecture change.

---

## Runtime behavior implemented

```text
WORK_EPOCH_PROTOCOL:
  publish: fetch_add(1, release)
  hint: false if disabled; else epoch.load(relaxed) != tls_last_seen
  service: if disabled return; else acquire/drain/maybe-swap/last_seen/recheck
  Self-requeue publish does not livelock: if the only epoch delta is our
  requeue increment, service returns with hint still true.

ENQUEUE_PUBLISH: YES  (central enqueue_external_message)
REQUEUE_PUBLISH: YES  (dequeue returns bool; callers publish)
SERVICE_IMPLEMENTED: YES
EVENT_DELIVERY_WHILE_MASKED: YES (drain always; swap only if cpu_preemptible)
CPU_PREEMPTIBLE_MODEL: IE && !EXL && !ERL && (combined & ~OS_IM_NONE)
OSSETINTMASK_IMPLEMENTED: YES (returns previous combined; writes IM+MI; not IE)
DISABLE_RESTORE_IMPLEMENTED: YES (IE save/OR, not a counter)
COP0_IE_IM_IMPLEMENTED: YES (allow IE/EXL/ERL/IM; keep FR; reject leftover)
STATUS_REG_INITIALIZED: YES (run_thread_function |= IE|IMASK)
MI_GLOBAL_STATE_INITIALIZED: YES (g_mi_mask=0x3F, g_global_int_mask=~0u)
```

osSend/osRecv/osJam cooperative path remains ungated.

---

## T01-E repair

```text
T01_E_REPAIRED: YES
```

MAIN now waits for `host_enqueued` **then** calls service (feature OFF),
latches `service_did_not_wake` only after that call, then coop `osSendMesg`.

---

## Harness results

Threaded tests run in `fork()` children so leftover guest `std::thread`s
cannot poison later cases. Parent never creates OSThreads except T01-A
(flag-only). T12 is isolated too.

Independent dirty-tree run and clean-reconstruction run:

```text
T01 PASS
T02 PASS
T03 PASS
T04 PASS
T05 PASS
T06 PASS
T07 PASS
T08 PASS
T09 PASS
T10 PASS
T11 SKIPPED — PATCH 3
T12 PASS
PATCH2 PASS  (pass=17 fail=0 skipped=1)  exit 0
HARNESS_SHUTDOWN: fork-per-threaded-test; children _Exit
```

---

## Production isolation

```text
DEFAULT_BUILD: PASS
TooieRecompiled relinked after ultramodern + librecomp changes.

INFRASTRUCTURE_LINKED: YES
  mesgqueue -> publish_external_work pulls preemption.o
  translations -> intmask pull intmask.o
ASYNC_PREEMPTION_ACTIVE: NO
  flag default false; hint false; no generated safepoint calls
PRODUCTION_PREEMPTION_ACTIVE: NO
```

`nm TooieRecompiled` shows hint/service/intmask symbols. That is expected
infrastructure, not feature activation.

---

## Cumulative artifact (strategy B)

```text
CUMULATIVE_PATCH_ARTIFACT:
  patches/runtime/n64modernruntime-preemption.patch
CUMULATIVE_PATCH_SHA256:
  c2e2a908cd7be0ddafb92405b2f3f8c37b27624815c95a9131f8f9467f4c7ade
OLD_PATCH1_ARTIFACT_STATUS: REPLACED
PATCH_SCRIPT_UPDATED: YES
```

Paths in the cumulative patch:

- `ultramodern/CMakeLists.txt`
- `ultramodern/include/ultramodern/ultramodern.hpp`
- `ultramodern/src/preemption.cpp`
- `ultramodern/src/intmask.cpp`
- `ultramodern/src/mesgqueue.cpp`
- `ultramodern/test/preemption_harness.cpp`
- `librecomp/src/recomp.cpp`
- `librecomp/src/ultra_translation.cpp`

Apply order: scheduler → cumulative preemption → N64Recomp JAL.

---

## Clean reconstruction

Disposable clone `/tmp/p08-p2-reconstruction` from `ca568b6` + `2b6f056`.
Versioned artifacts only. Second apply: already-applied.

```text
CLEAN_RECONSTRUCTION: PASS
PATCH_SCRIPT_IDEMPOTENT: YES
Reconstructed harness: PATCH2 PASS exit 0
```

Real-tree `apply_runtime_patches.sh`: all three already applied. Protected
hashes unchanged.

---

## Files changed

```text
RUNTIME_FILES_CHANGED:
  ultramodern/CMakeLists.txt
  ultramodern/include/ultramodern/ultramodern.hpp
  ultramodern/src/preemption.cpp
  ultramodern/src/intmask.cpp
  ultramodern/src/mesgqueue.cpp
  ultramodern/test/preemption_harness.cpp
LIBRECOMP_FILES_CHANGED:
  librecomp/src/recomp.cpp
  librecomp/src/ultra_translation.cpp
GENERATOR_FILES_CHANGED: NO
GENERATED_FILES_CHANGED: NO
```

---

## Classification

| claim | class |
| --- | --- |
| T01-E service-after-enqueue | PROVEN |
| Epoch publish on enqueue + requeue | PROVEN |
| Delivery while IE=0 / OS_IM_NONE; swap after restore/unmask | PROVEN |
| Same/lower pri no async swap | PROVEN |
| T08 hint stays true after requeue | PROVEN |
| T09 1000 swaps without queue corruption | PROVEN |
| T12 Fix B duplicate-insert / non-head remove | PROVEN |
| Guest preemption active in Tooie | REFUTED |
| PATCH 3 generator done | REFUTED |

=== END PATCH 2 VALIDATION ===
