---
title: "P08 — PATCH 3.5 semantic-closure design"
description: "Design/arbitration for A+ annotated-hold. Implemented and validated in p08-preemption-patch35-validation. Flag still OFF."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch35, design, n64modernruntime, interrupt-cause]
---

# P08 — PATCH 3.5 SEMANTIC-CLOSURE DESIGN

```text
MODE:                 DESIGN / ARBITRATION / TEST-PLAN ONLY
MODEL:                Grok 4.6 HIGH
ROLE:                 Principal N64/libultra runtime architect
TOOIE_HEAD:           7106cedead5988cfa809067a106afa95cf43031b
RUNTIME_BASE:         ca568b6ad79b9029d14077f0c3ffa757727c5559
N64RECOMP_BASE:       2b6f05688de2abc7d86da5b4a89b84c2c6acbabe
ARCHITECTURE:         Hybrid F' (not reopened)
PATCH35_DESIGN_COMPLETE: YES
PATCH35_IMPLEMENTED:  YES
THREAD_SWITCH_DELTA:   YES (4.8 resume hook; see thread-switch review)
PATCH4_READY:         YES (implementation gates; independent review next)
SOURCE_CODE_CHANGED:  YES
VALIDATION:           [[p08-preemption-patch35-validation]]
```

This document remains the implementation contract. PATCH 3.5 is landed and
validated; do not re-derive X1. Flag remains OFF. Do not regenerate Tooie.

Do not enable the flag. Do not regenerate Tooie. Do not implement PATCH 4.
Do not investigate `func_800BCF28`. DeepSeek Hybrid F remains
`CORRECTED/SUPERSEDED`.

---

## 0. Settled inputs (do not reopen)

```text
ROOT_CAUSE:                 PROVEN (no scheduler opportunity in pure guest loops)
PATCH 1 / 2 / 3:            landed; independent reviews B
PATCH 3:                    compiler capability only
PRODUCTION_FLAG:            OFF
TOOIE_REGEN:                NO
C2AB8_BACKEDGE:             bnel $s3, $zero, L_800C2AF4  (PATCH 3 covers after regen)
BRANCH_LIKELY:              SUPPORTED and PATCH3-correct (PROVEN)
DIRECT_BACKEDGES:           covered
JUMP_TABLE:                 covered
SELF_LOOP:                  pause_self is the runtime point
GENERIC_JR:                 coverage gap (PROVEN as LOOKUP_FUNC tail-call)
CROSS_FUNCTION_CYCLE:       INFERENCE
CONTEXT_SAFE / SWITCH:      CONDITIONAL (missing real-swap proof)
LIVE_ICALL_ABI:             Linux x64 PROVEN; other platforms CONDITIONAL
PREEMPTION_NEEDS_EVENT_CAUSE: YES (PATCH 2 review; RECONFIRMED below)
MTC0_REENABLE_NEEDS_IMMEDIATE_SERVICE: YES
NEW_DOWNSTREAM_BLOCKER:     func_800BCF28
```

---

## 1. Candidate classification

| ID | issue | class |
| --- | --- | --- |
| A | X1 interrupt cause/class fidelity | **MUST_FIX_BEFORE_PATCH4** |
| B | MTC0 Status re-enable scheduling | **MUST_FIX_BEFORE_PATCH4** |
| C | strengthen T04 return/mask proof | **MUST_TEST_BEFORE_PATCH4** |
| D | strengthen T07 event/order proof | **MUST_TEST_BEFORE_PATCH4** |
| E | status-register initialization | **SAFE_TO_DEFER** (KEEP current) |
| F | TLS epoch model | **SAFE_TO_DEFER** (KEEP current) |
| G | generic JR safepoint coverage | **SAFE_TO_DEFER** (document limitation) |
| H | CONTEXT_SAFE real-swap proof | **MUST_TEST_BEFORE_PATCH4** |
| I | switch-value survival proof | **MUST_TEST_BEFORE_PATCH4** |
| J | likely-branch tests in permanent T11 | **MUST_TEST_BEFORE_PATCH4** |
| K | Live SLJIT cross-platform ABI | **SAFE_TO_DEFER** (Linux x64 is the ship target) |
| L | external Generator pure-virtual API | **OUT_OF_SCOPE** (keep pure virtual) |

PATCH 3.5 is **A + B + the MUST_TEST items**. Nothing else is code.

```text
MUST_IMPLEMENT:
  X1 annotated-hold cause model
  MTC0 / mask-change pending service
  producer cause annotations
  cooperative drain uses the same gate when the flag is ON

MUST_TEST:
  X1-A..F, M1, M2, T04+, T07+, P3-X1/X2, P3-X4, P3-X6,
  REQUEUE_CAUSE, NONINTERRUPT, TLS redundant-service harmlessness

SAFE_TO_DEFER:
  status init change
  TLS epoch replacement
  generic JR generator safepoints
  function-entry safepoints
  cheaper hint API / T15
  Live SLJIT Windows/ARM64 execution
  Generator default no-op

EXPLICITLY_NOT_INCLUDED:
  PATCH 4 flag enable
  Tooie regeneration
  func_800BCF28
  DeepSeek Hybrid F
  full exceptasm / CP0 Cause / MI_INTR hardware bits
  exceptasm "once any RCP IP is taken, deliver every pending MI bit"
```

---

## 2. Original interrupt flow (primary libultra)

Sources (this session, on disk):

- `banjo-tooie-Recomp/lib/ultralib/src/os/exceptasm.s`
- `setintmask.s`, `interrupt.s`, `createthread.c`, `initialize.c`

### 2.1 Hardware path

```text
device sets MI_INTR.<class>  (or CP0 Compare / CART / SW)
    -> if MI_INTR_MASK.<class> enables it, RCP raises CPU IP2
    -> if SR.IE && !EXL && !ERL && SR.IM[matching IP]
         exception is taken
    -> exceptasm handle_interrupt:
         s0 = SR & CAUSE                 # only unmasked pending IPs
         dispatch via __osIntOffTable
    -> rcp:
         s1 = MI_INTR & (__OSGlobalIntMask >> 16)
         walk SP, VI, AI, SI, PI, DP in that order
         each taken class: ack hardware, jal send_mesg(OS_EVENT_*)
    -> send_mesg:
         if event mq is null or full: drop
         else write OS_EVENT msg; if a thread is blocked, enqueue it
    -> after all IPs: redispatch
         if peek(__osRunQueue).pri > current.pri: switch
```

`handle_interrupt` (`exceptasm.s:598-612`) computes `s0 = SR & CAUSE` and
only then indexes `__osIntOffTable`. A class whose CPU IM bit is clear is
**not** dispatched.

`rcp:` (`exceptasm.s:676-790`) is reached only when IP2 is in that
unmasked pending set. `send_mesg` (`exceptasm.s:880-918`) is the first
point a guest `OS_EVENT` becomes visible. It is not called from host
producers; it is called from the exception handler.

`__osRestoreInt` (`interrupt.s:44-52`) is `SR |= a0` plus two nops and
`jr ra`. A pending eligible interrupt can be taken on that boundary.
`osSetIntMask` (`setintmask.s:46-53`) writes only `SR.IM` via `mtc0`, then
the same hazard nops. Same boundary.

`osCreateThread` (`createthread.c:32-34`) sets

```text
sr  = (OS_IM_ALL & (SR_IMASK | SR_IE)) | SR_EXL   # 0x0000FF03
rcp = (OS_IM_ALL & RCP_IMASK) >> 16
```

IM is a **fixed** `OS_IM_ALL`, not copied from the creator. After
`__osDispatchThread` / `eret`, EXL clears. Running SR = `IE | IMASK`.

`__OSGlobalIntMask` is initialized to `OS_IM_ALL` (`initialize.c:20`).

### 2.2 MASKED_EVENT_GUEST_VISIBILITY

```text
MASKED_EVENT_GUEST_VISIBILITY: B
```

**B** = remain pending at the interrupt-controller level; become an
`OS_EVENT` message only after the corresponding interrupt is eligible.

Proof:

1. `send_mesg` is only reached from exception-handler leaves (`rcp:`,
   `counter:`, `cart:`, `sw1:`, `sw2:`, `prenmi:`, `handle_break`).
2. Those leaves are only reached after `handle_interrupt` has masked
   `CAUSE` with `SR.IM`.
3. RCP IP2 itself is only raised by `MI_INTR & MI_INTR_MASK`. An AI event
   whose MI bit is masked cannot independently enter `rcp:`.
4. Therefore a masked AI completion is **not** guest-visible as
   `OS_EVENT_AI` and cannot wake AUDIO.

```text
class: PROVEN
```

**Not A.** A is "enqueue the OS_EVENT now, suppress only the context
switch". exceptasm does not do that. `send_mesg` is the visibility
point, and it does not run.

### 2.3 exceptasm quirk we will **not** emulate

Once `rcp:` is entered, the walk of `MI_INTR` is filtered by
`__OSGlobalIntMask` only. It does **not** re-read `MI_INTR_MASK`.
So on hardware, a taken VI interrupt can also deliver a *pending* AI
bit that is MI-masked.

PATCH 3.5 will **not** implement that. Reasons:

- This runtime's AI producer is the VI thread (`events.cpp:246-249`).
  It enqueues an AI message on every started-game VI tick. That is not
  a hardware `MI_INTR.AI` level bit.
- Emulating the quirk would deliver AI whenever VI is eligible, which
  **fails X1-A** in this runtime even when no real AI DMA is pending.
- Observable scheduling fidelity for Tooie needs: "masked AI does not
  wake AUDIO." Strict per-cause is the minimum correct model here.

```text
EXCEPTASM_SHARED_IP2_DELIVERS_MASKED_SIBLINGS: PROVEN in exceptasm
PATCH35_EMULATES_THAT_QUIRK: NO
REASON: runtime AI is not a hardware MI_INTR bit (INFERENCE on
        product risk; the VI-thread coupling is PROVEN)
```

### 2.4 Full-queue policy

exceptasm `send_mesg` **drops** when `validCount >= msgCount`. It does
not block. PATCH 3.5 preserves each producer's existing
`requeue_if_blocked` flag (VI/AI drop; SP/DP/SI/PI/timer/flash requeue).
That extra requeue is a pre-existing runtime generosity, not a new
exceptasm clone.

---

## 3. Design ranking

| rank | design | verdict |
| ---: | --- | --- |
| 1 | **A+ annotated hold-until-eligible** | SELECTED |
| 2 | D two-stage queue | same semantics, more types |
| 3 | B pending bitmap only | REJECT — loses multiplicity (VI drop vs SP requeue; two PI completions) |
| 4 | C gate at producer | **REFUTED** — producer cannot safely read guest SR; later unmask would lose the event |

Design E is unnecessary. A+ is Design A with Design B's acceptance gate
and Design D's hold list, implemented on the existing MPMC.

```text
SELECTED_CAUSE_ARCHITECTURE: A+ ANNOTATED_HOLD_UNTIL_ELIGIBLE
CAUSE_MUST_TRAVEL_WITH_EVENT: YES
```

Cause cannot be recovered from the destination `OSMesgQueue`. The same
guest queue can be registered for different `osSetEventMesg` classes, and
PI/flash/timer already send to caller-supplied queues. Annotate at the
producer.

---

## 4. Selected architecture

### 4.1 Types (conceptual; names may be adjusted)

```cpp
enum class InterruptCause : uint8_t {
    None = 0,   // software / non-hardware host IPC
    SP,         // MI bit 0
    SI,         // MI bit 1
    AI,         // MI bit 2
    VI,         // MI bit 3
    PI,         // MI bit 4
    DP,         // MI bit 5
    Timer,      // CP0 IM7 analog; NOT an RCP class
};

struct QueuedMessage {
    PTR(OSMesgQueue) mq;
    OSMesg mesg;
    bool jam;
    bool requeue_if_blocked;
    InterruptCause cause;   // NEW; default None
};
```

Keep the existing 4-argument `enqueue_external_message` and give `cause`
a default of `None` so un-updated callers remain software.

Add one overload or extra argument only. Do not invent a second MPMC.

### 4.2 Two storage classes

```text
HOST_EVENT_PENDING:
  existing BlockingConcurrentQueue<QueuedMessage> external_messages
  + new game-thread-only vector held_ineligible

GUEST_EVENT_DELIVERY:
  do_send into the guest OSMesgQueue, only when
    flag OFF                              -> always (today)
    OR cause == None                      -> always
    OR cause_eligible(status, cause)      -> then

RUNQUEUE_WAKE:
  existing do_send: if a thread is blocked on recv, schedule_running_thread

THREAD_PREEMPTION:
  async service / after_interrupt_state_change:
    if IE && !EXL && !ERL: check_running_queue()
  cooperative osSend/osJam/osRecv:
    keep today's ungated check_running_queue
    (libultra send can yield while interrupts are disabled)
```

`held_ineligible` is **not** the full-mq requeue path. Full-mq requeues
stay on the MPMC and still `publish()`. Held-ineligible messages must
**not** `publish()`, or every safepoint would spin while AI is masked.

```text
PENDING_INTERRUPT_MODEL:
  per-event annotated queue + hold list
  NOT a collapsed pending bitmap
```

### 4.3 Eligibility predicate

```text
CAUSE_ELIGIBILITY_PREDICATE:

cause_eligible(sr, cause):
  if cause == None:           true
  if (sr & IE) == 0:          false
  if (sr & (EXL|ERL)) != 0:   false
  RCP (SP/SI/AI/VI/PI/DP):
    if (sr & IM2/0x0400) == 0: false
    if (g_mi_mask & mi_bit(cause)) == 0: false
    if ((g_global_int_mask >> 16) & mi_bit(cause)) == 0: false
    true
  Timer:
    (sr & IM7/0x8000) != 0
    AND (g_global_int_mask & IM7) != 0
```

`cpu_preemptible` after PATCH 3.5 (used as the all-masked / IE gate):

```text
IE && !EXL && !ERL
```

Drop "any IM/MI bit open" from the **swap** gate. Hardware messages are
not delivered unless their class is open, so that extra bit is redundant
for interrupt-caused wakes. Software (`None`) wakes may make a higher
thread runnable while all RCP classes are masked; if `IE` is on, the
safepoint may swap. That matches thread-context `osSendMesg`, not an
interrupt.

Keep the current `cpu_preemptible` formula **only** if a test proves a
safepoint swap from a `None` message during `OS_IM_NONE` is observable
and unwanted. Default: simplify as above. Record the choice in the
implementation evidence.

### 4.4 Drain algorithm (flag ON)

```text
had_full_requeue = false
delivered_any    = false

1. Move every try_dequeue from external_messages into a local vector
   (preserve relative pop order).

2. For each message M, in order, including a pass over held_ineligible
   first (older held before newer host arrivals):
     if cause_eligible(sr, M.cause):
       if do_send(M) succeeds: delivered_any = true
       else if M.requeue_if_blocked: full_requeue.push(M)
       else: drop                  # VI/AI full-queue policy
     else:
       held_ineligible.push(M)     # NO publish

3. Re-enqueue full_requeue onto external_messages.
   if !full_requeue.empty(): publish(); had_full_requeue = true

4. if IE && !EXL && !ERL: check_running_queue()

5. Epoch loop unchanged from PATCH 2 (special-case had_requeue && +1).
```

When the flag is OFF: skip the eligibility test. Behavior is today's
drain-all. This is the rollback hatch.

### 4.5 Mask-change hook

```text
MASK_CHANGE_PENDING_SERVICE:

void after_interrupt_state_change(RDRAM_ARG uint32_t* status_reg):
  if !guest_preemption_safepoints_enabled(): return;
  // Retry held + any already-queued host messages under the NEW status.
  service_preemption(rdram, status_reg);
```

Call **after** the new Status / IM / MI / IE has been committed.

| site | when |
| --- | --- |
| `apply_os_set_int_mask` | after IM + MI writes; replace the current `!was && cpu_preemptible` check |
| `restore_int` | after `SR \|= a0`; same replacement |
| `cop0_status_write` | after `ctx->status_reg = new_sr` |
| `disable_int` | **do not** call (only less interruptible) |

`OSSETINTMASK_INTERACTION`: the function already updates MI then IM.
After 3.5 it must retry held causes. A newly unmasked AI with a held AI
event must deliver and may swap.

`RESTORE_INTERACTION`: `SR |= a0` can turn IE on. Held eligible causes
must be serviced on that boundary, not at the next generated safepoint.

### 4.6 MTC0

```text
MTC0_REENABLE_DESIGN: A — call after the new Status is committed
```

Not B (deferred safepoint): a later backedge is not an instruction
boundary. Not C (next generated safepoint): rejected by PATCH 2 review.

`cop0_status_write` currently has no `rdram`. Do **not** change the
live/static callback signature. Add a game-thread TLS:

```text
thread_local uint8_t*  tls_rdram = nullptr;
thread_local uint32_t* tls_status_reg = nullptr;
```

Set both in `run_thread_function` before `func(rdram, &ctx)` and refresh
`tls_status_reg` at the start of `service_preemption`.
`after_interrupt_state_change` uses `tls_rdram` when the caller is
`cop0_status_write`.

If `tls_rdram == nullptr` (should not happen on a game thread), skip the
hook; do not crash.

### 4.7 Cause created / stored / cleared

| question | answer |
| --- | --- |
| WHERE CAUSE IS CREATED | at each host `enqueue_external_message` call site |
| WHERE IT IS STORED | `QueuedMessage.cause` on MPMC and on `held_ineligible` |
| HOW MASK ELIGIBILITY IS TESTED | `cause_eligible` above, game thread only |
| WHEN MESSAGE ENTERS GUEST OSMesgQueue | `do_send` after eligibility |
| WHEN RUNQUEUE IS CHECKED | after drain if IE && !EXL && !ERL; coop API still always |
| WHEN CAUSE IS CLEARED | when `do_send` succeeds or drop-on-full |
| QUEUE FULL | existing `requeue_if_blocked` |
| INTERRUPT REMAINS MASKED | stay in `held_ineligible`; no publish; no guest visibility |
| MASK LATER ENABLED | `after_interrupt_state_change` retries hold list |

### 4.8 Thread-dispatch resume hook (CORRECTED by thread-switch review)

> `CORRECTED/SUPERSEDED`: this design originally treated eligibility-change
> triggers as "mask API/MTC0 only". The thread-switch review
> ([[p08-preemption-patch35-thread-switch-review]]) proves a held cause can
> become eligible **solely** because the current guest OSThread changed
> (`status_reg` is per-thread; libultra restores SR.IM and MI_INTR_MASK
> per-thread at `__osDispatchThreadSave`). Add one trigger class.

There are now **two** eligibility-change trigger classes:

```text
TRIGGER A - interrupt state change within the same guest:
  osSetIntMask / __osRestoreInt / MTC0 Status  -> after_interrupt_state_change

TRIGGER B - current guest thread changes (new per-thread SR becomes current):
  native-thread resume -> service_pending_interrupts_for_current_guest
```

One primitive, two call sites (do not duplicate the held-retry algorithm):

```text
NEW_PRIMITIVE:
  service_pending_interrupts_for_current_guest(RDRAM_ARG1)
    if !guest_preemption_safepoints_enabled(): return;
    if tls_status_reg == nullptr: return;   // initial thread start
    service_preemption(rdram, tls_status_reg);   // retries held + drain + check_running_queue

NEW_CALL_SITE (runtime, REQUIRED):
  ultramodern/src/threads.cpp :: wait_for_resumed
    after `thread_context->running.wait();` returns AND after the
    context-replacement check (threads.cpp:152-155)
```

`wait_for_resumed` is the single funnel for every cooperative resume
(`run_next_thread_and_wait`, `resume_thread_and_wait`, initial `_thread_func`
start), so one hook covers `osRecvMesg`/`osSendMesg`/`osJamMesg` blocking,
`osYieldThread`, `pause_self`, `osStartThread`, `osSetThreadPri`, thread exit,
and preemption swaps.

```text
RESUME_HOOK_CAN_RESCHEDULE: YES (nested LightweightSemaphore re-block; no lock
                            held; token owned; skip when tls_status_reg null)
STATUS_POINTER_AVAILABLE_AT_HOOK: YES (tls_status_reg / tls_rdram; no
                            recomp_context* crosses the scheduler API)
```

This also closes a residual on the idle path: `wait_for_external_message`
blocks on `external_messages.wait_dequeue` and does not retry `held_ineligible`
after wake; the resume hook re-evaluates held under the current SR on resume.

```text
CORRECTION_SCOPE: one trigger + one primitive + one call site + test X1-G.
                  A+ hold/drain/cause-model otherwise unchanged.
```

---

## 5. Event cause map

All current `enqueue_external_message` callers:

| producer | file | cause | mask domain | OS_EVENT analog | can wake higher pri |
| --- | --- | --- | --- | --- | --- |
| VI retrace | `events.cpp` `vi_thread_func` | **VI** | CP0 IM2 + MI.VI | `OS_EVENT_VI` | YES (scheduler) |
| AI tick (VI thread) | `events.cpp` | **AI** | CP0 IM2 + MI.AI | `OS_EVENT_AI` | YES (AUDIO) |
| SP complete | `events.cpp` `sp_complete` | **SP** | CP0 IM2 + MI.SP | `OS_EVENT_SP` | YES |
| DP complete | `events.cpp` `dp_complete` | **DP** | CP0 IM2 + MI.DP | `OS_EVENT_DP` | YES |
| SI | `events.cpp` `send_si_message` | **SI** | CP0 IM2 + MI.SI | `OS_EVENT_SI` | YES |
| PI DMA / SRAM | `librecomp/src/pi.cpp` `do_dma` | **PI** | CP0 IM2 + MI.PI | user mq, PI-class | CONDITIONAL |
| Flash R/W complete | `librecomp/src/flash.cpp` | **PI** | same (PI device) | user mq | CONDITIONAL |
| `osSetTimer` expiry | `timer.cpp` `timer_thread` | **Timer** | CP0 IM7 | user mq; analog `OS_EVENT_COUNTER` then timer mgr | CONDITIONAL |
| display-list extension | `extensions.cpp` | **None** | none | host software | CONDITIONAL |
| `osSendMesg` / `osJamMesg` from non-game thread | `mesgqueue.cpp` | **None** | none | software IPC | CONDITIONAL |

```text
EVENT_CAUSE_MAP: table above
NON_INTERRUPT_MESSAGE_MODEL:
  InterruptCause::None
  always eligible
  never enters held_ineligible
  existing enqueue/dequeue/requeue unchanged for these
```

### 5.1 Timer honesty

```text
TIMER_MAPPING: UNKNOWN as CP0 Compare hardware
TIMER_RUNTIME: host thread sends the user timer message directly
PATCH35_LABEL: InterruptCause::Timer gated on IM7
```

This runtime does not implement `C0_COMPARE` / `OS_EVENT_COUNTER`.
Labelling the user message as RCP would be false precision. IM7 is the
minimum analog: `OS_IM_NONE` clears IM7, so timers must not become
guest-visible during that CS.

### 5.2 PI / flash honesty

`do_dma` and flash skip the libultra PI manager and send the **caller's**
completion message. That message is not `OS_EVENT_PI`. It is still a
PI-class completion: on hardware it would not be sent until the PI
interrupt was taken. Gate on PI.

---

## 6. Cooperative path

When the flag is ON, `osSendMesg` / `osJamMesg` / `osRecvMesg` /
`wait_for_external_message*` must use the **same** drain/hold rules.
Otherwise a masked AI event would still become guest-visible on the
next `osRecvMesg`.

```text
COOPERATIVE_DRAIN_WHEN_FLAG_ON: same eligibility as service
COOPERATIVE_SWAP: still ungated check_running_queue after the guest's
                  own send/recv (libultra can yield inside disabled IE)
```

`wait_for_external_message` is the idle/self-pause path
(`scheduling.cpp` `pause_self` / `yield_self`). If it pops an
ineligible hardware message it must hold it and wait again. It must
not treat a held AI as "work that unblocks idle."

`pause_self` remains the self-loop runtime point. PATCH 3.5 does not
add a safepoint there.

---

## 7. Status init / TLS epoch

```text
STATUS_INIT: KEEP
CURRENT_INIT_GOOD_ENOUGH_FOR_PATCH4: YES
```

PATCH 2 writes `ctx.status_reg |= IE|IMASK` in `run_thread_function`.
That is the **running** libultra value after `eret`, not the stopped
`EXL=1` snapshot. IM is fixed all-ones, matching `osCreateThread`.
MI defaults all-open, matching the previous no-op world and proven
C2AB8 `IE=1`.

Residual (SAFE_TO_DEFER): `init()` boot context is not initialized.
No Tooie `src/` `__osSetSR` call sites were found by the PATCH 2
review. Do not change it in 3.5.

```text
TLS_EPOCH: KEEP
TLS_EPOCH_KEEP: YES
```

Per-native-thread `tls_last_seen` can make a newly scheduled OSThread
re-service an already-drained epoch. Under the single-token model that
is an empty drain plus `check_running_queue` peek. No reordering, no
lost wakeup, no ping-pong. A global delivered-epoch is an optimization,
not a correctness fix. Do not change it without a failing test.

> `CORRECTED` by [[p08-preemption-patch35-thread-switch-review]]: TLS-epoch
> DOES guarantee held-retry on thread change, but only under two invariants
> that were previously implied, not stated:
>
> - **I1**: every entry into `held_ineligible` is preceded (happens-before) by
>   a publish (arrival or full-queue requeue) whose epoch the newly-current
>   thread has not yet seen;
> - **I2**: every drain path (service AND cooperative AND idle/wait) retries
>   the GLOBAL `held_ineligible` under the CURRENT SR.
>
> The thread-dispatch resume hook (4.8) makes these robust and explicit. The
> review's literal counterexample ("B may have `tls_last_seen == current
> epoch`") is refuted under the single-token + arrival-publish model.

---

## 8. Generic JR and function cycles

Current Tooie `RecompiledFuncs` (pre-PATCH3 regen, still valid as a
site census):

```text
LOOKUP_FUNC(ctx->rN)  = 265
  r25 ($t9, jalr ABI) = 132
  r2  ($v0)           =  85
  r31 ($ra)           =   0    # jr ra is emit_return
```

These are function-pointer / jalr-style dispatches, compiled as
`LOOKUP_FUNC` + `return`. They are not intra-function backedges.

```text
GENERIC_JR: DEFER
GENERIC_JR_MUST_FIX_BEFORE_PATCH4: NO
FUNCTION_CYCLE: DEFER
FUNCTION_CYCLE_PRE_PATCH4_BLOCKER: NO
```

Reasons:

- Approved minimum architecture is backedge + pre-switch.
- C2AB8 is a backward `bnel` (already covered).
- A true `jr`-only infinite cycle with no backedge and no switch is
  not identified in Tooie.
- A JAL A→B→A cycle without intra-function backedges would grow the
  host C stack and is not a demonstrated BT form.

Honest permanent claim after PATCH 4:

> Generic preemption of **direct backedges and jump tables**. Generic
> non-RA non-jump-table `jr`/`jalr` is a documented residual gap.

If a later session finds a concrete BT cycle that never hits a
safepoint, the lowest-cost fix is a safepoint in
`print_func_call_by_register` **after** the delay slot and **before**
`emit_function_call_by_register`, excluding `$ra`. Do not add
function-entry safepoints in 3.5.

---

## 9. API / ABI / performance

```text
LIVE_ABI: NONBLOCKING
LINUX_X64:   PROVEN (T11 + PATCH 3 review)
WINDOWS_X64: LIKELY (same SLJIT_CALL family as pause_self; not executed)
ARM64:       LIKELY (same)
LIVE_ABI_MUST_FIX_BEFORE_PATCH4: NO
```

```text
GENERATOR_API_COMPAT: keep Generator::emit_preemption_safepoint() = 0
EXTERNAL_API_CHANGE_ACCEPTABLE: YES
PATCH3_5_CHANGE_REQUIRED: NO
```

A default no-op would silently drop safepoints in a custom generator.
This project requires the capability. Out-of-tree break is acceptable.

```text
NULL_CALLBACK_POLICY: keep silent omit (backward compatible)
STATIC_LIVE_DIVERGENCE_POSSIBLE: YES if a caller leaves ptrs null
IN_TREE: mods.cpp and T11 tests supply both (PROVEN)
PATCH35_CODE_CHANGE: NO — assert/test only
```

```text
PERFORMANCE_DESIGN_CHANGE_BEFORE_PATCH4: NO
PERFORMANCE_CHANGE_REQUIRED: NO
```

Do not add JR/entry safepoints. Do not invent a cheaper hint. T15
measures the existing `~4300` static sites after PATCH 4 regen.
Held-ineligible does not keep `hint` true, so a long AI-masked CS
does not turn every backedge into a service call.

Rollback remains:

```text
ULTRAMODERN_GUEST_PREEMPTION=0
```

That restores: hint false, service no-op, drain-all (no hold), no
MTC0 hook. Generated code is unchanged. PATCH 4 can default ON for
Tooie and still bisect with the env var.

---

## 10. Test matrix

Harness tests enable the flag locally. Production stays OFF.

### 10.1 X1 / MTC0 (runtime harness)

| id | setup | expect |
| --- | --- | --- |
| X1-A | AI event, AI MI-masked, VI unmasked, AUDIO blocked on AI mq | AUDIO does **not** run; AI message **not** in guest mq |
| X1-B | same held AI, then `osSetIntMask` unmasks AI | AI delivered; AUDIO eligible; may preempt MAIN |
| X1-C | VI unmasked, VI event, higher-pri waiter on VI mq | preemption works (T02 analog) |
| X1-D | `OS_IM_NONE`, any hardware event | no delivery, no preemption |
| X1-E | IE=0, AI unmasked, AI event; then `__osRestoreInt(IE)` | no delivery while IE=0; deliver+maybe-swap after restore |
| X1-F | EXL or ERL set, else eligible AI; then clear EXL/ERL via MTC0 | held until clear; then deliver |
| M1 | MTC0 sets IE 0→1 with held eligible AI | service after commit; AUDIO may run |
| M2 | MTC0 unmasks IM2 with held AI and MI.AI already open | same |
| T04+ | `apply_os_set_int_mask` return equals previous combined mask for `OS_IM_NONE`, `OS_IM_ALL`, AI-only, VI-only; nested disable/restore; IE untouched | exact bits, not `saved_mask & OS_IM_NONE` |
| T07+ | one producer FIFO 1,2,3 still holds; two producers: **no** total-order assert; after drain, highest runnable wins | do not claim AUDIO-before-VI across native threads |
| REQUEUE_CAUSE | AI held, guest mq full with `requeue_if_blocked=true` after unmask, then space+service | cause stays AI through hold and full-requeue; never becomes None |
| NONINTERRUPT | `None` message while AI masked | delivered; may wake a waiter; not dropped as if it were AI |
| TLS | MAIN services epoch N, swap to AUDIO with `last_seen=0` | AUDIO may re-drain; empty; no reorder; no livelock |
| X1-G | A SR masks AI; AI arrives and is held; A blocks cooperatively; B (SR allows AI) is dispatched | held AI leaves `held_ineligible`, guest AI message becomes visible, waiter wakes, highest pri runs, no new external epoch required. Added by [[p08-preemption-patch35-thread-switch-review]] |

Existing T02 must be updated to enqueue with `InterruptCause::AI` (or
the test stays `None` and would still preempt — document which). Prefer
annotating T02 as AI so it remains an interrupt-preemption test.

### 10.2 Generator / T11 (N64Recomp tests; no production regen)

| id | expect |
| --- | --- |
| P3-X1 LIKELY_TAKEN | delay executes; safepoint after it; backedge |
| P3-X2 LIKELY_NOT_TAKEN | delay annulled; no taken-path safepoint |
| P3-X4 SWITCH | compute target; hostile service (mutate scratches / optional swap); correct case |
| P3-X6 CONTEXT | real `swap_to_thread` if the T11 process can host two OSThreads; else harness-level swap of a generated function. Survive: GPR, FPU, hi, lo, result/c1cs, status_reg, switch addend |
| JR (document) | generic `jr t0` still emits `LOOKUP_FUNC` and **no** safepoint — characterization, not a fail |

P3-X6 is the proof that closes `CONTEXT_SAFE` from CONDITIONAL to YES
or finds a real bug. Prefer the existing ultramodern token swap, not a
no-op callback.

If the N64Recomp T11 binary cannot host a real OSThread swap, implement
P3-X6 in `preemption_harness.cpp` by calling a tiny static C function
that the test compiles in-process (P3-X7 style) and parking that
thread via `swap_to_thread`. Record which vehicle was used.

```text
CONTEXT_SAFE_PROOF_PLAN: P3-X6 as above
SWITCH_SURVIVAL_TEST:    P3-X4 as above
LIKELY_TESTS:            P3-X1/X2 promoted into T11
T04_STRENGTHENING:       exact prev mask bits + nested disable/restore
T07_STRENGTHENING:       per-producer FIFO only; highest pri after drain
```

---

## 11. File/function plan

No source in this session. Later implementation only.

### 11.1 Runtime — MUST

| repo | file | function / struct | current | new | test |
| --- | --- | --- | --- | --- | --- |
| N64ModernRuntime | `ultramodern.hpp` | `InterruptCause`, enqueue decl | 4-arg enqueue | cause default `None`; declare `after_interrupt_state_change`, `cause_eligible` | X1-* |
| same | `mesgqueue.cpp` | `QueuedMessage` | no cause | store cause | REQUEUE_CAUSE |
| same | `enqueue_external_message` | publish + enqueue | pass cause through | T02/X1 |
| same | `dequeue_external_messages` | drain-all | eligibility + hold list when flag ON | X1-A/D |
| same | `wait_for_external_message*` | deliver first pop | skip/hold ineligible; wait again | X1-A + pause_self |
| same | `preemption.cpp` | `cpu_preemptible` | any-class | IE && !EXL && !ERL (see §4.3) | T03, X1-D |
| same | `preemption.cpp` | `after_interrupt_state_change` | n/a | retry hold via service | X1-B/E, M1 |
| same | `preemption.cpp` | `service_pending_interrupts_for_current_guest` | n/a | shared primitive for trigger A and B (4.8) | X1-G |
| same | `threads.cpp` | `wait_for_resumed` | resume only | add resume hook (4.8) after running.wait() + context check | X1-G |
| same | `preemption.cpp` | TLS rdram / status | epoch only | set/get for MTC0 | M1 |
| same | `intmask.cpp` | `apply_os_set_int_mask`, `restore_int` | became-preemptible check | call `after_interrupt_state_change` | X1-B, T04+ |
| same | `events.cpp` | VI/AI/SP/DP/SI enqueues | no cause | annotate | X1-A/C |
| same | `timer.cpp` | timer enqueue | no cause | `Timer` | X1-D analog |
| same | `librecomp/src/pi.cpp` | `do_dma` | no cause | `PI` | X1-D analog |
| same | `librecomp/src/flash.cpp` | flash complete | no cause | `PI` | X1-D analog |
| same | `librecomp/src/recomp.cpp` | `cop0_status_write` | commit only | after commit, hook | M1, M2, X1-F |
| same | `librecomp/src/recomp.cpp` | `run_thread_function` | status init | also set TLS rdram/status | T02 |
| same | `test/preemption_harness.cpp` | T01–T12 | no cause | X1/M/T04+/T07+ | all |

### 11.2 Runtime — do not touch

`threadqueue.cpp`, `threads.cpp` (protected dirty). `scheduling.cpp`
`check_running_queue` / `swap_to_thread` stay pure.

`extensions.cpp` keeps default `None`.

> `CORRECTED` by [[p08-preemption-patch35-thread-switch-review]]: the "do not
> touch `threads.cpp`" rule is amended to "do not touch the dirty NULLPTR guard
> (`osSetThreadPri`, `threads.cpp:305-320`); a minimal resume hook in
> `wait_for_resumed` (4.8) IS required". Do not fold the existing dirty
> NULLPTR guard into the patch; add the resume hook as a separate hunk on top.

### 11.3 N64Recomp — tests only

| file | change | test |
| --- | --- | --- |
| `LiveRecomp/preemption_safepoint_test.cpp` | add P3-X1/X2/X4; JR characterization | T11 |
| `LiveRecomp/live_recompiler_test.cpp` | only if inputs need a no-op service that still compiles | T11 |

No `recompilation.cpp` coverage change in 3.5.

### 11.4 TooieRecomp

Docs + later cumulative patch hashes only. No `RecompiledFuncs`. No
`src/**`.

---

## 12. Sequence

Every slice builds. Flag OFF for games the whole time.

```text
PATCH 3.5A — cause metadata + hold + producer annotations + X1-A..D
             + NONINTERRUPT + REQUEUE_CAUSE
             Update cumulative n64modernruntime-preemption.patch

PATCH 3.5B — after_interrupt_state_change + thread-dispatch resume hook
             wire osSetIntMask / Restore / MTC0 (trigger A)
             wait_for_resumed resume hook (trigger B, 4.8)
             X1-E/F/G, M1, M2, T04+
             Same cumulative runtime patch

PATCH 3.5C — T07+, TLS test, T11 P3-X1/X2/X4/X6
             Runtime harness and/or n64recomp-tooie.patch (tests only)

PATCH 3.5D — clean reconstruction twice, both hashes, T01–T12 + new
             cases, PATCH4_READY checklist. Still no Tooie regen.
```

If 3.5A is too large for one commit, split annotations vs hold only
if both commits compile and 3.5A's tests skip until annotations exist.
Prefer one 3.5A.

---

## 13. Artifact policy

Unchanged:

```text
one cumulative n64modernruntime-preemption.patch   vs ca568b6
  = PATCH 1 + 2 + 3 bridge + 3.5
one cumulative n64recomp-tooie.patch               vs 2b6f056
  = JAL + PATCH 3 + 3.5 tests (if any)
n64modernruntime-scheduler.patch                   separate
n64recomp-same-section-jal.patch                   inactive
```

Apply script: replace the two SHA256 pins after regeneration.
Run apply twice. Do not fold dirty `threadqueue.cpp` / `threads.cpp`.

---

## 14. PATCH4_READY gate

`PATCH4_READY` becomes YES only when **all** of the following are
recorded as actually run:

```text
CAUSE_CLASS_MODEL:              PASS
MASKED_CAUSE_TEST (X1-A..F):    PASS
MTC0_REENABLE (M1, M2):         PASS
T01–T12:                        PASS (T11 includes likely + switch)
CONTEXT_REAL_SWAP (P3-X6):      PASS
SWITCH_SURVIVAL (P3-X4):        PASS
T04 exact return/mask:          PASS
T07 per-producer FIFO:          PASS
REQUEUE_CAUSE:                  PASS
NONINTERRUPT:                   PASS
CLEAN_RECONSTRUCTION:           PASS
PATCH_SCRIPT_IDEMPOTENT:        YES
NO GENERATED TOOIE YET:         YES
PRODUCTION_FLAG:                still OFF
PERFORMANCE:                    explicitly deferred to PATCH4/T15
                                with ULTRAMODERN_GUEST_PREEMPTION=0 rollback
GENERIC_JR:                     documented residual, not a fail
```

Until then:

```text
PATCH4_READY: NO
```

PATCH 4, when allowed, is: rebuild N64Recomp (keep JAL policy),
`run_recomp.sh`, enable flag default ON for Tooie, T13–T15.
Expected leftover: `func_800BCF28`.

---

## 15. Implementation notes (for the next engineer)

1. Read this file, then `p08-preemption-implementation-plan.md`, then
   the PATCH 2/3 independent reviews. Do not re-derive X1.
2. Start from the versioned patches, not from inventing a second
   preemption stack.
3. Do not reset the real dirty runtime checkout.
4. Reconstruct in `/tmp` for the first compile.
5. Keep `enqueue_external_message` 4-arg source-compatible.
6. Never `publish()` when moving a message into `held_ineligible`.
7. Commit Status/IM/MI **before** `after_interrupt_state_change`.
8. Do not implement the exceptasm shared-IP2 quirk.
9. Do not add JR or function-entry safepoints.
10. Do not enable the flag. Do not regen Tooie. Do not touch
    `func_800BCF28`.

```text
NEXT_SINGLE_ACTION:
  implement PATCH 3.5 according to this approved semantic-closure design
```
