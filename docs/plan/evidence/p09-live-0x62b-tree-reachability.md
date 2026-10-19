---
title: "P09 live 0x62B GeoCmd tree — missing snout node is a sibling, not a missing branch"
description: "DE2A4 wrap sees list heads only. 0x11CB8 is a later sibling in the neck child list. Child 0x11CC8 was live. Diagnostic INDEX byte read was host-endian."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-03
tags: [tooie-recomp, p09, geocmd, dcf48, de2a4, snout, tree]
---

# P09 live 0x62B GeoCmd tree — missing snout node is a sibling, not a missing branch

CPU/static plus the existing DE2A4 census. No GPU readback. No production
fix, commit, or push. No additional user capture during investigation.

Authoritative census: [[p09-de2a4-dcf48-runtime-capture]].

```text
DE2A4_RAW_ENTRY_COUNT: 553908
DE2A4_IN_TRUE_0X62B_COUNT: 107677
DE2A4_TYPE2_COUNT: 65426
DE2A4_LAST_TYPE2_HANDLER: 0x800DCF48
DE2A4_REL_0X11CB8_COUNT: 0
DCF48_SNOUT_ACCEPT_COUNT: 0
LAST_MODEL_BIN: 0x8021EF60
LAST_MAGIC: 0xB
LAST_GTYPE: 0
LAST_BONE_COUNT: 66
```

## Phase 1 — registered model_bin identity

`TOOIE_DIAG_SECONDARY_SWAY` stores the DE448 5th argument (`16(sp)` /
`model_bin`) in up to four durable slots. Registration also fingerprints
magic `0xB`, `gtype`, setup bone count.

The last captured bin `0x8021EF60` reports magic `0xB`, `gtype=0`, 66
bones. Live DE2A4 list-head `NODE_REL=0x119F0` equals static header `+4`.

```text
REGISTERED_0X62B_VALUE_SEMANTICS: guest pointer to decompressed model header
IS_REGISTERED_VALUE_RAW_MODEL_BIN_BASE: YES
RESOURCE_0X62B_HEADER_AT_REGISTERED_BASE: YES (fingerprint)
HEADER_MAGIC: 0xB
HEADER_GEOCMD_ROOT_OFFSET: 0x119F0 (live NODE_REL of first DE2A4 in capture)
HEADER_BONE_COUNT: 66
RUNTIME_0X62B_HEADER_MATCHES_STATIC_ASSET: YES (magic/gtype/bones/GeoCmd +4)
```

## Phase 2 — multiple instances / lifetime

```text
REGISTERED_BIN_TRACKING_POLICY: up to 4 durable bins; IN_TRUE/snout rel uses
  lookup across all slots; LAST_MODEL_BIN is reporting-only
MAX_CONCURRENT_0X62B_BINS: 4 slots
ALL_ACTIVE_BINS_CHECKED_FOR_SNOUT: YES (rel == 0x11CB8 against whichever
  durable bin contains the pointer)
COULD_SNOUT_HAVE_BELONGED_TO_ANOTHER_ACTIVE_0X62B_BIN: NO as explanation of
  DE2A4_REL_0X11CB8=0 (any bin would still count that relative list-head)
```

## Phase 3–4 — live root

Generated `func_800DE448` LOOKUP `func_800DE498` with 6th arg = model_bin.
`func_800DE498` first walker call:

```text
a0 = gfx packet (A7180 v0)
a1 = model_bin + *(u32*)(model_bin+4)
```

Capture: `NODE_PTR=0x80230950` = `0x8021EF60+0x119F0`.

```text
STATIC_GEOCMD_ROOT_OFFSET: 0x119F0
EXPECTED_RUNTIME_ROOT: B + 0x119F0
ACTUAL_RUNTIME_ROOT_FORMULA: model_bin + lw(model_bin+4)
ROOT_IS_RAW_MODEL_BIN_PLUS_0X119F0: YES
ROOT_IS_RELOCATED_OR_COPIED: NO (0x62B GeoCmd walked in-place)
DE448_0X62B_MODEL_ARG: 5th arg model_bin (live last 0x8021EF60)
DE498_MODEL_BIN_ARG: 6th arg (0x14(sp) at DE448; 0x10C after DE498 prologue)
DE498_GEOCMD_ROOT_ARG: model_bin + lw(+4)
FIRST_DE2A4_NODE_ARG: that root
FIRST_DE2A4_NODE_RELATIVE_TO_MODEL_BIN: 0x119F0
```

## Phase 5–7 — static walk vs live census

`func_800DE2A4` loops siblings internally (`s0 += lw(+4)`). The wrap sees
**only a1 = list head**. Later siblings never increment `DE2A4_REL_*`.

Independent compact walk of cart `0x62B` (type-2 children at
`node+lbu(+8)`):

```text
0x119F0 root type2 idx0 anim43 continue 0
  child 0x11A00 hip idx1 anim3
    child 0x11A10 ...
      ... sibling 0x11C30 torso idx8 anim1
        child 0x11C40 neck idx9 anim18 continue 0x288
          child-list head 0x11C50 type2 idx10 anim74 continue 0x50
          sibling 0x11CA0 type 0xA continue 0x18
          sibling 0x11CB8 type2 idx11 anim42 continue 0x50   ← snout
            child-list head 0x11CC8 type 0x18 continue 0x30
            sibling 0x11CF8 type 0x11 continue 0            ← snout DL
          sibling 0x11D08 type 0xC
```

Live first-48 DE2A4 tuples include `0x11C40`, `0x11C50`, then **`0x11CC8`
type 0x18** — the snout *child list head* — and never `0x11CB8`.

```text
STATIC_TREE_CHAIN_TO_0X11CB8: CORRECTED
  (not 0x11C40 child directly; snout is the third node in that child list)
OFFSET_0X11CB8_HAS_INCOMING_TREE_EDGE: YES
INCOMING_PARENT: neck 0x11C40 child list (previous sibling type 0xA @ 0x11CA0)
INCOMING_EDGE_TYPE: SIBLING (not list head)
PARENT_RUNTIME_CONDITION: DCF48(neck) always DE2A4(neck+0x10) if lbu(+8)!=0
OFFSET_0X11CB8_REACHABLE_UNDER_ACTIVE_MODE: STRONG_INFERENCE YES
```

`DE2A4_REL_0X11CB8=0` is therefore expected. It does **not** prove the
snout handler never ran.

`DCF48_SNOUT_ACCEPT_COUNT=0` used `backpack_guest_byte(+9)` via raw RDRAM
host bytes. Compact word `0x100B002A` stores INDEX 11 in the N64 byte,
which that helper read as 0. `hword(+0xA)` was already correct (42).
Accept required INDEX==11, so DCF48 wrap could have entered `0x11CB8` and
still rejected. Byte extract is now BE-from-word (same as `MEM_B`).

## Phase 6 — DCF48 semantics (`PROVEN` generated C)

```text
DCF48_ROLE: compact GeoCmd type-2 bone: optional G_MTX then child walker
DCF48_CHILD_OFFSET_FIELD: lbu(node+8); recurse DE2A4(node+that) if != 0
DCF48_NEXT_SIBLING_FIELD: none in handler; DE2A4 uses lw(node+4)
DCF48_TERMINATION_RULE: handler returns; sibling loop is in DE2A4
DCF48_SKIP_CONDITIONS: child recurse skipped iff lbu(+8)==0
DCF48_VISIBILITY_OR_STATE_GATES: G_MTX push skipped if D_8012C968==0
  (anim mtx list); children still walked
DCF48_ALWAYS_RECURSES: NO (only if lbu(+8)!=0); snout lbu(+8)==0x10 so YES
```

Mtx: `lb INDEX at +9` → `func_800ADE2C` → `G_MTX PUSH DA380002` /
`POP D8380002`. Not E2180.

## Phase 8 — geometry vs transform

```text
VISIBLE_SNOUT_DL: GeoCmd type 0x11 @ 0x11CF8 (live handler 0x800DD65C)
VISIBLE_SNOUT_DL_COMMAND_OFFSET: 0x11CF8
VISIBLE_SNOUT_DL_PARENT_CHAIN: 0x11CB8 --child--> 0x11CC8 type 0x18
  (0x800DD998) --sibling--> 0x11CF8 type 0x11
VISIBLE_SNOUT_TRANSFORM_SOURCE: compact type-2 0x11CB8 bone INDEX 11 anim 42
VISIBLE_SNOUT_USES_BONE42_TRANSFORM: STRONG_INFERENCE YES if DCF48(0x11CB8)
  runs with D_8012C968 != 0 (not yet PROVEN_RUNTIME)
VISIBLE_SNOUT_VERTEX_RANGE: UNKNOWN exact; type-11 uses lh(+0xA)=0xA5 and
  lh(+0xC)=0x16; vertex region header +0xC = 0x2668
```

## Phase 9 — other bone-42 nodes

```text
ALL_0X62B_TYPE2_BONE42_NODES: 0x11CB8 only
ALL_0X62B_BONE_INDEX11_NODES: 0x11CB8 only
OTHER_SNOUT_CANDIDATES: none on this asset
```

## Phase 10 — handler mode

```text
ACTIVE_TYPE2_SLOT: 0x800DCF48 PROVEN_RUNTIME
ACTIVE_GEOCMD_MODE: UNKNOWN this capture (player DE448 does not jal DF4B0)
WHAT_SELECTS_MODE: func_800DF4B0(a0) stores D_80123198[a0] → D_8012CF8C
  init copies [0]; other sites pass 1 or 2 before some DE448 submits
IS_MODE_STABLE_DURING_PLAYER_DRAW: UNKNOWN
ACTIVE_HANDLER_TABLE_POINTER: *D_8012CF8C (to be printed LAST_HANDLER_TABLE)
```

Type-2 slot being DCF48 does not identify which of the three tables is
active unless they differ at index 2.

## Hypotheses A–J

| ID | Verdict |
| --- | --- |
| A wrong model_bin | `REFUTED` for this capture |
| B multiple 0x62B bins | not the snout-miss cause |
| C copied GeoCmd tree | `REFUTED` for live 0x62B walk |
| D wrong root | `REFUTED` (`+0x119F0` live) |
| E parser/tree relation | `CORRECTED` (sibling, not first child) |
| F conditional skip | `REFUTED` for DCF48 on this node (`+8=0x10`) |
| G other geometry owner | `REFUTED` as transform owner; DLs are children |
| H multiple roots | `REFUTED` (one `+4` list) |
| I diagnostic lifetime | `PARTIAL`: list-head wrap + broken INDEX byte |
| J other | DE2A4 wrap ≠ full traversal |

## Next witness (implemented, default-off)

```text
NEXT_WITNESS_SEAM: wrap func_800DE498 (LOOKUP map) + DE2A4 sibling scan + DCF48
NEXT_TRAVERSAL_IDENTITY: durable/header-matched 0x62B model_bin + root
  model_bin+lw(+4) TLS token around one DE498
NEXT_FILTER_CONFIDENCE: HIGH
NEXT_WITNESS_CAN_PROVE_EXACT_0X62B_TREE: YES
LIVE_0X62B_TREE_WITNESS_IMPLEMENTED: YES
```

Output: `/tmp/tooie-secondary-sway-62b-tree.csv`. Status adds
`DCF48_REL_0X11CB8_COUNT`, `DE2A4_LIST_CONTAINS_0X11CB8`, `LAST_GEOCMD_MODE`.

```text
INTERPOLATION_INVESTIGATION_BLOCKED_BY: live snout transform-owner execution
  (DCF48 a1 == bin+0x11CB8) not yet counted with a correct INDEX/list-head
  discriminator
CURRENT_ROOT_CLASS: MORE_EVIDENCE_REQUIRED
CURRENT_ROOT_CONFIDENCE: LOW
GPU_READBACK_USED: NO
```
