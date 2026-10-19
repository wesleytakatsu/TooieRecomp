---
title: "P09 Kazooie feather V3 capture analysis"
description: "Current V3 capture reaches the real feather MATRIX/G_MTX path but not VERTEX or MatchTransform."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-04
tags: [tooie-recomp, p09, kazooie, feathers, rt64, matrix, vertex, interpolation]
---

# P09 Kazooie feather V3 capture analysis

## Capture validity (`PROVEN`)

The capture files were generated at 04:31:43–04:32:48 -0300 on 2026-09-04.
The current `build/TooieRecompiled` is timestamped 04:25:05 -0300 and the
current diagnostic source is timestamped 04:24:56 -0300. Therefore the
capture postdates both the executable and the source. The status file reports
`P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V3`.

The primary witness is consistently resource `0x62B`, model bin
`0x8022A510`, node `0x8023C9B0` (`+0x124A0`), bone index 30, boneAnimID 103.
The identity is `HIGH` for this capture.

```text
CAPTURE_POSTDATES_CURRENT_EXECUTABLE: YES
CAPTURE_USABLE: YES
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V3
PRIMARY_FEATHER_ACCEPT_COUNT: 2421
```

## MATRIX → G_MTX identity (`PROVEN`)

The producer matrix CSV contains 512 rows (frames 602–1113). Every row has
the same composed-float source `0x8007B6F0`, and every row reports
`SOURCE_MATCHES_COMPOSED=YES`. The F2L destination equals the captured G_MTX
operand in all 512 rows. The destination is a rotating 15-entry guest Mtx
ring; the raw pointer is not stable, but the model/node/bone semantic identity
is stable.

The token counters report 2421 enqueues, 2421 nonzero matrix workloads, and
2421 destination-equal G_MTX matches. The identity CSV is capped at 1536 rows
and all 1536 rows are `MATRIX`; no VERTEX row is present. There is no
`tooie-kazooie-feather-rt64-match.csv` in this capture.

```text
MATRIX_TOKEN_COUNT: 2421
VERTEX_BIND_COUNT: 0
VERTEX_BIND_EXACT_COUNT: 0
VERTEX_BIND_FALLBACK_COUNT: 0
VERTEX_BIND_FAILURE_COUNT: NOT_INSTRUMENTED; no VERTEX event was observed
FEATHER_TOKEN_BOUND_TO_VERTEX_RUNTIME: NO
SAME_TRANSFORM_ACTIVE_AT_VERTEX: UNKNOWN
PRIMARY_FEATHER_VISIBLE_DL: UNKNOWN (only GFX command addresses were logged)
PRIMARY_FEATHER_VERTEX_RANGE: NOT_CAPTURED
INTERVENING_MATRIX_COMMANDS: NOT_CAPTURED; identity output is MATRIX-only
```

This is a mechanical boundary, not an address-key failure. The primary
identity path is:

```text
mlMtxApply source = 0x8007B6F0
→ guMtxF2L destination = captured F2L/G_MTX destination
→ G_MTX guest operand = same destination
→ physical operand = guest address with KSEG0 form removed
```

The capture reports zero address-normalization failures, zero source-equal
operand matches, and 2421 destination-equal operand matches.

## Guest rotation (`VALID`, no corruption anomaly proven)

The producer CSV contains 2568 target feather samples for each seam. The
scale-normalized quaternion witness reports:

```text
ROTATION_SAMPLE_COUNT: 2568
PARENT107_MAX_ANGULAR_DELTA: 0.552594 rad
BONE103_LOCAL_MAX_ANGULAR_DELTA: 0.281985 rad
BONE103_FINAL_MAX_ANGULAR_DELTA: 2.891356 rad
PARENT107_ROTATION_CONTINUITY: DISCONTINUOUS at capture-local animation transitions
BONE103_LOCAL_ROTATION_CONTINUITY: SMOOTH apart from bounded transition steps
FINAL_FEATHER_ROTATION_CONTINUITY: DISCONTINUOUS at propagated parent transitions
ORTHOGONALITY_ERROR_RANGE: 0..7.93859e-05
GUEST_ROTATION_ANOMALY_COUNT: 0 proven numerical-corruption anomalies
FIRST_GUEST_ROTATION_ANOMALY_FRAME: NONE
GUEST_ROTATION_VALID: YES
FIRST_BAD_GUEST_ROTATION_STAGE: NONE_OBSERVED
```

The largest observed final-rotation steps occur at frames 529, 3036, and
3037. They are accompanied by animation/scale transition data and valid
finite matrices. The composed-float seam has determinant 1 and zero measured
orthogonality error; the lower fixed-matrix determinant tracks uniform scale,
as established by the prior V2 witness. These samples do not prove a guest
rotation bug and are not treated as a normal-sway amplitude failure.

## RT64 temporal matching (`UNKNOWN`)

Because no VERTEX event bound a token, the real match observer had no eligible
trace. Consequently all of these are observed zero, not proven rejections:

```text
TOKEN_BOUND_TO_GAMEFRAME_MATCH: 0
TOKEN_BOUND_TO_CALL_STAGE: 0
TOKEN_BOUND_TO_TRANSFORM_PREREQUISITE: 0
TOKEN_BOUND_TO_CANDIDATE_STAGE: 0
TOKEN_BOUND_TO_MATCHTRANSFORM: 0
REAL_CALL_MATCHED: 0
REAL_CALL_UNMATCHED: 0
REAL_MATCHED_COUNT: 0
REAL_UNMATCHED_COUNT: 0
SEMANTIC_MATCH_CORRECT_COUNT: 0
SEMANTIC_MATCH_FAILURE_COUNT: 0
SEMANTIC_CROSS_MATCH_COUNT: 0
SAME_FEATHER_MATCHES_CORRECT_PREVIOUS_FEATHER: UNKNOWN
PREVIOUS_FRAME_MATCH_CONTINUITY: UNKNOWN
```

`RT64_MATCH_NO_TOKEN=496179` is an aggregate of unrelated match observations;
it is not a feather-specific AUTO identity failure. Synthetic finalization
rows are also absent. The capture therefore cannot distinguish
`RT64_AUTO_TRANSFORM_IDENTITY_BUG` from `RT64_POST_MATCH_INTERPOLATION_BUG`.

## Classification

```text
KAZOOIE_DIFFERS_FROM_SNOUT_AT: VERTEX/MatchTransform observability boundary; not G_MTX address identity
FIRST_REAL_DIVERGENCE_FRAME: UNKNOWN
FIRST_BAD_LAYER: UNKNOWN (MATRIX→VERTEX boundary not crossed)
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
MISSING_GROUP_CAUSAL_STATUS: UNKNOWN
BANJO_STYLE_AB_TEST_JUSTIFIED: NO
PRODUCTION_CANDIDATE_JUSTIFIED: NO
NEXT_SINGLE_ACTION: structural reverse-engineering of the runtime VERTEX callback/workload handoff
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
P09_KNOWLEDGE_PROMOTED_TO_RE_DOCS: YES
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

No identical capture is requested. The next useful step is to resolve why
the current Tooie runtime emits the MATRIX witness but never invokes the
bounded VERTEX handoff for this display-list path; only then can semantic
previous-frame matching or post-match interpolation be classified.
