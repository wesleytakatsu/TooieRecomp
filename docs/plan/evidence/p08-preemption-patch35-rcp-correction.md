---
title: "P08 — PATCH 3.5 RCP per-OSThread correction"
description: "Narrow PATCH 3.5 correction: current RCP/MI mask is per native guest thread. RCP-X1..X4 and P3-X6+ pass. Flag OFF. No Tooie regen. Independent review of this correction is next."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch35, rcp, mi, tls, n64modernruntime]
---

# P08 — PATCH 3.5 RCP PER-OSTHREAD CORRECTION

```text
MODE:                 IMPLEMENTATION / VALIDATION
MODEL:                Grok 4.6 HIGH
TOOIE_BASE_HEAD:      620fcfe8f13dfb9b3f2173979c15141b21a29da4
RUNTIME_BASE_HEAD:    ca568b6ad79b9029d14077f0c3ffa757727c5559
N64RECOMP_BASE_HEAD:  2b6f05688de2abc7d86da5b4a89b84c2c6acbabe
PRODUCTION_PREEMPTION_ACTIVE: NO
TOOIE_REGEN_RUN:      NO
RECOMPILEDFUNCS_CHANGED: NO
```

Independent review [[p08-preemption-patch35-independent-review]] (verdict C)
is the authority for the defect. This note records the narrow correction only.

## 0. Baseline before writes

```text
TOOIE_HEAD:                 620fcfe8f13dfb9b3f2173979c15141b21a29da4
TOOIE_WORKTREE:             clean
BASELINE_RUNTIME_HARNESS:   PASS (pass=36 fail=0 skipped=0) on a clean rerun
                            T03 flaked once (swapped_while_masked); rerun PASS
BASELINE_T11:               PASS (pass=61 fail=0)
BASELINE_TOOIE_BUILD:       PASS (ninja TooieRecompiled, no work)
```

## 1. TLS / native-thread affinity

Re-checked `osCreateThread`, `_thread_func`, `run_thread_function`,
`wait_for_resumed`, `osDestroyThread`:

```text
TLS_RCP_MODEL_VALID:           YES
OSTHREAD_NATIVE_THREAD_1_TO_1: YES  (for each current OSThread lifetime)
```

One `osCreateThread` allocates one `UltraThreadContext` and one `std::thread`
that stays on `_thread_func` for that OSThread lifetime. `wait_for_resumed`
parks/resumes that same native thread. Destroy/recreate allocates a new native
thread. C++ `thread_local` therefore initializes a new RCP mask to `0x3F`.

No RCP field was added to `recomp_context`. No explicit restore helper.

## 2. Storage correction

```text
ROOT_DEFECT:                   process-global RCP/MI mask
OLD_MI_STORAGE:                ultramodern::intmask::g_mi_mask  (namespace global)
NEW_MI_STORAGE:                thread_local uint32_t current_mi_mask
MI_INITIAL_VALUE:              0x3F   (OS_IM_ALL RCP portion)
GLOBAL_INT_MASK_STORAGE:       process-global g_global_int_mask  (UNCHANGED)
CURRENT_COMBINED_MASK_PER_THREAD:          YES
OSSETINTMASK_MUTATES_CURRENT_THREAD_ONLY:  YES
CAUSE_ELIGIBILITY_USES_CURRENT_THREAD_RCP: YES
EXPLICIT_RCP_RESTORE_ON_RESUME:            NO
THREAD_RESUME_HOOK_CHANGED:                NO
THREADS_CPP_PATCH35_FIX_CHANGE:            NO
THREADQUEUE_CHANGED:                       NO
```

`current_combined_mask`, `apply_os_set_int_mask`, and `cause_eligible` now
read/write `current_mi_mask`. Host producers (VI/AI/SP/DP/SI/PI/timer) never
call these helpers. Drain/eligibility remains game-token/guest-thread side.

T04+ still calls `apply_os_set_int_mask` on the forked child's host thread as
same-thread arithmetic. That uses that thread's own TLS and is not a guest
OSThread. Production mutation is `osSetIntMask_recomp` on a guest thread.

## 3. Discriminator (old global vs TLS)

Throwaway rebuild with `current_mi_mask` as an ordinary process-global
(same tests, only storage changed):

```text
RCP-X1 FAIL  (audio_ran timeout; held_ineligible remained nonzero)
RCP-X2 FAIL  ((old & AI)==0; old was A's VI)
RCP-X3 FAIL  (B/C observed leaked RCP bits)
RCP-X4 FAIL  (audio_ran timeout; held remained)
X1-G   PASS  (SR-only; still valid)
P3-X6+ PASS
PATCH35 FAIL (pass=36 fail=4)
```

TLS restored: PATCH35 PASS (pass=40 fail=0). This is the acceptance
discriminator: A changes its RCP mask, B resumes without `osSetIntMask` and
without a restore helper, and B observes its own previous RCP state.

## 4. Tests actually run

Dirty-tree harness (TLS), three additional repeats, and reconstructed harness:

```text
T01 T02 T03 T04 T04+ T05 T06 T07 T07+ T08 T09 T10 T11-F T12 PASS
X1-A X1-B X1-C X1-D X1-E X1-F X1-G PASS
M1 M2 PASS (same X1-F/M1/M2 case)
REQUEUE_CAUSE PASS
NONINTERRUPT PASS
TLS PASS
P3-X6+ PASS
RCP-X1 PASS
RCP-X2 PASS
RCP-X3 PASS
RCP-X4 PASS
PATCH35 PASS (pass=40 fail=0 skipped=0)
```

```text
P3_X6_PLUS:              PASS
P3_X6_SWAP_ORDER_PROVEN: YES
  BEFORE_SERVICE < HIGH_RAN(InServiceWindow) < AFTER_SERVICE
P3_X6_STATE_ASSERTIONS:  gpr2/gpr4/gpr16, HI, LO, fpu0/fpu1,
                         two native locals, status_reg IE bit
P3_X4:                   PASS (T11; switch target/addend coverage split)
```

P3-X6+ cannot naturally exercise `c1cs` / switch addend. P3-X4 remains the
switch-target proof. Generator policy unchanged.

N64Recomp `PreemptionSafepointTest`: T11 PASS (pass=61 fail=0) on dirty tree
and reconstructed tree.

Tooie: `ninja -C build TooieRecompiled` PASS. No regen. No RecompiledFuncs
edits.

## 5. Nonblocking residuals

```text
MTC0_END_TO_END_ADDED: NO
  Harness does not link librecomp. Production cop0_status_write is not
  callable here. X1-F/M1/M2 remain analog-level.

FLAG_SETTER_RACE: DOCUMENTED
  set_guest_preemption_safepoints must not be mutated concurrently with
  active guest service. held_ineligible is token-serialized; the public
  setter is not. Production env is PATCH 4 startup init, not hot toggle.
  Comment added on the setter. No mutex.
```

## 6. Artifacts

```text
RUNTIME_CUMULATIVE_PATCH:
  patches/runtime/n64modernruntime-preemption.patch
RUNTIME_PATCH_SHA256:
  8959055e6c51767fcd7c58b403a2c6ce8bc7aa0ab6c23ffab9dc181905eacd6b
  (against ca568b6 after scheduler; P1+P2+P3 bridge+P3.5+RCP correction;
   threads.cpp hook only; NO scheduler NULLPTR / threadqueue)

N64RECOMP_CUMULATIVE_PATCH:
  patches/runtime/n64recomp-tooie.patch
N64RECOMP_PATCH_SHA256:
  2045cd90cfae70c881a9504146bac5174ca7a53ba1f5e6200a6a3e4adb5a044c
  UNCHANGED (no N64Recomp file modified)

SCHEDULER_PATCH_SHA256:
  1b3d3d4666e02f748c44d19d867d02669b83753d85fe7eb62d746a5a8cf38309
  UNCHANGED

PATCH_SCRIPT_UPDATED: YES
GENERATOR_POLICY_CHANGED: NO
GENERIC_JR_CHANGED:       NO
```

Reconstruction `/tmp/p08-p35-rcp-reconstruction`:

```text
FIRST_APPLY:              scheduler, preemption, n64recomp applied
SECOND_APPLY:             all three already-applied / idempotent
CLEAN_RECONSTRUCTION:     PASS (0 mismatches vs dirty intended files)
PATCH_SCRIPT_IDEMPOTENT:  YES (script on dirty tree: already applied)
RECON_HARNESS:            PASS (pass=40 fail=0)
RECON_T11:                PASS (pass=61 fail=0)
```

## 7. Gate

```text
PATCH35_ACCEPTED_BY_IMPLEMENTER: YES
PATCH4_READY:                    YES (implementer; remaining gate below)
PATCH4_GATE_MISSING:             independent review of this RCP correction
                                 before flag enable / Tooie regen
DOWNSTREAM_BLOCKER:              func_800BCF28
PRODUCTION_PREEMPTION_ACTIVE:    NO
TOOIE_REGEN_RUN:                 NO
```

Do not implement PATCH 4 in this session.

=== END RCP CORRECTION ===
