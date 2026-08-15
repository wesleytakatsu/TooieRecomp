---
title: "P08 — session handoff: attract demo e render submission"
description: "Estado autossuficiente: C2AB8, BCF28, core1 RA wrappers e render-submission matrix fix resolvidos; visible attract demo validado; right-only input report not reproduced on e97."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, handoff, intro, attract, preemption, patch4, bcf28, 80013a7c, visible-demo, transform-hooks, matrix-fix, input]
---

# P08 — session handoff: ATTRACT DEMO & RENDER SUBMISSION

## CURRENT_HEAD

- Branch: `main`.
- Active Fix Evidence: [[p08-render-submission-matrix-fix]].
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
`func_800E4640`, `func_800E4628` overrides).

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
- `RecompiledFuncs` é gerado — não editar.

## CURRENT_BLOCKER / NEXT PHASE WORK

Visual Wooded Hollow attract demo renders correctly and advances.
The user "right-only movement" report was **not reproduced** on e97: host→N64
pad conversion is bidirectional (`input_direction_test` PASS; runtime LEFT
`pad=(-82,0)`). No playable file-select session was obtained autonomously.
Do not promote that report above title/file-selection unless a later
user-controlled session contradicts the conversion evidence
([[p08-directional-input-movement-diagnosis]]).
Next steps in Phase P08 / downstream phases:
1. Title screen / frontend menu / file selection input integration.
2. Full transform-tagging suite (`gEXEnable`, matrix group IDs for high-FPS motion interpolation).
3. Investigate remaining downstream gameplay scenes (e.g. later Y=-2800 scenes).

## ROOT_CAUSE_STATUS

```text
C2AB8_ROOT_CAUSE_PROVEN:             YES (PATCH 4, CLASS A)
BCF28_ROOT_CAUSE_PROVEN:             YES (Level A; ACCEPTED verdict B)
80013A7C_ROOT_CAUSE_PROVEN:         YES (Level A)
80013A7C_FIX_STATUS:                 RESOLVED (Tooie-native strong symbols)
ACTOR_ROTATION_EXPLAINS_POSE:        NO (Actor.rotation is upright)
RENDER_SUBMISSION_FIX_STATUS:        RESOLVED (Level A; transform_hooks.cpp)
MATRIX_EQUIVALENCE_TEST:             PASS
WOODED_HOLLOW_MODE5_VALIDATION:      PASS
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

Do not reopen these without new evidence.

## NEXT_SINGLE_ACTION

Address title screen input and frontend file-selection navigation in Phase P08.

## FILES_TO_READ_NEXT

0. [[p08-directional-input-movement-diagnosis]] — directional input (not reproduced on e97)
1. [[p08-render-submission-matrix-fix]] — render matrix submission fix
2. [[p08-render-submission-matrix-diagnosis]] — render matrix diagnosis
3. [[p08-actor-camera-transform-discriminator]] — actor vs camera discriminator
4. [[p08-visible-demo-movement-diagnosis]] — visible demo + movement diagnosis
5. [[p08-core1-ra-wrapper-fix]] — RA-wrapper fix (resolved; do not reopen)
6. `docs/HANDOFF.md`, `docs/plans/BT/08-rt64-input-and-frontend.md`

## CHECKPOINT SCOPE

```text
CODE_CHANGED             = YES (tooie_diag + get_n64_input wrap + osContGetReadData wrap
                               + input_direction_test; transform_hooks already on HEAD)
RUNTIME_CODE_CHANGED     = NO (N64ModernRuntime untouched)
GENERATOR_CODE_CHANGED   = NO (N64Recomp untouched)
PATCH_CODE_CHANGED       = NO
RECOMPILEDFUNCS          = NOT EDITED (untouched)
```
