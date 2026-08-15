---
title: "Handoff — TooieRecomp"
description: "Estado global corrente do port: P08 visible attract/gameplay demo reached; first fidelity gap is RT64 LOAD-path viewproj decompose."
status: current
type: handoff
project: TooieRecomp
updated: 2026-08-14
tags: [tooie-recomp, handoff, p08, intro, attract, slot7, lifecycle, bcf28, 80013a7c, visible-demo, movement]
---

# Handoff — TooieRecomp

## Prioridade atual

**P08 ACTIVE INTEGRATION BLOCKER:** PATCH 4 is accepted and default ON. The
`func_800BCF28` binding fix is **ACCEPTED (verdict B)**. The
`func_80013A7C` lookup-zero is **RESOLVED**
([[plan/evidence/p08-core1-ra-wrapper-fix]]). **Visible attract / gameplay
demo is reproduced** (mode-5, map 335 Wooded Hollow). Character and camera
transform discriminator is complete (**Level B**;
[[plan/evidence/p08-actor-camera-transform-discriminator]]). Render-submission
matrix diagnosis is complete (**Level B**;
[[plan/evidence/p08-render-submission-matrix-diagnosis]]): Tooie LOADs
`T_S * V_rot * P` onto `G_MTX_PROJECTION`; RT64 `matrixDecomposeViewProj`
preserves the clip product but extracts a half-length view Z because
Tooie P is scaled by 0.5. Banjo avoids this path. Fix not implemented.

Historical context:
> **PATCH 4 (2026-08-13):** Tooie-scoped startup ON;
> `ULTRAMODERN_GUEST_PREEMPTION=0` rollback. Regen real (4298 safepoints).
> ON: C2AB8 retorna, áudio avança, crash em `func_800BCF28`.
> OFF: freeze antigo (d/audio/ovl estáveis ≥58s). Sem yield/clear.
> Evidência: [[plan/evidence/p08-preemption-patch4-validation]].

Handoff técnico: [[plan/evidence/p08-session-handoff]].

## NEXT

Implement only the proven render-submission transform fix: split
`func_800E44FC` (and the `D_8012D558` reload) into `LOAD P` + `MUL` view.
Do not reopen BCF28, Hybrid F' / PATCH 1–4, the core1 RA wrappers, or
Actor.rotation. Do not modify shared RT64. PATCH 4 stays ON.

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
   divergence is in `RENDER_SUBMISSION`
   ([[plan/evidence/p08-actor-camera-transform-discriminator]]).
9. **Render-submission matrix diagnosis (Level B):** RT64 LOAD-path
   viewproj decompose vs Tooie combined scaled VP
   ([[plan/evidence/p08-render-submission-matrix-diagnosis]]).
10. Title/file-select input diferido.

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

## Current expected flow

```text
intro -> Jiggy -> gsattract -> map 335/entrance 8 -> type-4 complete
 -> warp 3 consumed -> A9924 -> A76F4(5) -> mode 5 -> A9A14 -> gsattract ep1
 -> gameplay demo
```

Observed: C2AB8 crossed; `func_800BCF28` completes; `func_80013A7C`
enters at the native strong symbol and the old `LOOKUP_FUNC(0)` does not
occur. Visible mode-5 Wooded Hollow / PRESS START reached. Character and
camera simulation state is upright and grounded; RT64 extraction of view Z
from Tooie's combined scaled `G_MTX_PROJECTION` LOAD is the first visual
fidelity divergence.

## Exonerated (current)

Frame-sync, event-3, cothemedll, type-4 stall, warp non-consumption,
`func_80026500` hang, scheduler corruption (not proven), `func_800BCF28` binding,
`func_80013A7C` lookup-zero, `Actor.rotation` simulation state.

## Files to read

0. [[plan/evidence/p08-render-submission-matrix-diagnosis]] (**CURRENT** — render matrix diagnosis)
1. [[plan/evidence/p08-actor-camera-transform-discriminator]] (transform discriminator)
2. [[plan/evidence/p08-visible-demo-movement-diagnosis]] (visible demo + movement diagnosis)
3. [[plan/evidence/p08-core1-ra-wrapper-fix]] (RA-wrapper fix; resolved)
4. [[plan/evidence/p08-80013a7c-lookup-zero-diagnosis]] (Level A diagnosis; resolved)
5. [[plan/evidence/p08-bcf28-binding-fix-independent-review]] (verdict B; BCF28 fix accepted)
6. [[plan/evidence/p08-bcf28-binding-fix]] (implementer binding-fix evidence)
7. [[plan/evidence/p08-bcf28-crash-diagnosis]] (root cause A diagnosis)
8. [[plan/evidence/p08-preemption-patch4-clean-build-independent-review]] (verdict B; PATCH 4 accepted)
9. [[plan/evidence/p08-preemption-patch4-clean-build-correction]] (implementer correction)
10. [[plan/evidence/p08-preemption-patch4-independent-review]] (historical verdict C preserved; finding resolved)
11. [[plan/evidence/p08-preemption-patch4-validation]] (implementer behavioral evidence)
12. [[plan/evidence/p08-session-handoff]]
13. [[plans/BT/08-rt64-input-and-frontend]]
