---
title: "P08 — session handoff: intro para attract demo"
description: "Estado autossuficiente: C2AB8, BCF28 e core1 RA wrappers resolvidos; visible attract/gameplay demo reproduzido; first fidelity gap is character/camera transform."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, handoff, intro, attract, preemption, patch4, bcf28, 80013a7c, visible-demo, movement]
---

# P08 — session handoff: INTRO → ATTRACT DEMO

## CURRENT_HEAD

- Branch: `main`.
- Visible-demo / movement diagnosis is based on
  `30b3f0619d94a18f2c748442a7f84983d0310e75` plus gated
  `TOOIE_DIAG_MOVEMENT` instrumentation.
- Discriminator evidence: [[p08-actor-camera-transform-discriminator]].
- Movement diagnosis: [[p08-visible-demo-movement-diagnosis]].
- RA-wrapper fix: [[p08-core1-ra-wrapper-fix]].

## CURRENT_PHASE

P08 — RT64, input e frontend. PATCH 4 is accepted and default ON. The
`func_800BCF28` binding fix is **ACCEPTED (verdict B)**. The
`func_80013A7C` lookup-zero is **RESOLVED**. **Visible attract / gameplay
demo is reproduced.** First fidelity gap is character/camera transform
discrimination (**Level B**): simulation state and CPU model matrix are
proven upright and correct; divergence is isolated to `RENDER_SUBMISSION`
(Rare `G_MTX_PROJECTION` view-in-projection vs host renderer).
Title/file-select input continua diferido.

## CURRENT_PRODUCT_STATE

- **`func_800BCF28` SIGSEGV RESOLVED and independently accepted (verdict B)**
  (2026-08-14): Tooie-native helpers bound at `0x800BCCC0` and `0x800BCCD8`.
- Intro visual completa (céu, logo, Jiggy).
- Attract request correto; type-4 completa; warp consumido; mode-5 alcançado.
- Hybrid F' startup behavior is independently **CLASS A** in the local
  regenerated binary; rollback OFF reproduces the old freeze.
- `func_800C2AB8` is behaviorally resolved with PATCH4 default ON.
- Clean-build correction tracks the empty `RecompiledPatches` scaffolds.
- **`func_80013A7C` lookup-zero RESOLVED** (Tooie-native RA wrappers;
  [[p08-core1-ra-wrapper-fix]]).
- **VISIBLE GAMEPLAY DEMO (2026-08-14):** intro → Jiggy → C2AB8 → BCF28 →
  RA wrappers → mode-5 Wooded Hollow (`map 335` / entrance 8). See
  [[p08-visible-demo-movement-diagnosis]].
- **ACTOR VS CAMERA DISCRIMINATION (2026-08-14):**
  `Actor.rotation` (`chjinjo` pitch=0, roll=0, yaw=41°; Banjo pitch=0, roll=0, yaw=276->148°)
  and CPU model matrix are upright and correct. Divergence isolated to `RENDER_SUBMISSION`.
  See [[p08-actor-camera-transform-discriminator]].
- `RecompiledFuncs` é gerado — não editar.

## CURRENT_BLOCKER

Visible demo exists. First fidelity divergence is **RENDER_SUBMISSION / RT64
matrix decomposition** (Level B). Banjo `D_80135490[0]` and Jinjo `Actor.rotation`
simulation state and CPU model matrices are upright and correct. Divergence is
in the render submission layer where Rare's `G_MTX_PROJECTION` view-in-projection
stack is interpreted by the host renderer.

## PROVEN_ROOT_MECHANISM (old C2AB8)

```text
YES — synchronous dependency cycle / busy-wait, starved because MAIN
held the host execution token in a pure MIPS loop:
  table[2].word0 != 0
  -> C269C(7)==0
  -> C2AB8 outer loop infinite
  -> AUDIO cannot run -> 205D0 never clears table[2]
```

## ROOT_CAUSE_STATUS

```text
C2AB8_ROOT_CAUSE_PROVEN:             YES (PATCH 4, CLASS A)
BCF28_ROOT_CAUSE_PROVEN:             YES (Level A; ACCEPTED verdict B)
80013A7C_ROOT_CAUSE_PROVEN:         YES (Level A)
80013A7C_FIX_STATUS:                 RESOLVED (Tooie-native strong symbols)
ACTOR_ROTATION_EXPLAINS_POSE:        NO (Actor.rotation is upright)
TRANSFORM_STAGE_IDENTIFIED:          YES (RENDER_SUBMISSION / Level B)
```

## CURRENT_FIRST_DIVERGENCE

Visible Wooded Hollow attract: Banjo and Jinjo simulation state and CPU model
matrices are upright and correct. Render submission / RT64 view-in-projection
decomposition causes the visual supine / clipping artifact.

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
11. **`func_800A907C -> func_8001EE34` as object driver** — REFUTED.
12. **"table[2] object NÃO está agendado"** — REFUTED.
13. **C2AB8 cooperative freeze as current blocker** — RESOLVED by PATCH 4.
14. Hybrid F' architecture / PATCH 1 / 2 / 3 / 3.5 internals.
15. **`func_800BCF28` SIGSEGV / `0x800BCCC0` lookup_gap_stub** — RESOLVED and accepted.
16. **`func_80013A7C` LOOKUP_FUNC(0) / core1 RA wrappers** — RESOLVED.
17. **Actor.rotation as root cause of supine pose** — REFUTED (Actor.rotation is upright).

Do not reopen these without new evidence.

## OPEN_QUESTIONS

1. RT64 handling of Rareware `G_MTX_PROJECTION` view-in-projection matrices.
2. Transform tagging / matrix interception patch parity with BanjoRecomp.
3. Timer IM7 / PI completion remain approved approximations (deferred).

## NEXT_SINGLE_ACTION

Inspect RT64 matrix upload / view-in-projection interception in the render
submission layer (or compare against BanjoRecomp's transform tagging patch).

## FILES_TO_READ_NEXT

0. [[p08-actor-camera-transform-discriminator]] — **CURRENT:** actor vs camera transform discriminator
1. [[p08-visible-demo-movement-diagnosis]] — visible demo + movement diagnosis
2. [[p08-core1-ra-wrapper-fix]] — RA-wrapper fix (resolved; do not reopen)
3. [[p08-80013a7c-lookup-zero-diagnosis]] — Level A diagnosis (resolved)
4. [[p08-bcf28-binding-fix-independent-review]] — verdict B; BCF28 fix accepted
5. `docs/HANDOFF.md`, `docs/plans/BT/08-rt64-input-and-frontend.md`

## CHECKPOINT SCOPE

```text
CODE_CHANGED             = YES (gated TOOIE_DIAG_MOVEMENT probe only; no gameplay fix)
RUNTIME_CODE_CHANGED     = NO
GENERATOR_CODE_CHANGED   = NO
PATCH_CODE_CHANGED       = NO
RECOMPILEDFUNCS          = NOT EDITED (untouched)
```
