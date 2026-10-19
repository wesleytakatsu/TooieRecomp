---
title: "P09 boned interpolation counterfactual and production-fix proof"
description: "Live AUTO world lerp is DECOMPOSE; SIMPLE counterfactual is in the same passive witness; production class is Tooie GeoCmd groups + boned SIMPLE."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-09
tags: [tooie-recomp, p09, kazooie, feather, interpolation, decompose, simple, pr22, pr23]
---

# P09 boned interpolation counterfactual

Schema: `P09_BONED_INTERPOLATION_COUNTERFACTUAL`.

No GPU readback, copy, layout, sync, staging, or occlusion query.
No weight change. No visual override. No production patch in this
task. Game not launched by the agent.

```text
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
WEIGHTS_MUTATED: NO
RENDER_PATH_MUTATED: NO
GENERATED_RECOMPILED_CODE_EDITED: NO
```

Authoritative prior:

```text
NORMAL_HIGH_RATE: GLITCH
ORIGINAL_FRAMERATE: FIXED
GLOBAL_CURRENT_FRAME: FIXED
V5C_NINE_LOGICAL_BONES_CURRENT: SAME
CPU_TEMPORAL_POSITION_MATH_EXPLAINS_VISUAL_GLITCH: YES
MAX_INTERMEDIATE_EXPANSION_RATIO: 58.596
EXPLOSION_TRIANGLE_COUNT: 44
WRONG_SAME_BONE_OCCURRENCE_PAIRING_OBSERVED: NO
EXPLOSION_CORRELATES_WITH_WRONG_OCCURRENCE_MATCH: NO
VIEW_MATCH_STABLE: YES
PROJECTION_MATCH_STABLE: YES
VISIBLE_FEATHER_EFFECTIVE_VELOCITY_ALWAYS_ZERO: YES
```

Do not reopen GPU lifetime, textures, alpha, depth, culling, sampler,
authored keys, or velocity as primary. Do not add another mixed-time
CURRENT visual override. V7 current-frame handoff stays `CORRECT`.

## Task 1 — live interpolation mode (`PROVEN`)

Untagged Tooie worlds use the default `TransformGroup`
(`rt64_transform_group.h`):

```text
matrixId = G_EX_ID_AUTO
decompose = true
position/rotation/scale/skew/perspective = G_EX_COMPONENT_AUTO
vertexInterpolation = G_EX_COMPONENT_SKIP
```

`G_EX_MATRIXGROUP_V1` passes GBI `mode` as `RSP::matrixId(..., decompose, ...)`.
`G_EX_INTERPOLATE_SIMPLE=0` → `decompose=false`.
`G_EX_INTERPOLATE_DECOMPOSE=1` → `decompose=true`.

`GameFrame::matchTransform` copies `curGroup.decompose` into
`RigidBody::updateDecomposition`. `TransformProcessor::process` then
uploads `rigidBody.lerp(curFrameWeight, prevWorld, curWorld, slerp=true)`.

`RigidBody::lerp` (`rt64_rigid_body.cpp`):

- if `lerpDecompose` and both decompositions valid:
  independently lerp translation, slerp/lerp rotation, lerp scale,
  lerp skew, lerp perspective, then `recomposeMatrix`
- else (`G_EX_INTERPOLATE_SIMPLE`):
  `lerpMatrixComponents(prev, cur, lerpTranslation, lerpRotation, lerpPerspective, w)`
  which lerps the top-left 3×3 and the translation (and the last
  column if perspective is INTERPOLATE)

AUTO rotation `FIXME` always sets `lerpRotation=true`. AUTO
perspective is **not** INTERPOLATE, so the last column stays current.
Affine modelviews are `[0,0,0,1]` there.

The schema-v2 witness counts `group.decompose` / `matrixId` on every
visible feather vertex. Until that recapture, live 0x62B is
`PROVEN` from the default group plus the absence of Tooie
`gEXMatrixGroup` on `func_800DCF48`.

```text
CURRENT_RT64_WORLD_INTERPOLATION_MODE: DECOMPOSE
AUTO_RESOLVES_TO: G_EX_INTERPOLATE_DECOMPOSE (decompose=true)
SOURCE: TransformGroup default; RSP::matrixId; GameFrame::matchTransform;
        RigidBody::lerp; TransformProcessor::process
```

## Task 2–3 — SIMPLE counterfactual (`IMPLEMENTED`, capture pending)

The previous status file kept NDC of the 16 strongest explosions, not
the endpoint world matrices. Exact SIMPLE reconstruction is therefore
**not** possible from `/tmp/tooie-feather-temporal-coherency-status.txt`
alone.

The **same** default-off witness now, analysis-only:

```text
Ws = lerpMatrixComponents(W_prev, W_cur, true, true, true, w)
clip_S = viewProj(w) * Ws * pos
```

That is the real RT64 SIMPLE result under Banjo PR #23 bone flags
(`G_EX_COMPONENT_INTERPOLATE` for pos/rot/persp). `Ws` is **not**
written into `lerpWorldTransforms` and is not uploaded.

Schema version 2 also keeps all 44 explosion records (`kMaxAnomalies=48`),
live vs SIMPLE max/p95/median, and counts `>2x/>5x/>10x/>20x`.

```text
EXPLOSION_COUNT_LIVE_MODE: 44  (prior capture)
MAX_EXPANSION_LIVE_MODE: 58.596
EXPLOSION_COUNT_SIMPLE_COUNTERFACTUAL: NOT_YET_CAPTURED
MAX_EXPANSION_SIMPLE_COUNTERFACTUAL: NOT_YET_CAPTURED
SIMPLE_REMOVES_ALL_GEOMETRIC_EXPLOSIONS: UNKNOWN
SIMPLE_MATERIALLY_REDUCES_EXPLOSIONS: UNKNOWN
NEW_RUNTIME_CAPTURE_REQUIRED: YES
```

User command (no visual classification, no other diag flags):

```text
rm -f /tmp/tooie-feather-temporal-coherency-status.txt
TOOIE_DIAG_FEATHER_TEMPORAL_COHERENCY=1 ./build/TooieRecompiled
```

Read `VERSION=2`, `SIMPLE_FEEDS_RENDER=NO`,
`EXPLOSION_COUNT_SIMPLE_COUNTERFACTUAL`,
`LIVE_EXPLOSION_STILL_SIMPLE_COUNT`, and `ratioS=` on `EXPANSION_*`.

## Task 4 — why DECOMPOSE fails on composed bones (`STRONG_INFERENCE`)

Tooie already composes parent⊗child in `func_800AE8A8` and submits
**world** matrices. DECOMPOSE then treats each composed world as an
independent rigid body: slerp of world rotation + lerp of world
translation + lerp of polar scale.

That does **not** preserve `W_child(w) = W_parent(w) * local_child(w)`.
Endpoints remain the true composed matrices, so V7 current-frame
handoff stays correct. At intermediate `w` the independently
recomposed parent and child can separate.

JOIN DLs (`func_800DD998`) mix child verts under the pushed bone with
parent verts after `G_POPMTX` (107↔100/103, 108↔79). Those triangles
amplify a parent/child world-lerp disagreement. Bone 107 also carries
the authored large Y steps (walk↔idle), so polar decomposition of its
composed world is a high-error case.

SIMPLE of the same composed pair is a linear map:
`Ws*pos = lerp(W_prev*pos, W_cur*pos)` in world space. Endpoints stay
exact; the intermediate cannot leave the world-space segment between
them.

```text
DECOMPOSE_BREAKS_ARTICULATED_WORLD_RELATIONSHIP: YES
JOIN_GEOMETRY_AMPLIFIES_WORLD_INTERPOLATION_ERROR: YES
WHY_ENDPOINTS_ARE_CORRECT_BUT_INTERMEDIATE_EXPLODES:
  DECOMPOSE slerps already-composed worlds independently;
  JOIN mixes parent and child slots; w=0.5 is not on the
  articulated path between the two valid endpoints.
```

## Task 5 — groups vs SIMPLE (`PROVEN_STATIC`)

Selecting SIMPLE **requires** a `gEXMatrixGroup` (the GBI `mode` bit).
The id **may** be `G_EX_ID_AUTO`. There is no other Tooie/RT64 switch
that sets `decompose=false` on untagged AUTO groups.

```text
CAN_TOOIE_SELECT_SIMPLE_WITHOUT_EXPLICIT_MATRIX_GROUPS: YES
  (gEXMatrixGroup(G_EX_ID_AUTO, G_EX_INTERPOLATE_SIMPLE, ...))
EXPLICIT_GROUP_IDS_REQUIRED_FOR_PRODUCTION_FIX: YES
  (Banjo-class production: stable pairing + JOIN/POP/sort occurrences)
STABLE_OCCURRENCE_IDS_REQUIRED_FOR_CORRECT_PAIRING: NO
  (not proven causal for the captured 44; those include stable prevMatch)
```

Do not treat AUTO+SIMPLE as a complete production substitute for
occurrence IDs. Mixed `sourceDlCount>1` calls still use AUTO greedy
pose matching. IDs are **infrastructure**; SIMPLE is the **observed
root-class** candidate.

## Task 6 — Tooie production tagging (design only)

Do **not** copy BK GeoCmd offsets. Do **not** hardcode model `0x62B`,
bones 79/100–108, or feather DL addresses.

Reset a per-submit occurrence counter at `func_800DE448` /
`func_800DE498` (one model instance). Increment in **logical**
GeoCmd order, including sort branches (process children in file
order; emit `gSPBranchList` if draw order differs, BK PR #22
concept only).

Emit `gEXMatrixGroup` + `gEXPopMatrixGroup` around:

| Site | Handler | Identity |
| --- | --- | --- |
| model root | `func_800DE498` start | instance base |
| compact GeoCmd2 bone | `func_800DCF48` after `G_MTX PUSH LOAD` | bone occurrence |
| type-0x11 own DL | inherits current bone group | no extra id |
| type-0x18 JOIN `G_POPMTX` parent verts | Tooie-specific | **new** parent occurrence before parent `G_VTX`; restore child group on trailing `G_MTX` |
| sort GeoCmd | Tooie sort type, if present | increment both children in logical order |
| billboard | Tooie billboard type, if present | own occurrence; skip rot if camera skipped |

Policy (PR #23 class, Tooie test): if the submit used any GeoCmd2
bone (`func_800DCF48` fired), use `G_EX_INTERPOLATE_SIMPLE` with
INTERPOLATE pos/rot/persp and SKIP vertex interp; else DECOMPOSE
for rigid unboned models.

`group_id = instance_base + occurrence_ordinal`. Instance base from
the DE448 `model_bin` / actor slot, not a hardcoded 0x62B constant.

Generalizes to backpack (`0x621`), Mumbo (`0x665`), and other boned
GeoCmd models.

## Task 7 — Banjo PR #22 / #23 (`PROVEN_STATIC`)

PR #22 `8b9ce251…`: `cur_model_transform_id_offset` in logical GeoCmd
order (bones, sort, billboard); `set_model_matrix_group`.

PR #23 `7e7d5d49…` / current `patches/transform_tagging.c`:
`cur_model_uses_bones ? G_EX_INTERPOLATE_SIMPLE : G_EX_INTERPOLATE_DECOMPOSE`.

Transfers: occurrence counter, push/pop groups, sort-stable logical
order, SIMPLE for boned models.

Does **not** transfer: BK GeoCmd2/3/1 layouts, map geo-list offsets,
`modelRenderModelBin` fields, ExtraSegments `unk28`, BK sort
`func_803385BC` opcode numbers, hardcoded BK transform-id ranges.

```text
PR22_CONCEPT_REQUIRED: YES
PR23_SIMPLE_POLICY_REQUIRED: YES
```

## Task 8 — production decision

Do **not** implement tagging in this task. SIMPLE numbers are not in
yet. Do not port PR22/23 until `VERSION=2` capture shows SIMPLE
removes or collapses the 44 CPU explosions.

If SIMPLE recapture is `YES` on `SIMPLE_REMOVES_ALL_GEOMETRIC_EXPLOSIONS`
or an overwhelming reduction, the next task implements the Tooie
GeoCmd tagging above (still no 0x62B special case).

```text
WORLD_MATRIX_INTERPOLATION_CAUSAL_STATUS: CORRECTED/SUPERSEDED
PRODUCTION_FIX_CANDIDATE_FOUND: NO
PRODUCTION_FIX_CLASS: NONE (PR23 SIMPLE REFUTED_FOR_CAPTURED_P09_EXPLOSIONS)
PRODUCTION_IMPLEMENTATION_READY: NO
```

VERSION=2 capture: 51 live explosions, 51 SIMPLE. Do not port PR22/23
from this SIMPLE hypothesis. See
[[p09-relative-view-world-interpolation-root-cause]].

## Build

```text
cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test
BUILD_RESULT: PASS (exit 0)
window_display_test: 38/38
tintro_projection_policy_test: 7/7
Tooie HEAD: ff5b6d9
RT64 HEAD: 6f1c2d9 (protected dirty; witness schema v2 uncommitted)
```

Witness files (protected RT64 dirty tree, no commit):

- `BanjoRecomp/lib/rt64/src/hle/rt64_p09_temporal_coherency.cpp`
- `BanjoRecomp/lib/rt64/src/hle/rt64_workload.h` (`P09_COHERENCY_SCHEMA_VERSION=2`)
