---
title: "Handoff — TooieRecomp"
description: "P08 implementation substantially complete (Verdict B; P08 ACTIVE / MANUAL_VALIDATION_PENDING). T07/T08 live integration reconciled and tested. Physical controller & multi-monitor validation pending."
status: current
type: handoff
project: TooieRecomp
updated: 2026-08-16
tags: [tooie-recomp, handoff, p08, intro, attract, demo2, sigsegv, bsrest, baflameline, slot7, lifecycle, bcf28, 80013a7c, visible-demo, matrix-fix, transform-hooks, input, stick-angle, window, error-handling]
---

# Handoff — TooieRecomp

## Prioridade atual

**PHASE P08 IMPLEMENTATION STATUS (Verdict B — ACTIVE / MANUAL_VALIDATION_PENDING, 2026-08-16):**
- **Canonical Task Audit**:
  - `P08-T01` (RT64 / Vulkan): **DONE**
  - `P08-T02` (Catalogar display lists/efeitos; scheduler DP/VI): **DONE** ([[plan/evidence/p08-display-effect-catalog]])
  - `P08-T03` (4:3 e timing originais): **DONE**
  - `P08-T04` (Path SDL/RecompFrontend): **DONE**
  - `P08-T05` (Defaults/persistência): **DONE**
  - `P08-T06` (Esconder launcher no auto-start): **DONE**
  - `P08-T07` (Janela, fullscreen, foco, multi-monitor, escalas): **IMPLEMENTED_MANUAL_VALIDATION_PENDING** (`tooie_window` helpers active and tested, reference SDL/RT64 path handles fullscreen/focus/display, physical hardware display validation pending).
  - `P08-T08` (Fallback/mensagem para driver/API incompatível): **DONE** (`tooie_error` reporting active, `assert(false)` eliminated, negative tests passing).
  - `P08-T09` (Evidência visual intro/first demo): **DONE**
  - `P08-T10` (Input title/file select): **DONE**
  - `P08-T11` (Máquina de estados / C2AB8 / BCF28 / RA wrappers / matrix split): **DONE**
- **Pending for Formal Gate Sign-off**: Physical gamepad hardware verification on physical controller devices and live multi-monitor physical migration. Do NOT open P09 yet.
- See [[plan/evidence/p08-t07-t08-frontend-system-validation]] for complete canonical audit matrix.

**ATTRACT DEMO2 CRASH RESOLVED (2026-08-16):** deterministic attract-loop SIGSEGV
after DEMO2 is RESOLVED and VALIDATED. `INTRO → Wooded Hollow 335 → TDL 278 → CCL 314 → CS Intro 395 → INTRO → Mayahem 235 → JRL 296`
succeed cleanly. `bsrest_entrypoint_11` calls `static_344_808003FC` directly without
LOOKUP_FUNC collision. Level A fix validated across unit tests, static contract, and 300s soak.
See [[plan/evidence/p08-attract-loop-after-demo2-crash]].

**P08 ACTIVE INTEGRATION STATE:** PATCH 4 is accepted and default ON. The
`func_800BCF28` binding fix is **ACCEPTED (verdict B)**. The
`func_80013A7C` lookup-zero is **RESOLVED**
([[plan/evidence/p08-core1-ra-wrapper-fix]]). **Visible attract / gameplay
demo is reproduced** (mode-5, map 335 Wooded Hollow). The **Tooie render-submission
matrix fix is RESOLVED and VALIDATED** ([[plan/evidence/p08-render-submission-matrix-fix]]):
`func_800E44FC`, `func_800E4640`, and `func_800E4628` native overrides split
the projection matrix submission into `LOAD P` followed by `MUL V_view`.
**Title Screen / File Selection Input (P08-T10)** is **VALIDATED and COMPLETE**
for frontend input ([[plan/evidence/p08-title-file-select-validation]]):
intro -> file select -> slot navigation -> `CONT_A` confirm is PASS.
Post-confirm destination is `MAP_A1_CS_INTRO` (161) / `mode=3`. The
"direct Spiral Mountain gameplay" claim is **CORRECTED/SUPERSEDED**.
The later synthetic `mode=1` FAIL is **REFUTED** (sequencer never injects
the START presses that skip the opening story cutscenes).
**Original 4:3 / Timing Baseline (P08-T03)** is **VALIDATED and COMPLETE**
([[plan/evidence/p08-original-timing-4x3-baseline]]): 60.0 Hz guest VI, 20/30 Hz logic,
30 Hz animation, host decouple (~935 Hz), and 4:3 content aspect confirmed under clean baseline config.
**Gameplay Demo Motion Synchronization (2026-08-16, MEASURED_ORIGINAL)**:
Full 765-sample Wooded Hollow clip compared against official mupen64plus Pure Interpreter ([[plan/evidence/p08-demo-full-motion-sync-diagnosis]] §9). Pad identity 100%. Same-index ~25u player lead is one physics step (recomp dumps at glrecord consume; original at VI). After +1 shift, first walk / analog / shot-stop match. Prior 45.11° camera-lag mismatch **CORRECTED/SUPERSEDED** (aligned lag 36.77° both). Remaining discriminator: post-egg run/jump after ~392. Level C (NO FIX).

Historical context:
> **PATCH 4 (2026-08-13):** Tooie-scoped startup ON;
> `ULTRAMODERN_GUEST_PREEMPTION=0` rollback. Regen real (4298 safepoints).
> ON: C2AB8 retorna, áudio avança, crash em `func_800BCF28`.
> OFF: freeze antigo (d/audio/ovl estáveis ≥58s). Sem yield/clear.
> Evidência: [[plan/evidence/p08-preemption-patch4-validation]].

Handoff técnico: [[plan/evidence/p08-session-handoff]].

## P08 forced-facing independent review (2026-08-16)

The earlier Level B diagnosis of the Wooded Hollow `chdiggerground` return
actor is **CORRECTED/SUPERSEDED** as a proven recompilation defect. Original
MIPS case 11 matches generated instruction-for-instruction: it advances the
return position while preserving yaw/`unk54`. The 180° observation is
therefore original actor/state semantics; no fix was implemented.

See [[plan/evidence/p08-forced-cinematic-facing-independent-review]].
`USER_BUG_B_THIS_ACTOR=REFUTED`; `USER_BUG_B_GLOBAL=OPEN`; `ROOT_CAUSE_LEVEL=R`.

## P08 forced-facing node discriminator (2026-08-16, DeepSeek V4 Flash Free)

The selected-call discriminator is now RESOLVED via a this-run dynamic
capture of `func_80802760_sucoaster` (strong-symbol override, env-gated,
reverted before checkpoint):

```text
FUNC_80802760_RETURNS:  0 (node index 0) on all 3438 calls
NODE_CASE:              B_LIVE_NODE — A_KEEP_OLD (-1) REFUTED (zero)
RETURN_LEG_HANDLER:     state 0x98bb1942 -> dispatch case 11 (u72=0x3008,
                        L_80800A3C) -> moves digger back WITHOUT writing
                        unk54/rotation.y
ORIGINAL_CASE11_WRITES_YAW: NO
ORIGINAL_CASE11_WRITES_UNK54: NO
GENERATED_CASE11_MATCHES_ORIGINAL: YES
ORIGINAL_REVERSE_WALKS_BACKWARD: YES
ORIGINAL_RETURN_STATE_DISPATCH_MATCHES_RECOMP: YES
DEEPSEEK_GENERATED_MATCH_CLAIM: CORRECTED/SUPERSEDED
FUNC_80802760_GENERATED_MATCH_ORIGINAL: UNKNOWN (not compared here)
FIRST_OBSERVED_MECHANISM: case-11 return movement with preserved yaw/unk54
ORIGINAL_MIPS_INTENT:   CASE11 PROVEN; func_80802760 equivalence UNKNOWN
FIX_IMPLEMENTED:        NO
MARKDOWN_VAULT_AUDIT:   PASS_WITH_PREEXISTING_WARNINGS (errors=0; 50 warnings)
```

## P08 unnamed same-section JAL fix — independent acceptance (2026-08-16)

The GPT-5.6 Luna X-HIGH review accepts the N64Recomp fix with a non-blocking
runtime-environment caveat. Implementation history above is preserved; the only
code change made during review was hardening the static regression test.

```text
BASE_HEAD:                                  1f2aba14665c064ce5af829955f1ac089792de77
PATCH_BASE_CORRECT:                         YES
PATCH_HASH_CORRECT:                         YES
UNNAMED_SAME_SECTION_CREATESTATIC:          PASS
CROSS_SECTION_POLICY_PRESERVED:             YES
NON_RELOCATABLE_POLICY_PRESERVED:           YES
HOST_HOOK_POLICY_PRESERVED:                 YES
CREATESTATIC_BOUNDARIES:                    7/7 PASS
BSREST_808003FC_VERDICT:                    A
STATIC_HELPER_SPOTCHECK_1:                  8080020C PASS
STATIC_HELPER_SPOTCHECK_2:                  8080032C PASS
TOTAL_AUDITED_CALLS:                        17
DIRECT_CORRECT_STATIC:                      17
WRONG_STATIC:                               0
CALL_MISSING:                               0
LOOKUP_REMAINS:                             0
STATIC_TEST_STRENGTH:                       B
STATIC_TEST_FALSE_PASS_RISK:                LOW
TEST_STRENGTHENED:                          YES
GENERATOR_POLICY_TEST_EXECUTED:             YES
GENERATOR_POLICY_CASES:                     4/4
CUMULATIVE_PATCH_PRIOR_CONTENT_PRESERVED:   YES
UNRELATED_GENERATOR_SEMANTICS_CHANGED:     NO
REGENERATED_LOCAL_BODIES:                  7/7
REGENERATED_DIRECT_CALLS:                  17/17
REGENERATED_VULNERABLE_LOOKUPS:            0
ATTRACT_STATIC_TEST:                        PASS
STICK_ANGLE_TEST:                           PASS
MATRIX_SUBMISSION_TEST:                    PASS
INPUT_DIRECTION_TEST:                      PASS
BCF28_TEST:                                 PASS
CORE1_RA_TEST:                              PASS
TOOIE_BUILD:                                PASS
CCL_REACHED:                                NO (independent short run; prior soak YES)
BSREST_EP11_REACHED:                        NO (not reached in short run)
CORRECT_LOCAL_TARGET:                       UNKNOWN (not reached in short run)
BAFLAMELINE_WRONG_BIND:                    UNKNOWN (not reached in short run)
INTRO_395_REACHED:                          NO (not reached in short run)
EXIT_139:                                   NO
FIX_VERDICT:                                B
FIX_ACCEPTED:                               YES
ATTRACT_DEMO2_CRASH:                        RESOLVED
MARKDOWN_VAULT_AUDIT:                      PASS (0 errors; pre-existing warnings/tasks)
COMMIT:                                     test checkpoint + docs checkpoint
PUSHED:                                     YES (scheduled origin/main push after checkpoints)
NEXT_SINGLE_ACTION:                         return to the next P08 residual
```

The `keep old yaw via -1` hypothesis is REFUTED. The original-MIPS case-11
comparison proves that the 180° freeze is original return-leg behavior for
this actor. No fix is authorized; the broader USER_BUG_B_GLOBAL remains OPEN.

## NEXT

1. **P08 Status: ACTIVE / MANUAL_VALIDATION_PENDING (Verdict B).**
   - Canonical task audit completed: T01, T03..T06, T08..T11 DONE; T02 display catalog DONE; T07 IMPLEMENTED_MANUAL_VALIDATION_PENDING.
   - Next residual after the original-runtime differential: walk only the post-egg run/jump split (~glidx 392).
   - Windowing, aspect ratio calculations, and display validation helpers implemented in `tooie_window`; fullscreen/focus/display event handling confirmed on reference SDL/RT64 path.
   - Graphics and startup error reporting verified via `tooie_error` without crash or unhandled aborts.
2. **Pending Before Formal P08 Close / P09 Open:**
   - Physical gamepad testing across real physical controllers (USB/Bluetooth N64/Xbox/PlayStation).
   - Live physical multi-monitor display switching and desktop fullscreen visual validation.
3. **P08 / P09 Invariants (Protected):**
   - Closed in P08: RT64 presentation baseline, input mapping, window/scale integration, graphics/driver error handling, launcher preview, static call correctness.
   - Do NOT reopen: attract DEMO2 crash, bsrest 0x808003FC, N64Recomp CreateStatic fix, first-straight ~510, fire-egg miss, cinematic facing, stick-angle 13B70, render-submission matrix, BCF28, core1 RA wrappers, PATCH 4 / preemption, P08-T10, original timing/4:3 baseline. Do not modify shared RT64. PATCH 4 stays ON.

## Estado do produto validado

1. Launcher/RT64/Vulkan e auto-start OK.
2. Música inicial OK.
3. Intro 3D + logo + Jiggy OK.
4. Attract request + type-4 complete + warp consumido OK.
5. C2AB8 behavior and delivery resolved (clean-built same-binary CLASS A;
   clean tracked Clang build PASS; PATCH 4 accepted by verdict B).
6. **`func_800BCF28` SIGSEGV RESOLVED** and independently accepted (verdict B;
   [[plan/evidence/p08-bcf28-binding-fix-independent-review]]).
7. First gameplay demo **VISIBLE** after `func_80013A7C` lookup-zero
   **RESOLVED** ([[plan/evidence/p08-core1-ra-wrapper-fix]],
   [[plan/evidence/p08-visible-demo-movement-diagnosis]]).
8. **Actor vs Camera Transform Discrimination (Level B):** Character
   `Actor.rotation` and CPU model matrix are proven upright and correct;
   divergence isolated to `RENDER_SUBMISSION`
   ([[plan/evidence/p08-actor-camera-transform-discriminator]]).
9. **Render-submission matrix fix (Level A, VALIDATED):** Incremental P LOAD +
   V MUL split implemented in `src/game/transform_hooks.cpp`
   ([[plan/evidence/p08-render-submission-matrix-fix]]). Characters and camera
   render upright and elevated; Wooded Hollow attract demo runs continuously for 75s.
10. **Title Screen / File Selection Input VALIDATED (P08-T10 COMPLETE):**
    Frontend input through `CONT_A` confirm is PASS. Direct Spiral
    Mountain / current `mode=1` regression claims are
    **CORRECTED/SUPERSEDED** / **REFUTED**
    ([[plan/evidence/p08-title-file-select-validation]]).
11. **Original timing / 4:3 baseline VALIDATED (P08-T03 COMPLETE):**
    60.0 Hz VI, 20/30 Hz logic, 30 Hz animation, host presentation decoupled, and 4:3 content aspect confirmed
    ([[plan/evidence/p08-original-timing-4x3-baseline]]).
12. **`func_80013B70` stick-angle secondary entry accepted at function level**
    ([[plan/evidence/p08-stick-angle-thunk-fix-rereview]]). Manual WASD
    runtime validation is **RESOLVED** (no more directional collapse).
    Full analog/gamepad remains **PENDING**. Gameplay-demo movement now
    travels in the broadly correct directions because that clip is
    recorded-pad playback through bastick
    ([[plan/evidence/p08-demo-movement-precision-diagnosis]]).
    **Post-physics velocity writer discriminator COMPLETE** (Grok 4.6 HIGH):
    510 is original slope-scaled physics integration (`func_800EF04C` from
    `func_8009AE08`); the post-physics-writer hypothesis is **REFUTED**;
    first-straight internal fidelity **PASS**. Attract/cinematic facing
    is the historical USER_BUG_B diagnosis, now corrected: this actor is
    refuted by original-MIPS review (global USER_BUG_B remains OPEN;
    [[plan/evidence/p08-forced-cinematic-facing-diagnosis]]).

## Arquitetura e regras

TooieRecomp: `N64Recomp` + `N64ModernRuntime` + `RT64` + `RecompFrontend`.
Não editar `RecompiledFuncs` gerado. Repo Git: `TooieRecomp/`, branch `main`.

## Resolved blockers / do not reopen

- Overlay data relocation / `LOOKUP_FUNC(0)`
- Actor manager artificial spawn
- Resident overlay / gcsection
- Scheduler queue (sem nova prova)
- IPL3 integrity gate
- R_MIPS_26 / do_dma
- Type-4 incomplete / warp gated (clean freeze)
- `func_80026500` as hang PC
- `func_800A907C → func_8001EE34` as object driver (refutado)
- **Freeze de C2AB8 como blocker terminal** — superado causalmente e agora
  **permanentemente** (PATCH 4 CLASS A)
- **`func_800BCF28` SIGSEGV / `0x800BCCC0` gap stub** — RESOLVED and
  independently accepted (verdict B) by
  [[plan/evidence/p08-bcf28-binding-fix-independent-review]]
- **`func_80013A7C` LOOKUP_FUNC(0) / core1 RA wrappers** — RESOLVED
  ([[plan/evidence/p08-core1-ra-wrapper-fix]])
- **Actor.rotation as root cause of supine pose** — REFUTED (Actor.rotation is upright)
- **Tooie render submission matrix split** — RESOLVED ([[plan/evidence/p08-render-submission-matrix-fix]])
- **Title/file-select pending validation** — CORRECTED/SUPERSEDED; P08-T10 is
  VALIDATED / COMPLETE ([[plan/evidence/p08-title-file-select-validation]])

## Current expected flow

```text
intro -> Jiggy -> gsattract -> map 335/entrance 8 -> type-4 complete
 -> warp 3 consumed -> A9924 -> A76F4(5) -> mode 5 -> A9A14 -> gsattract ep1
 -> gameplay demo
```

Observed: C2AB8 crossed; `func_800BCF28` completes; `func_80013A7C`
enters at the native strong symbol; visible mode-5 Wooded Hollow attract demo
renders correctly with upright characters and elevated camera, progressing for 75s.

## Exonerated (current)

Frame-sync, event-3, cothemedll, type-4 stall, warp non-consumption,
`func_80026500` hang, scheduler corruption (not proven), `func_800BCF28` binding,
`func_80013A7C` lookup-zero, `Actor.rotation` simulation state, render-submission matrix split.

## Files to read

0. [[plan/evidence/p08-attract-loop-after-demo2-crash]] (**RESOLVED and ACCEPTED** — DEMO2 attract SIGSEGV)
0. [[plan/evidence/p08-demo-movement-precision-diagnosis]] (first demo shot miss §16; preserved)
0a. [[plan/evidence/p08-forced-cinematic-facing-diagnosis]] (USER_BUG_B historical; currently not reproducible)
0a. [[plan/evidence/p08-stick-angle-thunk-fix]] (accepted stick-angle secondary-entry fix)
0a. [[plan/evidence/p08-stick-angle-thunk-fix-rereview]] (**CURRENT REVIEW** — verdict A; `$r1` blocker resolved)
0aa. [[plan/evidence/p08-stick-angle-thunk-fix-independent-review]] (historical verdict C; blocker resolved)
0b. [[plan/evidence/p08-player-direction-facing-diagnosis]] (Level A diagnosis; fix implemented)
0c. [[plan/evidence/p08-directional-input-movement-diagnosis]] (host→pad bidirectional; not reopened)
1. [[plan/evidence/p08-render-submission-matrix-fix]] (**CURRENT** — render matrix submission fix)
2. [[plan/evidence/p08-render-submission-matrix-diagnosis]] (render matrix diagnosis)
3. [[plan/evidence/p08-actor-camera-transform-discriminator]] (transform discriminator)
4. [[plan/evidence/p08-visible-demo-movement-diagnosis]] (visible demo + movement diagnosis)
4. [[plan/evidence/p08-core1-ra-wrapper-fix]] (RA-wrapper fix; resolved)
5. [[plan/evidence/p08-80013a7c-lookup-zero-diagnosis]] (Level A diagnosis; resolved)
6. [[plan/evidence/p08-bcf28-binding-fix-independent-review]] (verdict B; BCF28 fix accepted)
7. [[plan/evidence/p08-bcf28-binding-fix]] (implementer binding-fix evidence)
8. [[plan/evidence/p08-bcf28-crash-diagnosis]] (root cause A diagnosis)
9. [[plan/evidence/p08-preemption-patch4-clean-build-independent-review]] (verdict B; PATCH 4 accepted)
10. [[plan/evidence/p08-preemption-patch4-clean-build-correction]] (implementer correction)
11. [[plan/evidence/p08-preemption-patch4-independent-review]] (historical verdict C preserved; finding resolved)
12. [[plan/evidence/p08-preemption-patch4-validation]] (implementer behavioral evidence)
13. [[plan/evidence/p08-original-timing-4x3-baseline]] (P08-T03 current blocker)
14. [[plan/evidence/p08-session-handoff]]
15. [[plans/BT/08-rt64-input-and-frontend]]

## P08 unnamed overlay JAL fix & validation (2026-08-16, Gemini 3.7 Flash HIGH)

The attract DEMO2 SIGSEGV is **RESOLVED and VALIDATED** (Level A).
The root cause was N64Recomp falling back to global lookup for unnamed same-section
overlay targets, binding `bsrest_entrypoint_11` (`0x80800668`) to `baflameline_entrypoint_5`
at `0x808003FC`.

**Fix:** In N64Recomp `resolve_jal`, relocatable same-section targets take `CreateStatic`
before the lookup-all fallback.
- All 7 audited `.bsrest` targets generated as local static functions (`static_344_80800xxx`).
- 17 risky lookups reduced to 0.
- `tests/attract_demo2_transition_jal_test.py` PASS.
- 300s autonomous soak with Vulkan confirmed clean progression through CCL (314), CS Intro (395), and subsequent attract demos without SIGSEGV (`EXIT_139=NO`).

```text
FIX_OWNER: N64RECOMP
FIX_IMPLEMENTED: YES
FIX_VALIDATED: YES
PATCH_SHA256: 1f2ce7c0d9708df1024a332c1a162049158e368a4e86b747740bb1ccb4236034
VULNERABLE_LOOKUPS_AFTER: 0
NEXT_SINGLE_ACTION: independent review of the generator fix if all acceptance gates pass; otherwise exactly the failing discriminator.
```

## P08 post-egg motion divergence (2026-08-17, Grok 4.6 HIGH)

The independent phase-corrected differential is now recorded. No production
fix was made and P08 remains active.

```text
PHASE_CORRECTED_ALIGNMENT:        ORIGINAL[N] vs RECOMP[N+1]
CONTROL_REGION:                  xz median 0.0224, yaw median 0.0000°
PAD_MATCH_SELECTED_WINDOW:       765/765
FIRST_EXACT_DIFFERENCE:           glidx 385 ideal_yaw (99.38 vs 99.31)
FIRST_MEANINGFUL_DIVERGENCE:      glidx 392 position XZ 2.997 units
FIRST_BAD_FIELD:                  ideal_yaw
FIRST_BAD_STAGE:                  bastick / yaw-intention path
STATE_SEQUENCE_DIFFERENCE:        only capture-boundary bs4 at 355; persistent
                                  post-egg bs21 and jump bs8 sequence matches
TSPD_MULTIPLIER:                  700/700; 1.0000/1.0000
DRIFT_CLASS:                      TURN_DRIFT -> JUMP_TRAJECTORY_DRIFT
ORIGINAL_FIRST_WRITER_PC:         UNKNOWN
RECOMP_FIRST_WRITER_PC:           UNKNOWN
ROOT_CAUSE_LEVEL:                 C
FIX_IMPLEMENTED:                  NO
```

The exact writer is the only remaining discriminator. The camera is downstream,
the digger scan is UNKNOWN, and this split occurs after the first egg input was
already consumed/fired; do not reopen the first shot or closed capture-phase
movement evidence. Full table: [[plan/evidence/p08-demo-post-egg-motion-divergence]].

## P08 ideal-yaw writer differential — RESOLVED (2026-08-17)

The exact ideal_yaw writer and its input provenance are now PROVEN by a bounded
original/Recomp store trace
([[plan/evidence/p08-demo-ideal-yaw-writer-trace]]). The original
Pure-Interpreter build was rediscovered at `/tmp/p08-m64p` (core `6dca4c1`,
rice `038882d`) and rebuilt locally (no sudo) with a narrow store tracer;
the Recomp writer was captured via a GDB breakpoint on `yaw_setIdeal`.

```text
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
INITIAL_0P07_DELTA_CLASS:         CAUSAL_WRONG_VALUE (writer input differs)
EARLY_TURN_STORE_TRACE_CONTROL:   PASS (glidx 74..91 delta 0.0000)
ROOT_CAUSE_LEVEL:                 B (identical writer; stick path refuted;
                                  camera-angle writer instruction remains)
FIX_IMPLEMENTED:                  NO
```

The stick path (`bastick_update → func_80013B70`) is REFUTED as the first
divergent input. The remaining discriminator is the camera-control computation
that first writes the divergent player-control camera angle. No production
change is authorized.
