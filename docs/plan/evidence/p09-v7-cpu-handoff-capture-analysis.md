---
title: "P09 V7 CPU handoff capture analysis — target filter gate"
description: "Fresh V7 status heartbeat proves activation and raw hook reachability, but all target correlation counts are zero and the bounded CSV is absent."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-05
tags: [tooie-recomp, p09, feather, v7, cpu-handoff, capture]
---

# P09 V7 CPU handoff capture analysis

## Historical pre-repair analysis

The requested analysis-only pass found no fresh V7 runtime pair at the
implementation's absolute output paths:

```text
/tmp/tooie-kazooie-feather-v7-cpu-handoff.csv: MISSING
/tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt: MISSING
```

The files present in `/tmp` are older V6/V6B/V6C captures (their headers carry
the prior schemas and their timestamps predate this analysis). They were not
mixed into the V7 chain. No game was run to recreate the capture.

```text
CAPTURE_POSTDATES_CURRENT_EXECUTABLE: UNKNOWN (no V7 artifact)
V7_SCHEMA: NOT_OBSERVED
V7_VERSION: NOT_OBSERVED
CAPTURE_FILES_MUTUALLY_CONSISTENT: NO (required pair absent)
CAPTURE_USABLE: NO
CAPTURE_COMPLETE_FOR_CAUSAL_DECISION: NO
MANDATORY_TARGET_COVERAGE_COMPLETE: NO
OWN_103_COMPLETE: NO
OWN_104_COMPLETE: NO
OWN_105_COMPLETE: NO
JOIN_592_COMPLETE: NO
JOIN_54C_COMPLETE: NO
JOIN_564_COMPLETE: NO
POST_APPEND_FACE_INDICES_VALID: UNKNOWN
POST_APPEND_VERTEX_IDENTITY_VALID: UNKNOWN
POST_APPEND_WORLDINDEX_VALID: UNKNOWN
POST_APPEND_VELOCITY_ZERO: UNKNOWN
FACE_SLICE_CONTENT_EQUALS_APPEND: UNKNOWN
FACE_SLICE_MISMATCH_COUNT: UNKNOWN
UPLOAD_POSITION_MATCH: UNKNOWN
UPLOAD_VELOCITY_MATCH: UNKNOWN
UPLOAD_WORLDINDEX_MATCH: UNKNOWN
UPLOAD_MATRIX_MATCH: UNKNOWN
UPLOAD_FACEINDICES_MATCH: UNKNOWN
CPU_UPLOAD_PAYLOAD_MISMATCH_COUNT: UNKNOWN
JOIN_592_PROVENANCE_STATUS: INCOMPLETE
JOIN_54C_PROVENANCE_STATUS: INCOMPLETE
JOIN_564_PROVENANCE_STATUS: INCOMPLETE
ALL_RIGHT_PLUME_CONNECTING_GEOMETRY_PROVEN: NO
UNMAPPED_WITH_PREVIOUS_FRAME_COUNT: UNKNOWN
PROCESS_SKIPPED_BUT_LERP_UPLOAD_SELECTED_COUNT: UNKNOWN
TRANSFORMPROCESSOR_PREDICATE_MISMATCH_TRIGGERED_FOR_TARGET: UNKNOWN
APPEND_TO_HANDOFF_EQUAL: UNKNOWN
HANDOFF_TO_RENDER_SELECTION_EQUAL: UNKNOWN
APPEND_TO_RENDER_SELECTION_EQUAL: UNKNOWN
FIRST_DATA_DIVERGENCE: UNKNOWN
FIRST_DIVERGENCE_TARGET: NONE
CPU_HANDOFF_STATUS: UNKNOWN
FIRST_UNEXPLAINED_LAYER: UNKNOWN
ROOT_CLASS: MORE_EVIDENCE_REQUIRED
ROOT_CONFIDENCE: LOW
TARGET_RSP_MODIFY_COMMANDS_STATICALLY_POSSIBLE: UNKNOWN
NEXT_SHADER_OR_RENDER_SEAM: NOT_APPLICABLE_PENDING_V7_CAPTURE
NEXT_SINGLE_ACTION: run one bounded V7 capture and retain both the CSV and status pair
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
P09_KNOWLEDGE_PROMOTED_TO_RE_DOCS: NO
DIFF_CHECK: PASS
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```

The original analysis-only pass was non-mutating: `git diff --check` passed in
TooieRecomp and RT64, and no game, production behavior, GPU path, commit, or
push was changed. The activation repair below is the subsequent diagnostic-only
change.

## V7 runtime activation repair (2026-09-05)

The missing pair was traced to artifact lifecycle rather than feather data. The
only enable reader is `RT64::p09V7Truthy()` in
`BanjoRecomp/lib/rt64/src/hle/rt64_workload.cpp`; `p09V7Enabled()` caches that
value once and initializes the status file on the RT64 startup/first-enabled path. No Tooie-side
reader, secondary-sway dependency, or duplicated header-static writer exists.
Before this repair, status creation was reachable only from target APPEND or
INITIAL_HANDOFF, so a zero-target run produced no status and no CSV.

The minimal repair initializes the status heartbeat at enable with schema,
version, `ENABLED=1`, completeness, raw hook counters, and target counters. It
adds atomic raw APPEND/fullSync/render-selection counters before target or
record filtering, plus tracked counters after filtering. CSV creation remains
lazy and bounded. Existing APPEND, handoff, render-selection, join mappings,
and rendering behavior are unchanged. A normal-exit `atexit` finalizer now
flushes and closes the CSV; status writes close their file on every update.

```text
V7_STATUS_PATH: /tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt
V7_APPEND_CSV_PATH: /tmp/tooie-kazooie-feather-v7-cpu-handoff.csv
V7_HANDOFF_CSV_PATH: same combined CSV (INITIAL_HANDOFF_* records)
V7_RENDER_SELECTION_CSV_PATH: same combined CSV (RENDER_SELECTION records)
PREVIOUS_ANALYSIS_EXPECTED_CORRECT_FILENAMES: YES
ENV_ENABLE_PATH_VALID: YES
V7_HAS_HIDDEN_SECONDARY_SWAY_DEPENDENCY: NO
V7_HAS_OTHER_HIDDEN_ENABLE_DEPENDENCY: NO
V7_STATUS_CREATED_AT_ENABLE: YES
V7_RAW_HOOK_COUNTERS_IMPLEMENTED: YES
STATUS_FILE_CREATION_POLICY: eager during RT64 V7 startup initialization (also guarded by the first enabled query); rewritten and closed at enable and each lifecycle event
CSV_FILE_CREATION_POLICY: lazy on first accepted target APPEND/handoff/render record; flushed at each record and closed at normal exit
ZERO_TARGET_CAN_PRODUCE_NO_CSV: YES (status disambiguates)
V7_STATUS_PERIODICALLY_FLUSHED: YES (close per write)
V7_CSV_PERIODICALLY_FLUSHED: YES
V7_SHUTDOWN_FINALIZER_RELIABLE: YES (normal process exit via atexit)
V7_DIAGNOSTIC_STATE_SINGLETON: YES
DUPLICATED_STATIC_STATE_POSSIBLE: NO
CROSS_TU_STATE_LINKAGE_VALID: YES
FINAL_BINARY_CONTAINS_V7_SCHEMA_STRING: YES
FINAL_BINARY_CONTAINS_V7_ENV_STRING: YES
FINAL_BINARY_LINKS_V7_HOOKS: YES
BUILD_EXECUTABLE_PATH: /home/wesley/projetos/N64-Recomp/TooieRecomp/build/TooieRecompiled
RUN_TOOIE_EXECUTABLE_PATH: $TOOIE_BUILD_DIR/TooieRecompiled (default resolves to the same absolute path)
DIRECT_BUILD_BINARY_IS_CURRENT: YES
RUN_TOOIE_SCRIPT_USES_CURRENT_BUILD: YES
STALE_OR_DIFFERENT_BINARY_LAUNCH_POSSIBLE: YES if TOOIE_BUILD_DIR is overridden; NO for the authoritative direct path
V7_RUNTIME_ACTIVATION_REPAIRED: YES
REPAIR_SCOPE: RT64 V7 enable-time status initialization, raw counters, and normal-exit CSV finalizer only
NEXT_CAPTURE_CAN_DISTINGUISH_ENABLE_FAILURE: YES
NEXT_CAPTURE_CAN_DISTINGUISH_HOOK_FAILURE: YES
NEXT_CAPTURE_CAN_DISTINGUISH_FILTER_FAILURE: YES
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
PROJECTION_TEST: PASS (7/7)
TOOIE_DIFF_CHECK: PASS
RT64_DIFF_CHECK: PASS
USER_MANUAL_TEST_REQUIRED_NOW: YES
USER_TEST_COMMAND: cd /home/wesley/projetos/N64-Recomp/TooieRecomp && rm -f /tmp/tooie-kazooie-feather-v7-cpu-handoff.csv /tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt && TOOIE_DIAG_FEATHER_HANDOFF_V7=1 ./build/TooieRecompiled
NEXT_SINGLE_ACTION: run the one bounded V7 capture command above, then retain both artifacts
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## Fresh runtime V7 capture — state C (2026-09-05)

The requested fresh launch produced the status heartbeat at the exact V7
status path, but no combined CSV. The heartbeat proves that the enable path
ran and that the three raw diagnostic hook sites were reached. It also proves
that no bounded target record was admitted: `OBSERVED_TARGET_MASK=0x00`, all
six target record counts are zero, and every tracked count is zero.

```text
STATUS_FILE_PRESENT: YES
SCHEMA: P09_FEATHER_HANDOFF_V7
VERSION: 1
ENABLED: 1
RAW_APPEND_HOOK_COUNT: 3104629
RAW_FULLSYNC_HOOK_COUNT: 1799
RAW_RENDER_SELECTION_HOOK_COUNT: 3929
TRACKED_APPEND_COUNT: 0
TRACKED_HANDOFF_COUNT: 0
TRACKED_RENDER_SELECTION_COUNT: 0
CSV_PRESENT: NO
V7_ACTIVATION_VALID: YES
V7_RUNTIME_STATE: C
EARLIEST_FAILED_GATE: target/filter correlation (raw hooks reached; no target DL admitted)
CAPTURE_POSTDATES_CURRENT_EXECUTABLE: YES (status mtime 05:19:05; executable mtime 05:16:07, 2026-09-05 -0300)
CAPTURE_FILES_MUTUALLY_CONSISTENT: NO (CSV missing)
CAPTURE_USABLE: NO
CAPTURE_COMPLETE_FOR_CAUSAL_DECISION: NO
MANDATORY_TARGET_COVERAGE_COMPLETE: NO
OWN_103_COMPLETE: NO (0/3 stages)
OWN_104_COMPLETE: NO (0/3 stages)
OWN_105_COMPLETE: NO (0/3 stages)
JOIN_592_COMPLETE: NO (0/3 stages)
JOIN_54C_COMPLETE: NO (0/3 stages)
JOIN_564_COMPLETE: NO (0/3 stages)
POST_APPEND_FACE_INDICES_VALID: UNKNOWN
POST_APPEND_WORLDINDEX_VALID: UNKNOWN
POST_APPEND_VELOCITY_ZERO: UNKNOWN
FACE_SLICE_CONTENT_EQUALS_APPEND: UNKNOWN
FACE_SLICE_MISMATCH_COUNT: UNKNOWN
UPLOAD_POSITION_MATCH: UNKNOWN
UPLOAD_WORLDINDEX_MATCH: UNKNOWN
UPLOAD_MATRIX_MATCH: UNKNOWN
UPLOAD_FACEINDICES_MATCH: UNKNOWN
CPU_UPLOAD_PAYLOAD_MISMATCH_COUNT: UNKNOWN
JOIN_592_PROVENANCE_STATUS: INCOMPLETE
JOIN_54C_PROVENANCE_STATUS: INCOMPLETE
JOIN_564_PROVENANCE_STATUS: INCOMPLETE
ALL_RIGHT_PLUME_CONNECTING_GEOMETRY_PROVEN: NO
UNMAPPED_WITH_PREVIOUS_FRAME_COUNT: UNKNOWN
PROCESS_SKIPPED_BUT_LERP_UPLOAD_SELECTED_COUNT: UNKNOWN
TRANSFORMPROCESSOR_PREDICATE_MISMATCH_TRIGGERED_FOR_TARGET: UNKNOWN
APPEND_TO_HANDOFF_EQUAL: UNKNOWN
HANDOFF_TO_RENDER_SELECTION_EQUAL: UNKNOWN
APPEND_TO_RENDER_SELECTION_EQUAL: UNKNOWN
FIRST_DATA_DIVERGENCE: UNKNOWN (no target witness reached)
FIRST_DIVERGENCE_TARGET: NONE_OBSERVED
FIRST_DIVERGENCE_GENERATION: NONE_OBSERVED
CPU_HANDOFF_STATUS: UNKNOWN
FIRST_UNEXPLAINED_LAYER: target/filter correlation gate
ROOT_CLASS: UNKNOWN (capture stopped before any causal data stage)
ROOT_CONFIDENCE: HIGH for the failed gate; NONE for a rendering root
TARGET_RSP_MODIFY_COMMANDS_STATICALLY_POSSIBLE: UNKNOWN (not reached; D-state triage prohibited)
NEXT_SHADER_OR_RENDER_SEAM: NOT_APPLICABLE_PENDING_TARGET_WITNESS
NEXT_SINGLE_ACTION: resolve why all six target DLs are filtered out, then rerun one bounded V7 capture retaining status and CSV
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
DIFF_CHECK: PASS (TooieRecomp and RT64)
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_PRODUCTION_FIX: YES
NO_COMMIT: YES
NO_PUSH: YES
```

Per the V7 stop rule, no V6/V6B/V6C rows were combined with this run and no
append, face-slice, upload, join, render-selection, or downstream renderer
claim is made. The next investigation must repair or explain target/filter
correlation before requesting causal coverage again.

## Fresh collision-safe V7 capture — state D (2026-09-05)

This is the first capture after the exact-identity admission repair. Both
artifacts are present, carry the V7 schema, and postdate the current executable.
The raw append/fullSync/render hooks ran, and the low-page resolver produced
11,740 candidate records across all six targets. Every candidate was rejected
because the independent live-`0x62B` model-bin bridge supplied `model_bin=0`;
therefore no candidate passed the full V6C identity equation. The V7 stop rule
applies: causal CPU-handoff analysis stops at exact validation.

```text
STATUS_FILE_PRESENT: YES
CSV_PRESENT: YES
SCHEMA: P09_FEATHER_HANDOFF_V7
VERSION: 1
ENABLED: 1
CAPTURE_POSTDATES_CURRENT_EXECUTABLE: YES (artifacts 2026-09-05 14:56; executable 09:53:45 -0300)
CAPTURE_USABLE: YES for activation/admission-failure analysis; NO for causal handoff analysis
RAW_APPEND_HOOK_COUNT: 4282916
RAW_FULLSYNC_HOOK_COUNT: 2284
RAW_RENDER_SELECTION_HOOK_COUNT: 4890
TARGET_CANDIDATE_COUNT: 11740 (OWN_103=712, OWN_104=1970, OWN_105=4392, JOIN_592=1830, JOIN_54C=2832, JOIN_564=4)
TARGET_VALIDATED_COUNT: 0
TARGET_REJECTED_AFTER_VALIDATION_COUNT: 11740
TARGET_534_VALIDATED: 0
TARGET_54C_VALIDATED: 0
TARGET_564_VALIDATED: 0
TARGET_57C_VALIDATED: 0
TARGET_592_VALIDATED: 0
TARGET_5AA_VALIDATED: 0
EXACT_TARGET_VALIDATION_RUNTIME_WORKING: NO (model-bin context unavailable)
UNVALIDATED_TARGET_USED_CAUSALLY: NO
TRACKED_APPEND_COUNT: 0
TRACKED_HANDOFF_COUNT: 0
TRACKED_RENDER_SELECTION_COUNT: 0
OBSERVED_TARGET_MASK: 0x00
V7_RUNTIME_STATE: D_EXACT_VALIDATION_BLOCKED
EARLIEST_FAILED_GATE: exact target validation (all candidates have model_bin=0, validation_reason=1)
MANDATORY_TARGET_COVERAGE_COMPLETE: NO
OWN_103_COMPLETE: NO (0/3 stages)
OWN_104_COMPLETE: NO (0/3 stages)
OWN_105_COMPLETE: NO (0/3 stages)
JOIN_592_COMPLETE: NO (0/3 stages)
JOIN_54C_COMPLETE: NO (0/3 stages)
JOIN_564_COMPLETE: NO (0/3 stages)
POST_APPEND_FACE_INDICES_VALID: UNKNOWN
POST_APPEND_WORLDINDEX_VALID: UNKNOWN
POST_APPEND_VELOCITY_ZERO: UNKNOWN
FACE_SLICE_CONTENT_EQUALS_APPEND: UNKNOWN
FACE_SLICE_MISMATCH_COUNT: UNKNOWN
UPLOAD_POSITION_MATCH: UNKNOWN
UPLOAD_WORLDINDEX_MATCH: UNKNOWN
UPLOAD_MATRIX_MATCH: UNKNOWN
UPLOAD_FACEINDICES_MATCH: UNKNOWN
CPU_UPLOAD_PAYLOAD_MISMATCH_COUNT: UNKNOWN
JOIN_592_PROVENANCE_STATUS: INCOMPLETE
JOIN_54C_PROVENANCE_STATUS: INCOMPLETE
JOIN_564_PROVENANCE_STATUS: INCOMPLETE
ALL_RIGHT_PLUME_CONNECTING_GEOMETRY_PROVEN: NO
UNMAPPED_WITH_PREVIOUS_FRAME_COUNT: UNKNOWN
PROCESS_SKIPPED_BUT_LERP_UPLOAD_SELECTED_COUNT: UNKNOWN
TRANSFORMPROCESSOR_PREDICATE_MISMATCH_TRIGGERED_FOR_TARGET: UNKNOWN
APPEND_TO_HANDOFF_EQUAL: UNKNOWN
HANDOFF_TO_RENDER_SELECTION_EQUAL: UNKNOWN
APPEND_TO_RENDER_SELECTION_EQUAL: UNKNOWN
FIRST_DATA_DIVERGENCE: UNKNOWN (no validated target witness)
FIRST_DIVERGENCE_TARGET: NONE_OBSERVED
FIRST_DIVERGENCE_GENERATION: NONE_OBSERVED
CPU_HANDOFF_STATUS: UNKNOWN
FIRST_UNEXPLAINED_LAYER: exact target validation bridge
ROOT_CLASS: UNKNOWN (capture stopped before any causal data stage)
ROOT_CONFIDENCE: HIGH for validation-gate failure; NONE for rendering root
TARGET_RSP_MODIFY_COMMANDS_STATICALLY_POSSIBLE: UNKNOWN (D-state triage prohibited)
NEXT_SHADER_OR_RENDER_SEAM: NOT_APPLICABLE_PENDING_EXACT_TARGET_WITNESS
NEXT_SINGLE_ACTION: repair the model-bin bridge to admit raw 0x62A models normalized to live 0x62B, then run one bounded V7 capture
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_PRODUCTION_FIX: YES
NO_COMMIT: YES
NO_PUSH: YES
```

The likely bridge defect is independently supported by the existing model
capture: raw getter rows include `RESOURCE_ID=0x62A`, while the live Kazooie
feather asset is normalized to `0x62B` only after model metadata inspection.
The new V7 bridge tests raw resource equality to `0x62B`, so it cannot publish
the model bin for those `0x62A` getter rows. This is a diagnostic admission
blocker, not evidence about face indices, uploads, joins, or shaders.

## Fresh runtime V7 capture — state E (2026-09-05 23:51 -0300)

The bounded launch after the model-identity repair produced both expected
artifacts and postdates the rebuilt executable. The artifacts are useful for
activation and admission diagnostics, but `CAPTURE_COMPLETE=0` and no exact
target entered the causal pipeline. The CSV contains 47,574 candidate rows
across five targets; every row has `validation_reason=1` and `model_bin=0x0`.
The status bridge counters are decisive: `MODEL_GETTER_RAW_62A_COUNT=0`,
`MODEL_GETTER_RAW_62B_COUNT=0`, `MODEL_GETTER_NORMALIZED_62B_COUNT=0`,
`MODEL_BIN_BRIDGE_ACCEPT_COUNT=0`, and `MODEL_BIN_BRIDGE_REJECT_COUNT=1786`.
Therefore this run did not exercise the raw-`0x62A` metadata normalization
path at all. The exact subcause of the 1,786 bridge rejects is not
distinguishable from this schema (for example, zero return or an unrecognized
resource before classification), but the observable failure is that no live
model-bin identity reached RT64.

```text
STATUS_FILE_PRESENT: YES
CSV_PRESENT: YES
SCHEMA: P09_FEATHER_HANDOFF_V7
VERSION: 1
ENABLED: 1
CAPTURE_POSTDATES_CURRENT_EXECUTABLE: YES (artifacts 23:51:07; executable 22:37:25, 2026-09-05 -0300)
CAPTURE_USABLE: YES for activation/admission diagnostics; NO for causal handoff analysis
CAPTURE_COMPLETE: 0
CAPTURE_COMPLETE_FOR_CAUSAL_DECISION: NO
RAW_APPEND_HOOK_COUNT: 5214884
RAW_FULLSYNC_HOOK_COUNT: 2489
RAW_RENDER_SELECTION_HOOK_COUNT: 5223
TRACKED_APPEND_COUNT: 0
TRACKED_HANDOFF_COUNT: 0
TRACKED_RENDER_SELECTION_COUNT: 0
OBSERVED_TARGET_MASK: 0x00
V7_RUNTIME_STATE: E_EXACT_VALIDATION_BLOCKED_NO_MODEL_IDENTITY
TARGET_CANDIDATE_COUNT: 47574 (OWN_105=10992, JOIN_54C=2064, JOIN_564=12200, OWN_104=13168, JOIN_592=9150, OWN_103=0)
TARGET_VALIDATED_COUNT: 0
TARGET_REJECTED_AFTER_VALIDATION_COUNT: 47574
MODEL_GETTER_RAW_62A_COUNT: 0
MODEL_GETTER_RAW_62B_COUNT: 0
MODEL_GETTER_NORMALIZED_62B_COUNT: 0
MODEL_BIN_BRIDGE_ACCEPT_COUNT: 0
MODEL_BIN_BRIDGE_REJECT_COUNT: 1786
MODEL_BIN_NONZERO_OBSERVED: NO
FULL_WIDTH_MODEL_BIN_OBSERVED: NO
MODEL_BIN_BRIDGE_RUNTIME_WORKING: NO
EXACT_TARGET_VALIDATION_RUNTIME_WORKING: NO (all candidates rejected with validation_reason=1 / model-bin unavailable)
UNVALIDATED_TARGET_USED_CAUSALLY: NO
MANDATORY_TARGET_COVERAGE_COMPLETE: NO
OWN_103_COMPLETE: NO (0/3 stages; candidates=0)
OWN_104_COMPLETE: NO (0/3 stages; candidates=13168)
OWN_105_COMPLETE: NO (0/3 stages; candidates=10992)
JOIN_592_COMPLETE: NO (0/3 stages; candidates=9150)
JOIN_54C_COMPLETE: NO (0/3 stages; candidates=2064)
JOIN_564_COMPLETE: NO (0/3 stages; candidates=12200)
POST_APPEND_FACE_INDICES_VALID: UNKNOWN
POST_APPEND_WORLDINDEX_VALID: UNKNOWN
POST_APPEND_VELOCITY_ZERO: UNKNOWN
FACE_SLICE_CONTENT_EQUALS_APPEND: UNKNOWN
FACE_SLICE_MISMATCH_COUNT: UNKNOWN
UPLOAD_POSITION_MATCH: UNKNOWN
UPLOAD_WORLDINDEX_MATCH: UNKNOWN
UPLOAD_MATRIX_MATCH: UNKNOWN
UPLOAD_FACEINDICES_MATCH: UNKNOWN
CPU_UPLOAD_PAYLOAD_MISMATCH_COUNT: UNKNOWN
JOIN_592_PROVENANCE_STATUS: INCOMPLETE
JOIN_54C_PROVENANCE_STATUS: INCOMPLETE
JOIN_564_PROVENANCE_STATUS: INCOMPLETE
ALL_RIGHT_PLUME_CONNECTING_GEOMETRY_PROVEN: NO
UNMAPPED_WITH_PREVIOUS_FRAME_COUNT: UNKNOWN
PROCESS_SKIPPED_BUT_LERP_UPLOAD_SELECTED_COUNT: UNKNOWN
TRANSFORMPROCESSOR_PREDICATE_MISMATCH_TRIGGERED_FOR_TARGET: UNKNOWN
APPEND_TO_HANDOFF_EQUAL: UNKNOWN
HANDOFF_TO_RENDER_SELECTION_EQUAL: UNKNOWN
APPEND_TO_RENDER_SELECTION_EQUAL: UNKNOWN
FIRST_DATA_DIVERGENCE: UNKNOWN (no exact target witness)
FIRST_DIVERGENCE_TARGET: NONE_OBSERVED
FIRST_DIVERGENCE_GENERATION: NONE_OBSERVED
CPU_HANDOFF_STATUS: UNKNOWN
FIRST_UNEXPLAINED_LAYER: model-bin bridge before exact target validation
ROOT_CLASS: UNKNOWN (capture stopped before causal data)
ROOT_CONFIDENCE: HIGH for bridge-gate failure; NONE for a rendering root
TARGET_RSP_MODIFY_COMMANDS_STATICALLY_POSSIBLE: UNKNOWN (D-state triage not reached)
NEXT_SHADER_OR_RENDER_SEAM: NOT_APPLICABLE_PENDING_EXACT_TARGET_WITNESS
NEXT_SINGLE_ACTION: instrument or otherwise resolve the 1786 model-bin bridge rejects, then run one bounded V7 capture retaining both artifacts
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_PRODUCTION_FIX: YES
NO_COMMIT: YES
NO_PUSH: YES
```

The candidate counts above come from the literal-`\\n` CSV records; the
per-target status counters are a bounded current-workload window and therefore
show only the latest `0x57C` candidate rows. No face, upload, join, matrix,
shader, or later-render-stage conclusion is authorized by this run.

## V7 durable-bin bridge repair — state F (2026-09-07)

The V7 model-context publisher now runs at the existing
`secondary_sway_register_durable_bin()` normalization seam. A successful
registration is the only authoritative V7 identity source: the seam proves
normalized resource `0x62B` from model metadata (magic `0xB`, gtype flag clear,
66 bones), stores the model pointer as a sign-extended guest `uint64_t`, records
its physical base, and publishes a context generation. Replacement or durable
slot eviction clears the prior matching context before the next publication.
The getter wrapper remains non-authoritative; when it observes a returned model it may feed the same durable seam, but it never admits, clears, or overrides V7 context. Its argument, zero-return, resource, and metadata reject
reasons are counted explicitly.

The RT64 V7 status now exposes durable registration total, normalized-`0x62B`
count, context publish/clear/generation counters, last guest/physical context,
player, getter call count, per-reason reject counts, and first reject reason.
The exact validation expression remains
`current_dl == physical(model_bin) + 0x2670 + logical_index * 8`; low-page
matching remains candidate prefilter only, and unvalidated candidates cannot
enter causal analysis. All six target paths remain statically reachable once a
published context is present. No game launch or capture was performed for this
repair, and no face/upload/join/render/synchronization logic changed.

Validation: `cmake --build build --target TooieRecompiled -j2` passed;
`./build/window_display_test` passed 38/38; `./build/tintro_projection_policy_test`
passed 7/7; Tooie and RT64 `git diff --check` passed; `graft build` passed.
The next capture is authorized only after reviewing this implementation and
must retain both V7 artifacts.

## Fresh V7 capture after durable-bin repair — state G (2026-09-07)

The user-generated artifacts are present and fresh: status mtime `2026-09-07 00:42:28 -0300`, CSV mtime `2026-09-07 00:42:26 -0300`, rebuilt executable mtime `2026-09-07 00:26:36 -0300`. Schema is `P09_FEATHER_HANDOFF_V7`, version `1`, and `ENABLED=1`. The files are internally usable for gate diagnosis, but `CAPTURE_COMPLETE=0` and `CAPTURE_COMPLETE_FOR_CAUSAL_DECISION=NO`, so this capture is not causally complete.

The durable bridge is working at runtime. `DURABLE_BIN_REGISTER_TOTAL=1449`, `DURABLE_BIN_NORMALIZED_62B=1297`, `V7_MODEL_CONTEXT_PUBLISH_COUNT=1297`, `V7_MODEL_CONTEXT_CLEAR_COUNT=0`, last normalized model `0x62B`, last guest bin `0xFFFFFFFF80229070`, and last physical base `0x229070`. Nonzero model identity is observed in the candidate CSV, and publication precedes target validation. Getter diagnostics report 2,898 wrapper calls: `GETTER_REJECT_ARG_FILTER=1449`, `GETTER_REJECT_RETURN_ZERO=0`, `GETTER_REJECT_RESOURCE_UNAVAILABLE=0`, `GETTER_REJECT_RAW_RESOURCE_OTHER=152`, `GETTER_REJECT_METADATA_UNAVAILABLE=0`, `GETTER_REJECT_METADATA_MISMATCH=0`, `GETTER_REJECT_OTHER=0`; the first reason is `GETTER_REJECT_ARG_FILTER`.

The CSV contains 5,296 unique `CANDIDATE` records and no `VALIDATED` records. Candidate-level validation rejects are 2,526 reason `1` (model bin zero) and 2,770 reason `2` (candidate DL differs from the full-address expected DL); no reason-2 row has equal addresses. Target candidate/reject counts are: `OWN_105_0x534` 1,879/1,879 (896 reason 1, 983 reason 2); `JOIN_DISTAL_0x54C` 1,906/1,906 (414/1,492); `OWN_104_0x57C` 1,216/1,216 (1,216/0); `OWN_103_0x5AA` 295/295 (0/295); `JOIN_DISTAL_0x564` 0/0; `JOIN_ROOT_0x592` 0/0. Status committed target counters remain zero because only exact-validated records are committed. `OBSERVED_TARGET_MASK=0x00`.

The causal stop gate therefore fires at exact target validation: `TARGET_CANDIDATE_COUNT>0` with `TARGET_VALIDATED_COUNT=0`. `RAW_APPEND_HOOK_COUNT=4706319`, `RAW_FULLSYNC_HOOK_COUNT=2423`, and `RAW_RENDER_SELECTION_HOOK_COUNT=5268` are reported for context, while tracked append/handoff/render counts are all zero. Face-slice, upload, join-provenance, render-selection, three-stage equality, and first-divergence fields are `NOT_EVALUATED`/`UNKNOWN` by contract. No downstream root is promoted. The next single action is to resolve the candidate-to-full-address admission mismatch (including why some candidates carry zero context) and then run one bounded capture again; do not inspect downstream causal stages until validation produces a nonzero exact count. This turn did not launch the game, modify code, use GPU readback, add synchronization, commit, or push.
## Fresh V7 capture after exact direct-DL admission repair — state H (2026-09-07)

The fresh artifacts are present and internally valid: status mtime 2026-09-07 07:13:42 -0300, CSV mtime 2026-09-07 07:12:53 -0300, and executable mtime 2026-09-07 01:00:44 -0300. The CSV begins SCHEMA=P09_FEATHER_HANDOFF_V7;VERSION=1, has 51 schema columns, and 1,842 well-formed diagnostic records. CAPTURE_COMPLETE=0 and CAPTURE_COMPLETE_FOR_CAUSAL_DECISION=NO; the capture is usable for the admission and bounded pipeline gates, but not for a closed causal root decision.

The exact direct target identity is working. The runtime reports DURABLE_BIN_REGISTER_TOTAL=1631, DURABLE_BIN_NORMALIZED_62B=1485, V7_MODEL_CONTEXT_PUBLISH_COUNT=1485, V7_MODEL_CONTEXT_CLEAR_COUNT=0, last guest model bin 0xFFFFFFFF80230500, and last physical base 0x230500. APPEND_WITH_VALID_MODEL_CONTEXT=4411004 and APPEND_EXACT_TARGET_MATCH=40124; the six exact counters are 0x534=11464, 0x54C=5732, 0x564=5732, 0x57C=5732, 0x592=5732, and 0x5AA=5732. The formula is physical(model_bin)+0x2670+logical*8, with the old low-page prefilter removed.

All six target bits are observed (OBSERVED_TARGET_MASK=0x3F). The CSV contains 448 accepted APPEND samples (128 for OWN_105_0x534, 64 for each other target) and 96 INITIAL_HANDOFF_OK samples (16 per target). Raw hooks are APPEND=5163225, FULLSYNC=2285, RENDER_SELECTION=4890; tracked counts are APPEND=40124, HANDOFF=448, and RENDER_SELECTION=32. Therefore mandatory exact target coverage is complete for the bounded sample. The three OWN targets have complete sampled append/handoff coverage; the three JOIN targets are observed but their provenance is unresolved.

The handoff comparator reports valid face range and face content for all 96 sampled INITIAL_HANDOFF_OK records: missing=0, overflow=0, and zero face-slice mismatches. Append records have well-formed corner/global/RSP/source triplets, nonzero resolved source addresses, and zero velocity vectors; the runtime invariant is VELOCITY_INVARIANT=ZERO. The first sampled world matrices compare bit-identically with the eight INITIAL_MATRIX records. However every handoff row carries byte_mismatch=3; this counter combines world/view-proj index, position/velocity, and matrix checks, and the capture does not emit a per-component breakdown. Thus face identity and velocity are proven for the sampled handoffs, while complete post-append world-index and vertex-payload equality remain unresolved; append-to-handoff full-payload equality is NO.

Join provenance is the causal completeness blocker. All 192 sampled JOIN APPEND records have expected_role_resolved=0 and expected_join_role=4294967295/4294967295/4294967295; all 48 sampled JOIN handoffs have unknown_join=1. This keeps CAPTURE_COMPLETE_FOR_CAUSAL_DECISION=NO. Render selection is recorded 32 times with selected_matrix_source=LERP, prev_frame_present=1, prev_frame_matched=1, workload_mapped=1, process_transforms=1, and a nonempty selected matrix vector. No stale or empty LERP source and no predicate mismatch are observed, but these downstream observations cannot close the causal root while join provenance and the byte-mismatch category remain unresolved.

Classification for this capture: exact direct admission PROVEN; face-slice equality PROVEN for the 96 sampled handoffs; velocity-zero invariant PROVEN; join provenance INCOMPLETE; CPU upload component equality UNKNOWN; first real divergence UNKNOWN; CPU handoff status UNKNOWN; no later-renderer root is promoted. The next single action is to isolate the three handoff byte mismatches by component while resolving the JOIN expected-role provenance, then rerun one bounded V7 capture. No production code, GPU readback, synchronization, commit, or push was performed for this analysis.
