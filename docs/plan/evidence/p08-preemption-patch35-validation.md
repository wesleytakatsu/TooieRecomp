---
title: "P08 — PATCH 3.5 semantic-closure validation"
description: "A+ annotated-hold-until-eligible implemented. Flag OFF. No Tooie regen. Independent review is the next gate before PATCH 4."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch35, validation, n64modernruntime]
---

# P08 — PATCH 3.5 VALIDATION

```text
MODE:                 IMPLEMENTATION / VALIDATION
MODEL:                Grok 4.6 HIGH
TOOIE_BASE_HEAD:      976481e33ab0ea2e6d9ea9470a00ca7652e63b6c
RUNTIME_BASE_HEAD:    ca568b6ad79b9029d14077f0c3ffa757727c5559
N64RECOMP_BASE_HEAD:  2b6f05688de2abc7d86da5b4a89b84c2c6acbabe
CAUSE_ARCHITECTURE:   A+ ANNOTATED_HOLD_UNTIL_ELIGIBLE
PRODUCTION_PREEMPTION_ACTIVE: NO
TOOIE_REGEN_RUN:      NO
```

=== BEGIN PATCH 3.5 VALIDATION ===

## Baseline before writes

```text
TOOIE_HEAD:                 976481e33ab0ea2e6d9ea9470a00ca7652e63b6c
TOOIE_WORKTREE:             clean
BASELINE_RUNTIME_HARNESS:   PASS (pass=21 fail=0 skipped=0)
BASELINE_T11:               PASS (pass=39 fail=0)
BASELINE_TOOIE_BUILD:       PASS (ninja TooieRecompiled, no work)
```

Protected hashes before writes:

```text
threadqueue.cpp  1c61c91985dddfc8919f41afb54aa4ce1c74d5d9e0c4476e51dd4c34a60dbeae
threads.cpp      d12dd1335a79748711e0bf0763f9c638a424fe7b9f0701c3b1a2565970340f99
```

After PATCH 3.5:

```text
threadqueue.cpp  UNCHANGED (still Fix B only; scheduler patch owns it)
threads.cpp      c91698744a0eafbd3d132ff86c2cb4795bd5b387e51d89ee73324ac212452f6f
                 NULLPTR guard in osSetThreadPri byte-for-byte preserved
                 only wait_for_resumed resume hook added
```

```text
NULLPTR_GUARD_PRESERVED:          YES
THREADS_PATCH35_HUNK_ISOLATED:    YES
THREADS_EXISTING_NULLPTR_HUNK_CHANGED: NO
THREADQUEUE_CHANGED:              NO
```

## Architecture

```text
INTERRUPT_CAUSE_TYPE:  enum class ultramodern::InterruptCause : uint8_t
                       None, SP, SI, AI, VI, PI, DP, Timer

CAUSE_MAP:
  events.cpp VI retrace          -> VI
  events.cpp AI tick             -> AI
  events.cpp SP complete         -> SP
  events.cpp DP complete         -> DP
  events.cpp SI                  -> SI
  librecomp/src/pi.cpp DMA/SRAM  -> PI     (approximation; approved)
  librecomp/src/flash.cpp        -> PI     (approximation; approved)
  timer.cpp expiry               -> Timer  (IM7 analog; GOOD_ENOUGH)
  extensions.cpp                 -> None
  non-game osSend/osJam          -> None (default)

HELD_LIST_OWNERSHIP:   process-global std::vector in mesgqueue.cpp
HELD_LIST_THREAD_SAFE: YES under single-token model (game-thread only).
                       Host producers never touch it. Setter flush
                       enqueues onto the MPMC (thread-safe) then clears.

CAUSE_ELIGIBILITY:
  None:     always eligible
  hardware: IE && !EXL && !ERL
  RCP:      + SR.IM2 + (g_mi_mask & mi_bit) + (g_global_int_mask>>16 & mi_bit)
  Timer:    + SR.IM7 + (g_global_int_mask & IM7)

FEATURE_OFF_DRAIN:  legacy drain-all; no hold; service no-op
FEATURE_ON_DRAIN:   held first, then newly popped; per-event eligibility;
                    ineligible older AI does not HOL-block later eligible VI
COOPERATIVE_DRAIN_GATED: YES (osSend/osJam/osRecv/wait_for_external_message*)
FULL_REQUEUE_CAUSE_PRESERVED: YES
WORK_EPOCH_CHANGED: NO
TLS_EPOCH:          KEPT (per native guest thread)

TLS_RDRAM_STATUS:
  thread_local tls_rdram / tls_status_reg
  set in librecomp run_thread_function and harness run_thread_function
  refreshed at start of service_preemption
  resume hook / MTC0 use TLS; no recomp_context in ultramodern

AFTER_INTERRUPT_STATE_CHANGE:
  trigger A: osSetIntMask (after r2 write), __osRestoreInt, MTC0 after commit
TRIGGER B:
  THREAD_RESUME_HOOK: threads.cpp::wait_for_resumed
  RESUME_HOOK_POSITION: after running.wait() AND after context-replacement
                        check; before return to guest
  RESUME_HOOK_CAN_RESCHEDULE: YES (nested wait_for_resumed expected)
  first-start complement: service_pending after TLS set in run_thread_function

MTC0_AFTER_COMMIT_SERVICE: YES
OSSETINTMASK_CAUSE_AWARE:  YES (r2 assigned before after_interrupt_state_change)
RESTORE_CAUSE_AWARE:       YES (commit IE, then after_interrupt_state_change)

FINAL_CPU_PREEMPTIBLE_MODEL: IE && !EXL && !ERL
  Dropped "any IM/MI bit open". Hardware wakes are class-filtered before
  delivery. InterruptCause::None may schedule while RCP classes are masked.

FLAG_RUNTIME_DISABLE_BEHAVIOR:
  set_guest_preemption_safepoints(false) flushes held_ineligible onto
  external_messages and publish_external_work(). Legacy drain-all then
  sees them. Production env remains PATCH 4 / startup-immutable.
```

## Tests actually run

Dirty-tree harness (`/tmp/p08-p35-baseline-harness`):

```text
T01 PASS
T02 PASS
T03 PASS
T04 PASS
T04+ PASS
T05 PASS
T06 PASS
T07 PASS
T07+ PASS
T08 PASS
T09 PASS
T10 PASS
T11-F-masked PASS
T11-F-unmasked PASS
T11-G flag OFF PASS
T12 PASS
X1-A PASS
X1-B PASS
X1-C PASS
X1-D PASS
X1-E PASS
X1-F PASS
X1-G PASS
M1 PASS
M2 PASS
REQUEUE_CAUSE PASS
NONINTERRUPT PASS
TLS PASS
P3-X6 PASS  (vehicle: harness toy_generated + real ultramodern swap)
PATCH35 PASS (pass=36 fail=0 skipped=0)
```

Dirty-tree N64Recomp `PreemptionSafepointTest`:

```text
T11_FORWARD / BACKEDGE / DELAY / SELF_LOOP / SWITCH / LIVE_PARITY / ABI: PASS
P3-X1 likely TAKEN:     PASS
P3-X2 likely NOT taken: PASS
P3-X4 switch hostile:   PASS
T11 PASS (pass=61 fail=0)
```

```text
P3_X1_LIKELY_TAKEN:     PASS
P3_X2_LIKELY_NOT_TAKEN: PASS
P3_X4_SWITCH_SURVIVAL:  PASS
P3_X6_REAL_SWAP:        PASS
STATIC_CONTEXT_SAFE:    YES
LIVE_CONTEXT_SAFE:      YES  (live toys + P3-X4; P3-X6 vehicle is harness)
```

Tooie: `ninja -C build TooieRecompiled` PASS. No regen. No RecompiledFuncs edits.

Reconstruction (`/tmp/p08-p35-reconstruction`, versioned artifacts only):

```text
FIRST_APPLY:              PASS
SECOND_APPLY:             already-applied / idempotent
CLEAN_RECONSTRUCTION:     PASS
PATCH_SCRIPT_IDEMPOTENT:  YES
RECON_HARNESS:            PASS (pass=36 fail=0)
RECON source matches dirty tree for intended files
  (threadqueue remains scheduler-owned)
```

## Artifacts

```text
RUNTIME_CUMULATIVE_PATCH:
  patches/runtime/n64modernruntime-preemption.patch
RUNTIME_PATCH_SHA256:
  469537d6def22d267ee72bc6b52c204a3592c0abb024ad140c7e274fac0e5ad7
  (against ca568b6 after scheduler; P1+P2+P3 bridge+P3.5;
   threads.cpp hook only; NO scheduler NULLPTR / threadqueue)

N64RECOMP_CUMULATIVE_PATCH:
  patches/runtime/n64recomp-tooie.patch
N64RECOMP_PATCH_SHA256:
  2045cd90cfae70c881a9504146bac5174ca7a53ba1f5e6200a6a3e4adb5a044c
  (against 2b6f056; same-section JAL + PATCH 3 generator + P3.5 tests)

PATCH_SCRIPT_UPDATED: YES
GENERATOR_POLICY_CHANGED: NO
GENERIC_JR_CHANGED:       NO
```

## PATCH4 gate

```text
CAUSE_CLASS_MODEL:     PASS
X1-A..G:               PASS
M1/M2:                 PASS
T01–T12 / T04+ / T07+: PASS
P3-X4 / P3-X6:         PASS
REQUEUE_CAUSE:         PASS
NONINTERRUPT:          PASS
CLEAN_RECONSTRUCTION:  PASS
PATCH_SCRIPT_IDEMPOTENT: YES
PRODUCTION_FLAG:       OFF
TOOIE_REGEN:           NO
PERFORMANCE_CHANGE_REQUIRED_IN_PATCH35: NO

PATCH4_READY:          YES (IMPLEMENTER CLAIM; CORRECTED/SUPERSEDED)
PATCH4_GATE_MISSING:   independent high-risk review of PATCH 3.5
                       before flag enable / Tooie regen
DOWNSTREAM_BLOCKER:    func_800BCF28
```

> **CORRECTED/SUPERSEDED by independent review (2026-08-13):**
> [[p08-preemption-patch35-independent-review]] proves that current RCP/MI
> mask state is process-global although libultra saves/restores it per
> OSThread. Disposable RCP-only dispatch and `osSetIntMask` old-return tests
> both fail. `PATCH4_READY: NO`; correct PATCH 3.5 only before PATCH 4.

## Classification

| claim | class |
| --- | --- |
| Masked hardware cause is held, not delivered | PROVEN |
| Unrelated open class does not authorize a masked cause | PROVEN |
| Thread-dispatch resume retries held under the new SR | PROVEN |
| Full-mq requeue preserves cause (never becomes None) | PROVEN |
| Feature OFF is legacy drain-all / no async preemption | PROVEN |
| Guest preemption active in Tooie | REFUTED |
| Production generated C contains safepoints | REFUTED |
| PATCH4 ready after implementer validation | CORRECTED/SUPERSEDED |

=== END PATCH 3.5 VALIDATION ===
