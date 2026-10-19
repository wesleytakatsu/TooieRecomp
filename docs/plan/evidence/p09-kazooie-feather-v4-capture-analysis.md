---
title: "P09 Kazooie feather V4 stack-aware RT64 capture"
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-04
tags: [p09, kazooie, feather, rt64, transform-interpolation, capture]
---

# P09 Kazooie feather V4 stack-aware RT64 capture

## Capture validity

The capture files are newer than the executable used to generate them:

```text
CAPTURE_POSTDATES_CURRENT_EXECUTABLE: YES
executable: 2026-09-04 06:03:36 -0300
capture/status: 2026-09-04 06:14:56 -0300
schema: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V4
CAPTURE_USABLE: YES
```

The V4 status identifies `RT64_MATCH_WITNESS=REAL_MATCHTRANSFORM` and
`RT64_TOKEN_STACK=MODELVIEW_PARALLEL`. It is not the retired V3 scalar-token
witness.

## V4 token-stack and geometry result

The primary witness is `resource 0x62B`, node `+0x124A0`, bone index 30,
`boneAnimID=103`. The capture reports 1,874 token enqueues, all keyed by the
G_MTX destination, and 1,874 restore aliases with no restore ambiguity. The
identity CSV contains MATRIX, VERTEX, and RESTORE events; it is capped at 1,536
rows, so its 308/921/307 event counts are not whole-run totals.

```text
GMTX_TOKEN_PUSH_COUNT: 1874 (matched feather token enqueues)
CHILD_MATRIX_SLOT_COUNT: not separately instrumented
GMTX_TOKEN_RESTORE_COUNT: 1874 (restore aliases observed)
TOKEN_RESTORE_ALIAS_COUNT: 1874
TOKEN_RESTORE_ALIAS_USED_COUNT: 1874 (the only alias counter increments on use)
TOKEN_RESTORE_ALIAS_AMBIGUOUS: 0
TOKEN_STACK_OVERFLOW_COUNT: not separately instrumented
TOKEN_STACK_UNDERFLOW_COUNT: not separately instrumented
TOKEN_AMBIGUITY_COUNT: 0 for restore alias selection
VERTEX_BIND_COUNT: 5622 (derived from TOKEN_VERTEX_WORKLOAD_ZERO)
VERTEX_BIND_EXACT_COUNT: 5622 observed explicit-token binds; no fallback binds
VERTEX_BIND_RESTORE_ALIAS_COUNT: not separately instrumented
MODELVIEW_TOKEN_STACK_WORKING: YES
```

The callback-time `VERTEX_COUNT` is zero because `setVertexCommon` creates the
world-transform record before appending the vertices. This is not a no-geometry
result. Static GeoCmd reconstruction identifies the visible primary plume DL
as `+0x12580`, type `0x11`, DL `0x5AA`; the V4 capture observes the corresponding
G_VTX/world-transform path. Thus:

```text
VISIBLE_DL_OBSERVED: YES (static DL plus runtime VERTEX callback)
PRIMARY_FEATHER_DL_INDEX: 0x5AA
PRIMARY_FEATHER_VERTEX_RANGE: callback exposes starts but zero count; final span not serialized
SAME_BONE103_TRANSFORM_ACTIVE_AT_VERTEX: YES
BONE103_TOKEN_BINDS_ACTUAL_VISIBLE_GEOMETRY: YES
```

## Guest matrix

The V4 capture does not contradict the established guest result. All 512
bounded composed matrices are finite, the composed source matches the sampled
matrix source in all 512 rows, and the F2L destination equals the G_MTX operand
in all 512 rows. The producer capture has 3,810 target bone-103 rows with no
nonfinite or badly normalized final quaternion.

```text
GUEST_MATRIX_VALID: YES
GUEST_ROTATION_VALID: YES
GUEST_ROTATION_ANOMALY_COUNT: 0 numerical-corruption anomalies
```

## Real RT64 match path

The V4 match CSV contains 328 CALL rows, all with `CALL_HASH_MATCHED` and
`CALL_FAILURE_REASON=NONE`; 328 transform-prerequisite rows are eligible; 500
candidate rows are valid; and 133 real decisions select 126 matches and skip
7. The 1,741 `SYNTHETIC_FINALIZATION_ONLY` rows are excluded from these real
failure counts.

```text
TOKEN_BOUND_TO_GAMEFRAME_MATCH: 133
TOKEN_BOUND_TO_CALL_STAGE: 84
TOKEN_BOUND_TO_TRANSFORM_PREREQUISITE: 84
TOKEN_BOUND_TO_CANDIDATE_STAGE: 124
TOKEN_BOUND_TO_MATCHTRANSFORM: 126
REAL_CALL_MATCH_ATTEMPTS: 328
REAL_CALL_MATCHED: 328
REAL_CALL_UNMATCHED: 0
CALL_MATCH_RATE: 100 percent
TRANSFORM_PREREQUISITE_PASS_COUNT: 328
TRANSFORM_PREREQUISITE_FAIL_COUNT: 0
REAL_CANDIDATE_COUNT: 500
REAL_DECISION_COUNT: 133
REAL_MATCHED_COUNT: 126
REAL_UNMATCHED_COUNT: 7
REAL_REJECTION_REASON_HISTOGRAM:
  ONE_TO_ONE_CURRENT_ALREADY_USED=5
  ONE_TO_ONE_PREVIOUS_ALREADY_USED=2
```

The seven real unmatched rows are at frames 1195, 1259, 1299, 1311, 1339,
1609, and 1624. Every one is an allocator collision after a valid candidate;
none is `NO_PREVIOUS_CALL`, range mismatch, transform-disabled, matrix-ID, or
candidate-invalid rejection.

## Semantic previous-frame result

The capture contains 13 selected semantic feather contexts (48 call-layout
contexts before candidate/decision reduction). In all 126 selected matches,
current and previous workloads are adjacent, call hashes and scene projection
identity agree, and the selected previous transform is the valid candidate in
the same context. No cross-context match was observed.

```text
SEMANTIC_FEATHER_CONTEXT_COUNT: 13 selected contexts
SEMANTIC_MATCH_CORRECT_COUNT: 126
SEMANTIC_MATCH_FAILURE_COUNT: 0
SEMANTIC_CROSS_MATCH_COUNT: 0
SEMANTIC_DROPPED_MATCH_COUNT: 7 allocator drops
SAME_FEATHER_MATCHES_CORRECT_PREVIOUS_FEATHER: YES for observed matched contexts
PREVIOUS_FRAME_MATCH_CONTINUITY: STABLE for observed matches
```

The CSV does not serialize a second model/node witness for the previous frame,
so the same-feather statement is a semantic-context result, not an independent
previous-resource pointer proof. This is sufficient to reject an observed
AUTO-identity failure, but not sufficient to prove the visual root.

## Classification and next experiment

```text
AUTO_IDENTITY_STATUS: CORRECT for observed matches
EXPLICIT_IDENTITY_GROUP_NEEDED_FOR_MATCH: NO for observed AUTO matching
SIMPLE_INTERPOLATION_MODE_COULD_AFFECT_VISUAL_RESULT: YES
FIRST_UNEXPLAINED_LAYER: RT64_POST_MATCH_INTERPOLATION_OR_LATER
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
```

The Banjo-style A/B is justified only as a bounded causal experiment, not as
a production promotion. A should retain current ungrouped skeletal G_MTX/AUTO
behavior. B should generically add a stable model-instance ID, a deterministic
sorted GeoCmd transform offset, `gEXMatrixGroup` immediately after the bone
G_MTX and before child traversal, and a matching group pop at the end of the
bone traversal. It must not special-case resource `0x62B`, node `0x124A0`, or
bone 103. The identity part is not needed to make the observed matches work;
the controlled visual comparison primarily tests whether the explicit group
interpolation mode changes the post-match result.

```text
BANJO_STYLE_AB_TEST_JUSTIFIED: YES, bounded experiment only
PRODUCTION_CANDIDATE_JUSTIFIED: NO
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
```

Provenance: `/tmp/tooie-kazooie-feather-*` and
`/tmp/tooie-secondary-sway-status.txt`, captured 2026-09-04.
