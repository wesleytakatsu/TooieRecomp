---
title: "P08 — PATCH 3.5 thread-switch eligibility review"
description: "Targeted audit: can a held hardware interrupt become eligible solely because the current guest OSThread changed? Verdict + minimum retry point. Docs only."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch35, held-ineligible, thread-switch, interrupt-cause, n64modernruntime]
---

# P08 — PATCH 3.5 THREAD-SWITCH ELIGIBILITY REVIEW

```text
MODE:                 READ / ANALYZE / TEST / DOCS ONLY
MODEL:                DeepSeek V4 Pro 0813 MAX
ROLE:                 Principal N64/libultra scheduler architect / concurrency reviewer
BASE_HEAD:            d6b0ab35c64d3bcca4cca7d506a888c983ecb480
RUNTIME_HEAD:         ca568b6ad79b9029d14077f0c3ffa757727c5559
N64RECOMP_HEAD:       2b6f05688de2abc7d86da5b4a89b84c2c6acbabe
ARCHITECTURE:         Hybrid F' (not reopened)
VERDICT:              B - one thread-dispatch eligibility addition required
SOURCE_CODE_CHANGED:  NO
```

This review audits one potentially missing semantic case in the approved A+
ANNOTATED_HOLD_UNTIL_ELIGIBLE design ([[p08-preemption-patch35-design]]):
whether a held hardware interrupt can become eligible **solely** because the
current guest OSThread changed. It does not implement anything and does not
reopen settled results.

Primary sources read (on disk):

- `banjo-tooie-Recomp/lib/ultralib/src/os/exceptasm.s`
- `banjo-tooie-Recomp/lib/ultralib/src/os/setintmask.s`
- `banjo-tooie-Recomp/lib/ultralib/src/os/interrupt.s`
- `banjo-tooie-Recomp/lib/ultralib/src/os/createthread.c`
- `BanjoRecomp/lib/N64ModernRuntime/librecomp/src/recomp.cpp` (+ `recomp.h`)
- `BanjoRecomp/lib/N64ModernRuntime/ultramodern/src/threads.cpp`
- `BanjoRecomp/lib/N64ModernRuntime/ultramodern/src/scheduling.cpp`
- `BanjoRecomp/lib/N64ModernRuntime/ultramodern/src/mesgqueue.cpp`
- `BanjoRecomp/lib/N64ModernRuntime/ultramodern/src/preemption.cpp`
- `BanjoRecomp/lib/N64ModernRuntime/ultramodern/src/intmask.cpp`

---

## 1. PRIMARY QUESTION - ANSWER

> Can a held hardware interrupt become eligible solely because the current
> guest OSThread changed?

**YES - the eligibility state is genuinely a function of the current thread.**

The eligibility transition on thread change is real on both levels:

1. **Runtime level.** `recomp_context.status_reg` (`recomp.h:426`) is a stack
   local inside `run_thread_function` (`recomp.cpp:484-503`). Each guest
   `OSThread` owns one native `std::thread` (`threads.cpp:260-262`,
   `osCreateThread` -> `_thread_func`), and each native thread constructs its
   own `recomp_context` with its own `status_reg`. `osSetIntMask` writes only
   the current thread's `ctx->status_reg` IM bits
   (`ultra_translation.cpp:121-123` -> `intmask.cpp:24-43`). Thread A can hold
   `IM2=0` while thread B holds `IM2=1`.

2. **Hardware level.** libultra restores the full mask state **per-thread** at
   dispatch: `__osDispatchThreadSave` (`exceptasm.s:1053-1062`) loads the new
   thread's saved `THREAD_SR`, masks its IM bits by `__OSGlobalIntMask`, and
   `mtc0`s it into CP0 SR; then (`exceptasm.s:1184-1200`) it restores the new
   thread's `THREAD_RCP` into `MI_INTR_MASK_REG` and `eret`s. `osSetIntMask`
   (`setintmask.s:13-54`) writes **both** `MI_INTR_MASK_REG` and the current
   SR.IM, so the device mask is likewise saved/restored per-thread. A pending
   `MI_INTR.<cause>` bit is device-global and **latched**: a masked-away cause
   is never acknowledged (its `rcp:` branch is never entered - see
   `handle_interrupt`, `exceptasm.s:598-612`, which masks `CAUSE` by `SR.IM`
   before dispatch), so it stays pending until a thread whose masks allow it
   runs.

Consequently: a pending AI masked under A, held in `held_ineligible`, becomes
eligible when the scheduler dispatches B (SR.IM2 = 1 and RCP mask opens MI.AI).

### 1.1 BUT - is the A+ design actually stranded by this?

The literal scenario in the prompt is **NOT stranded under the A+ design as
written**, while a narrower residual **is** stranded. See section 5. The gap is
real in the eligibility transition, but the A+ model already re-evaluates held
causes on thread change through the arrival-publish + per-thread TLS epoch
mechanism - *provided* two invariants hold. Those invariants are currently
implied, not stated, and one drain path violates them.

---

## 2. Original N64 semantics (primary evidence)

```text
PENDING_CAUSE_REEVALUATED_ON_THREAD_DISPATCH: YES
INTERRUPT_CAN_FIRE_AFTER_ERET:               YES (architectural; next instruction boundary)
```

Reconstruction of the exact sequence (A masked -> A blocks -> dispatch B ->
B.SR eligible -> eret):

1. A masks AI via `osSetIntMask` (`setintmask.s:49 mtc0 ta0, C0_SR`; the same
   call also writes `MI_INTR_MASK_REG` at `setintmask.s:40`). A's SR.IM2 = 0.
2. An AI completion sets `MI_INTR.AI`. RCP raises IP2 only if
   `MI_INTR_MASK.AI` allows it; the CPU takes the exception only if
   `SR.IE && !EXL && !ERL && SR.IM[2]`. A has IM2 = 0, so no exception is
   taken, and the AI bit is **never acknowledged** - it stays latched.
3. A blocks (`osRecvMesg` etc.) -> the yield/exception save path saves A's
   SR.IM and the current `MI_INTR_MASK_REG` into A's `THREAD_SR` /
   `THREAD_RCP` (`exceptasm.s:934-997`).
4. `__osDispatchThread` pops B, restores B's `THREAD_SR` (IM2 = 1) into CP0 SR
   (`exceptasm.s:1053-1062`) and B's `THREAD_RCP` into `MI_INTR_MASK_REG`
   (`exceptasm.s:1184-1200`), then `eret`.
5. After `eret`, EXL clears and the latched `MI_INTR.AI` now raises IP2,
   which is unmasked in B's restored SR. The R4300 recognizes the pending
   eligible interrupt at the **next instruction boundary** - before any
   meaningful B guest execution.
6. `handle_interrupt` -> `rcp:` -> `send_mesg(OS_EVENT_AI)` wakes AUDIO;
   `redispatch` (`exceptasm.s:844-856`) switches to AUDIO (higher priority).

```text
class: PROVEN (from exceptasm.s / setintmask.s / interrupt.s on disk)
```

Key architectural fact: **SR.IM and MI_INTR_MASK are both saved/restored
per-thread, and the pending cause is device-global and latched.** Eligibility
is re-evaluated at every dispatch, not only at a mask instruction.

---

## 3. Per-thread status model (runtime)

```text
STATUS_REG_PER_GUEST_THREAD:            YES
CURRENT_RUNNING_SR_CHANGES_ON_SWAP:     YES
```

Trace: `osCreateThread` (`threads.cpp:247-267`) spawns one native
`std::thread` running `_thread_func` (`threads.cpp:185`). `_thread_func` sets
`thread_self` and calls `run_thread_function` (`recomp.cpp:484`), which builds
`recomp_context ctx{}` and runs the guest function on that native stack. The
`status_reg` field lives in `ctx` (`recomp.h:426`). A "swap" is not a native
thread migration: `swap_to_thread` (`scheduling.cpp:9-16`) inserts the current
thread into the running queue and blocks it on its own `running` semaphore
while the target native thread resumes with **its own** `recomp_context`.
Therefore the "current" SR changes whenever the token changes hands, even
though no mask instruction executed.

---

## 4. Where the current design retries held causes

The A+ design retries `held_ineligible` from exactly two trigger classes:

| trigger | site | section |
| --- | --- | --- |
| interrupt state change (same guest) | `after_interrupt_state_change` -> `service_preemption` | design 4.5 |
| new external epoch | `service_preemption` drain (passes over `held_ineligible` first) | design 4.4 step 2 |

The cooperative paths (`osRecvMesg`/`osSendMesg`/`osJamMesg`/`wait_for_external_message*`)
are required to "use the same drain/hold rules" (design 6), which by 4.4
includes the held pass.

A **thread change** is a third trigger class: the current SR changes from A's
to B's. The A+ design does not name it explicitly.

---

## 5. TLS epoch - proof attempt

```text
TLS_EPOCH_GUARANTEES_HELD_RETRY_ON_THREAD_CHANGE: YES (conditional - see I1/I2)
```

The prompt's counterexample ("B may have `tls_last_seen == current epoch`
already") is **REFUTED** under the stated single-token model. Proof:

Let `E_arrive` be the epoch immediately after a hardware event's most recent
enqueue (arrival or full-queue requeue). `enqueue_external_message` always
publishes after enqueueing (`mesgqueue.cpp:18-21`), so
`enqueue happens-before publish`. A drain that moves the event to
`held_ineligible` does **not** publish (design 4.2), so it does not advance the
epoch.

Claim: when a held event is eligible under the newly-current thread B, either
B's hint is true or B already processed the event itself.

- If B has not run since `E_arrive`, then `tls_last_seen[B] < E_arrive <=
  current_epoch`, so `hint(B)` is true and B's `service_preemption` retries
  the global held list (design 4.4 step 2).
- If B has run since `E_arrive`, then B drained the external queue after the
  arrival (release/acquire ordering makes the enqueued event visible), so B
  either delivered it (eligible) or held it (ineligible under B). In neither
  case can the event be simultaneously "held, not delivered, and eligible
  under B". If B later becomes eligible, that requires a B-side mask change,
  which fires `after_interrupt_state_change`.

The specific sequence ("AI arrives -> A holds -> A blocks -> B runs with
`last_seen == epoch`") is impossible because single-token execution forbids B
from servicing the arrival epoch between the arrival and A's hold.

```text
TLS_EPOCH_GUARANTEES_HELD_RETRY: YES with invariant
INVARIANT (I1): every entry into held_ineligible is preceded (happens-before)
                 by a publish whose epoch the eligible thread has not yet seen.
INVARIANT (I2): every drain path (service AND cooperative AND idle/wait)
                 retries the GLOBAL held_ineligible under the CURRENT SR.
```

The guarantee is real but **fragile**: I1 and I2 are only implied by the design
text ("same drain/hold rules", "enqueue publishes"), not stated as required
invariants, and are not covered by any test. Any future path that moves an
event to held without a preceding publish, or any drain that forgets the held
pass, silently reopens the gap. The `wait_for_external_message` idle path
already violates I2 (see 7.1).

---

## 6. Held-list ownership

```text
HELD_LIST_SHOULD_BE_GLOBAL: YES
HELD_LIST_THREAD_SAFE:       SAFE (process-global vector, single-token ownership)
```

A hardware interrupt cause is device-global; it is not owned by the thread that
happened to observe it first. The runtime runs guest code under a single-token
model (exactly one game thread executes guest code at a time; `swap_to_thread`
blocks the outgoing thread and resumes exactly one incoming thread). A
process-global `std::vector<QueuedMessage> held_ineligible` touched only by the
token holder is therefore race-free. It must **not** be per-native-thread
(`thread_local`): a held cause must be visible to whatever thread next holds the
token.

---

## 7. Thread-switch retry options - ranking

| rank | option | assessment |
| ---: | --- | --- |
| 1 | **E/A - retry held at native-thread resume** (`wait_for_resumed`) | hardware-faithful (matches eret re-evaluation); single chokepoint; makes I1/I2 unnecessary for correctness |
| 2 | D - status/mask signature comparison | precise but more state; not needed given E |
| 3 | B - separate `interrupt_eligibility_epoch` | works but delays to next safepoint + second hint channel |
| 4 | C - global hint stays true while held non-empty | **REJECT** - permanent hot poll while a cause stays masked |

**Recommended: E** - one central primitive called from both trigger classes,
reusing the existing held-retry algorithm (do not duplicate it).

### 7.1 Residual gap - the idle/self-pause path (real, not thread-switch)

`wait_for_external_message` (`mesgqueue.cpp:39-46`) blocks in
`external_messages.wait_dequeue`. If `external_messages` is empty while
`held_ineligible` contains a cause now eligible under the current SR, the
thread parks forever until the next external publish. Design 6 only states "if
it pops an ineligible message, hold it and wait again" - it does not retry the
existing held list after wake. This is a same-thread residual, not the
thread-switch case, but the resume-time retry closes it in the same motion: on
resume the thread re-evaluates held under its current SR.

---

## 8. Reentrancy / token safety

```text
RESUME_HOOK_CAN_RESCHEDULE: YES (with placement conditions)
```

At the resume point (after `thread_context->running.wait()` returns in
`wait_for_resumed`, `threads.cpp:149-156`):

- the resumed native thread owns the token (dequeued from the running queue
  before being signaled);
- no runtime mutex is held (the `LightweightSemaphore` has released its
  internal synchronization);
- the thread's `recomp_context` (and `status_reg`) is alive on the native
  stack below `run_thread_function`;
- `is_game_thread()` and `thread_self` are valid (set in `_thread_func:188`).

A service that wakes a still-higher-priority thread C and swaps to it re-blocks
the resumed thread inside its own `resume_thread_and_wait` ->
`wait_for_resumed`, a nested re-block of the same `LightweightSemaphore`. This
is the same pattern the scheduler already uses (e.g. `osStartThread` ->
`check_running_queue` -> `swap_to_thread`); it is not a reentrant lock. The
`assert(is_game_thread())` in `service_preemption` holds. Conditions: the hook
must skip when `tls_status_reg == nullptr` (the first-time-start path in
`_thread_func:202`, before `run_thread_function` sets the TLS), and must run
after the context-replacement check at `threads.cpp:152-155` so it does not
service a thread about to be destroyed.

---

## 9. Exact host safe point

```text
RECOMMENDED_THREAD_RESUME_HOOK:
  ultramodern/src/threads.cpp : wait_for_resumed
  position: after `thread_context->running.wait();` returns AND after the
            context-replacement check (threads.cpp:152-155)
```

`wait_for_resumed` is the single funnel for every cooperative resume:
`run_next_thread_and_wait` (`threads.cpp:173-177`), `resume_thread_and_wait`
(`threads.cpp:179-183`), and the initial start (`_thread_func:202`). It covers
`osRecvMesg`/`osSendMesg`/`osJamMesg` blocking paths, `osYieldThread`,
`pause_self`, `osStartThread`, `osSetThreadPri`, thread exit, and preemption
swaps - one chokepoint.

> **Constraint conflict:** the current design marks `threads.cpp` "do not touch
> (protected dirty)". This review **amends** that rule: the dirty NULLPTR guard
> (`threads.cpp:305-320`, `osSetThreadPri`) remains untouched and unfastened,
> but a minimal resume hook in `wait_for_resumed` is **required** by this
> review. See the design-delta (section 11).

---

## 10. Status pointer availability at the hook

```text
STATUS_POINTER_AVAILABLE_AT_HOOK: YES
```

The design already proposes `thread_local` `tls_rdram` / `tls_status_reg`, set
in `run_thread_function` before `func(rdram, &ctx)` (design 4.6). The resume
hook runs on the **resumed** native thread, so it reads that thread's own TLS -
the correct current `status_reg` - without any `recomp_context*` crossing the
scheduler API. When `tls_status_reg == nullptr` (initial start), skip.

---

## 11. Held events + cooperative blocking (chokepoint audit)

```text
COMMON_RESUME_POINT_COVERS_ALL_PATHS: YES
```

Every path that changes the currently executing OSThread funnels through
`wait_for_resumed`:

| path | swap mechanism | funnels through |
| --- | --- | --- |
| `osRecvMesg` (block, empty) | `do_recv` -> `run_next_thread_and_wait` | `wait_for_resumed` |
| `osSendMesg`/`osJamMesg` (block, full) | `do_send` -> `run_next_thread_and_wait` | `wait_for_resumed` |
| `osYieldThread` / `pause_self` | `check_running_queue` -> `swap_to_thread` | `wait_for_resumed` |
| `osStartThread` | `check_running_queue` -> `swap_to_thread` | `wait_for_resumed` |
| `osSetThreadPri` | `check_running_queue` -> `swap_to_thread` | `wait_for_resumed` |
| `osStopThread`/`osDestroyThread` self | `run_next_thread_and_wait` | `wait_for_resumed` |
| thread exit | `run_next_thread` | `wait_for_resumed` (next thread) |
| preemption service | `check_running_queue` -> `swap_to_thread` | `wait_for_resumed` |

One resume hook therefore re-evaluates held under the newly-current SR for all
of them.

---

## 12. Design delta (minimal addition to the A+ design)

```text
NEW_TRIGGER: guest-thread resume / current-SR change

NEW_PRIMITIVE:
  service_pending_interrupts_for_current_guest(RDRAM_ARG1)
    = retry held_ineligible (+ drain external) under tls_status_reg
      + check_running_queue if preemptible
    (same algorithm as after_interrupt_state_change / service_preemption
     - one implementation, two call sites)

NEW_CALL_SITE:
  ultramodern/src/threads.cpp :: wait_for_resumed
    after running.wait() returns, after context-replacement check
    -> service_pending_interrupts_for_current_guest()

NEW_TEST:
  X1-G THREAD-DISPATCH-UNMASK (design 10.1 extension)

EXISTING A+ MODEL: otherwise unchanged (hold list, cause map, eligibility
predicate, mask-change hook, MTC0 hook all stay as specified)
```

The addition is cheap, centralizes on one chokepoint, and makes the retry
invariant explicit and robust rather than dependent on the (correct but
unstated) arrival-publish argument.

---

## 13. Reverse case (stale thread status)

```text
STALE_THREAD_STATUS_RISK: NO
```

Eligibility must be evaluated against the CURRENT thread's SR at delivery time.
The design's `cause_eligible(status_reg, cause)` and `after_interrupt_state_change`
already take the current `status_reg` (via generated `&ctx->status_reg` or TLS).
As long as the implementation does not cache/snapshot the SR across the drain,
an event cannot be delivered using stale A-eligibility after B becomes current.
The resume hook makes this explicit by always using `tls_status_reg`.

---

## 14. Held ordering / multiplicity

```text
HELD_HEAD_OF_LINE_BLOCKING: NO
MULTIPLICITY_PRESERVED:      YES
```

The drain iterates the whole held list and delivers every eligible cause while
re-holding ineligible ones, so an older ineligible AI does not head-of-line
block a later eligible VI. Hardware itself uses cause priority order, not FIFO.
Per-event hold (not bitmap) preserves multiplicity; the resume hook reuses the
same move-out semantics, so no duplicate/drop/collapse.

---

## 15. Feature-off rollback

```text
FLAG_CAN_CHANGE_AT_RUNTIME: YES (public atomic setter), but treat as
                            startup-immutable by contract (set at preinit,
                            before any guest thread runs).
IF_DISABLED_WITH_HELD_EVENTS: must flush held_ineligible back onto
                              external_messages (legacy drain-all), else held
                              events strand forever under the OFF path.
```

`ULTRAMODERN_GUEST_PREEMPTION=0` is read once at preinit (implementation plan
1), so the env path is already startup-immutable. Document the flush rule for
defensiveness; do not add a dynamic toggle.

---

## 16. Secondary reviews

```text
TIMER_IM7_GATE: GOOD_ENOUGH
  Labeling the runtime timer message as InterruptCause::Timer gated on IM7 is
  the correct minimal analog (hardware OS_EVENT_COUNTER is gated on CP0 IM7).
  The runtime does not model C0_COMPARE, but gating direction is right: a timer
  firing under OS_IM_NONE (IM7 clear) must not become guest-visible. Low
  regression risk for Tooie (gating is only stricter, matching hardware).

PI_COMPLETION_CAUSE_MODEL: CONDITIONAL
  do_dma / flash send the caller's completion directly; on hardware that
  message is only delivered when the PI interrupt is taken (IM2 + MI.PI).
  Gating on PI is closer than immediate delivery. It is still an approximation
  (not OS_EVENT_PI), but directionally correct. No redesign.
```

---

## 17. Classification

```text
A_PLUS_ARCHITECTURE_STATUS: CORRECTED (minor - one explicit trigger added)
MINIMAL_DESIGN_DELTA:       one trigger + one primitive + one call site + one test
THREAD_SWITCH_GAP_REAL:     CONDITIONAL (eligibility transition real; literal
                            scenario already covered by arrival-publish + TLS
                            epoch; idle/wait path + unstated invariant are the
                            true residual - closed by the resume hook)
```

Verdict **B** (not A): the design needs one thread-dispatch eligibility
addition, not a larger revision. All other deferred items stay deferred.

---

## 18. Handoff summary

```text
VERDICT: B
PENDING_CAUSE_REEVALUATED_ON_THREAD_DISPATCH: YES
INTERRUPT_CAN_FIRE_AFTER_ERET: YES
STATUS_REG_PER_GUEST_THREAD: YES
CURRENT_RUNNING_SR_CHANGES_ON_SWAP: YES
TLS_EPOCH_GUARANTEES_HELD_RETRY: YES (conditional on I1/I2)
HELD_LIST_OWNERSHIP: process-global, single-token
HELD_LIST_THREAD_SAFE: SAFE
THREAD_SWITCH_GAP_REAL: CONDITIONAL (see 17)
RECOMMENDED_THREAD_RESUME_HOOK: threads.cpp:wait_for_resumed
RESUME_HOOK_CAN_RESCHEDULE: YES
STATUS_POINTER_AVAILABLE_AT_HOOK: YES
COMMON_ELIGIBILITY_SERVICE: service_pending_interrupts_for_current_guest()
X1_G_REQUIRED: YES
HELD_HEAD_OF_LINE_BLOCKING: NO
MULTIPLICITY_PRESERVED: YES
FLAG_RUNTIME_TRANSITION: startup-immutable by contract; flush on disable
TIMER_IM7_GATE: GOOD_ENOUGH
PI_COMPLETION_CAUSE_MODEL: CONDITIONAL
A_PLUS_ARCHITECTURE_STATUS: CORRECTED
PATCH4_READY: NO
```

```text
NEXT_SINGLE_ACTION:
  fold the thread-dispatch resume retry (section 12) into the PATCH 3.5 design
  doc, then implement PATCH 3.5 with it. Do not enable the flag. Do not regen.
```
