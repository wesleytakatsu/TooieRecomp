---
title: "P09 corrected 0x62B snout E2180 capture — runtime identity mismatch"
description: "A current-build CPU capture reaches E2180, but its observed a1 population does not match the independently parsed 0x62B snout identity."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-02
tags: [tooie-recomp, p09, snout, geocmd, e2180, runtime-cpu, filter]
---

# P09 corrected 0x62B snout E2180 capture — runtime identity mismatch

## Capture identity (`PROVEN`, `RUNTIME_CPU`)

Command:

```sh
rm -f /tmp/tooie-secondary-sway-*
TOOIE_DIAG_SECONDARY_SWAY=1 ./build/TooieRecompiled
```

All timestamps are UTC. The capture postdates the current executable:

| Item | Timestamp |
| --- | --- |
| `build/TooieRecompiled` | `2026-09-02T15:16:20.887Z` |
| E2180 tuples | `2026-09-02T20:00:24.468Z` |
| model CSV | `2026-09-02T20:00:28.103Z` |
| status | `2026-09-02T20:00:28.148Z` |

```text
CAPTURE_POSTDATES_CURRENT_EXECUTABLE: YES
CAPTURE_USABLE: YES (current corrected filter; no accepted snout sample)
GPU_READBACK_USED: NO
```

Artifact hashes:

```text
fca9d0d985c70a5aff5b69f08ba9e6239560ca45976561aed358fbde0ff54979  tooie-secondary-sway-status.txt
6eec36f75661dafbf95e4739f73ad44272212160a263a291f5f25136901663b9  tooie-secondary-sway-model.csv
7913f8a7bb31ea5493b1bd8e2d3149ec3e5bf8181d08e6dfca7c3ed7b08a07b9  tooie-secondary-sway-e2180-tuples.csv
```

## Runtime counts (`PROVEN`)

```text
DE448_0X62B_REGISTRATION_COUNT: 5386 registration hits
REGISTERED_0X62B_BIN_COUNT: 1
REGISTERED_0X62B_MODEL_BIN: 0x80226130
REGISTER_DURABLE_HITS: 5386 (two per model row)
E2180_RAW_ENTRY_COUNT: 1062
E2180_NODE_INSIDE_0X62B_COUNT: 216
E2180_REL_0X11CB8_COUNT: 0
E2180_TYPE2_COUNT: 0
E2180_BONE42_COUNT: 0
SNOUT_FILTER_ACCEPT_COUNT: 0
SNOUT_CSV_ROWS: 0 (file absent)
```

The model CSV has 2,816 rows: 123 for `0x62A` and 2,693 for `0x62B`.
All `0x62B` rows use the same observed model bin, `0x80226130`. The durable
registration counter is 5,386 hits—two per `0x62B` model row—so it is not a
unique-bin count.

The bounded E2180 tuple CSV has 30 rows. Its only in-span tuple is:

```text
FRAME=3038
A1=0x80241E50
MODEL_BIN=0x80226130
A1_MINUS_BIN=0x1BD20
WORD0=0x000A0004
BYTE9=0x00
HWORD_A=0x0020
IN_SPAN=1
TYPE2=0
BONE42=0
REL_11CB8=0
```

The other 29 bounded tuples are outside the durable span, usually because
the model bin was not yet available when the call arrived. Across the full
counter population, `846` calls are outside the registered span and `216`
are inside it. All `1,062` raw calls fail the current `type == 2` and
`boneAnimID == 42` predicates; none has relative offset `0x11CB8`.

## Rejection breakdown and failed assumption (`PROVEN`)

```text
SNOUT_FILTER_REJECTION_BREAKDOWN:
  NO_DURABLE_MODEL=472
  WRONG_NODE=1062
  NODE_OUTSIDE_MODEL_SPAN=846
  WRONG_NODE_TYPE=1062
  WRONG_BONE_ANIM_ID=1062
  NODE_POINTER_FORM_MISMATCH=0
  OTHER_NODE_REASON=0
```

`WRONG_NODE` is the post-registration matcher rejection. It includes the
node-type, boneAnimID, and exact-offset alternatives; the counters are not
mutually exclusive. The important discriminator is that the 216 in-span
calls still have zero type-2, zero bone-42, and zero `0x11CB8` matches.

This is not evidence that the independently parsed snout is absent from the
asset. It proves that the current E2180 population is not the assumed live
`0x62B` snout population, or that the runtime meaning/extraction of the
observed node fields is different from the diagnostic's assumptions. The
single in-span tuple is a node-like pointer at `+0x1BD20`, not the target
`+0x11CB8`; its raw header is `0x000A0004`, not the diagnostic's assumed
type value `2`, and its `+0xA` value is `0x20`, not `42`.

The generated path still proves that E2180 receives the saved E2AA4 `a1`.
Therefore the remaining uncertainty is structural: identify which GeoCmd
nodes E2AA4/E2180 traverses at runtime and decode the node header/field
semantics before treating any E2180 call as the live snout transform.

## Matrix/root classification

```text
LIVE_SNOUT_NODE_OFFSET: NOT_CAPTURED
LIVE_SNOUT_OFFSET_EQUALS_0X11CB8: UNKNOWN (no accepted live snout node)
LIVE_SNOUT_IDENTITY_CONFIDENCE: LOW
CURRENT_MATRIX_SOURCE_RUNTIME: NOT_CAPTURED
PREVIOUS_MATRIX_SOURCE_RUNTIME: NOT_CAPTURED
SNOUT_CURRENT_MATRIX_SAMPLE_COUNT: 0
SNOUT_PREVIOUS_MATRIX_SAMPLE_COUNT: 0
CURRENT_MATRIX_TRUE_ANOMALY_COUNT: UNKNOWN
STALE_PREVIOUS_MATRIX_COUNT: UNKNOWN
WRONG_PREVIOUS_MATRIX_ASSOCIATION_COUNT: UNKNOWN
PREVIOUS_EQUALS_PRIOR_FRAME_CURRENT: UNKNOWN
PREVIOUS_CURRENT_LOGICAL_PAIRING: UNKNOWN
MATRIX_IDENTITY_STABLE_ACROSS_FRAMES: UNKNOWN
SNOUT_CURRENT_GUEST_MATRIX_VALID: UNKNOWN
SNOUT_PREVIOUS_GUEST_MATRIX_VALID: UNKNOWN
TOOIE_MATRIX_GROUP_PRESENT: NO (no accepted witness)
DOES_CAPTURE_SHOW_IDENTITY_AMBIGUITY_THAT_GROUPING_WOULD_SOLVE: UNKNOWN
FIRST_BAD_LAYER: E2180 live-node identity/field decoding
ROOT_CLASS: RUNTIME_E2180_POPULATION_OR_NODE_SEMANTICS_MISMATCH
ROOT_CONFIDENCE: LOW
CAUSAL_CHAIN: durable 0x62B registration -> E2180 raw entries -> no in-span target identity -> no matrix witness -> transform/interpolation root unproven
GUEST_ANIMATION_STATE_VALID: UNKNOWN
```

No production fix is authorized by this evidence. Do not request another
identical capture: the corrected filter is running and the remaining blocker
is structural E2180/GeoCmd reverse engineering.

```text
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
NO_COMMIT: YES
NO_PUSH: YES
```
