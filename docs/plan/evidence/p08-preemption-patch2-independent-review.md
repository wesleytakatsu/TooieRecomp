---
title: "P08 — independent review of preemption PATCH 2"
description: "Revisão independente (Grok 4.6 HIGH) do Hybrid F' PATCH 2. Veredito B: aprovar com issues não bloqueantes. PATCH 3 ready sob constraints. Sem alteração de runtime/generator."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch2, independent-review, n64modernruntime]
---

# P08 — INDEPENDENT REVIEW OF PREEMPTION PATCH 2

```text
MODE:                 READ / TEST / DOCS ONLY
MODEL:                Grok 4.6 HIGH
ROLE:                 Independent principal runtime / concurrency reviewer
                      (did not implement PATCH 2)
TOOIE_HEAD:           21a9f1bb4eeaed255dd87bb52c70ff82f8737480
RUNTIME_BASE:         ca568b6ad79b9029d14077f0c3ffa757727c5559
N64RECOMP_BASE:       2b6f05688de2abc7d86da5b4a89b84c2c6acbabe
ARCHITECTURE:         Hybrid F' (not reopened)
VERDICT:              B — APPROVE WITH NONBLOCKING ISSUES
PATCH3_READY:         YES
SOURCE_CODE_CHANGED:  NO
```

Do not treat missing PATCH 3 safepoints as a PATCH 2 defect. DeepSeek Hybrid F
remains `CORRECTED/SUPERSEDED`. Do not enable the production flag.

---

## 0. Scope and sources

Reviewed the **versioned artifact and applied sources**, not the implementer
report. Primary libultra:

- `banjo-tooie-Recomp/lib/ultralib/src/os/{setintmask.s,interrupt.s,getintmask.s,exceptasm.s,createthread.c,initialize.c,setglobalintmask.c,resetglobalintmask.c}`
- `include/PR/{os_exception.h,R4300.h}`

Runtime inspected on the dirty checkout at `ca568b6` **and** on a disposable
reconstruction. Protected dirty files were not treated as PATCH 2:

```text
threadqueue.cpp  1c61c91985dddfc8919f41afb54aa4ce1c74d5d9e0c4476e51dd4c34a60dbeae
threads.cpp      d12dd1335a79748711e0bf0763f9c638a424fe7b9f0701c3b1a2565970340f99
N64Recomp/main.cpp        fb0ae67cfaf17902867d69b7b3b7aa499e4adabf0d7da921ce6a1a5fcd4035dd
N64Recomp/recompilation.cpp db8b443142b2513e3a5d00c2b0c453d387d70e95840b47b6de284b556e2eeede
```

Before/after this review: identical. Real dirty checkout was not reset.

---

## 1. Artifact

```text
PATCH_HASH:
  expected c2e2a908cd7be0ddafb92405b2f3f8c37b27624815c95a9131f8f9467f4c7ade
  actual   c2e2a908cd7be0ddafb92405b2f3f8c37b27624815c95a9131f8f9467f4c7ade
PATCH_HASH_VALID: YES
```

`tools/apply_runtime_patches.sh` pins the same SHA. Old
`n64modernruntime-preemption-patch1.patch` is absent.

Paths in the cumulative patch (PATCH 1 + PATCH 2 only):

| path | class |
| --- | --- |
| `ultramodern/CMakeLists.txt` | EXPECTED |
| `ultramodern/include/ultramodern/ultramodern.hpp` | EXPECTED |
| `ultramodern/src/preemption.cpp` | EXPECTED |
| `ultramodern/src/intmask.cpp` | EXPECTED |
| `ultramodern/src/mesgqueue.cpp` | EXPECTED |
| `ultramodern/test/preemption_harness.cpp` | EXPECTED |
| `librecomp/src/recomp.cpp` | EXPECTED |
| `librecomp/src/ultra_translation.cpp` | EXPECTED |

No hunk touches `threadqueue.cpp`, `threads.cpp`, N64Recomp generator files,
generated C, or Tooie `src/**`.

```text
SCOPE_VALID:          YES
DIRTY_FILE_OVERLAP:   NO
ARTIFACT_POLICY_VALID: YES
```

---

## 2. Clean reconstruction

Disposable clone `/tmp/p08-p2-independent-review/N64ModernRuntime`.
Start: runtime `ca568b6`, N64Recomp `2b6f056`. Clean trees.

Versioned artifacts only, twice:

1. `n64modernruntime-scheduler.patch`
2. `n64modernruntime-preemption.patch`
3. `n64recomp-same-section-jal.patch`

First apply: clean. Second: reverse `--check` succeeds; forward `--check`
fails (already applied). Script path would skip.

Reconstructed protected hashes match the real dirty tree (values in §0).

Reconstructed harness (`-DULTRAMODERN_BUILD_TESTS=ON`, Debug):

```text
CLEAN_RECONSTRUCTION:     PASS
SECOND_APPLY_IDEMPOTENT:  YES
HARNESS_FROM_RECONSTRUCTION: PASS
PATCH2 PASS  (pass=17 fail=0 skipped=1)  exit 0
T11 SKIPPED — PATCH 3
```

Real-tree `apply_runtime_patches.sh`: all three already applied. Protected
hashes unchanged. `ninja -C build TooieRecompiled`: no work.

---

## 3. Epoch protocol — interleavings

Actual service (`preemption.cpp`):

```text
if (!enabled) return;
for (;;) {
  seen = epoch.load(acquire);
  had_requeue = dequeue();          // drain MPMC; requeue full mqs
  if (had_requeue) publish();       // fetch_add(1, release)
  if (status && cpu_preemptible)
    check_running_queue();          // may park this native thread
  tls_last_seen = seen;
  now = epoch.load(relaxed);
  if (now == seen) break;
  if (had_requeue && now == seen + 1) break;
}
```

Publish is `fetch_add(1, release)` on every `enqueue_external_message` and
on every requeue path (`service`, `wait_*`, `osSend`/`osJam`/`osRecv`).

| case | WORK_LOST | HINT_EVENTUALLY_TRUE |
| --- | --- | --- |
| R1 enqueue before acquire | NO | YES (then consumed) |
| R2 enqueue between acquire and drain | NO (drain sees it; extra loop) | YES |
| R3 enqueue during drain | NO (in-flight pop or next loop) | YES |
| R4 enqueue after drain, before last_seen | NO (now != seen → loop) | YES |
| R5 enqueue between last_seen and final load | NO (loop) | YES |
| R6 enqueue after final load / before return | NO | YES (hint true) |
| R7 one requeue, no new producer | NO | YES (special case) |
| R8 requeue + concurrent host publish | NO | YES |
| R9 many requeues + many producers | NO | YES |
| R10 epoch changes while swapped in `check_running_queue` | NO | YES |

```text
EPOCH_PROTOCOL_VALID: YES
LOST_WAKEUP_FOUND:    NO
```

### Special case cannot steal a concurrent publication

If `had_requeue`, `publish()` has already executed before `now` is loaded.
That contributes **exactly +1**. `now == seen + 1` iff no other `fetch_add`
occurred in `(acquire, now-load]`. A concurrent host publish is another
`fetch_add`, so `now >= seen + 2` and the loop continues.

Proof is sequencing + atomic increment, not an inference from tests.

R6 after a special-case break still leaves `last_seen == seen` and
`epoch >= seen + 1`, so hint stays true.

---

## 4. TLS `last_seen` across guest swaps

Each `OSThread` is its own `std::thread`. `tls_last_seen` is per native
guest thread, initialized to 0.

MAIN servicing epoch 100 then swapping to AUDIO (last_seen=0) makes AUDIO's
next service see 100 as unseen. Under the single-token model only one
service runs at a time. Redundant drain of an empty external queue is a
no-op; `check_running_queue` is the existing peek/swap. No reordering of
external work. No ping-pong. No storm unless a requeue keeps hint true
(intentional retry).

A global delivered-epoch would avoid the extra first service on a new
thread. Not required: no concrete defect.

```text
TLS_EPOCH_MODEL_VALID: CONDITIONAL
  A (per native guest TLS) is acceptable.
  B (global delivered epoch) would be a later optimization, not a fix.
```

---

## 5. Interrupt class / cause — highest-priority question

exceptasm.s (`rcp:` … `ai:` …) is cause-specific: `MI_INTR & global_rcp`,
then one `send_mesg(OS_EVENT_*)` per set class (SP/VI/AI/SI/PI/DP).

Hardware: an interrupt is taken only if `IE && !EXL && !ERL` **and** the
matching `SR.IM` bit **and**, for RCP, the matching `MI_INTR_MASK` bit.
All six RCP classes share CPU `IM2` (`0x400`). `OS_IM_AI = 0x00040401`
is `IE | IM2 | AI`. Masking only AI leaves `IM2` and VI able to fire;
it does **not** wake AUDIO.

PATCH 2:

1. Host producers enqueue regardless of mask (pre-existing).
2. `service` always drains (policy B).
3. `cpu_preemptible` is “any combined bit other than `OS_IM_NONE`”.

Discriminator (AI event, AI/RCP masked, VI open):

- Original: AI not taken; AUDIO not woken; no preemption from that event.
- PATCH 2: AI message delivered; AUDIO queued; VI-open makes the CPU
  preemptible; AUDIO (50) can preempt MAIN (20).

```text
PREEMPTION_NEEDS_EVENT_CAUSE:           YES
GENERIC_ANY_UNMASKED_CLASS_VALID:       NO
EVENT_DELIVERY_WHILE_CLASS_MASKED_VALID: CONDITIONAL
```

Classification (do not reopen F'):

```text
GENERIC_ANY_UNMASKED_AS_CAUSE_EQUIVALENT:     REFUTED
GENERIC_ANY_UNMASKED_AS_ALL-MASKED_SWAP_GATE: PROVEN (T03/T04)
CAUSE_FILTERED_DELIVERY:                      not implemented
```

This is a **hardware-fidelity gap in the approved F' swap gate**, not a
PATCH 2 coding error versus the spec. It cannot fire while the flag is OFF
(service is a no-op). It must be closed **before PATCH 4 enables the flag**,
not before PATCH 3 emits dead calls.

Delivery-while-masked is pre-existing: `osSend`/`osRecv` already drain
without consulting IE/IM/MI. Observationally close if the guest does not
`osRecv` the event during a partial-mask CS. Libultra audio uses
`OS_IM_NONE` (all classes), which T04 covers.

IE=0 vs class-masked vs EXL/ERL are **not** identical:

| case | original message visible? | PATCH 2 service |
| --- | --- | --- |
| A `IE=0` | no exception; pending in CAUSE/MI | drain, no swap |
| B `IE=1`, relevant `SR.IM` masked | no exception | drain; swap if any other class open |
| C `IE=1`, relevant MI class masked | no exception | same as B |
| D EXL/ERL | exceptions blocked | drain, no swap |

A and D match the approved gate. B/C are the X1 hole.

---

## 6. osSetIntMask vs setintmask.s

Original: compute `v0` first (`SR & OS_IM_CPU` plus bits hidden by
`__OSGlobalIntMask` plus `MI_INTR_MASK<<16`); write MI via
`__osRcpImTable` (set/clr pairs); write **only** `SR.IM`; **do not**
touch `IE`; `jr ra`.

PATCH 2 software model (no real MI):

- `current_combined_mask = (SR&IE) | (SR&IM) | ((g_mi_mask<<16)&RCP_IMASK)`
- with `g_global_int_mask=~0u`, hidden-bit OR is a no-op → return matches
- `g_mi_mask = ((a0 & RCP_IMASK)>>16) & global_rcp`
- `SR.IM = (a0 & SR_IMASK) & global_im`; IE untouched
- `OS_IM_NONE` / `OS_IM_ALL` / partial masks write the expected 6 RCP bits
  and CPU IM field

```text
RETURN_VALUE:        PASS (software model, global=~0)
CPU_IM_UPDATE:       PASS
RCP_MI_UPDATE:       PASS (logical 6-bit, not hardware set/clr)
GLOBAL_MASK_FILTER:  PASS (AND present; no-op while global=~0)
IE:                  PASS (untouched)
OS_IM_NONE/ALL:      PASS
OSSETINTMASK_SEMANTICS: PASS
```

`g_global_int_mask = ~0u` vs `OS_IM_ALL = 0x003fff01`: extra bits are
never consumed by the ANDs. Functionally equivalent.

Writers of `__OSGlobalIntMask` (libultra only; none in Tooie `src/`):

- `initialize.c` → `OS_IM_ALL`
- `__osSetGlobalIntMask` / `__osResetGlobalIntMask`
- `exceptasm.s`, `leointerrupt.c`, `devmgr.c`

No librecomp translations. Guest ROM copies, if recompiled, would mutate
RDRAM, not host `g_global_int_mask`.

```text
GLOBAL_INT_MASK_WRITERS: initialize + Set/Reset + exceptasm + leo + devmgr
                         (no Tooie src/; no host translation)
STATIC_GLOBAL_MASK_MODEL_VALID: CONDITIONAL
GLOBAL_INT_MASK_MODEL: static ~0u sufficient unless Tooie runs recompiled
                       Set/Reset/devmgr against host state (not observed)
```

---

## 7. MI mask mapping

`g_mi_mask` is the logical 6-bit enable word. `RCP_IMASK = 0x003F0000`.

| class | combined bit | `g_mi_mask` bit | `OS_IM_*` |
| --- | --- | --- | --- |
| SP | 16 / `0x00010000` | 0 | `0x00010401` |
| SI | 17 / `0x00020000` | 1 | `0x00020401` |
| AI | 18 / `0x00040000` | 2 | `0x00040401` |
| VI | 19 / `0x00080000` | 3 | `0x00080401` |
| PI | 20 / `0x00100000` | 4 | `0x00100401` |
| DP | 21 / `0x00200000` | 5 | `0x00200401` |

Matches `__osRcpImTable` index order. Default `0x3F` = all six open
(software “previous no-op world”, not hardware reset MI=0).

```text
MI_MASK_MAPPING: PASS
```

---

## 8. `cpu_preemptible`

Current: `IE && !EXL && !ERL && (combined & ~OS_IM_NONE) != 0`.

EXL/ERL belong (R4300). Combined IM+MI belongs as an *all-masked* gate.
“Any bit” is not event-specific (see §5). Software global mask is in
`current_combined_mask` via the AND at apply time, not re-read here.
CPU software/timer `IM0/IM1/IM7` are represented in `SR.IM` but this
runtime does not raise them as host events.

```text
CURRENT_FORMULA_CORRECT: CONDITIONAL
CPU_PREEMPTIBLE_FORMULA: CONDITIONAL
CORRECT_FORMULA_IF_DIFFERENT:
  keep current formula as the all-masked gate;
  before PATCH 4, either (a) do not deliver events whose class is
  masked, or (b) swap only if a just-delivered event's class is open.
```

---

## 9. Unmask immediate swap / `ctx->r2`

`apply_os_set_int_mask` computes `prev`, updates MI, updates IM, then may
`check_running_queue` **before** returning. Translation:

```text
ctx->r2 = apply_os_set_int_mask(rdram, &ctx->status_reg, ctx->r4);
```

If the swap happens inside apply, `ctx->r2` is still stale. Original
setintmask writes `v0` **before** `mtc0` IM, so a taken interrupt saves
the correct return.

In this runtime `swap_to_thread` parks the native `std::thread`
(`resume_thread_and_wait`). It does not flush GPRs into OSThread
RDRAM. The C local `prev` survives on the parked stack; after resume,
`ctx->r2` is assigned. Another guest cannot observe this thread's
`recomp_context`. Translations run only on the token-owning game thread.
MI is updated before the swap.

Original libultra **does** allow a higher-priority thread to run at
return from `osSetIntMask` / `__osRestoreInt` (pending interrupt after
`mtc0`). The check is the right approximation for already-runnable
threads.

```text
UNMASK_IMMEDIATE_SWAP_VALID: YES
RETURN_REGISTER_ORDERING_SAFE: YES   (park-native-thread model)
```

These checks are **not** gated on the preemption flag. In production
(flag OFF) they only swap if a higher thread is already on the run
queue. Coop `osSend`/`osRecv` already do that. Practically a no-op
until service can wake-without-swap.

---

## 10. Disable / restore

Original `__osRestoreInt`: `SR |= a0`. Not a store of saved IE.
Nested:

```text
a = disable();  // IE was 1 → return 1, IE=0
b = disable();  // IE was 0 → return 0, IE=0
restore(b);     // IE |= 0 → 0
restore(a);     // IE |= 1 → 1
```

Final IE = 1. `restore(0)` cannot clear IE. PATCH 2 matches.
VERSION_J global-IM rewrite during disable is unimplemented; inert
while `g_global_int_mask` is static.

```text
DISABLE_RESTORE_SEMANTICS: PASS
```

---

## 11. CP0 Status

`cop0_status_write` still handles FR / `switch_error` / `f_odd`.
IE/EXL/ERL/IM are stripped from `changed`; leftover still `assert+exit`.
`ctx->status_reg = new_sr` **does** run. Allowing EXL/ERL is conservative
(`cpu_preemptible` false).

No `check_running_queue` on MTC0 that re-enables IE/IM. A higher-priority
runnable thread waits until the next safepoint or coop API. Original
`mtc0 SR` can take a pending interrupt immediately.

Tooie `src/` has no `__osSetSR` / `osGetIntMask` call sites. Typical
paths are `osSetIntMask` / disable / restore, which do check.

```text
MTC0_REENABLE_NEEDS_IMMEDIATE_SERVICE: YES
PATCH2_BLOCKER: NO
```

C ABI `ultramodern_service_preemption(uint8_t*, void*)` **ignores `ctx`**
and uses `status_reg_init()` (fully unmasked). Harness mask tests use
the C++ API. PATCH 3 must wrap this or T03/T04 become fiction the
moment generated code calls the C symbol.

---

## 12. Status initialization

`run_thread_function`: `ctx.status_reg |= IE|IMASK`. Applies to every
OSThread that starts guest code (MAIN, AUDIO, …), including live
`get_function` entry. `init()` boot context is **not** initialized
(remains 0 if the caller zero-inits).

Original `osCreateThread`: `sr = (OS_IM_ALL & (IM|IE)) | EXL` = `0xFF03`,
then `__osDispatchThread` / `eret` clears EXL. Running SR = `IE|IMASK`.
PATCH 2 matches the **running** value, not the stopped EXL=1 snapshot.
IM is a fixed all-ones, not copied from the creator (creator is also
`OS_IM_ALL` in libultra).

```text
STATUS_INIT_SEMANTICS: APPROXIMATE
```

---

## 13. MI state thread safety

`g_mi_mask` / `g_global_int_mask` are plain `uint32_t`. Host producers
do **not** read them (only `enqueue` → epoch). Only the token-holding
game thread writes them.

```text
MI_STATE_DATA_RACE: NO
```

---

## 14. External queue / requeue

Base `dequeue_external_messages` already drained into a vector then
re-enqueued. PATCH 2 adds `bool` return, `#include <vector>`, and
publish on enqueue/requeue. Relative requeue order is preserved.
`jam` stays on the `QueuedMessage` and is applied at the next
`do_send`.

Newly arriving messages can interleave between drain and requeue: an
older requeued event can land **behind** a newer one. **Pre-existing**,
not introduced by PATCH 2. MPMC does not promise a global order.

```text
REQUEUE_ORDERING: PREEXISTING drain-then-append; PATCH 2 adds publish only
```

---

## 15. Tests (test the tests)

| test | ACTUALLY_EXERCISES_TARGET | CAN_FALSE_PASS |
| --- | --- | --- |
| T02 | YES (service + pri-50 swap) | NO |
| T03 | YES (IE=0 no swap; delivery; restore swaps) | NO |
| T04 | YES for OS_IM_NONE no-swap + later swap; WEAK return check (`saved_mask & OS_IM_NONE`) | PARTIAL — does not read IM/MI bits |
| T05 | YES (equal pri stays queued) | NO |
| T06 | YES (lower pri stays queued) | NO |
| T07 | YES for delivery of both; does **not** assert AUDIO-before-VI | YES for “highest runs first” |
| T08 | YES (hint true after requeue; msg recvd after space+service) | NO |
| T09 | YES (1000 real token swaps) | NO |
| T10 | YES (single-producer 1,2,3); no multi-producer claim | NO |
| T12 | YES (dup insert no self-link; non-head remove) | NO |

T01-E is repaired (service after host enqueue, flag OFF).

### Proposed tests (do not implement)

```text
X1  AI masked, VI open, AI event — must NOT swap to AUDIO
X2  MTC0 re-enable with higher-pri runnable
X3  event arrives while current guest is parked in swap
X4  new thread TLS last_seen=0 after large epoch (redundant service OK)
X5  multiple persistent requeues (hint stays true; no inner livelock)
X6  osSetIntMask swap before ctx->r2 write (caller still sees prev)
X7  __OSGlobalIntMask mutation if a writer is ever wired
```

```text
T02_T10_T12_QUALITY: ADEQUATE for PATCH 2 gate; T04 return WEAK; T07 order WEAK
MISSING_TESTS: X1–X7 (X1 required before PATCH 4 enable)
```

---

## 16. Production-off claim

```text
PRODUCTION_SCHEDULER_PREEMPTION_ACTIVE: NO
PRODUCTION_INTERRUPT_SEMANTICS_CHANGED: YES
PATCH2_PRE_PATCH3_REGRESSION_RISK:      MEDIUM
```

Flag defaults false (`g_guest_preemption_enabled{}`). Hint is false.
Service returns immediately. `RecompiledFuncs` has **0** calls to
hint/service. Tooie `nm` shows hint/service/intmask linked (expected:
`mesgqueue` pulls `publish`; translations pull intmask).

Translations are **live**: `osSetIntMask` / disable / restore / MTC0
IE/IM now mutate real software state and write `v0`. Previous stubs
left `r2` unset. Tooie has many `osSetIntMask` CS sites (including
`func_800205D0`). Standard `mask = osSetIntMask(OS_IM_NONE); …;
osSetIntMask(mask);` is now a real mask/unmask. That is closer to
libultra and is the intended PATCH 2 change. It is **not** “behavior
unchanged”.

Full intro smoke was **skipped**: existing `TooieRecompiled` (2026-08-13
01:36) already links PATCH 2; a clean intro→Jiggy run is minutes of
Vulkan and is not a cheap discriminator. C2AB8 remains expected
without PATCH 3.

---

## 17. Verdict

```text
VERDICT: B — APPROVE WITH NONBLOCKING ISSUES
PATCH3_READY: YES
```

Not A: X1 fidelity gap, C ABI ignores `ctx`, MTC0 has no immediate
reschedule, T04/T07 weak, production translations are a real semantic
change.

Not C/D: no lost wakeup; special case is sound; reconstruction +
harness independently PASS; implementation matches approved F';
cause/class is a documented approximation that stays inert with the
flag OFF. Generator work does not depend on closing X1 first.

### Nonblocking issues

1. X1 cause/class (must close before PATCH 4 enable).
2. C ABI service ignores `ctx` — PATCH 3 **must** wrap.
3. `cop0_status_write` does not `check_running_queue`.
4. T04 return value only checks the IE bit.
5. T07 does not assert AUDIO-before-VI.
6. `init()` boot `status_reg` not initialized.
7. Production `osSetIntMask`/`restore` now have side effects.

### PATCH 3 constraints (inherited)

- C ABI remains `void*`; do **not** declare `recomp_context*` in
  `recomp.h` while `ultramodern.hpp` has `void*`.
- Generated fast path must go through a librecomp wrapper that
  extracts `&ctx->status_reg` into `ultramodern::service_preemption`.
- Static/live generator parity: one request site in
  `recompilation.cpp`; both backends implement the same virtual.
- Safepoint **after** delay slot, before goto when
  `target_vram <= instr_vram`; also immediately before `emit_switch`.
- Never between branch and delay slot.
- Flag remains OFF. No Tooie regen. Isolate hunks from dirty
  `resolve_jal`.
- Do not treat generic `cpu_preemptible` as cause-equivalent.
- Keep coop `osSend`/`osRecv`/`osJam` ungated.

```text
NEXT_SINGLE_ACTION:
  implement PATCH 3 generator safepoints only (flag OFF, no Tooie regen)
```

=== END PATCH 2 INDEPENDENT REVIEW ===
