---
title: "P09 V4 real RT64 match witness"
description: "Static implementation record for observing real GameFrame call and transform matching decisions for the proven 0x62B snout token."
status: evidence
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-03
tags: [tooie-recomp, p09, rt64, transform-interpolation, witness]
---

# P09 V4 real RT64 match witness

## Scope

This checkpoint implements the requested diagnostic witness only. It does not
change interpolation behavior, transform selection, grouping, or production
rendering. The diagnostic remains disabled unless
`TOOIE_DIAG_SECONDARY_SWAY=1` is set.

## Static RT64 flow

```text
GameFrame::match
  -> workload pairing by frame order
  -> matchScene
  -> buildCallHashMap / hashFromCall
  -> same-hash call pairs
  -> transformCheckSet (same-position world matrices)
  -> computeTransformMatch
  -> stable-sort and one-to-one mapped checks
  -> GameFrame::matchTransform
  -> tooie_diag_rt64_match
```

`matchTransform` is a successful mapping callback, not the rejection point.
V4 therefore observes the call pair, transform prerequisites, candidate
validity/metrics, and one-to-one decision before the existing callback. A final
step runs immediately after `GameFrame::match` and emits `REAL_UNMATCHED` for
pending correlated snout tokens. `FLUSH_SENTINEL` is retained only for tokens
that never reached the complete matrix/vertex witness.

## Recorded context

The bounded V4 CSV is
`/tmp/tooie-secondary-sway-rt64-snout-match-v4.csv`. Each row includes the
current/previous workload IDs, call hashes and indexes, scene-projection
indexes, current/previous world ranges, transform and group/matrix indexes,
projection type/index, candidate position/orientation/screen-space metrics,
selection result, and a single rejection reason.

The context key is derived from the current call hash, scene-projection index,
call index, and current world-transform range. This prevents comparing snout
transforms from unrelated render contexts. The existing guest token binding is
reused; no new guest token architecture was added.

## Reason vocabulary

```text
NO_PREVIOUS_CALL_MATCH
RANGE_POSITION_MISMATCH
TRANSFORM_MATCHING_DISABLED
MATRIX_ID_MISMATCH
MATRIX_ID_NOT_AUTO_OR_ORDERED
DETERMINANT_MIRROR_REJECT
ONE_TO_ONE_CURRENT_ALREADY_USED
ONE_TO_ONE_PREVIOUS_ALREADY_USED
MATCHTRANSFORM_TOKEN_NOT_BOUND
NO_CANDIDATE_OBSERVED
FLUSH_SENTINEL_ONLY (secondary only)
```

## Validation

```text
BUILD: cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test
BUILD_EXIT: 0
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
GIT_DIFF_CHECK: PASS
GAME_RUN: NO (required by task)
GPU_READBACK: NO
GPU_SYNCHRONIZATION: NO
```

## Classification and next action

```text
FINAL_GUEST_SNOUT_TRANSFORM_VALID: YES (prior V3 runtime proof)
MISSING_GROUP_CAUSAL_STATUS: HYPOTHESIS
ROOT_CLASS: RT64_MATCH_WITNESS_UNRESOLVED
ROOT_CONFIDENCE: LOW
RAW_V3_EVIDENCE_DUPLICATION_CLEANED: NOT_NEEDED (V3 page is distilled)
USER_MANUAL_TEST_REQUIRED_NOW: YES
NEXT_SINGLE_ACTION: run one V4 capture and analyze the real match CSV
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
NO_PRODUCTION_FIX: YES
NO_COMMIT: YES
NO_PUSH: YES
```

## Authoritative V4 capture result (`CORRECTED`, 2026-09-03)

Capture files under `/tmp/tooie-secondary-sway-*` postdate the current
executable (`04:39` capture files versus `04:15` executable). The status file
identifies schema V4 and `REAL_MATCHTRANSFORM`; the capture is therefore
usable for auditing the witness, but not for claiming an RT64 rejection.

```text
CAPTURE_USABLE: YES (guest proof and witness plumbing; RT64 decision not observed)
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V4
SOURCE_MATCHES_COMPOSED_YES: 512
SOURCE_MATCHES_COMPOSED_NO: 0
RT64_TOKEN_ENQUEUED: 2238
RT64_TOKEN_CONSUMED: 2236
TOKEN_CORRELATION_RATE: 99.91%
REAL_CALL_MATCH_ATTEMPTS: 0 (CALL rows: 0)
REAL_CALL_MATCHED: 0
REAL_CALL_UNMATCHED: 0 (not observed)
TRANSFORM_PREREQUISITE_PASS_COUNT: 0 (not observed)
TRANSFORM_PREREQUISITE_FAIL_COUNT: 0 (not observed)
REAL_CANDIDATE_COUNT: 0 (not observed)
REAL_MATCHED_COUNT: 0 (MATCHED rows: 0)
REAL_UNMATCHED_COUNT: 2236 (finalization fallback only)
REAL_REJECTION_REASON_HISTOGRAM: NO_CANDIDATE_OBSERVED=2236
SEMANTIC_MATCH_CORRECT_COUNT: 0 (not observed)
SEMANTIC_MATCH_FAILURE_COUNT: 0 (not observed)
SEMANTIC_CROSS_MATCH_COUNT: 0 (not observed)
SAME_SNOUT_MATCHES_CORRECT_PREVIOUS_SNOUT: UNKNOWN
FIRST_REAL_MATCH_FAILURE_FRAME: 616 (synthetic finalization row, not an RT64 failure)
REAL_REJECTION_REASON: NO_CANDIDATE_OBSERVED
FIRST_BAD_RT64_STAGE: NOT_PROVEN; witness binding precedes CALL stage
FIRST_BAD_LAYER: V4 diagnostic workload-identity binding
ROOT_CLASS: DIAGNOSTIC_ARGUMENT_MISMATCH_MATRIX_VS_VERTEX_WORKLOAD
ROOT_CONFIDENCE: HIGH (for witness failure; RT64 graphics root remains UNKNOWN)
MISSING_GROUP_CAUSAL_STATUS: UNKNOWN (not reached)
PRODUCTION_CANDIDATE_JUSTIFIED: NO
PROPOSED_FIX_LAYER: V4 diagnostic plumbing only
PROPOSED_GENERIC_SCOPE: NONE
NEXT_SINGLE_ACTION: preserve nonzero matrix-event workload when vertex workload is zero, rebuild, then capture once
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
```

The identity CSV provides the discriminator: `MATRIX` rows carry workload
IDs 773 onward, while `VERTEX` rows carry workload ID 0. The vertex callback
therefore replaces the token's valid workload identity before
`GameFrame::matchScene`; zero V4 `CALL`/candidate rows do not establish an
RT64 AUTO-matching failure. No new raw evidence file was created in the
canonical reverse-engineering tree.

## V5 diagnostic repair (`IMPLEMENTED_STATIC`, 2026-09-03)

The V4 witness plumbing is corrected without changing the proven guest-to-
G_MTX path. Each token now keeps separate `matrix_workload_id`,
`vertex_workload_id`, and `match_workload_id` fields. `match_workload_id`
preserves the non-zero `matrixCommon` identity when `setVertexCommon` supplies
zero, while retaining the vertex workload as context.

The finalization fallback no longer increments `REAL_UNMATCHED` or emits a
real-failure row. It emits `SYNTHETIC_FINALIZATION_ONLY`; only an observed
non-selected GameFrame decision can emit `REAL_UNMATCHED`. The witness schema
is now V5 and remains default-off, bounded, CPU-only, and diagnostic-only.

```text
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V5
TOKEN_WORKLOAD_BINDING: MATRIX_WORKLOAD_PRESERVED
TOKEN_VALID_WORKLOAD_OVERWRITE_BLOCKED: YES
REAL_UNMATCHED_REQUIRES_REAL_MATCH_DECISION: YES
SYNTHETIC_FINALIZATION_ONLY: SEPARATE_FROM_REAL_UNMATCHED
REAL_MATCHTRANSFORM_WITNESS_RETAINED: YES
PRODUCTION_FIX: NO
```
