---
title: "P08 — session handoff: attract demo e render submission"
description: "Estado autossuficiente P08: o oracle Angrylion RDP Plus + CXD4 LLE provou que o hardware renderiza Z em RDRAM e record+0x34=1 (25/25), classificando a divergência antiga do Rice como R-ORACLE."
status: evidence
phase: P08
updated: 2026-08-20
tags: [tooie-recomp, p08, handoff, zbuffer, oracle, angrylion, cxd4, lle, r-oracle, rt64, intro, attract, demo2, preemption, patch4, visible-demo]
---

# P08 — session handoff: ATTRACT DEMO & RENDER SUBMISSION

## CURRENT_HEAD

- Branch: `main`.
- Active Fix Evidence: [[p08-attract-loop-after-demo2-crash]] (**RESOLVED and VALIDATED**).
- Preserved diagnosis: [[p08-demo-movement-precision-diagnosis]] §16 & §17 (first demo shot miss & target collision volume resolved).
- Active Fix Evidence: [[p08-stick-angle-thunk-fix]].
- Independent Review: [[p08-stick-angle-thunk-fix-rereview]] — **VERDICT A**; prior `$r1` delay-slot blocker resolved.
- Stick-angle diagnosis: [[p08-player-direction-facing-diagnosis]].
- Gameplay Demo Motion Sync Diagnosis: [[p08-demo-full-motion-sync-diagnosis]] (**MEASURED_ORIGINAL vs MEASURED_RECOMP**, 2026-08-16).
- Display & Effect Catalog: [[p08-display-effect-catalog]].
- Title & File Select Validation: [[p08-title-file-select-validation]].
- Render-submission fix: [[p08-render-submission-matrix-fix]].
- Directional input diagnosis: [[p08-directional-input-movement-diagnosis]].
- Diagnosis: [[p08-render-submission-matrix-diagnosis]].
- Discriminator: [[p08-actor-camera-transform-discriminator]].
- Movement diagnosis: [[p08-visible-demo-movement-diagnosis]].
- RA-wrapper fix: [[p08-core1-ra-wrapper-fix]].
- Overlay angle producer: [[p08-demo-camera-overlay-angle-producer]]
  (`ncbapivot_entrypoint_3` PROVEN).
- Camera eye producer differential: [[p08-demo-camera-eye-producer-differential]] (PROVEN).
- Bounded branch predicate differential: [[p08-demo-ncbapivot-branch-predicate]]
  (PROVEN Case C / Epistemic correction: branch 0x808001C0 not taken in both;
  gate divergence at 0x808001D0 via `func_801138A0` return value and `func_80113D30` clearance tests).
- Four-query clearance differential: [[p08-demo-camera-clearance-rays]]
  (**CORRECTED/SUPERSEDED** by [[p08-demo-camera-collision-sol-review]]:
  the measured `func_800E8918` return divergence is real, but `func_800C4C34`
  is projection/viewport clipping, not collision, and its direct `v0` was not
  the value captured by the trace).

## CURRENT_PHASE

P08 — RT64, input e frontend is **ACTIVE (MANUAL_VALIDATION_PENDING under Verdict B)**.
Canonical tasks (T01..T11) audited:
- T01..T06, T08..T11: **DONE** (T02 display list and effect catalog completed; [[p08-display-effect-catalog]])
- T07: **IMPLEMENTED_MANUAL_VALIDATION_PENDING** (`tooie_window` helpers verified via unit tests; fullscreen/focus/display handling operational on reference SDL/RT64 path; physical multi-monitor and visual fullscreen validation pending)
- P08-T08 (Graphics driver / Vulkan failure handling): **DONE** (`tooie_error` reporting, `assert(false)` removal, negative env test PASS).
- Attract DEMO2 SIGSEGV (bsrest 0x808003FC): RESOLVED and VALIDATED (`1f2aba1`, `887d132`).
- Next phase: **P09 remains PENDING** until manual hardware validation is completed.

## CURRENT_PRODUCT_STATE

- **P08-T07 & P08-T08 VALIDATED**:
  - Safe window creation with dimension sanitization (>= 320x240) and high-DPI scaling calculations.
  - Multi-monitor / display index validation with fallback to primary display (0).
  - Aspect ratio calculations decoupled from host window size; 4:3 baseline preserved.
  - Controlled graphics/Vulkan startup failure reporting via user-visible message box and technical stderr diagnostics without crashes (`assert(false)` removed).
  - Dead duplicate state (`tooie_window::WindowState`) pruned to maintain clean single ownership in reference SDL/RT64 frontend.
- **`func_800BCF28` SIGSEGV RESOLVED and independently accepted (verdict B)**: Tooie-native helpers bound at `0x800BCCC0` and `0x800BCCD8`.
- Intro visual completa (céu, logo, Jiggy).
- Attract request correto; type-4 completa; warp consumido; mode-5 alcançado.
- `func_800C2AB8` is behaviorally resolved with PATCH4 default ON.
- **`func_80013A7C` lookup-zero RESOLVED** (Tooie-native RA wrappers; [[p08-core1-ra-wrapper-fix]]).
- **VISIBLE GAMEPLAY DEMO & RENDER SUBMISSION RESOLVED (2026-08-14):**
  - Incremental projection split implemented in `src/game/transform_hooks.cpp`.
  - RT64 avoids the erroneous unscaled `matrixDecomposeViewProj` path.
  - Gameplay demo full motion synchronization now has **MEASURED_ORIGINAL** ([[p08-demo-full-motion-sync-diagnosis]] §9). Official mupen64plus Pure Interpreter + Rice VI hook; ROM SHA-1 match; pad 100% on 765 samples. Same-index ~25u lead is one physics step (recomp dumps at glrecord consume; original at VI). After +1 shift the first walk, analog turn, and shot-stop match. Prior 45.11° camera-lag mismatch **CORRECTED/SUPERSEDED**. Remaining discriminator: post-egg run/jump after ~392. Level C (NO FIX).
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

**P08 Status: ACTIVE (MANUAL_VALIDATION_PENDING under Verdict B).**
- Implementation is substantially complete with all automated tests passing.
- Pending items before formal P08 exit gate close and P09 open:
  1. Manual physical controller testing across varied hardware (USB/Bluetooth N64/Xbox/PS gamepad).
  2. Live physical multi-monitor display migration and desktop fullscreen transition verification.
- P08 Invariants (DO NOT REOPEN): attract DEMO2 crash, bsrest 0x808003FC, N64Recomp CreateStatic fix, first-straight ~510, fire-egg miss, cinematic facing, stick-angle 13B70, render-submission matrix, BCF28, core1 RA wrappers, PATCH 4 / preemption, P08-T10, original timing/4:3 baseline. Do not modify shared RT64. PATCH 4 stays ON.

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
Identify the relocated overlay that calls `func_80110790` (ORIGINAL RA
`0x801D3E7C` at consumed 384) and trace who writes its `a1` stack vector
(`0x80045638`) before the setter. Mode 2 / `func_80110588` is PROVEN.
`func_80110790` is a copier; its `a1[1]` already differs (−0.0675°).
Do not reopen E1A58 or FE14/CA628. See [[p08-demo-camera-mode-producer-differential]].

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

00. [[p08-t07-t08-frontend-system-validation]] — **P08-T07 & P08-T08 VALIDATED**: window/display, aspect ratio, graphics failure handling
0. [[p08-attract-loop-after-demo2-crash]] — **RESOLVED and ACCEPTED** DEMO2 attract SIGSEGV (Level A, verdict B)
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
6. [[p08-demo-camera-control-divergence]] — player-control camera writer/selector (Level B closed)
6a. [[p08-demo-camera-e1a58-differential]] — E1A58 REFUTED
6b. [[p08-demo-camera-mode-producer-differential]] — mode 2 / func_80110588 PROVEN; 80110790 a1 already differs at 384
6c. [[p08-demo-projection-record-34-differential]] — +0x34 writer B-INPUT; a3 join later corrected
6d. [[p08-demo-projection-v1-direction-producer]] — v1[2] producer; a3 inequality CORRECTED/SUPERSEDED
6e. [[p08-demo-projection-classifier-rejoin]] — Q1 +0x34 real; B-STATE func_8001A0C0 v0
7. `docs/HANDOFF.md`, `docs/plans/BT/08-rt64-input-and-frontend.md`

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

## P08 post-egg movement divergence — independent narrow differential (2026-08-17)

The next discriminator was isolated without a production change. The fresh
Recomp run used the same attract setup and the reused official original trace;
input bytes matched `765/765`.

```text
PHASE_CORRECTED_ALIGNMENT:        ORIGINAL[N] vs RECOMP[N+1]
FIRST_EXACT_DIFFERENCE:           glidx 385 ideal_yaw 99.38 vs 99.31
FIRST_MEANINGFUL_DIFFERENCE:      glidx 392 position XZ error 2.997
FIRST_BAD_FIELD:                  ideal_yaw
STATE_SEQUENCE:                   bs21 and later bs8 match at persistent split
TSPD/MULTIPLIER:                  700/700 and 1.0000/1.0000
WRITER_PC:                        UNKNOWN on original and Recomp
ROOT_CAUSE_LEVEL:                 C
FIX_IMPLEMENTED:                  NO
```

This is a turn drift that grows into a jump-trajectory drift. The camera is
downstream, the actor control scan is UNKNOWN, and the later split occurs after
the already-fired first egg; the first shot miss remains closed. See
[[p08-demo-post-egg-motion-divergence]].

```text
NEXT_SINGLE_ACTION: obtain a bounded original/Recomp guest-store trace for the
                    ideal_yaw writer only; do not change production behavior.
```

## P08 ideal-yaw writer differential — RESOLVED (2026-08-17)

The exact ideal_yaw writer and its input provenance are now PROVEN by a
bounded original/Recomp store trace
([[p08-demo-ideal-yaw-writer-trace]]). The original Pure-Interpreter build was
rediscovered at `/tmp/p08-m64p` and rebuilt locally (no sudo) with a narrow
store tracer; the Recomp writer was captured via a GDB breakpoint on
`yaw_setIdeal`.

```text
MUPEN_SOURCE_FOUND:               YES (/tmp/p08-m64p; core 6dca4c1, rice 038882d)
MUPEN_REBUILT:                    YES (local, no sudo)
ROM_MATCH:                        YES (af1a89e12b638b8d82cc4c085c8e01d4cba03fb3)
ORIGINAL_IDEAL_WRITER_PCS:        {0x8009C930}  (yaw_setIdeal store)
RECOMP_IDEAL_WRITER_PCS:          {0x8009C930}  (same generated function)
WRITER_PC_MATCH:                  YES
IDEAL_YAW_WRITER_FUNCTION:        yaw_setIdeal (0x8009C914)
CALLER_FUNCTION:                  func_8009D088 (case 1; jal at 0x8009D0EC)
WRITER_INPUT_ALREADY_DIFFERS:     YES — glidx 385 a1 = 99.3756 vs 99.3081
RAW_PAD_MATCH:                    YES
REMAP_MATCH:                      YES
STICK_MAG_MATCH:                  YES
STICK_ANGLE_MATCH:                YES (bang identical at every glidx)
PLAYER_CONTROL_CAMERA_MATCH:      NO (first differs glidx 385)
PLAYER_CONTROL_CAMERA_SOURCE:     D_80127708[*(ps+0x184)*4] + 0x1C
SAME_AS_PREVIOUS_DISPLAY_CAMERA:  NO (display D_8012D500 departs at 386)
CAMERA_DIVERGENCE_REL_TO_PLAYER:  UPSTREAM_OF_PLAYER (CORRECTED/SUPERSEDED
                                  from AFTER_PLAYER for the control source)
PRIMARY_CLASS:                    SAME_WRITER_DIFFERENT_INPUT
                                  (first divergent input = CONTROL_CAMERA_INPUT)
INITIAL_0P07_DELTA_CLASS:         PROVEN_UPSTREAM_DIVERGENT_VALUE (writer input
                                   differs; earlier selector/internal camera
                                   state is not excluded)
EARLY_TURN_STORE_TRACE_CONTROL:   PASS (glidx 74..91 delta 0.0000)
ROOT_CAUSE_LEVEL:                 B (identical writer; stick path refuted;
                                  camera-angle writer instruction remains)
FIX_IMPLEMENTED:                  NO
```

The stick path (`bastick_update → func_80013B70`) is REFUTED as the first
divergent input. Independent review accepts the writer provenance with a
bounded correction: `WRITER_SET_COMPLETE=UNKNOWN`,
`ORIGINAL_INDEX_MATCH_RECOMP=UNKNOWN`, and the control/display-camera
dependency is not proven. The next discriminator is the aligned selector and
complete-width store trace for `D_80127708[index]+0x1C`. No production change
is authorized.

## P08 player-control camera divergence — RESOLVED to Level B (2026-08-17)

The selector/store-set/internal-state discriminator was executed
([[p08-demo-camera-control-divergence]]). The selector `ps+0x184` is **0 in
both runtimes at every sample**; the selected `D_80127708[0]` record is the
same logical camera-control object; the camctrl store set is complete
(`{0x800CA65C}` = `func_800CA628` only, no alias/block write); the camctrl
writer input first diverges at glidx 385 (`-0.0676` deg); the camera delta
maps 1:1 onto the ideal_yaw delta.

```text
MUPEN_ENV_REUSED:               YES (/tmp/p08-m64p; core 6dca4c1, rice 038882d)
PS_0X184_FIELD_SIZE:            4 bytes (s32; camera-control slot selector)
PS_0X184_SEMANTIC:              index into D_80127708 (8-entry pointer table of
                                player-control camera records)
ORIGINAL_SELECTOR_SEQUENCE:     0 at every writer call (367..405)
RECOMP_SELECTOR_SEQUENCE:       0 at every demo sample (1..480)
SELECTOR_MATCH:                 YES
D_80127708_KIND:                8-entry pointer table (.space 0x20, stride 4)
TABLE_SLOT_MATCH:               YES (index 0 in both)
SELECTED_RECORD_SEMANTIC_MATCH: YES (camera-control record; +0x1C = player yaw)
CAMCTRL_PLUS_0X1C_SEMANTIC:     player-control camera angle consumed by
                                bastick_getAngleRelativeToBanjo
CAMCTRL_DIRECT_STORE_PCS_ORIGINAL: {0x800CA65C} (func_800CA628, swc1 $f6,0x14($v1))
CAMCTRL_DIRECT_STORE_PCS_RECOMP:   func_800CA628 (same generated MIPS)
CAMCTRL_STORE_SET_COMPLETE:     YES (all widths + overlap detection; only
                                0x800CA65C touches selected+0x1C)
ALIAS_OR_BLOCK_WRITE_RISK:      LOW (aligned8_memcpy overlaps ps+0x184 but never
                                cam+0x1C)
CAMCTRL_WRITER_MATCH:           YES
CAMCTRL_PRIMARY_WRITER_PC:      0x800CA65C
CAMCTRL_PRIMARY_WRITER_FUNCTION: func_800CA628 (0x800CA628)
CAMCTRL_PRIMARY_WRITER_CALLER:  func_8010FC38 (JAL 0x8010FCD8) and
                                func_8010FE14 (JAL 0x8010FE1C)
EARLY_CAMERA_WRITER_CONTROL:    PASS (glidx 74..91; camctrl tracks <0.005 deg)
FIRST_INTERNAL_CAMERA_EXACT_DIFFERENCE:   glidx 385 (camctrl +0x1C -0.0676 deg)
FIRST_INTERNAL_CAMERA_MEANINGFUL_DIFFERENCE: glidx 385
FIRST_INTERNAL_DIVERGENT_FIELD: camctrl (+0x1C); eye +0x00 in same write
FIRST_INTERNAL_DIVERGENT_GLIDX: 385
CAMCTRL_WRITER_INPUT_ALREADY_DIFFERS: YES
FIRST_BAD_CAMERA_INPUT:         camctrl angle (src2[1] -> cam+0x1C)
                                CORRECTED/SUPERSEDED from src2[2]
FIRST_BAD_CAMERA_INPUT_SOURCE:  CORRECTED/SUPERSEDED — FE14/CA628 during
                                func_8010FC38, before CA8B4/E1A58
                                ([[p08-demo-camera-e1a58-differential]])
CONTROL_DISPLAY_CAMERA_RELATION: SHARED_UPSTREAM_STATE (no direct dependency
                                proven; display departs 386)
GLIDX385_CLASS:                 FIRST_CAUSAL_CAMERA_STATE
PLAYER_DRIFT_CHAIN_CONFIRMED:   YES (d_camctrl == d_ideal to 0.0001)
PRIMARY_CLASS:                  SAME_WRITER_DIFFERENT_INPUT
                                (first divergent input = CONTROL_CAMERA_INPUT)
FIRST_BAD_STAGE:                camctrl write at func_800CA628, glidx 385
FIRST_BAD_FUNCTION:             CORRECTED/SUPERSEDED — not E1A58 (identity).
                                FE14/CA628 during FC38 mode update
FIRST_BAD_GUEST_PC:             0x800CA65C SAME in both; src2[1] already differs
ROOT_CAUSE_LEVEL:               R (func_800E1A58 REFUTED;
                                [[p08-demo-camera-e1a58-differential]])
FIX_OWNER_CANDIDATE:            UNKNOWN (FC38 mode-update helpers)
FIX_IMPLEMENTED:                NO
```

## P08 func_800E1A58 differential — REFUTED (2026-08-17)

Function-level original/Recomp pipeline at consumed 70..95 and 370..410
([[p08-demo-camera-e1a58-differential]]). `func_800E1A58` is identity on both
sides (all 10 constraint slots type 1). The first meaningful camctrl is
already `src2[1]` at the FE14/`func_800CA628` write during `func_8010FC38`,
before CA8B4/E1A58 (consumed 384 = prior aligned 385, −0.0675°).
`cam+0x1C` is `out2[1]`, not `out2[2]`. Call alignment 1/1/1/2 PASS. Early
control PASS (`|Δ|≤0.0007°`). Level R. No production change.

## P08 FC38 mode producer — Level B (2026-08-17)

Mode 2 / `func_80110588` is **PROVEN**. `func_80110790` `a1[1]` already
differs at consumed 384 (−0.0675°). Caller is **PROVEN** `ncbapivot`
(781) / `ncbapivot_entrypoint_3` (RA offset `0x24C`). dest[1] =
`func_80013B70(eye.x-tgt.x, eye.z-tgt.z)`; inputs already differ at 384.
See [[p08-demo-camera-overlay-angle-producer]] and
[[p08-demo-camera-13b70-inputs]]: `EYE_INPUT_MATCH=NO` (grows at 384),
`TARGET_INPUT_MATCH=NO` (stable background). Causal component is eye;
last writer `func_80112E98`.

## P08 camera collision attribution — REFUTED (2026-08-18, GPT-5.6 Sol X-HIGH)

Independent original-MIPS and generated-path review
([[p08-demo-camera-collision-sol-review]]) proves that `func_800C4C34` is a
camera-space projection and viewport/depth clipping helper. It has no
broadphase, collision candidates, triangles, masks, or intersection helper.
`func_800E8918` returns `lh selected_record+0x34`, not the direct `v0` from
`func_800C4C34`; historical `q_hit_v0` / CLEAR / HIT labels are
**CORRECTED/SUPERSEDED**. The counter-to-camera causal chain remains
**PROVEN**. Q1 and Q2 already differ at the first measured relevant invocation,
glidx 70; glidx 384 is the first visible effect. Root level `R`; no fix.

`NEXT_SINGLE_ACTION`: trace the selected `func_800E8918` record identity and
the first differing writer/input to its `+0x34` halfword, beginning with the
candidate writer region in `func_800E82EC` (`0x800E851C..0x800E866C`).

## P08 projection record +0x34 differential — B-INPUT PROVEN (2026-08-19)

Full record-identity / store-provenance differential
([[p08-demo-projection-record-34-differential]]). The `func_800E8918` return
(`lh selected record+0x34`) already differs at the first relevant invocation
(Q1 glidx 70: Original 0 vs Recomp 1; Q0 control 0 vs 0 — PASS). The causal
writer is `func_800E82EC` (mode==1) in both runtimes, same logical record and
identical shared inputs (`a0=trunc(rec+0)=176`, `a1=trunc(rec+4)=161`,
`a2=v1[1]=0x3f820408`), but the classifier `func_800E81E0` receives a
causally different `a3 = v1[2]` (147.82677 original vs 147.41731 recomp), so
`f0(E81E0)` = -73.34 (orig) vs -1264.7 (recomp), flipping `f20 = rec+8 + f0`
and hence `+0x34` = 0 vs 1.

```text
B-INPUT (PROVEN)             NO FIX
E82EC same-writer:           YES
E8918 final return:          0x800E8A10 lh v0, 0x34(s0)
Q1 glidx70 rec+0x34:         ORIG 0 / RECOMP 1
Q0 control:                  PASS (0/0)
a3=v1[2] orig/recomp:        147.82677 / 147.41731
f0(E81E0) orig/recomp:       -73.34 / -1264.7
f20=rec+8+f0:                +947 / -244
a3 source:                   D_8012D600+phi*60+sub*12+8 (E82EC first loop,
                             func_800A8984/func_800CA6D4 direction vectors)
MUPEN_REUSED:                YES (core 6dca4c1, rice 038882d; REQUIRED --emumode 0
                             Pure Interpreter; Cached Interpreter does NOT carry the
                             pure_interp hooks)
ROM_MATCH:                   YES (af1a89e12b638b8d82cc4c085c8e01d4cba03fb3)
FIX_IMPLEMENTED:             NO
```

## P08 projection v1[2] direction producer — join CORRECTED (2026-08-20)

[[p08-demo-projection-v1-direction-producer]]. Aligned Q1 `a3` matches 26/26.
The `0x4313D3A7` vs `0x43136AD5` pair is consecutive stream values on both
runtimes. That inequality is **CORRECTED/SUPERSEDED**.

## P08 projection classifier rejoin — B-STATE (2026-08-20)

[[p08-demo-projection-classifier-rejoin]]. Q1 `+0x34` 0 vs 1 **survives**
correct join (25/25). Explicit `a0–a3` match. First difference:
`func_8001A0C0(176,161)` `v0` (Rice control 0 vs RT64 nonzero). E8918 selector 2
returns 0 vs 1 (**PROVEN** continuity). Q0 selector 1 remains 0/0.
**B-STATE + B-WRITER (2026-08-20)** [[p08-demo-1a0c0-state-differential]],
[[p08-demo-db-8007bcc4-lifecycle]]: `func_8001A0C0` is a pure leaf
`v0 = *(u16*)(D_8007BCC4 + 608*a1 + 2*a0)`. Allocation chain, base
(`D_8007BCC4 = 0x80137840` both runtimes), effective address
(`0x8014F800` both), and allocation-time cell (0 in both) all **match**.
ORIGINAL_RICE_CONTROL boot-bzeros the region once and never copies rendered Z
back to the target (store-watch: 1 write, the boot bzero, value 0). RT64's cell
is 0 right after allocation (ALLOC_AFTER), then copyback turns it to 0xFFFE and
live values (0x83A4..0x8BFC) by Q1. This proves a renderer-control difference,
not a real-N64 defect (B-ALLOC/B-ALIAS/B-INIT remain refuted).

```text
NEXT_SINGLE_ACTION: superseded by the hardware-faithful Original oracle run.
```

## P08 projection DB / Z-buffer target writer identification — B-WRITER PROVEN (2026-08-20)

[[p08-demo-db-target-writer]]. The exact Recomp writer to `D_8007BCC4 + 0x17FC0`
(`0x8014F800`) is PROVEN:
- `D_8007BCC4` (`0x80137840`, size `0x21D80` = 138,624 bytes) is the **304×228 16-bit N64 Z-Buffer (Depth Buffer)** (`304 * 228 * 2 = 138,624 = 0x21D80`).
- Target `0x8014F800` is the Z-buffer pixel at screen coordinates $(X=176, Y=161)$.
- `func_8001A0C0(176, 161)` is a Z-buffer depth query used by `func_800E81E0` for projection occlusion / visibility testing.
- The Recomp writer is `RT64::Framebuffer::copyNativeToRAM` -> `RT64::NativeTarget::copyToRAM` (`memcpy`), performing GPU depth buffer writeback to RDRAM (`0x80137840..0x801595C0`).
- Display list commands `gDPSetDepthImage(0x00137840)` and `gDPSetColorImage(0x00137840)` are emitted by `func_800E7BC8` and `func_800E7C28`.
- First write value is `0xFFFE` (clear depth); subsequent values `0x83A4..0x8BFC` are rendered geometry depths.
- ORIGINAL_RICE_CONTROL omitted depth writeback and left RDRAM at 0x0000.
  REAL_N64_EXPECTATION is a populated RDRAM Z image after proper RDP and cache
  synchronization. RT64 populates it, but its exact format is not bit-exact.

```text
ROOT_CAUSE_LEVEL:   C-ORACLE
FIX_IMPLEMENTED:    NO
NEXT_SINGLE_ACTION: run the bounded Mupen Pure Interpreter + Angrylion RDP Plus
                    + CXD4 LLE Original oracle experiment.
```

## Z-buffer oracle-validity & Angrylion hardware oracle — 2026-08-20

Controlling evidence:
- [[p08-demo-zbuffer-oracle-sol-review]] (epistemic arbitration)
- [[p08-demo-zbuffer-angrylion-oracle]] (**RESOLVED as R-ORACLE**)

- D_8007BCC4 identity as the 304×228 16-bit Z image: **PROVEN**.
- func_8001A0C0 screen-space compressed-Z sampling role: **PROVEN**.
- Rice CPU-visible depth writeback: **NOT IMPLEMENTED**; Rice is not an oracle
  for this camera path.
- Angrylion RDP Plus + CXD4 LLE hardware oracle run:
  - Live depth `0x866D..0x8C11` populated in RDRAM;
  - Clear value is `0xFFFC` (matches `ORIGINAL_GAME`);
  - `func_8001A0C0` reads live depth;
  - `func_800E81E0` produces `f0 ≈ -1240` (24/25 float match with RT64);
  - `record+0x34` is **`1`** across **25/25 aligned Q1 samples**, exactly agreeing with `RT64_CONTROL`.
- Camera differential status: **R-ORACLE** (Rice-based divergence was an emulator artifact; RT64 camera behavior is faithful to real N64 hardware).
- Subsidiary finding: RT64 forces dz=0 and clear 0xFFFE, but these low bits are discarded by `func_800E81E0` and are non-causal to the camera (**B-FORMAT-NONCAUSAL**).
- No production fix needed.

NEXT_SINGLE_ACTION: return to the next P08 residual motion discriminator (post-egg movement at ~glidx 392).
