---
title: "Handoff — TooieRecomp"
description: "P08 remains ACTIVE / MANUAL_VALIDATION_PENDING. USER_DEMO_4 idle follow-cam glidx 1..38 match ANGRYLION_CONTROL; glidx 39 first remaining split is native 0x800979D8 return (pad 0001 MATCH). BLUE-vs-FIRE egg remains. Formal P08 gates: PHYSICAL_GAMEPAD_VALIDATION plus 0x800979D8 dynamic acceptance."
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
- **Open P08 Gates**: **PHYSICAL_GAMEPAD_VALIDATION** plus the bounded
  `0x800979D8` held-input/downstream dynamic acceptance gate.
- **Player & AI Movement Speed Parity Baseline (2026-08-21)**: [[plan/evidence/p08-provisional-player-ai-speed-parity]]. `PLAYER_SCALAR_SPEED_PARITY: PASS`, `AI_ACTOR_SPEED_PARITY: PASS` **within the measured windows only**; `AI_GLOBAL_PARITY: NOT_PROVEN`. Speed magnitude matches within sub-pixel floating point bounds across straight walk (`510.2442 u/s`), trot run (`673.5218 u/s`), jump (`690.0 u/s`), and sampled AI actors (`chdiggerground` path, `subaddie` patrol). USER_DEMO_4 speed magnitude matches (`0.0017..2.87 u/s`), supporting `CAMERA_DIRECTION_DEFECT_WITH_SPEED_PARITY` for that bounded sample. Historical downhill ~510 speed is `PROVEN_CLEAN` against original N64 hardware.
- **Attract numbering / egg discriminator (2026-08-21)**: user demos are **continuous across intro blocks**, not zero-based Demo 0..3. Canonical evidence: [[plan/evidence/p08-provisional-follow-camera-and-egg-differential]].
  - `USER_DEMO_4` = first gameplay after the first return to intro/music = map **235 Haunted Cavern** (`stage=0 sub=4`), **not** "Demo 3".
  - `EGG_PARITY_DEFECT: PROVEN` — Angrylion fires **FIRE** (`cheggfire` 535); RT64 fires **BLUE** (`cheggnormal` 539). User confirmed Angrylion RDP Plus nightly-build visual PASS for fire egg and for the previously-broken demo behavior.
  - Follow-camera on USER_DEMO_4 is **REPRODUCED**: pad 877/877 match. Bind-tick yaw sincos is now **POST-FIX MATCH** vs ANGRYLION_CONTROL (`006d5328` / `bf4b29cc` / `sp+0x48`=`node+0x74`=`c43efa3b,43c49665,c4a6b798`). Idle CA8B4/CA628 glidx 1..38 is now **bit-identical**. First remaining bit difference is dump glidx 39 `ncbapivot_entrypoint_3` output (player pos/yaw/ideal MATCH). Demo-sync still prints 0.0853° @41 → 12.0481° @50 → 112.6789° @100. Canonical: [[plan/evidence/p08-user-demo4-post-round-full-state-discriminator]].
  - `EGG_PARITY_DEFECT` remains **PROVEN**: Angrylion **FIRE** (`cheggfire` 535); RT64 **BLUE** (`cheggnormal` 539). Independent of ROUND.W.S: **YES**. Independent of remaining camera/demo split: **UNKNOWN**. No egg fix.
  - b042865 `HAUNTED_CAVERN_NO_FIRE_EVENT` is **REFUTED**. `GLOBAL_VISUAL_DRIFT_STATUS=NOT_REPRODUCED_IN_ATTRACT_SET` is **CORRECTED/SUPERSEDED** for USER_DEMO_4. Demo 0..3 remains an incomplete attract slice.
- **ROUND.W.S checkpoint (2026-08-21)**: `5854872c79a7cab1c9395ddcd173cece449ffe11` (`fix: preserve MIPS round.w.s semantics`). `ROUND_W_S_ROOT_CAUSE: PROVEN`. `ORIGINAL_BIND_TICK_DISCRIMINATOR: RESOLVED`. `ROUND_W_S_BIND_AND_IDLE_CAMERA_EFFECT: FIXED`. `CAMERA_COMPLETE_FIX: NO`. `CAMERA_FEEDBACK_ACCUMULATION: STILL_PRESENT`. `ROUND_W_S_SPECIAL_VALUE_CONTRACT: PARTIAL`. Evidence: [[plan/evidence/p08-user-demo4-sincos-semantics-fix]].
- **Agent workspace (2026-08-21)**: open CLI/IDE in **TooieRecomp/**. MCP configs live here and bind markdown-vault to `docs/` ([[MARKDOWN_VAULT_TOOLS]]). Do not start agents at `N64-Recomp/`. `BanjoRecomp` and the cart ROM remain siblings for CMake/oracle only.
- **USER_DEMO_4 `0x800979D8` bainput fix & dynamic validation (2026-08-22)**: [[plan/evidence/p08-user-demo4-800979d8-postfix-validation]], [[plan/evidence/p08-user-demo4-800979d8-independent-review]], [[plan/evidence/p08-user-demo4-bainput-held-state-provenance]].
  - Root fix: `0x800979D8` (`tooie_bainput_func_800979D8`) implemented in `src/game/boot_hw_stubs.cpp`. Unit tests in `tests/bcf28_helpers_test.cpp` PASS.
  - Held-state provenance & dynamic validation: raw pad `(-48,-46,0001)` **MATCH**. AN and RT `prev_state[13]` are both `0`. Both execute `bakey_held` -> `0x800BCE84` (held=1), both return `v0=2` (`1 << input_id` from enabled mask), both execute the camera rotate-right chain `80097B94 -> 80110EFC -> 80110E00 -> 801111D0 -> 80112D90`, write `p60.C=1, p60.E=2`, set `p60+8=5` at `80112D08`, and take the matching `ncbapivot` transition branch.
  - Camera parity: `CAMCTRL_DELTA_41 = 0.0000°`, `CAMCTRL_DELTA_50 = 0.0000°`, `CAMCTRL_DELTA_100 = 0.0000°`. Accumulation drift is **RESOLVED**.
  - Egg parity: `USER_DEMO_4` fires `cheggfire` (535 FIRE) matching Angrylion Oracle (`cheggfire` 535).
  - Dynamic acceptance: instruction-level `0x800BCE84` live-input join is now
    **PROVEN** ([[plan/evidence/p08-user-demo4-bce84-live-input-join]]). The old
    RT `bs_pressed=0` probe read ring 0 (`obj+0x58`), not `unk8[13]` ring 1
    (`obj+0x90`, word `0x115`, LSB 1). Immutable recapture
    `/tmp/p08-979d8-final-20260822T070735Z/` (`SHA256SUMS` PASS twice).
    Worktree dirty; P08 **ACTIVE / MANUAL_VALIDATION_PENDING**; P09
    **NOT_FORMALLY_OPEN**; no commit/push.

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

The glidx 39 ncbapivot entry payload matches after pointer relocation for
`n74`/`n80`/`tgt`/pos/yaw/ideal, but **`*(node+0x60)+8` does not**: RT64 `0`,
oracle `5`. That word is already different at ncbapivot enter. `80112C3C`
`sltiu`/`beq` at `0x80112C5C` therefore takes opposite paths (skip jumptable vs
case 4 → `800F1738`).

The previously reported `800137AC` `a2`/`f14` difference is
**CORRECTED/SUPERSEDED**: both are dead at that stub (`f14` overwritten from
`0x800416C8`; `a2` unused). Post-overwrite `80013818` enter `f12`/`f14` MATCH.
The extra oracle `80013818` is an alignment artifact of host `func_800137F4`
calling `func_80013818` without `LOOKUP_FUNC`. Guest FCR31 is not stored in
`recomp_context`; the `80112C5C` split is integer and does not read FCSR.

```text
P08: ACTIVE / MANUAL_VALIDATION_PENDING
P09: PENDING / NOT_FORMALLY_OPEN
CALL_TREE_STRUCTURALLY_ALIGNED:    YES through 800137AC enter
EXTRA_80013818_ORACLE_CALL:        ALIGNMENT_ARTIFACT
800137AC_A2_USED_BY_CHILD:         NO
800137AC_F14_USED_BY_CHILD:        NO
RT64_FULL_FCSR_CAPTURED:           NO (UNEMULATED)
FCSR_DIFFERENCE_CAUSALLY_READ:     NO
FIRST_PROVEN_DISCRIMINATOR:        p60_8 0 vs 5 at glidx 39 ncbapivot enter; 80112C5C
ROOT_CLASS:                        B-NCBAPIVOT-CONTROL-INPUT
ROOT_CAUSE_LEVEL:                  PROVEN for this ncbapivot split; p60 writer outside it
FIX_IMPLEMENTED:                   NO
RT64_LOG:                          /tmp/p08-rt64-display-glidx39-r2.log
ORACLE_LOG:                        /tmp/p08-angrylion-ud4-cam.log + /tmp/p08-angrylion-user_demo4.csv
```

This remains diagnosis-only. Keep `round.w.s`, camera feedback, generated
recompiler output, and egg changes closed. Next: writer of `node+0x60` between
glidx 38 leave and glidx 39 enter. Canonical:
[[plan/evidence/p08-user-demo4-walkstart-ncbapivot-discriminator]].

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
4. **Current Status:** P08 remains active. Physical gamepad validation and the held-input/downstream dynamic acceptance for `0x800979D8` remain pending. P09 is not formally opened.

Next single action: user executes docs/plan/evidence/p08-final-physical-validation.md checklist on physical hardware.

## Current P08 residual: USER_DEMO_4 p60 writer provenance

The `glidx=39` camera split is now statically bounded to `80110588 → 80112A84
→ 80112D08`; historical RT64 records `a1=0`/`p60+0xC=0` while the oracle
reads `p60+8=5`; the oracle writer input is `a1=5`/`p60+0xC=1`. This closes
B-P60-WRITER-INPUT for the historical capture. The native helper semantics
are now corrected locally, but downstream acceptance is still open. See
[[plan/evidence/p08-user-demo4-p60-writer-provenance]] and
[[plan/evidence/p08-walkstart-reproduction-runbook]].

## 0x800979D8 independent review (2026-08-22)

[[plan/evidence/p08-user-demo4-800979d8-independent-review]] independently
verified the original guest bodies: `0x800979D8` is a real static bainput
helper, not a zero-valued placeholder. Its correct contract calls
`bakey_held`, uses `800BCE84` when `prev_state[id]==0`, and returns the
logical-input enabled mask. The corrected Tooie mapping is registered and the
full focused test passes. Paired held-state provenance
([[plan/evidence/p08-user-demo4-bainput-held-state-provenance]]) shows the
first remaining split in the old capture is **inside** that mapping at glidx
39: matching pad `0001` and matching `prev_state[13]=0`, AN return `2` vs RT
`0`. Probe `held=` is `prev_state`, not `bakey_held`. Instruction-level
`0x800BCE84` join ([[plan/evidence/p08-user-demo4-bce84-live-input-join]])
now proves the live source word `0x115` / mask `1` on both runtimes; the
old `bs_pressed=0` probe is **CORRECTED/SUPERSEDED**. The joined event also
matches `bakey_held=1`, `0x800979D8=2`, `p60.C/E/+8=1/2/5`, and ncbapivot.
The helper chain is **PROVEN in bounded scope**; do not promote this to global
attract parity. User Demo 1 and Demo 5 movement residuals remain open; see
[[plan/evidence/p08-attract-movement-residuals]].

```text
P08_STATUS:                         ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                         PENDING / NOT_FORMALLY_OPEN
NEXT_SINGLE_ACTION:                 trace USER_DEMO_5 to first movement/state divergence
FIX_COMMIT:                         BLOCKED — recorded log hashes do not match current files
```

## USER_DEMO_5 angular-input checkpoint (2026-08-23)

The new capture `/tmp/p08-demo5-ee780-source-20260823T053510Z/` passed its
SHA256 check twice. `800EE780` is confirmed as a downstream elementwise add;
the glidx-24 divergence is already in its `sp+0x54` input. `800EEBF0` and
`800EF214` are an angle-pair-plus-radius to Cartesian transform:
`x=r*cos(p)*sin(y)`, `y=r*sin(p)`, `z=r*cos(p)*cos(y)`.

Only the yaw/heading input differs (`4391e610` RT vs `43927d8b` AN); pitch and
radius match. The value is the second word at ncbapivot `sp+0x38`, written by
the caller add at `0x80800194` and stored at `0x808001A0` before the
`800EEBF0` call. The add combines `80012C3C` and `800125E8`, but its complete
operand join is not captured.

```text
ROOT_CLASS: B-ANGLE-SOURCE-PRECALL-COMBINATOR
MODEL: Grok 4.6 High (corrected canonical label)
FIX_IMPLEMENTED: NO
EVIDENCE_COMMIT: NONE
PUSHED: NO
NEXT_SINGLE_ACTION: paired trace of the two 0x80800194 add operands
```

## USER_DEMO_5 p60 mode-state review — 2026-08-23

`func_80112C3C` is confirmed: mode 0 passes `in_yaw` through, modes 1/2/5
step via `800F1738`, and modes 3/4 use `800F1DCC`. Existing paired evidence
shows p60+8 first differs at glidx 23 (`RT=5`, `AN=0`), while glidx 24 is the
first yaw-output difference. The static immediate writer is
`func_80112D08`, `sw $a1,0x8($t6)` at `0x80112D14`, reached from
`func_80112A84` at `0x80112C20`; dynamic first-write capture is pending because
RT64 Vulkan could not present under the current headless environment. No fix,
commit, or push.

**USER_DEMO_5 `func_801111D0` call-presence arbitration (2026-08-23):** the
`jal 80112D90` at `0x80111294` is enabled by
`(80112E84(a0)==1) || (8011349C(a0,&sp+0x40)!=0)`, with control branches at
`0x80111230` and `0x80111270`. At glidx 23 `p60+0xC=0` before the function in
both runtimes; RT takes the second gate and calls D90, while AN skips it. The
raw input `(2,1,2002)` matches, so this is not proven as a direct C_RIGHT
test. The exact `8011349C` return was not captured; boundary is
`B-801111D0-PREDICATE-INPUT` with call presence proven and helper provenance
pending. No production change, commit, or push.

## Final immutable checkpoint (2026-08-22T07:07Z)

The fresh paired capture `/tmp/p08-979d8-final-20260822T070735Z/` passes
`sha256sum -c SHA256SUMS` twice. The target join is stage 0/sub 4, glidx 39,
pad `(-48,-46,0001)`, button 13, ordinal 8; BCE84 ring-1 raw `0x115` and
return 1, `bakey_held=1`, `0x800979D8=2`, and p60 C/E/+8 `1/2/5` match in the
bounded ncbapivot chain. Task-specific BCE84/979D8 probes are removed;
reusable diagnostics remain env-gated. Tests/build/diff/generated-source
gates pass. Demo 1/5 movement residuals stay open; this checkpoint does not
start Demo 5.

## USER_DEMO_5 `0x80097B94` checkpoint — 2026-08-23

The wrapper contract is now independently fixed: `0x80097B94` passes action
`0xD` and input id `1` to `0x800979D8`, preserving the helper's return. Oracle
matches Gemini's glidx-23 record `(stage=0, sub=5, pad=(2,1,2002))` and shows
BCE84 ring 0/raw 0/masked 0, final result 0. The focused RT64 event is not the
same record (`pad=(-13,-15,0xF1F1)`) and also returns 0. Consequently the
published helper fix covers the implementation, but the claimed RT=2/AN=0
semantic split is not proven. No root move, production fix, commit, or push.

## USER_DEMO_5 `0x80097B70` checkpoint — 2026-08-24

The independent `0x80097B70` trace confirms action `0xA` (`C_LEFT`), input id
0, and the caller edge `0x800A4928` → `0x80110EFC(node, 0)` at `0x800A4930`.
The earlier joined run shows RT64 returning `1` and reaching the consumer at
glidx 23/24 while Oracle returns `0`; Oracle's record-23 state is
`prev=0/ring=1/raw=0xF3/masked=1/bakey=1`, with input 0 disabled before the
helper. A fresh RT64 run with the same visible `(2,1,2002)` record has
`ring=0/raw=0x72C/masked=0/bakey=0` and also returns `0`.

The result is bounded to `B-ACTION-0A-HISTORY-STATE`: the visible pad is
joined, but the history-selected BCE84 word and input-enabled state are not.
The published `0x800979D8` fix covers the shared helper contract; its upstream
state writer remains unresolved. No root move, production fix, commit, or push.

## Corrected USER_DEMO_5 camera re-anchor (2026-08-24)

The correctly paired replay changes the first camera-control anchor to record
24 MATCH -> record 25 DIFF. The Trot-exit input writer matches in RT64 and
Oracle: `bainput_enable(0,1)` changes `0xFFFFFFBE -> 0xFFFFFFBF` and sets bit
0; action `0xA` returns 1 in both. Thus the old `RT97B70=1/AN=0`, RT-only
`0x80110EFC`/`0x801111D0`/`0x800979D0`, and action-`0xA` causal chain are
superseded, including the old `0x80097B94 RT=2/AN=0` claim.
`B-TROT-ENABLE-RECOVERY` is not the root.

The demo synchronizer samples selected camera record `+0x1C`, selected through
`0x80127708[ps+0x184]`; static `func_800CA628` identifies writer PC
`0x800CA65C`, source2[1]. Record 24 is `293.5294` / `0x4392C3C2`; record 25
is RT64 `293.0478` versus Oracle `293.4005` / `0x4392B342`. The Oracle direct
writer is captured, but RT64's direct writer/raw bits are absent. Classification
is `B-CAMERA-WRITER-OR-SOURCE-INCOMPLETE-JOIN`.

Do not promote the old record-23 p60 `5/0`, D90, yaw, or camera-eye causal
claims; p60 must be rechecked only on record 24->25. Next bounded capture:
RT64 `0x800CA65C` with raw old/source/new bits and source2 inputs. Artifact:
`/tmp/p08-demo5-record25-camera-firstdiff-20260824T024500Z/`.
`P08 ACTIVE / MANUAL_VALIDATION_PENDING`; `P09 PENDING / NOT_FORMALLY OPEN`.
No production change.

### USER_DEMO_5 first-writer arbitration correction

The direct CA628 capture now includes RT64. The first record-25 writer observed
is CA628 ordinal `10329` (RA `0x8010FE24`), but RT64 enters with field
`0x4392861E` and source2[1] `0x43922BE8`; Oracle's paired ordinal `103` has
`0x4392B342` and `0x4392A91A`. Therefore CA628 is a downstream Case-B writer,
not the causal boundary. CA8B4 copies the field and E1A58 preserves it later;
the pre-CA628 field writer is still unresolved and needs a semantic write watch.

### USER_DEMO_5 camera-field identity correction

The direct artifact proves only `stack_angles+0x4` and downstream
`selected_record+0x1C`: CA628 is a downstream Case-B writer. `0x8010FE48` is
the `or s0,a0,zero` instruction inside `func_8010FE34`, while the incoming
heading is loaded from `active_node+0x3C` at `0x8010FEA8`. Neither FE34 nor
FE14 writes `active_node+0x34` or `active_node+0x3C`. The direct static
node+0x3C writer candidate is `func_8010FDEC:0x8010FDFC`; its first causal
runtime write remains uncaptured. Current boundary:
`B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`. Next single action: watch the joined
active camera node `+0x3C` semantically.

## USER_DEMO_5 input_id=0 enable-word provenance — 2026-08-24

`bainput_isEnabled` is exactly a bit test on `*(u32 *)(*(u32 *)(self+0xA4))`;
the action-0xA path tests bit 0. Exact storage writers are `bainput_init`
(`0x8009811C`) and `bainput_enable` (`0x80098140`). Static search found 29
setter call sites (15 set, 14 clear). In the target Oracle trace the word is
`FFFFFFBE` at glidx 22 and `FFFFFFBF` at glidx 23/24, proving a bit-0 state
transition with unchanged visible pad `(2,1,2002)`. The diagnostic writer hook
captured no init/set/clear event, and the new RT64 recapture was blocked by the
headless Zenity launcher. Thus the immediate runtime setter/predicate remains
unattributed; keep classification `B-ACTION-0A-HISTORY-STATE`.

Artifact: `/tmp/p08-demo5-input0-enable-firstdiff-20260824T013213Z/`.

### USER_DEMO_5 camera heading field correction (2026-08-24)

The latest `node34-write-watch` artifact's decisive `0x800EFA5C` store is to
temporary `stack_angles+0x4`, not `active_node+0x34`. Its input is the already
divergent `active_node+0x3C`, loaded at `func_8010FE34:0x8010FEA8`; CA628's
`selected_record+0x1C` write is downstream. The direct static writer candidate
for `active_node+0x3C` is `func_8010FDEC:0x8010FDFC`, called by
`plcamera_entrypoint_13:0x808004A0`, but no paired semantic node+0x3C write
watch captured it. Current classification:
`B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`.

Latest semantic join: RT active node `0x801C6610`, Oracle active node
`0x801CACB0`; the earlier `0x8039D820` was observed at an older FE14 entry and
is not a proven join to this capture. No production fix or generated C edit was
made.

### USER_DEMO_5 node+0x3C checkpoint audit correction (2026-08-24)

The newer node3C artifact does not prove the claimed post-player MATCH or
pre-FE34 DIFF. Oracle's raw checkpoint probe used `0x8031CF80` with mode `0`
through the target window; its later `0x801CACB0` read still returned
`0x01015590`, and RT64 emitted no paired raw node3C checkpoints. The angular
values were source/downstream boundary values, not direct semantic reads of
`active_node+0x3C`. Keep the safe boundary as
`B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`; no node+0x3C writer is dynamically
identified.

### USER_DEMO_5 EFA4C heading provenance (2026-08-24)

Safe pre-pass boundary: `B-EFA4C-HEADING-SOURCE-PROVENANCE-UNRESOLVED`.
EFA4C is a downstream stack writer (`f14` <- `a2` at `0x800EFA50`, store
`0x800EFA5C`). FE34 is a load/consumer **when executed**. FDEC remains
`REFUTED_FOR_TARGET`. Recapture
`/tmp/p08-demo5-efa4c-heading-provenance-20260824T061436Z/` (SHA256 PASS ×2
after chmod): Luna heading bits and the `10970→EFA4C` call were **not
observed**. `R24_EFA4C_EVENT_JOIN: FAIL`. `ROOT_CLASS: CASE E -
CORRECTED/SUPERSEDED`. Do not reuse node+0x3C identity. No fix/commit/push.

### USER_DEMO_5 record24 camera-path conflict arbitration (2026-08-24)

`ROOT_CLASS: JOIN-UNRESOLVED`. The latest Grok artifact is read-only and
SHA256-valid, but its metadata says Medium while the user reports High, and
its `sub=5` capture has colliding slices/two RT remapped-`glidx=24` series.
It does not provide the requested semantic record join or paired RT/Oracle
camera-call/CA628/state tables. RT-only wrapper lines (`ra=0`) cannot prove
`FC38 -> FE14 -> CA628`; Oracle has no matching semantic path. The old
FE34/10970/EFA4C claim is `CORRECTED/SUPERSEDED`, but the alternative is not
proven. `CONFLICT_WINNER: NEITHER — NEW CAPTURE REQUIRED`; no source-level
root cause or production fix is authorized. This is independent of the
separate node+0x3C boundary correction.

### USER_DEMO_5 collision-free map296 join and first camera difference (2026-08-24)

JOIN_VERDICT=PASS: the new raw logs independently contain 28 RT and 28 AN records, 28/28 raw6 matches, one epoch, stage=0/sub=5/map=296/mode=5, and zero duplicate canonical record keys. D_8012D5B0 holds the active demo pointer at +0x0 and the post-consume counter at +0x8 (D_8012D5B8), with six-byte records; camera tags are post-consume context, not logical ordinals. func_800151BC is the proven logic-tick boundary.

Current camera boundary: CAMERA_UPDATE_BOUNDARY_VERDICT=REFUTED for the claimed single 0x800A8E9C -> 0x800A9318 update. Static A9318 is a separate FE34 pass and raw R24 has EFA4C 66/73, FE14 1/1, CA628 2/2, A9318 0/0; R24_CALL_SEQUENCE_MATCH=NO.

The first comparable camera event is logical record 0, camera ordinal 1, CA628 call 1. Old bits match (0x4324BF6C), source bits do not (RT=0x4324B542, AN=0x4324B132): ROOT_CLASS=B-FIRST-CA628-SOURCE. At R24, old/source already differ (0x4324A910 vs 0x4324A7B0), so CA628 is downstream and output is unknown. The old FE34/10970/EFA4C chain is CORRECTED/SUPERSEDED; no deeper source is promoted and no fix was implemented.

Derived artifact: /tmp/p08-demo5-first-camera-diff-from-joined-run-20260824T074500Z/; manifest SHA256 d1139defdfa6903041cef31fd2e4db98ca328bd964e702a4e1dca6683a7cff3f.

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

**LATEST_PROVEN_FACTS:** 28/28 joined records; `func_80110588` entry base `0x43240367` and node+`0x50` `0x3F3C04A3` match; the earlier `EE7F8` write is RT `0x4324036A` versus AN `0x4324036D`; `0x80110634` already copies the difference to `sp+0x2C`; node+`0x50` first differs at `0x801161E0`; `0x80116380`/`CA628` are downstream.

**REFUTED_OR_SUPERSEDED_CLAIMS:** Gemini's `80116380` root; prior `B-R0-CA628-SOURCE-WRITER-PROVEN` as root; older FE34/10970/EFA4C/node+3C roots. Map 296 is Hailfire Peaks — Icy Side / `MAP_128_HP_ICY_SIDE`, not Mayahem; current TOML has no human-readable map table.

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

**PROJECT_PHASE:** P08 active causal arbitration; **P08_STATUS:** `ACTIVE / MANUAL_VALIDATION_PENDING`; **P09_STATUS:** `PENDING / NOT_FORMALLY_OPEN`.

**CURRENT_HEAD:** `25595a2eb8241ef7ca4aa135e6681e44abcf8598`. **WORKTREE_STATUS:** intentionally dirty state preserved; this round changed documentation only; no `RecompiledFuncs/**` or `src/overlays` change.

**CURRENT_INVESTIGATION:** `USER_DEMO_5`, `stage=0/sub=5/map=296`, semantic `Hailfire Peaks — Icy Side / MAP_128_HP_ICY_SIDE`.

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
  - `src_a2` = RT: `(0xC43E589D, 0x43BEC05A, 0x43983EF2)` vs AN: `(0xC43CBBB0, 0x43BEC05A, 0x43A***CURRENTLY_DOWNSTREAM_FUNCTIONS:** `func_800EE780`, `func_800EEBF0`, `func_80112C3C`, `func_80112D08`, `func_80112A84`, and `func_80112D90` are all faithful downstream functions whose internal logic is bit-exact and whose operations faithfully follow their entry inputs / control flow.

**REFUTED_OR_SUPERSEDED_CHAINS:** do not reopen the bounded USER_DEMO_4 `979D8 → BCE84 → P60` chain or the previously superseded generic movement/CA628/FE14 roots. The earlier `80116380` camera claim remains superseded by the prior Record-0 correction.

**LATEST_ARTIFACT:** `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/`.
- **MANIFEST_SHA:** `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`.
- **SHA256_VERIFY:** PASS (verified twice; read-only `0444`/`0555`).
- **RAW_LOG_PROVENANCE:** `NEW_RAW_CAPTURE` (Dual live capture with paired dynamic hooks in both RT64 and ANGRYLION_CONTROL).

**OBSERVATIONAL_GAP_RESOLUTION:** The prior gap (where ANGRYLION `func_80097B70` return = 0 was an inference) is now **RESOLVED**. Both runtimes directly measured `func_80097B70` (`bainput_should_rotate_camera_left`), `func_800979D8`, and branch `0x800A4928` (`beq $v0, $zero, 0x800A4940`):
- `glidx 22`: `func_800979D8` v0=0/0 MATCH; `func_80097B70` v0=0/0 MATCH; branch 0x800A4928 taken=1/1 MATCH (branches over rotation left in both runtimes).
- `glidx 23`: `func_800979D8` v0=1 (RT) vs v0=0 (AN) DIFF; `func_80097B70` v0=1 (RT) vs v0=0 (AN) DIFF; branch 0x800A4928 taken=0 (RT, fallthrough -> calls `func_80110EFC` -> `80110E00` -> `801111D0` -> `80112D90`) vs taken=1 (AN, branches over rotation left).

**CURRENT_UNKNOWNS:** upstream evaluation inside `func_800979D8` where `bakey_held(self, 10)` (`0x800919F4`) / `func_800BCE84` evaluates to `held = 1` in RT64 vs `held = 0` in ANGRYLION_CONTROL at `glidx 23`.

**PENDING_WORK:** Causal arbitration and next upstream boundary investigation of `bakey_held` (`0x800919F4`) and `func_800BCE84` for button 10 (C-Left) at `glidx 23`.

**NEXT_SINGLE_ACTION:** Perform static and mechanical dynamic boundary analysis of `bakey_held` (`0x800919F4`) and controller state query `func_800BCE84` for button 10 (C-Left) at `glidx 23`.

**NEXT_RECOMMENDED_MODEL:** Gemini 3.7 Flash High for mechanical boundary instrumentation / GPT-5.6 Luna High for causal arbitration.

**FIX_IMPLEMENTED:** NO. **COMMIT:** NONE. **PUSHED:** NO.

## Current independent arbitration — USER_DEMO_5 func_80097B70 / func_800979D8 Direct Measurement (glidx 22->23)

- **PROJECT_PHASE:** P08. **P08_STATUS:** ACTIVE / `func_80097B70` and `func_800979D8` directly measured. **P09_STATUS:** PENDING / NOT FORMALLY OPEN.
- **CURRENT_HEAD:** `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
- **LATEST_ARTIFACT:** `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/`; all `SHA256SUMS` entries pass; **MANIFEST_SHA:** `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`.
- **RAW_LOG_PROVENANCE:** `NEW_RAW_CAPTURE` (dual live captures of `TooieRecompiled` and Mupen64Plus `ANGRYLION_CONTROL`).
- **SEMANTIC_JOIN:** `PROVEN`. Normalized join key is `(stage=0, sub=5, map=296, mode=5, glidx, calling_region=func_800A4878, controlling_branch=0x800A4928, bainput_helper=func_800979D8)`.
- **LAST_COMMON_CONTROL_FLOW_EVENT:** glidx 22 (`func_800979D8` v0=0/0 MATCH, `func_80097B70` v0=0/0 MATCH, branch 0x800A4928 taken=1/1 MATCH, `func_80112D90` NOT called in either runtime; `p60+0xC = 0`).
- **FIRST_DIFFERENT_CONTROL_INPUT:** Return value `$v0` of `func_800979D8` and `func_80097B70` at glidx 23: RT64 `$v0 = 1` vs ANGRYLION `$v0 = 0`.
- **FIRST_DIFFERENT_BRANCH:** `0x800A4928: beq $v0, $zero, L_800A4940` (NOT TAKEN in RT64 -> invokes `func_80110EFC` -> `func_80110E00` -> `func_801111D0` -> `func_80112D90`; TAKEN in ANGRYLION -> bypasses camera rotation dispatch).
- **CALL_PRESENCE_AT_TARGET_CALLSITE (func_801111D0 @ 0x80111294 -> func_80112D90):**
  - RT64 (glidx 23): `YES` (executed, setting `p60+0xC = 1`).
  - ANGRYLION (glidx 23): `NO` (`NOT_CALLED / N/A`, `p60+0xC = 0`).
- **P60+0xC STATE TRANSITION:**
  - RT64: `OLD_P60_C = 0` -> `func_80112D90` writes `1` -> `NEW_P60_C = 1`.
  - ANGRYLION: `BEFORE = 0` -> no call -> `AFTER = 0`.
- **CURRENT_ROOT_CLASS:** `B-BAINPUT-HELD-STATE` / `B-ROTATE-CAMERA-LEFT-INPUT`.
- **CURRENT_ROOT_BOUNDARY:** `bakey_held(self, 10)` (`0x800919F4`) query inside `func_800979D8` evaluating to `held = 1` in RT64 vs `held = 0` in ANGRYLION at `glidx 23`.
- **LATEST_PROVEN_CAUSAL_CHAIN:** `bakey_held(self, 10) (1 vs 0)` -> `func_800979D8 v0 (1 vs 0)` -> `func_80097B70 v0 (1 vs 0)` -> branch `0x800A4928 (not taken vs taken)` -> `func_80110EFC` -> `func_80110E00` -> `func_801111D0` -> `func_80112D90 (called vs not called)` -> `p60+0xC (1 vs 0)` -> `func_80112A84 bundle (5 vs 0)` -> `func_80112D08 p60+8 (5 vs 0)` -> `func_80112C3C (5 vs 0)` -> yaw -> `800EEBF0` -> `800EE780` -> camera/movement.
- **NEXT_SINGLE_ACTION:** Perform static and mechanical dynamic boundary analysis of `bakey_held` (`0x800919F4`) and `func_800BCE84` for button 10 (C-Left) at `glidx 23`.
- **NEXT_RECOMMENDED_MODEL:** Gemini 3.7 Flash High / GPT-5.6 Luna High.
- **WORKTREE_STATUS:** intentionally dirty; no `RecompiledFuncs/**` or `src/overlays` changes. **FIX_IMPLEMENTED:** NO. **COMMIT:** NONE. **PUSHED:** NO.

## Superseding arbitration — `800979D8` primary-log correction (2026-08-25)

**ARTIFACT:** `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/`
**MANIFEST_SHA:** `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`
**SHA256_VERIFY:** PASS. **RAW_LOG_PROVENANCE:** `NEW_RAW_CAPTURE`.

The preceding `held RT=1 / AN=0` claim is **CORRECTED/SUPERSEDED** by the
primary logs. In the joined `USER_DEMO_5` (`stage=0/sub=5/map=296/mode=5`,
`glidx=22 -> 23`) target, RT and AN both have semantic `self`, `button=10`,
`input=0`, `prev_state=0`, map `0`, button word `0`, enabled word
`0xffffffbf`, enabled bit 0 `1`, and `bakey_held=1`. Raw self pointers differ
(`RT=0x801e5410`, `AN=0x801e84f0`) and are not join keys.

The first proven difference is `800979D8` leave: `RT v0=1`, `AN v0=0`.
`80097B70` forwards this value faithfully and `0x800A4928` branches on it.
The exact RT `800A4878` entry has no separate marker, but its branch PC and
callsite are directly observed; the semantic join remains **PROVEN**.

Static read-set: `bakey_held` reads `self+0x40`, `key+0x18+button_id`, and
`key+0`; on zero previous state it calls `800BCE84(state, button_id)` at
`0x80091A18`. `800BCE84` reads `state+0x8+button_id`, calls `800BCCC0`,
indexes by button, and returns bit 0. `bainput_isEnabled` reads `self+0xA4`
and its enabled word. Logged map/button-word fields are trace-only, not
functional `bakey_held` reads.

AN has no direct `800BCE84` marker for the mapped-byte/table inputs; therefore
the complete external read-set is **NOT_PROVEN_MATCHING**. `800BCE84` remains
`NEXT_UPSTREAM_CANDIDATE`, not root. **CURRENT_ROOT_CLASS:**
`B-800979D8-COMPLETE-READSET-UNKNOWN` (Case B). **CURRENT_ROOT_BOUNDARY:**
`800979D8` return at glidx 23. **NEXT_SINGLE_ACTION:** dual-runtime glidx
22/23 capture of the complete `bakey_held` pointer chain,
`800BCE84` presence/args/return, and `800979D8` held/return; stop at
`800BCE84`. **NEXT_RECOMMENDED_MODEL:** Gemini 3.7 Flash High. No fix.

## Superseding narrow arbitration — `800979D8` post-`isEnabled` body (2026-08-25)

Artifact `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/` is valid,
`NEW_RAW_CAPTURE`, manifest `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`,
and `sha256sum -c SHA256SUMS` passes. At joined glidx 23, `bakey_held=1/1`
and `enabled_word=0xffffffbf` gives enabled bit 0 `1/1` (**PROVEN**).

## Canonical Resolution — `func_800979D8` Exact MIPS Recovery & Static Implementation Mismatch (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/` is valid,
`NEW_RAW_CAPTURE`, manifest `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`,
and `sha256sum -c SHA256SUMS` passes (dual verification).

1. **Exact Original MIPS Body (`0x800979D8 .. 0x80097A4C`) Recovered and Proven:**
   - `0x800979E8`: calls `bakey_held(self, button_id)` (`0x800919F4`).
   - `0x800979F0`: `beqz $v0, 0x80097A2C` (if not held, jumps to unheld path).
   - `0x800979FC`: calls `bainput_isEnabled(self, input_id)` (`0x80098024`), which queries `*(input + 0x0) & (1 << input_id)`.
   - `0x80097A04`: `bnez $v0, 0x80097A18` (if enabled, skips disable call).
   - `0x80097A10`: calls `func_80097990(self, input_id, 0)` (when enabled == 0, clears bit in `*(input + 0x4)`).
   - `0x80097A1C`: calls `func_80097978(self, input_id)`, which returns `*(input + 0x4) & (1 << input_id)`!
   - `0x80097A24`: jumps to epilogue and returns `$v0` from `func_80097978`.
   - Unheld path (`0x80097A2C`): calls `func_80097990(self, input_id, 1)` (sets bit in `*(input + 0x4)`) and returns `$v0 = 0`.

2. **Root Cause of Divergence at glidx 23:**
   - At `glidx 23`, `bakey_held` returns `1` and `bainput_isEnabled` returns `1` in both runtimes.
   - Original MIPS: executes `0x80097A1C: jal 0x80097978`. `func_80097978` reads `*(input + 0x4) = 0xFFFFFFFE` -> evaluates `bit 0 = 0` -> returns `$v0 = 0`.
   - RT64 host replacement (`tooie_bainput_func_800979D8` in `src/game/boot_hw_stubs.cpp`): incorrectly calls `bainput_isEnabled(self, input_id)` a second time (reading `*(input + 0x0) = 0xFFFFFFBF` -> evaluates `bit 0 = 1`) -> returns `ctx->r2 = 1`.

3. **Classification:**
   - `STATIC_IMPLEMENTATION_MISMATCH_800979D8`: **PROVEN**.
   - `CURRENT_ROOT_CLASS`: `B-800979D8-STATIC-HOST-IMPLEMENTATION-MISMATCH`.
   - `CURRENT_ROOT_BOUNDARY`: Callee selection in `func_800979D8` (`0x80097A1C` calls `func_80097978` on `input+0x4`, not `bainput_isEnabled` on `input+0x0`).

4. **Next Actions:**
   - **NEXT_SINGLE_ACTION:** GPT-5.6 Luna High independently arbitrate the exact original-vs-host semantic mismatch before any production fix.
   - **NEXT_RECOMMENDED_MODEL:** GPT-5.6 Luna High. No fix, commit, or push.

### Corrected/Superseded — `800979D8` bug versus current `input+0x4` root (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/` is primary
and hash-valid: manifest SHA `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`;
all `SHA256SUMS` entries pass. The recovered MIPS body `0x800979D8..0x80097A4C`
and its post-`isEnabled` body are complete, with proven original semantics.

`STATIC_IMPLEMENTATION_MISMATCH_800979D8=PROVEN`: original `0x80097990`
writes `input+0x4`, while the host uses `bainput_enable` (writes `input+0x0`);
original `0x80097978` reads `input+0x4` at `0x80097984`, while the host's
second query uses `bainput_isEnabled`/`input+0x0`.

### Canonical Checkpoint — `input+0x4` First Temporal Writer & Host `800979D8` Causal Proof (2026-08-25)

Artifact `/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/` is valid;
`NEW_RAW_CAPTURE`, manifest SHA is `3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`,
and all 10 `SHA256SUMS` entries pass (dual verification).

1. **First Temporal Writer Event Identified:**
   - At `glidx 22`, occurrence `func_800979D8(self, button_id=10, input_id=0)`:
     - `held == 1` (`bakey_held` returns 1) and `enabled == 0` (`bainput_isEnabled` returns 0) in BOTH runtimes.
     - `*(input + 0x4)` is `0xFFFFFFFF` (bit 0 = 1) in BOTH runtimes immediately before the call.
     - **AN (Original MIPS):** Takes branch `0x80097A04` fallthrough (since `enabled == 0`) and executes `0x80097A10: jal func_80097990(self, 0, 0)`. `func_80097990` executes CLEAR store at `0x800979CC`, transforming `*(input + 0x4)` from `0xFFFFFFFF` (bit 0 = 1) to `0xFFFFFFFE` (bit 0 = 0).
     - **RT (Host C++ Replacement):** `tooie_bainput_func_800979D8` mistakenly calls `bainput_enable(self, 0, 0)` at `0x8009817C` (which writes `*(input + 0x0)`) instead of `func_80097990` (which writes `*(input + 0x4)`). As a result, `*(input + 0x4)` in RT64 **is never modified and remains `0xFFFFFFFF`** (bit 0 = 1).
   - This single host replacement defect at `glidx 22` is the **EXACT CAUSAL PRODUCER** of the `input+0x4` state divergence.

2. **Downstream Target Divergence at `glidx 23`:**
   - At `glidx 23`, `bainput_enable(self, 0, 1)` updates `*(input + 0x0)` to `0xFFFFFFBF` in both runtimes.
   - When `func_800979D8(self, button_id=10, input_id=0)` is called:
     - `held == 1`, `enabled == 1`.
     - **AN:** Executes `0x80097A1C: jal func_80097978(self, 0)`. `func_80097978` reads `*(input + 0x4) = 0xFFFFFFFE` (bit 0 = 0) -> returns `$v0 = 0`. Controlling branch `0x800A4928` branches over camera rotation left (`p60+0xC = 0`).
     - **RT:** Host replacement calls `bainput_isEnabled(self, 0)` on `*(input + 0x0) = 0xFFFFFFBF` (bit 0 = 1) -> returns `ctx->r2 = 1`. Controlling branch `0x800A4928` falls through to `func_80112D90`, setting `p60+0xC = 1` (unwanted camera rotation left).

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

## Final root checkpoint — `800979D8` first `input+4` divergence (2026-08-25)

**CORRECTED/SUPERSEDES** the earlier pending-arbitration wording and any
claim that complete external input state matched at glidx 22. The verified
`NEW_RAW_CAPTURE` is `/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/`
with manifest SHA `3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`;
all checksum entries pass.

- Semantic join is **PROVEN** for `stage=0/sub=5/map=296/mode=5`, player
  semantic self, `btn=10`, `input_id=0`; both runtimes call `800979D8`.
- At glidx22, held `1/1`, enabled `0/0`, and pre-write `input+4`
  `ffffffff/ffffffff` match. AN then performs `func_80097990(self,0,0)`;
  RT calls `bainput_enable(self,0,0)` on `input+0`.
- `PRE_WRITE_STATE_GLIDX22` matches; `POST_WRITE_STATE_GLIDX22` is AN
  `fffffffe` versus RT `ffffffff`. Therefore
  `HOST_800979D8_MISMATCH_CAUSES_INPUT_PLUS4_FIRST_DIFF=PROVEN` and
  `ROOT_CAUSE_FOUND=YES`.
- Definitive stores from raw opcodes: SET `0x800979B0` (`ac590004`), CLEAR
  `0x800979CC` (`ac4c0004`). The `0x80097A10` callsite has delay slot
  `0x80097A14`; `0x80097A18` is the `jal` return address. Earlier labels
  assigning CLEAR to `0x800979B0` or a store to `0x800979C4` are
  **CORRECTED/SUPERSEDED**.
- At glidx23, AN's `func_80097978` reads `input+4` and returns `0`; RT's
  second `bainput_isEnabled` reads `input+0` and returns `1`. This makes
  `80097B70=0/1` and `0x800A4928=TAKEN/NOT_TAKEN`; camera-chain activity is
  downstream.
- `HOST_MISMATCH_WRITE_HELPER=PROVEN` and `HOST_MISMATCH_READ_HELPER=PROVEN`;
  both are wrong callee substitutions in one replacement and must be fixed
  together. Exact `func_80097990/97978` public generated helpers were not
  found; existing host `bainput_enable/isEnabled` are not equivalent.
- A glidx23-only query fix is insufficient. A full faithful replacement is
  **EXPECTED** to restore this chain, but no runtime PASS is claimed.

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

Este bloco **CORRIGE/SUPERSEDE** o checkpoint anterior somente quanto ao
escopo, proveniência, classificação dos gates e trabalho restante. O histórico
anterior é preservado.

Artifact arbitrado: `/tmp/p08-demo5-800979d8-fix-validation-20260825T235800Z/`
com `MANIFEST_SHA=9a977b6e1c4c276dd2b6616008723d4b3c7139b9d1c4bd6c519c2f23ae3373da`.
`SHA256SUMS` confere integralmente (16/16 arquivos `OK`).

### Veredicto causal

- `RT_RAW_PROVENANCE: NEW_RAW_CAPTURE_POST_FIX`.
- `AN_RAW_PROVENANCE: REFERENCE_ORACLE_CAPTURE_REUSED`.
- `REFERENCE_ORACLE_REUSE_VALID: YES`: `oracle.log` é byte-identical ao
  `/tmp/oracle_fresh3.log`; os timestamps mostram a captura Oracle depois da
  instrumentação e antes do fix de produção. A reutilização é válida para este
  artifact delimitado; não prova um estado externo ilimitado.
- A implementação atual é semanticamente equivalente ao MIPS: no caminho
  não-held faz SET em `input+4` e retorna zero; no caminho held consulta
  `input+0`, faz CLEAR em `input+4` quando disabled, e consulta/retorna o bit
  de `input+4`. O shift usa os mesmos 5 bits baixos e o retorno preserva a
  extensão de sinal de 32 bits.
- `USER_DEMO5_TARGET_CHAIN_PARITY: PROVEN` — parity bit a bit do chain focal,
  não do jogo inteiro: `800979D8 → 80097B70 → 800A4928 → câmera/80112D90 →
  p60+0xC → yaw/EEBF0/EE780`.
- `GLOBAL_GAME_PARITY: NOT_PROVEN`; não foi feita nova captura nem long
  playthrough nesta rodada.

Gates críticos: `DIRECTLY_SUPPORTED` para held/enabled, old/new `input+4`,
operação CLEAR/SET, retornos de `800979D8`/`80097B70`, branch `800A4928`,
`p60+0xC`, yaw, `EEBF0/EE780` e regressão btn13; `DERIVED_BUT_PROVEN` para a
junção causal normalizada e a ausência de dispatch de rotação esquerda em
`80112D90`; `UNSUPPORTED` somente para parity global, long playthrough e
validação Windows/controle físico (ainda não executados).

### Residuais e status

- `P08_STATUS: ACTIVE / MANUAL_VALIDATION_PENDING`.
- `P09_STATUS: PENDING / NOT_FORMALLY_OPEN`.
- `USER_DEMO5_STATUS: RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`.
- `AUDIO_STARTUP_TICK: P08_RESIDUAL` (observado pelo usuário; não é prova de
  regressão causal deste chain).
- `WIDESCREEN_EDGE_GEOMETRY: POST_P08_ENHANCEMENT` (conteúdo original 4:3,
  não um gate de parity do alvo).
- `CHARACTER_EDGE_GRAPHICS_GLITCH: P08_RESIDUAL` (observado, ainda sem
  reprodução causal formal).
- `LONG_PLAYTHROUGH_VALIDATION: MANUAL_VALIDATION_PENDING`.
- `WINDOWS_GAMEPAD_VALIDATION: BLOCKING_P08 / MANUAL_VALIDATION_PENDING`.

Remaining P08: executar a checklist formal com Windows e controle físico;
depois realizar o long playthrough/regressão ampla; em paralelo, triagem
separada do tic de áudio e do glitch gráfico. A geometria de borda widescreen
fica fora do gate P08 e é enhancement posterior.

`CURRENT_FIX_READY_FOR_COMMIT: YES` para o fix focal, condicionado à revisão
do worktree: a produção está em `src/game/boot_hw_stubs.cpp`; os cinco docs
de evidência contêm este checkpoint; os artifacts em `/tmp` não são commitáveis.
O worktree ainda mistura instrumentação/diagnóstico e documentação histórica,
portanto não está sendo declarado como commit único limpo. `BUILD_THIS_ROUND:
NOT_RUN`; nenhum PASS de build é inferido. Nenhum commit/push foi feito.

`NEXT_SINGLE_ACTION: executar a validação formal Windows + controle físico e
registrar o resultado antes de abrir P09.`
