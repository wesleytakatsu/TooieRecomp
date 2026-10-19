---
title: "P09 multi-only WORLD selector visual candidate"
description: "Default-off WORLD_INDEX_MULTI_ONLY elementwise candidate. Mixed WORLD indices stay on RigidBody::lerp. Original selector unchanged."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-23
tags: [p09, rt64, interpolation, world, selector, multi-only, visual-candidate]
---

# P09 multi-only WORLD selector visual candidate

Role: `P09_MULTI_ONLY_WORLD_SELECTOR_VISUAL_CANDIDATE`.

Authoritative root stays:

```text
ROOT_CAUSE_CLASS=
NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS
```

This note does not reopen that root. The previous WORLD-index candidate
stays available and default-off. This candidate is a second toggle.

## Why this candidate exists

User audit (`USER_OBSERVED`): `SESSION_LEAK_OBSERVED=YES`,
`SESSION_PEAK_SELECTED_MIXED=31`, representative workload
`CURRENT_SELECTOR_SELECTED_MIXED_USAGE_COUNT=20` and
`CURRENT_SELECTOR_SELECTED_MULTI_ONLY_COUNT=81`. Mixed slots such as
`w=52` (1 multi-WORLD call, 7 single-WORLD calls) receive elementwise
interpolation for every later use of that index.

User visual A/B of `TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD`:

- Off: Kazooie head feathers, Mumbo feathers, and Banjo snout glitched.
  Klungo cutting lines and Mumbo one-frame disappearance absent.
- On: feathers and Banjo backpack/top appear corrected. Banjo snout
  still glitched. Klungo gains cutting lines. Mumbo gains an occasional
  near-one-frame disappearance.

`CURRENT_ORIGINAL_SELECTOR_STATUS=PARTIAL_FIX_WITH_REGRESSIONS`.

## Selector

`TOOIE_TEST_P09_MULTI_ONLY_ELEMENTWISE_WORLD=1`.

For the workload `TransformProcessor::process` is interpolating, a WORLD
index is elementwise only when that workload's GameCall walk shows:

```text
multiWorldGameCallUseCount > 0
AND
singleWorldGameCallUseCount == 0
```

The walk matches `p09WriteSelectorAudit`: same framebuffer pairs,
projections, and GameCalls; face corners through `faceIndices` into
`worldIndices`; empty face lists skipped; a call with no in-range WORLD
is not counted; two or more distinct in-range indices make the call
multi-WORLD. Counts are locals sized to this workload's
`worldTransforms`. The previous workload supplies matrix endpoints only.
It is not the classifier.

Mixed indices (`multiCalls > 0` and `singleCalls > 0`) stay unmarked.
Single-only and unused indices stay unmarked. Unmarked mapped slots use
the existing `RigidBody::lerp`. Unmapped slots still copy the current
matrix, as before.

Elementwise math is the existing `lerpMatrix`: each of the 4 columns is
`hlslpp::lerp`, which is all 16 entries,

```text
M[i][j] = (1 - w) * PREV[i][j] + w * CURRENT[i][j]
```

at `prevFrameWeight` and `curFrameWeight`.

If `TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD` is also set, the original
marker runs and this one does not. Enable one candidate per visual test.

```text
MULTI_ONLY_SELECTOR_IMPLEMENTED=YES
MULTI_ONLY_SELECTOR_CLASSIFICATION_WORKLOAD_CORRECT=YES
MULTI_ONLY_SELECTOR_EXCLUDES_MIXED_WORLD_INDICES=YES
PRODUCTION_CANDIDATE_KIND=WORLD_INDEX_MULTI_ONLY_ELEMENTWISE
PRODUCTION_CANDIDATE_DEFAULT_ON=NO
```

## What this build does not do

No duplicated WORLD matrices, no `worldIndices` remapping, no duplicated
vertices, no shader branch, no seam hash. VIEW interpolation, PROJECTION
interpolation, and `RSPProcessCS.hlsl` are unchanged. The removed W-sign
snap stays removed. No GPU readback, copy, staging, synchronization, or
query. The game was not launched.

While this candidate is on, a bounded CPU summary may be written to
`/tmp/tooie-p09-multi-only-summary.txt` (overwrite, peak or every 240
workloads). It records `MULTI_ONLY_SELECTED_COUNT` and
`MIXED_EXCLUDED_COUNT` plus session peaks. The visual test does not need
`TOOIE_DIAG_P09_SELECTOR_AUDIT` or `TOOIE_DIAG_RSP_ENDPOINT_BINDING`.

## How to read the visual result

1. Kazooie head feathers still fixed?
2. Banjo backpack/top still fixed?
3. Mumbo feathers still fixed?
4. Klungo new cutting lines gone?
5. Mumbo one-frame disappearance gone?
6. Banjo snout still glitched?
7. Any new squash, shear, or wobble?

- Case A: known fixes remain and Klungo/Mumbo regressions disappear.
  Mixed-use leakage caused those regressions. Still do not ship: the
  snout / cross-call false negative may remain.
- Case B: regressions disappear and a previously fixed seam returns.
  That seam needs a mixed WORLD index. A WORLD-slot lerp cannot cover it
  without call-local duplication or remapping. Stop for a fix-design
  review before that architecture.
- Case C: Klungo/Mumbo regressions remain with mixed slots excluded.
  Elementwise interpolation is harming some multi-only geometry.
- Case D: Banjo snout stays glitched. Expected possible false negative.
  Do not force it into this selector.

## Offline validation

```text
TOOIE_HEAD=ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf
RT64_HEAD=6f1c2d99a4ea571c139f449c326fd176ba8f3496
rt64_transform_processor.cpp SHA-256=
  24b85b670151f5c16a6cb89707162c9a7fe299b7e00e66b701af5b765246e4f8
RSPProcessCS.hlsl SHA-256=
  b092ff8466fd870deec62f1c908a74e84e0a9eb07d11b2d82f15e7c8a5addcff
  (unchanged this task; pre-existing dirty tree preserved)
cmake --build build --target TooieRecompiled                 exit 0
./build/window_display_test                                   38/38 PASS
./build/tintro_projection_policy_test                         7/7 PASS
git diff --check Tooie                                       exit 0
git diff --check RT64                                        exit 0
GAME_LAUNCHED=NO
GPU_READBACK_USED=NO
GPU_SYNCHRONIZATION_ADDED=NO
VIEW_INTERPOLATION_CHANGED=NO
PROJECTION_INTERPOLATION_CHANGED=NO
SHADER_CHANGED=NO
OLD_W_SIGN_PRODUCTION_CANDIDATE_ACTIVE=NO
```

```text
USER_VISUAL_TEST_COMMAND=
TOOIE_TEST_P09_MULTI_ONLY_ELEMENTWISE_WORLD=1 ./build/TooieRecompiled
USER_VISUAL_TEST_DIAGNOSTICS_HEAVY=OFF
NEXT_SINGLE_ACTION=user A/B visual test of WORLD_INDEX_MULTI_ONLY selector
NEXT_RECOMMENDED_MODEL=Grok 4.6 Medium after visual result
CLEANUP_READY=NO
COMMIT_READY=NO
NO_COMMIT=YES
NO_PUSH=YES
```
