---
title: "P08 — session handoff: attract demo e render submission"
description: "Estado autossuficiente P08: o oracle Angrylion RDP Plus + CXD4 LLE sustenta o R-ORACLE e o R-MOTION-ORACLE; USER_DEMO_4 bind-tick round.w.s e idle follow-cam 1..38 agora casam com o oracle; a primeira diferença restante é ncbapivot no dump glidx 39."
status: evidence
phase: P08
updated: 2026-08-21
tags: [tooie-recomp, p08, handoff, zbuffer, oracle, angrylion, cxd4, lle, r-oracle, r-motion-oracle, rt64, intro, attract, demo2, visible-demo]
---

# P08 — session handoff: ATTRACT DEMO & RENDER SUBMISSION

## CURRENT_HEAD

- Branch: `main` @ `f1fdfefb3346e16bc813c106f0947c0d60aa228c` (walkstart capture). ROUND.W.S fix is `5854872c79a7cab1c9395ddcd173cece449ffe11`.
- ANGRYLION_CONTROL tree: `tools/oracle/angrylion-control/` (relocated from `/tmp/p08-m64p*`; [[../../specs/BT/angrylion-control-oracle]]). Playable: `./run-tooie-angrylion-rdp.sh`. RT64: `./run-tooie.sh`. TooieRecomp stays RT64.
- Agent workspace: open CLI/IDE in **TooieRecomp/**. MCP/vault configs are in-repo ([[../../MARKDOWN_VAULT_TOOLS]]). Sibling `BanjoRecomp` + cart ROM remain for CMake/oracle only.
- Provisional Player & AI Speed Parity Baseline: [[p08-provisional-player-ai-speed-parity]] (`PLAYER_SCALAR_SPEED_PARITY: PASS`, `AI_ACTOR_SPEED_PARITY: PASS`, `USER_DEMO4_CLASSIFICATION: CAMERA_DIRECTION_DEFECT_WITH_SPEED_PARITY`, `HISTORICAL_FIRST_STRAIGHT_SPEED: PROVEN_CLEAN`).
- Attract user-numbering / USER_DEMO_4 egg defect: [[p08-provisional-follow-camera-and-egg-differential]] (`EGG_PARITY_DEFECT: PROVEN`).
- USER_DEMO_4 post-ROUND.W.S full-state discriminator: [[p08-user-demo4-post-round-full-state-discriminator]] (idle CA8B4/CA628 glidx 1..38 MATCH; first remaining bit difference = dump glidx 39 `ncbapivot_entrypoint_3` output; player pos/yaw/ideal MATCH).
- USER_DEMO_4 walkstart ncbapivot join: [[p08-user-demo4-walkstart-ncbapivot-discriminator]] (`800137AC` a2/f14 **CORRECTED/SUPERSEDED**; first split is `80112C5C` on `p60_8` 0 vs 5; `ROOT_CLASS=B-NCBAPIVOT-CONTROL-INPUT`).
- USER_DEMO_4 CA8B4 first seed: [[p08-user-demo4-ca8b4-first-seed]] (pre-fix glidx-1 ULP **CORRECTED/SUPERSEDED** by ROUND.W.S).
- USER_DEMO_4 ncbapivot first tick: [[p08-user-demo4-ncbapivot-first-instruction]] (pre-fix n74 DIFF; **POST-FIX bind n74 MATCH**).
- USER_DEMO_4 live-eye writer: [[p08-user-demo4-live-eye-writer]] (node init MATCH; `80110770`/`800EE7F8` faithful; first bit write is ordinal 2 on the mode=2 bind ncbapivot tick).
- USER_DEMO_4 bind-tick follow-eye: [[p08-user-demo4-bind-tick-follow-eye]] (pre-fix A-CANDIDATE `006d5329` vs `006d5328`; **POST-FIX MATCH** `006d5328`). Live replay: [[p08-user-demo4-sincos-semantics-fix]].
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
- T07: **IMPLEMENTED_MANUAL_VALIDATION_PENDING** (`tooie_window` helpers verified via unit tests; physical display validation `PASS` by user confirmation; formal physical gamepad validation `PENDING_HARDWARE_UNAVAILABLE`)
- P08-T08 (Graphics driver / Vulkan failure handling): **DONE** (`tooie_error` reporting, `assert(false)` removal, negative env test PASS).
- Attract DEMO2 SIGSEGV (bsrest 0x808003FC): **RESOLVED and VALIDATED**.
- User physical display confirmation: **PASS** — all 14 mandatory display/window tests passed user validation.
- User physical gamepad status: **PENDING_HARDWARE_UNAVAILABLE** — no gamepad hardware currently available for formal 15/15 checklist execution (`USER_CONFIRMED_CONTROLLER_GAMEPLAY: YES` retained).
- Only remaining formal P08 gate: **PHYSICAL_GAMEPAD_VALIDATION**.
- Next phase: **P09 remains PENDING / NOT_FORMALLY_OPEN** until the P08 manual gamepad gate is completed.
- Provisional pre-P09 parity evidence: **CORRECTED/SUPERSEDED** by [[p08-provisional-follow-camera-and-egg-differential]] (`EGG_PARITY_DEFECT: PROVEN` and USER_DEMO_4 follow-camera **REPRODUCED**; Demo 0..3 is not the complete attract cycle).
- USER_DEMO_4 post-ROUND.W.S full-state join: [[p08-user-demo4-post-round-full-state-discriminator]] — idle follow-cam glidx 1..38 bit-identical. Walkstart join [[p08-user-demo4-walkstart-ncbapivot-discriminator]]: first remaining split is glidx 39 `80112C3C` jumptable on `*(node+0x60)+8` (0 vs 5 at ncbapivot enter; player pos/yaw/ideal MATCH). Demo-sync still prints 0.0853° @41 then feedback accumulation. Egg remains BLUE vs FIRE; independence from remaining camera is **UNKNOWN**. ROUND.W.S architectural fix is committed as `5854872c79a7cab1c9395ddcd173cece449ffe11` (`CAMERA_COMPLETE_FIX: NO`, `CAMERA_FEEDBACK_ACCUMULATION: STILL_PRESENT`). `glidx 36` / pre-fix glidx-1 seed are **CORRECTED/SUPERSEDED**. `800137AC` a2/f14 as first live input is **CORRECTED/SUPERSEDED**.
- The glidx-39 p60 producer is now bounded and joined as **B-P60-WRITER-INPUT**: RT64 `80112A84 → 80112D08` supplies `a1=0` from `p60+0xC=0`; the oracle supplies `a1=5` from `p60+0xC=1`. See [[p08-user-demo4-p60-writer-provenance]]. No production fix is authorized.

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
- Physical display validation: **PASS** (user confirmed all 14 mandatory display tests).
- Remaining P08 exit gate before formal P09 open:
  1. Manual physical controller testing on real gamepad hardware (`PENDING_HARDWARE_UNAVAILABLE`).
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
6f. [[p08-user-demo4-ca8b4-first-seed]] — USER_DEMO_4 glidx-1 CA8B4 identity / rec init MATCH
6g. [[p08-user-demo4-ncbapivot-first-instruction]] — USER_DEMO_4 ncbapivot ep3 enter: node+0x74 already differs
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
- All native unit tests (`stick_angle_thunk_test`, `matrix_submission_test`, `input_direction_test`, `bcf28_helpers_test`, `core1_ra_wrappers_test`, `mips_rounding_test`): **PASS**.
- Full 300s autonomous attract soak with Vulkan: **PASS** (completed transitions `INTRO → 335 Wooded Hollow → 278 TDL → 314 CCL → 395 CS Intro → INTRO → 235 Mayahem → 296 JRL`, `EXIT_139=NO`, `UNEXPECTED_SIGSEGV=NO`).

```text
ROOT_CAUSE_LEVEL: A
FIX_OWNER: N64RECOMP
CREATESTATIC_ORDERING_FIX: IMPLEMENTED in patches/runtime/n64recomp-tooie.patch
PATCH_SHA256: adf97c9dfe20220143939ca1aa8728afc14a4eff957e35d8d2352ef027b98dc7
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

## Fresh USER_DEMO_4 walkstart ncbapivot join (2026-08-21)

The bounded real-display capture is now complete. RT64 and the Angrylion
oracle ran on `DISPLAY=:0` with RADV RX 580 (720 s timeout), using only the
diagnostic `TOOIE_UD4_WALKSTART=1` gate. RT64 reached `stage=0/sub=4` and
captured seven ncbapivot entries/leaves for glidx 36..42; the oracle reached
the same sub-window and produced the glidx 39 trace plus the motion CSV.
Joining by consumed glrecord and pad bytes gives exact agreement for glidx
38..42 (including glidx 39 `-48,-46,0001`).

At glidx 39, the entry read values match after pointer relocation (node+0x74,
node+0x80, tgt, selected record, vectors, position, yaw, ideal-yaw, f0/f12),
but the output differs. This does **not** close a same-complete-inputs/FPU
join: RT64 exposes only `rm=0`/`mips3=0`, while the oracle exposes FCSR
`01000800` at enter and `01800800` at leave. The earliest observed child-input
divergence is `800137AC` enter (`a2/f14`: RT64 `439a589e/41aa94a2`, oracle
`439a7fd5/439a054e`); call-stack order diverges immediately afterward.

```text
P08_STATUS:                        ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                        PENDING / NOT_FORMALLY_OPEN
NCBAPIVOT_WALK_READ_SET_COMPLETE: NO (RT64 FCSR/FPR provenance incomplete)
FIRST_EXECUTED_READ_DIFFERENCE:    NOT_PROVEN; earliest observed child-input difference 800137AC enter
SAME_INPUTS_DIFFERENT_OUTPUT:      NO / NOT_PROVEN
ROOT_CLASS:                        B-NCBAPIVOT-WALKSTART-FIRST-EXECUTED-INPUT
ROOT_CAUSE_LEVEL:                  NOT_PROVEN
FIX_IMPLEMENTED:                   NO
RT64_ARTIFACT:                     /tmp/p08-rt64-display-glidx39-r2.log
ORACLE_ARTIFACTS:                  /tmp/p08-angrylion-ud4-cam.log, /tmp/p08-angrylion-user_demo4.csv
```

This remains diagnosis-only. Do not reopen `round.w.s`, generated output,
camera feedback, or the egg path. The next bounded discriminator is missing
RT64 FCSR/read provenance around the `800137AC` inputs.

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

## Z-buffer & Post-Egg Motion Oracle Resolutions (2026-08-20)

Controlling evidence:
- [[p08-demo-zbuffer-oracle-sol-review]] (epistemic arbitration)
- [[p08-demo-zbuffer-angrylion-oracle]] (**RESOLVED as R-ORACLE**)
- [[p08-demo-post-egg-angrylion-motion-differential]] (**RESOLVED as R-MOTION-ORACLE**)
- [[p08-final-physical-validation]] (**PENDING USER EXECUTION**)

Summary of hardware oracle findings:
1. **Z-Buffer Oracle (R-ORACLE):** The hardware-faithful `ANGRYLION_CONTROL` run (Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE) produced live Z in RDRAM, with clear value `0xFFFC`; `record+0x34` evaluates to `1` across all 25/25 aligned Q1 samples. This is an emulator oracle, not a physical N64 measurement. The prior camera divergence was a Rice HLE reference artifact.
2. **Post-Egg Motion Oracle (R-MOTION-ORACLE):** The historical motion drift (which reached 58 units under Rice) disappeared completely under `ANGRYLION_CONTROL`. `camctrl` at glidx 385 matches within `+0.0030°` (delta `53.5799°` vs `53.5829°`), `ideal_yaw` matches identically (`102.59°` vs `102.59°`), maximum XZ error across the entire 765-frame Demo 0 stream is `< 0.89 units`, and behavior-state progression is 100% bit-identical (`0` state mismatches).
3. **No Production Fix Required for this measured stream:** Demo 0 movement, camera-control, and physics measurements are faithful to the hardware-faithful oracle. This does not close untested attract demos or the formal physical-display gate.
4. **Current Phase & Remaining Gates:**
   - `CURRENT_PHASE`: **P08 ACTIVE / MANUAL_VALIDATION_PENDING**
   - `AUTOMATED_SCOPE`: **COMPLETE**
   - `P09_FORMALLY_OPEN`: **NO**
   - `CURRENT_BLOCKERS`:
     - `PHYSICAL_GAMEPAD_VALIDATION`
     - `PHYSICAL_MULTI_MONITOR_DISPLAY_VALIDATION`

NEXT_SINGLE_ACTION: user executes docs/plan/evidence/p08-final-physical-validation.md checklist on physical desktop hardware.

For repeatable USER_DEMO_4 walkstart captures, use
[[p08-walkstart-reproduction-runbook]] before starting a new diagnostic run.

## Attract user-numbering and USER_DEMO_4 egg defect (2026-08-21)

Canonical: [[p08-provisional-follow-camera-and-egg-differential]].

The user numbers attract gameplay continuously across intro/music blocks.
`USER_DEMO_4` is map 235 Haunted Cavern (`stage=0 sub=4`) after the first
intro return, not zero-based Demo 3. Syscall spawn: Angrylion `cheggfire` 535
vs RT64 `cheggnormal` 539. `EGG_PARITY_DEFECT: PROVEN`. User confirmed this
Angrylion RDP Plus nightly-build run was visually correct for fire egg and
for the previously-broken demo behavior.

b042865 complete-attract claims (`REFERENCE_FIRE_EGG_EVENT_FOUND=NO`,
`GLOBAL_VISUAL_DRIFT_STATUS=NOT_REPRODUCED_IN_ATTRACT_SET`,
`REAL_PARITY_DEFECT_FOUND=NO`) are **DISPUTED** / **CORRECTED/SUPERSEDED**.
`HAUNTED_CAVERN_NO_FIRE_EVENT` is **REFUTED**. Demo 0 Wooded Hollow
R-MOTION-ORACLE measurements are retained for **that clip only**. USER_DEMO_4
follow-camera + player trajectory are **REPRODUCED** (pad 877/877, camctrl
split glidx 41, FEEDBACK_ACCUMULATION). P09 is **NOT_FORMALLY_OPEN**. No
production fix.

```text
EGG_PARITY_DEFECT:              PROVEN
CAMERA_DIVERGENCE_REPRODUCED:   YES
PLAYER_TRAJECTORY_DIVERGENCE_REPRODUCED: YES
CAMCTRL_WRITER:                 func_800CA628 CONFIRMED (USER_DEMO_4); stores src2[1]
CA628_INPUTS_AT_GLIDX_36:       ALREADY_DIFFER (eye ULP + src2[1] 1-2 ULP); producer func_800CA8B4
ANGRYLION_NIGHTLY_VISUAL:       USER_CONFIRMED PASS
FULL_ATTRACT_SEQUENCE_REPEAT_PROVEN: NO
FIX_IMPLEMENTED:                NO
NEXT_SINGLE_ACTION:             dump USER_DEMO_4 CA8B4/camera-record eye+camctrl for glidx 1-35 idle slew; no fix
```

## Provisional Player & AI Movement Speed Parity Baseline (2026-08-21)

Canonical: [[p08-provisional-player-ai-speed-parity]].

Independent parity check of movement speed magnitude, timing, animation rate, and trajectory across `ANGRYLION_CONTROL` (Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE) and `RT64_CONTROL` (TooieRecomp):

1. **Player Scalar Speed Parity:** **PASS**
   - Window A (`USER_DEMO_1` First Straight glidx 20..70): `tspd = 500.0`, `velxz = 510.2442 u/s` both runtimes (speed error = `0.000018 u/s`, direction error = `0.000204°`). Downhill slope scale `1.019454` confirmed original N64 behavior (`HISTORICAL_FIRST_STRAIGHT_SPEED: PROVEN_CLEAN`).
   - Window C (`USER_DEMO_1` Post-Egg glidx 380..460): `tspd = 620.0`, peak run `velxz = 673.5218 u/s` (speed error = `0.000564 u/s`, direction error = `0.005849°`).
   - Window B (`USER_DEMO_4` Haunted Cavern transition glidx 38..60): speed magnitude `sqrt(vx² + vz²)` matches within `0.0017..2.87 u/s` (sub-0.5% slope variation), while direction diverges due to the proven follow-camera defect (`CLASSIFICATION: CAMERA_DIRECTION_DEFECT_WITH_SPEED_PARITY`).
2. **AI Actor Speed Parity:** **PASS**
   - `chdiggerground` (Marker 337 / ovl 219+220): 100% exact route speed match (`0.0000 u/s` error).
   - Autonomous Patrol Baddie (Marker 2 / `sys 0x80086E60`): constant 15.8114 u/s waypoint patrol (`0.0000 u/s` error).
   - `chjinjo` (Marker 148): stationary idle and vertical hop parity (`0.0000 u/s` horizontal error).
3. **Timing and Physics dt:** **PASS**
   - Logic tick rate: 30.000 Hz (`1:1` advance per consumed glrecord sample).
   - Physics dt: `0.050000 s` during baphysics integration; `0.033333 s` input step.
   - Animation update frequency: 30.0 Hz.
4. **Classification & Status:**
   - `FIRST_PROVEN_SPEED_DEFECT: NONE`
   - `ROOT_CLASS: CAMERA_DIRECTION_DEFECT_WITH_SPEED_PARITY`
   - `FIX_IMPLEMENTED: NO`
   - `P08_STATUS: ACTIVE_MANUAL_VALIDATION_PENDING`
   - `P09_STATUS: PENDING_NOT_FORMALLY_OPEN`

## USER_DEMO_4 p60+0xC first-writer discriminator (2026-08-21)

Canonical: [[p08-user-demo4-p60-control-writer]].

The first reason why `p60+0xC` diverges (`RT64 = 0` vs `Oracle = 1` before glidx 39) is proven:
1. **Root Discriminator**: `0x800979D8` (`__bainput_func_800979D8`), a static internal helper in `bainput.c` that checks `bakey_held(self, button_id)` for C-buttons, was bound in `boot_hw_stubs.cpp` to `lookup_gap_stub<0x800979D8>` returning `v0 = 0`.
2. **Missing Write in RT64**: On `glidx=38`, the demo controller inputs hold C-Right. In Oracle, `__bainput_func_800979D8` returns 1 -> `bainput_should_rotate_camera_right` (0x80097B94) returns 1 -> `func_800A4854` calls `func_80110EFC(node, 1)` -> `func_80110E00` -> `func_801111D0` -> `func_80112D90`, storing `p60+0xC = 1` and `p60+0xE = 2`. In RT64, the gap stub returns 0, so the entire camera rotation write chain is skipped (`MISSING_WRITE`).
3. **Downstream Divergence at glidx 39**: In Oracle, `80112A84` reads `p60+0xC == 1` and sets `a1 = 5` for `80112D08`, storing `p60+8 = 5`, leading to transition pivot in `ncbapivot` (0x80112C3C). In RT64, `p60+0xC == 0`, so `80112A84` sets `a1 = 0` and `80112D08` stores `p60+8 = 0`, skipping transition pivot.
4. **Prior State**: 100% byte-matched player movement, positions, yaw, demo script ticks, and animation states across glidx 0..38.
5. **Status**: No production fix applied (worktree clean, diagnosis complete).

## Independent 0x800979D8 review and dynamic validation (2026-08-22)

The original ELF disassembly independently confirms that `0x800979D8` is a
real static helper. Its contract is stateful: `bakey_held(self, button_id)`,
logical-input enable gating via `a2`, and `BaInput.__enabled` bit updates. A
generic native mapping replaced the gap stub in
[[p08-user-demo4-800979d8-independent-review]].

`BAINPUT_HELPER_TESTS: PASS`; the four unrelated zero-return gaps remain
unchanged.

The post-fix runtime artifact is deliberately bounded; see
[[p08-user-demo4-800979d8-postfix-validation]]:
- Helper runtime: called at glidx 38–39, but `held=0` and return `0`; held-branch parity is **NOT_PROVEN**.
- Downstream `p60`: captured RT64 has `p60.C=0`, `a1=0`, and `p60+8=0`; P60 parity is **NOT_RECONCILED**.
- Movement/animation: sampled and bounded only; complete state parity is **NOT_PROVEN**.
- Egg Event: USER_DEMO_4 remains RT64 BLUE (`cheggnormal`/539) versus AN FIRE (`cheggfire`/535).
- Attract soak and memory behavior: **NOT_RUN / NOT_EVALUATED** for this fix.
- Worktree: implementation and focused test are local and uncommitted. P08 remains `ACTIVE / MANUAL_VALIDATION_PENDING`; P09 remains `PENDING / NOT_FORMALLY_OPEN`.

## USER_DEMO_4 0x800BCE84 live-input join (2026-08-22)

Canonical: [[p08-user-demo4-bce84-live-input-join]].

Exact guest `lw` at `0x800BCEAC` on USER_DEMO_4 glidx 39 C-Right (`ord=8`,
pad `(-48,-46,0001)`): both runtimes load `0x00000115` from
`obj + unk8[13]*56 + 0x24 + 13*4` (`ring=1`, offset `+0x90`), mask/return 1.
Old `bs_pressed=0` was ring 0 (`+0x58`) — **CORRECTED/SUPERSEDED**.
`LIVE_INPUT_EQUIVALENCE: PROVEN`. No commit.

## USER_DEMO_4 BaInput held-state provenance (2026-08-22)

Canonical: [[p08-user-demo4-bainput-held-state-provenance]].

Raw pad `(-48,-46,0001)` at glidx 39 **MATCH**. Diagnostic `held=` is
`prev_state[13]` and is `0` on both runtimes. AN `0x800979D8` returns `2`
and writes `p60.C=1/E=2/+8=5`; RT returns `0`. Gemini “pad 0001 == held
C-Right” is **CORRECTED/SUPERSEDED**. First discriminator is the native
helper (`ROOT_CLASS=A-CANDIDATE-HELPER`). Pending `0x800979D8` mapping is
preserved; no production edit; no commit.

```text
INPUT_DIVERGENCE_LAYER:        LOOKUP_GAP (native 0x800979D8 replacement)
FIRST_DIFFERENT_PC:            0x800979D8
NEXT_SINGLE_ACTION:            GPT-5.6 Luna x-high independent semantic
                               review of bakey_held polarity / v0=2 vs 0x2000
```

## Semantic arbitration update (2026-08-22)

The native mapping is now corrected, dirty, and uncommitted. `bakey_held`
uses the proven `prev_state` polarity and reaches `800BCE84` when that byte is
zero; `0x800979D8` returns the logical-input enabled mask (`a2=1` → `2`). The
full helper branch test passes. A fresh 180-second paired attempt was
incomplete (RT64 reached glidx 39 only in `stage=0/sub=0`; oracle did not reach
the joinable `stage=0/sub=4` target), so after-write p60 and ncbapivot parity
remain **NOT_CAPTURED / NOT_PROVEN**. P08/P09 statuses are unchanged and no
commit/push is authorized.

## Raw audit of reported paired capture (2026-08-22)

The reported `/tmp/p08-walkstart-20260822-021445/` artifacts exist and their
five reported hashes match. The target pad and downstream chain are present,
but the causal gate is not: RT64's raw `ud4-bainput-979d8-detail` target line
records `bs_pressed=0 bs_pressed_bit0=0`, while the oracle log has no equivalent
live `ButtonState.pressed` field. The reported 900-second duration is also not
supported by the RT64 `wall_ms` range (about 255 s); the real environment is
Linux Mint 22.2/kernel `6.8.0-138-generic` with RX 580 RADV. Bounded camera and
trajectory matches remain useful observations, but `800979D8_DYNAMIC_ROOT_CHAIN`
is **NOT_PROVEN**, memory leak status is **NOT_EVALUATED**, and no commit/push is
authorized. Temporary probes remain untouched because the final acceptance gate
did not pass.

## Final BCE84 checkpoint and residual register (2026-08-22)

The prior raw-audit paragraph is superseded by the second BCE84 capture. Its
hashes match the recorded `SHA256SUMS`, including `/tmp/p08-angrylion-ud4-cam.log`.
At stage 0/sub 4/glidx 39, pad `(-48,-46,0001)`, button 13, ordinal 8:

```text
800BCE84:       RT addr=8034c460 ring=1 raw=00000115 mask=1
                AN addr=80342760 ring=1 raw=00000115 mask=1
BAKEY_HELD:     RT=1 AN=1
979D8_RETURN:   RT=2 AN=2
P60_AFTER:      C=1 E=2 +8=5 on both
NCBAPIVOT:      matching branch
```

The old `bs_pressed=0` probe is **CORRECTED/SUPERSEDED** because it read ring
0. `800979D8_DYNAMIC_ROOT_CHAIN` is now **PROVEN for this bounded event**;
global attract parity is not. The exact BCE84/979D8 production probes were
removed; env-gated chain/state diagnostics remain for the next investigation.

Manual residuals are preserved in [[p08-attract-movement-residuals]]:
USER_DEMO_1 and USER_DEMO_5 are reproduced by the user; Demos 2, 3, and 6 only
appear correct manually; Demo 4 is supported by bounded paired evidence. The
Talon-Trot/locomotion-transition explanation remains an **INFERENCE / USER-
OBSERVED CORRELATION**. P08 stays `ACTIVE / MANUAL_VALIDATION_PENDING`; P09 is
`PENDING / NOT_FORMALLY_OPEN`.

## Artifact-integrity correction (2026-08-22)

The preceding “hashes match” statement is **CORRECTED/SUPERSEDED**. The
registered `SHA256SUMS` values versus current files are:

```text
FILE                              REGISTERED   CURRENT
p08-rt64-bce84-join.log          0c11c0f4...  9cdf01ae...
p08-angrylion-ud4-cam.log       47626f98...  ecc74dc1...
p08-angrylion-bce84-join.log    c58c55ff...  bf1af28d...
oracle-target-excerpt.log        3c56f5ee...  3c56f5ee...  MATCH
rt64-target-excerpt.log          0cb43009...  0cb43009...  MATCH
```

The current logs were modified at 03:24, after `SHA256SUMS` and excerpts at
03:19. Their target lines still show the corrected ring-1 join and downstream
values, so semantic content is **OBSERVED**, but the immutable capture gate is
**BLOCKED**. No commit/push is authorized. Next action: produce a fresh
bounded paired capture that writes final logs and `SHA256SUMS` atomically.

## Immutable recapture (2026-08-22T07:07Z)

Fresh directory `/tmp/p08-979d8-final-20260822T070735Z/`. Writers were stopped
before rename; `SHA256SUMS` was generated after final names + excerpts;
`sha256sum -c` **PASS** twice; artifacts then `chmod a-w`. Joined USER_DEMO_4
values match the prior semantic capture (`BCE84=0x115/1`, `bakey=1`,
`979D8=2`, `p60 C/E/+8=1/2/5`). `FINAL_ARTIFACT_INTEGRITY: PASS`.
`800979D8_DYNAMIC_ROOT_CHAIN: PROVEN` for this bounded event. No Demo 5 /
Talon Trot work. No commit/push.

## Final immutable checkpoint (2026-08-22T07:07Z)

`/tmp/p08-979d8-final-20260822T070735Z/SHA256SUMS` passes twice. The paired
target join is stage 0/sub 4, glidx 39, pad `(-48,-46,0001)`, button 13,
ordinal 8; BCE84 ring-1 raw `0x115`/return 1, `bakey_held=1`,
`0x800979D8=2`, and p60 C/E/+8 `1/2/5` match at the bounded ncbapivot event.
Temporary task-specific BCE84/979D8 probes are cleaned; reusable diagnostics
remain. Tests, build, diff check, and generated-source check pass. P08 remains
`ACTIVE / MANUAL_VALIDATION_PENDING`; Demo 1/5 movement residuals remain open.

## USER_DEMO_5 `800EEBF0` angular-input checkpoint — 2026-08-23

New immutable capture: `/tmp/p08-demo5-ee780-source-20260823T053510Z/`;
SHA256 verification passed twice. `800EE780` is a proven downstream vec3 add.
The first causal difference in the paired glidx-24 join is already its
`sp+0x54` input, produced by `800EEBF0`/`800EF214`.

`800EEBF0` takes a two-f32 angle pair plus a radius and emits
`(r*cos(p)*sin(y), r*sin(p), r*cos(p)*cos(y))`. The first and third inputs
match; only the yaw/heading differs (`4391e610` RT vs `43927d8b` AN). The
second angle resides at `sp+0x38` and is written by the caller's
`add.s @ 0x80800194`, stored at `0x808001A0` before the call at `0x8080019C`.
The add's `80012C3C`/`800125E8` operands were not dynamically joined.

Boundary: `B-ANGLE-SOURCE-PRECALL-COMBINATOR`. No production change. The
capture is documented under the actual prior-run label `Grok 4.6 High`; the
stale `Medium` string in immutable metadata was left untouched to preserve
hashes. Next action: capture only the two operands of the `0x80800194` add.

## USER_DEMO_5 p60 mode-state review — 2026-08-23

`func_80112C3C` is independently confirmed: mode `0` returns `in_yaw`, modes
`1/2/5` step through `800F1738`, and modes `3/4` use `800F1DCC`. The existing
paired trace places the first p60+8 value split at glidx 23 (`RT=5`, `AN=0`),
with the first returned-yaw split at glidx 24. Static provenance isolates
`func_80112D08` / store `0x80112D14` via caller `func_80112A84`; dynamic first
writer remains unproven due the headless RT64 Vulkan startup failure. No fix.

## USER_DEMO_5 D90 call-presence review — 2026-08-23

`func_801111D0` has two call-controlling branches: `0x80111230` tests
`func_80112E84(a0)==1` (`p60+0xC`), then `0x80111270` tests
`func_8011349C(a0,&sp+0x40)!=0`; `jal 80112D90` is at `0x80111294`.
At glidx 23 the first input is zero in both runtimes. The paired trace proves
RT reaches D90 and AN skips it, so the first control-flow split is the boolean
second-gate result at `0x80111270`; its raw helper return was not logged. The
same `(2,1,2002)` pad record is present in both runtimes. No direct C_RIGHT
predicate exists in `801111D0`; no production change.

## USER_DEMO_5 `0x80097B94` follow-up — 2026-08-23

The independent arbitration establishes the static contract: `0x80097B94`
passes action `0xD`/input `1` to `0x800979D8` and returns its `v0`. The Oracle
capture matches Gemini's `(2,1,2002)` record at glidx 23 and reaches BCE84 with
`prev_state=0`, ring 0, raw button word 0, hence result 0. The focused RT64
capture's normalized glidx 23 has a different pad record
`(-13,-15,0xF1F1)` and also returns 0. Result split `2/0` remains unjoined and
unproven; published helper implementation coverage is complete, semantic
event acceptance is not. No production change.

## USER_DEMO_5 `0x80097B70` / action `0xA` follow-up — 2026-08-24

The camera-left wrapper passes action `0xA` (`C_LEFT`) and input id `0` to
`0x800979D8`; caller `0x800A4878` tests its return at `0x800A4928` and reaches
`0x80110EFC(node, 0)` at `0x800A4930` when nonzero. The same visible Gemini
record `(2,1,2002)` is joined at glidx 22/23/24.

In the prior paired capture, record 23 is RT64 `v0=1`/consumer reached versus
Oracle `v0=0`/consumer skipped. Oracle's complete BCE84 state is
`prev=0, ring=1, raw=0xF3, masked=1, bakey=1`, with input 0 disabled before
the helper; prior RT64 had `bakey=1` and input 0 enabled. A fresh RT64 capture
of the same visible records instead had `ring=0, raw=0x72C, masked=0,
bakey=0`, returning 0. The first bounded difference is therefore the
history/ring-selected current word and enabled bit, not the pad.

Classification is `B-ACTION-0A-HISTORY-STATE`. The published `0x800979D8`
fix covers the helper contract, but the state writer remains unproven. No
production change.

## Corrected USER_DEMO_5 re-anchor: camera control (2026-08-24)

Use the correctly paired replay's record join: records 22-24 are matches and
record 25 is the first observed camera-control difference. The Trot-exit writer
also matches in both runtimes (`bainput_enable(0,1)`, `0xFFFFFFBE ->
0xFFFFFFBF`, bit 0 set), and action `0xA` returns 1 in both. This supersedes the
old `0x80097B94 RT=2/AN=0`, `RT97B70=1/AN=0`, RT-only
`0x80110EFC`/`0x801111D0`/`0x800979D0`, and
action-`0xA` causal chain claims; `B-TROT-ENABLE-RECOVERY` is not root.

`camctrl` is sampled from `0x80127708[ps+0x184]` record `+0x1C`. Static
`func_800CA628` gives the relevant writer PC `0x800CA65C`, source2[1]. Record
24 is `293.5294` / `0x4392C3C2`; record 25 is RT64 `293.0478` versus Oracle
`293.4005` / `0x4392B342`. Oracle has the direct writer event; RT64 lacks the
corresponding raw direct-writer capture. First-diff status is therefore
`B-CAMERA-WRITER-OR-SOURCE-INCOMPLETE-JOIN`, not a proven upstream input cause.

The old record-23 p60 `5/0` split and camera-chain causal claims are superseded
for this pairing; recheck p60 only on the true record-24->25 path. Next capture
must instrument RT64 `0x800CA65C` with raw old/source/new bits and source2
inputs. Artifact: `/tmp/p08-demo5-record25-camera-firstdiff-20260824T024500Z/`.
Status: `P08 ACTIVE / MANUAL_VALIDATION_PENDING`; `P09 PENDING / NOT_FORMALLY
OPEN`. No production change.

### Direct-writer arbitration correction (2026-08-24)

The newer CA628 artifact contains the RT64 raw event. At record 25 the first
relevant CA628 is RT64 ordinal `10329` (RA `0x8010FE24`), with old field
`0x4392861E`, source2[1] `0x43922BE8`, and new field `0x43922BE8`; Oracle's
paired event is ordinal `103`, `0x4392B342 -> 0x4392A91A`. This is Case B:
CA628 is the first observed writer, but the field and source are already
divergent at entry. `CA8B4` is only a vector copy and E1A58 preserves the
heading in the later call. The actual writer before CA628 remains uncaptured;
E1A58 is not root.

### Active camera-node heading boundary (2026-08-24 correction)

Keep these four storages separate: `active_node+0x34`,
`active_node+0x3C`, temporary `stack_angles+0x4`, and
`selected_record+0x1C`. The latest artifact watches the third and fourth
storages, not `active_node+0x34`. `func_8010FE34:0x8010FEA8` loads the second;
`func_800EFA4C:0x800EFA5C` stores it into the third; CA628 later copies the
third into the fourth.

The only direct static node+0x3C writer identified in the indexed primary code
is `func_8010FDEC:0x8010FDFC`, called by `plcamera_entrypoint_13` at
`0x808004A0`. The artifact did not capture its effective-address mutation, so
the first causal node+0x3C writer remains unresolved:
`B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`. Latest slot-0 semantic pointers are RT
`0x801C6610` and Oracle `0x801CACB0`; the prior `0x8039D820` pointer belongs to
an older FE14 capture and is not joined to this pair.

### Node+0x3C checkpoint audit correction (2026-08-24)

The later node3C artifact is hash-valid and read-only, but its temporal labels
are not valid semantic node+0x3C evidence. Oracle's raw records 22–26 read
`node=0x8031CF80`, `mode=0`, and `node+0x3C=0x01015590`; later it reads
`0x801CACB0` with the same non-heading field. RT64 has no paired raw node3C
checkpoint lines. The reported post-player MATCH and pre-FE34 DIFF were thus
inferred from the heading pipeline, not direct joined reads. Keep the boundary
as `B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`.

### USER_DEMO_5 EFA4C heading provenance (2026-08-24)

Current operational boundary **before** the register-provenance pass:
`B-EFA4C-HEADING-SOURCE-PROVENANCE-UNRESOLVED`. That pass recaptured USER_DEMO_5
glidx 22–25 and **failed** to rejoin the Luna EFA4C heading pair
(`0x4392861E` vs `0x4392B342`). `func_80110970` / `func_8010FE34` did not
execute. Observed consumer path is `FC38→FE14→CA628`. Classification:
`CASE E - CORRECTED/SUPERSEDED`. Artifact:
`/tmp/p08-demo5-efa4c-heading-provenance-20260824T061436Z/`. Next single
action: independently arbitrate whether the 10970→EFA4C event exists on a
map=296-only join, then promote at most one source/control producer.

### USER_DEMO_5 record24 camera-path conflict arbitration (2026-08-24)

That arbitration is complete but the event join is not: `ROOT_CLASS:
JOIN-UNRESOLVED`. The Grok artifact is immutable and hash-valid, yet its
`sub=5` window collides across attract slices and has two RT remapped-
`glidx=24` series. It lacks the requested record ordinal, 6-byte sequence,
pad/context join, paired call-order table, effective-CA628 table, and
record24/25 state files. RT has wrapper observations for FC38/FE14/CA628 with
`ra=0`; Oracle has no matching semantic lines. Therefore the replacement path
is not proven, and the old FE34/10970/EFA4C claim is only
`CORRECTED/SUPERSEDED`, not replaced by a causal winner.

`CONFLICT_WINNER: NEITHER — NEW CAPTURE REQUIRED`. Do not promote
`B-EFFECTIVE-CA628-SOURCE`; keep all record/input/state fields from this
capture `UNKNOWN` until a map=296-only paired run joins ordinal, sequence,
counter, pad tuple, stage/sub/map, relative call order, and effective CA628
destination/source/old/new bits in both runtimes. This does not alter the
separate `B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED` result.

### Independent map296 join validation and earliest camera boundary (2026-08-24)

The replacement capture resolves replay identity: raw ud5-record streams are 28/28, raw6 is 28/28 bit-perfect, canonical keys have no duplicates, and the single epoch is gated by stage=0, sub=5, map=296, mode=5. D_8012D5B0+0x0 is the demo pointer, D_8012D5B8 is its post-consume record counter, and camera rec_ord can be one greater than the logical record while sharing the same logic tick. func_800151BC remains the proven tick boundary.

The camera sub-join does not support the previously asserted sequence. R24 EFA4C counts are RT=66/AN=73, FE14=1/1, CA628=2/2, and A9318=0/0. Static 0x800A9318 is a separate 0x8010FE34 pass, not a proven exit of 0x800A8E9C; CAMERA_UPDATE_BOUNDARY_VERDICT=REFUTED and R24_CALL_SEQUENCE_MATCH=NO.

Earliest comparable camera state is logical record 0, camera ordinal 1, CA628 call 1 (capture tag rec_ord=1). Old destination bits match (0x4324BF6C), but source bits differ (RT=0x4324B542, AN=0x4324B132). Current boundary: ROOT_CLASS=B-FIRST-CA628-SOURCE. R24 is downstream: both old and source are already different (0x4324A910 vs 0x4324A7B0); output bits remain unknown. This supersedes JOIN-UNRESOLVED for replay identity while retaining old causal chains as CORRECTED/SUPERSEDED; it does not alter B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED.

Derived artifact: /tmp/p08-demo5-first-camera-diff-from-joined-run-20260824T074500Z/; manifest SHA256 d1139defdfa6903041cef31fd2e4db98ca328bd964e702a4e1dca6683a7cff3f. No production change, commit, or push.

### USER_DEMO_5 Record 0 CA628 source provenance & writer trace (2026-08-24)

- **Artifact Directory:** `/tmp/p08-demo5-r0-ca628-source-firstdiff-20260824T144859Z/`
- **Replay State:** `stage=0, sub=5, map=296, mode=5`, 28/28 bit-perfect record join.
- **Initial Map 296 Checkpoint:** `sub+0x84 = 0x43240367` (164.013290), `node+0x50 = 0x3f3c04a3` (0.734446), `node+0x58 = 0x3f800000` (1.000000) bit-identical in BOTH runtimes. Refutes Case E (Map Init).
- **First Divergent Writer:** `func_80116218` at `0x80116380` (`swc1 $f18, 0x4($s1)`).
- **Target Memory Address:** `$sp + 0x2C` (`0x8004567C`, caller stack frame of `func_80110588`).
- **Target Lifetime:** `STACK` (ephemeral stack-allocated vector `sp+0x28`).
- **Values Stored ($f18):** RT64: `0x4324b542` (164.708038) vs Angrylion: `0x4324b132` (164.692169).
- **Downstream Consumer:** `func_800CA628` Call 1 at `0x8010FE1C / 0x8010FE24` copies `src2[1]` (`*(sp+0x2C)`) into `dst+0x1C` (old value matched `0x4324bf6c`).
- **Classification:** `Case A — Writer input diff / upstream angular math evaluation in func_80116218 / func_80116120`.
- **Operational Boundary:** `B-R0-CA628-SOURCE-WRITER-PROVEN`.

### Independent pre-0x80116380 arbitration: first cam_sub+0x84 transition (2026-08-24)

**CURRENT_STATUS:** `P08 ACTIVE / MANUAL_VALIDATION_PENDING`; `P09 NOT FORMALLY OPEN`.

**CURRENT_ROOT_BOUNDARY:** `B-R0-FIRST-CAMSUB84-WRITER-SOURCE` — `func_800EE7F8:0x800EE804` writes persistent `cam_sub+0x84` from `0x80045638`; immediate caller unknown.

**LATEST_PROVEN_FACTS:** 28/28 joined records; `func_80110588` entry base `0x43240367` and node+`0x50` `0x3F3C04A3` match; the earlier `EE7F8` write is RT `0x4324036A` versus AN `0x4324036D`; the `0x80110634` stack copy already carries the difference; node+`0x50` first differs at `0x801161E0`; `0x80116380`/`CA628` are downstream.

**REFUTED_OR_SUPERSEDED_CLAIMS:** Gemini's `80116380` root; prior `B-R0-CA628-SOURCE-WRITER-PROVEN` as root; all older FE34/10970/EFA4C/node+3C roots. Map 296 is Hailfire Peaks — Icy Side / `MAP_128_HP_ICY_SIDE`, not Mayahem; current TOML has no human-readable map table.

**UNRESOLVED_QUESTIONS:** immediate caller/source producer; conditional `0x800893E0` execution/value; complete `func_80116120` input parity.

**ARTIFACT_PATHS:** `/tmp/p08-demo5-r0-pre-80116380-firstdiff-20260824T082500Z/`, joined `/tmp/p08-demo5-map296-semantic-join-20260824T071156Z/`, source `/tmp/p08-demo5-r0-ca628-source-firstdiff-20260824T144859Z/`.

**ARTIFACT_MANIFEST_SHA256:** `6c793196c0457e162bcf6ebd1ff8ed2f4e764f1a72d544cf09232edd0148c29b`.

**WORKTREE_STATE:** intentionally dirty and preserved; documentation-only changes this run; no `RecompiledFuncs`/`src/overlays` changes.

**CODE/BUILD/TEST_STATUS:** no production fix; no build/test; `git diff --check` pending.

**WHAT_WAS_DONE_THIS_RUN:** independent raw/static arbitration plus immutable artifact and handoff update.

**WHAT_REMAINS_PENDING:** exact caller of the earlier persistent `EE7F8` write.

**NEXT_SINGLE_ACTION:** paired Record 0 capture with RA/callsite around that write, `f71`, and post-`EE7F8` stack checkpoint; stop there.

**NEXT_RECOMMENDED_MODEL:** GPT-5.6 Luna High / Terra equivalent, adversarial float provenance.

## USER_DEMO_5 `800EE780 → 800EEBF0 src_dir[1]` arbitration (2026-08-24)

**PROJECT_PHASE:** P08 active causal arbitration; **P08_STATUS:** `ACTIVE / MANUAL_VALIDATION_PENDING`; **P09_STATUS:** `PENDING / NOT_FORMALLY OPEN`.

**CURRENT_HEAD:** `25595a2eb8241ef7ca4aa135e6681e44abcf8598`. **WORKTREE_STATUS:** intentionally dirty state preserved; documentation-only change this round; no `RecompiledFuncs/**` or `src/overlays` change.

**CURRENT_INVESTIGATION:** `USER_DEMO_5`, `stage=0/sub=5/map=296`, `Hailfire Peaks — Icy Side / MAP_128_HP_ICY_SIDE`.

**CURRENT_ROOT_CLASS:** `B-800EEBF0-INPUT` — `PROVEN`.

**CURRENT_ROOT_BOUNDARY:** `func_800EEBF0` consumes `sp+0x34` (`src_dir[0] = pitch`, `src_dir[1] = yaw`) and radius `a2`, producing `sp+0x54`. At glidx 24, its `src_dir[1]` (yaw) is the first divergent input (RT: `0x4391E610` vs AN: `0x43927D8B`), causing its Cartesian output `sp+0x54` to diverge. `func_800EE780` is an elementwise vector adder (`sp+0x48 = sp+0x28 + sp+0x54`) whose input A is bit-identical and whose output divergence is strictly inherited from `sp+0x54`.

**LAST_PROVEN_MATCH:** glidx 23:
- `func_800EEBF0`: RT64 ord=42 <-> ANGRYLION ord=6 (MATCH, 100% bit-exact).
  - `old = (0xC43BDF50, 0x43BEBF32, 0x439FDC90)`, `src_dir = (0x41C84F23, 0x43928610, 0x446158B2)`, `scale = 0x446158D2`, `out = (0xC43BDF66, 0x43BEBF2F, 0x439FDC60)`.
- `func_800EE780`: RT64 ord=374 <-> ANGRYLION ord=454 (MATCH, 100% bit-exact).
  - `old = (0x45818C16, 0x447DB8E3, 0x458225C9)`, `src_a1 = (0x45990800, 0x441E594A, 0x45705000)`, `src_a2 = (0xC43BDF66, 0x43BEBF2F, 0x439FDC60)`, `out = (0x45818C13, 0x447DB8E2, 0x458225C6)`.

**FIRST_PROVEN_DIFF:** glidx 24:
- `func_800EEBF0`: RT64 ord=43 <-> ANGRYLION ord=7.
  - `old = (0xC43CBBD0, 0x43BEC05C, 0x43A01330)` [MATCH].
  - `src_dir[0]` (pitch) = `0x41C79912` (24.949741°) [MATCH].
  - `src_dir[1]` (yaw) = RT: `0x4391E610` (291.797363°) vs AN: `0x43927D8B` (292.980804°) [DIFF].
  - `scale` = `0x44621A8F` (904.414978) [MATCH].
  - `out` = RT: `(0xC43E589D, 0x43BEC05A, 0x43983EF2)` vs AN: `(0xC43CBBB0, 0x43BEC05A, 0x43A01366)` [DIFF].
- `func_800EE780`: RT64 ord=382 <-> ANGRYLION ord=462.
  - `old = (0x45818C16, 0x447DB8E2, 0x458225C9)` [MATCH].
  - `src_a1` = `(0x45992390, 0x441E58B4, 0x4570492C)` [MATCH].
  - `src_a2` = RT: `(0xC43E589D, 0x43BEC05A, 0x43983EF2)` vs AN: `(0xC43CBBB0, 0x43BEC05A, 0x43A01366)` [DIFF from 800EEBF0].
  - `out` = RT: `(0x4581587C, 0x447DB8E1, 0x4581A885)` vs AN: `(0x45818C1A, 0x447DB8E1, 0x458225CC)` [DIFF].

**LATEST_PROVEN_CAUSAL_CHAIN:** `func_800EEBF0(input_yaw DIFF)` → `sp+0x54 (DIFF)` → `func_800EE780(a1 MATCH, a2 DIFF)` → `sp+0x48 (DIFF)`.

**CURRENTLY_DOWNSTREAM_FUNCTIONS:** `func_800EE780` and `func_800EEBF0` are both faithful downstream helpers with identical logic between runtimes.

**REFUTED_OR_SUPERSEDED_CHAINS:** bounded USER_DEMO_4 `979D8 → BCE84 → P60`; generic movement, CA628, FE14, and prior `80116380` camera-root claims remain superseded.

**LATEST_ARTIFACT / CAPTURE_DIR:** `/tmp/p08-demo5-eebf0-ee780-20260824T182550Z/`.
- **MANIFEST_SHA:** `6d1ec2322e9b9022b5600a3c54e74bc2bb09c6454af2773fe91793ad63bc7610`.
- **SHA256_VERIFY:** PASS (verified twice; read-only `0444`/`0555`).

**CURRENT_UNKNOWNS:** upstream evaluation producing return value `$v0` of `func_80097B70` (`bainput_should_rotate_camera_left` calling `func_800979D8` at `0x80097B7C`) at `glidx 23`.

**PENDING_WORK:** GPT-5.6 Luna High causal arbitration of the `func_80112D90` call-presence boundary artifact before advancing upstream.

**NEXT_SINGLE_ACTION:** GPT-5.6 Luna High independently arbitrates the new immutable paired `func_80112D90` call-boundary capture (`/tmp/p08-demo5-80112d90-call-boundary-20260825T040600Z/`) where the call presence difference (`RT64=YES` vs `ANGRYLION=NO`) is proven to originate upstream at branch `0x800A4928` (`beq $v0, $zero`) due to `func_80097B70` returning `1` in RT64 vs `0` in ANGRYLION.

**NEXT_RECOMMENDED_MODEL:** GPT-5.6 Luna High.

**FIX_IMPLEMENTED:** NO. **COMMIT:** NONE. **PUSHED:** NO.

## Current arbitration checkpoint — 2026-08-25 (func_80097B70 & func_800979D8 Direct Measurement glidx 22->23)

`/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/` is the current immutable paired artifact. `sha256sum -c SHA256SUMS` passes for every file and `SHA256SUMS` hashes to `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`.

**RAW_LOG_PROVENANCE:** `NEW_RAW_CAPTURE` (Dual live capture of `TooieRecompiled` and Mupen64Plus `ANGRYLION_CONTROL`).

**Observational Gap Resolution:**
The prior gap where ANGRYLION `func_80097B70` return = 0 was an inference is now **RESOLVED**. Both runtimes directly measured `func_80097B70`, `func_800979D8`, branch `0x800A4928`, and the downstream dispatch chain:
- **`glidx 22`:**
  - `func_800979D8` leave: RT `v0=0` vs AN `v0=0` (**MATCH / PROVEN**).
  - `func_80097B70` leave: RT `v0=0` vs AN `v0=0` (**MATCH / PROVEN**).
  - Branch `0x800A4928`: `v0 == 0`, `taken = 1` in both runtimes (**MATCH / PROVEN**).
  - `func_80112D90`: NOT called in either runtime; `p60+0xC = 0` (**MATCH / PROVEN**).
- **`glidx 23`:**
  - `func_800979D8` leave: RT `v0=1` vs AN `v0=0` (**DIVERGENCE / PROVEN**).
  - `func_80097B70` leave: RT `v0=1` vs AN `v0=0` (**DIVERGENCE / PROVEN**).
  - Branch `0x800A4928`:
    - RT64: `v0 == 1`, `taken = 0` (falls through -> calls `func_80110EFC` -> `func_80110E00` -> `func_801111D0` -> `func_80112D90`, setting `p60+0xC = 1`).
    - ANGRYLION: `v0 == 0`, `taken = 1` (branches to `0x800A4940` -> skips camera rotation dispatch; `p60+0xC` remains `0`).

**Semantic join:** `PROVEN` across both runtimes. The normalized tuple `(stage=0, sub=5, map=296, mode=5, glidx, calling_region=func_800A4878, controlling_branch=0x800A4928, bainput_helper=func_800979D8)` matches across the glidx 22->23 transition.

**CURRENT_ROOT_CLASS:** `B-BAINPUT-HELD-STATE` / `B-ROTATE-CAMERA-LEFT-INPUT`.
**CURRENT_ROOT_BOUNDARY:** `bakey_held(self, 10)` (`0x800919F4`) query inside `func_800979D8` evaluating to `held = 1` in RT64 vs `held = 0` in ANGRYLION at `glidx 23`.

**NEXT_SINGLE_ACTION:** Perform static and mechanical dynamic boundary analysis of `bakey_held` (`0x800919F4`) and `func_800BCE84` for button 10 (C-Left) at `glidx 23`.
**NEXT_RECOMMENDED_MODEL:** Gemini 3.7 Flash High for mechanical boundary instrumentation / GPT-5.6 Luna High for causal arbitration. No fix, commit, or push.
**OPTIONAL_MECHANICAL_MODEL:** DeepSeek V4 Flash.
**OPTIONAL_SECONDARY_MODEL:** Grok 4.6 Medium for narrow timing/register ambiguity.

## Superseding checkpoint — `800979D8` primary-log correction (2026-08-25)

Artifact `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/` is valid;
manifest SHA is `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`
and all `SHA256SUMS` entries pass. `analysis.md` is
**CORRECTED/SUPERSEDED** where it claims `held RT=1 / AN=0`.

Primary logs show at target glidx 23: `bakey_held=1` in both runtimes,
enabled word `0xffffffbf` with bit 0 set in both, and `800979D8` return
`RT=1` versus `AN=0`. `80097B70` is faithful forwarding; this is the first
proven diff. AN lacks a direct `800BCE84` marker for its mapped-byte/table
inputs, so complete external-state parity is not proven and the current
classification is **Case B**, not a promoted `800BCE84` root.

Static: `bakey_held` reads `self+0x40`, `key+0x18+button_id`, and `key+0`,
then calls `800BCE84(state, button_id)` at `0x80091A18`; `800BCE84` reads
`state+0x8+button_id`, calls `800BCCC0`, indexes the returned table, and
returns bit 0. Map/button-word entry fields are trace-only. **CURRENT_ROOT_CLASS:**
`B-800979D8-COMPLETE-READSET-UNKNOWN`. **NEXT_SINGLE_ACTION:** capture the
complete `bakey_held` state and nested `800BCE84` call/args/return at glidx
22/23 in both runtimes; stop at `800BCE84`. **NEXT_RECOMMENDED_MODEL:**
Gemini 3.7 Flash High. No production fix.

## Superseding narrow arbitration — `800979D8` post-`isEnabled` body (2026-08-25)

The artifact is valid (`NEW_RAW_CAPTURE`, manifest
`650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`, all
checksums **PASS**). At semantic glidx 23, `bakey_held=1/1`, enabled bit
0 `1/1`, and final `800979D8` `RT=1/AN=0` are **PROVEN**. The host path
with held and enabled both true has no enable side effect and returns `1`.

## Canonical Checkpoint — `func_800979D8` Exact MIPS Recovery & Static Implementation Mismatch (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/` is valid;
`NEW_RAW_CAPTURE`, manifest SHA is `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`,
and all `SHA256SUMS` entries pass (dual verification).

1. **Exact Original MIPS Body (`0x800979D8 .. 0x80097A4C`) Recovered:**
   - `0x800979E8`: calls `bakey_held(self, button_id)` (`0x800919F4`).
   - `0x800979F0`: `beqz $v0, 0x80097A2C` (if held == 0, jumps to unheld path).
   - `0x800979FC`: calls `bainput_isEnabled(self, input_id)` (`0x80098024`), which reads `*(input + 0x0) & (1 << input_id)`.
   - `0x80097A04`: `bnez $v0, 0x80097A18` (if enabled != 0, skips disable helper).
   - `0x80097A10`: calls `func_80097990(self, input_id, 0)` (when enabled == 0, clears bit in `*(input + 0x4)`).
   - `0x80097A1C`: calls `func_80097978(self, input_id)`, which reads and returns `*(input + 0x4) & (1 << input_id)`!
   - `0x80097A24`: jumps to epilogue `0x80097A40` and returns `$v0` from `func_80097978`.
   - Unheld path (`0x80097A2C`): calls `func_80097990(self, input_id, 1)` (sets bit in `*(input + 0x4)`) and returns `$v0 = 0`.

2. **Root Cause of glidx 23 Divergence:**
   - At `glidx 23`, `bakey_held` returns `1` and `bainput_isEnabled` returns `1` in both runtimes.
   - Original MIPS executes `0x80097A1C: jal 0x80097978`. `func_80097978` queries `*(input + 0x4) = 0xFFFFFFFE` -> bit 0 is 0 -> returns `$v0 = 0`.
   - RT64 host replacement (`tooie_bainput_func_800979D8` in `src/game/boot_hw_stubs.cpp`): incorrectly calls `bainput_isEnabled(self, input_id)` a second time (reading `*(input + 0x0) = 0xFFFFFFBF` -> bit 0 is 1) -> returns `ctx->r2 = 1`.

3. **Classification & Boundaries:**
   - `STATIC_IMPLEMENTATION_MISMATCH_800979D8`: **PROVEN**.
   - `CURRENT_ROOT_CLASS`: `B-800979D8-STATIC-HOST-IMPLEMENTATION-MISMATCH`.
   - `CURRENT_ROOT_BOUNDARY`: Callee selection in `func_800979D8` (`0x80097A1C` calls `func_80097978` on `input+0x4`, not `bainput_isEnabled` on `input+0x0`).
   - `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High independently arbitrate the exact original-vs-host semantic mismatch before any production fix.
   - `NEXT_RECOMMENDED_MODEL`: GPT-5.6 Luna High. No fix, commit, or push.

### Corrected/Superseded — `800979D8` bug versus current `input+0x4` root (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/` is
SHA-valid (manifest `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`; all
checks pass). The original body `0x800979D8..0x80097A4C`, including the
post-`isEnabled` path, is complete and statically proven.

`STATIC_IMPLEMENTATION_MISMATCH_800979D8=PROVEN`: original `80097990` writes
`input+0x4` but host `bainput_enable` writes `input+0x0`; original `80097978`
reads `input+0x4` at `80097984` but the host's second query reads `input+0x0`.
This remains a real production bug and should eventually be fixed.

At glidx23, `input+0=ffffffbf` matches in both runtimes, but `input+4` is
RT64 `ffffffff` (bit0=1) versus Angrylion `fffffffe` (bit0=0). The prior
`COMPLETE_EXTERNAL_INPUTS_MATCH=YES` claim is self-contradictory and
**CORRECTED/SUPERSEDED** to **NO**. glidx22 `ffffffff`/`ffffffff` is the
last proven match; glidx23 is the first proven difference.

With no state change, a corrected host would return RT64 `1`, while original
Angrylion returns `0`; counterfactual parity is **NO**. Hence
`WOULD_FIXING_800979D8_ALONE_RESTORE_GLIDX23_PARITY=NO`.

`PRODUCTION_CORRECTNESS_BUG`: static `800979D8` host mismatch.
`CURRENT_PARITY_CAUSAL_ROOT`: divergent active/triggered `input+0x4` bit.
`CURRENT_ROOT_CLASS`: `B-BAINPUT-ACTIVE-MASK-INPUT-PLUS4-STATE`.
`CURRENT_ROOT_BOUNDARY`: bit0 of `input+0x4` for `input_id=0`, immediately
before `80097A1C -> 80097978` / read PC `80097984`.

`FIX_800979D8_NOW=NO`. `NEXT_SINGLE_ACTION`: statically enumerate immediate
writers/callsites of this bit, starting at known writer `80097990`, then use
one future dual-runtime join of old/new words, source, writer PC, callsite,
call presence, and semantic occurrence. No new capture, upstream walk, or
production edit in this checkpoint. Recommended model: **Gemini 3.7 Flash High**
(DeepSeek V4 Flash for mechanical candidate enumeration; Grok 4.6 Medium only
for ordering ambiguity).

## Canonical Checkpoint — `input+0x4` First Temporal Writer & Host `800979D8` Causal Proof (2026-08-25)

Artifact `/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/` is valid;
`NEW_RAW_CAPTURE`, manifest SHA is `3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`,
all 10 `SHA256SUMS` entries pass (dual verification).

1. **First Temporal Writer Event Identified:**
   - At `glidx 22`, occurrence `func_800979D8(self, button_id=10, input_id=0)`:
     - `held == 1` and `enabled == 0` in BOTH runtimes.
     - `*(input + 0x4)` is `0xFFFFFFFF` (bit 0 = 1) in BOTH runtimes before call.
     - **AN (Original MIPS):** Follows `0x80097A04` branch fallthrough and executes `0x80097A10: jal func_80097990(self, 0, 0)`. `func_80097990` executes CLEAR store at `0x800979CC`, transforming `*(input + 0x4)` from `0xFFFFFFFF` (bit 0 = 1) to `0xFFFFFFFE` (bit 0 = 0).
     - **RT (Host C++ Replacement):** `tooie_bainput_func_800979D8` calls `bainput_enable(self, 0, 0)` at `0x8009817C` (modifying `input + 0x0`) instead of `func_80097990` (modifying `input + 0x4`). Therefore, `*(input + 0x4)` in RT64 is never modified and remains `0xFFFFFFFF` (bit 0 = 1).
   - This single host defect at `glidx 22` is the exact causal producer of the `input+0x4` state divergence.

2. **Downstream Target Divergence at `glidx 23`:**
   - At `glidx 23`, `bainput_enable(self, 0, 1)` updates `*(input + 0x0)` to `0xFFFFFFBF` in both runtimes.
   - When `func_800979D8(self, button_id=10, input_id=0)` is called:
     - `held == 1`, `enabled == 1`.
     - **AN:** Executes `0x80097A1C: jal func_80097978(self, 0)`, reads `*(input + 0x4) = 0xFFFFFFFE` (bit 0 = 0) -> returns `$v0 = 0`. Controlling branch `0x800A4928` branches over camera rotation left (`p60+0xC = 0`).
     - **RT:** Host calls `bainput_isEnabled(self, 0)` on `*(input + 0x0) = 0xFFFFFFBF` (bit 0 = 1) -> returns `ctx->r2 = 1`. Controlling branch `0x800A4928` falls through to `func_80112D90`, setting `p60+0xC = 1` (unwanted camera rotation left).

3. **Classification & Verification:**
   - `FIRST_INPUT_PLUS4_DIFFERING_EVENT`: `glidx 22`, `func_800979D8(self, btn=10, in_id=0)`.
   - `FIRST_INPUT_PLUS4_WRITER_RT`: `NO_WRITE_OBSERVED` (host called `bainput_enable` on `input+0`, omitting write to `input+4`).
   - `FIRST_INPUT_PLUS4_WRITER_AN`: `func_80097990` at `0x800979CC` (CLEAR bit 0).
   - `FIRST_INPUT_PLUS4_WRITER_EVENT_MATCH`: `ABSENT_IN_ONE_RUNTIME`.
   - `FIRST_INPUT_PLUS4_OLD_MATCH`: `YES` (`0xFFFFFFFF`, bit0=1).
   - `FIRST_INPUT_PLUS4_SOURCE_OPERATION_MATCH`: `NO`.
   - `FIRST_INPUT_PLUS4_NEW_MATCH`: `NO` (`0xFFFFFFFE` [bit0=0] vs `0xFFFFFFFF` [bit0=1]).
   - `HOST_800979D8_MISMATCH_CAUSES_INPUT_PLUS4_FIRST_DIFF`: **`PROVEN`**.
   - `CURRENT_ROOT_CLASS`: `B-800979D8-HOST-REPLACEMENT-WRONG-SIDE-EFFECT`.
   - `BAINPUT_INIT_EXECUTES_IN_WINDOW_RT`: `NO`.
   - `BAINPUT_INIT_EXECUTES_IN_WINDOW_AN`: `NO`.
   - `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High arbitrate the first input+4 divergence and the causal proof that the host 800979D8 mismatch produces it.
   - `NEXT_RECOMMENDED_MODEL`: GPT-5.6 Luna High. No fix, commit, or push.

## Final causal root — raw `input+4` writer arbitration (2026-08-25)

**CORRECTED/SUPERSEDES** the prior pending root. Artifact
`/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/` is a valid
`NEW_RAW_CAPTURE`; manifest SHA is
`3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`.

`GLIDX22_800979D8_SEMANTIC_JOIN=PROVEN` for
`stage=0/sub=5/map=296/mode=5`, player semantic self, button 10, input 0;
call presence is YES in both. Held and enabled are `1/1` and `0/0`, and
the old `input+4` is `ffffffff/ffffffff`. The pre-write glidx22 state thus
matches. AN executes `0x80097A10 jal func_80097990` with `a1=0,a2=0` and
changes `input+4` to `fffffffe`; RT calls `bainput_enable` and leaves
`input+4=ffffffff` while writing `input+0`. The post-write state is the
first divergence: `HOST_800979D8_MISMATCH_CAUSES_INPUT_PLUS4_FIRST_DIFF=PROVEN`.

Raw opcode correction: `CLEAR_STORE_PC=0x800979CC` (`ac4c0004`),
`SET_STORE_PC=0x800979B0` (`ac590004`); `80097A10` is the callsite,
`80097A14` its delay slot, and `80097A18` its return address. Any earlier
store-PC or caller-PC labels are **CORRECTED/SUPERSEDED**. At glidx23,
AN reads `input+4` through `func_80097978` and returns 0; RT reads `input+0`
through `bainput_isEnabled` and returns 1, yielding downstream
`80097B70=0/1` and `800A4928=TAKEN/NOT_TAKEN`.

`ROOT_CAUSE_FOUND=YES`, `FIX_IMPLEMENTED=NO`,
`CURRENT_ROOT_CLASS=B-800979D8-HOST-REPLACEMENT-WRONG-SIDE-EFFECT`.
The write-helper and read-helper mismatches are both **PROVEN** and belong
to the same incorrect replacement; fix them together. A query-only glidx23
change is insufficient, while a full faithful replacement is **EXPECTED**
to restore this causal chain (not runtime-tested). Next: production fix plus
dual-runtime A/B validation, followed by broader attract regression checks.

## Post-Fix Validation Checkpoint — `tooie_bainput_func_800979D8` Faithful Fix (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-fix-validation-20260825T235800Z/` is valid;
`NEW_RAW_CAPTURE_POST_FIX`, manifest SHA is `9a977b6e1c4c276dd2b6616008723d4b3c7139b9d1c4bd6c519c2f23ae3373da`,
all 16 `SHA256SUMS` entries pass (dual verification).

- **`ROOT_CAUSE_FOUND:`** `YES`
- **`ROOT_CAUSE_FIX_VALIDATED:`** `YES`
- **`FIX_IMPLEMENTED:`** `YES`
- **`FIX_SCOPE:`** `src/game/boot_hw_stubs.cpp` (`tooie_bainput_func_80097990` on `input+4`, `tooie_bainput_func_80097978` on `input+4`, and corrected `tooie_bainput_func_800979D8`)
- **`HOST_WRITE_HELPER_FIXED:`** `YES`
- **`HOST_READ_HELPER_FIXED:`** `YES`

**Validation Results (100% Parity with ANGRYLION_CONTROL):**
- `GLIDX21_CONTROL_PARITY:` `PASS` (held=0, enabled=1, input+4=0xFFFFFFFF, v0=0)
- `GLIDX22_HELD_PARITY:` `PASS` (held=1/1)
- `GLIDX22_ENABLED_PARITY:` `PASS` (enabled=0/0)
- `GLIDX22_INPUT4_OLD_PARITY:` `PASS` (0xFFFFFFFF/0xFFFFFFFF)
- `GLIDX22_OPERATION_PARITY:` `PASS` (func_80097990 CLEAR store)
- `GLIDX22_INPUT4_NEW_PARITY:` `PASS` (0xFFFFFFFE/0xFFFFFFFE)
- `GLIDX23_INPUT4_PARITY:` `PASS` (0xFFFFFFFE/0xFFFFFFFE)
- `GLIDX23_800979D8_RETURN_PARITY:` `PASS` (v0=0/0)
- `GLIDX23_80097B70_RETURN_PARITY:` `PASS` (v0=0/0)
- `GLIDX23_800A4928_PARITY:` `PASS` (TAKEN/TAKEN, skipping camera rotation left)
- `GLIDX23_80112D90_CALL_PARITY:` `PASS` (NO/NO, camera rotation left completely collapsed)
- `GLIDX23_P60C_PARITY:` `PASS` (p60+0xC = 0/0)
- `POST_FIX_EEBF0_PARITY:` `PASS` (yaw=270.0 in both runtimes)
- `POST_FIX_EE780_PARITY:` `PASS` (a3=0 in glidx 21..23, 3fa21681 in glidx 24, 3fe2ec4e in glidx 25)
- `BTN13_NEARBY_REGRESSION:` `PASS` (input+4 bit1 SET, v0=0 in both runtimes)

**Status & Next Steps:**
- The entire USER_DEMO_5 locomotion/camera divergence chain is **RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX**.
- P08 remains **ACTIVE / MANUAL_VALIDATION_PENDING** pending broader physical validation gates.
- `NEXT_SINGLE_ACTION:` GPT-5.6 Luna High independently arbitrate the post-fix validation artifact and determine the remaining P08 work.
- `NEXT_RECOMMENDED_MODEL:` GPT-5.6 Luna High. No commit/push.

## Arbitragem final pós-fix P08 / USER_DEMO_5 — 2026-08-26 (GPT-5.6 Luna High)

Este bloco **CORRIGE/SUPERSEDE** o checkpoint anterior quanto ao escopo e ao
status; o registro histórico permanece intacto.

Artifact: `/tmp/p08-demo5-800979d8-fix-validation-20260825T235800Z/`;
`MANIFEST_SHA=9a977b6e1c4c276dd2b6616008723d4b3c7139b9d1c4bd6c519c2f23ae3373da`;
checksum integral `SHA256SUMS`: 16/16 `OK`.

`RT_RAW_PROVENANCE: NEW_RAW_CAPTURE_POST_FIX`;
`AN_RAW_PROVENANCE: REFERENCE_ORACLE_CAPTURE_REUSED`;
`REFERENCE_ORACLE_REUSE_VALID: YES`: `oracle.log` coincide byte a byte com
`/tmp/oracle_fresh3.log`, e a linha temporal mostra a Oracle após a
instrumentação e antes do fix. Validade limitada a este artifact.

Veredicto: a substituição host de `800979D8` é semanticamente fiel ao MIPS
(SET/CLEAR em `input+4`, consulta de enabled em `input+0`, query final em
`input+4`, low-5-bit shift e retorno sign-extended). O chain focal
`800979D8 → 80097B70 → 800A4928 → câmera/80112D90 → p60+0xC → yaw/EEBF0/EE780`
tem `USER_DEMO5_TARGET_CHAIN_PARITY: PROVEN`; `GLOBAL_GAME_PARITY: NOT_PROVEN`.

Classificação dos gates: `DIRECTLY_SUPPORTED` para estado held/enabled,
old/new `input+4`, operações, retornos, branch, p60+0xC, yaw, EEBF0/EE780 e
btn13; `DERIVED_BUT_PROVEN` para a junção causal normalizada e ausência do
dispatch de rotação esquerda; `UNSUPPORTED` para parity global, long
playthrough e Windows/controle físico.

`P08_STATUS: ACTIVE / MANUAL_VALIDATION_PENDING`;
`P09_STATUS: PENDING / NOT_FORMALLY_OPEN`;
`USER_DEMO5_STATUS: RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`.
`AUDIO_STARTUP_TICK` e `CHARACTER_EDGE_GRAPHICS_GLITCH` são `P08_RESIDUAL`;
`WIDESCREEN_EDGE_GEOMETRY` é `POST_P08_ENHANCEMENT` por ser conteúdo 4:3;
`LONG_PLAYTHROUGH_VALIDATION` é `MANUAL_VALIDATION_PENDING`;
`WINDOWS_GAMEPAD_VALIDATION` é `BLOCKING_P08 / MANUAL_VALIDATION_PENDING`.

Remaining P08: checklist Windows/controle físico (bloqueador), long
playthrough/regressão ampla e triagem separada de áudio/gráficos. O fix focal
está `CURRENT_FIX_READY_FOR_COMMIT: YES`, mas o worktree não é um commit único
limpo por conter instrumentação/diagnóstico e docs históricos. Artifact `/tmp`
não é commitável; `BUILD_THIS_ROUND: NOT_RUN`; nenhum commit/push foi feito.

`NEXT_SINGLE_ACTION: executar e registrar a validação formal Windows + controle
físico antes de abrir P09.`
