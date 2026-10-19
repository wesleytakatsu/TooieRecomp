---
title: "P09 V3 real guest matrix and RT64 previous-frame match"
status: current
type: evidence
project: TooieRecomp
updated: 2026-09-03
tags: [p09, secondary-sway, snout, guest-matrix, rt64, interpolation]
---

# P09 V3 real guest matrix and RT64 previous-frame match

## Capture validity

The capture was preserved at `/tmp/tooie-secondary-sway-old.ZV7Gmx/` after
the accidental second launch moved the old root files instead of deleting
them. It is usable for the V3 objective.

```text
CAPTURE_POSTDATES_CURRENT_EXECUTABLE: YES
CURRENT_EXECUTABLE: 2026-09-03 03:10:04 -0300
LATEST_CAPTURE_FILE: 2026-09-03 03:30:08 -0300
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V3
CAPTURE_USABLE: YES
```

The executable contains the V3 strings and uses `D_8007BCB0`; the capture
status confirms the same schema. V1/V2 composition values are not used here.

## Guest pointer chain

V3 proves the real chain for the filtered snout (`resource=0x62B`,
`node_rel=0x11CB8`, `bone_index=11`, `boneAnimID=42`):

```text
DCF48/ADE2C
  -> func_80019AA0
  -> composed float matrix at MEM_W(D_8007BCB0) = 0x8007B630
  -> guMtxF2L source = 0x8007B630
  -> guMtxF2L destination = entry a0 in the GFX Mtx ring
  -> G_MTX operand = that destination
  -> RT64 guest/physical matrix address = that same G_MTX operand
```

Measured in `tooie-secondary-sway-snout-matrix.csv`:

```text
GUEST_MATRIX_SAMPLE_COUNT: 512
SOURCE_MATCHES_COMPOSED_YES: 512
SOURCE_MATCHES_COMPOSED_NO: 0
SOURCE_MATCH_RATE: 100%
G_MTX_OPERAND_MATCH_COUNT: 512
RT64_TOKEN_CONSUMED_COUNT: 2567 / 2568
TOKEN_DEST_EQ_GMTX_OPERAND: 2567
TOKEN_SOURCE_EQ_GMTX_OPERAND: 2567
TOKEN_ADDRESS_NORMALIZATION_FAILURE: 0
REAL_GUEST_MATRIX_CORRELATION_WORKING: YES
```

The one token not consumed is the diagnostic ring's expired candidate. It is
not an address-normalization failure. The matrix CSV contains two identical
rows per frame/bin sample; all 512 persisted rows have finite 16-float data,
and all 512 have `GFX_MATRIX_EQUALS_F2L_DEST=YES`.

## Guest matrix quality

The real V3 matrix is finite and structurally valid. Across the two observed
model bins, persisted consecutive samples have positive determinant throughout
(`0.999973...` to `0.999999...`), with gradual rotation/translation changes
consistent with locomotion. No true guest-matrix anomaly is observed.

```text
NAN_INF_COUNT: 0
TRANSLATION_CONTINUITY: SMOOTH
ROTATION_CONTINUITY: SMOOTH
SCALE_CONTINUITY: SMOOTH
DETERMINANT_SIGN_STABLE: YES
TRUE_GUEST_MATRIX_ANOMALY_COUNT: 0 observed
STRONGEST_GUEST_MATRIX_ANOMALY: NONE_OBSERVED
FINAL_GUEST_SNOUT_TRANSFORM_VALID: YES
```

The largest adjacent translation steps were about 70.13 and 47.69 units in
the two model-bin runs; these are not classified as anomalies without a
semantic discontinuity or invalid matrix.

## Render contexts and RT64 matching

The bounded RT64 identity CSV contains 768 matrix/vertex pairs. Its vertex
rows show 13 repeated transform/range signatures, all under projection
`1/1`, `othermode H=0x00982C3F`:

| Transform | Draw range | Samples |
| --- | --- | ---: |
| `0x31` | 47–48 | 19 |
| `0x32` | 48–49 | 125 |
| `0x33` | 49–50 | 225 |
| `0x34` | 50–51 | 73 |
| `0x35` | 51–52 | 37 |
| `0x36` | 52–53 | 13 |
| `0x3A` | 56–57 | 66 |
| `0x3B` | 57–58 | 82 |
| `0x3C` | 58–59 | 50 |
| `0x3D` | 59–60 | 20 |
| `0x3E` | 60–61 | 25 |
| `0x54` | 82–83 | 3 |
| `0x55` | 83–84 | 30 |

```text
DISTINCT_SNOUT_RENDER_CONTEXT_COUNT: 13 observed signatures
SIX_ORDINALS_EXPLAINED_AS_STABLE_CONTEXTS: PARTIAL
PROJECTION_TYPE: 1
PROJECTION_INDEX: 1
OTHERMODE_H: 0x00982C3F
MATRIX_ID_MODE: AUTO (0xFFFFFFFF)
EXPLICIT_GROUP_PRESENT: NO
```

The repeated range/transform signatures explain the bounded identity rows,
but `DRAW_UID` is always zero and no stable call hash is captured. Therefore
these signatures do not yet prove one-to-one semantic context identity across
frames. V3 has more than the six V2 ordinals in the persisted capture.

The authoritative match CSV is capped at 2,048 rows. Every row is:

```text
EVENT=MATCH
MATCH_STATUS=UNMATCHED
REASON=NO_MATCHTRANSFORM
CUR_WORKLOAD=0
PREV_WORKLOAD=0
PREV_TRANSFORM=0xFFFFFFFF
```

The status totals are `RT64_MATCH_MATCHED=0`,
`RT64_MATCH_UNMATCHED=2567`, and `RT64_MATCH_NO_TOKEN=415799`.

These values do **not** prove that RT64 failed to match a valid previous
transform. `secondary_sway_rt64_match_flush_event()` writes unmatched rows
with hard-coded zero/`0xFFFFFFFF` previous fields when no pending token was
attached to the match callback. `secondary_sway_rt64_match_event()` only
writes a matched row after its token predicate succeeds; it does not record
the incoming callback arguments when the predicate fails. Thus the exact
failed predicate remains unobserved.

RT64's static path confirms that `matchTransform()` is the semantic seam: it
calls `tooie_diag_rt64_match()` only after a current/previous transform pair
has been selected. AUTO groups are excluded from the linear transform-ID map
and can be selected later through call-hash transform checks. The current
witness does not distinguish “no `matchTransform` callback for this snout”
from workload/transform mismatch or callback ordering.

```text
MATCHED_COUNT: 0 observed by this witness
UNMATCHED_COUNT: 2567 flush rows
CROSS_MATCH_COUNT: UNKNOWN
DROPPED_PREVIOUS_MATCH_COUNT: UNKNOWN
ONE_TO_ONE_CONFLICT_COUNT: UNKNOWN
MIRRORED_DETERMINANT_REJECT_COUNT: UNKNOWN
SEMANTIC_MATCH_CORRECT_COUNT: UNKNOWN
SEMANTIC_MATCH_FAILURE_COUNT: UNKNOWN
SEMANTIC_MATCH_CROSS_TO_OTHER_TRANSFORM_COUNT: UNKNOWN
SEMANTIC_MATCH_DROPPED_COUNT: UNKNOWN
SAME_SNOUT_MATCHES_CORRECT_PREVIOUS_SNOUT: UNKNOWN
PREVIOUS_FRAME_MATCH_CONTINUITY: UNKNOWN
FIRST_MATCH_FAILURE_FRAME: UNKNOWN (frame 1034 is only a flush row)
```

## Arbitration

```text
FIRST_BAD_LAYER: RT64 inter-frame match witness seam (previous association unobserved)
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
CAUSAL_CHAIN: DCF48 -> ADE2C -> 19AA0/D_8007BCB0 -> guMtxF2L -> G_MTX -> RT64 matrix is proven; the real guest matrix is valid; the match CSV lacks the actual callback arguments and therefore cannot prove a temporal RT64 failure.
BANJO_GROUP_APPLICABLE_TO_FAILED_CASE: UNKNOWN
GROUP_WOULD_STABILIZE_FAILED_AUTO_KEY: UNKNOWN
MISSING_GROUP_CAUSAL_STATUS: HYPOTHESIS
PRODUCTION_CANDIDATE_JUSTIFIED: NO
```

The exact next seam is a bounded diagnostic at `tooie_diag_rt64_match()`
entry that records, for the snout token scope, the incoming current/previous
workload and transform indices plus whether the token failed due to absent
callback, workload mismatch, transform mismatch, or ordering. A repeated
ordinary capture is not required until that witness exists.

```text
NEXT_TARGET: RT64_MATCH_CALLBACK_ARGUMENT_WITNESS
NEXT_SINGLE_ACTION: capture actual matchTransform callback arguments and token-predicate rejection reason before flush
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
```

No production fix was made. No commit, push, GPU readback, or GPU
synchronization was used.
