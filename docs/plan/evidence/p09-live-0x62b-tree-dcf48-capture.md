---
title: "P09 — live 0x62B tree, snout DCF48 and ADE2C capture"
description: "Sibling-aware runtime proof of the live 0x62B snout path through DCF48 and ADE2C."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-03
tags: [p09, banjo, geocmd, dcf48, ade2c, snout, matrix]
---

# P09 — live 0x62B tree, snout DCF48 and ADE2C capture

## Provenance

The capture was inspected directly from `/tmp/tooie-secondary-sway-*` before
any diagnostic edit. It postdates the executable used for the run:

```text
CURRENT_EXECUTABLE_MTIME: 2026-09-02T23:43:17.209809503-0300
TREE_CAPTURE_MTIME: 2026-09-02T23:51:55.079771518-0300
STATUS_MODEL_SNOUT_CAPTURE_MTIME: 2026-09-02T23:53:09.623606695-0300
CAPTURE_POSTDATES_CURRENT_EXECUTABLE: YES
CAPTURE_USABLE: YES (live-tree, DCF48 and current-slot witness)
GPU_READBACK_USED: NO
```

The status identifies the corrected witness:

```text
FILTER: DCF48_compact_type2_0x62B_bin_plus_0x11CB8_idx11_bone42
TREE_WITNESS: DE498_0x62B_token_plus_DE2A4_sibling_scan_plus_DCF48
E2AA4_PRIMARY_SNOUT_WITNESS: DEMOTED
```

## Phase 1 — tree witness

```text
REGISTERED_0X62B_BIN_COUNT: 1 (derived from model.csv)
LIVE_0X62B_MODEL_BIN: 0x80226940
LIVE_0X62B_ROOT: 0x80238330
EXPECTED_ROOT_REL: 0x119F0
TREE_WITNESS_ROWS: 800
TREE_WITNESS_UNIQUE_NODE_COUNT: 89
DE2A4_LIST_CONTAINS_0X11CB8: 7 (sibling-list membership observations)
LAST_GEOCMD_MODE: 0x0
LAST_HANDLER_TABLE: 0x8012306C
```

The old direct `DE2A4` census only saw list heads. In the new witness, the
snout appears as a sibling in the list headed by `+0x11C50`, which is the
child list of neck `+0x11C40`:

```text
SNOUT_PARENT_REL: 0x11C40 (direct GeoCmd parent)
SNOUT_LIST_HEAD_REL: 0x11C50
SNOUT_RELATION: SIBLING
```

The tree CSV records the following rows in the capture (the same rows repeat
across the bounded witness frames):

```text
REL 0x119F0: type 0x2, handler 0x800DCF48, index 0, anim 43, list head
REL 0x11CB8: type 0x2, handler 0x800DCF48, index 11, anim 42, sibling
REL 0x11CC8: type 0x18, handler 0x800DD998, child-list head
REL 0x11CF8: type 0x11, handler 0x800DD65C, sibling in 0x11CC8 child list
```

```text
SNOUT_TREE_ROW_FOUND: YES
SNOUT_RUNTIME_POINTER: 0x802385F8
SNOUT_REL_OFFSET: 0x11CB8
SNOUT_TYPE: 0x2
SNOUT_BONE_INDEX: 11
SNOUT_BONE_ANIM_ID: 42
SNOUT_HANDLER: 0x800DCF48
LIVE_SNOUT_TRANSFORM_NODE_PROVEN: YES (PROVEN_RUNTIME)
REL_0X11CC8_SEEN: YES (7 bounded rows)
REL_0X11CF8_SEEN: YES (7 bounded rows)
SNOUT_TO_SETUP_RELATION_PROVEN_RUNTIME: YES
SNOUT_TO_VISIBLE_DL_RELATION_PROVEN_RUNTIME: YES
VISIBLE_SNOUT_USES_BONE42_TRANSFORM: YES (DCF48 push, child walk, pop)
```

`+0x11CB8` has `byte(+8)=0x10`, so `func_800DCF48` calls
`DE2A4(node+0x10)`, reaching `+0x11CC8`. Its child-list continuation reaches
`+0x11CF8`. The handler emits one matrix push before this child walk and one
pop after it. This proves the CPU hierarchy relationship, not a final RT64
interpolation identity.

## Phase 4 — DCF48 execution

```text
DCF48_RAW_ENTRY_COUNT: 103104
DCF48_REL_0X11CB8_COUNT: 2243
DCF48_SNOUT_ACCEPT_COUNT: 2243
DCF48_SNOUT_NODE_POINTER: 0x802385F8
DCF48_SNOUT_INDEX: 11
DCF48_SNOUT_BONE_ANIM_ID: 42
X62B_SNOUT_ACTUAL_HANDLER: func_800DCF48
X62B_SNOUT_HANDLER_CONFIDENCE: HIGH
```

This corrects the previous `E2AA4`/direct-`DE2A4` reachability uncertainty.
`E2AA4_RAW_ENTRY_COUNT=0`, so the old dual-matrix witness is not the live
snout path in this capture.

## Phase 5 — ADE2C and single G_MTX

```text
ADE2C_SNOUT_HITS: 2243
ADE2C_SNOUT_MATRIX_POINTER: 17 rotating current-slot pointers
CURRENT_SLOT_POINTER_RANGE: 0x801D1520..0x801F0E10
EXPECTED_SNOUT_MATRIX_SLOT: D_8012C968 + 0x50 + (11 << 6) = D_8012C968 + 0x310
ADE2C_SNOUT_MATRIX_SLOT_MATCH: YES (runtime r2 is the ADE2C return slot)
NUMBER_OF_G_MTX_FOR_SNOUT_SCOPE: 1 per DCF48 snout invocation (static path)
BANJO_SNOUT_MATRIX_CLASS: SINGLE_G_MTX
ADE2C_RECEIVES_FINAL_COMPOSED_SNOUT_MATRIX: NO (it selects/returns a slot; no matrix argument)
SNOUT_USES_E2180: NO
SNOUT_USES_DUAL_MTX: NO
SNOUT_USES_DCF48: YES
SNOUT_USES_ADE2C: YES
OLD_E2180_TAGGING_ROOT_FOR_SNOUT: REFUTED / NOT_APPLICABLE
```

`func_800DCF48` calls `ADE2C(anim_mtx_list, index)`, passes the returned
pointer through the single `mlMtxApply` path (`func_80018C28`), then recurses
the child list. The CSV identity fields are `SINGLE_G_MTX` and
`ADE2C_BONE_INDEX`; these are witness labels, not proof that RT64 received a
production transform tag.

## Phase 6 — matrix quality

```text
SNOUT_MATRIX_SAMPLE_COUNT: 2243
CURRENT_MATRIX_SOURCE_RUNTIME: ADE2C return in ctx->r2, current guest slot
PREVIOUS_MATRIX_SOURCE_RUNTIME: not captured; PREV_MTX_PTR=0 and field=NONE
SNOUT_TRANSLATION_CONTINUITY: UNKNOWN
SNOUT_ROTATION_CONTINUITY: UNKNOWN
SNOUT_SCALE_CONTINUITY: UNKNOWN
SNOUT_MATRIX_TRUE_ANOMALY_COUNT: 0 proven in the captured prefix; full matrix unclassified
STRONGEST_MATRIX_ANOMALY: NONE_PROVEN
SNOUT_GUEST_MATRIX_VALID: UNKNOWN
```

The node pointer and model bin are stable in all 2,243 snout rows. The
diagnostic stores 8 of the 16 32-bit words of the N64 64-byte matrix, so the
captured current prefix is present and changes over time, but it is not enough
to validate the complete fixed-point matrix, translation, scale, or a true
semantic anomaly. No previous matrix is recorded; stale-previous and
previous/current pairing are therefore `UNKNOWN`, not zero.

## Phase 7–8 — causal classification

```text
SNOUT_USES_E2180: NO
SNOUT_USES_DUAL_MTX: NO
SNOUT_USES_DCF48: YES
SNOUT_USES_ADE2C: YES
FIRST_BAD_LAYER: SINGLE_G_MTX_INTERPOLATION_IDENTITY (next unobserved seam; no fault proven)
ROOT_CLASS: SINGLE_G_MTX_IDENTITY_UNRESOLVED
ROOT_CONFIDENCE: LOW
CAUSAL_CHAIN: 0x62B → DE498 root → DE2A4 list-head/sibling traversal → DCF48 snout accept → ADE2C current slot → one mlMtxApply → RT64 identity still unobserved
```

The capture refutes the direct old E2180 dual-matrix root for the live `0x62B`
snout. It does not establish that interpolation is correct: the current
witness has no previous matrix and no renderer identity/tag observation.
There is no high-confidence narrow production fix.

## Next step

```text
NEXT_SINGLE_ACTION: trace DCF48's func_80018C28/mlMtxApply into the RT64 single-G_MTX identity seam
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
USER_MANUAL_TEST_REQUIRED_NOW: NO
BUILD_RESULT: SKIP
WINDOW_DISPLAY_TEST: NOT_RUN
PROJECTION_TEST: NOT_RUN
DIFF_CHECK: PASS (after documentation update)
NO_COMMIT: YES
NO_PUSH: YES
```
