---
title: "P08 — session handoff: attract demo e render submission"
description: "Estado autossuficiente: USER_BUG_A WASD resolved; first-straight 510 and first analog transition PASS; first demo fire-egg at glidx 322 characterized as a collision-space miss (CLASS G, Level D); cinematic facing currently not reproducible by the user; T10 current regression REFUTED."
status: evidence
phase: P08
updated: 2026-08-16
tags: [tooie-recomp, p08, handoff, intro, attract, preemption, patch4, bcf28, 80013a7c, visible-demo, transform-hooks, matrix-fix, input, stick-angle]
---

# P08 — session handoff: ATTRACT DEMO & RENDER SUBMISSION

## CURRENT_HEAD

- Branch: `main`.
- Active diagnosis: [[p08-demo-movement-precision-diagnosis]] §16 & §17 (first demo shot miss & target collision volume resolved).
- Active Fix Evidence: [[p08-stick-angle-thunk-fix]].
- Independent Review: [[p08-stick-angle-thunk-fix-rereview]] — **VERDICT A**; prior `$r1` delay-slot blocker resolved.
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
  - Synthetic input sequencer tests startup intro -> file select
    (`chgameselect`, overlay 205) -> slot navigation -> `CONT_A` confirm.
  - Zero frontend input divergence. Post-confirm destination is
    `MAP_A1_CS_INTRO` (161) / `mode=3`, not live Spiral Mountain.
  - Overlay IDs 128/111/148 are `baeggcursor` / `baattach` / `bashoes`
    (old `chspiralmountain` / `chbanjohouse` / `chjinjo` labels
    **CORRECTED/SUPERSEDED**).
  - Later synthetic `mode=1` FAIL is **REFUTED**: the sequencer does not
    inject the later START presses that skip the opening story cutscenes.
- **ORIGINAL 4:3 & TIMING BASELINE VALIDATED (P08-T03 COMPLETE):**
  - Gated live timing counters prove exact 60.000 Hz VI event rate, 20/30 Hz logic rate, 30 Hz skeletal animation rate, decoupled host presentation (~935 Hz), and 4:3 content aspect.
- `RecompiledFuncs` é gerado — não editar.

## CURRENT_BLOCKER / NEXT PHASE WORK

`func_80013B70` stick-angle secondary entry has been corrected
([[p08-stick-angle-thunk-fix-correction]]): native shared tail, strong-symbol bind,
reproducing `$at/$r1 = 0x43340000` delay-slot clobber on all `y == 0` paths.
All unit tests pass (`stick_angle_thunk_test`, `matrix_submission_test`, `input_direction_test`,
`bcf28_helpers_test`, `core1_ra_wrappers_test`).
Focused independent re-review is **ACCEPTED (verdict A)**. Manual WASD
runtime is **RESOLVED**. Full analog/gamepad is **PENDING**.
Gameplay-demo player movement uses `glrecord` recorded pad through bastick
([[p08-demo-movement-precision-diagnosis]]). **Per-logic-tick speed contract
discriminator (DeepSeek V4 Flash HIGH) COMPLETE** (2026-08-15):
`tspd` is a **pre-acceleration target** (`phys+0x3C`). **Post-physics
velocity writer discriminator (Grok 4.6 HIGH) COMPLETE** (§14): there is
**no** post-physics `vx`/`vz` write. Settle at ~510.2 is original slope-scaled ramp;
`FIRST_STRAIGHT_INTERNAL_FIDELITY = PASS`.
**First later analog transition discriminator (Gemini 3.7 Flash HIGH) COMPLETE** (§15):
`glidx = 74..91` (`raw = (-63, 74)`) strictly exercises `func_80013ABC` (table-based
arcsin evaluation, error vs original 0.0000°, error vs atan2 < 0.001°), camera-relative
desired yaw addition, `yaw_update` rotational easing, and `func_8009AE08` slope-scaled
velocity integration. `SELECTED_TRANSITION_INTERNAL_FIDELITY = PASS`. Attract/cinematic facing (USER_BUG_B) is
**CURRENTLY_NOT_REPRODUCIBLE_BY_USER** for the scenes the user checked;
`USER_BUG_B_GLOBAL_PROVEN_RESOLVED=NO`. Do not diagnose another cinematic
actor. **First demo shot miss & target collision volume (Gemini 3.7 Flash HIGH) COMPLETE** (§16, §17):
earliest C-up fire-egg at `glidx=322` has `TRUE_COLLISION_MISS_MARGIN = +36.85f` (3D center dist `187.85f` vs sum of radii `151.00f`; $R_{\text{egg}}=38.00\text{f}, R_{\text{target}}=113.00\text{f}$, target center offset $(0, +44, +4)$ from origin `(-5890.15, -548.61, 1597.84)`).
`SHOULD_SWEEP_INTERSECT_TARGET_GEOMETRICALLY = NO` (PROVEN). World intercept stops egg at `(-5912.92, -348.00, 1500.63)` via terrain triangle `0x8037bf94`.
`USER_PLAYER_SPEED_VISUAL_RESIDUAL: OPEN_SEPARATE`.

Next steps in Phase P08:
1. ~~First straight, first analog transition, first demo shot, target collision volume~~ **COMPLETE**.
2. Do not implement a facing fix. USER_BUG_B cinematic facing is
   currently not reproducible by the user; global is not proven resolved.
3. Analog/gamepad validation of USER_BUG_A.
4. High-FPS transform tagging; window options (`P08-T07`).

## ROOT_CAUSE_STATUS

```text
C2AB8_ROOT_CAUSE_PROVEN:             YES (PATCH 4, CLASS A)
BCF28_ROOT_CAUSE_PROVEN:             YES (Level A; ACCEPTED verdict B)
80013A7C_ROOT_CAUSE_PROVEN:         YES (Level A)
80013A7C_FIX_STATUS:                 RESOLVED (Tooie-native strong symbols)
ACTOR_ROTATION_EXPLAINS_POSE:        NO (Actor.rotation is upright)
RENDER_SUBMISSION_FIX_STATUS:        RESOLVED (Level A; transform_hooks.cpp)
FRONTEND_INPUT_VALIDATION:           PASS
P08_T10_FRONTEND_INPUT:              PASS
DIRECT_SPIRAL_MOUNTAIN_TRANSITION_CLAIM: CORRECTED/SUPERSEDED
CURRENT_T10_REGRESSION:              REFUTED
MATRIX_EQUIVALENCE_TEST:             PASS
WOODED_HOLLOW_MODE5_VALIDATION:      PASS
STICK_ANGLE_13B70_FIX:               ACCEPTED (WASD runtime RESOLVED; analog PENDING)
DEMO_PATH_RESIDUAL:                  LEVEL D — first straight & first analog PASS
  First demo shot (glidx 322 fire-egg) TRUE_COLLISION_MISS_MARGIN = +36.85f (PROVEN NO HIT)
  Target collision center: (-5890.15, -504.61, 1601.84), radius = 113.00f
  World intercept: (-5912.92, -348.00, 1500.63) terrain triangle 0x8037bf94
  USER_PLAYER_SPEED_VISUAL_RESIDUAL: OPEN_SEPARATE
USER_BUG_B:                          CURRENTLY_NOT_REPRODUCIBLE_BY_USER
  GLOBAL_PROVEN_RESOLVED=NO
  Historical chdiggerground actor remains REFUTED
  ([[p08-forced-cinematic-facing-independent-review]]).
P08_T10_SYNTHETIC_FLOW_CURRENT:      REFUTED as a current-HEAD regression;
                                     sequencer never injects later START skips
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

First demo shot miss discriminator is RESOLVED (`TRUE_COLLISION_MISS_MARGIN = +36.85f`, geometric miss proven).
`USER_PLAYER_SPEED_VISUAL_RESIDUAL: OPEN_SEPARATE`.
Analog/gamepad validation of USER_BUG_A (P08-T06).

## P08 FORCED FACING INDEPENDENT REVIEW (2026-08-16)

```text
SELECTED_ACTOR:        Wooded Hollow chdiggerground return
RETURN_STATE:          0x98bb1942
RETURN_ANIMATION:      0xc3aa1065 -> 0xc3ab8000
MOVEMENT_INTENTION:    PROVEN — position integrates backward (294.21°);
                       original case 11 never writes unk54/rotation.y
EXPECTED_FACING:       PRESERVE_PRIOR_YAW (original case-11 MIPS)
NODE_DISCRIMINATOR:    RESOLVED — func_80802760 returns node 0 (B_LIVE_NODE)
                       on all 3438 calls; A_KEEP_OLD (-1) REFUTED (0)
ROOT_CAUSE_LEVEL:      R
USER_BUG_B_THIS_ACTOR: REFUTED
USER_BUG_B_GLOBAL:     OPEN
FIX_IMPLEMENTED:       NO
MARKDOWN_VAULT_AUDIT:  PASS_WITH_PREEXISTING_WARNINGS (errors=0; 50 warnings
                       across 118 files)
```

```text
ORIGINAL_CASE11_WRITES_YAW:       NO
ORIGINAL_CASE11_WRITES_UNK54:     NO
GENERATED_CASE11_MATCHES_ORIGINAL: YES
DOES_ORIGINAL_CASE11_ADVANCE_POSITION_ALONG_RETURN_DIRECTION: YES
DOES_ORIGINAL_CASE11_UPDATE_FACING_TO_RETURN_DIRECTION:       NO
ORIGINAL_REVERSE_WALKS_BACKWARD:  YES
ORIGINAL_RETURN_STATE_DISPATCH_MATCHES_RECOMP: YES
DEEPSEEK_GENERATED_MATCH_CLAIM:  CORRECTED/SUPERSEDED
FUNC_80802760_GENERATED_MATCH_ORIGINAL: UNKNOWN
FIRST_OBSERVED_MECHANISM:         case-11 return movement with preserved yaw
ROOT_CAUSE:                       original actor/state contract; no recomp defect
USER_BUG_B_GLOBAL:                OPEN
NEXT_SINGLE_ACTION:               select a different user-confirmed actor
```

Evidence: [[p08-forced-cinematic-facing-independent-review]].

```text
REVIEW_CHECKPOINT_COMMIT: 902348f
REVIEW_CHECKPOINT_PUSHED: YES
DEEPSEEK_DISCRIMINATOR_COMMIT: (this checkpoint; docs-only)
DEEPSEEK_DISCRIMINATOR_PUSHED: YES
```

## FILES_TO_READ_NEXT

0. [[p08-demo-movement-precision-diagnosis]] — **CURRENT** first demo shot miss §16
0. [[p08-forced-cinematic-facing-diagnosis]] — USER_BUG_B historical; currently not reproducible
0a. [[p08-stick-angle-thunk-fix]] — stick-angle secondary-entry fix
0a. [[p08-stick-angle-thunk-fix-rereview]] — verdict A; blocker resolved
0aa. [[p08-stick-angle-thunk-fix-independent-review]] — historical verdict C
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
CODE_CHANGED             = NO (historical diagnostic probe reverted)
RUNTIME_CODE_CHANGED     = NO (N64ModernRuntime untouched)
GENERATOR_CODE_CHANGED   = NO (N64Recomp untouched)
PATCH_CODE_CHANGED       = NO
RECOMPILEDFUNCS          = NOT EDITED (untouched)
```
