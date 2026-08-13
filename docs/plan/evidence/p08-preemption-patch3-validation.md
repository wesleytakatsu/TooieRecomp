---
title: "P08 — preemption PATCH 3 generator safepoint validation"
description: "Hybrid F' PATCH 3: generated safepoints, C ABI status_reg pointer, T11 static/live parity. Flag OFF. No Tooie regen. PATCH4_READY NO."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch3, n64recomp, generator, harness]
---

# P08 — PREEMPTION PATCH 3 VALIDATION

## Meta

```text
MODE:                 IMPLEMENT PATCH 3 ONLY
MODEL:                Grok 4.6 HIGH
ARCHITECTURE:         Hybrid F' (APPROVED)
TOOIE_BASE_HEAD:      89737cac4e84ca76378432bc6b1844935b2d8b88
RUNTIME_BASE_HEAD:    ca568b6ad79b9029d14077f0c3ffa757727c5559
N64RECOMP_BASE_HEAD:  2b6f05688de2abc7d86da5b4a89b84c2c6acbabe
PRODUCTION_PREEMPTION_ACTIVE: NO
TOOIE_REGEN_RUN:      NO
RECOMPILEDFUNCS_CHANGED: NO
PATCH4_READY:         NO
```

DeepSeek Hybrid F remains `CORRECTED/SUPERSEDED`.

---

## Inherited PATCH 2 review (do not erase)

Independent review B ([[p08-preemption-patch2-independent-review]]):

```text
EPOCH_PROTOCOL_VALID:                 YES
LOST_WAKEUP_FOUND:                    NO
PREEMPTION_NEEDS_EVENT_CAUSE:         YES
GENERIC_ANY_UNMASKED_CLASS_VALID:     NO
MTC0_REENABLE_NEEDS_IMMEDIATE_SERVICE: YES
```

```text
PRE_PATCH4_REQUIRED_FIXES:
  - X1 interrupt cause/class gating
  - MTC0 Status re-enable scheduling/service semantics
  - associated strengthening tests (T04 return WEAK, T07 ordering WEAK,
    status initialization APPROXIMATE, TLS epoch model CONDITIONAL)
```

These are **not** PATCH 3 blockers (production flag remains OFF).
They **are** blockers before PATCH 4 enable.

---

## Baseline before writes

```text
TOOIE_HEAD:                 89737cac4e84ca76378432bc6b1844935b2d8b88
TOOIE_WORKTREE:             clean
BASELINE_N64RECOMP_BUILD:   PASS (ninja LiveRecompTest)
PATCH2_HARNESS_BASELINE:    PASS (pass=17 fail=0 skipped=1 T11, exit 0)
```

Protected dirty hashes before writes (must not reset):

```text
threadqueue.cpp           1c61c91985dddfc8919f41afb54aa4ce1c74d5d9e0c4476e51dd4c34a60dbeae
threads.cpp               d12dd1335a79748711e0bf0763f9c638a424fe7b9f0701c3b1a2565970340f99
N64Recomp/src/main.cpp    fb0ae67cfaf17902867d69b7b3b7aa499e4adabf0d7da921ce6a1a5fcd4035dd
N64Recomp/src/recompilation.cpp
                          db8b443142b2513e3a5d00c2b0c453d387d70e95840b47b6de284b556e2eeede
```

After PATCH 3:

```text
threadqueue.cpp           UNCHANGED
threads.cpp               UNCHANGED
N64Recomp/src/main.cpp    UNCHANGED (JAL-only dirty hunk)
N64Recomp/src/recompilation.cpp
                          d2462a3d37006216c44d99e98f0bd415656c745bed9dd9c6a45878553af43684
                          (JAL hunks preserved; PATCH 3 request-site hunks added)
```

```text
DIRTY_FILES_PRESERVED: YES
JAL_POLICY_PRESERVED:  YES
```

---

## ABI

```text
C_ABI_SIGNATURE:
  bool ultramodern_preemption_hint(void);
  void ultramodern_service_preemption(uint8_t* rdram, void* status_reg);

GENERATED_ARGUMENT: (void*)&ctx->status_reg
ABI_BRIDGE_FIXED:   YES
```

The C bridge in `ultramodern/src/preemption.cpp` now casts `void*` to
`uint32_t*` and forwards to `ultramodern::service_preemption`. No
`recomp_context` layout, no `offsetof` in ultramodern, no `recomp.h`
include from ultramodern.

Both `recomp.h` and `ultramodern.hpp` declare the same `void*` signature.

```text
ABI_HEADER_COEXISTENCE: PASS
  g++ -std=c++20 -fsyntax-only with recomp.h + ultramodern.hpp
```

---

## Generator

```text
GENERATOR_INTERFACE:          Generator::emit_preemption_safepoint()
STATIC_GENERATOR_IMPLEMENTED: YES
LIVE_GENERATOR_IMPLEMENTED:   YES
SHARED_REQUEST_SITE:          process_instruction() in src/recompilation.cpp
                                print_goto_with_delay_slot
                                print_branch
                                cpu_jr jump-table arm
BACKWARD_TRANSFER_SAFEPOINT:  YES  (target_vram <= instr_vram)
SAFEPOINT_AFTER_DELAY_SLOT:   YES
JUMP_TABLE_SAFEPOINT:         YES  (always, before emit_switch)
SELF_LOOP_DUPLICATE:          NO   (cpu_j/cpu_b to self: emit_pause_self only)
```

Static C emit:

```c
if (ultramodern_preemption_hint()) { ultramodern_service_preemption(rdram, (void*)&ctx->status_reg); }
```

Live: sljit `icall` hint; AND 1; skip if 0; pass rdram and
`ctx + offsetof(recomp_context, status_reg)`. Null function pointers
are a no-op so existing LiveRecompTest binaries stay safe.

```text
STATIC_CONTEXT_SAFE: CONDITIONAL
  GPRs/FPU/status_reg live in ctx. hi/lo/result/c1cs are stack locals;
  native-thread park preserves them. Delay slot is executed first.
  No extra register flush; C call is an ABI spill.
LIVE_CONTEXT_SAFE:   CONDITIONAL
  GPRs/FPU/status_reg in ctx memory. hi/lo in sljit saved regs (S3/S4).
  Same park + C-call ABI. No extra flush.
```

No instruction-budget safepoints. No uncovered infinite-control-flow
form found beyond backward direct transfer + pre-switch.

---

## Tests

Dirty-tree N64Recomp `PreemptionSafepointTest`:

```text
T11_FORWARD:            PASS
T11_BACKEDGE:           PASS
T11_DELAY_SLOT:         PASS  (0x1234 delay-slot materialization before hint before goto)
T11_SELF_LOOP:          PASS
T11_SWITCH:             PASS
T11_LIVE_PARITY:        PASS  (same service counts; live last_status == &ctx.status_reg)
T11_ABI_STATUS_POINTER: PASS  (static text + live pointer identity)
T11_G flag/hint false:  PASS
T11 PASS (pass=39 fail=0)
```

Dirty-tree harness (`/tmp/p08-p3-harness-build`):

```text
T01–T10 PASS
T11-F-masked PASS    (IE=0 status word: C ABI does not swap)
T11-F-unmasked PASS  (fully unmasked word: C ABI does swap)
T11-G flag OFF PASS
T12 PASS
PATCH2 PASS (pass=21 fail=0 skipped=0)
```

Reconstruction (`/tmp/p08-p3-reconstruction`, versioned artifacts only):

```text
CLEAN_RECONSTRUCTION:     PASS
PATCH_SCRIPT_IDEMPOTENT:  YES
RECON_T11:                PASS (pass=39 fail=0)
RECON_HARNESS:            PASS (pass=21 fail=0 skipped=0)
```

```text
N64RECOMP_BUILD: PASS
RUNTIME_BUILD:   PASS  (ultramodern + harness)
TOOIE_BUILD:     PASS  (TooieRecompiled linked; existing RecompiledFuncs)
```

Production generated C still dated 2026-08-11; grep for
`ultramodern_preemption_hint` in `RecompiledFuncs/**` is empty.

```text
MARKDOWN_VAULT_AUDIT: MCP connected vault is not TooieRecomp/docs
  (path-traversal denied). Local structural check of edited notes:
  no new issues. Pre-existing: VAULT.md has no frontmatter;
  specs/BT/rsp-and-audio.md has two H1s.
```

---

## Artifacts

```text
RUNTIME_CUMULATIVE_PATCH:
  patches/runtime/n64modernruntime-preemption.patch
RUNTIME_PATCH_SHA256:
  93e34a43bbc55f9da563ed0cd195719e055375764fcc52fecf80a66f19d779b9
  (against ca568b6; P1+P2+ABI bridge+T11-F; includes mods.cpp fn-ptr wire)

N64RECOMP_CUMULATIVE_PATCH:
  patches/runtime/n64recomp-tooie.patch
N64RECOMP_PATCH_SHA256:
  da84f056b371f1bf229cae3cabdcead528288a2739aa066653b24803cb0ab712
  (against 2b6f056; same-section JAL + PATCH 3 generator)

OLD_JAL_PATCH_STATUS: INACTIVE
  patches/runtime/n64recomp-same-section-jal.patch kept as history.
  apply_runtime_patches.sh applies only n64recomp-tooie.patch.

PATCH_SCRIPT_UPDATED: YES
```

Apply order: scheduler → cumulative runtime preemption → cumulative
N64Recomp Tooie (JAL + safepoints).

---

## Files changed

```text
N64RECOMP_FILES_CHANGED:
  include/recompiler/generator.h
  include/recompiler/live_recompiler.h
  include/recomp.h
  src/cgenerator.cpp
  src/recompilation.cpp
  LiveRecomp/live_generator.cpp
  LiveRecomp/live_recompiler_test.cpp
  LiveRecomp/preemption_safepoint_test.cpp
  CMakeLists.txt
  src/main.cpp          (pre-existing JAL dirty hunk only; not edited this session)

RUNTIME_FILES_CHANGED:
  ultramodern/src/preemption.cpp
  ultramodern/include/ultramodern/ultramodern.hpp
  ultramodern/test/preemption_harness.cpp

LIBRECOMP_FILES_CHANGED:
  librecomp/src/mods.cpp   (LiveGeneratorInputs hint/service ptrs)

TOOIE_GAME_FILES_CHANGED:      NO
TOOIE_GENERATED_FILES_CHANGED: NO
```

---

## Classification

| claim | class |
| --- | --- |
| Generator emits safepoint after delay slot on backward transfer | PROVEN |
| Forward branch emits no safepoint | PROVEN |
| Self `j`/`b` to self is pause_self only | PROVEN |
| Jump-table path safepoints before switch | PROVEN |
| Static/live service-opportunity parity on toys | PROVEN |
| C ABI consumes `uint32_t* status_reg` | PROVEN |
| Flag OFF: generated/live safepoint does not service | PROVEN |
| Production Tooie generated code contains safepoints | REFUTED |
| Guest preemption active in Tooie | REFUTED |
| PATCH 4 ready | REFUTED |
| X1 cause/class hardware-equivalent | REFUTED (inherited) |

=== END PATCH 3 VALIDATION ===
