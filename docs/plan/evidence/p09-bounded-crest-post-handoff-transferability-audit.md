---
title: "P09 bounded crest post-handoff transferability audit"
description: "Right-plume Q / RSPProcess / RSPModify / RasterVS closures transfer to crest 108→79; full visible-feather position path is closed."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-07
tags: [p09, kazooie, feather, crest, transferability, rspprocess, q, rspmodify]
---

# P09 bounded crest post-handoff transferability audit

Bounded objective: decide whether already-closed right/left post-handoff
results transfer to Kazooie crest geometry (head 108 → bone 79, DLs
`0x4A6` / `0x4B0`) using static code and existing V3 / Q captures.
Static + existing-capture analysis only. No game. No new
instrumentation. No production fix. No GPU readback or added
synchronization. No commit. No push.

Do not reopen Banjo snout. Do not continue linearly into depth /
compositing. Do not treat Q-witness class C as `ROOT_CLASS`.

Authoritative V3 CPU handoff (`/tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt`,
CSV same stem) remains `CORRECT` for all 14 exact targets, including
crest (`CREST_COMPONENT_MISMATCH_COUNT=0`,
`FULL_VISIBLE_FEATHER_CPU_HANDOFF_STATUS=CORRECT`). This turn only
resolves the three crest transfer unknowns (Q, RSPProcess, RSPModify)
plus RasterVS.

Checkout: Tooie `ff5b6d9` dirty. Asset `/tmp/asset_062B.bin` (78200
bytes). Captures used offline:

- V3: `/tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt` + `.csv`
- Q: `/tmp/tooie-kazooie-feather-postprocess-q-v1-status.txt` + `.csv`
- shaders: `BanjoRecomp/lib/rt64/src/shaders/{RSPProcessCS,RasterVS,RSPModifyCS}.hlsl`

```text
RIGHT_TO_CREST_Q_RESULT_TRANSFERABLE: YES
RIGHT_TO_CREST_RSPPROCESS_RESULT_TRANSFERABLE: YES
RIGHT_TO_CREST_RSPMODIFY_RESULT_TRANSFERABLE: YES
RIGHT_TO_CREST_RASTERVS_RESULT_TRANSFERABLE: YES
FULL_VISIBLE_FEATHER_POST_HANDOFF_POSITION_PATH_STATUS: CLOSED
CURRENT_BEST_ROOT_CLASS: VISIBLE_FEATHER_POSITION_PIPELINE_CLOSED_ROOT_ELSEWHERE
PRODUCTION_FIX_CANDIDATE_FOUND: NO
NEW_CAPTURE_REQUIRED: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```

## 1. Crest Q / viewProj (`PROVEN_FOR_CAPTURE` + `PROVEN` source)

V3 `APPEND` rows (1040 total; 16 workloads generations 828–835):

| Region | APPEND rows | viewProjIndex corners |
| --- | ---: | --- |
| CREST (`0x4A6`/`0x4B0`) | 144 | `4`: 432 / 432 |
| RIGHT | 448 | `4`: 1344 / 1344 |
| LEFT | 448 | `4`: 1344 / 1344 |

`RENDER_VIEWPROJ_REFERENCED` is `index=4` on 512 / 512 samples.
`RENDER_SELECTION` is `target=ALL` (workload-wide),
`prev_frame_present=1`, `process_transforms=1`.

Q witness (right plume + body; **no** crest DLs admitted) already
proved for every sample including Kazooie head `0x48F` (parent of
crest): `viewproj_index=4`, `BODY_CONTROL_SHARES_TARGET_Q=YES`,
`TARGET_LARGE_QPRIME_DELTA=0/384`, `PRELIMINARY_CLASS=C`.
ProjectionProcessor writes `modViewProjTransforms[i]` per viewProj
slot, not per DL.

Crest therefore consumes the same processed `Q'[4]` as right, left,
torso `0xBC`, and head `0x48F`. Identical P/W is **not** required to
transfer the statement “the shared Q source is not stale/wrong as a
localized feather primary.” Viewport is `rspViewportVector[viewProjIndex]`.

```text
CREST_VIEWPROJ_INDEX_COUNTS: 4:432 (APPEND vertex-corners; 144/144 rows)
CREST_USES_SAME_VIEWPROJ_INDEX_AS_RIGHT_LEFT: YES
CREST_USES_SAME_PROJECTIONPROCESSOR_OUTPUT_Q: YES
CREST_USES_SAME_VIEWPORT: YES
RIGHT_Q_RESULT_TRANSFERABLE_TO_CREST: YES
WHY: crest viewProjIndex is exclusively 4 on the V3 capture; Q' is a
  per-index ProjectionProcessor output; Q-witness class C is about that
  shared slot (also consumed by head 0x48F). Negligible Q' vs current
  screen delta remains global, not crest-local.
```

## 2. Crest velocity (`PROVEN_FOR_CAPTURE`)

All 144 crest `APPEND` rows have `velocity_xyz` identically zero
(1040 / 1040 including plumes). Status `VELOCITY_INVARIANT=ZERO`.
`INITIAL_VELOCITY invariant=ZERO` on 224 / 224 handoff rows.
Tooie path `TransformGroup.vertexInterpolation = G_EX_COMPONENT_SKIP`;
F3DEX2 `setVertexCommon<true>` writes `vel=0`. RSPProcess
`P - V*(1-curFrameWeight)` collapses to `P`.

```text
CREST_VELOCITY_ZERO: YES
CREST_VERTEX_INTERPOLATION_MODE: SKIP (V=0; same as plumes)
RSPPROCESS_VELOCITY_TERM_ACTIVE_FOR_CREST: NO
```

## 3. Crest RSPProcess indexing (`PROVEN` source + V3)

Same shader `RSPProcessCS.hlsl` `CSMain` for the whole workload.
`INITIAL_RANGES` is shared: `position=0/8439` (2813×3),
`worldIndex=0/2813`, `face=0/6045`, `matrix=0/196`.
`INITIAL_HANDOFF_OK` crest rows: `rsp_vertex_range=0/2813` (same as
right tip), `worldindex_match=1`, `viewproj_match=1`,
`matrix_value_match=1`. Crest global vertices ~2031–2047; right tip
~2111+. `RENDER_SELECTION` `rsp_vertex_range=0/N` with `vertexStart=0`
after reset.

Crest worldIndex namespace: own `115` (bone 79), join mixed `115` /
`116` (parent 108). Right 124–131, left 132–139. Same
`worldTransforms` / `lerpWorldTransforms` vector (`selected_matrix_size=196`).
V7 `APPEND_WORLDINDEX_MATCH=1040/0`.

Join cache map `0–4` / `27–31` is an RSP **slot** fact at `G_VTX` time.
V7 already resolved those slots to global vertices + worldIndex
(`JOIN_4B0_ROLE_RESOLVED=YES` `96/0`). RSPProcess indexes
`srcPos/srcVel/srcWorldIndices[globalVertex]`, not the cache slot.

```text
CREST_RSPPROCESS_DATA_LAYOUT_SAME: YES
CREST_WORLDINDEX_NAMESPACE_CORRECT: YES
CREST_VIEWPROJ_NAMESPACE_CORRECT: YES
CREST_VERTEXSTART_OR_RANGE_DIFFERENCE: NO
CREST_JOIN_MAP_DIFFERENCE_SURVIVES_INTO_RSPPROCESS: NO
```

Do not keep RSPProcess unknown because crest join topology differs.

## 4. Matrix path (`PROVEN_FOR_CAPTURE` + V5B)

`INITIAL_HANDOFF_OK` records `selected_matrix_source=ORIGINAL` because
`prev_frame_present=0` at fullSync construction (all 14 targets).
`RENDER_SELECTION target=ALL` later selects `LERP`,
`process_transforms=1`, `prev_frame_present=1`, `prev_frame_matched=1`.
There is no crest-specific TransformProcessor mode.

V5B seven-bone mask `0x7F` includes bone 79. `NO_INTERP` visual `SAME`
already covered crest world matrices.

```text
CREST_TRANSFORMPROCESSOR_MODE: generic W' lerp when prev mapped (same
  RENDER_SELECTION ALL path)
RIGHT_PLUME_TRANSFORMPROCESSOR_MODE: same
CREST_HAS_DISTINCT_POST_HANDOFF_MATRIX_PROCESSING: NO
V5B_INCLUDED_BONE79: YES
V5B_WORLD_LERP_REFUTATION_TRANSFERS_TO_CREST: YES
```

## 5. RSPModify static command audit (`PROVEN` `STATIC_ROM`)

F3DEX2 walk of `/tmp/asset_062B.bin`, Gfx = `model+0x2670+dl*8`.

`0x4A6` (own): `G_GEOMETRYMODE` ×2, `G_TEXTURE`, `G_RDPPIPESYNC`,
`G_SETCOMBINE`, `G_DL dest=0x03000010` (call, `push=0`), `G_VTX n=7
v0=0 0x01003010`, `G_TRI2`, `G_TRI1`, `G_ENDDL`. **Zero** `G_MODIFYVTX`.

`0x4B0` (join): same setup + nested `0x03000010`, `G_VTX n=5 v0=0
0x01003080`, `G_POPMTX`, `G_VTX n=5 v0=27 0x010030D0`, three `G_TRI2`,
`G_ENDDL`. **Zero** `G_MODIFYVTX`.

Nested `G_DL 0x03000010` runs **before** that DL's `G_VTX`. Even if
the nested list contained a position modify, crest vertices are loaded
afterwards and overwrite slots 0–6 (own) or 0–4 and 27–31 (join).
Segment-3 base stays `RUNTIME_REQUIRED`. Head own `0x48F` uses the
same nested call. Hypothetical `seg3=Gfx[0]` at `+0x10` starts
`G_TEXTURE` / combine / recursive `G_DL`, still no `G_MODIFYVTX`.
No `G_MWO_POINT_XYSCREEN`/`ZSCREEN` (`where` `0x18`/`0x1C`) on these
command streams.

```text
CREST_4A6_HAS_G_MODIFYVTX: NO
CREST_4B0_HAS_G_MODIFYVTX: NO
CREST_NESTED_03000010_HAS_POSITION_MODIFY: NO
CREST_RSPMODIFY_POSITION_REACHABLE: NO
```

RSPModify transfers closed.

## 6. Numeric edges (offline V3)

Paired same-generation `APPEND` P + `RENDER_MATRIX` W' +
`RENDER_VIEWPROJ_REFERENCED[4]` Q (CPU column-vector recon; **not**
GPU clip truth; same convention as the Q-witness caveat).

| Region | n | min \|clip.w\| | max \|clip.w\| | near0 (<100) | NaN/Inf |
| --- | ---: | ---: | ---: | ---: | ---: |
| CREST | 432 | 1.16e8 | 5.58e8 | 0 | 0 |
| RIGHT | 1344 | 4.90e8 | 2.13e9 | 0 | 0 |
| LEFT | 1344 | 3.05e6 | 1.69e9 | 0 | 0 |

Crest all-positive. Magnitude is smaller than right; not near-zero.
Do not request a capture for theoretical float concerns.

```text
EXISTING_DATA_CAN_TEST_CREST_NUMERIC_EDGES: YES
CREST_NEAR_ZERO_CLIP_W: NO
CREST_NAN_OR_INF_RISK_OBSERVED: NO
CREST_NUMERIC_RANGE_DISTINCT_FROM_PLUMES: YES (CPU |clip.w| magnitude
  vs right; not a remaining RSPProcess unknown)
```

## 7. RasterVS (`PROVEN` source)

`RasterVS.hlsl`: convert N64 screen XY to framebuffer NDC, re-multiply
by homogeneous W, apply `screenScale`/`screenOffset` (per-projection
batch). Optional prim-depth Z only. No extra world or projection.
`baseVertex=0`. Crest DLs contain no `G_SETPRIMDEPTH`. Same
`screenPosBuffer` path as plumes.

```text
CREST_USES_SAME_RASTERVS_PATH: YES
CREST_RASTERVS_HAS_ADDITIONAL_POSITION_LOGIC: NO
RIGHT_TO_CREST_RASTERVS_RESULT_TRANSFERABLE: YES
```

## 8–9. Full post-handoff decision

All four crest transfers are YES. Combined with V3 CPU handoff
`CORRECT` on 14/14 and the already-transferred right→left Q /
RSPProcess / RSPModify results:

```text
FULL_VISIBLE_FEATHER_CPU_HANDOFF_STATUS: CORRECT
PROJECTIONPROCESSOR_PRIMARY_CAUSE: REFUTED_FOR_CURRENT_WITNESS
  (shared Q'[4]; class C; transfers to crest)
RSPPROCESS_TARGET_LOCAL_MECHANISM: CLOSED for all 14 visible feather DLs
RSPMODIFY: ABSENT for all 14
RASTERVS: PASSIVE / SAME for all 14
FULL_VISIBLE_FEATHER_POST_HANDOFF_POSITION_PATH_STATUS: CLOSED
CURRENT_BEST_ROOT_CLASS: VISIBLE_FEATHER_POSITION_PIPELINE_CLOSED_ROOT_ELSEWHERE
ROOT_CONFIDENCE: HIGH for the position-path close; LOW for the remaining
  visual root. Do not call GPU bug.
```

Stop extending position witnesses.

## 10. Next non-position assumption (not investigated)

Ranked, not audited this turn:

1. triangle culling / winding / face visibility
2. depth values / depth-test interaction
3. triangle/draw temporal association
4. disappearance / occlusion rather than position
5. other non-position state

```text
NEXT_NON_POSITION_ASSUMPTION: triangle culling / winding / face visibility
WHY: P, V, worldIndex, W', viewProjIndex, Q', viewport, RSPModify, and
  RasterVS extra transforms are closed for all 14 visible feather DLs.
  Correct screen positions can still look geometrically wrong if
  winding/cull flips the silhouette across frames. User report (some
  frames correct, others distorted, movement-only) is compatible with
  intermittent face visibility without a remaining position writer.
```

## 11. No missing crest transfer

```text
MISSING_VALUE: NONE
CAN_EXISTING_CAPTURE_BE_EXTENDED_OFFLINE: N/A
NEW_CAPTURE_REQUIRED: NO
EXPECTED_CAPTURE_COUNT: 0
```

## Validation this turn

Docs-only forensic. No code. No game. No build.

```text
PRODUCTION_FIX_CANDIDATE_FOUND: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: bounded static / command-stream audit of triangle
  culling / winding / face visibility on the 14 visible feather DLs;
  do not add another position witness; do not GPU-readback
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
NO_COMMIT: YES
NO_PUSH: YES
```
