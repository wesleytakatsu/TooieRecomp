---
title: "P09 selective WORLD selector granularity audit"
description: "WORLD-index selective elementwise policy leaks across GameCalls. Workload counts need the light CPU audit. Candidate stays default-off."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-23
tags: [p09, rt64, interpolation, world, selector, gamecall, false-positive, false-negative]
---

# P09 selective WORLD selector granularity audit

Role: `P09_SELECTIVE_WORLD_SELECTOR_FALSE_POSITIVE_FALSE_NEGATIVE_FORENSIC`.

Authoritative root stays:

```text
ROOT_CAUSE_CLASS=
NONLINEAR_COMPOSED_WORLD_INTERPOLATION_BREAKS_ARTICULATED_CONSTRAINTS
```

The elementwise WORLD causal result is already proven. This note does not
reopen it. `TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD` stays default-off.
No production shader change. No new interpolation candidate.

## User A/B (`USER_OBSERVED`, 2026-09-23)

Control, candidate off (`./build/TooieRecompiled`):

- Kazooie head feathers glitched
- Mumbo feathers glitched
- Mumbo one-frame disappearance not observed
- Klungo body cutting/line artifacts not observed
- Banjo snout/muzzle glitched

Candidate on (`TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD=1`):

- Kazooie head feathers corrected
- Banjo backpack/top flap appears corrected
- Mumbo feathers appear corrected
- Banjo snout/muzzle still glitched
- Klungo: new line/cutting artifacts while moving
- Mumbo: new occasional near-one-frame whole-character disappearance
- camera and ordinary rigid objects appear normal

```text
SELECTIVE_WORLD_INDEX_POLICY_FIXES_REAL_P09_CASES=YES
SELECTIVE_WORLD_INDEX_POLICY_HAS_FALSE_POSITIVE_REGRESSIONS=YES
SELECTIVE_WORLD_INDEX_POLICY_HAS_FALSE_NEGATIVE_OR_UNCOVERED_CASE=YES
CURRENT_VISUAL_CANDIDATE_STATUS=PARTIAL_FIX_WITH_REGRESSIONS
PRODUCTION_FIX_UPDATED=NO
```

Do not promote the candidate.

## Task 1 — selector dataflow (`PROVEN`, static)

```text
GameCall.meshDesc.faceIndicesStart
  + callDesc.triangleCount * 3
  → drawData.faceIndices
  → drawData.worldIndices[vertex]
  → p09MarkMultiWorldGameCallMatrices
```

`p09MarkMultiWorldGameCallMatrices` (`rt64_transform_processor.cpp`) walks
every GameCall in the workload. If that call's triangle corners reference
two or more distinct in-range WORLD slots, every one of those slots is
marked. The mark is an OR across calls. It is not cleared for other calls.

`TransformProcessor::process` then, for each mapped slot `t`:

- marked: `lerpMatrix` of all 16 entries at `prevFrameWeight` and
  `curFrameWeight`
- unmarked: `RigidBody::lerp`

The current-weight result is stored once in `lerpWorldTransforms[t]`.
`upload` copies that array to `worldTransformsBuffer` for the whole slot
range. `RSPProcessCS.hlsl` loads `worldTransforms[srcWorldIndices[vertex]]`.
There is no per-GameCall matrix.

RSP assigns the slot in `setVertexCommon`: a new `worldTransforms` entry
is pushed when the model-view-projection state changes, and every vertex
loaded under that state stores the same `curTransformIndex`. Later
triangles in any GameCall that reference those vertices, or any other
vertices loaded under the same slot, share it. GameCalls flush on RDP
draw-state changes (`State::checkDrawState` → `flush`) and on projection
changes, not on a matrix change. One call can therefore hold many WORLD
slots, and one slot can appear in many calls.

```text
DOES_SELECTED_WORLD_INDEX_CHANGE_ALL_USES_OF_THAT_INDEX=YES
CAN_ONE_SELECTED_WORLD_INDEX_BE_USED_BY_BOTH_MULTI_WORLD_AND_SINGLE_WORLD_CALLS=YES
CAN_CURRENT_SELECTOR_AFFECT_GEOMETRY_OUTSIDE_THE_TRIGGERING_GAMECALL=YES
TRANSFORMPROCESSOR_WORLD_INDEX_SELECTOR_GRANULARITY_SAFE=NO
```

`YES` on the second and third lines is structural: the code permits it
and the upload has no call identity. Whether a played frame actually
mixes multi-call and single-call uses of one selected slot is
`NOT_CAPTURED` until the audit file below is collected. That empirical
mix is not assumed.

## Task 2 — generic usage counts

The classifier can be evaluated entirely from CPU GameCall metadata:

| Field | Meaning |
| --- | --- |
| `totalGameCallUseCount` | calls whose faces reference the slot |
| `multiWorldGameCallUseCount` | those calls with ≥2 distinct slots |
| `singleWorldGameCallUseCount` | those calls with exactly 1 slot |
| `distinctGameCallCount` | sum of the two call counts |
| `distinctTransformGroupCount` | `worldTransformGroups[slot]` (one group id per slot) |
| `vertexUseCount` | face-corner references, not unique vertices |
| `selectedByCurrentPolicy` | `multiWorldGameCallUseCount > 0` |
| `usedExclusivelyByMultiWorldCalls` | multi > 0 and single = 0 |
| `usedByBothMultiAndSingleWorldCalls` | both counts > 0 |

No model, bone, or display-list id is involved.

Workload totals (`WORLD_INDEX_*`, `CURRENT_SELECTOR_SELECTED_*`) require a
live workload. Static inspection cannot count them.

```text
WORLD_INDEX_TOTAL_COUNT=NOT_CAPTURED
WORLD_INDEX_CURRENT_SELECTOR_COUNT=NOT_CAPTURED
WORLD_INDEX_MULTI_ONLY_COUNT=NOT_CAPTURED
WORLD_INDEX_MIXED_MULTI_SINGLE_COUNT=NOT_CAPTURED
WORLD_INDEX_SINGLE_ONLY_COUNT=NOT_CAPTURED
CURRENT_SELECTOR_SELECTED_MIXED_USAGE_COUNT=NOT_CAPTURED
CURRENT_SELECTOR_SELECTED_MULTI_ONLY_COUNT=NOT_CAPTURED
```

## Task 3 — false-positive mechanism (`PROVEN` mechanism, occurrence `NOT_CAPTURED`)

The candidate applies elementwise interpolation to the slot, not to the
triggering call's triangles. Any later consumer of that slot gets the
elementwise matrix:

- single-WORLD GameCalls that reuse the slot
- other multi-WORLD GameCalls that reuse the slot
- non-seam triangles inside the triggering call that use the slot

```text
CURRENT_SELECTOR_LEAKS_BEYOND_TRIGGERING_GAMECALL=YES
```

This `YES` is the architectural leak. A WORLD-index TransformProcessor
fix cannot confine the lerp to the triggering call: the slot has one
uploaded matrix. Avoiding the leak requires either a stricter slot
predicate (still global for every use of that slot) or a second matrix
plus a per-vertex index remap. Stronger metadata is required for
call-local behavior. No workaround is implemented here.

Klungo cutting and Mumbo disappearance are consistent with this leak and
also with elementwise non-rigidity on a multi-only slot that shares a
call with many bones. The audit's
`CURRENT_SELECTOR_SELECTED_MIXED_USAGE_COUNT` separates those. It is not
yet captured, so the played-frame cause is not proven.

## Task 4 — false-negative classes (`PROVEN` as escape classes)

`SAME_GAMECALL_MULTI_WORLD_REQUIRED_BY_CURRENT_SELECTOR=YES`.

A slot is selected only when some single GameCall's face corners contain
at least two distinct `worldIndices`. These structures therefore escape:

- A. An attachment whose vertices are split across GameCalls, when each
  call shows only one WORLD.
- B. Parent and child WORLDs that never appear together in one GameCall.
- C. A partner WORLD that the face list does not carry: RDP state or
  projection flush splits calls; matrices that are loaded but not
  referenced by `faceIndices` are invisible; `minWorldMatrix`/`maxWorldMatrix`
  are not the classifier.
- D. Reuse through the shared slot or `worldTransformGroups`. The slot is
  the only render identity. Group classification is coarser (many Tooie
  slots share one AUTO group) and cannot see a partner that never shares
  a call.
- E. Articulation already composed into one guest matrix before the slot
  is created. One slot has no cross-WORLD seam for this classifier to see.

```text
CURRENT_SELECTOR_KNOWN_FALSE_NEGATIVE_CLASSES=A_CROSS_GAMECALL_SPLIT B_PARENT_CHILD_NEVER_SHARE_A_GAMECALL C_FACE_LIST_HIDES_PARTNER_WORLD D_SLOT_OR_GROUP_REUSE_HAS_NO_CALL_IDENTITY E_SINGLE_SLOT_ALREADY_COMPOSED
CROSS_GAMECALL_ARTICULATED_CONSTRAINT_CAN_ESCAPE=YES
SNOUT_FALSE_NEGATIVE_CLASS=NOT_PROVEN
```

The Banjo snout/muzzle remaining glitched shows that some real case
escapes or is not fixed by the selected slots. Existing metadata does
not prove which class the snout is. The old snout dual-matrix theory is
not revived.

## Task 5 — smaller generic selectors (not implemented)

### 1. `WORLD_INDEX_MULTI_ONLY`

Elementwise only when `singleWorldGameCallUseCount == 0`. Same
TransformProcessor upload. No shader change.

```text
WORLD_INDEX_MULTI_ONLY_SELECTOR_FEASIBLE=YES
```

False-positive risk falls only for mixed slots. A multi-WORLD call that
batches many bones still marks every slot it touches, and those slots
stay multi-only if nothing else uses them. Klungo-style shear inside
that batch is not removed. False-negative risk rises: a seam bone that
is also used by its own single-WORLD limb call loses elementwise
interpolation, which re-opens the constraint the current candidate
fixed. Do not switch to this until the audit shows the corrected cases
are multi-only and the regressions are mixed.

### 2. `GAMECALL_LOCAL`

Elementwise only for vertices of the triggering multi-WORLD call.
In-place lerp of `lerpWorldTransforms[t]` cannot do that, because every
vertex with that index shares the upload.

```text
GAMECALL_LOCAL_SELECTOR_FEASIBLE_WITHOUT_SHADER_CHANGE=CONDITIONAL
```

Feasible without a shader change only by allocating a second WORLD slot,
writing the elementwise matrix there, and remapping `worldIndices` for
vertices that belong to the triggering call. A vertex referenced by both
a multi-WORLD call and a single-WORLD call has to be duplicated so the
two uses can carry different indices. Shader branching is the other
implementation, and it is not required if the remap is done. Neither is
implemented. If the whole character is already one GameCall, call-local
selection matches the current candidate for those slots.

### 3. `GENERIC_ARTICULATED_SEAM`

A generic attachment is a pair of local positions on different WORLD
slots whose endpoints coincide under row-vector multiplication:

```text
worldPos = mul(localPos4, WORLD)
pA * WA_prev ≈ pB * WB_prev
pA * WA_cur  ≈ pB * WB_cur
```

`posFloats` are local vertex positions. Current WORLD slots and the
matched previous slots both exist before interpolation. Same-triangle
corners with different `worldIndices` can be tested in time linear in
the face list. Cross-call discovery needs a bounded spatial hash of
those endpoint positions, not an all-pairs search. Either test can mark
WORLD slots before `lerp`. It still selects slots, so the uploaded
matrix remains global for every use of that slot. It can reject a
multi-WORLD call whose positions do not meet, which is the co-batching
false positive, and it can see a cross-call seam the current classifier
misses. No hierarchy and no game ids. Not implemented.

```text
GENERIC_ARTICULATED_SEAM_DISCOVERY_FEASIBLE=YES
```

## Task 6 — interpolation property (justification only)

If both endpoints already coincide, elementwise interpolation of the two
WORLD matrices preserves the coincidence, because each entry is an
affine mix and the shared position is linear in those entries. That is
why a verified seam is a valid selector signal. It does not make an
unrelated mesh that shares the slot rigid. Decomposed `RigidBody::lerp`
does not preserve the same coincidence. No hierarchy is implied.

## Task 7 — light diagnostic

Static inspection answers the yes/no granularity questions and does not
answer workload counts. Added default-off CPU audit
`TOOIE_DIAG_P09_SELECTOR_AUDIT=1`. It does not change interpolation, the
visual candidate, or the shader. Output is one overwritten file,
`/tmp/tooie-p09-selector-audit.txt`, with the aggregate fields plus at
most 24 mixed and 8 multi-only `SEL` lines. Session peak counters stay
in fixed scalars. No endpoint factorial, no percentile pass, no
per-vertex log.

```text
USER_TEST_COMMAND=TOOIE_DIAG_P09_SELECTOR_AUDIT=1 ./build/TooieRecompiled
USER_TEST_DIAGNOSTIC_HEAVY=NO
```

Run it on the same high-rate scene as the A/B (Kazooie, Banjo, Mumbo,
Klungo visible). Paste the aggregate keys and `SESSION_PEAK_*`. Do not
enable `TOOIE_DIAG_RSP_ENDPOINT_BINDING`. The visual candidate can stay
off; the audit classifies the policy without applying it.

## Offline validation

```text
TOOIE_HEAD=ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf
RT64_HEAD=6f1c2d99a4ea571c139f449c326fd176ba8f3496
rt64_transform_processor.cpp SHA-256=
  54a79a85dd1b054af7c16d71a9d15de00fc9c5dd0d4bceb4cfad07b31a98b23b
RSPProcessCS.hlsl SHA-256=
  b092ff8466fd870deec62f1c908a74e84e0a9eb07d11b2d82f15e7c8a5addcff
  (unchanged this task)
cmake --build build --target TooieRecompiled                 exit 0
./build/window_display_test                                   38/38 PASS
./build/tintro_projection_policy_test                         7/7 PASS
git diff --check Tooie                                       exit 0
git diff --check RT64                                        exit 0
GAME_LAUNCHED=NO
GPU_READBACK_USED=NO
GPU_SYNCHRONIZATION_ADDED=NO
```

```text
BEST_NEXT_SELECTOR_GRANULARITY=GENERIC_ARTICULATED_SEAM
```

Design that seam test only after the audit file shows whether selected
slots are mixed or multi-only. `WORLD_INDEX_MULTI_ONLY` is the smaller
TransformProcessor experiment if, and only if, the corrected witnesses
are multi-only and the regressions are mixed. Call-local or seam-driven
selection that needs duplicated matrices or remapping gets a fix-design
review before implementation.

```text
NEXT_SINGLE_ACTION=run TOOIE_DIAG_P09_SELECTOR_AUDIT=1 on the A/B scene and paste /tmp/tooie-p09-selector-audit.txt aggregates
NEXT_RECOMMENDED_MODEL=Grok 4.6 Medium
CLEANUP_READY=NO
COMMIT_READY=NO
NO_COMMIT=YES
NO_PUSH=YES
```

## User capture after this note (`USER_OBSERVED`, 2026-09-23)

The lightweight audit was run. Session peaks:
`SESSION_WORKLOADS_SEEN=11828`, `SESSION_LEAK_OBSERVED=YES`,
`SESSION_PEAK_SELECTED_MIXED=31`, `SESSION_PEAK_SELECTED=233`,
`SESSION_PEAK_SELECTED_MULTI_ONLY=206`.

Representative workload: `GAMECALL_COUNT=233`,
`MULTI_WORLD_GAMECALL_COUNT=42`, `SINGLE_WORLD_GAMECALL_COUNT=191`,
`WORLD_INDEX_TOTAL_COUNT=117`, `WORLD_INDEX_USED_COUNT=116`,
`WORLD_INDEX_CURRENT_SELECTOR_COUNT=101`,
`WORLD_INDEX_MULTI_ONLY_COUNT=81`,
`WORLD_INDEX_MIXED_MULTI_SINGLE_COUNT=20`,
`WORLD_INDEX_SINGLE_ONLY_COUNT=15`,
`CURRENT_SELECTOR_SELECTED_MIXED_USAGE_COUNT=20`,
`CURRENT_SELECTOR_SELECTED_MULTI_ONLY_COUNT=81`,
`CURRENT_SELECTOR_LEAKS_BEYOND_TRIGGERING_GAMECALL=YES`.

Mixed examples include `w=52` (1 multi call, 7 single calls),
`w=34` (1 multi, 2 single), `w=81` (5 multi, 2 single).

The follow-up candidate is
[[plan/evidence/p09-multi-only-world-selector-visual-candidate]].
It does not replace `TOOIE_TEST_P09_SELECTIVE_ELEMENTWISE_WORLD`.
