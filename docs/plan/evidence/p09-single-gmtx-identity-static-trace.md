---
title: "P09 live 0x62B single-G_MTX identity trace"
description: "Static trace below the proven snout GeoCmd path and RT64 matching review."
status: current
type: evidence
project: TooieRecomp
updated: 2026-09-03
tags: [p09, snout, gmtx, rt64, interpolation]
---

# P09 live 0x62B single-G_MTX identity trace

## Result

The live snout path is proven through the ordinary single-G_MTX route:

```text
0x62B+0x11CB8
  -> func_800DCF48
  -> func_800ADE2C
  -> func_80019AA0
  -> mlMtxApply
  -> guMtxF2L
  -> G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW
```

The runtime witness accepted the exact node 2,243 times and recorded 2,243
ADE2C current slots.  The live node was stable at model-relative `0x11CB8`,
type 2, bone index 11, boneAnimID 42.  The old E2180 dual-matrix hypothesis
is not the live snout path (`CORRECTED/SUPERSEDED`).

## Guest composition

`func_800ADE2C` returns `D_8012C968 + 0x50 + (11 << 6)`, a local bone-matrix
slot.  It selects a slot and does not receive a composed matrix argument.
`func_80019AA0` reads that local matrix and the current guest matrix at its
other input, multiplies them, allocates/writes the float result at the next
0x40-byte slot from the `D_8007BCB0` cursor, and advances that cursor.
`mlMtxApply` passes that float source to `guMtxF2L`, using the `s0+4` matrix
buffer as destination; DCF48 then emits `0xDA380002` with that destination
address and advances `s0+4` by 0x40.

Therefore the 17 ADE2C current addresses in the prior capture are rotating
animation-matrix source-table addresses (`PROVEN_STATIC`), not 17 logical
bones and not direct G_MTX addresses.  The exact source-buffer owner and
lifecycle, plus the float arena base/end/reset cadence and total slot counts,
are not established by the witness (`UNKNOWN`; at least 17 source addresses
were observed).

## RT64 identity

RT64's plain F3DEX2 matrix decoder calls `RSP::matrixCommon`, which applies
LOAD/MUL and PUSH to the CPU model-view stack and records segmented/physical
addresses.  At vertex processing it snapshots the effective world transform
and associates it with the current `TransformGroup`.  A plain path has the
default `matrixId = G_EX_ID_AUTO`.

RT64 frame matching hashes the draw-call state and the matrix-ID sequence.
AUTO groups opt into transform matching; matching pairs transform ranges by
their position within the matched call and updates previous/current rigid-body
state.  The static path therefore does not require a stable guest matrix
address.  A rotating temporary address alone cannot prove identity churn.

`gEXMatrixGroup` supplies an explicit stable matrix ID and interpolation
properties through an extended group stack.  It affects subsequent ordinary
single-G_MTX transforms, but the current evidence does not prove that a group
is required for this symptom.

## Banjo comparison and status

Banjo `func_803387F8` is the exact single-G_MTX structural analog: one local
bone matrix, push/load/modelview, child traversal, and pop.  Banjo additionally
places `set_model_matrix_group` around that submission, deriving a group ID
from the model transform ID plus the bone/list offset.  Tooie's observed
`func_800DCF48` path has no corresponding explicit group command.

```text
FIRST_BAD_LAYER: NONE_OBSERVED
ROOT_CLASS: SINGLE_G_MTX_IDENTITY_UNRESOLVED
ROOT_CONFIDENCE: LOW
ROTATING_SLOT_CAN_CAUSE_RT64_IDENTITY_CHURN: NO (by itself)
MISSING_GROUP_CAN_CAUSE_THIS_SPECIFIC_SNOUT_FAILURE: UNKNOWN
SINGLE_G_MTX_IDENTITY_ROOT_PLAUSIBILITY: MEDIUM
```

The runtime witness still lacks full 16-word matrix data, G_MTX command
address/flags, stack depth, and RT64's actual workload transform identity.
No production fix is authorized.

## Next discriminator

`NEXT_WITNESS_SEAM`: CPU-side `mlMtxApply/guMtxF2L` to RT64 `RSP::matrixCommon`
and the resulting workload `worldTransforms/worldTransformGroups`, with one
bounded sample of the guest source matrix, composed matrix, G_MTX flags,
stack depth, physical address, group ID, and previous-frame match result.

This separates a bad guest/local matrix from a bad composed transform and an
RT64 identity mismatch without GPU readback.  No user capture is requested in
this round.

No commit, push, build, or GPU readback was performed.
