---
title: "P08 — independent review of preemption PATCH 3.5"
description: "Independent high-risk review of PATCH 3.5, centered on per-OSThread RCP/MI mask state."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch35, independent-review, libultra, rcp, mi]
---

# P08 — INDEPENDENT REVIEW OF PREEMPTION PATCH 3.5

```text
BASE_HEAD:       baeae496d0836e522bf9bf91589d124a1db1feee
MODEL:           GPT-5.6 SOL HIGH
MODE:            READ / TEST / DOCS ONLY
VERDICT:         C — PATCH 3.5 CORRECTION REQUIRED BEFORE PATCH 4
PATCH4_READY:    NO
SOURCE_CHANGED:  NO
```

> **RESOLVED pointer (2026-08-13):** implementer applied the recommended TLS
> `current_mi_mask` fix and added RCP-X1..X4 plus P3-X6+. The correction was
> independently approved with verdict B in
> [[p08-preemption-patch35-rcp-independent-review]]. This verdict C remains
> the authority for the pre-correction defect and is not rewritten.

This review treated the PATCH 3.5 handoff and its PASS list as claims. It
reconstructed the exact artifacts, inspected current source, checked primary
libultra, and added two disposable review-only discriminators under `/tmp`.
No implementation fix, PATCH 4 work, production enable, Tooie regeneration,
or `func_800BCF28` investigation was performed.

## 1. Blocking finding

```text
ROOT_DEFECT:
  PATCH 3.5 models the current MI/RCP interrupt mask as process-global
  ultramodern::intmask::g_mi_mask. Libultra saves and restores that mask in
  each OSThread context. A thread switch can therefore leave the resumed
  guest thread observing the previous thread's RCP mask.

CURRENT_MI_MASK_STORAGE:              GLOBAL
MI_MASK_SAVED_ON_RUNTIME_THREAD_BLOCK: NO
MI_MASK_RESTORED_ON_RUNTIME_RESUME:    NO
RCP_INT_MASK_STORAGE:                  WRONG
GLOBAL_INT_MASK_STORAGE:               CORRECT
```

Current runtime source:

- `ultramodern/src/intmask.cpp:11`: `g_mi_mask = 0x3F` is an ordinary
  namespace-global scalar; only `tls_rdram`, `tls_status_reg`, and the epoch
  cursor are thread-local elsewhere.
- `intmask.cpp:17-22`: `current_combined_mask` returns RCP bits from the same
  global scalar.
- `intmask.cpp:24-39`: `apply_os_set_int_mask` reads the old combined mask and
  overwrites the global scalar.
- `intmask.cpp:71-99`: `cause_eligible` gates every RCP cause through the
  global scalar.
- `threads.cpp:149-162`: the resume hook retries pending causes but restores
  no RCP state.
- `recomp.cpp:488-512`: each guest function owns a per-native-thread
  `recomp_context` and per-thread `status_reg`; no corresponding RCP field is
  saved or restored.

A full-source search found no alternate `g_mi_mask` save/restore mechanism.

## 2. Primary libultra contract

Primary sources were read from
`banjo-tooie-Recomp/lib/ultralib/src/os/`.

### `osSetIntMask`

`setintmask.s:13-54`:

- lines 14-34 assemble the returned old CPU and RCP mask from CP0 Status and
  `MI_INTR_MASK_REG`, corrected for `__OSGlobalIntMask`;
- lines 36-40 translate the requested RCP bits and write
  `MI_INTR_MASK_REG`;
- lines 42-49 apply requested CPU IM bits and write CP0 Status;
- IE is returned/preserved rather than changed by `osSetIntMask`.

Thus the hardware MI mask changed by `osSetIntMask` is the value later saved
for the currently running OSThread.

### Yield/block save

`exceptasm.s:934-997` (`__osEnqueueAndYield`):

- lines 938-941 read CP0 Status, set EXL in the saved snapshot, and store
  `THREAD_SR`;
- lines 969-985 fold global-mask state into the saved SR representation;
- lines 986-997 read `MI_INTR_MASK_REG`, fold global-mask state into it, and
  store `THREAD_RCP`.

### Dispatch restore

`exceptasm.s:1053-1062` (`__osDispatchThreadSave`) loads the selected
thread's `THREAD_SR`, intersects its IM bits with `__OSGlobalIntMask`, and
writes CP0 Status.

`exceptasm.s:1184-1200` loads the selected thread's `THREAD_RCP`, intersects
it with the global RCP allowance, translates it through `__osRcpImTable`,
writes `MI_INTR_MASK_REG`, and executes `eret`.

### Initial thread state and global mask

`createthread.c:29-32` initializes every new thread from `OS_IM_ALL`:

```text
THREAD_SR  = (OS_IM_ALL & (SR_IMASK | SR_IE)) | SR_EXL
THREAD_RCP = (OS_IM_ALL & RCP_IMASK) >> RCP_IMASKSHIFT
```

`interrupt.s:7-51` and the dispatch paths treat `__OSGlobalIntMask` as a
single global policy mask layered over the current thread's saved mask. It
is not a replacement for `THREAD_RCP`.

```text
RCP_MASK_IS_PER_OSTHREAD_IN_LIBULTRA: YES
```

## 3. Disposable discriminators

Review tree:
`/tmp/p08-p35-independent-review-oYP7WP/script`.
The test edits were not committed and do not exist in TooieRecomp or either
dependency worktree.

Baseline reconstructed harness before test edits:

```text
PATCH35 PASS (pass=36 fail=0 skipped=0)
X1-G PASS
P3-X6 PASS
```

### RCP-X1 / requested RCP-only X1-G

Both guest threads kept IE=1 and SR.IM2=1. B set MI.AI open, yielded to A,
and A set only MI.AI closed. An AI event was drained under A and entered
`held_ineligible`. A blocked and B resumed.

Expected from libultra: B's RCP mask becomes current, the resume hook retries
the held AI, and AUDIO runs.

Observed:

```text
RCP-X1 FAIL
audio_ran timeout
held_ineligible_count remained nonzero
```

```text
X1_G_RCP_ONLY: FAIL
```

### RCP-X2 / `osSetIntMask` old return

B set MI.AI open, yielded to A, A set MI.AI closed, and B resumed and called
`osSetIntMask` again. The old mask should contain B's prior MI.AI bit.

Observed:

```text
RCP-X2 FAIL
(old & 0x00040000) == 0
```

The returned RCP bits came from A's process-global mask.

```text
PER_THREAD_OSSETINTMASK_RETURN: FAIL
```

## 4. Existing X1-G coverage

`preemption_harness.cpp:400-426` says explicitly:

> Mask AI via this thread's SR.IM2 only so process-global MI stays open.

The test varies A/B `status_reg` pointers and therefore proves that the
resume hook re-evaluates per-thread SR state. It deliberately leaves the
RCP dimension globally open and cannot detect the blocking defect.

```text
CURRENT_X1_G_TESTS_SR_THREAD_STATE:  YES
CURRENT_X1_G_TESTS_RCP_THREAD_STATE: NO
DOES_X1_G_AVOID_GLOBAL_MI_PROBLEM:   YES
CLASSIFICATION: TEST COVERAGE GAP
```

Keep X1-G. Add separate RCP tests rather than replacing its useful SR proof.

## 5. Minimum correction analysis

### A. Native TLS — recommended minimum

Use one `thread_local uint32_t` current RCP mask, initialized to `0x3F`, in
place of the process-global `g_mi_mask`.

Pros:

- matches the runtime's present one-guest-OSThread-to-one-native-thread
  representation;
- native suspension/resume automatically preserves the guest RCP mask;
- fixes eligibility and `osSetIntMask` old-return together;
- no `recomp_context` ABI/generator change;
- each first-start guest thread and the bootstrap native thread receive the
  libultra `OS_IM_ALL` RCP default naturally;
- fork-per-test harness isolation remains unchanged.

Constraints:

- intmask helpers that read/write the current mask are guest-thread/current-
  context operations; host producer threads must not call them as if they
  represented a guest;
- this representation depends on the current no-migration scheduler model;
  document that dependency.

### B. `recomp_context`

Architecturally colocates SR and RCP, but changes an anonymous generated ABI,
touches static/live generators, and expands PATCH 3.5 correction scope. It is
not the minimum correction while guest native-thread affinity is guaranteed.

### C. `UltraThreadContext` or OSThread-keyed map

Avoids reliance on TLS affinity and would support future migration, but adds
explicit save/restore plumbing, lifecycle synchronization, and lookup state.
It is unnecessary for the current scheduler.

### Native-thread affinity proof

- `threads.cpp:253-273`: every `osCreateThread` allocates one
  `UltraThreadContext` and constructs one `std::thread` running `_thread_func`;
- `_thread_func` (`threads.cpp:191-234`) waits and resumes on that same native
  thread, then calls `run_thread_function` once;
- `wait_for_resumed` parks/resumes the native thread via its context semaphore;
- `osDestroyThread` signals the same context or lets the owning thread unwind;
- recreating an OSThread creates a new context/native thread; it does not move
  a live guest context between native threads.

```text
OSTHREAD_NATIVE_THREAD_1_TO_1: YES (for each current OSThread lifetime)
RECOMMENDED_RCP_STATE_MODEL:   thread_local uint32_t, initial 0x3F
```

Future guest-context migration would invalidate the TLS shortcut and require
model B or C.

### Resume hook after the correction

With RCP state in native TLS, B's own mask is already current when
`wait_for_resumed` returns. The existing call to
`service_pending_interrupts_for_current_guest` then evaluates the held list
under B's SR and B's RCP mask. No explicit `restore_current_rcp_mask()` call
is needed.

```text
THREAD_RESUME_HOOK_VALID: CONDITIONAL — valid once RCP state is per-thread
```

## 6. Cause, queue, and wait-path audit

No additional blocking error was found in the A+ mechanics after separating
the RCP storage defect:

- causes `None`, `SP`, `SI`, `AI`, `VI`, `PI`, `DP`, and `Timer` are preserved;
- `None` is always eligible;
- RCP causes require IE, no EXL/ERL, SR.IM2, current RCP mask, and the global
  allowance;
- Timer uses the documented IM7 approximation;
- `held_ineligible` is process-global and older masked causes do not block
  later eligible causes;
- full-queue requeue preserves cause and does not enter the held class;
- masked causes are not inserted into guest queues.

Feature-ON wait loops:

- retry eligible held work before sleeping;
- block when the only state is masked-held, avoiding spin;
- return on full requeue without treating it as held interrupt delivery;
- compute one steady-clock deadline and preserve it across timed retries;
- remove delivered entries from the held list before a nested resume service,
  preventing duplicate delivery.

```text
WAIT_PATHS: PASS
```

## 7. Held-vector setter race

`held_ineligible` has no lock. Normal service/drain access is serialized by
the single guest token. `set_guest_preemption_safepoints(false)`, however, is
a public API and directly calls `flush_held_ineligible_to_external`, which
iterates and clears that vector without acquiring the guest token.

In-tree production has no setter caller; the flag is still OFF. Harness tests
toggle it before starting guest threads and each case runs in a forked child.
Those usages are quiescent. A concurrent external caller could race a guest
drain.

```text
FLAG_SETTER_THREAD_CONTRACT:
  startup-immutable in production; tests/quiescent owner only; not enforced
HELD_VECTOR_DATA_RACE_POSSIBLE:
  CONDITIONAL — yes if the public setter transitions ON->OFF concurrently
  with guest service; no in current in-tree usage
```

This is nonblocking for the per-thread RCP correction, but the contract should
be documented and preferably asserted or structurally enforced before runtime
mutation is supported.

## 8. Producer annotation audit

All production `enqueue_external_message` callers were enumerated:

```text
events.cpp: VI -> VI; AI -> AI; SP -> SP; DP -> DP; SI -> SI
pi.cpp:      ROM/SRAM DMA completion -> PI
flash.cpp:   read/write completion -> PI
timer.cpp:   timer expiration -> Timer
mesgqueue.cpp host osSendMesg/osJamMesg forwarding -> default None
```

The default-`None` calls are generic software messages, not hardware
completion producers.

```text
UNANNOTATED_HARDWARE_CALLERS: NONE
```

## 9. Test-quality audit

```text
X1-A/B:
  Uses real apply_os_set_int_mask, queue drain, held list, and unmask hook.
  Good same-thread cause test, but cannot prove per-thread RCP isolation.

X1-C/D/E:
  Use real cause metadata, service, disable/restore helpers, and scheduler.
  Adequate for their stated dimensions.

T04+:
  Calls the production intmask helper directly on one stack SR. It proves
  same-thread combined-mask arithmetic but has no thread switch and bypasses
  osSetIntMask_recomp/r2 plumbing.

X1-F/M1/M2:
  "MTC0-analog" directly edits the harness SR and calls
  after_interrupt_state_change. M2 also calls that hook directly with the
  feature off. These can pass if production cop0_status_write wiring is
  absent. Static inspection confirms cop0_status_write currently commits the
  new SR then calls the hook, but the harness does not independently prove it.

X1-G:
  Strong SR-dispatch proof; explicit RCP coverage gap described above.
```

```text
X1_TEST_QUALITY: ADEQUATE for covered dimensions, incomplete for per-thread
                 RCP; MTC0 cases are analog-level and have a false-pass path
```

## 10. P3-X6

The harness invokes `toy_generated`, drains a `None` event through the real
ultramodern scheduler, and eventually runs AUDIO. It checks one GPR, HI, LO,
one FPU word, and a native local. It records but does not assert Status and
does not represent/assert `result`, `c1cs`, or switch-addend state.

More importantly, it does not assert that AUDIO ran between the service call
and the post-service state checks. If delivery succeeds but preemptive
`swap_to_thread` is skipped, `toy_generated` can finish, MAIN can later park,
and the ordinary blocking swap can run AUDIO; all current assertions still
pass. The test therefore cannot prove that representative state survived the
claimed real swap at the safepoint.

```text
P3_X6_STRENGTH: INVALID as the claimed context-survival proof
```

This is an independent pre-PATCH4 test-quality issue in addition to the RCP
blocker. Strengthen the test during the PATCH 3.5 correction, without changing
generator behavior.

## 11. Artifact isolation and reruns

Exact hashes:

```text
n64modernruntime-scheduler.patch:
  1b3d3d4666e02f748c44d19d867d02669b83753d85fe7eb62d746a5a8cf38309
n64modernruntime-preemption.patch:
  469537d6def22d267ee72bc6b52c204a3592c0abb024ad140c7e274fac0e5ad7
n64recomp-tooie.patch:
  2045cd90cfae70c881a9504146bac5174ca7a53ba1f5e6200a6a3e4adb5a044c
```

The script was run in a fresh local clone layout from runtime base
`ca568b6ad79b9029d14077f0c3ffa757727c5559` and N64Recomp base
`2b6f05688de2abc7d86da5b4a89b84c2c6acbabe`:

```text
FIRST APPLY:  scheduler, preemption, and generator patches applied
SECOND APPLY: all three reported already applied
24 intended current-source files compared to reconstruction: 0 mismatches
reconstructed baseline harness: PATCH35 PASS (36/0/0)
current PreemptionSafepointTest: T11 PASS (61/0)
```

The scheduler artifact alone contains the `thread_queue_remove` NULLPTR and
`osSetThreadPri(... t->queue != NULLPTR)` fixes. The preemption patch contains
no `threadqueue.cpp` diff and does not own those hunks. Its `threads.cpp` hunk
is only the PATCH 3.5 resume hook against the scheduler-adjusted source.

```text
PATCH_HASHES_VALID:       YES
THREADS_HUNK_OWNERSHIP:   PASS
CLEAN_RECONSTRUCTION:     PASS
PATCH_SCRIPT_IDEMPOTENT:  YES
```

## 12. Required correction before PATCH 4

```text
MINIMAL_CORRECTION:
  Replace process-global current RCP state with per-native-guest-thread TLS,
  initial value 0x3F. Keep g_global_int_mask process-global. Keep the resume
  hook; it requires no explicit restore with TLS.

FILES/FUNCTIONS:
  N64ModernRuntime/ultramodern/src/intmask.cpp
    g_mi_mask storage; current_combined_mask; apply_os_set_int_mask;
    cause_eligible
  N64ModernRuntime/ultramodern/test/preemption_harness.cpp
    add permanent RCP isolation/return/dispatch tests and strengthen P3-X6
  TooieRecomp/patches/runtime/n64modernruntime-preemption.patch
    regenerate cumulative artifact against ca568b6 after scheduler patch
  TooieRecomp/tools/apply_runtime_patches.sh
    update only the validated preemption artifact hash
  validation/plan/handoff docs
    record correction and new evidence; keep production flag OFF

NEW TESTS:
  RCP-X1: A MI.AI masked / B MI.AI open / both SR.IM2 open
  RCP-X2: osSetIntMask old RCP bits are B's after A->B switch
  RCP-X3: three-thread RCP isolation over repeated real swaps
  RCP-X4: held AI follows B's restored RCP eligibility and wakes waiter
  P3-X6+: assert a high-priority thread ran between service and post-service
          state checks; assert every documented representative state field
```

Do not remove existing X1-G. Do not add RCP state to `recomp_context` unless
the one-to-one native-thread invariant is intentionally removed.

## 13. Verdict

```text
VERDICT:          C — PATCH 3.5 CORRECTION REQUIRED BEFORE PATCH 4
PATCH35_ACCEPTED: NO
PATCH4_READY:     NO
BLOCKING_ISSUES:
  1. current RCP/MI mask is process-global instead of per OSThread
  2. RCP-only dispatch and per-thread osSetIntMask old-return tests fail
  3. P3-X6 does not prove context survival across the claimed safepoint swap
NONBLOCKING_ISSUES:
  1. public flag setter can race held vector if runtime-mutated concurrently
  2. MTC0 harness cases test the hook analog, not production wiring end-to-end
NEXT_SINGLE_ACTION:
  Correct PATCH 3.5 RCP/per-thread state only, add RCP-X1..X4 and strengthen
  P3-X6. Keep the feature flag OFF; do not implement PATCH 4 or regenerate
  Tooie.
```

=== END INDEPENDENT REVIEW ===
