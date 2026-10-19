---
title: "P09 19AA0 → G_MTX dataflow and RT64 match stage"
description: "Generated-code dataflow for func_80019AA0/mlMtxApply; V2 D_8007BCD0 REFUTED; V3 witness."
status: current
type: evidence
project: TooieRecomp
updated: 2026-09-03
tags: [p09, snout, gmtx, rt64, interpolation]
---

# P09 19AA0 → G_MTX dataflow and RT64 match stage

No user capture this round. Generated C only (`RecompiledFuncs` read, not
edited). GPU unused.

## Proven dataflow

```text
DCF48 a0 = Gfx context (s0)
DCF48 a1 = GeoCmd node
ADE2C(D_8012C968, signed byte node+9)
19AA0(0x8012CE58, ADE2C v0)
  cursor = D_8007BCB0   // lui 0x8008; addiu -0x4350
  *cursor += 0x40
  write affine parent*local at *cursor
mlMtxApply(lw(s0+4))
  a1 = gfx Mtx dest
  a0 = MEM_W(D_8007BCB0)
  guMtxF2L(float src, Mtx dest) @ 0x8002ED00
G_MTX DA380002 operand = lw(s0+4)
s0+4 += 0x40
DE2A4 children; mlMtxPop; G_POPMTX
```

`OLD_CLAIM_19AA0_FINAL_OUTPUT_AT_D8007BCD0`: **REFUTED**.
`D_8007BCD0` is neighboring BSS. V2 `COMPOSED_PTR=0x80159600` NaNs are
not game evidence.

G_MTX is `PUSH|LOAD|MODELVIEW`. RSP replaces the pushed slot; bone
hierarchy is CPU-side (`ADE2C`/`AF04C` plus `19AA0` × `0x8012CE58`).

## Matrix rings

- Float stack: `D_8007BCB0` over arena `0x8007B4F0`; push `19AA0`, pop
  `mlMtxPop`. Per hierarchical draw.
- Gfx Mtx ring: `gfx_ctx+4`, +0x40 per G_MTX. Explains 12–16 rotating
  G_MTX operands. Expected.

## Six transform ordinals

V2 RT64 rows used world-transform indices `0x31,0x32,0x33` and
`0x3A,0x3B,0x3C`. These are **slots in one workload's**
`worldTransforms` vector (stable across frames), two clusters of three.
Exact pass names (opaque/xlu/shadow/reflection) remain `UNKNOWN` until
the V3 `PROJECTION_TYPE`/`OTHERMODE_H`/`DRAW_UID` columns are captured.
Do not compare identity across clusters.

## RT64 match

Live AUTO path: `GameFrame::match` → `matchScene` → call-hash map →
same-index world matrices → `computeTransformMatch` → one-to-one
`matchTransform`. Explicit IDs skip AUTO in `buildTransformIdMap`.

V2 CSV had only MATRIX/NEW + VERTEX because:

1. identity CSV cap 1536 filled before `GameFrame::match`;
2. token ring was 64 slots;
3. MATCH required a still-pending token.

`tooie_diag_rt64_match` was already on the correct function; it did not
observe unmatched either.

V3 writes `/tmp/tooie-secondary-sway-rt64-snout-match.csv` from
`matchTransform` and `tooie_diag_rt64_match_flush` at end of
`GameFrame::match`. Guest float capture uses `D_8007BCB0`. Schema
`P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V3`.

Missing group remains `HYPOTHESIS`. Kazooie feathers reuse the same RT64
match CSV by changing only the guest G_MTX token filter.

```text
PRODUCTION_FIX: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```
