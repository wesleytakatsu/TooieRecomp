---
title: "P08 — session handoff: intro para attract demo"
description: "Estado autossuficiente: C2AB8, BCF28 e core1 RA wrappers resolvidos; visible attract/gameplay demo reproduzido; first fidelity gap is RT64 LOAD-path viewproj decompose."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, handoff, intro, attract, preemption, patch4, bcf28, 80013a7c, visible-demo, movement]
---

# P08 — session handoff: INTRO → ATTRACT DEMO

## CURRENT_HEAD

- Branch: `main`.
- Render-submission diagnosis is based on
  `fe2683ebc70309191ef339efd4881d61f8471a76`.
- Current evidence: [[p08-render-submission-matrix-diagnosis]].
- Discriminator: [[p08-actor-camera-transform-discriminator]].
- Movement diagnosis: [[p08-visible-demo-movement-diagnosis]].
- RA-wrapper fix: [[p08-core1-ra-wrapper-fix]].

## CURRENT_PHASE

P08 — RT64, input e frontend. PATCH 4 is accepted and default ON. The
`func_800BCF28` binding fix is **ACCEPTED (verdict B)**. The
`func_80013A7C` lookup-zero is **RESOLVED**. **Visible attract / gameplay
demo is reproduced.** First fidelity gap is **RENDER_SUBMISSION / RT64
LOAD-path viewproj decompose (Level B)**: Tooie LOADs `T_S * V_rot * P`
onto `G_MTX_PROJECTION`; RT64 splits it with `P[2][3]=-1` while Tooie P
is scaled by 0.5. Clip product is preserved; camera/view Z extraction is
wrong. Banjo avoids this path with incremental `LOAD P` + `MUL` view.
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
- **RENDER-SUBMISSION MATRIX (2026-08-14):** RT64 `matrixDecomposeViewProj`
  on Tooie's combined scaled VP. Product preserved; extracted view Z
  wrong. See [[p08-render-submission-matrix-diagnosis]].
- `RecompiledFuncs` é gerado — não editar.

## CURRENT_BLOCKER

Visible demo exists. First fidelity divergence is **RENDER_SUBMISSION /
RT64 LOAD-path viewproj decompose** (Level B). Banjo `D_80135490[0]` and
Jinjo `Actor.rotation` plus CPU model matrices remain upright. Tooie
`func_800E44FC` LOADs `T_S * V_rot * P`; RT64 extracts a half-length view
Z. Permanent hook not implemented.

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
FINAL_VERTEX_TRANSFORM_WRONG:        NO
CAMERA_EXTRACTION_WRONG:             YES
TRANSFORM_CLASSIFICATION_WRONG:      YES
```

## CURRENT_FIRST_DIVERGENCE

Visible Wooded Hollow attract: Banjo and Jinjo simulation state and CPU model
matrices are upright and correct. RT64 LOAD-path decompose of Tooie's
combined scaled viewproj extracts a wrong view Z / camera Z (product
preserved). See [[p08-render-submission-matrix-diagnosis]].

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

1. Live Wooded Hollow `func_800CB124` fovy/near/scale (form proven; exact
   parameters inferred in the offline probe).
2. Whether `func_800E4640` re-LOADs the combined VP on the first Wooded
   Hollow frame (definition proven; call-graph UNKNOWN).
3. Timer IM7 / PI completion remain approved approximations (deferred).

## NEXT_SINGLE_ACTION

Implement only the proven render-submission transform fix: split
`func_800E44FC` (and the `D_8012D558` reload) into `LOAD P` + `MUL` view
so RT64 takes the Banjo incremental path. Do not port unrelated Banjo
tagging. Do not modify shared RT64.

## FILES_TO_READ_NEXT

0. [[p08-render-submission-matrix-diagnosis]] — **CURRENT:** render matrix diagnosis
1. [[p08-actor-camera-transform-discriminator]] — actor vs camera discriminator
2. [[p08-visible-demo-movement-diagnosis]] — visible demo + movement diagnosis
3. [[p08-core1-ra-wrapper-fix]] — RA-wrapper fix (resolved; do not reopen)
4. [[p08-80013a7c-lookup-zero-diagnosis]] — Level A diagnosis (resolved)
5. [[p08-bcf28-binding-fix-independent-review]] — verdict B; BCF28 fix accepted
6. `docs/HANDOFF.md`, `docs/plans/BT/08-rt64-input-and-frontend.md`

## CHECKPOINT SCOPE

```text
CODE_CHANGED             = NO (docs + offline probe script only; no gameplay fix)
RUNTIME_CODE_CHANGED     = NO
GENERATOR_CODE_CHANGED   = NO
PATCH_CODE_CHANGED       = NO
RECOMPILEDFUNCS          = NOT EDITED (untouched)
```
