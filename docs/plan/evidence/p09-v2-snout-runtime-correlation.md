---
title: "P09 V2 Snout Runtime Correlation"
description: "Arbitragem de captura runtime V2 do focinho (snout) e correlação de matriz única DCF48."
status: proven
type: evidence
project: TooieRecomp
phase: P09
date: 2026-09-03
tags: [p09, snout, dcf48, secondary-sway, matrix, rt64, correlation]
---

# P09 V2 snout runtime correlation

## 2026-09-03 — V2 capture arbitration

Capture files under `/tmp/tooie-secondary-sway-*` finished at
`2026-09-03 02:07:59 -0300`, after the current executable timestamp
`2026-09-03 01:48:48 -0300`. The capture is usable and reports
`P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V2` with
`PRIMARY_WITNESS=SNOUT_DCF48_SINGLE_G_MTX`.

### Proven runtime facts

- The authoritative DCF48 witness accepted 3,064 snout records for resource
  `0x62B`, node `0x11CB8`, GeoCmd type 2, index 11, boneAnimID 42.
- 3,064 tokens were enqueued; 3,063 were consumed by RT64. The resulting
  primary correlation rate is `3063/3064 = 99.967%`.
- `TOKEN_SOURCE_EQ_GMTX_OPERAND=3063` and
  `TOKEN_DEST_EQ_GMTX_OPERAND=0`. This confirms the V2 lookup key is the raw
  `G_MTX` operand matching the `mlMtxApply` source pointer. It does not confirm
  that the F2L destination is the G_MTX operand.
- The persisted RT64 CSV is capped at 1,536 rows: 768 `MATRIX` rows and 768
  `VERTEX` rows. It contains no `MATCH` rows, so previous-frame semantic
  continuity was not observed. This is not evidence of an RT64 mismatch.
- All persisted matrix events use `MATRIX_ID=0xFFFFFFFF` (AUTO),
  `GROUP_INDEX=0`, and `EXPLICIT_GROUP=0`. No explicit Tooie snout group was
  observed.

### Corrected witness interpretation

The V2 matrix CSV has 512 rows. In all rows:

```text
COMPOSED_PTR=0x80159600
F2L_DEST=0x80159600
F2L_SOURCE != COMPOSED_PTR
GFX_MATRIX_PTR=F2L_SOURCE
GFX_MATRIX_PTR != F2L_DEST
```

`F2L_SOURCE == COMPOSED_PTR` is `0/512`; the source cycles through eight
addresses. The captured `COMPOSED_FLOAT16` at `COMPOSED_PTR` contains a final
`NaN` in `512/512` rows and is therefore not a valid completed-float witness
for this runtime path. The raw current-matrix records are present (3,064
rows), but the previous-matrix field is `NONE` in all 3,064 rows. Consequently
the capture cannot establish final guest transform validity or previous-frame
matrix pairing.

This corrects the earlier interpretation that post-call `D_8007BCD0` was
already proven to be the float matrix consumed by `guMtxF2L`. The V2 RT64 key
remains correct; the composition witness is unresolved and its NaN must not
be promoted to a guest-animation anomaly. V1 NaN/Inf evidence remains
invalidated by the witness-pointer problem.

```text
GUMTXF2L_SOURCE_EQUALS_COMPOSED_OUTPUT: NO (0/512)
G_MTX_OPERAND_EQUALS_TOKEN_SOURCE: YES (3063/3063 consumed)
RT64_TOKEN_ADDRESS_MATCH_VALID: YES
MATRIX_ID_MODE: AUTO
SAME_SNOUT_GETS_STABLE_LOGICAL_IDENTITY: UNKNOWN
PREVIOUS_FRAME_MATCH_CONTINUITY: UNKNOWN
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
PRODUCTION_FIX: NOT_JUSTIFIED
```

No production fix, commit, push, GPU readback, or GPU synchronization was
performed.
