---
title: "P09 backpack flap runtime witness"
description: "P09 backpack geometry owner is overlay babackpack draw via func_800DE448 asset 0x621; dual-mtx child path is func_800E2180. Filtered default-off witness implemented. Flap node ID still UNKNOWN."
status: provisional
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-01
tags: [tooie-recomp, p09, backpack, bsbflap, animation, transform, interpolation, witness]
---

# P09 backpack flap runtime witness

## CURRENT (2026-09-01) — self-verifying dual-matrix witness repair

The zero-sample capture did not prove or refute interpolation tagging. The
diagnostic audit found a real propagation defect: the previous single
thread-local `active` scope was cleared by every nested `func_800DE448`, and
an inner tracked draw could leave its parent inactive on return.

The source-owned diagnostic now uses a bounded 32-entry thread-local draw
scope stack. Every DE448 call pushes a frame and restores it on return;
untracked nested models therefore cannot inherit or erase a tracked parent.
Overflow is counted and reported. No generated file and no production
transform path was changed.

The wrappers and status counters now cover the complete known chain:

```text
BABACKPACK_ENTRY3_HITS
ASSET_RESOLVE_HITS + ASSET_0621/0625/060A/061B/0831_HITS + ASSET_OTHER_HITS
DE448_TOTAL_HITS + DE448_BACKPACK_FILTER_HITS
DE498_TOTAL_HITS + DE498_BACKPACK_FILTER_HITS
E2AA4_TOTAL_HITS + E2AA4_BACKPACK_FILTER_HITS
E2180_TOTAL_HITS + E2180_BACKPACK_FILTER_HITS
E2180_SAMPLE_WRITTEN_COUNT
E2180_REJECT_NO_ACTIVE_MODEL_CONTEXT
E2180_REJECT_MODEL_ID_MISMATCH
E2180_REJECT_NO_NODE_CONTEXT
E2180_REJECT_UNEXPECTED_NODE_TYPE
E2180_REJECT_OTHER
```

The stable runtime identity is `MODEL_BIN_POINTER`: the resource ID is
proven at `func_800D674C`, then the resolved pointer is carried into the
DE448 fifth argument (`sp+0x10`) and used to open the draw scope. Resource ID
comparison is not repeated at inner GeoCmd levels.

Static asset selection in `babackpack_set_state` is:

| State case | Asset | Static evidence |
| ---: | ---: | --- |
| 2 | `0x621` | scale `1.0`; primary gameplay model |
| 3 | `0x625` | scale `0.1`; fixed small-state variant, visual role otherwise unknown |
| 4 | `0x60A` | scale `0.1`; fixed small-state variant, visual role otherwise unknown |
| 5 | `0x61B` | scale `0.1`; fixed small-state variant, visual role otherwise unknown |
| 6 | `0x831` | scale `0.1`; fixed small-state variant, visual role otherwise unknown |
| 7 | dynamic | ID comes from `func_80084F38`; not included in the fixed-ID filter |

Validation completed:

```text
FILTER_PROPAGATION_BUG_FOUND: YES (fixed)
FILTER_CONTEXT_NEST_SAFE: YES (bounded stack, depth 32)
BABACKPACK_ENTRY3_COUNTER: IMPLEMENTED
ASSET_RESOLVE_COUNTERS: IMPLEMENTED
DE448_STAGE_COUNTERS: IMPLEMENTED
DE498_STAGE_COUNTERS: IMPLEMENTED
E2AA4_STAGE_COUNTERS: IMPLEMENTED
E2180_STAGE_COUNTERS: IMPLEMENTED
E2180_REJECTION_COUNTERS: IMPLEMENTED
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
DIFF_CHECK: PASS
ROOT_CLASS: INTERPOLATION_TAGGING_ROOT_CANDIDATE / MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
GPU_READBACK_USED: NO
```

The diagnostic remains default-off. One short capture is now justified:

```sh
cd /home/wesley/projetos/N64-Recomp/TooieRecomp && \
rm -f /tmp/tooie-backpack-flap-* && \
TOOIE_DIAG_BACKPACK_FLAP=1 ./build/TooieRecompiled
```

Enter gameplay, walk/run Banjo for about 30 seconds, then exit. The next
analysis must begin with the stage counters; no production tagging change is
authorized until a populated flap path, a true anomaly, and a narrow
high-confidence root are established.

## Runtime result

The completed run reached diagnostic initialization but never reached the
wrapped function:

```text
DIAGNOSTIC_ENABLED: YES
INIT_REACHED: YES
BSBFLAP_FIRST_HIT: NO
BSBFLAP_UPDATE_COUNT: 0
SAMPLE_COUNT: 0
ANOMALY_COUNT: 0
LAST_FRAME: 0
LAST_FLUSH_REASON: shutdown
RUNTIME_WITNESS_CAPTURED: NO
```

This is `Case A`, not evidence of a clean animation or of a transform
anomaly. The zero is captured in `/tmp/tooie-backpack-flap-status.txt`; the
witness CSV contains only its header and the anomaly report says
`NO_ANOMALY_PROVEN`.

The corrected capture on 2026-09-01 reports the proven filter constants but
still has no syscall witness:

```text
SYSCALL_HOOK_INSTALLED=YES
BABACKPACK_SYSCALL_FIRST_HIT=NO
BABACKPACK_SYSCALL_HIT_COUNT=0
SECTION_ID=113
ENTRY_INDEX=3
TARGET_SYMBOL=babackpack_entrypoint_3
BACKPACK_INSTANCE_POINTER=UNKNOWN
LAST_FRAME=0
LAST_FLUSH_REASON=shutdown
```

The witness CSV is header-only and the anomaly report has
`first_bad_stage=NO_ANOMALY_PROVEN`. `RUNTIME_WITNESS_CAPTURED: NO` remains
the correct result.

## Zero-update arbitration

`bsbflap_update` is a real generated function at section-relative offset
`0x37C` in overlay section 90 (`bsbflap`), but the live backpack asset is a
different overlay: section 113 (`babackpack`). Its narrow render boundary is
`babackpack_entrypoint_3` at section-relative offset `0x320`; that function
passes the backpack object through the asset-specific path ending at the
common `func_800DE448` submit helper.

The installed wrapper is not a reliable runtime dispatch hook. Tooie's
`recomp_load_overlays()` and `recomp_load_overlays_by_rom()` call the runtime
`load_overlays()` directly. Runtime `load_overlay()` writes each loaded
function into `func_map[ram + offset]`, overwriting an earlier
`add_loaded_function(0x8080037C, ...)` entry. The wrapper is reinstalled only
by the special `func_80019EC0` path, not by the generic overlay-load APIs.

```text
ZERO_UPDATE_ROOT: HOOK_NOT_CONNECTED_TO_RUNTIME_RECOMP_DISPATCH
SECONDARY_STATE: BSBFLAP_RUNTIME_ACTIVATION_UNPROVEN
NEXT_CORRECT_HOOK: babackpack_entrypoint_3 (overlay section 113, offset 0x320)
```

This runtime result therefore supersedes the earlier claim that the
`bsbflap_update` wrapper would execute during ordinary Banjo movement. It
does not prove the backpack model is correct, incorrect, or interpolated.
No production fix and no diagnostic change are made here.

### Corrected section-113 zero-hit arbitration

The second run was not a repeat of the old section-456 filter. It used
`overlay_index == 113` and `entry_index == 3`, but the selected seam is still
not observed. The generated `_babackpack_entrypoint_3` at `0x80084500` is a
synthetic syscall stub which calls `recomp_syscall_handler`; its encoded
identity is 113/3. The live generated caller at `0x80092710`, however, emits
`LOOKUP_FUNC(0x80084500)`. That host lookup can resolve directly to the loaded
`babackpack_entrypoint_3` at overlay-relative `0x80800320`, bypassing the
synthetic stub and therefore bypassing `recomp_syscall_handler`.

The capture supplies no actual handler arguments, decoded words, dispatch
table entry, or resolved target pointer: `LAST_FRAME=0`, the syscall hit count
is zero, and no dispatch/error stderr artifact was captured. Therefore the
exact zero-hit classification is:

```text
HANDLER_RUNTIME_ARGUMENTS: UNKNOWN (handler not observed)
HANDLER_RUNTIME_TABLE_VALUES: UNKNOWN (handler not observed)
ZERO_HIT_ROOT: SELECTED_SEAM_NOT_ON_LIVE_LOOKUP_PATH
113_3_IDENTITY: STATIC_SYSCALL_STUB_IDENTITY_ONLY
RUNTIME_WITNESS_CAPTURED: NO
```

This is not evidence that 113/3 decodes to another table row, and it is not
evidence of an animation, transform, or culling anomaly. Do not request
another identical handler-filter run. The handler seam is retained for
comparison, but the next discriminator is now the caller-local lookup witness
described below.

### Caller-local lookup witness implementation

The generated caller was resolved accurately: `func_80092444` contains the
guest callsite `0x80092710`, whose delay-slot setup supplies `a0` through `a3`
before `LOOKUP_FUNC(0x80084500)`. The new source-owned
`src/game/backpack_lookup_diag.cpp` uses Linux linker interposition around that
caller and `get_function`. It opens a thread-local scope only while
`func_80092444` runs, records only the requested `0x80084500` lookup inside
that scope, and delegates both the original caller and lookup unchanged. The
probe is default-off; no generated file and no global lookup result is
modified.

The status file now includes `CALLER_HOOK_INSTALLED`, caller hit count and
frame, first/last resolved target pointers, resolved symbol when it matches
`babackpack_entrypoint_3`, resolution-change status, context/register
arguments, and the `a0` instance candidate. It flushes at initialization,
first caller hit, first lookup result, every 60 lookup results, and shutdown.
Until a user capture reaches the caller, all runtime instance, flap, model,
transform, and interpolation fields remain `UNKNOWN`.

### Caller-local capture result (2026-09-01)

The first caller-local capture reached the wrapper 2,026 times, but recorded
zero requests for `0x80084500`:

```text
CALLER_FIRST_HIT=YES
CALLER_HIT_COUNT=2026
LOOKUP_HIT_COUNT=0
LOOKUP_RESOLVED_SYMBOL=UNKNOWN
REAL_TARGET_ARGUMENTS_CAPTURED=NO
RUNTIME_WITNESS_CAPTURED=NO
```

This distinguishes entry into `func_80092444` from execution of its internal
guest callsite. Static control flow shows that `0x80092710` is reached only
after the conditional at `0x800926EC`; a zero byte at
`MEM_W(a0, 0x50) + 0x11` returns directly to `0x8009271C`. The capture did
not record `a0` or those gate bytes, so the conditional value and actual
backpack instance remain unproven. A follow-up caller-entry witness now
records the first/last `a0`, the `+0x50` data pointer, and the `+0xA2`,
`+0x18`, and `+0x11` gate bytes. It remains diagnostic-only and does not add
another global lookup hook or alter generated code.

### Updated caller-entry gate capture (2026-09-01)

The subsequent run produced 1,299 caller-wrapper hits and again produced no
lookup request for `0x80084500`. The artifact does not contain the requested
per-hit gate distributions: the witness CSV has only its header, and the
status file persists only first/last caller scalars plus the first gate tuple.

```text
CALLER_HIT_COUNT=1299
CALLER_FIRST_A0=0x801cb580
CALLER_LAST_A0=0x801c6580
CALLER_FIRST_T6=0x801cb94c
CALLER_FIRST_GATE_A2=0x00
CALLER_FIRST_GATE_18=0x00
CALLER_FIRST_GATE_11=0x01
LOOKUP_HIT_COUNT=0
SAMPLE_COUNT=0
ANOMALY_COUNT=0
```

Therefore no population-level skip counts, pointer clusters, or dominant path
can be promoted from this run. The first recorded call does establish the
branch `+A2=0` → `0x80092474` → `func_800A89F8` → `func_800A4C68` /
`func_800A940C` → `+18=0` → `0x8009271C`; it does not reach the later
`+11` gate or the `0x80092710` lookup. The `+0x50` value is used by the
caller as a per-object secondary state pointer, but its concrete structure
type and backpack ownership remain unproven.

This closes the 113/3 lookup as an observed path for this reproduction, while
leaving other `func_80092444` populations unclassified. No additional hook or
identical manual run is justified by this artifact.

## Unresolved transform path

Because the selected hook never ran, all witness-stage fields remain
unresolved:

```text
BACKPACK_FLAP_GEO_NODE: UNKNOWN
BACKPACK_FLAP_GEO_NODE_TYPE: UNKNOWN
BACKPACK_FLAP_DL: UNKNOWN
BACKPACK_FLAP_TRANSFORM_FUNCTION: UNKNOWN
BACKPACK_FLAP_DRAW_FUNCTION: babackpack_entrypoint_3 -> func_800DE448 (static boundary; runtime unmeasured)
BACKPACK_FLAP_MATRIX_STACK_PATH: UNKNOWN
BACKPACK_FIRST_BAD_STAGE: NO_RUNTIME_WITNESS
BACKPACK_FIRST_BAD_FRAME: UNKNOWN
BACKPACK_FIRST_BAD_VALUE: UNKNOWN
PREVIOUS_GOOD_VALUE: UNKNOWN
NEXT_GOOD_VALUE: UNKNOWN
BACKPACK_USES_INTERPOLATION: UNKNOWN
BACKPACK_INTERPOLATION_TAG: UNKNOWN
CURRENT_MATRIX_PRESENT: UNKNOWN
PREVIOUS_MATRIX_PRESENT: UNKNOWN
INTERPOLATED_MATRIX_PRESENT: UNKNOWN
PREVIOUS_CURRENT_MATRIX_MIX: UNKNOWN
BANJO_TRANSFORM_TAGGING_ANALOG: NOT_ASSESSED
TOOIE_MISSING_EQUIVALENT_TAG: UNKNOWN
TOOIE_INCORRECT_TAG: UNKNOWN
```

The Mumbo/Kazooie comparison is likewise deferred until the asset-specific
boundary is reached:

```text
MUMBO_USES_BACKPACK_TRANSFORM_FUNCTION: NOT_APPLICABLE
KAZOOIE_USES_BACKPACK_TRANSFORM_FUNCTION: NOT_APPLICABLE
MUMBO_USES_SAME_MATRIX_STACK_PATH: UNKNOWN
KAZOOIE_USES_SAME_MATRIX_STACK_PATH: UNKNOWN
```

## Diagnostic

`TOOIE_DIAG_BACKPACK_FLAP=1` is default OFF. A Tooie source-layer wrapper
records a bounded 256-sample pre/post controller ring and conservatively flags
non-finite values or an impossible `+0x170` single-update jump above `10000`.
It flushes `/tmp/tooie-backpack-flap-witness.csv` and
`/tmp/tooie-backpack-flap-anomalies.txt` on first hit, every 60 completed
updates, anomaly, or shutdown. It also writes
`/tmp/tooie-backpack-flap-status.txt` immediately when initialization detects
the flag, on the first hook hit, and at each periodic/anomaly/shutdown flush.
The status marker records initialization, PID, first-hit frame/callsite,
update count, bounded sample count, anomaly count, last frame, and flush
reason. Update count is incremented on hook entry, so a crash inside the
generated function still leaves evidence that the hook was reached. It does
not change animation, interpolation, culling, model data, or GPU behavior,
and it does not edit `RecompiledFuncs/**`.

## Implementation checkpoint

Static mapping is `babackpack_entrypoint_3` at overlay section `113`,
entrypoint `3`, with section-relative offset `0x320` (the old address-map key
was `0x80800320`). Its role is a narrow asset draw/state boundary: `ctx->r4`
is the backpack instance, it calls the asset-specific helper,
`babackpack_entrypoint_1`, and then `func_800DE448`.
`SYSCALL_MAPPING_CONFIDENCE: HIGH`.

The old section-90 `bsbflap_update` wrapper and the `0x80800320` address-map
wrapper are removed. The direct syscall probe remains around the resolved
`entry.entrypoints[entry_index]` call in `recomp_syscall_handler`, but the
caller-local lookup probe is the live-path discriminator. Therefore
`DIRECT_DISPATCH_PATH_INSTRUMENTED: YES`,
`CALLER_LOOKUP_PATH_INSTRUMENTED: YES`, `SYSCALL_HOOK_INSTALLED: YES`,
`HOOK_RUNTIME_REACHABILITY: UNPROVEN`, and
`OTHER_SYSCALLS_BEHAVIOR_CHANGED: NO`. Both probes remain default-off and
delegate their original functions unchanged.

The status file now reports the direct seam fields and the caller-local fields
`CALLER_HOOK_INSTALLED`, `CALLER_FIRST_HIT`, `CALLER_HIT_COUNT`, resolved target
pointers, symbol, resolution-change status, target arguments, and the first
caller `a0`/`+0x50` data pointer with the `+0xA2`, `+0x18`, and `+0x11` gate
bytes. The existing
bounded sampler and anomaly detector remain active at the syscall boundary,
but its state offsets are not yet proven to be flap-state fields. Consequently:

```text
RUNTIME_FLAP_OWNER: UNKNOWN until live hit
RUNTIME_FLAP_STATE_SOURCE: backpack instance at ctx->r4; exact child state UNKNOWN
ANOMALY_DETECTOR_ACTIVE_ON_NEW_HOOK: YES
ANOMALY_DETECTOR_NOW_HAS_REAL_TRANSFORM_DATA: NO
BACKPACK_USES_INTERPOLATION: UNKNOWN
BANJO_TRANSFORM_TAGGING_ANALOG: NOT_ASSESSED
```

## Validation

```text
ninja -C build TooieRecompiled -j2                 PASS
./build/window_display_test                        PASS (38/38)
./build/tintro_projection_policy_test              PASS (7/7)
git diff --check                                   PASS
```

The implementation builds and the focused tests pass. The first zero-hit run
proved the filter was incorrectly hardcoded to overlay 456 (`chblackeye`),
while the ROM syscall at `0x80084500` decodes to overlay 113 (`babackpack`),
entrypoint 3. The corrected second run proved that this identity is not
observed at the selected handler seam; the static caller uses a direct
`LOOKUP_FUNC` path. The first caller-local run proved entry into
`func_80092444`, but not execution of its conditional `0x80084500` lookup;
the new gate witness is required before the backpack instance can be claimed.
Do not implement a production fix from these captures.

```sh
cd /home/wesley/projetos/N64-Recomp/TooieRecomp && \
rm -f /tmp/tooie-backpack-flap-* && \
TOOIE_DIAG_BACKPACK_FLAP=1 ./build/TooieRecompiled
```

Artifact: the next user capture will create `/tmp/tooie-backpack-flap-*`.

`BACKPACK_ROOT_CLASS: LIVE_CALLER_REACHABILITY_UNPROVEN`;
`SHARED_THREE_CASE_ROOT: UNKNOWN`.

## Caller population diagnostic repair (2026-09-01)

The existing `func_80092444` source-layer wrapper was repaired without adding
another guest, syscall, GeoCmd, or renderer hook. Each completed caller hit now
produces `/tmp/tooie-backpack-flap-caller.csv` with `HIT_INDEX`, `FRAME`,
`A0`, `DATA_PTR_PLUS_50`, the three gate bytes, a deterministic branch class,
lookup eligibility/execution, and the request/resolved target when observed.
The row is finalized at caller exit so a lookup callback can be associated
with the same hit. The first row is flushed immediately; subsequent rows are
flushed every 60 hits and at shutdown.

The status file now includes bounded unique-pointer counts (4,096 entries per
set plus overflow attempts), all nonzero values for the three byte
histograms, branch-class counts, skip counts for `+A2`, `+0x18`, and `+0x11`,
lookup-eligible/executed counts, and the dominant branch class. The static
gate map is recorded in `func-80092444-control-flow.txt`.

```text
CALLER_DIAGNOSTIC_REPAIRED: YES
BRANCH_CLASSIFICATION_IMPLEMENTED: YES
PER_HIT_CLUSTER_KEYS_CAPTURED: YES
CSV_FIRST_HIT_PERSISTENCE: YES
CSV_PERIODIC_PERSISTENCE: YES
STATUS_HISTOGRAMS_IMPLEMENTED: YES
STATIC_CONTROL_FLOW_MAP_COMPLETE: YES
DIAGNOSTIC_DEFAULT_OFF: YES
```

Validation: `cmake --build build --target TooieRecompiled -j2` PASS;
`window_display_test` PASS (38/38); `tintro_projection_policy_test` PASS
(7/7); `git diff --check` PASS. No runtime capture has been made after this
repair, so no population, backpack cluster, or transform conclusion is
claimed yet.

## Real render path (2026-09-01) — model-side escalation

Do not restart from `func_80092444`. That caller is generic object/controller
traffic. Latest capture: 1503/1503 `PATH_A2_ZERO_RETURN_18`,
`LOOKUP_ELIGIBLE_COUNT=0`, `LOOKUP_EXECUTED_COUNT=0`.
`OLD_BABACKPACK_LOOKUP_HYPOTHESIS: REFUTED_FOR_THIS_RUNTIME_PATH`.

MATCHED overlay `src/overlays/ba/ckpack.c` / `backpack.h` (Mr-Wiseguy/banjo-tooie):

| FUNCTION | ADDRESS/OFFSET | CALLERS | CALLEES | ROLE | MODEL_RELEVANCE | EVIDENCE |
| --- | --- | --- | --- | --- | --- | --- |
| `babackpack_get_size` | overlay 113 | playerstate alloc | none | OTHER | none | returns `sizeof(BaBackpack)=0x18` |
| `babackpack_entrypoint_9` | overlay 113 | player init | `set_state(1)` | INITIALIZATION | clears models | MATCHED C |
| `babackpack_set_state` | overlay 113 | body-state overlays (`bsbansack`, etc.) | `func_8080011C`, preload | STATE + MODEL/ASSET LOAD | HIGH | state 2 → asset `0x621` |
| `func_8080011C_babackpack` | overlay 113 | `set_state` | `800DBFF8`, `800D674C`, `80100D24` | MODEL/ASSET LOAD | HIGH | caches model, creates attachment |
| `babackpack_entrypoint_1` | overlay 113 | draw/load | `func_800D674C(unk0)` | MODEL/ASSET LOAD | HIGH | model_bin getter |
| `babackpack_entrypoint_2` | overlay 113 | update | `800DF41C` / `800DF818` | UPDATE / ANIMATION | MEDIUM | not DL submit |
| `babackpack_entrypoint_3` | `0x80800320` | player draw | `800DF738`, `800DF818`, `800DF720`, `800DF41C`, **`800DE448`** | DRAW | HIGH | actual visible submit |
| `func_8080021C_babackpack` | overlay 113 | draw | `80100A74` | MODEL PART CONTROL | MEDIUM | attachment ping-pong `unk9` |
| `babackpack_entrypoint_5/6` | overlay 113 | unknown | `800DBEFC` | OTHER | LOW | palette/slot write |
| `babackpack_entrypoint_8` | overlay 113 | teardown | `set_state(0)` | STATE | none | |

Overlay 113 does own the visual mesh as an attached model, not as Banjo's
body GeoList. Code-overlay identity is still not equivalent to Banjo player
mesh ownership.

```text
BABACKPACK_ACTUAL_DRAW_ENTRYPOINT: babackpack_entrypoint_3
BABACKPACK_MODEL_LOAD_FUNCTION: babackpack_set_state / func_8080011C_babackpack / babackpack_entrypoint_1
BABACKPACK_MODEL_RESOURCE_ID: 0x621 (primary visible); also 0x625, 0x60A, 0x61B, 0x831, state-7 preload
BACKPACK_MODEL_POINTER_SOURCE: func_800D674C(self->backpack->unk0)
BACKPACK_GEOMETRY_OWNER: attached babackpack model_bin, submitted through common func_800DE448
MODEL_RESOURCE_CONFIDENCE: HIGH (MATCHED overlay C)
MODEL_RENDER_ENTRY: func_800DE448 → func_800DE498
GEOCMD_TRAVERSAL_FUNCTION: func_800DE498
CHILD_NODE_DISPATCH_FUNCTION: func_800E2AA4 (siblings 800E2C54 / 800E2E48)
MODEL_MATRIX_BUILD_FUNCTION: func_800E1C00 / func_800E204C
CHILD_MATRIX_COMPOSE_FUNCTION: func_800E2180
MATRIX_STACK_PUSH: G_MTX DA380002 in func_800E2AA4 then dual G_MTX in func_800E2180
MATRIX_STACK_POP: G_MTX D8380002 after func_800E2260
DL_SUBMISSION_FUNCTION: func_800E2180 (G_MTX prev/curr + DE00 via 800AF5F0/800AF5FC)
BACKPACK_FLAP_PART_ID: UNKNOWN
BACKPACK_FLAP_GEO_NODE: UNKNOWN
BACKPACK_FLAP_GEO_NODE_TYPE: dual-mtx GeoCmd2 analog (class), instance UNKNOWN
BACKPACK_FLAP_DL: UNKNOWN
PART_ID_CONFIDENCE: LOW
BANJO_STRUCTURAL_ANALOG: YES
BANJO_ANALOG_FUNCTION: func_803387F8 + set_model_matrix_group
BANJO_ANALOG_NODE_TYPE: GeoCmd2 bone
TOOIE_EQUIVALENT_PATH: func_800E2AA4 → func_800E2180
TOOIE_MISSING_TAGGING_EQUIVALENT: YES
TOOIE_INCORRECT_TAGGING_EQUIVALENT: NO
PRECEDENT_RELEVANCE: HIGH
MUMBO_SAME_NODE_CLASS: UNKNOWN
KAZOOIE_SAME_NODE_CLASS: UNKNOWN
MUMBO_SAME_TRANSFORM_FUNCTION: UNKNOWN
KAZOOIE_SAME_TRANSFORM_FUNCTION: UNKNOWN
SHARED_THREE_CASE_ROOT_PLAUSIBILITY: HIGH
RECOMMENDED_RUNTIME_WITNESS_SEAM: func_800E2180 filtered by backpack model_bin from func_800DE448
FILTER_IDENTITY: func_800D674C asset in {0x621,0x625,0x60A,0x61B,0x831} → DE448 5th arg → E2180 in that draw
FILTER_CONFIDENCE: MEDIUM
OLD_CALLER_DIAGNOSTIC_RETIRED: YES
BRANCH_DOUBLE_COUNT_FIXED: YES
NEW_MODEL_NODE_WITNESS_IMPLEMENTED: YES
CANDIDATE_IMPLEMENTED: NO
```

The flap node is not guessed from proximity. The witness records every dual-mtx
node under the backpack model so a later capture can identify the moving part.

Diagnostic default OFF: `TOOIE_DIAG_BACKPACK_FLAP=1`. Wrappers:
`--wrap=func_800D674C`, `--wrap=func_800DE448`, `--wrap=func_800E2180`.
Syscall 113/3 and `func_80092444`/`get_function` wrappers removed.

Validation (this round): `TooieRecompiled` PASS; `window_display_test` PASS
(38/38); `tintro_projection_policy_test` PASS (7/7); `git diff --check` PASS.
No user capture yet. `RUNTIME_WITNESS_CAPTURED: NO`.

## Stage-counter capture (2026-09-01) — filter stops before DE448

The fresh capture exercised the diagnostic, but no fixed backpack resource
identity was observed. The complete stage snapshot is:

```text
BABACKPACK_ENTRY3_HITS=0
ASSET_RESOLVE_HITS=28639
ASSET_0621_HITS=0
ASSET_0625_HITS=0
ASSET_060A_HITS=0
ASSET_061B_HITS=0
ASSET_0831_HITS=0
ASSET_OTHER_HITS=28639
DE448_TOTAL_HITS=17139
DE448_BACKPACK_FILTER_HITS=0
DE498_TOTAL_HITS=17955
DE498_BACKPACK_FILTER_HITS=0
E2AA4_TOTAL_HITS=0
E2AA4_BACKPACK_FILTER_HITS=0
E2180_TOTAL_HITS=1103
E2180_BACKPACK_FILTER_HITS=0
E2180_SAMPLE_WRITTEN_COUNT=0
E2180_REJECT_NO_ACTIVE_MODEL_CONTEXT=1103
E2180_REJECT_MODEL_ID_MISMATCH=0
E2180_REJECT_NO_NODE_CONTEXT=0
E2180_REJECT_UNEXPECTED_NODE_TYPE=0
E2180_REJECT_OTHER=0
FILTER_SCOPE_OVERFLOW_EVENTS=0
SAMPLE_COUNT=0
ANOMALY_COUNT=0
```

The last proven raw stage is D674C asset resolution; there is no proven
backpack-filtered stage. The first missing filtered stage is the D674C
asset-ID/model-bin association, before DE448. The E2180 calls are unrelated
dual-matrix traffic under this filter: all 1,103 see no active model scope.
The zero E2AA4 count also means the observed E2180 traffic cannot be used to
claim the backpack's E2AA4 path.

This is not a nesting failure: the repaired stack recorded zero overflow
events. The capture exposed an instrumentation limitation instead: the
existing D674C probe retained only the five presumed IDs and discarded the
actual nonmatching argument values. The existing probe was extended with a
bounded top-frequency observation table for nonmatching asset IDs and with
`DRAW_CONTEXT_MAX_DEPTH`/`DRAW_CONTEXT_STACK_OVERFLOW` status fields. No new
hook, generated file, production transform, or interpolation behavior was
added.

Because no E2180 sample exists, node clustering, flap identification, matrix
pairing, anomaly causality, and Banjo/Mumbo/Kazooie runtime equivalence remain
unknown. No production candidate is authorized.

## D674C nonmatching-resource capture (2026-09-02)

The follow-up capture populated the repaired observer, but still produced no
backpack-filtered draw or node witness:

```text
ASSET_RESOLVE_HITS=44620
ASSET_FIXED_ID_HITS=0 (0x621/0x625/0x60A/0x61B/0x831)
ASSET_OTHER_HITS=44620
ASSET_OTHER_UNIQUE_SLOTS=32
ASSET_OTHER_EVICTIONS=18095
DE448_TOTAL_HITS=28006
DE448_BACKPACK_FILTER_HITS=0
DE498_TOTAL_HITS=28892
DE498_BACKPACK_FILTER_HITS=0
E2AA4_TOTAL_HITS=0
E2180_TOTAL_HITS=1167
E2180_BACKPACK_FILTER_HITS=0
E2180_REJECT_NO_ACTIVE_MODEL_CONTEXT=1167
DRAW_CONTEXT_MAX_DEPTH=1
DRAW_CONTEXT_STACK_OVERFLOW=NO
SAMPLE_COUNT=0
ANOMALY_COUNT=0
```

The retained pairs were the 32 `ASSET_OTHER_XX_ID`/`MODEL_BIN` rows in the
status artifact. They are bounded top-frequency slots, not a complete unique
set; the evictions prove additional values occurred. Per-pair first/last
frame fields were not captured, and the run was not frame-scoped, so it does
not prove a walking-dominant set or identify Banjo, backpack, snout, Mumbo, or
Kazooie ownership.

Static reconciliation establishes that D674C's input is a resource-table key
(the `babackpack` object field supplies it), while its return is the loaded
resource/model pointer subsequently passed as DE448's fifth argument. The
`0x621`/`0x625` values are valid `babackpack` setup keys, but are not a
universal runtime identity. The observed `0x1cfa -> 0x80321370` pair is not
ownership proof. No E2AA4/E2180 sample exists; no production fix is
authorized.
