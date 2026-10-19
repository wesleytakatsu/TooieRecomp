---
title: "P09 DE2A4 census — live compact type-2 DCF48 runtime capture"
description: "Fresh CPU-only capture proves DCF48 for live 0x62B compact type-2 nodes but does not reach the exact snout node."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-03
tags: [tooie-recomp, p09, geocmd, dcf48, de2a4, snout]
---

# P09 DE2A4 census — live compact type-2 DCF48 runtime capture

CPU-only evidence; no GPU readback, production fix, commit, or push.

## Freshness and artifact

Capture files under `/tmp/tooie-secondary-sway-*` have mtime
`2026-09-03T02:02:48Z`. The current `build/TooieRecompiled` has mtime
`2026-09-03T00:51:52Z`; therefore:

```text
CAPTURE_POSTDATES_CURRENT_EXECUTABLE: YES
CAPTURE_USABLE: YES (valid handler census; no snout matrix sample)
```

The status identifies the current filter:

```text
FILTER=DCF48_compact_type2_0x62B_bin_plus_0x11CB8_idx11_bone42
E2AA4_PRIMARY_SNOUT_WITNESS=DEMOTED
GENERIC_E2180_DISCOVERY_FILTER=DEMOTED
```

`de2a4-tuples.csv` exists with 48 data rows. No snout CSV exists because no
snout sample was accepted.

## Runtime result

```text
DE2A4_RAW_ENTRY_COUNT=553908
DE2A4_IN_TRUE_0X62B_COUNT=107677
DE2A4_TYPE2_COUNT=65426
DE2A4_LAST_TYPE2_HANDLER=0x800DCF48
DCF48_RAW_ENTRY_COUNT=122445
DCF48_IN_TRUE_0X62B_COUNT=87393
DCF48_SNOUT_ACCEPT_COUNT=0
ADE2C_SNOUT_SAMPLES=0
```

The bounded tuples contain 30 `type=0x2` rows, all with
`HANDLER=0x800DCF48`; all 48 bounded rows are inside a registered `0x62B`
asset range. No bounded row has `NODE_REL=0x11CB8`, and no row reaches the
snout predicate. This proves the runtime compact type-2 dispatch, not the
exact snout node's execution.

For the last registered live model bin `0x8021EF60`, the static expected
snout pointer is `0x80230C18` (`+0x11CB8`), with compact children beginning
at `0x80230C28` (`+0x11CC8`). Neither the exact expected pointer nor a
snout-specific DCF48 accept was captured.

## Interpretation

```text
LIVE_0X62B_TYPE2_HANDLER: func_800DCF48
LIVE_TYPE2_HANDLER_CONFIDENCE: HIGH (PROVEN_RUNTIME)
LIVE_SNOUT_HANDLER_REACHABILITY: UNKNOWN
SNOUT_MATRIX_SAMPLE_COUNT: 0
SNOUT_USES_E2180: NO (E2AA4 raw entry count is 0)
SNOUT_USES_DUAL_MTX: UNKNOWN (snout path not reached)
FIRST_BAD_LAYER: exact snout-node runtime reachability
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
```

`func_800DCF48` statically calls `func_800ADE2C` for the selected bone-matrix
slot and recurses with `node + lbu(node+8)`. That establishes the compact
handler's semantics, but with zero snout accepts there is no evidence here
for a fully composed snout matrix, matrix identity, interpolation tagging,
or a first bad render layer. Do not request another identical capture; the
next work is structural reverse engineering of the live DE2A4/DCF48 node
population and handler mode.

## Reproducibility

```text
TOOIE_HEAD: ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf (worktree dirty)
CURRENT_EXECUTABLE_MTIME: 2026-09-03T00:51:52Z
CAPTURE_MTIME: 2026-09-03T02:02:48Z
GPU_READBACK_USED: NO
NO_COMMIT: YES
NO_PUSH: YES
```
