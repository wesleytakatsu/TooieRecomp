---
title: "P09 paired-edge seam dry-run v2"
description: "Default-off CPU paired-edge plan. Search is per-workload or fail-closed. Rendering is unchanged."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-15
tags: [tooie-recomp, p09, rt64, interpolation, dry-run]
---

# P09 paired-edge seam dry-run v2

## Scope

Implements only the versioned default-off CPU dry-run required after
[[p09-seam-alias-dry-run-validity-review]]. V1
(`TOOIE_DIAG_P09_SEAM_ALIAS_DRY_RUN`, `/tmp/tooie-p09-seam-alias-dry-run.txt`)
is unchanged. No WORLD aliases, no `worldIndices` remap, no vertex clones,
no `lerpWorldTransforms` edit, no shader/VIEW/PROJECTION/RSP change, no
elementwise enablement. The game was not launched.

> [!NOTE]
> Actual exact-use pathology correlation implemented in [[p09-exact-use-pathology-correlation]] uses a same-RSP-call **local plan builder** (`p09BuildLocalAliasPlan`), avoiding the unsafe global last-workload API (`gP09V2Plan`) and transform-all-before-RSP overwrites. The standalone V2 dry-run documented here remains unchanged, and its prior capture is preserved as historical evidence.

Toggle: `TOOIE_DIAG_P09_SEAM_EDGE_DRY_RUN_V2=1`.
Output: `/tmp/tooie-p09-seam-edge-dry-run-v2.txt`.

`P09_SEAM_DRY_RUN_VERSION=2_PAIRED_EDGE_PROVENANCE`.
`OLD_POINT_PAIR_CERTIFICATION_USED=NO`.
`POINT_ONLY_MATCH_CAN_CERTIFY_SEAM=NO`.
`PAIRED_EDGE_CERTIFICATION_USED=YES`.
`RENDERING_BEHAVIOR_CHANGED=NO`.
`PRODUCTION_FIX_UPDATED=NO`.

## Edge occurrences

Each retained occurrence is one directed face edge from a GameCall triangle.
Distinct face corners that share a stored vertex are not collapsed.

`EDGE_PROVENANCE_KEY=workloadId,fbPair,projection,gameCall,triangleLocal,edgeInFace,faceCorner0,faceCorner1,corner0,corner1,storedVertex0,storedVertex1,world0,world1`.

`fbPair` is the index in `workload.fbPairs`. `projection` is the index in
that pair. `gameCall` is the index in that projection. `triangleLocal` is
the triangle index inside the call. `edgeInFace` is 0, 1, or 2.
`faceCorner*` are indices into `faceIndices`.

Internal edges are counted, then discarded. Only boundary edges are stored.
`EDGE_OCCURRENCE_PROVENANCE_COMPLETE=YES` for every stored occurrence.
`RETAINED_OCCURRENCES=BOUNDARY_EDGES_ONLY`.

## Boundary

`BOUNDARY_EDGE_DISCOVERY_FEASIBLE=YES`.

`BOUNDARY_EDGE_DEFINITION`: inside one GameCall face slice, an undirected
pair of stored vertex ids is a boundary edge when that pair occurs on
exactly one triangle. Opposite winding counts as the same edge. Distinct
stored vertex ids stay distinct, so a duplicated seam remains two boundary
occurrences. Degenerate edges are ignored. No model, display list, bone, or
character id is used.

Captured on 2026-10-15 (latest workload):
`BOUNDARY_EDGE_COUNT=4627`.
`SAME_CALL_BOUNDARY_EDGE_COUNT=4627`.
`CROSS_CALL_BOUNDARY_EDGE_COUNT=4609`.

Same-call search pairs boundary edges of one GameCall. It does not reuse
V1's intra-triangle cross-WORLD test.
`SAME_CALL_SEARCH=pairs of boundary edges in the same GameCall, not intra-triangle cross-WORLD edges`.

## Certification

A candidate is two vertex-disjoint boundary occurrences whose matched
endpoints use different original WORLD slots. Both orientations are tried.
Both endpoints must match at once. One shared point cannot certify.

`TWO_DISTINCT_ENDPOINT_MATCHES_REQUIRED=YES`.
`ONE_TO_ONE_ENDPOINT_ASSIGNMENT_REQUIRED=YES`.
`SINGLE_SHARED_POINT_CAN_CERTIFY=NO`.
`VERTEX_DISJOINT_EDGE_PAIR_REQUIRED=YES`.

`SEAM_ENDPOINT_SEMANTICS=ROW_VECTOR_MUL_LOCALPOS_WORLD`.
`PREV_AND_CURRENT_REQUIRED=YES`.
`OLD_INVALID_CPU_MATRIX_SEMANTICS_USED=NO`.
World matrices are not interpolated to certify a seam.

`TOLERANCE_USES_WORLD_TRANSLATION=NO`.
`CURRENT_0_25_FLOOR_USED=NO`.

`NUMERIC_TRANSFORM_ERROR_BOUND_SOURCE`: float32 `gamma_3` bound on the
linear part of `mul(localPos4, WORLD)`. `gamma_3 = 3*eps/(1-3*eps)` with
`eps = 2^-23`. Only WORLD rows 0..2 (the columns that multiply local xyz)
enter the bound. The translation row does not.

`VERTEX_QUANTIZATION_BOUND_SOURCE`: `RSP::Vertex::x/y/z` are `int16_t`
(`rt64_rsp.h`) and `setVertexCommon` stores them with an exact float
conversion into `posFloats` (`rt64_rsp.cpp`). Every int16 is exactly
representable in float32, so the stored local position adds no further
rounding. `VERTEX_QUANTIZATION_BOUND=0`. A nonzero radius would be a guess
about authoring, not about this buffer.

`EDGE_SCALE_TERM`: `1e-4 * min(transformed length of edge A, transformed length of edge B)`
at the frame being tested. Length is the world distance between that edge's
own endpoints, so translation cancels.
`EDGE_SCALE_COEFFICIENT_STATUS=DIAGNOSTIC_NOT_PROVEN`.

`SEAM_TOLERANCE_FORMULA`: for each matched endpoint and each of PREV and
CURRENT, `dist(mul(localA,WORLD), mul(localB,WORLD)) <= linearError(A)+linearError(B)+edgeScaleTerm`.

Both frames must pass. Reject counters are mutually exclusive:
previous-only, current-only, or both frames
(`SAME_CALL_REJECT_TOLERANCE_COUNT` / `CROSS_CALL_REJECT_TOLERANCE_COUNT`).

`COMPETING_MATCH_MARGIN_REQUIRED=YES`.
`AMBIGUOUS_MATCHES_DEFAULT_TO_RIGID=YES`.
`MATCH_MARGIN_FORMULA`: the two edges must be each other's closest compatible
partner, and `nextBestCompatibleResidual - bestMatchResidual > seamTolerance(bestPair)`
on both edges. An equal residual is ambiguous. No stored partner table is
ordered by hash insertion; best and next-best are a running minimum. A
missing next best counts as unique only when that edge's partner search did
not drop a possible neighbor.

Cross-call retrieval uses the same cell size 32 and the same 26-neighborhood.
The index is an exact per-workload map from cell to the edges stored there,
so a bucket walk visits every stored edge in those cells. It does not certify.
`CROSS_CALL_SPATIAL_HASH_IS_RETRIEVAL_ONLY=YES`.
`CROSS_CALL_SPATIAL_INDEX=PER_WORKLOAD_EXACT_CELL`.

An unordered retrieved pair is evaluated once. The lower stored edge id owns
it, after the querying edge removes duplicate hits from overlapping cells.
`CROSS_CALL_PAIR_OWNERSHIP=LOWER_EDGE_ID_AFTER_PER_QUERY_UNIQUE`.

Runtime pair counters for the latest workload were captured on 2026-10-15:
`SAME_CALL_RAW_RETRIEVED_PAIR_COUNT=66588`, `SAME_CALL_STRUCTURALLY_COMPATIBLE_PAIR_COUNT=10828`,
`SAME_CALL_TOLERANCE_PASS_PAIR_COUNT=55`, `SAME_CALL_CERTIFIED_PAIR_COUNT=55`, `SAME_CALL_AMBIGUOUS_PAIR_COUNT=0`.
`CROSS_CALL_RAW_RETRIEVED_PAIR_COUNT=664920`, `CROSS_CALL_DUPLICATE_PAIR_RETRIEVAL_COUNT=554413`,
`CROSS_CALL_UNIQUE_PAIR_EVALUATED_COUNT=110507`, `CROSS_CALL_STRUCTURALLY_COMPATIBLE_PAIR_COUNT=86530`,
`CROSS_CALL_TOLERANCE_PASS_PAIR_COUNT=151`, `CROSS_CALL_CERTIFIED_PAIR_COUNT=135`, `CROSS_CALL_AMBIGUOUS_PAIR_COUNT=16`.
Search accounting accepted for latest workload; structural/coherence review pending.

## Exact-use plan

`EXACT_USE_PLAN_KEY=workloadId,fbPair,projection,gameCall,triangleLocal,cornerInTriangle,faceCornerIndex,storedVertexId,originalWorldIndex`.

`EXACT_USE_PLAN_MEMBERSHIP_AVAILABLE=YES`.
`EXACT_USE_PLAN_MEMBERSHIP_IS_WORLD_SLOT_GLOBAL=NO`.

`p09SeamEdgeDryRunV2WouldAliasExactUse` binary-searches the latest
workload's certified corners. `p09SeamEdgeDryRunV2AliasPlanExhaustive`
reports whether dropped search or plan caps make a missing key inconclusive.
`PATHOLOGY_CORRELATION_API_READY=YES`.
`PATHOLOGY_CORRELATION_CAPTURED=NO`.
`PATHOLOGICAL_USE_TOTAL=NOT_CAPTURED`.
`PATHOLOGICAL_USE_COVERED_BY_SEAM_ALIAS_PLAN=NOT_CAPTURED`.
`PATHOLOGICAL_USE_NOT_COVERED=NOT_CAPTURED`.
`PATHOLOGICAL_USE_AMBIGUOUS=NOT_CAPTURED`.
`PATHOLOGICAL_USE_COVERAGE_RATE=NOT_CAPTURED`.
`NONPATHOLOGICAL_USE_ALIAS_PLANNED_COUNT=NOT_CAPTURED`.

Certified corners request an elementwise alias. Other corners stay rigid.
`CERTIFIED_ALIAS_IS_PER_EXACT_SEAM_USE_NOT_WHOLE_WORLD_SLOT=YES`.
`UNRELATED_GEOMETRY_IN_SAME_GAMECALL_REMAINS_RIGID_BY_DESIGN=YES`.
`CANDIDATE_ALIAS_COUNT` equals the unique exact-use count. Nothing is
materialized. Alias, clone, and seam totals are `NOT_CAPTURED` here.

The B+C vertex counts use that exact-use set. A stored vertex with both a
certified corner and a rigid corner needs one physical clone. No clone is
allocated.

## Search exhaustiveness

A captured workload before this repair reported
`BOUNDARY_EDGE_COUNT=1481`, one same-call certified seam, and zero
cross-call certified seams, with
`CAP_SAME_CALL_CANDIDATE_DROPPED=4701`,
`CAP_CROSS_CALL_SPATIAL_BUCKET_WALK_DROPPED=180588`,
`CAP_CROSS_CALL_CANDIDATE_PAIR_DROPPED=450026`, and
`CAP_CROSS_CALL_PAIR_DEDUP_PROBE_FAILED=YES`.
`CROSS_CALL_CERTIFIED_EDGE_SEAM_COUNT=0` from that file is not a negative
result.

The repair does not change paired-edge identity, PREV/CURRENT, exact-use
keys, the competing-partner rule, or `EDGE_SCALE_COEFFICIENT=1e-4`.
Same-call search evaluates every unordered pair of boundary edges in the
call and keeps only a streaming best and next-best partner plus the
tolerance-pass pairs. Boundary classification is an exact per-call count of
undirected stored-vertex ids. Cross-call search no longer uses a 4096-bucket
hash, a 48-entry walk, or a fixed dedup table.

Counter names are staged:
`RAW_RETRIEVED`, `STRUCTURALLY_COMPATIBLE`, `TOLERANCE_PASS`,
`UNIQUE_PARTNER_CANDIDATE`, `CERTIFIED`, `AMBIGUOUS`.
`SAME_CALL_CANDIDATE_EDGE_PAIR_COUNT` and
`CROSS_CALL_CANDIDATE_EDGE_PAIR_COUNT` still mean tolerance-pass pairs.
`CAP_SAME_CALL_CANDIDATE_ACCEPTED` means pairs evaluated, not tolerance-pass
pairs. `CROSS_CALL_DUPLICATE_PAIR_RETRIEVAL_COUNT` is raw neighbor hits
before per-query unique, minus pairs owned by the lower edge id. That count
includes multi-cell repeats and the reverse-direction hit. It is not a
failed dedup probe. `CROSS_CALL_PAIR_DEDUP_PROBE_FAILED` stays `NO`.

A static re-read on 2026-10-14 confirmed this repair is still in
`rt64_transform_processor.cpp`. That pass did not edit the processor, did
not rebuild, and did not launch the game. The validation record below is
from 2026-09-24.

Certified detail lines, at most 12, name both edges (GameCall, triangle,
edge in face, face corners, stored vertices, WORLD slots), the orientation,
previous and current endpoint residuals and tolerances, edge lengths, best
residual, next-best residuals, and the competitor margin.

`SEARCH_MEMORY_SCOPE=PER_WORKLOAD`.
`UNBOUNDED_SESSION_STORAGE=NO`.
Session data is fixed aggregate counters, including
`SESSION_PEAK_SEARCH_CPU_MS`, plus the latest workload's exact-use vector,
which is replaced on the next workload. CPU timing is
`LATEST_SEARCH_CPU_MS`, `LATEST_SAME_CALL_CPU_MS`, and
`LATEST_CROSS_CALL_CPU_MS`. There is no GPU timing.

Safety bounds, not search capacities: 1000000 boundary edges or
classification keys, 32000000 same-call pair evaluations, 4000000 unique
cross-call pairs, 1000000 exact uses, and 2000000 vertices in the clone
audit. A hit sets `EMERGENCY_LIMIT_HIT=YES`, records the safety drop, and
clears `LATEST_WORKLOAD_COUNTS_EXHAUSTIVE` or `ALIAS_PLAN_EXHAUSTIVE` as
appropriate. Detail truncation still does not. The edge-scale versus cell
check still refuses an exhaustive alias plan when `1e-4 * edge length`
exceeds the 32-unit cell, because that retrieval neighborhood would be
incomplete. That check is not a candidate cap.

`PATHOLOGY_CORRELATION_API_READY=YES`.
`PATHOLOGY_CORRELATION_CAPTURED=NO`.
The repaired runtime file was captured on 2026-10-15 (`/tmp/tooie-p09-seam-edge-dry-run-v2.txt`, preserved snapshot `/tmp/tooie-p09-seam-edge-dry-run-v2-20261015-030500.txt`). Search accounting is accepted as exhaustive for the latest workload: `SAME_CALL_SEARCH_EXHAUSTIVE=YES`, `CROSS_CALL_BUCKET_SEARCH_EXHAUSTIVE=YES`, `CROSS_CALL_UNIQUE_PAIR_GENERATION_EXHAUSTIVE=YES`, `LATEST_WORKLOAD_COUNTS_EXHAUSTIVE=YES`, `ALIAS_PLAN_EXHAUSTIVE=YES`, `EMERGENCY_LIMIT_HIT=NO`, `RETRIEVAL_CELL_SMALLER_THAN_EDGE_SCALE=NO` (correcting fictitious `RETRIEVAL_RADIUS_RISK`). Detail output cap (`DETAIL_OUTPUT_ONLY_SATURATION=YES`, 12/190 accepted) does not invalidate exhaustive search (`AFFECTS_EXHAUSTIVENESS=NO`). Certification coherence remains pending inspection. Pathology correlation remains `NOT_CAPTURED`.

## Saturation

`ANY_SEARCH_SATURATION=NO`, `ANY_CERTIFICATION_SATURATION=NO`,
`ANY_PLAN_SATURATION=NO`, `DETAIL_OUTPUT_ONLY_SATURATION=YES`,
`LATEST_WORKLOAD_COUNTS_EXHAUSTIVE=YES`, and `ALIAS_PLAN_EXHAUSTIVE=YES` were
confirmed on 2026-10-15 for the latest workload. Detail truncation alone does
not affect search exhaustiveness.

Compatible pairs also append fixed 8-bucket histograms of the numeric
bound, the edge-scale term, and the residual. Those distributions are why
`1e-4` stays diagnostic.

## Cost

`MEMORY_OVERHEAD_STATUS=PARTIAL_ESTIMATE`.
`BUFFER_GROWTH_AUDIT_COMPLETE=NO`.
`DRY_RUN_PARTIAL_ESTIMATE_BYTES` is `aliasCount*6*64 + cloneCount*(55+88)`.
That product is not a measured allocation. Six matrix streams are not a
finished consumer audit. Counting one alias per exact use overcounts if one
call-local matrix later serves many corners. This note does not call B+C
low-cost.

Session peaks kept without a percentile ring:
`SESSION_WORKLOAD_COUNT`, `SESSION_WORKLOADS_WITH_CERTIFIED_EDGE_SEAMS`,
`SESSION_PEAK_CERTIFIED_EDGE_SEAMS`, `SESSION_PEAK_ALIAS_COUNT`,
`SESSION_PEAK_VERTEX_CLONE_COUNT`, `SESSION_PEAK_HYPOTHETICAL_WORLD_COUNT`,
`SESSION_PEAK_SAME_CALL_CERTIFIED_EDGE_SEAMS`,
`SESSION_PEAK_CROSS_CALL_CERTIFIED_EDGE_SEAMS`,
`SESSION_PEAK_SEARCH_CPU_MS`.

## Validation

Tooie HEAD `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`. RT64 HEAD
`6f1c2d99a4ea571c139f449c326fd176ba8f3496`. Dirty worktrees were preserved.
The RT64 edit for the exhaustiveness repair is
`src/render/rt64_transform_processor.cpp`. The header was not changed.
`RecompiledFuncs/` was not edited. Dirty worktrees were preserved.
No commit and no push.

```text
cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test
exit 0
./build/window_display_test           38/38 PASS, exit 0
./build/tintro_projection_policy_test 7/7 PASS, exit 0
git diff --check                      exit 0 (Tooie and the RT64 files above)
```

`GPU_READBACK_USED=NO`.
`GPU_SYNCHRONIZATION_ADDED=NO`.
`ROOT_CAUSE_CLASS=NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`.
`CURRENT_WORLD_INDEX_FIX_STATUS=INSUFFICIENT_GRANULARITY`.
`CLEANUP_READY=NO`.
`COMMIT_READY=NO`.
`NO_COMMIT=YES`.
`NO_PUSH=YES`.

`NEXT_SINGLE_ACTION`: inspect certified paired-edge A/B provenance and residual/tolerance/margin details from preserved snapshot; only after coherent certification proceed exact-use pathology correlation. Do not implement aliases yet.
