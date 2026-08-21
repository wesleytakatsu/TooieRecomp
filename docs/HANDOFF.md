---
title: "Handoff — TooieRecomp"
description: "P08 remains ACTIVE / MANUAL_VALIDATION_PENDING. USER_DEMO_4 bind-tick round.w.s and idle follow-cam glidx 1..38 now match ANGRYLION_CONTROL; dump glidx 39 ncbapivot output and BLUE-vs-FIRE egg remain. Only formal P08 gate is PHYSICAL_GAMEPAD_VALIDATION."
status: current
type: handoff
project: TooieRecomp
updated: 2026-08-21
tags: [tooie-recomp, handoff, p08, intro, attract, demo2, sigsegv, bsrest, baflameline, slot7, lifecycle, bcf28, 80013a7c, visible-demo, matrix-fix, transform-hooks, input, stick-angle, window, error-handling]
---

# Handoff — TooieRecomp

## Prioridade atual

**PHASE P08 IS ACTIVE / MANUAL_VALIDATION_PENDING; PHASE P09 IS PENDING / NOT_FORMALLY_OPEN (2026-08-21):**
- **Physical Display Validation**: **PASS** — User explicitly confirmed all 14 mandatory display/window integration tests passed (`PHYSICAL_DISPLAY_RESULT: PASS`, `FORMAL_DISPLAY_CHECKLIST_PROVEN: YES`).
- **Physical Gamepad Status**: **PENDING_HARDWARE_UNAVAILABLE** — Formal 15/15 gamepad checklist cannot be executed because controller hardware is temporarily unavailable. `USER_CONFIRMED_CONTROLLER_GAMEPLAY: YES` retained.
- **Only Remaining P08 Gate**: **PHYSICAL_GAMEPAD_VALIDATION**.
- **Player & AI Movement Speed Parity Baseline (2026-08-21)**: [[plan/evidence/p08-provisional-player-ai-speed-parity]]. `PLAYER_SCALAR_SPEED_PARITY: PASS`, `AI_ACTOR_SPEED_PARITY: PASS`. Speed magnitude matches within sub-pixel floating point bounds across straight walk (`510.2442 u/s`), trot run (`673.5218 u/s`), jump (`690.0 u/s`), and AI actors (`chdiggerground` path, `subaddie` patrol). USER_DEMO_4 speed magnitude matches (`0.0017..2.87 u/s`), proving `CAMERA_DIRECTION_DEFECT_WITH_SPEED_PARITY` (the observed trajectory split is downstream of follow-camera direction, NOT scalar speed). Historical downhill ~510 speed is `PROVEN_CLEAN` against original N64 hardware.
- **Attract numbering / egg discriminator (2026-08-21)**: user demos are **continuous across intro blocks**, not zero-based Demo 0..3. Canonical evidence: [[plan/evidence/p08-provisional-follow-camera-and-egg-differential]].
  - `USER_DEMO_4` = first gameplay after the first return to intro/music = map **235 Haunted Cavern** (`stage=0 sub=4`), **not** "Demo 3".
  - `EGG_PARITY_DEFECT: PROVEN` — Angrylion fires **FIRE** (`cheggfire` 535); RT64 fires **BLUE** (`cheggnormal` 539). User confirmed Angrylion RDP Plus nightly-build visual PASS for fire egg and for the previously-broken demo behavior.
  - Follow-camera on USER_DEMO_4 is **REPRODUCED**: pad 877/877 match. Bind-tick yaw sincos is now **POST-FIX MATCH** vs ANGRYLION_CONTROL (`006d5328` / `bf4b29cc` / `sp+0x48`=`node+0x74`=`c43efa3b,43c49665,c4a6b798`). Idle CA8B4/CA628 glidx 1..38 is now **bit-identical**. First remaining bit difference is dump glidx 39 `ncbapivot_entrypoint_3` output (player pos/yaw/ideal MATCH). Demo-sync still prints 0.0853° @41 → 12.0481° @50 → 112.6789° @100. Canonical: [[plan/evidence/p08-user-demo4-post-round-full-state-discriminator]].
  - `EGG_PARITY_DEFECT` remains **PROVEN**: Angrylion **FIRE** (`cheggfire` 535); RT64 **BLUE** (`cheggnormal` 539). Independent of ROUND.W.S: **YES**. Independent of remaining camera/demo split: **UNKNOWN**. No egg fix.
  - b042865 `HAUNTED_CAVERN_NO_FIRE_EVENT` is **REFUTED**. `GLOBAL_VISUAL_DRIFT_STATUS=NOT_REPRODUCED_IN_ATTRACT_SET` is **CORRECTED/SUPERSEDED** for USER_DEMO_4. Demo 0..3 remains an incomplete attract slice.
- **ROUND.W.S checkpoint (2026-08-21)**: `5854872c79a7cab1c9395ddcd173cece449ffe11` (`fix: preserve MIPS round.w.s semantics`). `ROUND_W_S_ROOT_CAUSE: PROVEN`. `ORIGINAL_BIND_TICK_DISCRIMINATOR: RESOLVED`. `ROUND_W_S_BIND_AND_IDLE_CAMERA_EFFECT: FIXED`. `CAMERA_COMPLETE_FIX: NO`. `CAMERA_FEEDBACK_ACCUMULATION: STILL_PRESENT`. `ROUND_W_S_SPECIAL_VALUE_CONTRACT: PARTIAL`. Evidence: [[plan/evidence/p08-user-demo4-sincos-semantics-fix]].
- **Agent workspace (2026-08-21)**: open CLI/IDE in **TooieRecomp/**. MCP configs live here and bind markdown-vault to `docs/` ([[MARKDOWN_VAULT_TOOLS]]). Do not start agents at `N64-Recomp/`. `BanjoRecomp` and the cart ROM remain siblings for CMake/oracle only.
- **ANGRYLION_CONTROL location (2026-08-21)**: `tools/oracle/angrylion-control/` (`work/` gitignored). Playable test: `./run-tooie-angrylion-rdp.sh`. RT64 port: `./run-tooie.sh`. Spec: [[specs/BT/angrylion-control-oracle]]. Angrylion remains measurement-only; TooieRecomp stays RT64.
- **Next Single Action**: Instruction-join `ncbapivot_entrypoint_3` at dump glidx 39 (first walking-player follow tick). Do not reopen bind-tick `round.w.s`. P09 remains **NOT_FORMALLY_OPEN**.

**ATTRACT DEMO2 CRASH RESOLVED (2026-08-16):** deterministic attract-loop SIGSEGV
after DEMO2 is RESOLVED and VALIDATED. `INTRO → Wooded Hollow 335 → TDL 278 → CCL 314 → CS Intro 395 → INTRO → Haunted Cavern 235 → MAP_296`
succeed cleanly (historical soak labeled 235 as Mayahem; that map-name is **CORRECTED/SUPERSEDED** to WW Haunted Cavern). `bsrest_entrypoint_11` calls `static_344_808003FC` directly without
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
**Gameplay Demo Motion Synchronization & Camera Clearance Ray Differential (2026-08-17, MEASURED_ORIGINAL)**:
- Full Wooded Hollow attract demo motion investigated under synchronized original-vs-recomp instrumentation ([[plan/evidence/p08-demo-camera-eye-producer-differential]], [[plan/evidence/p08-demo-ncbapivot-branch-predicate]], [[plan/evidence/p08-demo-camera-clearance-rays]]).
- **CORRECTED/SUPERSEDED (independent Sol review, 2026-08-18)**:
  - In `func_80113D30`, all four query inputs (`start`, `direction`, `destination`) are **NUMERICALLY EQUIVALENT** between Original and Recomp at glidx 384 (position Δ < 0.05, direction IEEE-754 bit patterns match to within 1 LSB).
  - In Original N64, `func_800E8918` returns `0` for all four selected records, so all four counters `word_3C[0..3]` exceed 4, yielding `half_0C = 4`; this return is projection state, not a collision result.
  - In Recomp, `func_800E8918` returns nonzero projection state for **Q1** and **Q2**, resetting their counters to `0` and resulting in `half_0C = 1`.
  - First differing directional sample: **Q1** (camera-local RIGHT / +X).
  - `func_800C4C34` is a camera projection/viewport helper, not a collision query. `func_800E8918` returns `record+0x34`, not its direct `v0`.
  - Causal chain to missing 3rd `SET74`: **PROVEN**. Collision attribution: **REFUTED**, root level `R` (NO PRODUCTION FIX).

**PROJECTION RECORD +0x34 — B-STATE (2026-08-20, canonical):** correctly
joined Q1 `+0x34` is **real** (Original 0 vs Recomp 1, 25/25). Explicit
classifier args `a0/a1/a2/a3` match. First difference is hidden
`func_8001A0C0(176,161)` `v0` (Original 0 vs Recomp nonzero) inside
`func_800E81E0` ([[plan/evidence/p08-demo-projection-classifier-rejoin]]).
The 2026-08-19 **B-INPUT (`a3=v1[2]`)** wording is **CORRECTED/SUPERSEDED**.
No production fix.

**PROJECTION v1[2] DIRECTION PRODUCER — JOIN CORRECTED (2026-08-20):**
aligned Q1 `a3` is bit-identical; `0x4313D3A7` vs `0x43136AD5` are consecutive
stream values on **both** runtimes
([[plan/evidence/p08-demo-projection-v1-direction-producer]]).

**func_8001A0C0 STATE & Z-BUFFER DEPTH WRITER — B-WRITER PROVEN (2026-08-20):**
[[plan/evidence/p08-demo-1a0c0-state-differential]],
[[plan/evidence/p08-demo-db-8007bcc4-lifecycle]],
[[plan/evidence/p08-demo-db-target-writer]]. `D_8007BCC4` (base `0x80137840`, size `0x21D80`)
is the **game's 304×228 16-bit N64 Z-Buffer (Depth Buffer)** (`304 * 228 * 2 = 138,624 = 0x21D80`).
`func_8001A0C0` is a pure leaf depth sampler returning `*(u16*)(D_8007BCC4 + 608*a1 + 2*a0)`.
The Recomp post-boot writer to `0x8014F800` (screen pixel $X=176, Y=161$) is
`RT64::Framebuffer::copyNativeToRAM` -> `RT64::NativeTarget::copyToRAM` (`memcpy`),
performing GPU depth buffer writeback for display list commands `gDPSetDepthImage` / `gDPSetColorImage`
emitted by `func_800E7BC8` / `func_800E7C28`. Written values: `0xFFFE` (clear depth) then
`0x83A4..0x8BFC` (geometry depth). Original Mupen64plus pure-interpreter control (Rice Video HLE)
omitted depth writeback to RDRAM, leaving `0x0000`. Class = **B-WRITER**. NO production fix.

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

## P08 USER_DEMO_4 walkstart ncbapivot fresh join (2026-08-21)

The requested hardware-faithful diagnostic capture completed. RT64 and the
Angrylion oracle used real `DISPLAY=:0` with the AMD RX 580 RADV path and a
720-second bound; the only new gate was `TOOIE_UD4_WALKSTART=1`. RT64 reached
`stage=0/sub=4` with seven ncbapivot entries/leaves (glidx 36..42), and the
oracle produced the paired glidx 39 trace and motion CSV. Pad/glrecord joining
is exact for glidx 38..42, including target glidx 39.

The glidx 39 ncbapivot entry payload matches after pointer relocation, but the
output differs. This is not a completed instruction join: RT64 diagnostics do
not expose full FCSR bits (`rm=0`, `mips3=0` only), while oracle FCSR is
`01000800` at enter and `01800800` at leave. The earliest observed child input
divergence is `800137AC` enter (`a2/f14`: RT64 `439a589e/41aa94a2`; oracle
`439a7fd5/439a054e`), followed by a call-stack order difference and one extra
oracle `80013818` invocation.

```text
P08: ACTIVE / MANUAL_VALIDATION_PENDING
P09: PENDING / NOT_FORMALLY_OPEN
NCBAPIVOT_WALK_READ_SET_COMPLETE: NO
FIRST_EXECUTED_READ_DIFFERENCE:    NOT_PROVEN; earliest observed 800137AC enter child-input difference
SAME_INPUTS_DIFFERENT_OUTPUT:      NO / NOT_PROVEN
ROOT_CLASS:                        B-NCBAPIVOT-WALKSTART-FIRST-EXECUTED-INPUT
ROOT_CAUSE_LEVEL:                  NOT_PROVEN
FIX_IMPLEMENTED:                   NO
RT64_LOG:                           /tmp/p08-rt64-display-glidx39-r2.log
ORACLE_LOG:                         /tmp/p08-angrylion-ud4-cam.log + /tmp/p08-angrylion-user_demo4.csv
```

This remains diagnosis-only. Keep `round.w.s`, camera feedback, generated
recompiler output, and egg changes closed; the next discriminator is the
missing RT64 FCSR and executed-read provenance around `800137AC`.

Reproduction checklist for other agents: [[plan/evidence/p08-walkstart-reproduction-runbook]].

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
`ORIGINAL_INDEX_MATCH_RECOMP=UNKNOWN`, and no direct control/display-camera
dependency is proven. The next discriminator is the aligned selector and
complete-width store trace for `D_80127708[index]+0x1C`. No production change
is authorized.

## P08 player-control camera divergence — RESOLVED to Level B (2026-08-17)

The exact player-control camera discriminator
([[plan/evidence/p08-demo-camera-control-divergence]]) is now PROVEN: the
selector `ps+0x184` is `0` in **both** runtimes at every sample, the selected
`D_80127708[0]` record is the same logical camera-control object, the camctrl
store set is complete (`{0x800CA65C}` = `func_800CA628` only, no alias/block
write), and the camctrl writer input first diverges at glidx 385
(`-0.0676` deg). The camera delta maps 1:1 onto the ideal_yaw delta
(`PLAYER_DRIFT_CHAIN_CONFIRMED=YES`).

```text
SELECTOR_MATCH:                 YES  (ps+0x184 == 0, both runtimes)
TABLE_SLOT_MATCH:               YES  (D_80127708[0])
CAMCTRL_WRITER_MATCH:           YES  (func_800CA628 / store 0x800CA65C)
CAMCTRL_STORE_SET_COMPLETE:     YES  ({0x800CA65C} only)
CAMCTRL_PRIMARY_WRITER_FUNCTION: func_800CA628 (caller func_8010FC38 / func_8010FE14)
FIRST_INTERNAL_DIVERGENT_FIELD: camctrl +0x1C at glidx 385 (-0.0676 deg)
CAMCTRL_WRITER_INPUT_DIFFERS:   YES  (glidx 385)
GLIDX385_CLASS:                 FIRST_CAUSAL_CAMERA_STATE
PRIMARY_CLASS:                  SAME_WRITER_DIFFERENT_INPUT
                                (first divergent input = CONTROL_CAMERA_INPUT)
ROOT_CAUSE_LEVEL:               R  (func_800E1A58 REFUTED;
                                   [[plan/evidence/p08-demo-camera-e1a58-differential]])
FIX_IMPLEMENTED:                NO
```

**CORRECTED/SUPERSEDED (2026-08-17):** `func_800E1A58` is identity.
**Mode 2 / `func_80110588` is now PROVEN**
([[plan/evidence/p08-demo-camera-mode-producer-differential]]). FE14 copies
stack images of mode-object `+0x74/+0x80`. Last angle writer is setter
`func_80110790`; its `a1[1]` already differs at consumed 384 (−0.0675°).
Caller is **PROVEN** `ncbapivot` (781) / `ncbapivot_entrypoint_3`
(RA offset `0x24C`; prior `0x801D3E7C` is the same site). dest[1] =
`func_80013B70(eye.x-tgt.x, eye.z-tgt.z)`; look-at inputs already
differ at 384 ([[plan/evidence/p08-demo-camera-overlay-angle-producer]]).
Exact 13B70 operands at that call: eye MATCH=NO (grows at 384),
target MATCH=NO (stable background). Causal component is eye
`obj+0x74`; last writer `func_80112E98`
([[plan/evidence/p08-demo-camera-13b70-inputs]]).
No production change.

## P08 post-egg camera eye vector producer differential — PROVEN (2026-08-17)

The exact causal mechanism creating the `mode_obj+0x74` eye vector divergence
at glidx 384 is **PROVEN** ([[plan/evidence/p08-demo-camera-eye-producer-differential]]):

```text
FUNC_80112E98_ARITHMETIC_MATCH: YES (internal candidate generator is bit-faithful)
DIVERGENCE_MECHANISM:           CAUSAL_INVOCATION_OMISSION_AT_384 (phase lag)
ORIGINAL_N64_SET74_COUNT_384:   3 (1st: 80112094, 2nd: 801132F8, 3rd: 80113028)
RECOMP_SET74_COUNT_384:         2 (1st: 80112094, 2nd: 801132F8; 3rd omitted)
ORIGINAL_3RD_SET74_CANDIDATE:   (-4446.002930, -206.578674, 1266.833740)
RECOMP_RETAINED_2ND_SET74:      (-4439.804688, -203.940430, 1272.204346)
PROVEN_CALLER_TRIGGER:          ncbapivot_entrypoint_3 -> func_801138F4 -> func_80112E98
FIX_IMPLEMENTED:                NO (zero production lines modified)
```

In Original N64, `ncbapivot` invokes `func_801138F4 -> func_80112E98` starting
at glidx 384, writing the 3rd candidate vector to `mode_obj+0x74`. In Recomp,
the branch condition in `ncbapivot` bypasses `func_801138F4` at glidx 384-385,
leaving `mode_obj+0x74` at the 2nd SET74 vector and creating the 0.0645° yaw divergence.

## P08 alleged camera collision differential — REFUTED (2026-08-18)

Independent Sol arbitration ([[plan/evidence/p08-demo-camera-collision-sol-review]])
verified the original MIPS at `0x800C4C34..0x800C4E54`. The routine performs
camera-relative matrix transformation, perspective projection, depth
classification, and viewport bounds/offset handling. It reads no collision
world state and evaluates no candidates or triangles. The previous CLEAR/HIT
labels also conflated `func_800E8918`'s final `lh record+0x34` with the internal
`func_800C4C34` return. Q1/Q2 projection-state returns already differ at the
first captured relevant invocation (glidx 70); 384 is only the first visible
counter consequence. Root level `R`; no production fix.

Next: dynamically match the selected projection record and trace the first
writer/input divergence to `record+0x34`, starting at `func_800E82EC`.

## P08 Z-buffer & post-egg motion hardware oracle resolution (2026-08-20, R-MOTION-ORACLE)

[[plan/evidence/p08-demo-zbuffer-angrylion-oracle]] and [[plan/evidence/p08-demo-post-egg-angrylion-motion-differential]]
resolve the Z-buffer camera and post-egg motion differentials against the hardware-faithful oracle
(Mupen64Plus Pure Interpreter + Angrylion RDP Plus + mupen64plus-rsp-cxd4 LLE):

1. **Z-Buffer (R-ORACLE):** The hardware-faithful `ANGRYLION_CONTROL` run produced live depth in RDRAM (`0x866D..0x8C11`), clear writes `0xFFFC`, and `record+0x34 = 1` across all 25/25 aligned Q1 samples, matching RT64. This is an emulator oracle, not a physical N64 measurement.
2. **Post-Egg Motion (R-MOTION-ORACLE):** The historical motion drift (which reached 58 units under Rice) disappeared completely under `ANGRYLION_CONTROL`. `camctrl` at glidx 385 matches within `+0.0030°` (`53.5799°` vs `53.5829°`), `ideal_yaw` matches identically (`102.59°` vs `102.59°`), maximum XZ error across all 765 Demo 0 frames is `< 0.89 units`, and behavior states are 100% bit-identical (`0` mismatches).
3. **Verdict:** Demo 0 movement, camera-control, and physics match the hardware-faithful oracle within the measured scope. Full render-state parity and global attract-demo fidelity remain open.
4. **Current Status:** P08 automated implementation and evidence are COMPLETE. Physical gamepad + multi-monitor display validations remain PENDING. P09 is not formally opened.

Next single action: user executes docs/plan/evidence/p08-final-physical-validation.md checklist on physical hardware.
