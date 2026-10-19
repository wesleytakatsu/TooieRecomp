---
title: "P09 generic selective WORLD interpolation production design"
description: "Default-off SELECTIVE_ELEMENTWISE_WORLD in TransformProcessor; multi-world GameCall vertex classifier; W-sign snap disabled. Visual test required."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-17
tags: [p09, rt64, interpolation, world, elementwise, selective, gamecall, production-candidate]
---

# P09 generic selective WORLD interpolation production design

Role: `P09_GENERIC_SELECTIVE_WORLD_INTERPOLATION_PRODUCTION_DESIGN`.

Bounded objective: smallest generic CPU selector for elementwise WORLD
interpolation of articulated multi-transform GameCalls, keeping decomposed
interpolation for ordinary rigid WORLDs. No new causal CPU arm. No VIEW /
projection / W / cohort / camera-pose / CLIP_LERP / NDC_LERP / hierarchy
experiments. No game launch in this task.

Authoritative root (`PROVEN`, `USER_OBSERVED`, prior V2 captures):

```text
ROOT_CAUSE_CLASS=
NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS
```

Latest capture used as motivation (not re-measured here):

```text
LIVE WORLDSPACE JOIN MAX=8.36268425 P99=1.38291669
STANDARD_SLERP WORLDSPACE JOIN MAX=5.07381058 P99=1.28550041
ELEMENTWISE WORLDSPACE JOIN MAX=1.00009584 P99=0.999989033
PATHOLOGY TOTAL=427
STANDARD_SLERP FIX_RATE=37.2365% (159/427)
ELEMENTWISE FIX_RATE=100% (427/427)
ELEMENTWISE max orthogonality error=0.233893916 max shear=0.57420224
```

Do not ship global elementwise WORLD. Standard SLERP is a contributor, not
a sufficient production fix.

## Task 1 — generic data before/during WORLD lerp (`PROVEN`, `OTHER`, static)

Available on the current mapped workload when `TransformProcessor::process`
runs (after `GameFrame::match`):

| Fact | Where |
| --- | --- |
| `TransformGroup` | `DrawData.transformGroups[]`; `decompose` default true for WORLD |
| `worldTransformGroups[t]` | group index per WORLD slot |
| `TransformMap` | `GameFrameMap::WorkloadMap.transforms[t]`; mapped pair + `RigidBody` |
| `GameCall` | `Workload.fbPairs[].projections[].gameCalls[]` |
| `minWorldMatrix` / `maxWorldMatrix` | contiguous index range of vertex world slots in the call |
| `worldIndices` | per-vertex WORLD slot used by the triangle |
| `matrixId` | `TransformGroup.matrixId` (Tooie WORLD is typically `G_EX_ID_AUTO`) |
| VIEW / PROJECTION | `ProjectionProcessor`; not read by this candidate |

`min`/`max` is **not** a distinct-WORLD set. GameFrame matching already
walks the dense range `[min, max]`. Distinct usage is recoverable by
scanning `faceIndices` → `worldIndices` for the call.

A new `TransformGroup` is created only when the extended matrix-id stack
changes (`matrixId` / `popMatrixId`). Without tagging, many Tooie WORLD
slots share one AUTO group, so classifying **only** at group granularity
is equivalent to near-global WORLD policy (`STRONG_INFERENCE`).

```text
GENERIC_MULTI_WORLD_USAGE_DISCOVERABLE: YES
GENERIC_TRANSFORM_GROUP_CAN_BE_CLASSIFIED_BEFORE_LERP: YES
GENERIC_ARTICULATED_SELECTOR_AVAILABLE_WITHOUT_GAME_HARDCODE: YES
```

The last line is YES at **WORLD index** granularity from GameCall vertex
usage, not as a Tooie-specific bone/DL/model table. Group-only
classification is too coarse without `gEXMatrixGroup` tagging.

## Task 2 — simplest generic classifier (`PROVEN_STATIC`)

Definition implemented:

A WORLD index is an articulated/multi-world candidate if **any** GameCall
in the current workload uses **more than one distinct** WORLD index on
its triangle vertices (`worldIndices`). Every distinct WORLD used by that
call is marked.

This maps onto RT64: GameCalls exist before lerp; vertices already carry
`worldIndices`; type-0x18 join DLs that `G_POPMTX` between two `G_VTX`
loads are the architectural case of one GameCall with two WORLDs
(`STRONG_INFERENCE` from prior GeoCmd/DL reconstruction, not a hardcoded
selector).

Do **not** use `maxWorldMatrix > minWorldMatrix` alone: that would mark
unused intermediate slots in the dense range.

```text
MULTI_WORLD_GAMECALL_SELECTOR_FEASIBLE: YES
SELECTOR_GRANULARITY: WORLD_INDEX (from GameCall vertex worldIndices)
FALSE_POSITIVE_RISK: GameCalls that batch two unrelated WORLDs under the
  same RDP state (no flush) get elementwise; shear risk is confined to
  those slots, not the whole frame
FALSE_NEGATIVE_RISK: articulated parent/child that never share a GameCall
  keep decomposed lerp (type-0x11-only parts are still selected if their
  WORLD also appears in a multi-world join call)
```

Setting `TransformGroup.decompose=false` is **not** the same as 16-entry
lerp: `RigidBody::lerp` then uses `lerpMatrixComponents`, which can skip
translation / 3x3 / last column from AUTO flags.

## Task 3 — layer

```text
BEST_PRODUCTION_FIX_LAYER: TransformProcessor
WHY: owns PREV/CURRENT WORLD, curFrameWeight, and the upload of
  lerpWorldTransforms; can scan current-workload GameCalls before lerp;
  ProjectionProcessor stays on VIEW/PROJECTION; RSPProcessor would be
  after upload; no second GPU WORLD buffer
```

Candidate C (RSPProcessor) is unnecessary. Candidate B
(`TransformGroup.decompose=false` at match time) is the wrong numeric
policy.

## Task 4–5 — implementation (default-off)

Toggle: `TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD=1` (default off).

For marked mapped WORLDs:

```text
M = lerpMatrix(PREV, CURRENT, w)
  ≡ (1-w)*PREV[i][j] + w*CURRENT[i][j]  (all 16 entries)
```

No `RigidBody::lerp`, no standard SLERP. Unmarked WORLDs keep current
decomposed interpolation. Unmapped/current duplication is unchanged.
VIEW interpolation is unchanged. PROJECTION interpolation is unchanged.
No Tooie model/bone/DL IDs.

## Task 6 — confounders

The failed production W-sign snap in `RSPProcessCS.hlsl`:

```text
if (prevClipW * curClipW <= 0) tfPos = curClip
```

is **removed**. Default-off `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT` still
snaps every vertex when explicitly set. `TOOIE_DIAG_RSP_MARK_W_CROSSING`
only offsets NDC X under that diagnostic. CPU STANDARD_SLERP /
ELEMENTWISE diagnostic arms remain diagnostic-only behind
`TOOIE_DIAG_RSP_ENDPOINT_BINDING`.

```text
OLD_W_SIGN_PRODUCTION_CANDIDATE_ACTIVE: NO
OTHER_P09_PRODUCTION_CANDIDATE_ACTIVE: NO
```

The W-sign removal is confounder cleanup, not a new interpolation shader
path. The selective WORLD candidate does not add shader branching.

## Offline validation (this task)

```text
TOOIE_HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf (dirty; pre-existing)
RT64:       6f1c2d99a4ea571c139f449c326fd176ba8f3496 (dirty; pre-existing)
BANJORECOMP: ec859632cfa584e2272d32e46051543429a2baf4
rt64_transform_processor.cpp SHA-256:
  98de6a9d522eac5b58bfcea75432901f6be37af25d4df6b323e72c1eece09c4d
RSPProcessCS.hlsl SHA-256:
  b092ff8466fd870deec62f1c908a74e84e0a9eb07d11b2d82f15e7c8a5addcff
cmake --build build --target TooieRecompiled                 exit 0
cmake --build build --target window_display_test \
        tintro_projection_policy_test                         exit 0
./build/window_display_test                                   38/38 PASS
./build/tintro_projection_policy_test                         7/7 PASS
git diff --check (intended Tooie/RT64 paths)                  empty
GAME_LAUNCHED=NO
GPU_READBACK_USED=NO
GPU_SYNCHRONIZATION_ADDED=NO
PRODUCTION_CANDIDATE_KIND=SELECTIVE_ELEMENTWISE_WORLD
PRODUCTION_CANDIDATE_DEFAULT_ON=NO
```

## Visual acceptance (user)

```text
USER_VISUAL_TEST_COMMAND=
TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD=1 ./build/TooieRecompiled
USER_VISUAL_TEST_DIAGNOSTICS_HEAVY=OFF
```

High-rate, normal play. Witnesses: Kazooie head feathers, Banjo
backpack/top flap, snout/muzzle, Mumbo feathers if convenient.

1. Are the long articulated streaks gone?
2. Is interpolation still smooth?
3. Is there visible squash/shear/wobble introduced?
4. Are ordinary rigid objects/camera behavior unaffected?

Do not enable `TOOIE_DIAG_RSP_ENDPOINT_BINDING`. Do not require video.

If selective removes P09, stays smooth, and shows no obvious regressions,
it is the preferred production fix (still default-off until that visual).
If streaks remain, false-negative risk of the GameCall selector is the
next design question — do not silently switch to global elementwise.

```text
CLEANUP_READY: NO
COMMIT_READY: NO
NO_COMMIT: YES
NO_PUSH: YES
```
