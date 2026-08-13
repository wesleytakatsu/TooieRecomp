---
title: "P08 — preemption PATCH 1 scaffold validation"
description: "Validação da infraestrutura Hybrid F' PATCH 1: stubs, flag OFF, harness T01, isolamento do dirty pré-existente."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch1, n64modernruntime, harness]
---

# P08 — PREEMPTION PATCH 1 VALIDATION

## Meta

```text
MODE:                 IMPLEMENT PATCH 1 ONLY
MODEL:                Grok 4.6 HIGH
ARCHITECTURE:         Hybrid F' (APPROVED)
TOOIE_BASE_HEAD:      9bce91d2d4c7e44b4e198d234abaed95cd2863d4
RUNTIME_BASE_HEAD:    ca568b6ad79b9029d14077f0c3ffa757727c5559
IMPLEMENTATION_READY: PATCH 1 landed; PATCH 2 not started
```

Do not reopen Hybrid F'. DeepSeek Hybrid F remains `CORRECTED/SUPERSEDED`.

---

## PREEXISTING_STATE

Captured **before** any PATCH 1 write. **PROVEN.**

```text
TOOIE:
  HEAD 9bce91d2d4c7e44b4e198d234abaed95cd2863d4
  worktree clean

N64ModernRuntime (BanjoRecomp/lib/N64ModernRuntime):
  HEAD ca568b6ad79b9029d14077f0c3ffa757727c5559  (detached)
  branch policy: external runtime left uncommitted
  dirty:
    M ultramodern/src/threadqueue.cpp   (Fix B)
    M ultramodern/src/threads.cpp       (osSetThreadPri NULLPTR)
    m N64Recomp

N64Recomp submodule:
  HEAD 2b6f05688de2abc7d86da5b4a89b84c2c6acbabe  (detached)
  dirty:
    M src/main.cpp
    M src/recompilation.cpp             (Tooie JAL policy)

FILE HASHES (pre + post PATCH 1, identical):
  threadqueue.cpp     1c61c91985dddfc8919f41afb54aa4ce1c74d5d9e0c4476e51dd4c34a60dbeae
  threads.cpp         d12dd1335a79748711e0bf0763f9c638a424fe7b9f0701c3b1a2565970340f99
  N64Recomp/main.cpp  fb0ae67cfaf17902867d69b7b3b7aa499e4adabf0d7da921ce6a1a5fcd4035dd
  recompilation.cpp   db8b443142b2513e3a5d00c2b0c453d387d70e95840b47b6de284b556e2eeede

DIRTY DIFF SHA256 (matches existing Tooie patch artifacts):
  scheduler patch     1b3d3d4666e02f748c44d19d867d02669b83753d85fe7eb62d746a5a8cf38309
  same-section JAL    0afeb0e57bfdd18be7a125bebe4f7bd48273622c5f5f626ae3279fd1ded0efa8
```

`DIRTY_FILES_PRESERVED_UNTOUCHED: YES` — **PROVEN** by SHA-256 before and after.

Do not `git reset` / `checkout` / `restore` / `clean` / `stash` this tree.

---

## SLICING CORRECTION

```text
RECOMP_H_REQUIRED_IN_PATCH1: NO
```

**PROVEN.** PATCH 1 has no generated safepoints. Generated C does not call
`ultramodern_preemption_hint` / `ultramodern_service_preemption`. The C ABI
is declared at file scope in `ultramodern.hpp` with `void* ctx` because
`recomp_context` is an anonymous `typedef struct` in `recomp.h` and
ultramodern must not gain a N64Recomp include dependency.

`N64Recomp/include/recomp.h` decls are deferred to **PATCH 3** (when the
generator emits the calls). This is implementation slicing, not an
architecture change.

N64RECOMP_COMMIT: **NONE**. Dirty JAL files remain unstaged and uncommitted.

---

## FEATURE FLAG / ENV

```text
PREEMPTION_DEFAULT:                 OFF
ULTRAMODERN_GUEST_PREEMPTION env:   DEFERRED to PATCH 4
set_guest_preemption_safepoints:    stores process flag only
guest_preemption_safepoints_enabled: reports stored flag
ultramodern_preemption_hint:        always false in PATCH 1
                                    (even if setter is true)
```

Env parsing is assigned to PATCH 4 (bisect switch when the default becomes
ON). PATCH 1 must not introduce observable behavior from the environment.
**Choice: no getenv.** **PROVEN** by source of `preemption.cpp`.

---

## FILES

Created (runtime worktree, uncommitted; versioned via Tooie patch):

- `ultramodern/src/preemption.cpp`
- `ultramodern/src/intmask.cpp`
- `ultramodern/test/preemption_harness.cpp`

Modified (runtime):

- `ultramodern/CMakeLists.txt` — add sources; `ULTRAMODERN_BUILD_TESTS` +
  `preemption_harness` `EXCLUDE_FROM_ALL`
- `ultramodern/include/ultramodern/ultramodern.hpp` — C++ flag API + C ABI

Tooie:

- `patches/runtime/n64modernruntime-preemption-patch1.patch` **NEW**
- `tools/apply_runtime_patches.sh` — apply the new patch
- this evidence note + handoff/plan updates

Not touched:

- `threadqueue.cpp`, `threads.cpp`, `mesgqueue.cpp`, `scheduling.cpp`
- `librecomp` translations / `cop0_status_write` / `run_thread_function`
- N64Recomp generator / `recomp.h` / dirty JAL hunks
- Tooie `src/**`, `RecompiledFuncs/**`

---

## STUB SEMANTICS

```text
HINT_BEHAVIOR:            return false
SERVICE_STUB_BEHAVIOR:    ignore rdram/ctx; no drain; no swap
ENQUEUE_CHANGED:          NO
INTMASK_TRANSLATIONS:     NO (defaults only; no call sites)
COP0_CHANGED:             NO
GENERATOR_CHANGED:        NO
GENERATED_CODE_CHANGED:   NO
TOOIE_GAME_CODE_CHANGED:  NO
```

`intmask.cpp` holds software defaults (`SR_IE/EXL/ERL/IM`, `OS_IM_*`,
`g_mi_mask=0x3F`, `g_global_int_mask=~0u`) for PATCH 2. Nothing calls it.

---

## BUILD

```text
BASELINE_BUILD:  PASS  (pre-existing; ninja no work)
  cmake --build TooieRecomp/build --target ultramodern
  cmake --build TooieRecomp/build --target TooieRecompiled

DEFAULT_BUILD:   PASS
  same Tooie command after PATCH 1
  CMake re-ran; compiled preemption.cpp + intmask.cpp into libultramodern.a
  relinked TooieRecompiled
  ULTRAMODERN_BUILD_TESTS:BOOL=OFF in Tooie cache
  preemption_harness not built in the Tooie tree
  nm TooieRecompiled | grep preemption → empty
    (static unused .o not pulled — extra isolation)

HARNESS_BUILD:   PASS
  cmake -S BanjoRecomp/lib/N64ModernRuntime/ultramodern \
        -B /tmp/ultramodern-preemption-harness \
        -DULTRAMODERN_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
  cmake --build ... --target preemption_harness

HARNESS_RUN:     PASS
T01:             PASS   (pass=15 fail=0 skipped=11, exit 0)
T02–T12:         SKIPPED / NOT_IMPLEMENTED
```

T01-A: flag defaults false; setter reports state; hint stays false; service
no-op with null args.

T01-B: MAIN pri 20 / AUDIO pri 50. AUDIO blocks on `osRecvMesg`. Host
`enqueue_external_message`. Service stub does **not** wake AUDIO. Existing
`osSendMesg` drain + `check_running_queue` wakes AUDIO. Same-thread
send/recv 42 still works. Feature remains OFF.

Harness supplies `run_thread_function` + `exited` so it links ultramodern
alone (those symbols live in librecomp in the game).

---

## BEHAVIORAL ISOLATION

```text
PREEMPTION_FEATURE_ENABLED:  NO     PROVEN (hint false; no game call sites)
ENQUEUE_BEHAVIOR_CHANGED:    NO     PROVEN (mesgqueue.cpp untouched)
MESSAGE_DELIVERY_CHANGED:    NO     PROVEN
SCHEDULER_BEHAVIOR_CHANGED:  NO     PROVEN (scheduling.cpp untouched)
GENERATED_CODE_CHANGED:      NO     PROVEN
TOOIE_GAME_CODE_CHANGED:     NO     PROVEN
PERFORMANCE:                 NONE / NEGLIGIBLE
                             (no safepoints; unused .o not in the game binary)
```

---

## COMMIT TOPOLOGY

Project policy (existing `apply_runtime_patches.sh` + scheduler commit):
**external N64ModernRuntime / N64Recomp left uncommitted.** Changes are
versioned as Tooie patch artifacts. PATCH 1 follows that policy.

```text
RUNTIME_COMMIT:           NONE  (worktree dirty + PATCH 1 files; not committed)
N64RECOMP_COMMIT:         NONE
UPSTREAM_PUSH:            NO
DEPENDENCY_PATCH_ARTIFACT: patches/runtime/n64modernruntime-preemption-patch1.patch
  SHA256 1dc7bab8cc5004b2243f1e890c0a79b6bc094532f247cdefba8350d7030521dd
  base   ca568b6ad79b9029d14077f0c3ffa757727c5559
  git apply --reverse --check  OK on the current worktree
```

Paths in the patch (only these):

- `ultramodern/CMakeLists.txt`
- `ultramodern/include/ultramodern/ultramodern.hpp`
- `ultramodern/src/intmask.cpp`
- `ultramodern/src/preemption.cpp`
- `ultramodern/test/preemption_harness.cpp`

---

## DEFERRED (do not start)

PATCH 2: epoch publish, enqueue/requeue, real service, intmask translations,
`status_reg` init, T02–T10, T12.

PATCH 3: generator safepoints, `recomp.h` decls, T11.

PATCH 4: Tooie regen, flag ON, env override, T13–T15.

`func_800BCF28` remains `NEW_DOWNSTREAM_BLOCKER`.

---

## Classification

| claim | class |
| --- | --- |
| Dirty Fix B / JAL / NULLPTR files byte-identical after PATCH 1 | PROVEN |
| `recomp.h` not required to compile PATCH 1 | PROVEN |
| hint always false; service does not wake AUDIO | PROVEN |
| osSend/osRecv coop path still wakes AUDIO when feature OFF | PROVEN |
| Tooie default binary does not link preemption symbols | PROVEN |
| Env override implemented | REFUTED (deferred) |
| Guest preemption implemented | REFUTED (scaffold only) |
| T02–T12 pass | REFUTED (SKIPPED) |
| Hybrid F' architecture changed | REFUTED |
