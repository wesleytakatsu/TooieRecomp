---
title: "P08 — preemption Hybrid F' implementation plan"
description: "Patch spec file/function-level e matriz de regressão T01–T15 para o Hybrid F' aprovado. PATCH 3.5 requires per-OSThread RCP correction before PATCH 4."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, implementation-plan, n64modernruntime, n64recomp, harness]
---

# P08 — PREEMPTION IMPLEMENTATION PLAN

## Meta

```text
MODE:            READ / DESIGN / DOCS ONLY
MODEL:           Grok 4.6 HIGH
BASE_HEAD:       e729e7efd6caee620670cd490b789ffff0433cf1
ARCHITECTURE:    Hybrid F' (p08-preemption-design-adversarial-review)
                 NOT Hybrid F DeepSeek (SUPERSEDED)
TOOIE:           clean at BASE_HEAD
RUNTIME_HEAD:    ca568b6ad79b9029d14077f0c3ffa757727c5559
RUNTIME_DIRTY:   threadqueue Fix B + threads NULLPTR + N64Recomp JAL
                 DO NOT reset. DO NOT fold into these patches.
IMPLEMENTATION_READY: NO (PATCH 3.5 per-OSThread RCP correction required)
ARCHITECTURE_STATUS:  APPROVED
```

Não reabrir arquitetura salvo contradição concreta. Contradição encontrada:
**não existe harness de threadqueue/scheduler** no runtime (DeepSeek assumiu
um). T12 é teste **novo** do contrato Fix B, não um binário pré-existente.

Constraint de implementação (não muda F'):

```text
recomp_context.status_reg is not maintained today.
If run_thread_function zero-inits ctx, status_reg==0 → cpu_preemptible
is forever false and safepoints never swap.
REQUIRED: init IE|IM (and global MI) to the current no-op world:
  "interrupts enabled / all classes unmasked" until the guest
  explicitly masks. Matches proven C2AB8 IE=1.
```

`NEW_DOWNSTREAM_BLOCKER = func_800BCF28`. Não investigar.

---

## 0. Settled architecture (do not reopen)

From [[p08-preemption-design-adversarial-review]]:

```text
EVENT_NOTIFICATION:  enqueue then work_epoch.fetch_add(1, release);
                     requeue also publishes
SAFEPOINT:           after delay slot, if target_vram <= instr_vram;
                     also immediately before emit_switch
FAST_PATH:           TLS last_seen vs work_epoch relaxed load
INTERRUPT:           per-thread IE+IM in ctx->status_reg;
                     global MI + __OSGlobalIntMask
                     cpu_preemptible = IE && !EXL && !ERL
                                   && (IM or MI unmasked)
                     osSetIntMask = full return+mask, not a bool
                     Disable/Restore = IE save/restore, not a counter
SERVICE:             drain all, then one check; deliver even if
                     !preemptible; swap only if predicate
PREDICATE:           game thread && RUNNING && cpu_preemptible
                     && !empty(runq) && peek.pri > self.pri
SWAP:                existing swap_to_thread; assert is_game_thread
FLAG:                runtime no-op; generator always emits the call
```

---

## 1. New public API

C ABI (generated code cannot call C++). Mirror `pause_self`.

```c
/* N64Recomp/include/recomp.h  +  ultramodern implementation */
bool ultramodern_preemption_hint(void);
void ultramodern_service_preemption(uint8_t* rdram, recomp_context* ctx);
```

C++ (host / harness / translations):

```cpp
// ultramodern/include/ultramodern/ultramodern.hpp
namespace ultramodern {
  void set_guest_preemption_safepoints(bool enabled);
  bool guest_preemption_safepoints_enabled();
  void publish_external_work();          // epoch.fetch_add(1, release)
  bool cpu_preemptible(const recomp_context* ctx);
  void dequeue_external_messages(RDRAM_ARG1); // promote existing TU-local
}

// Env override, read once at preinit:
//   ULTRAMODERN_GUEST_PREEMPTION=0  → force off (bisect)
// Default after PATCH 4: ON. Patches 1–3: OFF for games
// (hint returns false; generated calls are nops).
```

```text
FAST PATH (generated, PATCH 3+):
  if (ultramodern_preemption_hint())
      ultramodern_service_preemption(rdram, ctx);

hint:
  if (!enabled) return false;
  return work_epoch.load(relaxed) != tls_last_seen;

service:  // game thread only
  assert(is_game_thread());
  for (;;) {
    uint64_t seen = work_epoch.load(acquire);
    dequeue_external_messages(rdram);
    if (cpu_preemptible(ctx) && higher_pri_runnable())
      check_running_queue(rdram);   // existing; may park
    tls_last_seen = seen;
    if (work_epoch.load(relaxed) == seen) break;
  }
```

MEMORY ORDER: publish `release`; hint `relaxed`; service first load `acquire`.
THREAD-SAFETY: publish from any host producer; hint/service only on the
token-holding game thread. Epoch is the only cross-thread shared word.

---

## 2. File/function patch list

### 2.1 N64ModernRuntime — new

| FILE | FUNCTION | CURRENT | NEW RESPONSIBILITY | TEST |
| --- | --- | --- | --- | --- |
| `ultramodern/src/preemption.cpp` | `work_epoch`, TLS `last_seen`, `enabled` | n/a | epoch + flag | T02, T08 |
| same | `publish_external_work` | n/a | `fetch_add(1, release)` | T08, T10 |
| same | `ultramodern_preemption_hint` | n/a | relaxed compare; false if disabled | T01, T15 |
| same | `ultramodern_service_preemption` | n/a | protocol §1; `assert(is_game_thread)` | T02–T10 |
| same | `cpu_preemptible` | n/a | IE && !EXL && !ERL && (IM\|\|MI) | T03, T04 |
| same | `set/get_guest_preemption_safepoints` | n/a | process flag; env at first get | T01 |
| `ultramodern/src/intmask.cpp` | `g_mi_mask`, `g_global_int_mask` | n/a | software MI + `__OSGlobalIntMask`, default all-ones | T03, T04 |
| same | `apply_os_set_int_mask(ctx, a0) -> prev` | n/a | libultra contract; write IM in ctx; write MI; **not IE** | T04 |
| same | `disable_int(ctx) -> old_IE` | n/a | clear IE; return old IE | T03 |
| same | `restore_int(ctx, a0)` | n/a | `IE \|= a0`; if became preemptible, `check_running_queue` | T04 |
| `ultramodern/test/preemption_harness.cpp` | `main` / cases T01–T12 | **does not exist** | standalone harness; see §4 | T01–T12 |
| `ultramodern/CMakeLists.txt` | library + optional test | no tests | add `preemption.cpp` `intmask.cpp`; `ULTRAMODERN_BUILD_TESTS` → `preemption_harness` EXCLUDE_FROM_ALL | T01 |

Do **not** add `preemption.cpp` to Tooie sources.

### 2.2 N64ModernRuntime — existing

| FILE | FUNCTION | CURRENT CONTRACT | EXACT NEW RESPONSIBILITY | CALL SITES | TEST |
| --- | --- | --- | --- | --- | --- |
| `ultramodern/include/ultramodern/ultramodern.hpp` | enqueue / check decls | no preemption API | declare publish, service helpers, `dequeue_external_messages` | all T* | T01 |
| `ultramodern/src/mesgqueue.cpp` `enqueue_external_message` | enqueue only | after enqueue: `publish_external_work()` | events.cpp VI/AI, sp/dp_complete, send_si, timer.cpp, pi.cpp, flash.cpp, osSend/Jam from non-game | T02, T07, T10 |
| same `dequeue_external_messages` | drain + requeue without notify | if any requeued: `publish_external_work()` | service, osSend/Recv/Jam | T08 |
| same `osSendMesg` / `osJamMesg` / `osRecvMesg` | dequeue + do_* + ungated `check_running_queue` | **unchanged** (keep coop path ungated) | guest API | T01 |
| `ultramodern/src/scheduling.cpp` `check_running_queue` | `peek.pri > self.pri` → swap | **unchanged** (pure). Gate lives in service + restore | service, osSend/Recv, osSetThreadPri, osStartThread | T01, T05, T06 |
| same `swap_to_thread` | insert self + resume_and_wait | **unchanged**; document Fix B interaction | check_running_queue | T09 |
| `ultramodern/src/events.cpp` / `timer.cpp` / `librecomp/src/pi.cpp` / `flash.cpp` | call enqueue | **no change** (chokepoint is enqueue) | — | T07 |
| `ultramodern/src/threads.cpp` `_thread_func` / `osSetThreadPri` | dirty NULLPTR guard present | **do not edit** in these patches | — | T12 |
| `ultramodern/src/threadqueue.cpp` insert/remove | dirty Fix B | **do not edit**; T12 locks the contract | schedule / swap | T12 |
| `librecomp/src/ultra_translation.cpp` `osSetIntMask_recomp` | empty; `r2` unset | `ctx->r2 = apply_os_set_int_mask(ctx, ctx->r4)` | generated jal to osSetIntMask | T04 |
| same `__osDisableInt_recomp` | empty | `ctx->r2 = disable_int(ctx)` | generated | T03 |
| same `__osRestoreInt_recomp` | empty | `restore_int(ctx, ctx->r4)` | generated | T04 |
| same `osGetIntMask_recomp` | **no symbol today** | implement **only if** a stub exists; do not invent | — | — |
| `librecomp/src/recomp.cpp` `cop0_status_write` | FR only; any other bit `exit` | allow IE (0x1), EXL (0x2), ERL (0x4), IM (0xFF00); keep FR; still assert leftover bits | MTC0 Status | T03 |
| same `run_thread_function` (find def; decl `threads.cpp:55`) | ctx likely `{}` | `ctx.status_reg \|= IE\|IM_ALL` (and do not clear FR handling). Init global MI all-ones once | every guest thread start | T02, T13 |
| `librecomp/src/recomp.cpp` or `mods.cpp` LiveGeneratorInputs | pause_self wired | add `preemption_hint` + `service_preemption` fn ptrs | live mods/overlays | T11 |
| `N64Recomp/include/recomp.h` | `pause_self` only | declare hint + service next to `pause_self` | generated C | T11, T13 |

`osSetIntMask` signature for translations: a0=`ctx->r4`, v0=`ctx->r2`.
Disable: v0=old IE. Restore: a0=`ctx->r4` (0 or `SR_IE`).

Init constants (software, not hardware):

```text
SR_IE          = 0x00000001
SR_EXL         = 0x00000002
SR_ERL         = 0x00000004
SR_IMASK       = 0x0000FF00
OS_IM_NONE     = 0x00000001
OS_IM_ALL      = 0x003FFF01
RCP_IMASK      = 0x003F0000
default sr     = OS_IM_ALL   (IE + all CPU IM; FR separately)
default mi     = 0x3F
default global = ~0u
```

### 2.3 N64Recomp — generator

| FILE | FUNCTION | CURRENT | NEW | TEST |
| --- | --- | --- | --- | --- |
| `include/recompiler/generator.h` `Generator` | no safepoint | `virtual void emit_preemption_safepoint() const = 0;` after `emit_pause_self` | T11 |
| same `CGenerator` | — | `void emit_preemption_safepoint() const final;` | T11 |
| `src/cgenerator.cpp` | — | emit the two-line C in §1 | T11, T13 |
| `include/recompiler/live_recompiler.h` `LiveGenerator` | — | same virtual | T11 |
| same `LiveGeneratorInputs` | pause_self ptr | `bool (*preemption_hint)(void);` `void (*service_preemption)(uint8_t*, recomp_context*);` | T11 |
| `LiveRecomp/live_generator.cpp` `emit_preemption_safepoint` | n/a | sljit: call hint; branch; call service(rdram, ctx) — same registers as `emit_pause_self` | T11 |
| `src/recompilation.cpp` `print_goto_with_delay_slot` / `print_branch` | delay slot then `emit_goto` | **after** `process_delay_slot`, if `target_vram <= instr_vram`: `emit_preemption_safepoint()` then goto | T02, T11, T13 |
| same JR jump-table arm | `process_delay_slot`; `emit_switch` | **after** delay slot, **before** `emit_switch`: always `emit_preemption_safepoint()` | T11 |
| same `cpu_j`/`cpu_b` self | `emit_pause_self` | **no** extra safepoint | T01 |
| `src/main.cpp` `resolve_jal` dirty | Tooie JAL policy | **do not touch** | — |
| `LiveRecomp/live_recompiler_test.cpp` | constructs inputs | fill new ptrs with no-op/hint-false **or** real symbols | T11 |

```text
SAFEPOINT_PLACEMENT (settled):
  AFTER delay slot, BEFORE goto, when target_vram <= current instr_vram.
  BEFORE emit_switch (covers static emit_case gotos + live ijump).
  NEVER between branch and delay slot.
  NEVER only inside CGenerator::emit_goto (live/jtbl would diverge).
```

`CONTEXT_SAFE`: no extra flush. Swap still parks the native thread.
Service is a real C call (ABI spill). Pass `ctx`.

STATIC_LIVE_PARITY: one request site in `recompilation.cpp`. Both
backends implement the same virtual. T11 compiles one tiny function
both ways and compares swap/service counts.

### 2.4 TooieRecomp

| FILE | CHANGE | WHEN |
| --- | --- | --- |
| `RecompiledFuncs/**` | regenerate via `tools/run_recomp.sh` only | PATCH 4 |
| `src/**` | none (no Tooie yield) | — |
| generated C manual edit | **FORBIDDEN** | — |

After PATCH 3, rebuild `N64Recomp/build/N64Recomp` **from the dirty
submodule worktree** (JAL policy must stay). Then `run_recomp.sh`.

---

## 3. Patch sequence

Every commit must **build**. Games stay feature-disabled until PATCH 4.

```text
PATCH 1 — infrastructure / tests   [default flag OFF]  LANDED + REVIEWED (B)
  See [[p08-preemption-patch1-validation]] and
  [[p08-preemption-patch1-independent-review]].
  New: preemption.cpp stubs (hint=false, service no-op),
       intmask.cpp defaults (no call sites), header decls,
       CMake sources, preemption_harness + T01.
  SLICING CORRECTION: recomp.h decls NOT in PATCH 1
    (RECOMP_H_REQUIRED_IN_PATCH1=NO; defer to PATCH 3).
  Env ULTRAMODERN_GUEST_PREEMPTION: deferred to PATCH 4.
  No enqueue publish. No generator. No translation bodies.
  T01: PASS. T02–T12: SKIPPED.
  Isolated: yes. External runtime left uncommitted.

PATCH 2 — runtime service          [flag still OFF for games]  LANDED
  See [[p08-preemption-patch2-validation]].
  Real publish on enqueue+requeue. Real service protocol.
  Real osSetIntMask / Disable / Restore / cop0 IE/IM.
  status_reg + MI init.
  Harness sets flag ON and calls service directly (no generated code).
  T02–T10, T12.
  Isolated: games do not call service; coop path ungated.

PATCH 3 — generator safepoints     [flag still OFF for games]  LANDED
  See [[p08-preemption-patch3-validation]].
  Independent review B: [[p08-preemption-patch3-independent-review]].
  emit_preemption_safepoint + recompilation.cpp hooks + live inputs.
  C ABI void* is status_reg; generated arg is (void*)&ctx->status_reg.
  Isolate hunks from dirty resolve_jal. T11 PASS. No Tooie regen.

PATCH 3.5 — semantic closure       [flag still OFF]            LANDED
  See [[p08-preemption-patch35-design]].
  MUST_IMPLEMENT: X1 annotated-hold cause model + MTC0/mask-change
  pending service + thread-dispatch resume hook (trigger B).
  MUST_TEST: X1-A..F, X1-G, M1/M2, T04+, T07+, P3-X1/X2/X4/X6.
  DEFER: generic JR, function-entry safepoints, status-init change,
  TLS epoch replacement, cheaper hint. Do not enable. Do not regen.
  INDEPENDENT REVIEW C: [[p08-preemption-patch35-independent-review]].
  BLOCKER: current RCP/MI mask is process-global, but libultra restores
  THREAD_RCP per OSThread. RCP-only X1-G and cross-thread old-return fail.
  CORRECT PATCH 3.5 before PATCH 4; keep flag OFF and do not regen.

PATCH 4 — integration              [flag ON]
  Tooie: rebuild N64Recomp, run_recomp.sh, cmake build TooieRecompiled.
  Enable flag (default ON or Tooie preinit).
  T13 C2AB8, T14 intro/audio, T15 numbers.
  Expected leftover: SIGSEGV func_800BCF28.
```

Do **not** merge PATCH 3 generator into the dirty JAL commit.
Three repos, four commits (runtime 1–2, N64Recomp 3, Tooie 4 docs/scripts
only — generated C is gitignored).

Rollback: `ULTRAMODERN_GUEST_PREEMPTION=0` after PATCH 4; or revert
PATCH 4 regen + flag. PATCH 1–2 stay inert if flag off.

---

## 4. Test matrix

Harness location: `N64ModernRuntime/ultramodern/test/preemption_harness.cpp`.
Build: `-DULTRAMODERN_BUILD_TESTS=ON`. Tooie default CMake does not
enable it.

Harness setup common:

```text
Allocate RDRAM. Create MAIN (id 6, pri 20) and AUDIO (id 4, pri 50)
via osCreateThread/osStartThread. AUDIO blocks on osRecvMesg(audio_mq).
MAIN holds the token. Host helper thread calls
enqueue_external_message(audio_mq, msg, false, requeue_flag).
Flag ON unless noted.
```

### T01 baseline cooperative scheduler regression

```text
SETUP:    flag OFF. AUDIO blocked. MAIN calls osRecvMesg(dummy) or
          yield_self_1ms after host enqueues to AUDIO mq.
ACTION:   existing coop path only. No service() calls.
EXPECTED: AUDIO runs then MAIN resumes. Same as HEAD.
FAIL:     hang, inversion, or extra swap vs HEAD.
```

### T02 high-priority async preemption

```text
SETUP:    flag ON. MAIN in a leaf spin that calls service each
          backedge (harness loop; no generator yet).
ACTION:   host enqueues to AUDIO mq.
EXPECTED: MAIN services → AUDIO (50) preempts → AUDIO recvs →
          AUDIO blocks → MAIN resumes.
FAIL:     AUDIO never runs; MAIN spins forever.
```

### T03 interrupts-disabled no-preempt

```text
SETUP:    MAIN __osDisableInt. Host enqueues AUDIO.
ACTION:   service at backedge.
EXPECTED: message delivered to guest mq (policy B); NO swap;
          AUDIO still blocked; MAIN continues.
FAIL:     swap while IE=0.
```

### T04 pending event after interrupt re-enable

```text
SETUP:    continue T03.
ACTION:   __osRestoreInt(old_IE)  [old_IE == 1]
EXPECTED: restore sees preemptible + AUDIO queued → swap to AUDIO.
          Also: osSetIntMask(OS_IM_NONE) then event then
          osSetIntMask(prev): no swap under NONE (IE still 1!);
          swap on restore of prev. Return value of Set equals
          previous mask.
FAIL:     swap under OS_IM_NONE; no swap after restore; r2 garbage.
```

### T05 same-priority no-preempt

```text
SETUP:    waiter pri == MAIN (20).
ACTION:   enqueue that wakes waiter; service.
EXPECTED: waiter queued; MAIN keeps token.
FAIL:     swap to equal pri.
```

### T06 lower-priority no-preempt

```text
SETUP:    waiter pri 10.
ACTION:   same.
EXPECTED: no swap.
FAIL:     swap to lower.
```

### T07 simultaneous VI+AI

```text
SETUP:    two host threads enqueue to distinct mqs (VI-like, AI-like)
          without delay. AUDIO waits on AI mq; another pri-30 waits
          on VI mq.
ACTION:   MAIN service once.
EXPECTED: both messages delivered FIFO-per-producer; one scheduling
          decision; highest pri of the woken set runs.
FAIL:     lost message; inversion; hang.
```

### T08 lost-wakeup stress

```text
SETUP:    AUDIO mq size 1, already full. Producer uses
          requeue_if_blocked=true. MAIN spinning on service.
ACTION:   N producers enqueue while MAIN drains; no further publish
          after the last requeue.
EXPECTED: requeue publishes; MAIN retries; no stuck message.
FAIL:     message sits in external queue forever with hint false
          (the DeepSeek hole).
```

### T09 repeated thread swaps

```text
SETUP:    T02 loop 1000 times (enqueue, preempt, AUDIO block, repeat).
ACTION:   — 
EXPECTED: 1000 AUDIO wakeups; no double-insert cycle (Fix B).
FAIL:     "No threads left", self-linked queue, hang, abort.
```

### T10 message ordering

```text
SETUP:    one producer enqueues 1,2,3 to a mq with a blocked recver.
ACTION:   service.
EXPECTED: recver sees 1 then 2 then 3.
FAIL:     reorder within one producer.
NOTE:     multi-producer global order is NOT asserted (MPMC).
```

### T11 static vs LiveRecomp equivalence

```text
SETUP:    toy MIPS function: label L; addiu; bne L (backward) plus a
          2-entry jump table that can loop. Compile with CGenerator
          and LiveGenerator using the same recompilation.cpp hooks.
ACTION:   run both against the same harness (service counts).
EXPECTED: same number of hint-true entries and swaps ±0.
FAIL:     static swaps, live does not (or the reverse).
```

### T12 existing threadqueue harness

```text
THERE IS NO EXISTING BINARY. T12 is NEW.
SETUP:    unit-test thread_queue_insert/remove/peek against the
          Fix B contract in p08-threadqueue-b-fix-validation:
          duplicate insert is a no-op; remove non-head works;
          membership is link-scan not stale t->queue.
ACTION:   insert A, insert A again, remove non-head, peek order.
EXPECTED: no cycle; remove returns true for non-head; pri order.
FAIL:     self-link; remove only-head; crash on NULLPTR queue
          (NULLPTR is the threads.cpp dirty guard — do not
          "fix" it here; just do not pass NULLPTR in this test).
```

### T13 Tooie C2AB8 regression

```text
SETUP:    PATCH 4 binary. TOOIE_AUTO_START=1 TOOIE_DIAG=1
          TOOIE_DISABLE_LAUNCHER_MUSIC=1
          VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json
          ./build/TooieRecompiled
ACTION:   clean run through intro/Jiggy/type-4/mode-5.
EXPECTED: C2AB8 returns; table[2].word0 cleared; audio counters
          advance during the former freeze window; attract/world
          init proceeds until SIGSEGV func_800BCF28.
FAIL:     d-counter freeze ≥6s at C2AB8; audio counters frozen;
          table[2] stays live.
```

### T14 Tooie intro/audio regression

```text
SETUP:    same binary as T13.
ACTION:   listen + TOOIE_DIAG counters through intro to Jiggy.
EXPECTED: intro music present; rsp_aud/queue_samp increase;
          no new hang before C2AB8.
FAIL:     silent intro; boot hang; crash before Jiggy.
```

### T15 performance microbenchmark

```text
SETUP:    harness loop = C2AB8-shaped 60-iter inner + outer
          (no guest ROM). Three modes:
            A  flag OFF
            B  flag ON, no publishes (hint miss)
            C  flag ON, publish at 60 Hz
ACTION:   N=1e7 inner backedges. Record ns/iter.
EXPECTED: A ≈ B (hint is one relaxed load + branch).
          C rare service; not 1:1 with backedges.
          THRESHOLD: TO_DEFINE — project has no budget.
          Recommend: record A/B/C on this machine in the
          evidence note; fail only if B calls service
          (hint broken) or C services every iter with
          60 Hz publish (protocol broken).
FAIL:     functional (hint/service ratio), not a made-up %.
```

---

## 5. Performance plan

```text
FAST_PATH_OVERHEAD:     1 atomic relaxed load + cmp + branch
NO_EVENT_BENCHMARK:     T15-B; must not enter service
EVENT_HEAVY_BENCHMARK:  T15-C at 60 Hz + T07 burst
ACCEPTANCE_THRESHOLD:   TO_DEFINE
JUSTIFICATION:          no prior ultramodern/Tooie number exists.
MEASURE:                T15 on the same box as clean-freeze
                        (document CPU, compiler, -O).
OPTIONAL later gate:    B within 2x of A on x86_64 — only after
                        T15 produces a baseline.
```

---

## 6. Dirty-state handling

```text
threadqueue Fix B:     INTERACTS / leave in place / T09+T12
osSetThreadPri NULLPTR: INTERACTS / leave in place / do not call
                        osSetThreadPri on queue-less threads in harness
N64Recomp JAL policy:  INDEPENDENT / same file as PATCH 3 hook /
                        separate hunk; do not rewrite resolve_jal
```

Do not commit the dirty tree as part of PATCH 1–2 unless a later chat
is explicitly authorized to upstream Fix B. PATCH 3 applies on top of
the dirty `recompilation.cpp`.

---

## 7. Rollback

```text
Runtime: ULTRAMODERN_GUEST_PREEMPTION=0
         or set_guest_preemption_safepoints(false) before boot
Generator: keep emitting; hint short-circuits
Tooie:   re-run old N64Recomp binary + old RecompiledFuncs
         (gitignored; keep a tarball if needed)
Do not hand-edit generated C to strip safepoints.
```

---

## 8. Specs

`recomp-readiness.md` items 1–4 stay the N64 **requirement**, not the
delivered behavior. This plan is evidence, not a completed contract.
`build.md` / `matching.md` untouched. `rsp-and-audio.md` already points
at the design review.

---

## 8b. PATCH 2 independent review (do not erase)

See [[p08-preemption-patch2-independent-review]] (verdict B).

```text
GENERIC_ANY_UNMASKED_AS_CAUSE_EQUIVALENT: REFUTED
  Approved F' swap gate remains the all-masked predicate.
  Not hardware-equivalent when a masked-class event is delivered
  while another class is open (X1). Close before PATCH 4 enable.
  Not a PATCH 2 blocker; not an F' architecture reopen.
```

## 9. PATCH 2 constraints from independent review

See [[p08-preemption-patch1-independent-review]].

```text
RECOMMENDED_PATCH_ARTIFACT_STRATEGY: B
  Replace n64modernruntime-preemption-patch1.patch with a cumulative
  preemption patch against runtime base ca568b6. Update the SHA in
  apply_runtime_patches.sh. Do not add an incremental patch2 on top
  of patch1 without teaching the script a reverse-check stack.

PATCH3_C_ABI:
  Do not declare ultramodern_service_preemption(uint8_t*, recomp_context*)
  in recomp.h while ultramodern.hpp keeps void*. Keep void* in both
  headers, or name the recomp_context struct tag first.

T01-E:
  Reorder the harness so service() runs after host enqueue, then
  settle, then coop osSendMesg. Current T01-E is WEAK.
```

## 9b. PATCH 3.5 independent review

See [[p08-preemption-patch35-independent-review]] (verdict C).

```text
PATCH4_READY: NO
ROOT_DEFECT: process-global current RCP/MI mask; libultra is per OSThread
MINIMUM: thread_local RCP mask, initial 0x3F; keep global mask global
TESTS: RCP-X1..X4 plus a P3-X6 proof that cannot pass without the
       safepoint swap
```

## 10. Next

```text
NEXT_SINGLE_ACTION:
  Correct PATCH 3.5 RCP/per-thread state only and add RCP-X1..X4;
  strengthen P3-X6 so it cannot pass without the safepoint swap.
  Do not enable the flag, implement PATCH 4, or regenerate Tooie.
```

=== END IMPLEMENTATION PLAN ===
