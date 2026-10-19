---
title: "P09 Kazooie carry-Banjo head-feather V1 capture"
description: "Runtime identity and guest-matrix analysis for the user-observed Kazooie feather glitch."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-04
tags: [tooie-recomp, p09, kazooie, feathers, guest-matrix, runtime]
---

# P09 Kazooie carry-Banjo head-feather V1 capture

## Validity

The current executable was `build/TooieRecompiled`, mtime
`2026-09-03T05:10:43-0300`. The new feather artifacts were written at
`2026-09-03T12:03:11` through `12:04:36-0300`, so the capture postdates the
executable. The status file reports the exact expected schema:
`P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V1`, with witness
`KAZOOIE_CARRY_BANJO_HEAD_FEATHER`.

The capture is usable for live identity and guest-matrix analysis. It is not
usable for the requested real RT64 match analysis because no current
Kazooie match CSV was produced and the status reports `RT64_MATCH_ROWS=0`.

## Live identity — PROVEN_FOR_THIS_CAPTURE

The accepted primary filter selected the combined `0x62B` model:

```text
KAZOOIE_FEATHER_PRIMARY_HITS=1977
KAZOOIE_FEATHER_62B_PRIMARY_HITS=1977
KAZOOIE_FEATHER_61C_PRIMARY_HITS=0
KAZOOIE_FEATHER_83A_PRIMARY_HITS=0
KAZOOIE_FEATHER_SIBLING_HITS=11862
MODEL_BIN=0x80228690
NODE=0x8023AB30
NODE_REL=0x124A0
BONE_INDEX=30
BONE_ANIM_ID=103
```

The model capture contained 2,149 rows for resource `0x62B` and 139 for
`0x62A`; no `0x61C` or `0x83A` resource was observed. The accepted feather
identity is therefore `0x62B + 0x124A0`, bone index 30, boneAnimID 103, with
high confidence for this runtime state. The sibling count is not a second
primary identity.

## Guest matrix — CORRECTED (uniform scale, not degenerate)

The bounded matrix CSV contains 512 primary-feather samples, frames 1277–1898.
Every row has `SOURCE_MATCHES_COMPOSED=YES` and the captured G_MTX pointer
equals the `guMtxF2L` destination. There are no NaN or Inf values.

The parent matrix remains well-conditioned (determinant approximately 1,
orthogonality ~0). The local and composed 3x3s at 1420–1424 are uniformly
scaled orthonormal rotations (SVD condition ~1, shear none, right-handed).
The determinant sequence is smooth/monotonic:

```text
frame 1420: scale ~0.417  det 0.072510
frame 1421: scale ~0.571  det 0.186164
frame 1422: scale ~0.741  det 0.407684
frame 1423: scale ~0.896  det 0.718102
frame 1424: scale ~1      det ~1
```

```text
LOW_DETERMINANT_MECHANISM: UNIFORM_SCALE
IS_MATRIX_ACTUALLY_NEAR_SINGULAR: NO
IS_1420_TO_1424_CHANGE_SMOOTH: YES
OLD_DEGENERATE_CLASSIFICATION: TOO_STRONG
FIRST_SUSPICIOUS_LAYER: LOCAL_BONE_MATRIX_PRODUCER
FIRST_BAD_GUEST_STAGE: UNRESOLVED
```

Whether that scale is clip-encoded, parent-accumulated, additive, or
corrupt is not answered by V1. See
[[p09-kazooie-feather-v2-producer-witness]].

The only large capture gap is 1309→1420 (111 guest frames). That gap is
sample-cap/filter: the model CSV continues through 1310–1419.

## RT64 status — NOT_OBSERVED

`tooie-kazooie-feather-rt64-identity.csv` contains 1,536 `MATRIX` rows, all
for the same live identity, and no `VERTEX`, `CALL`, `TRANSFORM_PREREQUISITE`,
`CANDIDATE`, `DECISION`, or `MATCHED` rows. Matrix identity consumption is
present (`TOKEN_ENQUEUED=1977`, `SNOUT_TRACE_TOKEN_CONSUMED_RT64=1977`), but
the V5 real-match binding is absent in this run:

```text
TOKEN_BOUND_TO_GAMEFRAME_MATCH=0
TOKEN_BOUND_TO_CALL_STAGE=0
TOKEN_BOUND_TO_TRANSFORM_PREREQUISITE=0
TOKEN_BOUND_TO_CANDIDATE_STAGE=0
TOKEN_BOUND_TO_MATCHTRANSFORM=0
REAL_MATCHED=0
REAL_UNMATCHED=0
RT64_MATCH_ROWS=0
```

The global `RT64_MATCH_NO_TOKEN` count is not treated as a feather rejection;
there are no feather-bound match rows with which to classify it.

## Classification

```text
FIRST_BAD_GUEST_STAGE=LOCAL_BONE_MATRIX
FINAL_GUEST_FEATHER_TRANSFORM_VALID=NO (three degenerate samples)
ROOT_CLASS=MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE=MEDIUM
MISSING_GROUP_CAUSAL_STATUS=UNKNOWN
PRODUCTION_CANDIDATE_JUSTIFIED=NO
NEXT_SINGLE_ACTION=trace the producer of the 0x62B/+0x124A0 local feather matrix around the 1420–1422 anomaly
NEXT_RECOMMENDED_MODEL=Grok 4.6 Medium
GPU_READBACK_USED=NO
```

No production fix is justified. The next investigation should preserve this
capture and trace the local-bone/secondary-animation producer; it should not
repeat the same capture without changing the diagnostic seam.
