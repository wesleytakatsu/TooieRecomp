---
title: "P08 — PATCH 3.5 RCP correction independent review"
description: "Independent differential review approves the per-OSThread TLS RCP correction with nonblocking residuals. PATCH4_READY YES; production flag remains OFF and Tooie was not regenerated."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch35, rcp, mi, tls, independent-review]
---

# P08 — PATCH 3.5 RCP CORRECTION INDEPENDENT REVIEW

```text
BASE_HEAD:       bc636edd28f784d9e2a8ad10998eaacc1f681907
MODEL:           GPT-5.6 SOL HIGH
MODE:            READ / TEST / DOCS ONLY
VERDICT:         B — CORRECTION APPROVED WITH NONBLOCKING ISSUES
PATCH35_ACCEPTED:YES
PATCH4_READY:    YES
SOURCE_CHANGED:  NO
```

The implementer report in [[p08-preemption-patch35-rcp-correction]] was
treated as an untrusted claim. This review inspected the artifact source,
reconstructed both dependencies from their exact bases, applied all active
artifacts twice, rebuilt the harness and T11, reproduced the old-global
negative discriminator, and ran repeated isolated T03 coverage.

The historical verdict C in [[p08-preemption-patch35-independent-review]] is
preserved. Its specific process-global current-RCP finding is **RESOLVED** by
the correction independently approved here; the historical finding is not
erased or reclassified.

## 1. Differential scope

Range reviewed:

```text
620fcfe8f13dfb9b3f2173979c15141b21a29da4
..
bc636edd28f784d9e2a8ad10998eaacc1f681907
```

The only production semantic delta inside the cumulative runtime artifact is:

```cpp
// ultramodern/src/intmask.cpp
thread_local uint32_t current_mi_mask = 0x3Fu;
```

All three former `g_mi_mask` reads/writes became `current_mi_mask`.
`preemption.cpp` changes only the startup-immutable/test-quiescent setter
contract comment. The remaining artifact delta is RCP-X1..X4 and P3-X6+ in
`preemption_harness.cpp`. `apply_runtime_patches.sh` changes the cumulative
preemption description and SHA256 pin only.

No N64Recomp artifact changed. No generator policy, `threads.cpp`,
`threadqueue.cpp`, or `RecompiledFuncs` delta exists in the reviewed range.

```text
CORRECTION_SCOPE_NARROW:      YES
UNEXPECTED_PRODUCTION_CHANGE: NONE
```

## 2. Storage and OSThread/native-thread model

Repository-wide reconstructed-runtime search found no `g_mi_mask` reference.
`current_mi_mask` is defined once as TLS with initial value `0x3F` and is used
by `current_combined_mask`, `apply_os_set_int_mask`, and `cause_eligible`.
`g_global_int_mask = ~0u` remains an ordinary namespace global; it was not
converted to TLS.

`osCreateThread` allocates one `UltraThreadContext` and constructs one
`std::thread` running `_thread_func`. Scheduling parks and resumes that same
native thread through its `running` semaphore. Destruction signals the old
context; `_thread_func` exits and the cleaner joins/deletes it. Recreating an
OSThread allocates a new context and native thread, hence a fresh TLS value
`0x3F`.

`wait_for_resumed` has no RCP restore helper. After token acquisition and the
context-replacement check it calls
`service_pending_interrupts_for_current_guest()`. The resumed native thread's
TLS already contains its own saved RCP mask; eligibility therefore combines
that mask with the resumed thread's `status_reg`.

```text
OLD_GLOBAL_MI_REFERENCES:       NONE
CURRENT_MI_MASK_STORAGE:        THREAD_LOCAL
MI_INITIAL_VALUE:               0x3F
GLOBAL_MASK_STILL_GLOBAL:       YES
OSTHREAD_NATIVE_THREAD_1_TO_1:  YES
TLS_RCP_MODEL_VALID:            YES
EXPLICIT_RCP_RESTORE_REQUIRED:  NO
RESUME_HOOK_NOW_FULLY_VALID:    YES
```

## 3. RCP-X1..X4

Fresh Debug harness build from the protected runtime checkout and from the
independent reconstruction both passed:

```text
RCP-X1 PASS
RCP-X2 PASS
RCP-X3 PASS
RCP-X4 PASS
PATCH35 PASS  (pass=40 fail=0 skipped=0)
```

RCP-X1 keeps IE and SR.IM2 open on A and B. A closes only MI.AI (leaving VI),
B opens MI.AI, and AI is enqueued once while A is current. A holds it and
blocks; B resumes without another mask call, explicit restore, or host enqueue.
B's TLS makes the held AI eligible and wakes the higher-priority waiter.

RCP-X2 calls `apply_os_set_int_mask` through a harness analog that reproduces
the production `osSetIntMask_recomp` ordering and `ctx.r2` assignment. It does
not link/call the production translation symbol itself, so its level is
`HELPER_PLUS_ANALOG`. Static inspection confirms production wiring uses the
same helper, writes its return to `ctx->r2`, then calls
`after_interrupt_state_change`. After A uses VI and B uses AI, B receives
exactly `OS_IM_AI`, not A's VI bits.

RCP-X3 creates three real ultramodern guest threads, assigns AI, VI, and SP+DP
once, and passes a message token around the three queues for eight cycles.
Each thread reads its mask after every real guest swap; no loop iteration
restores the mask before checking it.

RCP-X4 holds one AI under A, resumes B with its pre-existing AI-open TLS, and
uses only the existing resume hook. It proves held count `1 -> 0`, waiter wake,
message consumption (`mq_count == 0` after the waiter receives it), and the
expected scheduler execution (`audio_ran`).

```text
RCP_X1_ACTUALLY_RCP_ONLY:       YES
RCP_X1:                         PASS
RCP_X2_TEST_LEVEL:              HELPER_PLUS_ANALOG
RCP_X2:                         PASS
PER_THREAD_OLD_MASK_SEMANTICS:  PASS
RCP_X3_REAL_SWAPS:              YES
RCP_X3_NO_MANUAL_RESTORE:       YES
RCP_X3:                         PASS
RCP_X4:                         PASS
```

## 4. Independent old-global discriminator

In the disposable reconstruction only, `thread_local` was replaced by an
ordinary global scalar and the exact same harness rebuilt. No test code was
changed:

```text
GLOBAL_STORAGE_RCP_X1: FAIL expected / observed
GLOBAL_STORAGE_RCP_X2: FAIL expected / observed
GLOBAL_STORAGE_RCP_X3: FAIL expected / observed
GLOBAL_STORAGE_RCP_X4: FAIL expected / observed
X1_G_WITH_GLOBAL_STORAGE: PASS expected / observed
PATCH35 FAIL  (pass=36 fail=4 skipped=0)
```

TLS was restored in the disposable tree afterward. The permanent tests
therefore discriminate the rejected process-global design.

```text
RCP_TESTS_DISCRIMINATE_OLD_BUG: YES
```

## 5. P3-X6+ and context boundary

`toy_generated` stores `BeforeService`, then `InServiceWindow` immediately
before `ultramodern_service_preemption`, and stores `AfterService` only after
that call returns. HIGH records the observed phase after waking from the guest
message queue. The test requires HIGH to observe `InServiceWindow` and the
final phase to be `AfterService`. A later cooperative park cannot satisfy the
assertion because the phase has already changed.

The transfer is real:

```text
service_preemption
  -> dequeue_external_messages
  -> check_running_queue
  -> swap_to_thread
  -> resume_thread_and_wait
  -> native guest semaphore/token handoff
```

No direct HIGH call or host `std::thread::yield` can produce the asserted
phase. P3-X6+ also asserts `gpr2`, `gpr4`, `gpr16`, HI, LO, `fpu0`, `fpu1`,
two native locals, and status IE after the service returns.

The proof is intentionally split. P3-X6+ is a hand-written harness vehicle for
real ultramodern swap/call-stack survival. T11 independently covers static and
Live generator placement/parity; P3-X4 executes Live-generated switch code
through a hostile service callback and proves switch target/addend survival.
P3-X6+ does not itself run generated Tooie or a Live-generated function.

```text
P3_X6_SWAP_ORDER_PROOF: STRONG
P3_X6_REAL_GUEST_SWAP:  YES
P3_X6_STATE_COVERAGE:   gpr2/gpr4/gpr16, HI/LO, fpu0/fpu1,
                        two native locals, status_reg IE
STATIC_CONTEXT_SAFE:    YES (T11 static placement/ABI + P3-X6 real swap)
LIVE_CONTEXT_SAFE:      YES (T11 Live parity + P3-X4 hostile switch;
                        P3-X6 real-swap vehicle is not Live-generated code)
```

## 6. T03 flake and fork/TLS isolation

The implementer's one baseline `swapped_while_masked` failure occurred before
the RCP correction writes. It did not reproduce here:

```text
20 full harness processes
20 isolated T03 child executions (fork-per-test)
T03: 20 PASS / 0 FAIL
full harness: 20 PASS / 0 FAIL
```

The historical observation cannot be assigned to a proven harness race,
semantic masked-preemption defect, contamination, or timeout from current
evidence. It remains class E (`unknown`). It is not a demonstrated PATCH 4
blocker; recurrence must preserve the child output/scheduling evidence before
classification.

`run_isolated` forks before invoking each case. The parent-only T01-A changes
the feature flag but never mutates the RCP mask. Every case child therefore
starts from the initialized parent TLS `0x3F`, and each guest native thread is
created after the fork and independently initializes its TLS to `0x3F`.

```text
T03_FLAKE_REPRODUCED:  NO
T03_FLAKE_CLASS:       E
T03_PATCH4_BLOCKER:    NO
FORK_TLS_TEST_ISOLATION: PASS
```

## 7. Caller and global-mask audit

Production call sites are:

- `apply_os_set_int_mask`: `osSetIntMask_recomp`, executing on a guest thread;
- `cause_eligible`: `drain_pending_external`, executing on the game-token owner;
- `current_combined_mask`: internal to the guest-side mask helper.

Host VI/AI/SP/DP/SI/PI/timer producers only enqueue typed work. They do not
read guest TLS mask state. `g_global_int_mask` has one process-global
definition and only global-mask reads in current production source; no TLS
conversion or per-thread writer was introduced.

```text
PRODUCTION_HOST_MI_MASK_CALLERS: NONE
GLOBAL_INT_MASK_MODEL:           PASS
```

## 8. Reconstruction and regression

Independent disposable root:

```text
/tmp/p08-rcp-independent-RGkwPG
```

Exact bases:

```text
N64ModernRuntime: ca568b6ad79b9029d14077f0c3ffa757727c5559
N64Recomp:        2b6f05688de2abc7d86da5b4a89b84c2c6acbabe
```

Artifact hashes independently calculated:

```text
scheduler:  1b3d3d4666e02f748c44d19d867d02669b83753d85fe7eb62d746a5a8cf38309
preemption: 8959055e6c51767fcd7c58b403a2c6ce8bc7aa0ab6c23ffab9dc181905eacd6b
N64Recomp:  2045cd90cfae70c881a9504146bac5174ca7a53ba1f5e6200a6a3e4adb5a044c
```

`tools/apply_runtime_patches.sh` first applied scheduler, preemption, and
N64Recomp; its second run reported all three already applied. All 15 runtime
paths and 10 N64Recomp paths named by the active artifacts byte-match the
protected dirty checkouts. `threadqueue.cpp` remains scheduler-only. Both
scheduler and preemption artifacts own intentional, distinct `threads.cpp`
hunks; the correction range itself changes neither.

Regression results:

```text
PATCH 3.5 harness, protected checkout: PASS (40/0/0)
PATCH 3.5 harness, reconstruction:     PASS (40/0/0)
N64Recomp T11, protected checkout:     PASS (61/0)
N64Recomp T11, reconstruction:         PASS (61/0)
TooieRecompiled build:                 PASS (ninja: no work to do)
```

No recompiler/regeneration command ran. `RecompiledFuncs` has no generated
`ultramodern_preemption_hint` or `ultramodern_service_preemption` call, and
Tooie production CMake has no flag activation. Production remains unchanged.

```text
PATCH_HASHES_VALID:           YES
CLEAN_RECONSTRUCTION:         PASS
PATCH_SCRIPT_IDEMPOTENT:      YES
TOOIE_REGEN_RUN:              NO
STATIC_TOOIE_SAFEPOINTS_PRESENT: NO
PRODUCTION_PREEMPTION_ACTIVE: NO
```

## 9. Residuals and verdict

No blocking issue was found. Nonblocking residuals remain:

1. RCP-X2 and the MTC0 harness cases are helper-plus-analog rather than linked
   translation/cop0 end-to-end tests; static production wiring is direct.
2. The public runtime flag setter is not safe for concurrent hot mutation of
   `held_ineligible`; PATCH 4's contract is startup-time configuration.
3. Generic non-RA JR safepoint coverage remains documented residual scope.
4. Timer IM7 and direct PI completion remain approved approximations.
5. The historical T03 one-off is class E and did not reproduce in 20 isolated
   child executions plus the other review harness runs.

All PATCH 4 entry conditions listed in the narrow review prompt are satisfied:
RCP-X1..X4, old-global discriminator, X1-A..G, T01–T12, P3-X4, P3-X6+,
reconstruction, idempotence, feature OFF, and no regeneration.

```text
VERDICT:          B — CORRECTION APPROVED WITH NONBLOCKING ISSUES
BLOCKING_ISSUES:  NONE
PATCH35_ACCEPTED: YES
PATCH4_READY:     YES
NEXT_SINGLE_ACTION:
  Implement PATCH 4 activation/regeneration/validation only.
```

## Handoff

```text
=== P08 PATCH 3.5 RCP CORRECTION INDEPENDENT REVIEW ===

BASE_HEAD: bc636edd28f784d9e2a8ad10998eaacc1f681907
MODEL: GPT-5.6 SOL HIGH
VERDICT: B
CORRECTION_SCOPE_NARROW: YES
CURRENT_MI_MASK_STORAGE: THREAD_LOCAL
OLD_GLOBAL_MI_REFERENCES: NONE
MI_INITIAL_VALUE: 0x3F
GLOBAL_MASK_STILL_GLOBAL: YES
OSTHREAD_NATIVE_THREAD_1_TO_1: YES
TLS_RCP_MODEL_VALID: YES
EXPLICIT_RCP_RESTORE_REQUIRED: NO
RESUME_HOOK_NOW_FULLY_VALID: YES
RCP_X1_ACTUALLY_RCP_ONLY: YES
RCP_X1: PASS
RCP_X2_TEST_LEVEL: HELPER_PLUS_ANALOG
RCP_X2: PASS
PER_THREAD_OLD_MASK_SEMANTICS: PASS
RCP_X3_REAL_SWAPS: YES
RCP_X3_NO_MANUAL_RESTORE: YES
RCP_X3: PASS
RCP_X4: PASS
RCP_TESTS_DISCRIMINATE_OLD_BUG: YES
P3_X6_SWAP_ORDER_PROOF: STRONG
P3_X6_REAL_GUEST_SWAP: YES
P3_X6_STATE_COVERAGE: gpr2/gpr4/gpr16, HI/LO, fpu0/fpu1,
                      two native locals, status_reg IE
STATIC_CONTEXT_SAFE: YES
LIVE_CONTEXT_SAFE: YES (split proof: T11/P3-X4 + P3-X6 real swap)
T03_FLAKE_REPRODUCED: NO
T03_FLAKE_CLASS: E
T03_PATCH4_BLOCKER: NO
FORK_TLS_TEST_ISOLATION: PASS
PRODUCTION_HOST_MI_MASK_CALLERS: NONE
GLOBAL_INT_MASK_MODEL: PASS
PATCH35_HARNESS: PASS (40/0/0; reconstruction also 40/0/0)
T11: PASS (61/0; reconstruction also 61/0)
TOOIE_BUILD: PASS
PATCH_HASHES_VALID: YES
CLEAN_RECONSTRUCTION: PASS
PATCH_SCRIPT_IDEMPOTENT: YES
TOOIE_REGEN_RUN: NO
STATIC_TOOIE_SAFEPOINTS_PRESENT: NO
PRODUCTION_PREEMPTION_ACTIVE: NO
BLOCKING_ISSUES: NONE
NONBLOCKING_ISSUES: analog-level RCP-X2/MTC0; startup-only flag setter;
                    generic JR; Timer/PI approximations; historical T03 E
PATCH35_ACCEPTED: YES
PATCH4_READY: YES
SOURCE_CODE_CHANGED: NO
EVIDENCE_CREATED: docs/plan/evidence/p08-preemption-patch35-rcp-independent-review.md
SESSION_HANDOFF_UPDATED: YES
GLOBAL_HANDOFF_UPDATED: YES
PHASE_P08_UPDATED: YES
SPECS_REVIEWED: recomp-readiness, rsp-and-audio, build, matching
SPECS_CHANGED: recomp-readiness, rsp-and-audio (stable review/gate state only)
MARKDOWN_VAULT_AUDIT: PASS (0 errors; new evidence 100/100;
                           repository has 49 pre-existing warnings)
DOC_COMMIT: docs(p08): review per-thread RCP correction (this commit)
PUSHED: YES (origin/main; verified after commit)
NEW_HEAD: this document's containing commit; exact SHA in final handoff
WORKTREE: clean after commit/push
NEXT_SINGLE_ACTION: implement PATCH 4 activation/regeneration/validation only

=== END HANDOFF ===
```
