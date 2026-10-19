---
title: "P09 feather postprocess Q' CPU witness V1"
description: "Bounded CPU-only reconstruction of RSPProcess inputs after ProjectionProcessor; Q current/previous/prime and body control sharing viewProj."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-07
tags: [p09, kazooie, feather, projection, rspprocess, diagnostic]
---

# P09 feather postprocess Q' CPU witness V1

**Later correction (2026-09-09):** Small Q interpolation deltas do
not outrank later global-current `FIXED`. They also do not justify a
mixed-time projection-only A/B (`INVALID_TEMPORALLY_INCOHERENT`).
Canonical: [[p09-temporal-coherency-root-cause]].

Bounded objective: implement a CPU-only witness that can confirm or
refute ProjectionProcessor Q' as the primary remaining cause before
`RSPProcessCS`. No production fix. No game. No GPU readback/sync. No
commit. No push.

V7 VERSION=2 CPU handoff remains closed. World-matrix interpolation
remains closed as primary (V5B). Target DLs have no `G_MODIFYVTX`.

```text
DIAGNOSTIC_SCHEMA: P09_FEATHER_POSTPROCESS_Q_WITNESS_V1
DIAGNOSTIC_VERSION: 1
V8_CREATED: NO
PRODUCTION_FIX: NO
GPU_READBACK_REQUIRED: NO
GPU_SYNC_REQUIRED: NO
GPU_BUFFER_MAPPING_REQUIRED: NO
PRODUCTION_DATA_MUTATED: NO
SHADER_CODE_CHANGED: NO
```

## ProjectionProcessor exact order (`PROVEN` source)

`ProjectionProcessor::process` copies raw `view/proj/viewProj` into
`mod*` and `prev*`. `processScene` then, per live projection:

1. Load current `viewTransforms[i]` / `projTransforms[i]`.
2. Aspect-adjust **current** projection (`adjustProjectionMatrix` scales
   column 0 by `projRatioScale`).
3. If previous view/proj are mapped: aspect-adjust previous projection
   the same way; lerp view with `RigidBody::lerp`; lerp projection only
   if `lerpTranslation || lerpRotation`.
4. `Q' = mul(view, proj)` into `modViewProjTransforms[i]`.
5. `upload` sends `modViewProjTransforms` to `viewProjTransformsBuffer`.

```text
PROJECTIONPROCESSOR_EXACT_ORDER: current/previous -> aspect_adjust(proj) -> interpolation -> Q'=mul(view,proj)
CURRENT_VIEWPROJ_MATRIX_SOURCE: drawData.viewTransforms[i] and drawData.projTransforms[i] (raw CPU handoff). Copied viewProjTransforms is overwritten.
PREVIOUS_VIEWPROJ_MATRIX_SOURCE: prevWorkload.drawData.viewTransforms[viewProjMap.prevTransformIndex] and matching projTransforms when mapped.
FINAL_Q_PRIME_UPLOAD_SOURCE: drawData.modViewProjTransforms when processProjections; else original viewProjTransforms.
ASPECT_ADJUSTMENT_LOCATION: both current and previous projection matrices, before interpolation.
LERP_FUNCTION_OR_FORMULA: view = RigidBody::lerp(curFrameWeight, prevView, curView, slerp=true); proj = lerpMatrix(adjPrev, adjCur, curFrameWeight) if interpolateProjection else adjCur.
```

Aspect is a linear first-column scale, so
`lerp(adjust(A), adjust(B)) = adjust(lerp(A,B))`. The witness still
stores pre-aspect and post-aspect Q separately.

## CPU RSPProcess reconstruction

Shader `RSPProcessCS`:

```text
clip = mul(mul(Q, W), float4(P - V*(1-curFrameWeight), 1))
if clip.w == 0: clip.w = 1e-6
ndc = (clip.x/clip.w, clip.y/(-clip.w), clip.z/clip.w)
screen = ndc * rspViewport.scale + rspViewport.translate
dstPos = float4(screen, clip.w)
```

CPU uses the same hlslpp ops. GPU bit identity is **not** required.
This witness compares alternate CPU reconstructions (Q_current /
Q_previous / Q_pre-aspect / Q') with **fixed selected W'**.

```text
CPU_RSPPROCESS_FORMULA: as above
CPU_RECONSTRUCTION_BIT_OR_NUMERIC_EQUIVALENCE_EXPECTATION: hlslpp equivalent of the shader; matrix eps 1e-5; screen hypot eps 1e-3; no GPU output
TARGET_AND_CONTROL_Q_ANALYSIS_USES_FIXED_SELECTED_W: YES
```

`LARGE` position delta: N64 screen hypot(XY) >= 1.0 or |dZ| >= 1.0.

## Body control (`PROVEN` `STATIC_ROM`)

Same model `0x62B`, same address formula as V7
(`physical(model_bin)+0x2670+logical*8`).

| Control | GeoCmd | DL | Bone |
| --- | --- | --- | --- |
| Banjo torso | type-0x11 `+0x11C20` | `0xBC` | anim 3 INDEX 1 |
| Kazooie head | type-0x11 `+0x12358` | `0x48F` | anim 108 INDEX 26 |

Not plume DLs `0x534/0x54C/0x564/0x57C/0x592/0x5AA`. Not snout `0x102`.
Not crest `0x4A6`. Runtime `SHARES_TARGET_VIEWPROJ_INDEX` is recorded
when a body sample uses the same `viewProjIndex` as a feather sample
(V7 sampled feather index 4).

```text
BODY_CONTROL_FOUND: YES
BODY_CONTROL_DL_OR_SOURCE: 0xBC (Banjo torso) and 0x48F (Kazooie head 108)
BODY_CONTROL_VIEWPROJ_INDEX: RUNTIME (expected share of feather camera slot)
```

## Boundedness

```text
MAX_RECORDS: 32 per workload
MAX_RECORDS_PER_TARGET: 4
MAX_BODY_CONTROL_RECORDS: 8 (4 per control)
WORKLOAD_WINDOW: 8
POSTPROCESS_WITNESS_CORRELATION_KEY: diagnosticGeneration+identity+globalVertex+worldIndex+viewProjIndex
```

One vertex (first corner) per sampled triangle. Status is created at
enable. CSV `/tmp/tooie-kazooie-feather-postprocess-q-v1.csv`. Status
`/tmp/tooie-kazooie-feather-postprocess-q-v1-status.txt`.

Enable: `TOOIE_DIAG_FEATHER_POSTPROCESS_Q=1`. Default off.

## Classification (post-capture)

- **A** — Q' large target delta and body sharing same Q has comparable delta
- **B** — Q' large target delta; body sharing same Q does not
- **C** — Q' ~= Q_current / negligible target delta
- **D** — bad/stale/wrong Q source or index (`viewProjIndex==0` or OOB)

`PRELIMINARY_CLASS` is written on the status file. Magnitude is not
visual causation.

## Seams

- Append: `RSP::drawIndexedTri` after real face-index append (identity only).
- Projection snapshot: after `Q'` is written in `processScene` (copies only).
- Reconstruction: `WorkloadQueue::threadRenderFrame` after both processors
  and immediately before `RSPProcessor::process`.

No shader changes. No production matrix writes.

## Later correction (2026-09-09)

Small Q interpolation deltas in this witness do **not** refute
ProjectionProcessor as a high-rate stretch cause. Global current-frame
is later `USER_OBSERVED` `FIXED` while V5C world-matrix freeze is
`SAME`. Isolation:
[[p09-high-rate-force-current-projection]].

## Validation

```text
BUILD_RESULT: PASS (cmake --build build --target TooieRecompiled -j2, exit 0)
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7 tintro_projection_policy_test)
TOOIE_DIFF_CHECK: recorded at close
RT64_DIFF_CHECK: recorded at close
USER_MANUAL_TEST_REQUIRED_NOW: YES
USER_TEST_COMMAND: TOOIE_DIAG_FEATHER_POSTPROCESS_Q=1 ./build/TooieRecompiled
NO_COMMIT: YES
NO_PUSH: YES
```
