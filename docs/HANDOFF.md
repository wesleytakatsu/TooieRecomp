---
title: "Handoff — TooieRecomp"
description: "Estado global corrente do port: USER_BUG_A WASD runtime resolved; first-straight 510 and first analog transition (glidx 74..91) PASS; USER_BUG_B diagnosed on one sucoaster chdiggerground (Level B); full controller still pending."
status: current
type: handoff
project: TooieRecomp
updated: 2026-08-16
tags: [tooie-recomp, handoff, p08, intro, attract, slot7, lifecycle, bcf28, 80013a7c, visible-demo, matrix-fix, transform-hooks, input, stick-angle]
---

# Handoff — TooieRecomp

## Prioridade atual

**P08 ACTIVE INTEGRATION STATE:** PATCH 4 is accepted and default ON. The
`func_800BCF28` binding fix is **ACCEPTED (verdict B)**. The
`func_80013A7C` lookup-zero is **RESOLVED**
([[plan/evidence/p08-core1-ra-wrapper-fix]]). **Visible attract / gameplay
demo is reproduced** (mode-5, map 335 Wooded Hollow). The **Tooie render-submission
matrix fix is RESOLVED and VALIDATED** ([[plan/evidence/p08-render-submission-matrix-fix]]):
`func_800E44FC`, `func_800E4640`, and `func_800E4628` native overrides split
the projection matrix submission into `LOAD P` followed by `MUL V_view`.
**Title Screen / File Selection Input (P08-T10)** is **VALIDATED and COMPLETE**
([[plan/evidence/p08-title-file-select-validation]]): autonomous synthetic input
proves zero input divergence from intro -> file select -> slot navigation -> confirmation -> Spiral Mountain map load.
**Original 4:3 / Timing Baseline (P08-T03)** is **VALIDATED and COMPLETE**
([[plan/evidence/p08-original-timing-4x3-baseline]]): 60.0 Hz guest VI, 20/30 Hz logic,
30 Hz animation, host decouple (~935 Hz), and 4:3 content aspect confirmed under clean baseline config.

Historical context:
> **PATCH 4 (2026-08-13):** Tooie-scoped startup ON;
> `ULTRAMODERN_GUEST_PREEMPTION=0` rollback. Regen real (4298 safepoints).
> ON: C2AB8 retorna, áudio avança, crash em `func_800BCF28`.
> OFF: freeze antigo (d/audio/ovl estáveis ≥58s). Sem yield/clear.
> Evidência: [[plan/evidence/p08-preemption-patch4-validation]].

Handoff técnico: [[plan/evidence/p08-session-handoff]].

## P08 forced-facing independent review (2026-08-16)

The earlier Level B diagnosis of the Wooded Hollow `chdiggerground` return
actor is **CORRECTED/SUPERSEDED** as a proven recompilation defect. The
return state/animation and 180° logical-vs-travel observation are real, but
the original return-facing contract and selected node index were not proven.
The generated `func_8080264C` and `func_8010CD50` bodies remain faithful on
the reviewed control flow; no fix was implemented.

See [[plan/evidence/p08-forced-cinematic-facing-independent-review]].
`USER_BUG_B_THIS_ACTOR=UNKNOWN`; `USER_BUG_B_GLOBAL=OPEN`; `ROOT_CAUSE_LEVEL=C`.

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
FUNC_80802760_GENERATED_MATCH: YES (funcs_262.c body structurally faithful)
ORIGINAL_MIPS_INTENT:   UNKNOWN (no ROM/ELF in workspace to disassemble)
FIX_IMPLEMENTED:        NO
```

The `keep old yaw via -1` hypothesis is REFUTED. The 180° freeze is the
return-leg handler never updating yaw during the reverse walk. Whether the
original BT code does the same is undecidable here without an original-MIPS
dump. No fix is authorized.
## NEXT

1. ~~First straight & first later analog transition~~ **COMPLETE (2026-08-15)**
   ([[plan/evidence/p08-demo-movement-precision-diagnosis]] §14, §15):
   first straight 510 is original downhill slope-scaled walk speed
   (`FIRST_STRAIGHT_INTERNAL_FIDELITY = PASS`). First later analog transition
   (`glidx = 74..91`, `(-63, 74)`) is internally faithful across all stages
   (`func_80013ABC` table arcsin, bastick distance, camera-relative yaw,
   `yaw_update` easing, velocity integration; `SELECTED_TRANSITION_INTERNAL_FIDELITY = PASS`).
   Demo-path residual is DEFERRED_PENDING_SPECIFIC_VISUAL_SEGMENT.
2. Analog/gamepad validation of USER_BUG_A remains PENDING.
3. Separately diagnose the file-select → Spiral Mountain regression
   (CONT_A confirm still ends in mode=3, not mode=1). Do not use that flow
   as a gameplay automation gate until it is fixed.
4. USER_BUG_B one-actor diagnosis COMPLETE (Level B)
   ([[plan/evidence/p08-forced-cinematic-facing-diagnosis]]): Wooded Hollow
   sucoaster `chdiggerground` reverse walk, heading 294.21° vs yaw 114.21°.
   Independent review before any facing fix. Do not implement transform tagging.

Do not reopen BCF28, Hybrid F' / PATCH 1–4, the core1 RA wrappers,
Actor.rotation, the render-submission matrix split, the title/file-select
input validation, or the original timing/4:3 baseline. Do not modify shared
RT64. PATCH 4 stays ON.

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
    Autonomous end-to-end synthetic input validation proves full flow with zero divergence
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
    is USER_BUG_B, now diagnosed on one actor (Level B;
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

0. [[plan/evidence/p08-forced-cinematic-facing-diagnosis]] (**CURRENT** — USER_BUG_B one-actor facing)
0a. [[plan/evidence/p08-demo-movement-precision-diagnosis]] (demo path residual; first straight + first analog PASS)
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
