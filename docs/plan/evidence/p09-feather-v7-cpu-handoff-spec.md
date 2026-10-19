---
title: "P09 V7 bounded CPU handoff witness specification"
description: "Implementation-only contract for Luna/Grok; six right-plume DLs, post-append baseline, upload handoff and render matrix selection."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-05
tags: [p09, kazooie, rt64, diagnostic-specification]
---

# P09 V7 bounded CPU handoff witness

Status: **NOT_IMPLEMENTED**. This is an experimental implementation contract,
not a production specification or a proven root. Read the source anchors and
causal boundaries in [[p09-post-triangle-architectural-review]] first.
S1–S11 below refer to that review's source-anchor table.

## Bounded V7 implementation specification

```text
BEST_NEXT_WITNESS_SEAM: State::fullSync after meshDesc.faceIndicesStart construction and updateDrawDataRanges, before uploadDrawData (S2:1136–1144)
WHY_EARLIEST: first point with both accumulated triangle indices and the derived draw slice plus complete initial upload sources; flush alone lacks the final slice
V7_IMPLEMENTATION_SPEC: one default-off right-plume CPU handoff witness, with an append-time baseline and a small render-matrix selection continuation
EXPECTED_INFORMATION_GAIN: HIGH
NEXT_IMPLEMENTATION_MODEL: GPT-5.6 Luna / Grok 4.6 Medium
SHOULD_USE_ASTRA_FOR_NEXT_IMPLEMENTATION: NO
```

### Scope and identity

Implement under a new default-off diagnostic switch, e.g.
`TOOIE_DIAG_FEATHER_HANDOFF_V7=1`, in owning Tooie/RT64 source, following the
existing external patch workflow. Do not change behavior or generated code.
Reuse the proven live model `0x62B` ownership and right-plume DLs
`0x534,0x54C,0x564,0x57C,0x592,0x5AA`. No whole-scene CSV, no per-frame millions
of triangles, no full matrix/vertex buffer dumps. Left/crest coverage is explicitly
OUT_OF_SCOPE; this experiment cannot clear the whole visible chain.

Use a Workload-owned diagnostic generation incremented on reset/reuse, together
with ring slot, model instance, and actual face offset. Carry that value across
the normal producer-consumer handoff; record final workloadId as an additional
label when assigned. Do not use live guest-frame counters on the renderer thread
as authoritative identity. Do not use `find_feather_world_hit` fallback across
workloads as proof. Do not identify a draw by pre-flush callIndex or primSeq
metadata, since these are not the finalized slice identity.

Reserve samples independently for each of the six target DLs and resolved/
unresolved/mismatch classes. A bounded window of 16 consecutive **target
workloads** and their display samples is sufficient for a first attempt; cap at
128 target vertices and 128 target triangles per workload, with explicit overflow
and coverage counters. These caps are ceilings, not assumed asset counts. A cap
hit makes the affected scope INCOMPLETE; it cannot become PASS. Arm on first
target inclusion, not at boot. The implementing model must provide a manual
capture command using the documented launcher; this review does not run it.

### Three records in one witness, not separate diagnostic suites

1. **APPEND baseline:** immediately after the three real `faceIndices.push_back`
   operations, snapshot their offset and actual corner order (including winding
   swap), values and source tuple. Capture only target geometry. Record the
   current CPU world matrix by value for each distinct world index. Include load
   provenance and actual G_MTX/restore role for joins without requiring a non-null
   semantic bone label. Resolve parent/child expected matrix identity from the
   existing G_MTX/modelview witness; do not infer it from worldIndex adjacency or
   the slot's bone label. If unavailable, preserve UNKNOWN and block join closure.
2. **INITIAL_HANDOFF:** at the stated fullSync seam, compare the baseline with
   the actual face slice and SoA sources/ranges. Check that each target triangle
   belongs to exactly one ordinary indexed GameCall and its indices are in range.
   Check byte equality of local position, world index, referenced original world
   matrix and view index; include local velocity and target modifyPos overlap.
   Log only failures plus one compact success summary per target workload/DL.
3. **RENDER_SELECTION:** at `TransformProcessor::upload`, record only referenced
   feather matrix slots from the **actual selected vector**, its size and upload
   range, matching predicates, current/previous indices and render weights.
   This is downstream of V5B's temporary `finalMatrixForWitness`. When this
   processor is skipped, record that fact at `threadRenderFrame` and use the
   known initial-upload source. Include selected projection index/matrix and
   RSP dispatch start/count in that same small render record. No new matcher,
   A/B renderer override, GPU staging allocation, readback or synchronization.

The append baseline must copy values, not retain vector pointers. Diagnostic
storage should be owned by the same Workload generation, immutable after
publication; do not build a racy global side map. Invalid selected-vector sizes
must be reported before diagnostic dereference. Do not silently repair or clamp
production data while collecting evidence.

```text
EXACT_FIELDS:
  capture_id, workload_generation, ring_slot, finalized_workload_id,
  model_bin/resource, DL identity/offset, actual face_offset, corner_order,
  global_vertex[3], RSP_slot[3], load_generation[3], load_DL[3],
  source_segmented/resolved/index[3], expected_join_role[3], expected_role_resolved,
  worldIndex[3], viewProjIndex[3], posXYZ[3], velocityXYZ[3],
  original_world_matrix16[distinct worldIndex], G_MTX/restore provenance,
  fbPair/projection/GameCall indices, meshDesc.faceIndicesStart, triangleCount,
  minWorldMatrix/maxWorldMatrix, actual target face slice,
  position/worldIndex/face/matrix source size and upload [first,second), stride,
  selected matrix source kind/size, selected matrix16[distinct worldIndex],
  prevFrame present/matched, current workload mapped, previous workload/index,
  render_sample_sequence, currentWeight, previousWeight, NO_INTERP status,
  selected viewProj matrix/viewport, RSP vertexStart/count, target modifyPos records,
  missing/overflow/unknown-join/byte-mismatch counters
```

Normal records may deduplicate identical matrices by index/hash, but retain the
actual values for the bounded first sample and failures. Log matrix bytes in a
specified order compatible with `interop::float4x4`; the offline check must use
the shader-equivalent multiplication convention, not infer row/column layout
from human-readable printing. Float copies and indices require exact equality.
Arithmetic replay uses a stated float32 tolerance and is not a GPU result.

### Classification and stop rules

```text
WHAT_RESULT_PROVES_CPU_POST_TRIANGLE_BUG:
  a same-generation accepted APPEND tuple differs from the corresponding
  INITIAL_HANDOFF tuple, or its assigned draw slice selects different corners;
  alternatively, actual render upload source/count is invalid, or a referenced
  selected world matrix differs from CURRENT while the established NO_INTERP
  mode is active. Report the first observed differing seam, not visual causality.

WHAT_RESULT_REFUTES_CPU_POST_TRIANGLE_BUG:
  exact comparisons plus valid sizes/ranges pass for the complete bounded
  target window. This refutes mutation/slice/selection hypotheses only for
  those captured fields and samples, not every CPU renderer/backend behavior.

WHAT_RESULT_FORCES_SHADER_OR_BACKEND:
  NONE from CPU source equality alone. Correct sources do not prove the
  asynchronous copy, descriptor binding, shader execution, or image ownership.
  A valid handoff narrows the next boundary to uploader/resource binding /
  shader position and raster coverage. It does not prove a GPU root.
```

If join input/semantic coverage fails, report `JOIN_INPUT_INCOMPLETE` and retain
both causal branches. If a join source or matrix is wrong already at APPEND,
stop and identify that input-side difference; do not call it post-triangle.
If a first CPU mismatch appears, save the bounded evidence and stop before a
fix. If every comparison passes, stop with `CPU_HANDOFF_CORRECT_FOR_CAPTURE`;
do not implement GPU diagnostics. A displayed good/bad classification requires
an independently identified displayed sample; this witness does not manufacture
that association from frame weights or video parity.

The earliest seam deliberately precedes uploader execution. A future escalation
could compare CPU source bytes at the uploader's existing memcpy and bindings
without GPU readback, but that is **not required in this first V7**. It must not
be preemptively expanded into a generic logging framework.


## Completion contract

No production fix, GPU readback, new GPU synchronization, broad capture, commit
or push. Preserve dirty source and follow the repository's external-patch
ownership rules. Build and validation commands must come from the current
phase/evidence; do not claim a runtime PASS without a user-authorized capture.
Update the review's successor evidence, session handoff and phase status with
observed results and a single next action. No requirement here authorizes the
implementation model to run the game without the next task's instructions.

Previous runtime results and static proof: [[p09-post-triangle-architectural-review]].

## Review-time offline verification

These checks were run during the static review, not during a V7 implementation.

Reproduce the offline count check without executing the game:

```sh
rtk proxy python3 - <<'PY'
import csv, collections, hashlib
p = '/tmp/tooie-kazooie-feather-v6c-tri.csv'
rows = list(csv.DictReader(open(p)))
joins = [r for r in rows if r['DL_KIND'] == 'JOIN']
print(hashlib.sha256(open(p, 'rb').read()).hexdigest())
print(len(rows), collections.Counter((r['DL_KIND'], r['CLASS']) for r in rows))
print('join_world_equals_load_active', all(r['WORLD_'+c] == r['LOAD_ACTIVE_'+c] for r in joins for c in 'ABC'))
print('join_load_dl_matches', all(r['CURRENT_DL'] == r['LOAD_DL_'+c] for r in joins for c in 'ABC'))
print('unresolved_joins', sum(r['CLASS'] == 'UNRESOLVED' for r in joins))
print('workload_values', sorted({r['WORKLOAD'] for r in rows}))
PY
```

Observed hashes, immutable identifiers of the inspected bytes (paths in `/tmp`
are mutable and are not archival guarantees):

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| `/tmp/tooie-kazooie-feather-v6c-tri.csv` | 57,563 | `3c9cde042146e6b82178fdbf033f6c30ddbf6cd52899a5e941cbdd5b4762035b` |
| `/tmp/tooie-secondary-sway-status.txt` | 13,157 | `ea7f5534033d536e8f393d14edb6c21ffebe96f64cc1a45193ed443d8ec4968c` |
| `/tmp/tooie-kazooie-feather-postmatch.csv` | 42,140 | `f019202756d9bb0c85461905f1999fb42d05049bb2d83359018a5832ee99ce93` |

Non-document source preservation was verified before/after documentation edits
by hashing sorted tracked and nonignored untracked regular files, with each
repository-relative path followed by NUL and its file bytes. Digests matched:

| Scope | Files | Before = after SHA256 |
| --- | ---: | --- |
| TooieRecomp excluding `docs/` | 145 | `fda05bbfec6133fb874e5cbc2ca2455368eff619147a618d5e4999eb150faf52` |
| RT64 excluding `docs/`, submodule contents excluded | 303 | `e6fc060696bbbbf11fd73a469b5f77c43801d7b76afd94d9fcb60df600dbba42` |

Plume was read for status only and not edited. jCodeMunch RSP index refresh
changed tool cache only; no material code change required a graph rebuild.
