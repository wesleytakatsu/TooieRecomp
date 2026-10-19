---
title: "P09 V7 CPU handoff witness implementation"
description: "V7 CPU handoff witness; VERSION=2 stride-3 comparator and join-role assignment."
status: current
type: evidence
updated: 2026-09-07
---

# P09 V7 bounded CPU handoff witness — implementation checkpoint

## Result

`V7_SPEC_READ_COMPLETELY: YES`

`V7_IMPLEMENTED: YES`

`V7_SCHEMA: P09_FEATHER_HANDOFF_V7 / VERSION=1`

```text
APPEND_RECORD_IMPLEMENTED: YES
INITIAL_HANDOFF_RECORD_IMPLEMENTED: YES
RENDER_SELECTION_RECORD_IMPLEMENTED: YES
POST_APPEND_BASELINE_IMPLEMENTED: YES
FULLSYNC_FACE_SLICE_WITNESS_IMPLEMENTED: YES
UPLOAD_SOURCE_WITNESS_IMPLEMENTED: YES
JOIN_0X592_TARGETED: YES
DISTAL_JOIN_0X54C_TARGETED: YES
DISTAL_JOIN_0X564_TARGETED: YES
PER_TARGET_RESERVED_SAMPLING: YES (6 targets; 128 triangles/target/workload; 16 workloads)
GLOBAL_VERTEX_IDENTITY_CAPTURED: YES
MATRIX_BYTES_OR_EQUIVALENT_CAPTURED: YES
VELOCITY_INVARIANT_CAPTURED: YES
TRANSFORM_PROCESSOR_PREDICATES_CAPTURED: YES
RENDER_SELECTED_MATRIX_SOURCE_CAPTURED: YES
CAPTURE_CAN_SELF_DECLARE_INCOMPLETE: YES
CONTROL_PATH_UNCHANGED_WHEN_DISABLED: YES
V7_OUTPUT_BOUNDED: YES
```

The implementation is default-off under `TOOIE_DIAG_FEATHER_HANDOFF_V7=1` and
is confined to the owning RT64 workload/RSP/fullSync/transform source. It does
not rewrite face indices, matrices, buffers, rendering state, or synchronization.
Generated recompilation output was not edited.

## Witness coverage

The append record is emitted immediately after the three real face-index
appends in `RSP::drawIndexedTri`. It is owned by the current `Workload` and
keyed by a workload-owned diagnostic generation, ring slot, model resource
`0x62B`, finalized workload ID, target DL, and actual face offset. It copies the
bounded corner/global/RSP identity, load provenance, world/view indices,
position/velocity, matrix bytes, and G_MTX provenance.

The initial handoff record runs after `meshDesc.faceIndicesStart` assignment
and `updateDrawDataRanges`, before `uploadDrawData`. It checks the exact face
slice, source-array bounds, position, velocity, world index, view index, and
original world-matrix byte equality. It emits one success per target/DL per
workload and bounded mismatch records.

The render-selection record runs from `TransformProcessor::upload`; when that
processor is skipped, `threadRenderFrame` records the known original-source
path. It records previous-frame/match/map/process predicates, selected source
kind and size, upload range, weights, selected view-projection size/matrix,
referenced selected world-matrix bytes, and RSP vertex range. No GPU resource is
read or synchronized.

Target quotas are independent for `0x534`, `0x54C`, `0x564`, `0x57C`, `0x592`,
and `0x5AA`, with 128 target triangles per target/workload and a 16-workload
bounded aggregate. Overflow and unresolved-join counters are written to
`/tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt`; unresolved join role
coverage keeps `CAPTURE_COMPLETE_FOR_CAUSAL_DECISION=NO`.

`JOIN_UNRESOLVED_IS_BLOCKER: YES` remains in force. No runtime capture was
authorized or run, so no causal result is claimed.

## Validation

- `cmake --build build --target TooieRecompiled -j2`: **PASS**.
- `cmake --build build --target window_display_test tintro_projection_policy_test -j2`: **PASS**.
- `./build/window_display_test`: **PASS** (38/38 assertions).
- `./build/tintro_projection_policy_test`: **PASS** (7/7 assertions).
- TooieRecomp `git diff --check`: **PASS**.
- RT64 `git diff --check`: **PASS**.
- Game/manual capture: **SKIPPED** by instruction; no runtime PASS is claimed.

`NEXT_SINGLE_ACTION`: user runs one documented `TOOIE_DIAG_FEATHER_HANDOFF_V7=1`
capture with the launcher, then inspect the bounded CSV/status pair and retain
the incomplete-join gate.

`USER_MANUAL_TEST_REQUIRED_NOW: YES`

`USER_TEST_COMMAND: TOOIE_DIAG_FEATHER_HANDOFF_V7=1 ./build/TooieRecompiled`

`GPU_READBACK_USED: NO`; `GPU_SYNCHRONIZATION_ADDED: NO`; `NO_COMMIT: YES`;
`NO_PUSH: YES`.

## Activation and artifact lifecycle repair (2026-09-05)

The first requested capture produced neither artifact because status creation
was previously deferred until a target APPEND or INITIAL_HANDOFF. The RT64
diagnostic now creates the status heartbeat during RT64 startup (with the
cached `TOOIE_DIAG_FEATHER_HANDOFF_V7` path as a guard), independent of target
hits. The status includes schema/version, enabled state, completeness,
raw APPEND/fullSync/render-selection hook counters, and post-filter tracked
counters. CSV creation remains lazy at the existing combined witness path.

```text
V7_STATUS_PATH: /tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt
V7_COMBINED_CSV_PATH: /tmp/tooie-kazooie-feather-v7-cpu-handoff.csv
V7_RUNTIME_ACTIVATION_REPAIRED: YES
V7_STATUS_CREATED_AT_ENABLE: YES
V7_RAW_HOOK_COUNTERS_IMPLEMENTED: YES
V7_SHUTDOWN_FINALIZER: normal-exit atexit flush/close
```

No witness semantics, production rendering, GPU behavior, commit, or push was
changed. The next manual run must remove exactly those two files first and use
the direct build binary documented in the activation analysis.

## Collision audit before capture (2026-09-05)

The low-page runtime resolver must be treated as a candidate filter, not an
identity proof. Its exact expression is
`(current_dl & 0xFFFu) == ((0x2670u + logical * 8u) & 0xFFFu)`; only 12 bits
are compared. Any two addresses differing by `k * 0x1000` collide. The six
target offsets remain mutually distinct under this mask (`0x010`, `0x0D0`,
`0x190`, `0x250`, `0x300`, `0x3C0`), but unrelated models can share a page
pattern.

The later handoff path verifies face/index/matrix consistency and may compare a
game-call `sourceDl` to the candidate's same `currentDl`. It does not recover
model identity, verify the full V6C model-relative address, or require a
generation/model mapping. The `sourceDl == record.currentDl` check is only
same-record full-address self-consistency, so it does not reject a collision.
Because APPEND records and `OBSERVED_TARGET_MASK`
are published before that path, a false candidate can enter the CSV and causal
completeness logic. A bounded static scan found no attributable non-feather DL
collision, but the complete runtime address set is unavailable, so the count is
`UNKNOWN`.

```text
V7_TARGET_MATCH_COLLISION_SAFE_FOR_CAUSAL_USE: NO
V7_CAN_REPORT_FALSE_POSITIVE_CANDIDATES: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
```

No production fix was applied; capture authorization is blocked pending an
independent validation state.

## Exact identity validation implementation (2026-09-05)

The low-page resolver is now candidate-only. Each candidate preserves the full
`current_dl`, logical target, workload generation, and a bounded record. The
RT64 append seam obtains the latest game-side model-bin identity through the
diagnostic bridge and validates only the full V6C address:
`current_dl == physical(model_bin) + 0x2670 + logical_index * 8`.
Unvalidated candidates are emitted with a rejection reason and are excluded
from append records, observed-target coverage, and causal CSV rows.

```text
LOW_PAGE_RESOLVER_ROLE_AFTER_REPAIR: CANDIDATE_PREFILTER_ONLY
APPEND_CANDIDATE_PRESERVES_FULL_CURRENT_DL: YES
TARGET_VALIDATION_USES_FULL_ADDRESS: YES
TARGET_VALIDATION_USES_MODEL_CONTEXT: YES
V7_VALIDATION_STRENGTH_EQUIVALENT_TO_V6C: YES
TARGET_CANDIDATE_COUNTERS_IMPLEMENTED: YES
TARGET_VALIDATED_COUNTERS_IMPLEMENTED: YES
TARGET_REJECTED_COUNTERS_IMPLEMENTED: YES
UNVALIDATED_CANDIDATE_CAN_REACH_CAUSAL_ANALYSIS: NO
BOUNDED_VALIDATION_REJECTION_SAMPLE_IMPLEMENTED: YES
```

This is diagnostic correlation only. No game run, GPU operation, commit, or
push was performed.

The implementation now satisfies the collision-safety gate for one user-run
capture. The command remains manual and was not executed by this agent.

## Target admission repair and self-diagnosis (2026-09-05)

The V7 append seam receives `DlContext::current_dl` as a resolved physical
RDRAM address, while the proven V6C witness names model-bin entries by logical
offsets (`0x534`, `0x54C`, `0x564`, `0x57C`, `0x592`, `0x5AA`). `p09V7TargetIndex`
now preserves logical matching and also matches the V6C entry layout by low
page offset (`physical(model_bin) + 0x2670 + index * 8`) when the model-bin base
is unavailable at the RT64 seam. No workload identity, generation, face,
matrix, upload, render, or synchronization behavior is part of admission.

The append path now publishes aggregate filter counters in the status file:
raw hook, nonzero DL context, resolver match, target kind assignment, and
accepted target. It retains at most eight rejection samples containing the DL,
representation classification, target value, workload ID, diagnostic generation,
reason, and an explicit `model_bin:UNAVAILABLE` marker. This makes states A–E
distinguishable from one status artifact without unbounded logging.

Validation after the repair and counter addition: `TooieRecompiled` build,
`window_display_test` (38/38), `tintro_projection_policy_test` (7/7), and both
repository `git diff --check` commands passed. No game run was performed.
## Exact model-bin bridge repair (2026-09-05)

The V7 bridge now reuses `secondary_sway_register_durable_bin()` as the
metadata classifier. The already-proven raw `0x62B` identity remains directly
admissible; raw `0x62A` is admitted only when the model header has the
established `magic=0xB`, non-flagged geometry type, and 66-bone shape that
normalizes to live `0x62B`. It is therefore not treated as an alias without
metadata proof. The bridge retains the full
64-bit `ctx->r2` guest pointer and masks only when crossing into the 32-bit
guest-RDRAM/physical-address access seam. Rejected getter events clear the previous bin
to prevent stale identity.

The V7 status now exposes raw `0x62A`, raw `0x62B`, normalized-`0x62B`,
accepted, and rejected getter counters, plus bounded first/last accepted raw
resource, normalized identity, full guest pointer, and physical-base fields.
The exact validation equation and candidate-only low-page prefilter are
unchanged.

## State F — durable-bin authoritative model-context publication (2026-09-07)

After the fresh V7 capture showed 1,786 getter bridge rejects and no model-bin
identity, the bridge was repaired at `secondary_sway_register_durable_bin()`.
The seam's existing metadata proof (magic `0xB`, gtype flag clear, 66 bones)
now publishes normalized `0x62B` context directly. The guest pointer is
sign-extended into `uint64_t`; the physical base is retained separately.
Context generation, publish, clear, replacement, and eviction counters are
exported through the RT64 status path. `func_80092B04` remains a non-authoritative observer/entry path with explicit rejection reasons; any model it supplies is still admitted only by the durable seam and it cannot override published context.

Exact V7 validation and all append/geometry/upload/join/render paths are
unchanged. Build, window, projection, diff, and graft checks pass. No game
run or capture was performed.

## Exact target admission repair after 0x62B context (2026-09-07)

The durable model-context bridge published 1297 normalized `0x62B` identities
(`LAST_PUBLISHED_PHYSICAL_BASE=0x229070`) with 0 clears, but
`TARGET_VALIDATED_COUNT` stayed 0. The low-page candidate prefilter compared
`(current_dl & 0xFFF) == ((0x2670 + logical * 8) & 0xFFF)` and omitted
`physical(model_bin) & 0xFFF`. That base is `0x070`, so true target low pages
are `0x080/0x140/0x200/0x2C0/0x370/0x430` while the prefilter looked for
`0x010/0x0D0/0x190/0x250/0x300/0x3C0`. The 5296 "candidates" were false
low-page collisions; exact validation never saw the six real DLs.

`ZERO_VALIDATION_EXPLAINED_BY_BAD_PREFILTER: YES`. The durable context is
not blamed. 1297 publications with 0 clears prove repeated publication of one
bin (`0xFFFFFFFF80229070` / `0x229070`); a registry scan is not required for
this bounded Player capture.

Admission is now `SINGLE_EXACT_CONTEXT_DIRECT_MATCH`: if the published
normalized `0x62B` physical base is missing, count `NO_MODEL_CONTEXT` and do
not track; otherwise compare `current_dl` to the six V6C addresses
`base+0x5010/0x50D0/0x5190/0x5250/0x5300/0x53C0`. The `0xFFF` prefilter is
removed. Counters are `APPEND_WITH_VALID_MODEL_CONTEXT`,
`APPEND_EXACT_TARGET_MATCH`, `APPEND_NO_EXACT_TARGET_MATCH`, plus per-target
exact counts. Face indices, APPEND payload contents, vertices, matrices,
fullSync, upload, render, join ownership, and synchronization are unchanged.

```text
CURRENT_LOW_PAGE_CANDIDATE_EXPRESSION: (current_dl & 0xFFF) == ((0x2670 + logical*8) & 0xFFF)  [REMOVED]
CURRENT_MODEL_BASE_LOW_12: 0x070
TRUE_TARGET_LOW_PAGE_INCLUDES_MODEL_BASE: YES
CURRENT_PREFILTER_SYSTEMATICALLY_MISSES_TRUE_TARGET_WHEN_BASE_NOT_PAGE_ALIGNED: YES
ZERO_VALIDATION_EXPLAINED_BY_BAD_PREFILTER: YES
V7_CAN_USE_SAME_EXACT_FORMULA_AT_APPEND_NOW: YES
LOW_PAGE_PREFILTER_STILL_ARCHITECTURALLY_NECESSARY: NO
UNIQUE_62B_MODEL_BIN_COUNT_FROM_AVAILABLE_EVIDENCE: 1
REPEATED_PUBLICATION_SAME_BIN: YES
MULTIPLE_LIVE_62B_CONTEXTS_POSSIBLE_FOR_THIS_CAPTURE: NO
CURRENT_SINGLE_CONTEXT_CAN_MISATTRIBUTE_TARGET: NO
SELECTED_TARGET_IDENTITY_DESIGN: SINGLE_EXACT_CONTEXT_DIRECT_MATCH
LOW_PAGE_PREFILTER_AFTER_REPAIR: REMOVED
DIRECT_MATCH_EQUIVALENT_TO_V6C_IDENTITY_STRENGTH: YES
FALSE_LOW_PAGE_CANDIDATES_CAN_APPEAR_AS_TARGETS_AFTER_REPAIR: NO
V7_EXACT_TARGET_ADMISSION_REPAIRED: YES
```

For observed base `0x229070` the six exact addresses are statically reachable:
`0x22E080`, `0x22E140`, `0x22E200`, `0x22E2C0`, `0x22E370`, `0x22E430`.

Validation after this repair:

- `cmake --build build --target TooieRecompiled -j2`: **PASS**
- `./build/window_display_test`: **PASS** (38/38)
- `./build/tintro_projection_policy_test`: **PASS** (7/7)
- TooieRecomp `git diff --check`: **PASS**
- RT64 `git diff --check`: **PASS**
- Game: **SKIPPED** by instruction

`USER_MANUAL_TEST_REQUIRED_NOW: YES`

`USER_TEST_COMMAND: TOOIE_DIAG_FEATHER_HANDOFF_V7=1 ./build/TooieRecompiled`

Remove `/tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt` and
`/tmp/tooie-kazooie-feather-v7-cpu-handoff.csv` before the capture.

`GPU_READBACK_USED: NO`; `GPU_SYNCHRONIZATION_ADDED: NO`; `NO_COMMIT: YES`;
`NO_PUSH: YES`.

## Exact-admission capture + byte_mismatch / join-role diagnostic repair (2026-09-07)

Fresh runtime proved exact admission (`APPEND_EXACT_TARGET_MATCH=40124`,
mask `0x3F`). Face slice vs APPEND was equal (`FACE_SLICE_MISMATCH_COUNT=0`)
but every sampled `INITIAL_HANDOFF_OK` row had `byte_mismatch=3`.

`p09V7LogInitialHandoff` accumulated `byte_mismatch` as a **per-corner
increment**, not a field bitmask. Value 3 means one increment on each of
the three corners. Face equality was tracked separately (`mismatch` bool)
and was false. Matrix bytes on the first per-target dumps matched APPEND
exactly. Velocity was zero on both sides.

The increment is a **comparator artifact**: APPEND/handoff copied and
compared `posFloats`/`velFloats` with stride 4 (`global*4`), while
`RSP::setVertexCommon` stores object-space xyz with stride 3
(`globalIndex*3`). At APPEND time `global*4+2` is typically past the
current buffer size, so `posXYZ` stayed `0`. At `fullSync` the buffer is
larger, the same wrong index becomes in-range, and 12 zero bytes compare
unequal to later floats. Direct xyz field compares have no struct padding.
`float4x4` is `float m[4][4]` (64 bytes) versus `uint32_t[16]`; that copy
is semantic matrix bits, not padding.

Join expected-role was hardcoded `UINT32_MAX` / `expectedRoleResolved=0`
for `0x54C`/`0x564`/`0x592`. Static GeoCmd plus the same capture already
prove the slot map:

- `0x54C` (bone 105 type-0x18): child slots 0–3 source `0x010035D0`, POP,
  parent slots 28–31 source `0x01003610` (parent bone 104)
- `0x564` (bone 104 type-0x18): child `0x01003650` / parent `0x01003690`
  (parent bone 103)
- `0x592` (bone 103 type-0x18): child `0x01003750` / parent `0x01003790`
  (parent of `+0x124A0`)

CSV triangles only use those slots, with two world-index clusters.

Repair (diagnostic only, schema `P09_FEATHER_HANDOFF_V7` **VERSION=2**):
stride-3 field-wise xyz copies/compares; per-component match/mismatch
counters; join roles assigned from the proven slot map.

```text
BYTE_MISMATCH_PRODUCER_FUNCTION: RT64::p09V7LogInitialHandoff
BYTE_MISMATCH_VALUE_3_MEANING: one per-corner increment on all 3 corners; stride-4 pos compare vs stride-3 store
HANDOFF_BYTE_MISMATCH_CLASS: COMPARATOR_ARTIFACT
FIRST_SEMANTIC_HANDOFF_DIVERGENCE: NONE
SCHEMA_CHANGE_REQUIRED: YES
NEW_SCHEMA_VERSION: 2
DIAGNOSTIC_REPAIR_REQUIRED: YES
```

Validation after this repair:

- `cmake --build build --target TooieRecompiled -j2`: **PASS**
- `./build/window_display_test`: **PASS** (38/38)
- `./build/tintro_projection_policy_test`: **PASS** (7/7)
- TooieRecomp `git diff --check`: **PASS**
- RT64 `git diff --check`: **PASS**
- Game: **SKIPPED** by instruction

`USER_MANUAL_TEST_REQUIRED_NOW: YES`

`USER_TEST_COMMAND: rm -f /tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt /tmp/tooie-kazooie-feather-v7-cpu-handoff.csv && TOOIE_DIAG_FEATHER_HANDOFF_V7=1 ./build/TooieRecompiled`
