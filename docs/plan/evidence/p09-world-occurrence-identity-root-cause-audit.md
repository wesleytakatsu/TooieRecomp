---
title: "P09 world-occurrence identity root-cause audit"
description: "39 session-union worldIndex values vs 9 bones; V5C token-only CURRENT; CPU 58.6x is DECOMPOSE world lerp, not proven wrong pairing."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-09
tags: [tooie-recomp, p09, kazooie, feather, interpolation, high-framerate, world-index, v5c, pr22]
---

# P09 world-occurrence identity root-cause audit

Schema: `P09_WORLD_OCCURRENCE_IDENTITY_ROOT_CAUSE_AUDIT`.

No GPU readback, copy, layout, sync, staging, or occlusion query.
No temporal-weight change. No new visual override. Game not launched.
No production patch. Capture already present at
`/tmp/tooie-feather-temporal-coherency-status.txt`.

```text
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NEW_CAPTURE_REQUIRED: NO
NEW_VISUAL_EXPERIMENT_REQUIRED: NO
GENERATED_RECOMPILED_CODE_EDITED: NO
```

Authoritative prior plus this capture:

```text
NORMAL_HIGH_RATE: GLITCH
ORIGINAL_FRAMERATE: FIXED
GLOBAL_CURRENT_FRAME: FIXED
V5C_NINE_LOGICAL_BONES_CURRENT: SAME
CPU_TEMPORAL_POSITION_MATH_EXPLAINS_VISUAL_GLITCH: YES
MAX_INTERMEDIATE_EDGE_EXPANSION_RATIO: 58.596
INTERMEDIATE_GEOMETRIC_EXPLOSION_OBSERVED: YES
EXPLOSION_TRIANGLE_COUNT: 44
```

Do not reopen GPU lifetime, textures, alpha, depth, culling, sampler,
authored keys, or velocity as primary. V7 current-frame handoff stays
`CORRECT`; this is the intermediate sample.

## Capture identity (`PROVEN`)

```text
MODEL: 0x62B
LAST_MODEL_BIN: 0xFFFFFFFF8022B850
INTERMEDIATE_RENDERS: 1955
VISIBLE_FACE_VERTEX_COUNT: 157755
VISIBLE_TRIANGLE_COUNT: 52585
VISIBLE_WORLD_TRANSFORM_SET: 79,100,101,102,103,104,105,107,108
UNEXPECTED_WORLD_TRANSFORM_COUNT: 0
VISIBLE_WORLD_CROSS_IDENTITY_PAIRING_OBSERVED: NO
CROSS_IDENTITY_COUNT: 0
SEGMENTED_ADDR_MAP_HITS: 0
VIEW_MATCH_STABLE: YES
PROJECTION_MATCH_STABLE: YES
VIEWPROJ_PAIRING_ANOMALY_OBSERVED: NO
VIEWPROJ_DEGENERACY_OBSERVED: NO
VISIBLE_FEATHER_EFFECTIVE_VELOCITY_ALWAYS_ZERO: YES
MATCH_WRITTEN_VELOCITY_ON_VISIBLE_FEATHERS: NO
INTERMEDIATE_VERTEX_NONFINITE_COUNT: 0
INTERMEDIATE_W_NEAR_ZERO_COUNT: 0
MIXED_SOURCE_DL_CALLS: 11326 / VISIBLE_TARGET_CALLS 11326
```

`CROSS_IDENTITY_COUNT=0` is **not** a strong proof. The witness only
labels previous-slot bone via `worldTransformSegmentedAddresses`, and
`SEGMENTED_ADDR_MAP_HITS=0`. Bone labels on explosion rows are the
**current** slot's learned bone, not a proven previous-slot bone.

## Task 1 — Why 39 world indices (`PROVEN` + `STRONG_INFERENCE`)

`VISIBLE_WORLD_INDEX_UNIQUE_COUNT=39` is a **process-lifetime union**
of numeric `worldIndex` values (`103..148` with seven holes). The
witness never resets `uniqueWorld`. It is **not** 39 simultaneous
slots in one frame.

Per-frame, the same logical bone still occupies **more than one**
slot. Workload `1332` uses bone 100 at both `137` (JOIN with 107) and
`138` (own triangle). Workload `1467` uses bone 100 at `138` and `139`.

RT64 allocates a new world slot on the first `G_VTX` after
`modelViewProjChanged` (`RSP::setVertexCommon`). `G_MTX` and
`G_POPMTX` both set that flag.

Static 0x62B feather walk ([[p09-kazooie-feather-gmtx-to-dl-trace]]):

- GeoCmd2 `func_800DCF48` pushes the composed bone matrix.
- Type-0x11 own DL: `G_VTX` under that matrix (first slot).
- Type-0x18 JOIN DL: child `G_VTX`, then `G_POPMTX`, then parent
  `G_VTX` (second slot for the parent bone). After the DL, type-0x18
  emits `G_MTX` restore.

Visible JOIN extras: `0x592`/`0x61E` (107), `0x4B0` (108), `0x564`
(103), `0x54C` (104), `0x5F0` (100), `0x5D8` (101). That is nine PUSH
slots plus seven JOIN-POP parent slots per walk (`STRONG_INFERENCE`
~16 in-frame slots), then numeric IDs drift as earlier scene matrices
come and go (session union 39).

```text
VISIBLE_WORLD_INDEX_COUNT: 39
VISIBLE_WORLD_INDEX_COUNT_IS_SESSION_UNION: YES
LOGICAL_BONE_COUNT: 9
MULTIPLE_WORLD_OCCURRENCES_PER_BONE: YES
SAME_BONE_OCCURRENCES_MATRIX_EQUIVALENT_WITHIN_FRAME: YES
```

`SAME_BONE_OCCURRENCES_MATRIX_EQUIVALENT_WITHIN_FRAME=YES` is
`STRONG_INFERENCE` for PUSH vs later POP-restore of the **same**
GeoCmd2 node (same stack matrix, no extra local compose). It is not a
dumped Frobenius compare. A second full 0x62B submit (shadow / other
pass) would not be equivalent; this capture did not dump that.

## Task 2 — V5C coverage (`PROVEN`)

V5C does **not** walk visible `worldIndex` values.

`TransformProcessor::process` calls `tooie_diag_rt64_no_interpolation`
for every mapped `t` in `worldTransforms`. The Tooie callback returns
true only when **exactly one** `SingleMtxTraceToken` matches
`(workloadId, transform_index, group_index)` with matrix+vertex+match
seen and `is_full_kazooie_feather_trace`.

Tokens are created at DCF48 `mlMtxApply` (one per GeoCmd2 compose),
bound at that destination `G_MTX`, and consumed on the **first**
`G_VTX` (`p09DiagnosticMatrixToken = 0` after `tooie_diag_rt64_vertex`).
JOIN parent verts after `G_POPMTX` allocate a new `worldIndex` with
token `0`. Those slots never receive V5C CURRENT.

`BONE*_HIT` / `ALL_NINE_NO_INTERP_RESULT_EQUALS_CURRENT` only prove
that each **logical bone** had at least one token override. They do
not prove every concrete slot used by visible triangles was frozen.

```text
V5C_MATCHES: token (GeoCmd2 compose → first G_VTX worldIndex)
V5C_DOES_NOT_MATCH: JOIN-POP parent slots; any later G_MTX of the same bone
V5C_CONCRETE_WORLD_OCCURRENCES_TOTAL: 39 (session-union visible worldIndex)
V5C_CONCRETE_WORLD_OCCURRENCES_FORCED_CURRENT: not 39; ≤1 token slot per bone per GeoCmd2 visit
V5C_COVERED_ALL_39_VISIBLE_WORLD_INDICES: NO
V5C_COVERAGE_BY_WORLD_INDEX: first G_VTX after tracked GeoCmd2 G_MTX only
TEMPORAL_MATRIX_INTERPOLATION_CAUSAL_STATUS: CORRECTED/SUPERSEDED
  (no longer REFUTED_FOR_ALL_TRANSFORMS_DIRECTLY_USED_BY_VISIBLE_FEATHER_TRIANGLES)
```

V5C visual `SAME` therefore does **not** refute interpolation of every
concrete world matrix used by visible feather vertices. It refutes
only the token-bound first occurrence per tracked bone.

## Task 3 — PAIR_CHANGE semantics (`PROVEN`)

The witness keeps **one** `(cur,prev)` pair per logical bone slot, not
per world occurrence. `PAIR_CHANGE` increments when that bone's last
seen `prevTransformIndex` differs from this vertex's AUTO previous.

That fires for:

1. walking several in-frame occurrences of the same bone
2. session index drift (`103`…`148`)
3. a true previous-identity change

It is **not** by itself a bug.

```text
PAIR_CHANGE_IS_EXPECTED_INDEX_RENUMBERING: PARTIAL
WRONG_SAME_BONE_OCCURRENCE_PAIRING_OBSERVED: NO
```

AUTO, when call hashes match and world-count matches, first proposes
same-offset pairs inside `GameCall` `[minWorldMatrix, maxWorldMatrix]`,
then greedy-sorts those candidates by `computeTransformMatch`. All
visible target calls in this capture are mixed (`sourceDlCount>1`).

Explosion rows (first 16 strongest):

| curId | worlds | prevMatch | pairing |
| --- | --- | --- | --- |
| 866 | 147/147/146 | 147/147/146 | stable |
| 1332 | 136/137 and 138 | +1 | shift |
| 1467 | 137/138/139 | 137/138/139 | stable |
| 1475 | 134/133/134 | 134/133/134 | stable |

10/16 strongest records have `prevMatch == current worldIndex`.
Wrong-occurrence pairing is **not** required for the 58× result.

## Task 4 — Stable occurrence identity (`PROVEN_STATIC` / `HYPOTHESIS`)

A stable identity **can** be derived without changing rendering:

```text
model 0x62B + GeoCmd node rel + boneAnimID + DL logical + G_MTX/POP ordinal
```

Tooie does not emit `gEXMatrixGroup`. Default `TransformGroup` is
`matrixId=G_EX_ID_AUTO`, `decompose=true`.

```text
STABLE_OCCURRENCE_IDENTITY_DERIVABLE: YES
TOOIE_AUTO_LACKS_STABLE_OCCURRENCE_IDENTITY: YES
EXPLOSION_COUNT: 44
EXPLOSION_CORRELATES_WITH_WRONG_OCCURRENCE_MATCH: NO
WRONG_OCCURRENCE_EXPLOSION_COUNT: 0 proven
CORRECT_OCCURRENCE_EXPLOSION_COUNT: ≥10 of the 16 strongest (stable index)
```

## Task 5 — BanjoRecomp PR #22 (`PROVEN_STATIC`)

PR #22 (`8b9ce251…`) assigns `cur_model_transform_id_offset` in
**logical** GeoCmd order, including sort branches, so concrete matrix
occurrences keep the same `gEXMatrixGroup` id across frames.

Tooie 0x62B has no equivalent. That gap is real.

Causal rejection in the earlier portability audit used V5B/V5C as
interpolation closure. That closure is `CORRECTED/SUPERSEDED` because
V5C did not cover JOIN-POP (and other extra) slots.

PR #22-class IDs would make AUTO match by group id instead of call
range + greedy pose. They would **not** by themselves stop a
stable-index decomposed lerp exploding (the 139→139 class).

Banjo PR #23 (`G_EX_INTERPOLATE_SIMPLE` when `cur_model_uses_bones`)
sets `decompose=false` for boned models. That class addresses the
proven CPU term: independent decomposed interpolation of composed
world matrices.

```text
BANJO_PR22_CAUSAL_RELEVANCE_REOPENED: YES
PR22_CLASS_CAN_PREVENT_OBSERVED_WRONG_PAIRING: NO
  (observed strongest explosions are not proven wrong-pairing)
PR23_SIMPLE_CLASS_ADDRESSES_PROVEN_CPU_TERM: YES
```

Do not blindly copy BK GeoCmd layouts. Tooie type-2/0x11/0x18 are not
BK GeoCmd2/3/1.

## Task 6 — The 44 explosions (`PROVEN` from bounded records)

`INTERMEDIATE_W_NEAR_ZERO_COUNT=0` and finite vertices: not a
perspective-divide collapse.

Dominant 58.596× example (`curId=1332`, `w=0.5`, all bone 100,
`world=138→prev 139`):

- endpoints NDC ~1e-3
- intermediate NDC ~0.08 on one vertex
- `ratioB` (current viewProj + interpolated world) = 8.18
- `ratioC` (interpolated viewProj + current world) = 0.845
- `ratioD` (vel=0) = 58.596

Offline A/B/C/D on the 16 strongest: 14/16 are removed by **current
world**; 2/16 (JOIN 100/107 at `curId=866`) are removed by **current
viewProj**. Aggregate `REMOVED_BY_CURRENT_VIEWPROJ=YES` is the
minority class, not the typical one.

```text
EXPLOSION_CAUSED_BY_CLIP_W_COLLAPSE: NO
EXPLOSION_CAUSED_BY_WORLD_POSITION_DIVERGENCE: YES
EXPLOSION_CAUSED_BY_MATRIX_INTERPOLATION_REPRESENTATION: YES
EXPLOSION_CAUSED_BY_WRONG_OCCURRENCE_PAIRING: NO
FIRST_CPU_TEMPORAL_CAUSAL_TERM: WORLD
```

Representation: default AUTO `decompose=true`. `RigidBody::lerp` then
independently lerps decomposed TRS. Endpoints can be close in NDC
while `w=0.5` is far. That matches USER_OBSERVED 10–30× streaks with
the base still attached. Bone 107's large authored Y steps sit in
**composed** child worlds (100–105) even when the child's local 0x28
is smooth.

## Task 7 — Why both current viewProj and current world can “remove” some rows

`clip = viewProj(w) * world(w) * pos`. Replacing either factor with
its current endpoint can shrink an unstable product. That is not
causal isolation.

View/proj pairing is stable and non-degenerate.
`VIEWPROJ_ITSELF_WRONG=NO`. Interpolated viewProj can still **amplify**
a bad world (the 8× leftover on ratioB).

`WORLD_INTERPOLATION_ITSELF_WRONG=YES` for the 14/16 majority: putting
the current world matrix at the same interpolated viewProj removes
the explosion.

## Task 8 — No new capture

clip.w health is already in the status (`W_NEAR_ZERO=0`).
Occurrence pairing vs explosion is in the 16 bounded rows.
V5C token vs JOIN-POP is static RT64 + Tooie source.

```text
NEW_CAPTURE_REQUIRED: NO
```

## Task 9 — Production decision

Wrong same-bone occurrence pairing is **not** proven. Do **not**
implement a PR #22 port in this task.

A production **candidate** exists:

```text
PRODUCTION_FIX_CANDIDATE_FOUND: YES
PRODUCTION_FIX_CLASS:
  EXPLICIT_STABLE_MATRIX_GROUP_IDENTITY
  + BONED SIMPLE / hierarchy-consistent interpolation
  (Banjo PR22 identity + PR23 SIMPLE policy; Tooie GeoCmd mapping)
```

Not: disable interpolation, global CURRENT, feather special-case,
hardcoded 0x62B in production, or RT64 greedy heuristics.

**Later correction (boned SIMPLE, 2026-09-09):** do **not** implement
an occurrence-complete CURRENT visual override. Mixed-time CURRENT
is `INVALID_TEMPORALLY_INCOHERENT`. The discriminator is an offline
SIMPLE reconstruction in the same passive witness. See
[[p09-boned-interpolation-counterfactual]].

## Status

```text
CPU_TEMPORAL_POSITION_MATH_EXPLAINS_VISUAL_GLITCH: YES
MAX_INTERMEDIATE_EXPANSION_RATIO: 58.596
VISIBLE_WORLD_INDEX_COUNT: 39
LOGICAL_BONE_COUNT: 9
MULTIPLE_WORLD_OCCURRENCES_PER_BONE: YES
SAME_BONE_OCCURRENCES_MATRIX_EQUIVALENT_WITHIN_FRAME: YES
V5C_COVERED_ALL_39_VISIBLE_WORLD_INDICES: NO
PAIR_CHANGE_IS_EXPECTED_INDEX_RENUMBERING: PARTIAL
WRONG_SAME_BONE_OCCURRENCE_PAIRING_OBSERVED: NO
STABLE_OCCURRENCE_IDENTITY_DERIVABLE: YES
EXPLOSION_CORRELATES_WITH_WRONG_OCCURRENCE_MATCH: NO
WRONG_OCCURRENCE_EXPLOSION_COUNT: 0
EXPLOSION_CAUSED_BY_CLIP_W_COLLAPSE: NO
EXPLOSION_CAUSED_BY_WORLD_POSITION_DIVERGENCE: YES
EXPLOSION_CAUSED_BY_MATRIX_INTERPOLATION_REPRESENTATION: YES
EXPLOSION_CAUSED_BY_WRONG_OCCURRENCE_PAIRING: NO
VIEWPROJ_ITSELF_WRONG: NO
WORLD_INTERPOLATION_ITSELF_WRONG: YES
BANJO_PR22_CAUSAL_RELEVANCE_REOPENED: YES
TOOIE_AUTO_LACKS_STABLE_OCCURRENCE_IDENTITY: YES
PR22_CLASS_CAN_PREVENT_OBSERVED_WRONG_PAIRING: NO
NEW_CAPTURE_REQUIRED: NO
PRODUCTION_FIX_CANDIDATE_FOUND: YES
COMMIT_READY: NO
```
