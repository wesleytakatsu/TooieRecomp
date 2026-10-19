---
title: "P09 VIEW×WORLD interaction root discriminator"
description: "CLCL is the factorial safe class. CPU world-space JOIN, hierarchical world, and modelview interpolation wait recapture. Diagnostic status write throttled. No production fix."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-15
tags: [tooie-recomp, p09, interpolation, join, view, world, hierarchy, modelview, factorial]
---

# P09 VIEW×WORLD interaction root discriminator

Authoritative factorial recapture (`USER_OBSERVED` / `RUNTIME_CPU`)
closed the 16-way tfPos ablation:

```text
FACTORIAL_ALL_CURRENT_MATCHES_EXISTING_CURRENT=YES
MAX_FACTORIAL_CURRENT_POSITION_DIFF=0
POSITION_TEMPORAL_TERM_CAN_AFFECT_P09=NO
TARGET_POSITION_SAMPLE_COUNT=66495
TARGET_NONZERO_VELOCITY_COUNT=0
JOIN_NONZERO_VELOCITY_COUNT=0
STRONGEST_SINGLE_FACTOR=WORLD
STRONGEST_PAIR=VIEW_WORLD
MINIMUM_CURRENT_FACTOR_SET_MATCHING_ALL_CURRENT_BEHAVIOR=CLCL
FACTORIAL_BEST_POLICY=CLCL
```

LLLL JOIN MAX/P99 = 11.62 / 0.908. CLLL (CURRENT VIEW only)
worsens MAX to 39.4. LLCL (CURRENT WORLD only) stays ~11.3.
LCLL (CURRENT PROJECTION only) matches LIVE. CLCL (CURRENT
VIEW+WORLD, LIVE PROJECTION, LIVE POSITION) reaches the safe
class (JOIN MAX 1.010, P99 1.000). Do **not** ship CURRENT
VIEW+WORLD: that snaps both temporal factors to the current
endpoint.

Production shader/policy unchanged. Game not launched this
session.

## Task 0 — diagnostic progressive cost (`PROVEN` static)

`RSPProcessor::process` always called `p09RspEndpointBindingNote`
under `g_p09RspEndpointBindingMutex` and wrote
`/tmp/tooie-rsp-endpoint-binding-status.txt` (tmp+rename) on
**every** process. `P09ResidualAccum` keeps up to
`kP09ResidualSampleCap=262144` samples plus a sorted copy; ~80
accums (residual + JOIN + 16 factorial × 3). New samples clear
`sortedReady`, so each status write re-sorted until the cap.
`policyAExplosionNormWs` used uncached `p09PercentileCopy` every
process. Session counters and vectors therefore make CPU cost
grow with capture length until the cap, then plateau at a still
large per-process fprintf.

```text
P09_DIAGNOSTIC_PROGRESSIVE_COST_FOUND: YES
P09_DIAGNOSTIC_SAMPLE_MEMORY_GROWS_WITH_SESSION: YES_UNTIL_SAMPLE_CAP_262144
P09_STATUS_WRITE_COST_GROWS_WITH_SESSION: YES_UNTIL_CAP_BEFORE_THROTTLE
CPU_DIAG_STATUS_WRITE_PER_RSP_PROCESS: NO
CPU_DIAG_STATUS_WRITE_THROTTLED_1S: YES
CPU_DIAG_SHARED_STATE_LOCKED: YES
STATUS_WRITE_ATOMIC: YES
```

Throttle: full status generation (percentiles + fprintf +
tmp+rename) at most once per second. Sampling still runs every
process. Mutex/atomic write preserved. No GPU sync. No render
path change.

## Task 1 — world-space JOIN (instrumented)

Same V7 articulated pairs
(`107→103→104→105`, `107→100→101→102`, `108→79`).
Attachment positions are `hlslpp::mul(WORLD, pos4)` before VIEW
and PROJECTION. Relative scale is
`max(prevWorldGap, curWorldGap, 1e-3)` world units, not NDC.

Policies: PREV_WORLD, CURRENT_WORLD, LIVE_WORLD, RIGID_WORLD.

```text
WORLDSPACE_JOIN_PAIR_COUNT: NOT_CAPTURED
PREV_WORLDSPACE_JOIN_MAX: NOT_CAPTURED
CURRENT_WORLDSPACE_JOIN_MAX: NOT_CAPTURED
LIVE_WORLDSPACE_JOIN_MAX: NOT_CAPTURED
LIVE_WORLDSPACE_JOIN_P99: NOT_CAPTURED
LIVE_WORLD_INTERPOLATION_BREAKS_ARTICULATED_ATTACHMENTS: NOT_CAPTURED
```

If LIVE already opens large world-space gaps while endpoints do
not, VIEW is not the root of those gaps.

## Task 2 — HIERARCHICAL_WORLD (CPU diagnostic only)

Local parent→child uses the RT64 column-vector convention:

```text
local = hlslpp::mul(inverse(parentWorld), childWorld)
childWorld = hlslpp::mul(parentWorld, local)
```

Roots 107 and 108 use production LIVE world (`RigidBody::lerp`).
Locals are TRS+SLERP via existing `p09RigidWorldLerp`. Invalid
decompose is counted, not silently replaced. Model-specific
hierarchy is diagnostic-only.

```text
HIERARCHICAL_WORLDSPACE_JOIN_MAX: NOT_CAPTURED
HIERARCHICAL_WORLDSPACE_JOIN_P99: NOT_CAPTURED
HIERARCHICAL_SCREEN_JOIN_MAX: NOT_CAPTURED
HIERARCHICAL_SCREEN_JOIN_P99: NOT_CAPTURED
HIERARCHICAL_WORLD_MATERIALLY_REDUCES_ATTACHMENT_GAP: NOT_CAPTURED
```

Screen JOIN uses LIVE VIEW×PROJECTION with reconstructed child
worlds.

## Task 3 — MODELVIEW_RIGID (CPU diagnostic only)

```text
prevVW = hlslpp::mul(PREV_VIEW, PREV_WORLD)
curVW  = hlslpp::mul(CURRENT_VIEW, CURRENT_WORLD)
interpVW = RigidBody TRS+SLERP when both DecomposedTransform valid
clip = hlslpp::mul(hlslpp::mul(interpVW, LIVE_PROJ), pos4)
```

Invalid decompose is counted; no arbitrary fallback. POSITION is
omitted (`vel=0`). Production tfPos order remains
`mul(mul(mul(VIEW,PROJ),WORLD),pos4)`; this counterfactual tests
interpolating VIEW×WORLD as a product with LIVE projection
applied after.

```text
MODELVIEW_DECOMPOSE_VALID_COUNT: NOT_CAPTURED
MODELVIEW_DECOMPOSE_INVALID_COUNT: NOT_CAPTURED
MODELVIEW_JOIN_MAX: NOT_CAPTURED
MODELVIEW_JOIN_P99: NOT_CAPTURED
MODELVIEW_INTERPOLATION_MATCHES_SAFE_CLASS: NOT_CAPTURED
```

## Task 4 — root class

Runtime classifier (status file after recapture):

- LIVE world-space JOIN broken and HIERARCHICAL_WORLD fixes it
  → `INDEPENDENT_WORLD_HIERARCHY_INTERPOLATION`
- world-space JOIN good and MODELVIEW matches safe class
  → `SEPARATE_VIEW_WORLD_INTERPOLATION`
- both contribute → `BOTH`
- else → `UNKNOWN`

```text
VIEW_WORLD_INTERACTION_ROOT_CLASS: NOT_CAPTURED
```

## Task 5 — genericity (`PROVEN` static)

`TransformGroup` is interpolation policy + `matrixId` for
occurrence matching. `worldTransformGroups` indexes that policy.
RSP has a matrix stack at GBI time; uploaded `worldTransforms`
are already flattened worlds. There is no stored parent index on
interpolated world slots.

Modelview interpolation is feasible on CPU without GPU readback:
per used `(viewProj, world)` pair, form endpoint VIEW×WORLD
products and interpolate one 4x4 instead of interpolating VIEW
and WORLD independently. Extra cost is two matmuls plus one
decompose/SLERP per pair (same class as current world lerp).
Shared-camera objects would no longer share one interpolated
VIEW; that is the point of the test, and it can change relative
motion of unrelated draws that share a camera.

```text
GENERIC_HIERARCHY_INFORMATION_AVAILABLE: NO
GENERIC_MODELVIEW_INTERPOLATION_FEASIBLE: YES_CPU_PER_VP_WORLD
PREFERRED_GENERIC_FIX_LAYER: WAIT_COUNTERFACTUAL
  hierarchy win without generic parentage → architecture review
    (do not hardcode Tooie bone IDs)
  modelview win → TransformProcessor + ProjectionProcessor
    combined product, not RSPProcessCS
```

## Task 6

No production change. Do not implement CURRENT VIEW+WORLD.

```text
PRODUCTION_FIX_UPDATED: NO
CLEANUP_READY: NO
COMMIT_READY: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## Validation this session

```text
Tooie HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf
RT64 HEAD:  6f1c2d99a4ea571c139f449c326fd176ba8f3496
cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test
TooieRecompiled: link OK (rt64_rsp_processor.cpp rebuilt)
window_display_test: 38/38 PASS
tintro_projection_policy_test: 7/7 PASS
git diff --check RT64 rt64_rsp_processor.cpp: PASS
```

`NEXT_SINGLE_ACTION`: `TOOIE_DIAG_RSP_ENDPOINT_BINDING=1
./build/TooieRecompiled` and paste the world-space / hierarchical
/ modelview keys from `/tmp/tooie-rsp-endpoint-binding-status.txt`.
