---
title: "P09 seam-alias dry-run validity and coverage review"
description: "Static review of seam identity, saturation, cost, and the missing corrected V2 use-level correlation."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-24
tags: [tooie-recomp, p09, rt64, interpolation, seam, review]
---

# P09 seam-alias dry-run validity and coverage review

## Scope and evidence

CPU/static review only; no game run, GPU operation, production alias, code edit,
commit or push. Tooie HEAD `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf`;
RT64 HEAD `6f1c2d99a4ea571c139f449c326fd176ba8f3496`, both with protected
dirty worktrees. The supplied and local `/tmp/tooie-p09-seam-alias-dry-run.txt`
agree on the latest workload and session counts. User visual A/B history is
accepted as `USER_OBSERVED`; the dry-run does not change rendering.

`ARCHITECTURE_DECISION=CASE_B`. The call-local B+C data path remains
`CONDITIONAL`, but the current point-pair certification is too permissive to
drive production aliases. The next mechanical task is a new default-off CPU
dry-run identity rule, not alias materialization.

## Interpretation of the current capture

`SEAM_PREVALENCE_PLAUSIBLE=UNCERTAIN`. 9857/11903 (82.81%) repeated workloads
have at least one reported certified pair. Frequent shared boundaries and
repeated character draws can produce this pattern; workloads are not
independent observations. The count cannot establish true attachment
prevalence. `CURRENT_CERTIFICATION_FALSE_POSITIVE_RISK=HIGH` for production
selection: `testPair` checks two world-space point distances, finite affine
endpoints, distinct WORLD slots and previous mapping. It does not establish
paired-edge topology, original vertex occurrence, persistent call relation,
or whether two coincident surfaces are one attachment. The cross-call scan
samples every fourth face corner, at most 24 per call, and only its eight
nearest hash neighbors. The hash is retrieval, not identity. Multiple draws
of the same geometry, duplicated vertices, coincident surfaces, adjacent
objects and shared pivots can pass two-frame point coincidence. Ambiguity
removes multiple partners for one stored vertex in the sampled set, not
one-to-one identity across all vertex occurrences or frames.

`ZERO_SAME_CALL_CERTIFIED_IS_PLAUSIBLE=YES`: the same-call pass tests only
edges *within triangles* whose endpoints have distinct WORLD indices. It
does not enumerate distinct triangles' duplicated boundary vertices.
All 268 tested edges fail at PREV. This is consistent with mixed-WORLD
triangles whose edges have length, not coincident endpoints.
`CROSS_CALL_DOMINANCE_IS_PLAUSIBLE=YES`, but true attachments are not proven:
the cross-call pass is a different sampled point-neighbor test, yielding 29
certified pairs in the latest workload. `DOES_THIS_CONTRADICT_PREVIOUS_MULTI_WORLD_VISUAL_SUCCESS=NO`:
the broad index selector and this seam detector select different units.
Visual success from a multi-WORLD call does not imply the required join is
an intra-triangle zero-length edge.

`CURRENT_TOLERANCE_MATHEMATICALLY_JUSTIFIED=NO`. The coordinates are in
WORLD/model units; the 0.25 floor can merge distinct quarter-unit features.
For magnitudes above 2500, `1e-4 * max(localMag,worldMag)` dominates, so
large absolute world translation can admit unrelated points even when local
edges are small. No residual or numeric-error distribution justifies either
constant. The corrected V2 WORLD-space join residual reached about 1 unit;
that aggregate does not show how many pathological uses satisfy this 0.25
gate. `CURRENT_0_25_FLOOR_FALSE_MATCH_RISK=HIGH` as an identity rule.
`RECOMMENDED_SCALE_AWARE_TOLERANCE`: measure endpoint residuals and nearest
alternative gaps on the corrected V2 uses first. Use a bounded combination
of transformed local edge scale, known vertex quantization and a floating
point error bound from `|WORLD| * |localPos4|`; exclude absolute world
translation as a geometric scale. Require the paired geometry to pass at
both endpoints with a margin below competing neighbors. Keep constants
diagnostic until these distributions are captured; do not silently relax the
gate to cover a witness.

`POINT_PAIR_TWO_ENDPOINT_CERTIFICATION_SUFFICIENT=NO`.
`MINIMUM_ADDITIONAL_IDENTITY_SIGNAL=paired boundary edge with two distinct,
one-to-one endpoint correspondences, compatible face/call provenance, and
both-frame residual checks`. This is the first generic structural signal.
Ranked by benefit/cost: (1) face-edge and vertex-occurrence provenance,
paired edge direction/length at both frames, moderate CPU cost; (2)
one-to-one partners and stable call relation across diverse frames, bounded
history cost; (3) normal-side compatibility, cheap when normals are reliable;
(4) UV/color continuity, optional because real seams can be discontinuous.
Matching motion delta follows from two-frame endpoint equality and adds
little independent proof. A persistent shared pivot with no paired edge
remains `UNKNOWN` until separate evidence can establish identity.

## Missing coverage proof

`PATHOLOGICAL_COVERAGE_CORRELATION_REQUIRED_BEFORE_ALIASES=YES`.
The corrected V2 `P09JoinVert` records WORLD and transformed positions, but
not a stable `(workload, framebuffer pair, projection, GameCall, face corner,
stored vertex, WORLD)` use key. Its nearest parent/child pair and pathology
count cannot be joined to the dry-run plan by WORLD index or bone alone.
The V2 prior 156/156 elementwise result is a causal counterfactual for those
sampled join pairs, not alias-plan coverage.

`CORRELATION_DIAGNOSTIC_DESIGN`: add the exact CPU face-corner use key to each
V2 join vertex while the V2 face traversal has it, and carry the exact two
use keys into each corrected pathological pair. Reuse the same shader-equivalent
V2 pathology predicate and WORLD endpoint semantics; do not restore old
MODELVIEW or invalid `mul` order. Make the revised seam dry-run expose the
planned remap set keyed by those uses. At the same workload/phase, join both
pathological endpoints to that set, counting total pairs and also the exact
required uses: `PATHOLOGICAL_USE_TOTAL`,
`PATHOLOGICAL_USE_COVERED_BY_SEAM_ALIAS_PLAN`,
`PATHOLOGICAL_USE_NOT_COVERED`, `PATHOLOGICAL_USE_AMBIGUOUS`,
`PATHOLOGICAL_USE_COVERAGE_RATE`, and
`NONPATHOLOGICAL_USE_ALIAS_PLANNED_COUNT`. Report reasons for misses and
separate `NO_FRAME_MATCH`, `SEARCH_CAP`, `IDENTITY_REJECT` and `UNKNOWN`.
Every pair is covered only when both of its exact call/corner/vertex/WORLD
uses would actually receive an elementwise alias; deduplicate uses for the
use metrics. Bound records and emit aggregate counts only. Correlation is a
diagnostic; it does not remap vertices or alter rendering.

## Bounds and cost

`BOUNDS_SATURATED_AFFECTS_COUNTS=YES`. The single flag covers skipped calls,
96 cross-world edges per call, 3072/24-per-call sampled corners, a 1024
provisional-pair cap, bounded hash/pair-table probes, 1024 aliases, eight
alias WORLDs per call, or vertex-audit cap. `VERTEX_AUDIT_CAPPED=NO` rules out
only one cause for the latest workload. The capture does not identify which
other limit fired. Same/cross candidates, certified pairs and alias counts
can be lower than exhaustive counts; sampling also creates false negatives.
`BOUNDS_SATURATED_AFFECTS_ALIAS_PLAN=YES`: omitted pairs cannot create
aliases, and an alias or per-call WORLD cap can truncate the plan itself.
`COUNTS_TRUSTWORTHY_FOR_ARCHITECTURE=PARTIAL`: the reported counts and session
peaks describe what this bounded algorithm observed, not complete seam
coverage, worst-case alias count or production cost. The session has no
saturation frequency or per-limit counters.

`BUFFER_GROWTH_AUDIT_COMPLETE=NO`. The named five CPU matrix vectors and four
GPU matrix buffers cover the immediate WORLD matrix path. The full audit
still must trace every WORLD-index consumer, `worldTransformGroups` and
original matching ranges, vertex clone arrays, output buffers, modified
vertex references and the raw-triangle path. The 54,234-byte peak is an
arithmetic estimate from `139 * 6 * 64 + 6 * 143`, not measured allocation.
It counts six 64-byte matrix copies per alias even though the named list has
five CPU vectors and four GPU buffers; allocator capacity and alias metadata
are also unmeasured. `MEASURED_COST_SUPPORTS_B_PLUS_C=CONDITIONAL`: observed
bounded counts are encouraging, but neither complete coverage nor total
memory has been established. The 16-bit fit is proven only for observed
hypothetical counts, not future unsaturated workloads.

`ROOT_CAUSE_CLASS=NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS`.
`PRODUCTION_FIX_UPDATED=NO`; `CLEANUP_READY=NO`; `COMMIT_READY=NO`.
`GPU_READBACK_USED=NO`; `GPU_SYNCHRONIZATION_ADDED=NO`;
`NO_COMMIT=YES`; `NO_PUSH=YES`.

`NEXT_SINGLE_ACTION`: implement only a versioned, default-off CPU seam
identity dry-run with paired-edge/provenance tests, explicit per-limit
saturation counters and exact-use plan output for subsequent V2 correlation.
No aliases, remap, shader edit, GPU operation, or game launch in that edit.
