---
title: "P09 high-rate force-current-frame diagnostic"
description: "V5C SAME plus original-framerate FIXED; split extra-frame rendering from temporal weight interpolation without dropping high refresh."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-09
tags: [tooie-recomp, p09, kazooie, feather, interpolation, high-framerate, force-current]
---

# P09 high-rate force-current-frame diagnostic

Schema: `P09_HIGH_RATE_FORCE_CURRENT_FRAME`.

**Later correction (2026-09-09):** User visual of this diagnostic is
`FIXED`. Projection-only isolation that followed is
`INVALID_TEMPORALLY_INCOHERENT` and was removed. Canonical:
[[p09-temporal-coherency-root-cause]]. Do not productionize this env.

Default-off. No GPU readback, copy, layout, sync, staging, or occlusion
query. No production interpolation change. Game was not launched by the
agent.

## USER_OBSERVED (this session)

High framerate (existing graphics setting):

- some frames completely correct
- walking: ~30–50% of visible frames stretch a feather ~10–30× into a
  thin red/yellow streak (`USER_OBSERVED`, not measured)
- base stays attached; geometry does not disappear
- stationary glitch rare (~2% `USER_OBSERVED`)

```text
V5C_VISUAL_RESULT: SAME
ORIGINAL_FRAMERATE_VISUAL_RESULT: FIXED
HIGH_FRAMERATE_DEPENDENCE: CONFIRMED
```

V5C froze TransformProcessor world matrices for bones
`79,100–105,107,108` to CURRENT. That is now
`REFUTED_FOR_ALL_TRANSFORMS_DIRECTLY_USED_BY_VISIBLE_FEATHER_TRIANGLES`.
It does **not** equate original-framerate `FIXED` with
TransformProcessor interpolation.

Do not reopen: authored keys, sample time, clip transitions, AE8A8,
bone 107/108 animation, AUTO identity, CPU handoff, worldIndex,
culling, depth, texture, V5C bone coverage.

## Task 1 — High-rate frame path (`PROVEN_STATIC`)

Original-rate (`UserConfiguration::RefreshRate::Original`):

```text
ORIGINAL_RATE_FRAME_PATH:
WorkloadQueue config sets targetRate=0
requiresFrameMatching=false (unless raytracing)
generateInterpolatedFrames=false
displayFrames=1
curFrameWeight=1 prevFrameWeight=0
threadRenderFrame once
PresentQueue presents the current color target
```

High-rate extra frames (`RefreshRate::Display` or `Manual` with
`targetRate > viOriginalRate`):

```text
HIGH_RATE_EXTRA_FRAME_PATH:
targetRate = swapChainRate or refreshRateTarget
requiresFrameMatching=true
GameFrame::match(prevFrame)
generateInterpolatedFrames = !paused && targetRate>viOriginalRate && interpolation FB exists
logicalTicks += targetRate
displayFrames = (logicalTicks-displayTicks)/viOriginalRate  (typically >1)
for each display frame:
  prevFrameWeight/curFrameWeight from tick ratio
  extra frames (frame>0 or MSAA) render into interpolatedColorTargets
  threadRenderFrame(cur, prev, weights, ...)
PresentQueue presents interpolated targets at targetRate
```

```text
FIRST_CODE_PATH_USED_ONLY_OR_DIFFERENTLY_FOR_EXTRA_FRAMES:
WorkloadQueue::renderThreadLoop generateInterpolatedFrames loop
(displayFrames>1, interpolatedColorTargets override, PresentQueue extra presents)
```

`PresentQueue::skipInterpolation()` is **not** a high-rate current-only
mode. It marks interpolated presents as already presented when the
present queue is behind. Original-framerate mode removes extra frames
and cannot distinguish interpolation math from extra-frame rendering.

## Task 2 — Frame-weight consumers (`PROVEN_STATIC`)

All reachable from `WorkloadQueue::threadRenderFrame`:

| component | source | previous data | V5C neutralized | prior P09 closure | remaining |
| --- | --- | --- | --- | --- | --- |
| TRANSFORM | `rt64_transform_processor.cpp` `process` | matched prev world matrix via `RigidBody::lerp(curFrameWeight)` | only bones 79,100–105,107,108 | V5C visual SAME | other world transforms still lerp |
| PROJECTION | `rt64_projection_processor.cpp` `process` | prev view/proj via `RigidBody::lerp` / `lerpMatrix` | no | camera Q' closed as local mismatch | camera/view lerp still live on extra frames |
| TILE | `rt64_tile_processor.cpp` `process` | prev uls/ult/lrs/lrt | no | feather ST static ROM | not feather-causal by prior evidence; still a weight consumer |
| LOOKAT | `rt64_look_at_processor.cpp` `process` | prev look-at delta | no | not a prior feather root | still a weight consumer |
| RSP | `rt64_rsp_processor.cpp` + `RSPProcessCS.hlsl` | `pos - vel*(1-curFrameWeight)`, `tc - tcVel*(1-w)` | no (feather vel=0 proven) | RSPProcess path CLOSED as local mismatch | weight still consumed; vel=0 makes it CURRENT for feathers |
| VERTEX | `rt64_vertex_processor.cpp` + `RSPWorldCS.hlsl` | same vel term; `prevWorldMats` | no | world-index CLOSED | still a weight consumer for non-zero vel / prev mats |
| FRAMEBUFFER/PRESENT | `interpolatedColorTargets` + `PresentQueue::threadPresent` | extra RT copies of the *re-rendered* frame, not a color lerp of prev/cur | no | draw-temporal association CLOSED | extra-frame resource reuse remains |
| OTHER | `GameFrame::match` | pairs prev workload/transforms | no | AUTO identity CORRECT | matching still runs at high rate |

```text
V5C_WAS_EQUIVALENT_TO_GLOBAL_CURRENT_FRAME: NO
```

V5C only replaced selected TransformProcessor outputs. Projection, RSP,
Vertex, Tile, LookAt, extra interpolated targets, and GameFrame matching
continued.

## Task 3 — Existing high-rate no-interpolation mode

```text
EXISTING_HIGH_RATE_NO_INTERPOLATION_MODE: NO
```

- `RefreshRate::Original` (`targetRate=0`) removes extra frames.
- `PresentQueue::skipInterpolation()` drops extra presents when behind.
- V5C `TOOIE_DIAG_FEATHER_NO_INTERP` is per-transform, not global.

## Task 4 — Diagnostic

Env (cached, default off; either name):

```text
TOOIE_DIAG_FORCE_CURRENT_FRAME=1
RT64_DIAG_FORCE_CURRENT_FRAME=1
```

At `WorkloadQueue::threadRenderFrame` entry, before processors:

```text
curFrameWeight = 1.0
prevFrameWeight = 0.0
```

Preserved: `targetRate`, `displayFrames`, extra `threadRenderFrame`
calls, `interpolatedColorTargets`, present cadence, game logic.

Not preserved as interpolation: the two weight floats consumed by
Projection/Transform/Tile/LookAt/RSP/Vertex.

Status: `/tmp/tooie-force-current-frame-status.txt`.

Separate previous-frame paths **not** controlled by these weights:

- `GameFrame::match` (pairing)
- extra interpolated render targets and their presents
- velocity uploader used by matching

Those remain live so this experiment can distinguish A (weight math)
from B (extra-frame render/resource reuse).

## Task 5 — Logic-level proof (before user visual)

With the diagnostic on:

- extra-frame loop still runs (`displayFrames` unchanged)
- every `threadRenderFrame` processor receives `cur=1 prev=0`
- `RigidBody::lerp(1.0, prev, cur)` is the CURRENT endpoint
- `RSPProcessCS`/`RSPWorldCS` `vel*(1-1)=0`

Runtime confirmation is the status file after the user run:

```text
DIAG_ENABLED=YES
HIGH_RATE_ACTIVE=YES
EXTRA_FRAMES_STILL_RENDERED=YES
EFFECTIVE_CUR_WEIGHT=1.000000
EFFECTIVE_PREV_WEIGHT=0.000000
ALL_TEMPORAL_PROCESSORS_RECEIVE_CURRENT_ONLY=YES
```

Do not treat the visual as causal if `HIGH_RATE_ACTIVE=NO` or
`EXTRA_FRAMES_STILL_RENDERED=NO`.

## Decision tree (after user visual)

- `FIXED`: `HIGH_FRAMERATE_TEMPORAL_INTERPOLATION_CLASS=CAUSAL`; isolate
  Projection / RSP / Vertex / Tile / LookAt / other. Not a production fix.
- `BETTER`: weights contribute; another extra-frame mechanism remains.
- `SAME`: frame-weight interpolation is not causal; next class
  `HIGH_FRAMERATE_EXTRA_RENDER_OR_RESOURCE_REUSE_PATH`.

```text
PRODUCTION_FIX_CANDIDATE_FOUND: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```

## Later correction (2026-09-09, USER_OBSERVED)

```text
GLOBAL_CURRENT_FRAME_RESULT: FIXED
HIGH_FRAMERATE_TEMPORAL_INTERPOLATION_CLASS: CAUSAL
```

Isolation continues in
[[p09-high-rate-force-current-projection]]. Do not promote the global
diagnostic to a production fix.
