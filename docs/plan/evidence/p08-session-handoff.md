---
title: "P08 — session handoff: attract demo e render submission"
description: "Estado autossuficiente: DEMO2 attract-loop SIGSEGV (bsrest 0x808003FC → baflameline) is RESOLVED and VALIDATED via N64Recomp CreateStatic ordering fix. Fire-egg miss, cinematic facing, and T10 results are preserved."
status: evidence
phase: P08
updated: 2026-08-16
tags: [tooie-recomp, p08, handoff, intro, attract, demo2, sigsegv, bsrest, baflameline, preemption, patch4, bcf28, 80013a7c, visible-demo, transform-hooks, matrix-fix, input, stick-angle]
---

# P08 — session handoff: ATTRACT DEMO & RENDER SUBMISSION

## CURRENT_HEAD

- Branch: `main`.
- Active Fix Evidence: [[p08-attract-loop-after-demo2-crash]] (**RESOLVED and VALIDATED**).
- Preserved diagnosis: [[p08-demo-movement-precision-diagnosis]] §16 & §17 (first demo shot miss & target collision volume resolved).
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
**Attract DEMO2 SIGSEGV (bsrest 0x808003FC)** is **RESOLVED and VALIDATED**
([[p08-attract-loop-after-demo2-crash]]).

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
- **ATTRACT DEMO2 CRASH RESOLVED (2026-08-16):**
  - Unnamed same-section overlay targets resolved via N64Recomp CreateStatic ordering fix.
  - 17 risky lookups -> 0; all 7 static bodies generated and verified.
  - 300s autonomous soak passed all transitions through CCL 314, CS Startup Intro 395, and beyond.
- `RecompiledFuncs` é gerado — não editar.

## CURRENT_STATUS / NEXT PHASE WORK

**RESOLVED:** attract loop dies after DEMO2. `INTRO → 335 → 278 → 314 → 395 → INTRO → 235 → 296` succeed;
`bsrest_entrypoint_11` calls `static_344_808003FC` directly and completes player init cleanly.
See [[p08-attract-loop-after-demo2-crash]].

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
1. Independent review of [[p08-attract-loop-after-demo2-crash]]; no
   production fix in the diagnosis checkpoint.
2. ~~First straight, first analog transition, first demo shot, target collision volume~~ **COMPLETE**.
3. Do not implement a facing fix. USER_BUG_B cinematic facing is
   currently not reproducible by the user; global is not proven resolved.
4. Analog/gamepad validation of USER_BUG_A.
5. High-FPS transform tagging; window options (`P08-T07`).

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
ATTRACT_DEMO2_LOOP_CRASH:            PROVEN Level A
  bsrest_entrypoint_11 LOOKUP_FUNC(0x808003FC) = baflameline_entrypoint_5
  T1 335 and T2 278 succeed; T3 314 init SIGSEGV EXIT=139
```

## Independent acceptance review — 2026-08-16 (GPT-5.6 Luna, X-HIGH)

The fix is accepted with a non-blocking runtime-environment caveat. The exact
expected repository head was present, the durable patch base/hash contract was
verified, and a clean regeneration reproduced the intended output. The policy
unit test executed all four requested JAL cases (`T11 PASS`, `pass=75 fail=0`).

```text
BASE_HEAD:                                  1f2aba14665c064ce5af829955f1ac089792de77
PATCH_BASE_CORRECT:                         YES
PATCH_HASH_CORRECT:                         YES
PATCH_APPLICATION_CONTRACT:                 PASS
UNNAMED_SAME_SECTION_CREATESTATIC:          PASS
CROSS_SECTION_POLICY_PRESERVED:             YES
NON_RELOCATABLE_POLICY_PRESERVED:           YES
HOST_HOOK_POLICY_PRESERVED:                 YES
CREATESTATIC_BOUNDARIES:                    7/7 PASS
BSREST_808003FC_VERDICT:                    A
STATIC_HELPER_SPOTCHECK_1:                  8080020C — PASS (72/72, branches/JALs bounded)
STATIC_HELPER_SPOTCHECK_2:                  8080032C — PASS (52/52, branches/JALs bounded)
TOTAL_AUDITED_CALLS:                        17
DIRECT_CORRECT_STATIC:                      17
WRONG_STATIC:                               0
CALL_MISSING:                               0
LOOKUP_REMAINS:                             0
STATIC_TEST_STRENGTH:                       B
STATIC_TEST_FALSE_PASS_RISK:                LOW
TEST_STRENGTHENED:                          YES (test-only)
GENERATOR_POLICY_TEST_EXECUTED:              YES
GENERATOR_POLICY_CASES:                      4/4
CUMULATIVE_PATCH_PRIOR_CONTENT_PRESERVED:    YES
UNRELATED_GENERATOR_SEMANTICS_CHANGED:      NO
REGENERATED_LOCAL_BODIES:                   7/7
REGENERATED_DIRECT_CALLS:                   17/17
REGENERATED_VULNERABLE_LOOKUPS:             0
ATTRACT_STATIC_TEST:                         PASS
STICK_ANGLE_TEST:                            PASS
MATRIX_SUBMISSION_TEST:                     PASS
INPUT_DIRECTION_TEST:                       PASS
BCF28_TEST:                                  PASS
CORE1_RA_TEST:                               PASS
TOOIE_BUILD:                                 PASS
```

The short independent LVP/Xvfb run launched and reached `T_INTRO`, with no
SIGSEGV, but timed out before CCL; the Radeon/Xvfb path lacked DRI3 presentation.
The earlier 300-second Vulkan soak remains implementation evidence. For this
independent run: `CCL_REACHED=NO`, `BSREST_EP11_REACHED=NO`,
`CORRECT_LOCAL_TARGET=UNKNOWN`, `BAFLAMELINE_WRONG_BIND=UNKNOWN`,
`INTRO_395_REACHED=NO`, `EXIT_139=NO`.

```text
FIX_VERDICT:                                B
FIX_ACCEPTED:                               YES
ATTRACT_DEMO2_CRASH:                        RESOLVED
MARKDOWN_VAULT_AUDIT:                       PASS (0 errors; pre-existing warnings/tasks remain)
NEXT_SINGLE_ACTION:                         return to the next P08 residual
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

Independent review of [[p08-attract-loop-after-demo2-crash]] (Level A).
Do not implement a null-guard or skip-rest hack.

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
PREVIOUS_NEXT_SINGLE_ACTION:      select a different user-confirmed actor
                                  (CORRECTED/SUPERSEDED by the unnamed-JAL review)
```

Evidence: [[p08-forced-cinematic-facing-independent-review]].

```text
REVIEW_CHECKPOINT_COMMIT: 902348f
REVIEW_CHECKPOINT_PUSHED: YES
DEEPSEEK_DISCRIMINATOR_COMMIT: (this checkpoint; docs-only)
DEEPSEEK_DISCRIMINATOR_PUSHED: YES
```

## FILES_TO_READ_NEXT

0. [[p08-attract-loop-after-demo2-crash]] — **CURRENT BLOCKER** DEMO2 attract SIGSEGV
0. [[p08-demo-movement-precision-diagnosis]] — first demo shot miss §16 (preserved)
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
CODE_CHANGED             = YES (apply_runtime_patches.sh hash, attract static test)
RUNTIME_CODE_CHANGED     = NO (N64ModernRuntime core untouched)
GENERATOR_CODE_CHANGED   = YES (N64Recomp recompilation.cpp CreateStatic ordering fix via patch)
PATCH_CODE_CHANGED       = YES (patches/runtime/n64recomp-tooie.patch)
RECOMPILEDFUNCS          = REGENERATED CLEANLY (not hand-edited)
```

## P08 unnamed overlay JAL fix & validation (2026-08-16, Gemini 3.7 Flash HIGH)

`bsrest_entrypoint_11` at `0x80800668` JALs to unnamed `.bsrest` `0x808003FC`.
The root cause was Level A: `resolve_jal` previously fell back to
`use_lookup_for_all_function_calls` (Ambiguous -> `LOOKUP_FUNC`), binding to
`baflameline_entrypoint_5` due to shared overlay VRAM.

**Fix Implemented:** In N64Recomp `resolve_jal`, for relocatable sections, if
`in_current_section` is true and no existing function symbol matched in this
section, `JalResolutionResult::CreateStatic` is returned before the lookup-all
fallback. All 7 audited `.bsrest` unnamed targets (`0x80800000`, `0x8080003C`,
`0x80800080`, `0x808000E4`, `0x8080020C`, `0x8080032C`, `0x808003FC`) now
generate local static bodies in `funcs_416.c`, and all 17 vulnerable same-section
JAL sites call them directly without `LOOKUP_FUNC`.

**Validation:**
- `tests/attract_demo2_transition_jal_test.py`: **PASS** (17 lookups -> 0, 7/7 bodies present, semantics match original MIPS).
- Generator unit test in `PreemptionSafepointTest` (`test_jal_resolution_policy`): **PASS** (75/75 assertions).
- All native unit tests (`stick_angle_thunk_test`, `matrix_submission_test`, `input_direction_test`, `bcf28_helpers_test`, `core1_ra_wrappers_test`): **PASS**.
- Full 300s autonomous attract soak with Vulkan: **PASS** (completed transitions `INTRO → 335 Wooded Hollow → 278 TDL → 314 CCL → 395 CS Intro → INTRO → 235 Mayahem → 296 JRL`, `EXIT_139=NO`, `UNEXPECTED_SIGSEGV=NO`).

```text
ROOT_CAUSE_LEVEL: A
FIX_OWNER: N64RECOMP
CREATESTATIC_ORDERING_FIX: IMPLEMENTED in patches/runtime/n64recomp-tooie.patch
PATCH_SHA256: 1f2ce7c0d9708df1024a332c1a162049158e368a4e86b747740bb1ccb4236034
VULNERABLE_LOOKUPS_BEFORE: 17
VULNERABLE_LOOKUPS_AFTER: 0
GENERATED_LOCAL_BODIES: 7/7
BSREST_808003FC_GENERATED_MATCH_ORIGINAL: YES
ATTRACT_STATIC_TEST: PASS
TOOIE_BUILD: PASS
WOOD_HOLLOW_REACHED: YES
TDL_REACHED: YES
CCL_REACHED: YES
CCL_MODE2_PLAYER_INIT_COMPLETED: YES
BSREST_EP11_EXECUTED: YES
CORRECT_BSREST_808003FC_TARGET_EXECUTED: YES
WRONG_BAFLAMELINE_BIND_COUNT: 0
STARTUP_INTRO_395_REACHED: YES
SURVIVED_OLD_CRASH_POINT: YES
EXIT_139: NO
ATTRACT_RUNTIME_SECONDS: 300s
ATTRACT_TRANSITIONS_COMPLETED: 7
UNEXPECTED_SIGSEGV: NO
FIX_IMPLEMENTED: YES
FIX_VALIDATED: YES
```
