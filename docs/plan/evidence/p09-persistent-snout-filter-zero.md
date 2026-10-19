---
title: "P09 corrected snout filter attempt — stale artifact set"
description: "The only available artifacts predate the corrected 0x11CB8 executable; their zero-accept result belongs to the retired 0x14960 filter."
status: superseded
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-02
tags: [tooie-recomp, p09, snout, geocmd, runtime-cpu, filter]
---

# P09 corrected snout filter attempt — stale artifact set

## Capture identity (`PROVEN`, `RUNTIME_CPU`)

Command:

```sh
TOOIE_DIAG_SECONDARY_SWAY=1 ./build/TooieRecompiled
```

Times below are UTC. The only artifacts currently present are older than both
the corrected source and the current executable:

| Item | Timestamp |
| --- | --- |
| `src/game/tooie_diag.cpp` (current) | `2026-09-02T14:57:54.597Z` |
| `build/TooieRecompiled` (current) | `2026-09-02T15:16:20.887Z` |
| capture CSV/status | `2026-09-02T14:29:09.301Z` |

The current executable contains `OLD_0X14960_ASSUMPTION=RETIRED`,
`E2180_REL_0X11CB8_COUNT`, and the corrected `0x11CB8` filter string. The
`status.txt` artifact instead contains the retired filter and
`SNOUT_NODE_OFFSET=0x14960`. Therefore this artifact set is not a capture of
the corrected diagnostic.

Artifact hashes:

```text
53ffbf552da308a525743e762aeb6566d1b7c09b3c8d23285bc84802fae8d546  tooie-secondary-sway-status.txt
762af3984b8b2c95b1c5658a18d4dbc1b46d507fc7093d4a8045dea37baab726  tooie-secondary-sway-model.csv
```

## Historical result from the stale artifact

```text
DE448_0X62B_REGISTRATION_COUNT: 6054
REGISTERED_0X62B_BIN_COUNT: 1 (derived from model.csv)
REGISTERED_0X62B_MODEL_BIN: 0x8020E340
E2180_RAW_ENTRY_COUNT: 1614
SNOUT_FILTER_ACCEPT_COUNT: 0
SNOUT_CSV_ROWS: 0 (file absent)
BONES_CSV_ROWS: 0 (file absent)
HIP_CSV_ROWS: 0 (file absent)
```

The stale model CSV contains 3,151 rows: 3,027 rows for resource `0x62B` and one
unique `0x62B` model bin. The 6,054 registration hits are exactly two
registrations per model row (fingerprint registration and resource-resolved
registration). This proves the durable registration path was working in the
retired-filter run; it does not prove a corrected-filter snout node match.

The 1,614 raw E2180 entries split completely as:

```text
SNOUT_REJECT_NO_DURABLE_MODEL: 496
SNOUT_REJECT_WRONG_NODE: 1118
SNOUT_REJECT_NO_GETTER: 0
SNOUT_REJECT_WRONG_RESOURCE: 0
```

`NO_DURABLE_MODEL` is the exact early predicate for 496 calls: no durable
model list was available when those calls arrived. The remaining 1,118 calls
reach `secondary_sway_match_durable_snout`; its single `WRONG_NODE` counter
merges these checks, in source order:

1. `word(node + 0) == 2`;
2. `hword(node + 0xA) == 42`;
3. `node` lies in a registered model-bin span.

Therefore the stale artifact proves that one or more of those three
conditions fails, but it does not identify which one. `TYPE2_COUNT`,
`BONE42_COUNT`, and `E2180_IN_REGISTERED_0X62B_SPAN_COUNT` are not recoverable
from this capture because no per-entry raw argument CSV exists and the
diagnostic does not count those predicates separately. The model fingerprint
is not the observed failure (`WRONG_RESOURCE=0`, with a `0x62B` registration
present).

## Interpretation

```text
PERSISTENT_MODEL_REGISTRATION_WORKING: YES
PERSISTENT_SNOUT_ACCEPTANCE_WORKING: NO (zero accepts)
SNOUT_NODE_FILTER_WORKING: UNKNOWN (aggregate WRONG_NODE only)
E2180_RUNTIME_MAP_REACHABLE: YES
E0B8C_RAW_ENTRY_COUNT: 0
HIP/ADDITIVE_STATE: UNKNOWN (conditional record writer not entered)
SNOUT_TRANSFORM_WITNESS: UNAVAILABLE
FIRST_BAD_LAYER: UNKNOWN
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
```

Because the artifact is stale, these counters are retained only as historical
evidence of the retired filter and must not be used to evaluate the corrected
`0x11CB8` predicates. No corrected tuple or snout matrix was captured.

This is not evidence of a guest-animation, hierarchy, previous/current
matrix, or interpolation-tagging defect. No matrix or node pointer was
accepted, so matrix continuity, logical pairing, identity stability, and
anomaly counts remain `UNKNOWN`. The next step is targeted structural reverse
engineering of the E2180 live arguments and node layout, not another
identical manual capture. No production fix, commit, or push was made.

`CAPTURE_USABLE: NO`

`NEXT_SINGLE_ACTION: run TOOIE_DIAG_SECONDARY_SWAY=1 with the current
build/TooieRecompiled, then inspect the corrected tuple/status/snout files`

`GPU_READBACK_USED: NO`
