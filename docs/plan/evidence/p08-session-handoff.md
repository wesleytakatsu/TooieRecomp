---
title: "P08 — session handoff: attract demo e render submission"
description: "Estado autossuficiente: func_80013B70 implementation rejected by independent semantic review for a narrow $r1 side-effect mismatch; P08-T10 synthetic gameplay gate still FAIL; attract/cinematic facing separate."
status: evidence
phase: P08
updated: 2026-08-15
tags: [tooie-recomp, p08, handoff, intro, attract, preemption, patch4, bcf28, 80013a7c, visible-demo, transform-hooks, matrix-fix, input, stick-angle]
---

# P08 — session handoff: ATTRACT DEMO & RENDER SUBMISSION

## CURRENT_HEAD

- Branch: `main`.
- Active Fix Evidence: [[p08-stick-angle-thunk-fix]].
- Independent Review: [[p08-stick-angle-thunk-fix-independent-review]] — **VERDICT C**; missing `$r1` delay-slot side effect.
- Stick-angle diagnosis: [[p08-player-direction-facing-diagnosis]].
- Title & File Select Validation: [[p08-title-file-select-validation]].
- Render-submission fix: [[p08-render-submission-matrix-fix]].
- Directional input diagnosis: [[p08-directional-input-movement-diagnosis]].
- Diagnosis: [[p08-render-submission-matrix-diagnosis]].
- Discriminator: [[p08-actor-camera-transform-discriminator]].
- Movement diagnosis: [[p08-visible-demo-movement-diagnosis]].
- RA-wrapper fix: [[p08-core1-ra-wrapper-fix]].

## CURRENT_PHASE

P08 — RT64, input e frontend. PATCH 4 is accepted and default ON. The
`func_800BCF28` binding fix is **ACCEPTED (verdict B)**. The
`func_80013A7C` lookup-zero is **RESOLVED**. **Visible attract / gameplay
demo is reproduced and running.** The **RENDER_SUBMISSION matrix split** is
**RESOLVED (Level A)** via `src/game/transform_hooks.cpp` (`func_800E44FC`,
`func_800E4640`, `func_800E4628` overrides). **Title Screen / File Selection
Input (P08-T10)** is **VALIDATED and COMPLETE** ([[p08-title-file-select-validation]]).
**Original 4:3 / Timing Baseline (P08-T03)** is **VALIDATED and COMPLETE**
([[p08-original-timing-4x3-baseline]]): 60.0 Hz guest VI, 20/30 Hz logic, 30 Hz animation,
host decouple (~935 Hz), and 4:3 content aspect confirmed under clean baseline config.

## CURRENT_PRODUCT_STATE

- **`func_800BCF28` SIGSEGV RESOLVED and independently accepted (verdict B)**: Tooie-native helpers bound at `0x800BCCC0` and `0x800BCCD8`.
- Intro visual completa (céu, logo, Jiggy).
- Attract request correto; type-4 completa; warp consumido; mode-5 alcançado.
- `func_800C2AB8` is behaviorally resolved with PATCH4 default ON.
- **`func_80013A7C` lookup-zero RESOLVED** (Tooie-native RA wrappers; [[p08-core1-ra-wrapper-fix]]).
- **VISIBLE GAMEPLAY DEMO & RENDER SUBMISSION RESOLVED (2026-08-14):**
  - Incremental projection split implemented in `src/game/transform_hooks.cpp`.
  - RT64 avoids the erroneous unscaled `matrixDecomposeViewProj` path.
  - Characters (`chjinjo` and `Banjo`) and camera are upright and elevated.
  - Banjo walks smoothly along ground path throughout 75s Wooded Hollow demo.
  - Host matrix regression test `matrix_submission_test` PASS (`MAX_MATRIX_ERROR: 0.000000e+00`).
  - Unit tests `bcf28_helpers_test` and `core1_ra_wrappers_test` PASS.
- **TITLE SCREEN / FILE-SELECTION VALIDATED (P08-T10 COMPLETE):**
  - Synthetic input sequencer tested the entire flow: startup intro -> file select (`chgameselect`, overlay 205) -> slot navigation -> confirmation -> Spiral Mountain map load.
  - Zero input divergence found. Game transitions cleanly to gameplay.
- **ORIGINAL 4:3 & TIMING BASELINE VALIDATED (P08-T03 COMPLETE):**
  - Gated live timing counters prove exact 60.000 Hz VI event rate, 20/30 Hz logic rate, 30 Hz skeletal animation rate, decoupled host presentation (~935 Hz), and 4:3 content aspect.
- `RecompiledFuncs` é gerado — não editar.

## CURRENT_BLOCKER / NEXT PHASE WORK

`func_80013B70` stick-angle secondary entry has an implementation
([[p08-stick-angle-thunk-fix]]): native shared tail, strong-symbol bind,
cardinal/diagonal/third-arg tests PASS. Independent review rejected acceptance
with **VERDICT C** because the `y==0, x>=0` path omits the original `$r1`
delay-slot clobber. Live player motion is **NOT_RESOLVED** because the P08-T10 synthetic file-select
→ mode=1 transition still **FAIL**s (CONT_A confirm ends in mode=3).
That regression is a **separate next blocker**. Attract/cinematic facing
(USER_BUG_B) is **OPEN_SEPARATE** — attract bypasses bastick.

Next steps in Phase P08:
1. Correct only the missing `$r1` side effect identified by the independent review.
2. Rerun the focused stick-angle and regression tests/build.
3. Then: USER_BUG_B if still open; then transform-tagging; then `P08-T07`.

## ROOT_CAUSE_STATUS

```text
C2AB8_ROOT_CAUSE_PROVEN:             YES (PATCH 4, CLASS A)
BCF28_ROOT_CAUSE_PROVEN:             YES (Level A; ACCEPTED verdict B)
80013A7C_ROOT_CAUSE_PROVEN:         YES (Level A)
80013A7C_FIX_STATUS:                 RESOLVED (Tooie-native strong symbols)
ACTOR_ROTATION_EXPLAINS_POSE:        NO (Actor.rotation is upright)
RENDER_SUBMISSION_FIX_STATUS:        RESOLVED (Level A; transform_hooks.cpp)
FRONTEND_INPUT_VALIDATION:           PASS historically; current synthetic mode=1 FAIL
MATRIX_EQUIVALENCE_TEST:             PASS
WOODED_HOLLOW_MODE5_VALIDATION:      PASS
STICK_ANGLE_13B70_FIX:               IMPLEMENTATION_REJECTED (review verdict C; `$r1` correction pending)
P08_T10_SYNTHETIC_FLOW_CURRENT:      FAIL (separate)
```

## RESOLVED_DO_NOT_REOPEN

1. Overlay data relocation / `LOOKUP_FUNC(0)`.
2. Actor manager null from artificial spawn.
3. `func800D6E54` as population pass.
4. Resident overlay / gcsection stale mapping.
5. Scheduler queue insert/remove / `No threads left` (without new proof).
6. IPL3 integrity gate.
7. R_MIPS_26 same-section / `do_dma` SIGSEGV.
8. Type-4 incomplete / warp gated as clean-freeze blocker.
9. Warp not consumed as clean-freeze blocker.
10. `func_80026500` list walk as current hang PC.
11. `func_800A907C -> func_8001EE34` as object driver — REFUTED.
12. "table[2] object NÃO está agendado" — REFUTED.
13. C2AB8 cooperative freeze as current blocker — RESOLVED by PATCH 4.
14. Hybrid F' architecture / PATCH 1 / 2 / 3 / 3.5 internals.
15. `func_800BCF28` SIGSEGV / `0x800BCCC0` lookup_gap_stub — RESOLVED and accepted.
16. `func_80013A7C` LOOKUP_FUNC(0) / core1 RA wrappers — RESOLVED.
17. Actor.rotation as root cause of supine pose — REFUTED (Actor.rotation is upright).
18. Tooie render submission matrix split — RESOLVED.
19. Title screen / file-selection input flow — VALIDATED (No divergence).
20. P08-T03 original timing/4:3 baseline — BLOCKED; no claim of baseline
    correctness until the clean live measurements exist.

Do not reopen these without new evidence.

## NEXT_SINGLE_ACTION

Correct only the missing `$r1` delay-slot side effect identified by
[[p08-stick-angle-thunk-fix-independent-review]]; then rerun the focused
tests/build. Keep the synthetic file-select regression separate and do not
use it as a gameplay automation
gate.

## FILES_TO_READ_NEXT

0. [[p08-stick-angle-thunk-fix]] — stick-angle secondary-entry fix
0a. [[p08-stick-angle-thunk-fix-independent-review]] — verdict C; `$r1` side-effect blocker
0b. [[p08-player-direction-facing-diagnosis]] — Level A diagnosis
0c. [[p08-directional-input-movement-diagnosis]] — host→pad (not reopened)
1. [[p08-render-submission-matrix-fix]] — render matrix submission fix
2. [[p08-render-submission-matrix-diagnosis]] — render matrix diagnosis
3. [[p08-actor-camera-transform-discriminator]] — actor vs camera discriminator
4. [[p08-visible-demo-movement-diagnosis]] — visible demo + movement diagnosis
5. [[p08-core1-ra-wrapper-fix]] — RA-wrapper fix (resolved; do not reopen)
6. `docs/HANDOFF.md`, `docs/plans/BT/08-rt64-input-and-frontend.md`

## CHECKPOINT SCOPE

```text
CODE_CHANGED             = YES (boot_hw_stubs func_80013B70 + stick_angle_thunk_test)
RUNTIME_CODE_CHANGED     = NO (N64ModernRuntime untouched)
GENERATOR_CODE_CHANGED   = NO (N64Recomp untouched)
PATCH_CODE_CHANGED       = NO
RECOMPILEDFUNCS          = NOT EDITED (untouched)
```
