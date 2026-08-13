---
title: "P08 — independent review of preemption PATCH 1"
description: "Revisão independente (Grok 4.6 HIGH) do Hybrid F' PATCH 1. Veredito B: aprovar com issues menores de teste/ABI futuro. PATCH 2 ready. Sem alteração de runtime/generator."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch1, independent-review, n64modernruntime]
---

# P08 — INDEPENDENT REVIEW OF PREEMPTION PATCH 1

```text
MODE:                 READ / TEST / DOCS ONLY
MODEL:                Grok 4.6 HIGH
ROLE:                 Independent senior runtime reviewer (did not implement)
TOOIE_HEAD:           417880fc4ea4e78387399e3f32d1b58fd8c62915
RUNTIME_BASE:         ca568b6ad79b9029d14077f0c3ffa757727c5559
ARCHITECTURE:         Hybrid F' (not reopened)
VERDICT:              B — APPROVE WITH DOC/TEST MINOR ISSUES
PATCH2_READY:         YES
SOURCE_CODE_CHANGED:  NO
```

Do not reopen Hybrid F'. Do not treat missing PATCH 2 behavior as a PATCH 1
defect. DeepSeek Hybrid F remains `CORRECTED/SUPERSEDED`.

---

## 0. Scope

Reviewed the versioned artifact, not only the implementer evidence.

Canonical inputs:

- [[p08-preemption-design-adversarial-review]]
- [[p08-preemption-implementation-plan]]
- [[p08-preemption-patch1-validation]]
- `patches/runtime/n64modernruntime-preemption-patch1.patch`
- `tools/apply_runtime_patches.sh`

Known external dirty state was **not** treated as PATCH 1:

- `ultramodern/src/threadqueue.cpp` (Fix B)
- `ultramodern/src/threads.cpp` (osSetThreadPri NULLPTR)
- `N64Recomp/src/main.cpp`, `src/recompilation.cpp` (Tooie JAL)

---

## 1. Versioned diff

```text
PATCH_HASH:
  expected 1dc7bab8cc5004b2243f1e890c0a79b6bc094532f247cdefba8350d7030521dd
  actual   1dc7bab8cc5004b2243f1e890c0a79b6bc094532f247cdefba8350d7030521dd
PATCH_HASH_VALID: YES
apply_runtime_patches.sh pins the same SHA. PROVEN.
```

| path | class |
| --- | --- |
| `ultramodern/CMakeLists.txt` | EXPECTED |
| `ultramodern/include/ultramodern/ultramodern.hpp` | EXPECTED |
| `ultramodern/src/preemption.cpp` | EXPECTED |
| `ultramodern/src/intmask.cpp` | EXPECTED |
| `ultramodern/test/preemption_harness.cpp` | EXPECTED |

```text
PATCH_CONTAINS_PREEXISTING_DIRTY_FILES: NO
```

No hunk touches `threadqueue.cpp`, `threads.cpp`, `mesgqueue.cpp`,
`scheduling.cpp`, `recomp.h`, generator sources, or Tooie `src/**` /
`RecompiledFuncs/**`.

---

## 2. Tooie commit scope-creep

`417880fc` changes only:

| file | class |
| --- | --- |
| `patches/runtime/n64modernruntime-preemption-patch1.patch` | EXPECTED |
| `tools/apply_runtime_patches.sh` | EXPECTED |
| `docs/plan/evidence/p08-preemption-patch1-validation.md` | EXPECTED |
| `docs/plan/evidence/p08-preemption-implementation-plan.md` | EXPECTED |
| `docs/plan/evidence/p08-session-handoff.md` | EXPECTED |
| `docs/HANDOFF.md` | EXPECTED |
| `docs/plans/BT/08-rt64-input-and-frontend.md` | EXPECTED |
| `docs/VAULT.md` | EXPECTED |

No unexpected path. **PROVEN.**

---

## 3. Clean reconstruction

Disposable clone at `/tmp/p08-p1-independent-review/N64ModernRuntime`.
The real dirty checkout was not reset, restored, or rewritten.

Start: runtime `ca568b6`, N64Recomp `2b6f056`. Working trees clean.

Canonical order, versioned artifacts only:

1. `n64modernruntime-scheduler.patch`
2. `n64modernruntime-preemption-patch1.patch`
3. `n64recomp-same-section-jal.patch`

```text
CLEAN_RECONSTRUCTION: PASS
SCHEDULER_PATCH:      PASS
PREEMPTION_PATCH1:    PASS
JAL_PATCH:            PASS
PATCH_ORDER_DEPENDENCY:
  none between scheduler and PATCH 1 (disjoint paths).
  JAL is a different repository. Order is policy, not a hunk dependency.
```

Reconstructed Fix B / NULLPTR hashes match the real dirty tree:

```text
threadqueue.cpp 1c61c91985dddfc8919f41afb54aa4ce1c74d5d9e0c4476e51dd4c34a60dbeae
threads.cpp     d12dd1335a79748711e0bf0763f9c638a424fe7b9f0701c3b1a2565970340f99
```

---

## 4. Idempotency

Script logic emulated twice on the disposable tree. Real
`tools/apply_runtime_patches.sh` then run once against the already-applied
production checkout (reverse-check only; no write).

```text
FIRST_APPLICATION:  all three patches applied
SECOND_APPLICATION: all three "already applied (reverse --check)"
IDEMPOTENT:         YES
PATCH_SCRIPT_IDEMPOTENT: YES
```

Real-tree dirty exclusives unchanged after the live script run. **PROVEN.**

---

## 5. Patch interaction

```text
PATCH1_SCHEDULER_OVERLAP:           NO
PATCH1_DEPENDS_ON_FIX_B_AT_BUILD_TIME: NO
PATCH1_DEPENDS_ON_FIX_B_AT_RUNTIME:    NO
```

PATCH 1 does not compile against Fix B symbols. Feature is OFF; T01 does not
double-insert. Scheduler files remain exclusively owned by the existing
scheduler patch.

---

## 6. C ABI / PATCH 3 collision

Current declarations (file scope, after `namespace ultramodern`):

```cpp
extern "C" {
bool ultramodern_preemption_hint(void);
void ultramodern_service_preemption(uint8_t* rdram, void* ctx);
}
```

```text
C_ABI_VALID: YES
```

- Unmangled names match `pause_self` style. Object file exports
  `ultramodern_preemption_hint` / `ultramodern_service_preemption` (`T`).
- File-scope placement is correct (not inside the C++ namespace).
- `void* ctx` is a safe temporary PATCH 1 ABI. `uint8_t` is visible via
  `ultra64.h`. Generated C does not include `ultramodern.hpp`.
- `recomp_context` is an **anonymous** `typedef struct { ... } recomp_context;`
  in `N64Recomp/include/recomp.h`. A `struct recomp_context` forward
  declaration is therefore impossible today. `void*` is the necessary
  PATCH 1 choice. **PROVEN.**

```text
PATCH3_DECLARATION_COLLISION_RISK: HIGH if PATCH 3 adds
  void ultramodern_service_preemption(uint8_t*, recomp_context*);
  to recomp.h while ultramodern.hpp keeps void*.
```

Compile probe (disposable): including both headers is fine today. Adding the
plan's `recomp_context*` declaration next to the current `void*` declaration
is a hard error:

```text
conflicting declaration of C function
  void ultramodern_service_preemption(uint8_t*, recomp_context*)
previous declaration
  void ultramodern_service_preemption(uint8_t*, void*)
```

Many `librecomp` TUs already include both headers
(`ultra_translation.cpp`, `recomp.cpp`, `pi.cpp`, …). This is **not** a
PATCH 1 compile defect. It **is** a PATCH 3 contract that must be decided
before `recomp.h` grows the symbol.

Safer PATCH 3 options (do not change PATCH 1):

1. Keep `void*` in **both** headers forever; cast inside the implementation.
2. Name the struct (`typedef struct recomp_context { ... } recomp_context;`)
   and use `recomp_context*` in both headers.
3. Declare the C ABI in only one header.

Do not follow the implementation-plan snippet literally without one of those.

---

## 7. Stub semantics

`preemption.cpp`:

- `g_guest_preemption_enabled` is an anonymous-namespace `std::atomic<bool>`
  constant-initialized to `false`. No dynamic ctor / startup side effect.
- Setter/getter use `memory_order_relaxed`. Fine: no other PATCH 1 reader.
- `ultramodern_preemption_hint` is `return false;` (disassembly: `mov $0,%eax`).
- `ultramodern_service_preemption` stores args to the frame and returns
  (Debug). No drain, no swap, no epoch.

```text
FEATURE_EFFECTIVELY_OFF:              YES
OBSERVABLE_RUNTIME_CHANGE_FROM_STUBS: NO
```

No existing runtime call site references the new symbols. `setter=true` is
observable only through the getter; hint stays false. No `getenv`. **PROVEN**
by source + object dump.

---

## 8. intmask.cpp scaffold

Namespace `ultramodern::intmask`. External C++ linkage. No header. No call
sites. Defaults match F' (`g_mi_mask=0x3F`, `g_global_int_mask=~0u`,
`default_status_reg() -> OS_IM_ALL`). No static initializer beyond two
`uint32_t`s.

```text
INTMASK_SCAFFOLD_SAFE: YES
SHOULD_INTMASK_CPP_HAVE_BEEN_DEFERRED_TO_PATCH2: YES — STYLE ONLY
```

Not an ODR / export / startup defect. Dead archive member until PATCH 2.
Not a real PATCH 1 defect.

---

## 9. CMake isolation

```text
DEFAULT_BUILD_GRAPH_CHANGED: NO   (ALL still = libultramodern only)
PRODUCTION_BEHAVIOR_CHANGED: NO
TEST_TARGET_ISOLATED:        YES
```

- `option(ULTRAMODERN_BUILD_TESTS ... OFF)` is acceptably namespaced.
- Default OFF: harness target is not created. Tooie cache has
  `ULTRAMODERN_BUILD_TESTS:BOOL=OFF`.
- Independent Debug configure with `-DULTRAMODERN_BUILD_TESTS=ON`: default
  `cmake --build` built `ultramodern` only; `preemption_harness` appeared
  only after `--target preemption_harness`. `EXCLUDE_FROM_ALL` behaves.
- `find_package(Threads)` is inside the tests `if`.
- Parent-option collision: possible in principle (CMake cache is global) but
  Tooie / N64ModernRuntime do not define the same option. Non-blocking.
- Two extra `.cpp` files always compile into `libultramodern.a`. That is
  expected scaffold, not a test leak.

---

## 10. Static archive / link isolation

Independent dummy that references only `osCreateMesgQueue` against the
reconstructed PATCH 1 archive: `nm` has **no** preemption/intmask symbols.

Existing `TooieRecompiled` (2026-08-13 00:52, PATCH 1 tree): same empty `nm`.

```text
PREEMPTION_OBJECT_LINKED_IN_PRODUCTION: NO
INTMASK_OBJECT_LINKED_IN_PRODUCTION:    NO
```

If they were linked it would still not matter in PATCH 1 (hint false,
service empty, intmask unreferenced). Extra isolation is real.

Archive size Debug: baseline `5428210` vs PATCH 1 `5506336` (two extra
members). Baseline archive `nm` has no new symbols.

---

## 11. Builds

```text
BASELINE_BUILD:  PASS  (disposable ultramodern, scheduler only)
PATCH1_BUILD:    PASS  (disposable ultramodern + explicit harness)
FEATURE_OFF_BUILD_REGRESSION: NO
```

Full TooieRecompiled was **not** rebuilt from the disposable tree (would
require pointing Tooie at the temp runtime). Production isolation was
checked on the existing Tooie binary + dummy link. Standalone library
builds are the independent compile proof.

---

## 12. Harness / T01

Independent run of reconstructed `preemption_harness`:

```text
T01 PASS  (pass=15 fail=0 skipped=11)  exit 0
T02–T12: SKIPPED / NOT_IMPLEMENTED
```

Layout audit:

| item | result |
| --- | --- |
| `sizeof(OSThread)` | 48; 256-byte gap leftover 208 |
| `sizeof(OSMesgQueue)` | 24 |
| `sizeof(OSMesg)` | 4 |
| RDRAM | `0x8000`; max used guest offset `0x2FF0` (`sp-0x10`) |
| `TO_PTR` | sign-extends `0x8000xxxx` correctly |
| Native stack | harness entries ignore guest `sp`; RDRAM stacks unused |
| `running_queue` | host sentinel, not in RDRAM |
| atomics | acquire/release on coop flags |
| enqueue | lock-free only; does **not** wake guest |

`std::this_thread::yield` while MAIN holds the token is a host wait for
`host_enqueued` / `audio_blocked`. It does not drain `external_messages` and
does not substitute guest scheduling. Acceptable.

### T01 semantic matrix

| claim | PROVEN_BY_TEST |
| --- | --- |
| A flag defaults false | YES |
| B setter observable; hint stays false | YES |
| C service stub does nothing | PARTIAL — null call does not crash; no postcondition |
| D AUDIO blocks before coop service | YES (`audio_ran==false` after block, before enqueue) |
| E service stub does not wake AUDIO | PARTIAL / WEAK |
| F existing coop path wakes AUDIO | YES |
| G MAIN resumes | YES |
| H feature OFF does not replace scheduler | YES |

### Exact T01-E weakness

`main_entry` calls `ultramodern_service_preemption` **before** waiting for
`host_enqueued`. `service_did_not_wake` is latched while the external queue
is typically still empty.

Common interleaving (1 ms host poll vs immediate `osRecvMesg`):

1. AUDIO sets `audio_blocked`, blocks on first recv, token returns to MAIN.
2. MAIN calls `service()` on an empty queue.
3. Host then enqueues and sets `host_enqueued`.
4. MAIN `osSendMesg` drains and wakes AUDIO.

A no-op `service()` is obvious from source/disassembly. The test can PASS
without ever calling `service()` against a pending external message. That is
a **false-positive structure for claim E**, not a runtime bug.

Rare opposite race (host enqueue between `audio_blocked` and AUDIO's first
`dequeue_external_messages`) would make first recv consume the message,
leave `service_did_not_wake` false, and **fail** T01. Unlikely with a 1 ms
poll; still a harness race.

Corrective plan (do **not** implement in this review):

```text
MAIN: wait audio_blocked
      wait host_enqueued
      service()
      settle (short timed wait / yield) while checking audio_ran==false
      latch service_did_not_wake
      osSendMesg dummy   // existing coop drain
      latch main_resumed
```

T02+ will exercise a real `service()`. Reorder T01-E when PATCH 2 edits the
harness.

```text
T01_FLAG_TEST:         PASS
T01_SERVICE_NOOP_PROOF: WEAK
T01_COOP_WAKE_PROOF:   PASS
```

F is proven by protocol, not just CHECK order: `enqueue_external_message`
does not wake; only the later guest `osSendMesg` dequeues.

---

## 13. Harness shutdown

MAIN loops on `yield_self_1ms`. AUDIO parks on a second blocking recv.
`std::_Exit` skips destructors and kills native threads. `g_rdram` /
`g_coop` are nulled after checks; guest threads no longer touch those
globals. `init_thread_cleanup()` is never called; threads never return
(would throw "No threads left to run!").

```text
HARNESS_SHUTDOWN_SAFE: CONDITIONAL
```

Severity: **low**. Process-only test remains reliable. Killing live
`std::thread`s via `_Exit` is intentional and documented. Not production
code. Not a PATCH 1 blocker.

---

## 14. apply script

```text
PATCH_SCRIPT_CORRECT: YES
```

Correct current behavior:

- SHA pin per artifact
- HEAD must remain the **base** commit; patches are working-tree diffs
- reverse-check = already applied
- `git apply --check` before apply
- unrelated dirty files are left alone
- scheduler then PATCH 1 then JAL; disjoint paths
- `set -e`: failed `--check` aborts

Potential issues (not current defects):

1. **PATCH 2 incremental on PATCH 1 files breaks reverse-check of PATCH 1.**
   After PATCH 2 edits `preemption.cpp` / the header / CMake / harness,
   `git apply --reverse --check patch1` fails, then forward `--check`
   also fails → script exit 2. Idempotency of a stacked incremental
   PATCH 2 would be broken unless the script learns a patch stack.
2. Partial apply: if patch 2/3 fails, earlier patches stay applied.
3. One-of-two applied is fine today because the artifacts do not overlap.

---

## 15. PATCH 2 artifact strategy

```text
RECOMMENDED_PATCH_ARTIFACT_STRATEGY: B
```

**B — replace the PATCH 1 artifact with a cumulative preemption patch**
against the same runtime base `ca568b6`.

Why B is safer with the current script:

- All preemption files will be rewritten in PATCH 2.
- Reverse-check is the only idempotency signal and assumes each artifact's
  result is still present on disk.
- Scheduler stays a separate disjoint patch.
- One SHA to pin; one reverse-check; same-base working-tree apply remains.

A (keep patch1 + incremental patch2) is review-friendly but **requires a
script change** (if later reverse-check succeeds, skip earlier preemption
patches). Do not choose A without that change.

Migration for B: a tree that still has only the old PATCH 1 must reverse
that artifact, then apply the cumulative one. Document it in the PATCH 2
evidence. Do not `git reset` the dirty Fix B / JAL files.

---

## 16. Verdict

```text
VERDICT: B — APPROVE WITH DOC/TEST MINOR ISSUES
PATCH2_READY: YES
```

Not C/D: no feature-OFF behavior bug, no dirty-file contamination, no ABI
break in PATCH 1, no failed reconstruction, no production link of the new
objects.

### BLOCKING_ISSUES

```text
none
```

### NONBLOCKING_ISSUES

1. T01-E calls `service()` before host enqueue; `service_did_not_wake` can
   latch on an empty queue. Fix when PATCH 2 touches the harness.
2. PATCH 3 must not declare `recomp_context*` beside the current `void*`
   C ABI. Decide `void*` everywhere, or name the struct, before PATCH 3.
3. `intmask.cpp` is unused scaffold (style). Harmless.
4. Harness teardown is `_Exit` of live guest host threads (intentional).
5. Choose artifact strategy **B** before writing PATCH 2.

### PATCH 2 constraints (do not implement here)

- Flag remains OFF for games.
- Do not edit `threadqueue.cpp` / `threads.cpp` / dirty JAL hunks.
- Real publish / service / intmask translations / T02–T10 / T12 only.
- Reorder T01-E while editing the harness.
- Replace `n64modernruntime-preemption-patch1.patch` with a cumulative
  artifact and update the SHA in `apply_runtime_patches.sh`.

---

## Classification

| claim | class |
| --- | --- |
| PATCH hash / path list / no dirty-file hunks | PROVEN |
| Clean reconstruction + idempotent apply | PROVEN |
| Feature OFF; unused .o not linked | PROVEN |
| T01-A flag/hint | PROVEN |
| T01 coop wake + MAIN resume | PROVEN |
| T01-E service-does-not-wake | PARTIAL |
| PATCH 3 `void*` vs `recomp_context*` collision | PROVEN (compile probe) |
| Hybrid F' architecture changed | REFUTED |
| Guest preemption implemented | REFUTED |
| T02–T12 pass | REFUTED (SKIPPED) |

=== END INDEPENDENT REVIEW ===
