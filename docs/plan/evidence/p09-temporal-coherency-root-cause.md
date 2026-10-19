---
title: "P09 temporal coherency root-cause analysis"
description: "Projection-only CURRENT is INVALID mixed-time; global CURRENT FIXED; identify coupled interpolation invariant. No new visual A/B."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-09
tags: [tooie-recomp, p09, kazooie, feather, interpolation, high-framerate, coherency]
---

# P09 temporal coherency root-cause analysis

Schema: `P09_TEMPORAL_COHERENCY_ROOT_CAUSE`.

No GPU readback, copy, layout, sync, staging, or occlusion query.
No production interpolation change. Game was not launched by the agent.
Projection-only diagnostic code was removed this session.

## USER_OBSERVED (authoritative)

```text
NORMAL_HIGH_RATE: GLITCH
NORMAL_HIGH_RATE_AFTER_PROJECTION_EXPERIMENT: GLITCH
ORIGINAL_FRAMERATE: FIXED
V5C_ALL_9_VISIBLE_WORLD_TRANSFORMS_CURRENT: SAME
GLOBAL_CURRENT_FRAME (TOOIE_DIAG_FORCE_CURRENT_FRAME=1): FIXED
PROJECTION_ONLY_CURRENT: INVALID_TEMPORALLY_INCOHERENT
  (original glitch plus severe frame ghosting/overlay)
PROJECTION_DIAG_LEAKED_INTO_DEFAULT: NO
```

Do not treat projection-only as causal evidence that
`ProjectionProcessor` is the original root. It mixed:

```text
viewProj sample time = CURRENT
world / RSP / Vertex / Tile / LookAt sample time = interpolated
```

That mixed-time state is **not** the original high-rate path.

Do not continue per-component CURRENT overrides. The remaining class is independent VIEW and WORLD interpolation of
coupled relative motion; see
[[p09-relative-view-world-interpolation-root-cause]].
World-only DECOMPOSE is `CORRECTED/SUPERSEDED`.

`TOOIE_DIAG_FORCE_CURRENT_FRAME` stays diagnostic-only. It is **not** a
production fix: it keeps extra high-rate presents but repeats current
game-frame samples.

## Task 1 — Projection-only diagnostic removed (`PROVEN`)

Removed from `BanjoRecomp/lib/rt64/src/hle/rt64_workload_queue.cpp`:

- `TOOIE_DIAG_FORCE_CURRENT_PROJECTION` / `RT64_DIAG_FORCE_CURRENT_PROJECTION`
- dedicated stats / `/tmp/tooie-force-current-projection-status.txt`
- split `projectionCurFrameWeight` path

Kept: `TOOIE_DIAG_FORCE_CURRENT_FRAME`, V5C `TOOIE_DIAG_FEATHER_NO_INTERP`.

Historical evidence [[p09-high-rate-force-current-projection]] is
`INVALID_TEMPORALLY_INCOHERENT`.

## Raster position equation (`PROVEN_STATIC`)

Ordinary raster (`RasterVS`) consumes `RSPProcessCS` screen positions:

```text
tfPos = viewProjMat(w) * worldMat(w) * (pos - vel * (1 - w))
screen = ndc(tfPos) * viewport
```

`viewProjMat(w)` is `ProjectionProcessor` output
(`RigidBody::lerp` view, optional `lerpMatrix` proj, then `mul`).

`worldMat(w)` is `TransformProcessor` `lerpWorldTransforms`
(`RigidBody::lerp(w, prevWorld, curWorld)`).

`w = curFrameWeight` is **shared** on the original high-rate path
across Projection, Transform, Tile, LookAt, RSP, Vertex.

`RSPWorldCS` is **not** the ordinary raster POSITION source. It writes
world pos/norm and `dstVel = worldPos - prevWorldPos` for raytracing
(`RT_ENABLED`). Visible feather raster does not sample `prevWorldMats`
for screen XY.

## Coupled temporal domains (`PROVEN_STATIC`)

These must represent the **same interpolation sample** `w`:

| Domain | Producer | Consumed by raster as |
| --- | --- | --- |
| view / proj | ProjectionProcessor | `viewProjMat` |
| world | TransformProcessor | `worldMat` (and `prevWorld` only for RT motion) |
| local vel | G_VTX empty + optional `GameFrame::matchTransform` | `pos - vel*(1-w)` |
| match pairing | `GameFrame::match` | which prev view/world/call is `prev` |

Invariant (`PROVEN` by projection-only catastrophe + global-current
`FIXED`):

```text
A triangle must not combine viewProj(w_a) with world(w_b) for w_a != w_b.
```

The original high-rate bug already uses `w_a = w_b = w` (coherent
weights). Global current uses `w=1` for every domain. Therefore the
remaining class is **interpolation at intermediate w**, or **match
pairing of prev resources**, not a mixed weight knob.

V5C `SAME` only froze token-bound first occurrences of the nine
logical bones. It does **not** prove that interpolating every
concrete world slot used by visible triangles is harmless. Later:
[[p09-world-occurrence-identity-root-cause-audit]].

## Velocity (`PROVEN_STATIC`, capture scope restated)

Where vel=0 was observed:

1. F3DEX2 `RSP::setVertexCommon<addEmptyVelocity=true>` writes
   `velFloats = (0,0,0)` at G_VTX (`rt64_rsp.cpp`).
2. Target DLs do not use `G_MODIFYVTX` (prior P09, `CLOSED`).
3. `TransformGroup::vertexInterpolation` defaults to
   `G_EX_COMPONENT_SKIP`. Tooie does not emit `gEXMatrixGroup`, so
   AUTO groups keep SKIP.
4. `GameFrame::matchTransform` only overwrites `velFloats` with
   `(curPos - prevPos)` when `vertexInterpolation != SKIP` and vertex
   hashes differ.

Therefore post-match shader `srcVel` stays 0 on this path
(`STRONG_INFERENCE` from default group + no extended GBI; not a new
GPU capture).

Exact RSPProcessCS arithmetic with `vel = 0`:

```text
tfPos = viewProjMat(w) * worldMat(w) * pos
```

The `(1-w)` vel term is a no-op. `prevWorldMats` is not sampled.
`RSPWorldCS` still computes `dstVel` from `prevWorldMats` even with
`vel=0`, but that buffer is not raster POSITION.

```text
ZERO_VELOCITY_PROVES_RSP_VERTEX_TEMPORAL_NOOP: NO
```

Vel=0 only proves the **extra model-space displacement** is zero. RSP
still multiplies interpolated `viewProjMat` and `worldMat`. Do not
treat RSP/Vertex as temporally inert.

## Double interpolation (`PROVEN_STATIC`)

```text
TEMPORAL_POSITION_PROCESSING_STAGES:
  1. GameFrame::match pairs prev/cur viewProj and world (and may fill vel)
  2. ProjectionProcessor: view' = RigidBody::lerp(w, prevView, curView)
                         proj' = lerpMatrix(prevProj, curProj, w) if view lerp on
                         viewProj' = view' * proj'
  3. TransformProcessor: world' = RigidBody::lerp(w, prevWorld, curWorld)
                         prevWorld' = RigidBody::lerp(1-w, prevWorld, curWorld)
  4. RSPProcessCS: clip = viewProj' * world' * (pos - vel*(1-w))
  5. RSPWorldCS (non-raster): worldPos = world' * (pos - vel*(1-w))
                              dstVel = worldPos - prevWorld'*(pos - vel*w)
```

```text
DOUBLE_INTERPOLATION_POSSIBLE: YES
```

If `vertexInterpolation != SKIP`, stage 1 writes model-space `vel`
from matched previous vertices **and** stage 3 already interpolates
the bone matrix. Then stage 4 applies both. On Tooie AUTO default SKIP
that vel rewrite does **not** run, so this double path is
`HYPOTHESIS`/`UNAVAILABLE` for live 0x62B feathers unless a group
overrides SKIP.

A second double-capable coupling **does** run on the original path:
`world'` is already at time `w` and `viewProj'` is independently
lerped to time `w`. That is required coherency, not a second sample.
It becomes a stretch if **match** pairs a current draw with a
**stale or different-identity** previous matrix, because lerp(w,
wrongPrev, cur) is a huge interpolated pose.

```text
MIXED_PREVIOUS_CURRENT_RESOURCE_POSSIBLE: YES
```

Sources:

- `GameFrame::match` AUTO pairing (`matrixId = G_EX_ID_AUTO`) of
  viewProjections and world transforms.
- JOIN DLs mix child/parent world indices in one triangle (by design;
  V5C froze both).
- Nested `0x03000040` leftover-slot candidate (prior V6; not reopened
  as proven live).
- Projection-only diagnostic (removed) forced mixed **weights**;
  original path does not.

## Global current vs normal (`PROVEN_STATIC`)

Shared `w` on extra high-rate `threadRenderFrame` calls.

| Processor | Normal high-rate | `FORCE_CURRENT_FRAME` (`w=1`) |
| --- | --- | --- |
| Projection | `view=lerp(w,prev,cur)`, `proj=lerpMatrix` if view lerp | `view=cur`, `proj=cur`, `viewProj=curView*curProj` |
| Transform | `world=lerp(w,prev,cur)` | `world=cur` (RigidBody lerp at 1) |
| RSPProcessCS | `pos - vel*(1-w)` then `viewProj*world` | `pos` (vel term 0) times current matrices |
| Vertex/RSPWorldCS | same vel; `dstVel` from prevWorld' | `prevWorld'` at weight 0 is previous sample, but raster ignores it |
| Tile | lerp uls/ult | current tiles |
| LookAt | lerp look-at X/Y | current; not clip XY |

Smallest mathematical difference capable of a 10–30× streak at
intermediate `w` while V5C currenting the nine worlds does nothing:

```text
clip(w) = viewProj(w) * world_other(w) * pos
```

where `world_other` is **not** one of the nine V5C bones, **or**
`viewProj(w)` is a degenerate/mismatched camera lerp. Either can
produce a long screen-space edge from a small local feather.

Global current collapses both to current resources. V5C collapses
only the nine worlds.

## BanjoRecomp (`PROVEN` architectural gap, not a port)

BanjoRecomp `patches/transform_tagging.c` +
`patches/projection_transform_tagging.c` emit `gEXMatrixGroup` with
stable IDs so `GameFrame::match` pairs the same logical bone/camera.

Tooie `func_800E44FC` splits P LOAD then V MUL to avoid combined
viewproj decomposition. It does **not** emit group IDs. Live 0x62B is
AUTO.

```text
BANJORECOMP_RELEVANT_DIFFERENCE: explicit matrix/projection group IDs
prevent AUTO match from pairing unrelated prev view/world. This is
the structural guard against mixed-identity lerp. It is NOT a ready
P09 production port (prior tagging audit stands).
```

Do not assume the answer is “missing patch”. Do not permanently
disable interpolation.

## No new visual experiment

Per-component CURRENT overrides are forbidden. A later test is valid
only if every dependent domain keeps the same `w`.

```text
NEW_RUNTIME_TEST_REQUIRED: YES
```

One coherent next test (CPU-only, default-off, no weight change):

After `TransformProcessor`/`ProjectionProcessor` on a high-rate
interpolated frame, write a status file for the already-identified
visible-feather face vertices:

- unique `worldIndices` vs V5C set `{79,100–105,107,108}`
- unique `viewProjIndices` and whether `ViewProjectionMap.mapped`
- max `|velFloats|` after match
- `vertexInterpolation` of those groups
- whether `RigidBody` decompose was valid for those world/view maps

This does not create a mixed-time render. It decides whether the next
production investigation is leftover/other world indices, unmatched
viewProj, or decompose/slerp of AUTO pairs.

Do not ask the user for another Frankenstein visual.

```text
PRODUCTION_FIX_CANDIDATE_FOUND: NO
CLEANUP_READY: NO
COMMIT_READY: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```
