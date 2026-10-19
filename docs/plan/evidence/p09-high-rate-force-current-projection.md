---
title: "P09 high-rate projection-only current-frame diagnostic"
description: "INVALID mixed-time experiment. User visual was glitch plus severe ghosting. Diagnostic code removed. See temporal-coherency analysis."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-09
tags: [tooie-recomp, p09, kazooie, feather, interpolation, high-framerate, projection, invalid]
---

# P09 high-rate projection-only current-frame diagnostic

**INVALID_TEMPORALLY_INCOHERENT.** Do not use this experiment as
causal evidence that `ProjectionProcessor` is the original P09 root.
User visual: original glitch **plus** severe frame ghosting/overlay.
The diagnostic forced Projection=`CURRENT` while Transform/RSP/Vertex
stayed interpolated. That mixed-time state is not the original
high-rate path. Code (`TOOIE_DIAG_FORCE_CURRENT_PROJECTION`) was
**removed**. Canonical follow-up:
[[p09-temporal-coherency-root-cause]].

Schema: `P09_HIGH_RATE_FORCE_CURRENT_PROJECTION` (historical).

Default-off. No GPU readback, copy, layout, sync, staging, or occlusion
query. No production interpolation change. Game was not launched by the
agent.

## USER_OBSERVED (this session)

```text
HIGH_FRAMERATE_DEPENDENCE: CONFIRMED
GLOBAL_CURRENT_FRAME_RESULT: FIXED
V5C_RESULT: SAME
GLOBAL_FRAME_WEIGHT_INTERPOLATION_CLASS: CAUSAL
SELECTED_WORLD_MATRIX_INTERPOLATION: REFUTED
```

`TOOIE_DIAG_FORCE_CURRENT_FRAME=1` with high refresh and extra
`threadRenderFrame` calls is `FIXED`. V5C (CURRENT world matrices for
bones `79,100–105,107,108`) remains `SAME`. Original-framerate graphics
remain `FIXED`.

The global current-frame diagnostic is **not** a production fix.

Do not reopen: authored keys, sample time, clip transitions, AE8A8,
bone 107/108 animation, AUTO identity, CPU handoff, worldIndex, culling,
depth, texture, V5C bone coverage.

## Task 1 — Static reduction (`PROVEN_STATIC` + prior P09)

Screen position of a visible feather vertex in the normal raster path
(`RSPProcessCS`):

```text
tfPos = viewProjMat * worldMat * (pos - vel * (1 - curFrameWeight))
```

`viewProjMat` is `ProjectionProcessor` `modViewProjTransforms`.
`worldMat` is `TransformProcessor` `lerpWorldTransforms`. Local
velocity on ordinary TransformGroup vertices starts at zero; target
DLs do not use `G_MODIFYVTX`.

| component | CAN_CHANGE_SCREEN_POSITION_OF_VISIBLE_FEATHER_VERTICES | ALREADY_CAUSALLY_REFUTED | SURVIVES_GLOBAL_CURRENT_RESULT |
| --- | --- | --- | --- |
| PROJECTION | YES — `RigidBody::lerp` of view and `lerpMatrix` of proj; output is `viewProjMat` | NO. Prior Q-witness small deltas did **not** include global-current `FIXED`. That witness is weaker than this experiment. | YES |
| TRANSFORM | YES in general | YES for the nine world transforms used by visible feather triangles (V5C `SAME`) | NO for those nine |
| RSP | YES only via `vel*(1-w)` and the matrices it multiplies | YES for the vel term (captured visible-feather velocity = 0). Matrices come from Projection/Transform, not a second lerp. | NO as an independent weight consumer |
| VERTEX (`RSPWorldCS`) | same vel term; world motion, not ordinary raster POSITION | YES for vel=0 on these DLs | NO as an independent raster-position consumer |
| TILE | NO — interpolates `uls/ult/lrs/lrt` | YES as a geometry-stretch cause (ST static ROM; texture forensic CLOSED) | NO |
| LOOKAT | NO — look-at X/Y for lighting/texgen | YES as a geometry-stretch cause (texgen off on plume DLs) | NO |
| FRAMEBUFFER/PRESENT | extra RTs/presents, not a clip-space lerp | extra-frame reuse is **not** the remaining class: extra renders still ran under global CURRENT and the symptom was `FIXED` | NO for the weight-math class |

```text
SURVIVING_POSITION_TEMPORAL_COMPONENTS: ProjectionProcessor view/proj lerp
STRONGEST_COMPONENT_CANDIDATE: ProjectionProcessor
PROJECTION_ONLY_TEST_JUSTIFIED: YES
V5C_WAS_EQUIVALENT_TO_GLOBAL_CURRENT_FRAME: NO
```

V5C froze selected **world** matrices. Global CURRENT also froze
**view/proj**. The new `FIXED` therefore cannot be explained by the nine
feather world transforms.

Tooie `src/game/transform_hooks.cpp` (`func_800E44FC`) splits guest
perspective into `G_MTX_PROJECTION LOAD` then affine view
`G_MTX_PROJECTION MUL` so RT64 does not take the combined-viewproj
decomposition path. It does **not** emit `gEXMatrixGroup` / projection
IDs. BanjoRecomp `patches/projection_transform_tagging.c` assigns
stable IDs (`PROJECTION_GAMEPLAY_TRANSFORM_ID` and HUD/title variants).
That architectural gap is **relevant to a later production investigation**
if this diagnostic is `FIXED`/`BETTER`. It is not this diagnostic and
not a production fix.

The prior Q-witness
([[p09-feather-postprocess-q-witness-v1]]) reported small interpolation
deltas. It is not a causal refute of ProjectionProcessor under high-rate
walking. Do not treat it as stronger than global-current `FIXED`.

## Task 2 — Projection-only diagnostic

Env (cached, default off; either name):

```text
TOOIE_DIAG_FORCE_CURRENT_PROJECTION=1
RT64_DIAG_FORCE_CURRENT_PROJECTION=1
```

Do **not** also set `TOOIE_DIAG_FORCE_CURRENT_FRAME` or
`TOOIE_DIAG_FEATHER_NO_INTERP`.

At `WorkloadQueue::threadRenderFrame`, only
`ProjectionProcessor::ProcessParams` weights become
`cur=1` / `prev=0`. Shared `curFrameWeight`/`prevFrameWeight` stay on
the high-rate interpolation schedule for Transform, Tile, LookAt, RSP,
and Vertex.

Preserved: `targetRate`, `displayFrames`, extra `threadRenderFrame`
calls, `interpolatedColorTargets`, present cadence, game logic.

Status: `/tmp/tooie-force-current-projection-status.txt`.

```text
PROJECTION_ONLY_DIAGNOSTIC_IMPLEMENTED: YES
HIGH_FRAMERATE_PRESERVED: YES (logic; confirm HIGH_RATE_ACTIVE after run)
EXTRA_FRAMES_STILL_RENDERED: YES (logic; confirm after run)
OTHER_TEMPORAL_COMPONENTS_LEFT_NORMAL: YES
```

## Task 3 — Fallback if projection-only is SAME

Do not test Tile or LookAt for a geometry-stretch symptom.

RSP/Vertex weights cannot change these clip positions while
`vel=0`. Do not run them as the next visual A/B.

If the user reports `SAME` **and** the status file shows
`PROJECTION_FORCED_CURRENT=YES`, `TRANSFORM_NORMAL=YES`,
`HIGH_RATE_ACTIVE=YES`, `EXTRA_FRAMES_STILL_RENDERED=YES`:

the remaining class is not a second processor-weight knob. Next is
whether extra-frame **view/proj resource association** still feeds a
previous matrix into `viewProjMat` despite CURRENT weights (match
pairing / buffer contents), without GPU readback.

If `SAME` but `PROJECTION_FORCED_CURRENT=NO` or
`COMBINED_WITH_FORCE_CURRENT_FRAME=YES`, the run is invalid.

No second env is required for this user test.

## Task 4 — Production-fix policy

```text
PRODUCTION_FIX_READY: NO
CLEANUP_READY: NO
COMMIT_READY: NO
```

`TOOIE_DIAG_FORCE_CURRENT_FRAME` stays diagnostic-only.

If projection-only later returns `FIXED`/`BETTER`, investigate a
**correct** projection interpolation:

- projection transform identity / tagging (BK `gEXMatrixGroup` IDs vs
  Tooie AUTO + `transform_hooks` P/V split)
- projection interpolation representation (`RigidBody::lerp` view,
  `lerpMatrix` proj)
- not a permanent global disable of projection interpolation unless no
  correct interpolation exists

Do not port BanjoRecomp `projection_transform_tagging.c` as the
production fix in this step.

## Decision tree (after user visual)

- `FIXED`: ProjectionProcessor interpolation is causal. Next:
  isolate view vs proj lerp / match identity. Not a production commit.
- `BETTER`: projection contributes; another weight or extra-frame
  mechanism remains.
- `SAME` (valid status): frame-weight interpolation of projection is
  not the remaining cause; do not test Tile/LookAt.
- `WORSE`: stop; record and do not broaden.

```text
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```
