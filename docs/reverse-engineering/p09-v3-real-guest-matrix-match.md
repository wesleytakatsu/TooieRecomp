---
title: "P09 V3 guest matrix and RT64 match seam"
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-09-03
tags: [p09, rt64, transform-interpolation, guest-matrix]
---

# P09 V3 guest matrix and RT64 match seam

This page is a distilled canonical record of the V3 guest-matrix proof, not a
raw runtime capture. Its temporal-match wording is superseded by the V4
witness in [[systems/transform-interpolation]] and
[[../plan/evidence/p09-v4-real-match-witness]].

## PROVEN

For the live Tooie snout witness (`resource 0x62B`, node offset `0x11CB8`,
bone index 11, `boneAnimID=42`), the V3 runtime capture proves:

```text
DCF48/ADE2C -> func_80019AA0 -> MEM_W(D_8007BCB0)
             -> guMtxF2L source
             -> entry-a0 GFX Mtx destination
             -> G_MTX operand
             -> RT64 matrix address
```

The composed float source matches in all 512 bounded matrix rows, the F2L
destination equals the G_MTX operand in all 512 rows, and the full 16-float
matrices are finite with positive determinant. The earlier V1/V2 `NaN` result
was a witness bug and is superseded, not guest animation evidence.

## CORRECTED / SUPERSEDED

`D_8007BCD0` must not be used as the V3 float-stack cursor. V3 uses
`D_8007BCB0`; the V2 lookup-key status is superseded by the direct
G_MTX-operand key:

```text
RT64_TOKEN_LOOKUP_KEY: G_MTX_matrix_address_equals_gfx_mtx_operand
ADDRESS_NORMALIZATION_REQUIRED_FOR_TOKEN_MATCH: NO
```

## UNKNOWN

The V3 `rt64-snout-match.csv` does not contain actual failed
`matchTransform()` arguments. Its `UNMATCHED/NO_MATCHTRANSFORM` rows are
written by the diagnostic flush path with sentinel previous values. Therefore
the capture does not establish an RT64 AUTO identity bug, an interpolation
bug, a missing-group cause, or a semantic previous-frame mismatch.

The next reusable seam is a narrow callback-entry witness that records actual
current/previous workload and transform arguments and the token predicate's
rejection reason before flush.

Provenance: [P09 V3 evidence](../plan/evidence/p09-v3-real-guest-matrix-match.md).
