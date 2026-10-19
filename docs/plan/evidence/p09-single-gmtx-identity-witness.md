---
title: "P09 bounded single-G_MTX identity witness"
description: "Implementation and static validation of the CPU-only snout transform identity witness."
status: current
type: evidence
project: TooieRecomp
updated: 2026-09-03
tags: [p09, snout, gmtx, rt64, interpolation, diagnostic]
---

# P09 bounded single-G_MTX identity witness

## Scope

This checkpoint implements the CPU-only witness for the proven live snout:

```text
0x62B + 0x11CB8 (type 2, index 11, boneAnimID 42)
 -> func_800DCF48 -> func_800ADE2C -> func_80019AA0
 -> mlMtxApply / guMtxF2L -> F3DEX2 G_MTX PUSH|LOAD|MODELVIEW
 -> RT64 RSP::matrixCommon -> setVertexCommon -> GameFrame::matchTransform
```

No production behavior, transform group, interpolation policy, or GPU path
was changed.

## Implemented witness

- `func_80019AA0` captures the local ADE2C matrix, current/pair input, and the
  complete sixteen-word composed float matrix from the result slot selected by
  the post-call `D_8007BCD0` cursor (`old_cursor + 0x40`). The first runtime capture
  corrected the static address assumption: the guest cursor is
  `D_8007BCD0` (`0x8007BCD0`), not `0x8007BCB0`.
- `mlMtxApply` captures the complete sixteen-word fixed-point destination
  written by `guMtxF2L` and creates a bounded token only for the active DCF48
  snout context.
- RT64 F3DEX2 matrix dispatch carries the exact guest display-list command
  address into `RSP::matrixCommon`.
- RT64 records matrix flags, model-stack depth before/after, explicit/default
  transform-group identity, workload ID, segmented/physical matrix address,
  transform creation, and the existing frame-match pair.
- Output is default-off and bounded: guest matrices are written to
  `/tmp/tooie-secondary-sway-snout-matrix.csv`; RT64 identity events are
  written to `/tmp/tooie-secondary-sway-rt64-snout-identity.csv`, capped at
  1,536 identity events (512 guest matrix rows plus bounded RT64 events).

The token bridge is not a generic address-only filter: it is created only by
the exact proven DCF48 node and composition call, then consumed by RT64 for
the matching G_MTX source/guest matrix pointer. The actual F2L destination is
captured independently from `D_8007BCD0`.

## Static validation

```text
BUILD_COMMAND: cmake --build build --target TooieRecompiled -j2
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
GENERATED_RECOMPILED_CODE_EDITED: NO
```

Runtime proof is intentionally pending. Inspect schema version, complete guest
matrices, command/stack fields, and `MATCHED` rows before raising root
confidence or considering a production fix.

## V1 witness correction and V2 repair (CORRECTED, 2026-09-03)

The V1 runtime capture is invalid for transform or RT64 conclusions:

- `0x8007BCB0` was a wrong diagnostic address. The cursor used by
  `func_80019AA0` is `D_8007BCD0); after the function returns, the
  completed float output is at the post-call cursor (`old_cursor + 0x40`).
- `mlMtxApply` entry `a0` is the float source and is forwarded to
  `guMtxF2L` as `a1`. Entry `r5` is not the final destination. The
  destination is loaded by `mlMtxApply` from `D_8007BCD0`.
- The DCF48 G_MTX operand is the node `+4` pointer (the F2L source pointer);
  the V2 witness records that pointer separately from the actual F2L
  destination and matches RT64 on the G_MTX operand.
- V1 `NaN/Inf` observations were read through the false composed pointer and
  are `INVALIDATED_BY_WITNESS_POINTER_BUG`, not game evidence.

V2 adds the corrected cursor/destination flow, explicit token counters
(`CANDIDATES`, `ENQUEUED`, `CONSUMED_RT64`, `EXPIRED`, and rejection
classes), and schema marker
`P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V2`. No production behavior or RT64
matching policy was changed.

## First runtime arbitration (2026-09-03)

The capture postdated the current executable and used the new schema, but it
did not produce the RT64 identity CSV. The guest path was reached:

```text
DCF48_SNOUT_ACCEPT_COUNT: 4728
ADE2C_SNOUT_SAMPLES: 4728
SNOUT_MATRIX_ROWS: 512 (bounded cap)
RT64_IDENTITY_FILE: MISSING
```

The failure is mechanical and does not establish an RT64 identity failure.
`func_80019AA0` computes `a2 = MEM_W(D_8007BCD0) + 0x40` and advances
`D_8007BCD0`; the diagnostic read `0x8007BCB0`, yielding a false composed
pointer (`0x8007B630`) while `mlMtxApply` received source pointers such as
`0x8017C5B0`. Every bounded row had `SOURCE_MATCHES_COMPOSED=NO`.

There is a second related witness issue: `mlMtxApply` receives the source in
`a0`, copies it to `a1`, then loads the actual `guMtxF2L` destination from
`D_8007BCD0`. The diagnostic recorded the pre-call `ctx->r5` as the
destination, so its `F2L_DEST` field is not the actual conversion destination.
The token predicate therefore never queued an RT64 token. This is a
`CORRECTED` diagnostic finding; final guest-transform validity and RT64 AUTO
matching remain `UNKNOWN`.

```text
FIRST_BAD_LAYER: NONE_OBSERVED
ROOT_CLASS: SINGLE_G_MTX_IDENTITY_UNRESOLVED
ROOT_CONFIDENCE: LOW
SINGLE_G_MTX_IDENTITY_ROOT_PLAUSIBILITY: MEDIUM
MISSING_GROUP_CAUSAL_STATUS: HYPOTHESIS
```

No commit. No push.

## V2 implementation validation (2026-09-03)

The corrected diagnostic was compiled without executing the game:

```text
DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V2
OLD_CURSOR_ADDRESS: 0x8007BCB0
CORRECT_CURSOR_ADDRESS: 0x8007BCD0
ALL_WRONG_8007BCB0_DIAGNOSTIC_REFERENCES_RETIRED: YES
COMPOSED_MATRIX_CAPTURE_TIMING: AFTER_WRITE
COMPOSED_FLOAT_MATRIX_CAPTURE_CORRECTED: YES
TRACE_TOKEN_COUNTERS_IMPLEMENTED: YES
RT64_SINGLE_MTX_IDENTITY_WITNESS_READY: YES
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
USER_MANUAL_TEST_REQUIRED_NOW: YES
```

The next run must be a V2 capture. It is the first capture permitted to
evaluate guest composed-matrix finiteness and guest-to-RT64 identity
correlation; the V1 NaN/Inf observation remains invalidated.

## V2 pointer-key audit (PROVEN_STATIC, 2026-09-03)

The apparent contradiction was resolved by following the generated DCF48
instructions. `mlMtxApply` entry `a0` is forwarded to `guMtxF2L` as its
source, while `guMtxF2L` receives its destination from
`MEM_W(D_8007BCD0)`. After returning from `mlMtxF2L`, DCF48 writes
`MEM_W(node + 0x4)` into the G_MTX command operand. That value is the same
entry `a0` source pointer, not a statically proven alias of the F2L
destination.

RT64 `RSP::matrixCommon` receives the raw G_MTX operand as `matrix_address`
and separately derives `physical_address` with `fromSegmentedMasked`.
Consequently the V2 primary key,
`candidate.source_ptr == matrix_address`, is correct and requires no
normalization. The token now retains both source and destination pointers and
stores the GFX command pointer and raw RT64 matrix operand at consumption.

```text
CURRENT_V2_LOOKUP_KEY_CORRECT: YES
CORRECT_RT64_TOKEN_LOOKUP_KEY: raw G_MTX operand == mlMtxApply entry a0/source
TOKEN_POINTER_CHAIN_COUNTERS_IMPLEMENTED: YES
FINAL_DIAGNOSTIC_SCHEMA: P09_SECONDARY_SWAY_SINGLE_MTX_IDENTITY_V2
```
