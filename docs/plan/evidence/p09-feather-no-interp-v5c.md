---
title: "P09 feather NO_INTERP V5C — complete visible-triangle transforms"
description: "V5B omitted JOIN parents 107/108; V5C extends default-off NO_INTERP to all nine world transforms used by visible feather triangles."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-09
tags: [tooie-recomp, p09, kazooie, feather, interpolation, v5c, join]
---

# P09 feather NO_INTERP V5C

Schema: `P09_FEATHER_NO_INTERP_V5C` (status field; parent diagnostic
schema remains `P09_SECONDARY_SWAY_KAZOOIE_FEATHER_V6C`).

Default-off. No GPU readback, copy, layout, sync, staging, or occlusion
query. No production interpolation change. Game was not launched by the
agent.

## Later correction (2026-09-09, occurrence identity)

User visual of V5C: `SAME`. That froze the **token-bound first G_VTX**
of each of the nine logical bones, not every concrete `worldIndex`
used by visible triangles. JOIN `G_POPMTX` parent slots still
interpolate. `TEMPORAL_MATRIX_INTERPOLATION_CAUSAL_STATUS:
REFUTED_FOR_ALL_TRANSFORMS_DIRECTLY_USED_BY_VISIBLE_FEATHER_TRIANGLES`
is `CORRECTED/SUPERSEDED`. Live untagged worlds still use
DECOMPOSE. See
[[p09-world-occurrence-identity-root-cause-audit]],
[[p09-boned-interpolation-counterfactual]].

## Later correction (2026-09-09)

User visual of V5C: `SAME`. Original-framerate graphics setting:
`FIXED`. Do not treat V5C as a global current-frame test. Next:
[[p09-high-rate-force-current-frame]].

## USER_OBSERVED (this session)

Kazooie head feathers, visual only (no recording):

- some frames completely correct
- other frames stretch a feather roughly 10–30× into a thin red/yellow streak
- feather base stays attached
- geometry does not disappear
- stationary: glitch rare, ~2% of visible frames (`USER_OBSERVED`)
- walking: ~30–50% of visible frames (`USER_OBSERVED`)
- subjective: alternating correct frames and incorrectly derived/intermediate frames
- severity rises with movement

## Task 1 — V5B coverage gap (`PROVEN`)

V5B `TOOIE_DIAG_FEATHER_NO_INTERP=1` gated on
`is_tracked_kazooie_feather_anim` / `classify_kazooie_feather` for only:

```text
V5B_NO_INTERP_BONES: 79,100,101,102,103,104,105
```

Visible JOIN DLs load parent vertices after `G_POPMTX` and mix them with
child vertices in the same triangles
([[p09-visual-geometry-identity-and-scope-forensic]],
[[p09-kazooie-feather-v6-post-matrix-vertex-path]]):

| DL | JOIN | Child verts | Parent verts after POP |
| --- | --- | --- | --- |
| `0x592` | 107↔103 | bone 103 | bone **107** |
| `0x61E` | 107↔100 | bone 100 | bone **107** |
| `0x4B0` | 108↔79 | bone 79 | bone **108** |
| `0x564` | 103↔104 | 104 | 103 (already in V5B) |
| `0x54C` | 104↔105 | 105 | 104 (already in V5B) |
| `0x5F0` | 100↔101 | 101 | 100 (already in V5B) |
| `0x5D8` | 101↔102 | 102 | 101 (already in V5B) |

Parent nodes (`PROVEN` `STATIC_ROM`):

- 107: compact type-2 `+0x12320`, INDEX 25, boneAnimID 107
- 108: compact type-2 `+0x12330`, INDEX 26, boneAnimID 108

```text
VISIBLE_TRIANGLE_WORLD_TRANSFORM_BONES: 79,100,101,102,103,104,105,107,108
PARENT107_USED_DIRECTLY_BY_VISIBLE_TRIANGLES: YES
PARENT108_USED_DIRECTLY_BY_VISIBLE_TRIANGLES: YES
V5B_COVERED_107: NO
V5B_COVERED_108: NO
V5B_WAS_COMPLETE_FOR_EVERY_TRANSFORM_USED_BY_VISIBLE_TRIANGLE_VERTICES: NO
```

Canonical label `TEMPORAL_MATRIX_INTERPOLATION_CAUSAL_STATUS=
REFUTED_FOR_VISIBLE_FEATHER_CHAIN` is `CORRECTED/SUPERSEDED`. It only
refuted interpolation of the seven child/crest bones, not of every
world transform used by visible feather triangle vertices.

## Task 2 — Mixed parent/child interpolation under V5B (`PROVEN` capability)

RT64 displayed position with F3DEX2 `vel=0` is
`worldMats[per-vertex worldIndex]`. JOIN triangles store mixed
world-indices (child slots then parent slots). V5B set
`prevMatrix = curMatrix = curTransform` only for tokens whose
boneAnimID was in `{79,100–105}`.

Under V5B a JOIN triangle **can** have:

- child vertices transformed by NO_INTERP current child matrices
- parent vertices transformed by the still-interpolated 107 or 108 matrix

```text
V5B_MIXED_PARENT_CHILD_INTERPOLATION_POSSIBLE: YES
```

V5B visual `SAME` therefore **cannot** refute temporal interpolation as
the cause of JOIN stretch. It can even increase parent/child mismatch
on `0x592` / `0x61E` / `0x4B0`. Do not restore the strong closure until
V5C is visually `SAME` with all nine `BONE*_HIT=YES` and
`ALL_NINE_NO_INTERP_RESULT_EQUALS_CURRENT=YES`.

Do not reopen: CPU handoff, worldIndex correctness, current guest
matrices, ProjectionProcessor current Q, culling, depth, texture, or
the animation sampler.

## Task 3 — BanjoRecomp PR #23 (`STRONG_INFERENCE` for mechanism)

BanjoRecomp `set_model_matrix_group` uses
`G_EX_INTERPOLATE_SIMPLE` when `cur_model_uses_bones`, else
`G_EX_INTERPOLATE_DECOMPOSE` (`patches/transform_tagging.c`).

```text
WHY_BANJO_BONED_MODELS_USE_SIMPLE: decomposed independent world-matrix interpolation of articulated bones can shear/stretch geometry even when both endpoint frames are valid; SIMPLE is BanjoRecomp's boned-model policy
HIERARCHICAL_INTERPOLATION_CAN_STRETCH_JOIN_GEOMETRY: YES
BANJO_PR23_NOW_CAUSALLY_RELEVANT: YES
```

Independent interpolation of parent world vs child world is not
hierarchy-consistent: `lerp(P) * lerp(L) ≠ lerp(P*L)`. A JOIN edge
between a parent-space vertex and a child-space vertex can elongate
on intermediate frames while both endpoint poses remain valid. That
matches the USER_OBSERVED 10–30× streak with the base attached.

SIMPLE is **not** implemented here. It is not a proven fix. If V5C is
later `FIXED`/`BETTER`, the next step is to audit/adapt Banjo's
boned-model interpolation policy (or a hierarchy-consistent equivalent),
not to leave permanent NO_INTERP on.

PR #23 relevance does **not** revive “just use SIMPLE” as the V5B-closed
primary fix for the seven child bones alone.

## Task 4 — V5C implementation (`IMPLEMENTED_STATIC`)

Extended the existing default-off seam only:

- `is_tracked_kazooie_feather_anim` includes 107 and 108
- `classify_kazooie_feather` accepts exact 0x62B nodes
  `+0x12320` INDEX 25 anim 107 and `+0x12330` INDEX 26 anim 108
- `kTrackedKazooieFeatherMask` `0x7F` → `0x1FF`
- status lists nine bones
- post-match counts `final == current` when `NO_INTERPOLATION=1`

Still required: live model `0x62B`, full sign-extended guest pointers,
DCF48 type-2 identity, token + vertex + match before override. Control
path unchanged when `TOOIE_DIAG_FEATHER_NO_INTERP` is unset.

107/108 `vertex_seen` is expected from their own type-0x11 children
(`0x524` under 107, `0x48F` under 108), plus JOIN parent `G_VTX` after
`G_POPMTX` if the token stack restores the parent. Status `BONE107_HIT`
/ `BONE108_HIT` will self-declare if that fails.

RT64 `TransformProcessor::process` is unchanged: when
`tooie_diag_rt64_no_interpolation` returns true it assigns
`prevMatrix = curMatrix = curTransform` and the witness final matrix
is `curTransform`.

```text
V5C_IMPLEMENTED: YES
ALL_NINE_NO_INTERP_RESULT_EQUALS_CURRENT: YES at logic level (override copies current); runtime confirmation is /tmp/tooie-secondary-sway-status.txt after the user run
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
PRODUCTION_FIX_CANDIDATE_FOUND: NO
```

## Task 7 — Decision tree (do not execute until visual result)

If user visual is `FIXED` or `BETTER`:

```text
TEMPORAL_PARENT_CHILD_INTERPOLATION: CAUSAL_OR_CONTRIBUTING
NEXT: audit/adapt BanjoRecomp boned-model SIMPLE matrix-group policy or a hierarchy-consistent interpolator for Tooie
```

Do **not** promote permanent NO_INTERP.

If user visual is `SAME` (and status shows all nine hits +
`ALL_NINE_NO_INTERP_RESULT_EQUALS_CURRENT=YES`):

```text
TEMPORAL_MATRIX_INTERPOLATION_CAUSAL_STATUS: REFUTED_FOR_TOKEN_BOUND_FIRST_OCCURRENCE_ONLY
  (later CORRECTED/SUPERSEDED as a full-slot closure; see occurrence audit)
```

If visual is `WORSE`, record it; do not invent a production fix.

## Validation (agent)

```text
BUILD_RESULT: PASS (cmake --build build --target TooieRecompiled, exit 0)
WINDOW_DISPLAY_TEST: PASS 38/38
PROJECTION_TEST: PASS 7/7
```

`git diff --check` on intended Tooie paths. No RT64 source edit.
No commit. No push. Game not launched.

## User test

```sh
TOOIE_DIAG_SECONDARY_SWAY=1 TOOIE_DIAG_FEATHER_NO_INTERP=1 ./build/TooieRecompiled
```

Do not also set `TOOIE_DIAG_FEATHER_INTERP_SIMPLE`. Enter combined
`0x62B` Kazooie and walk.

Before treating the visual as causal, confirm
`/tmp/tooie-secondary-sway-status.txt`:

```text
P09_FEATHER_NO_INTERP_EXPERIMENT=V5C
NO_INTERP_AB_ENABLED=YES
NO_INTERP_TARGET_MASK=0x1FF
BONE79_HIT=YES
BONE100_HIT=YES
BONE101_HIT=YES
BONE102_HIT=YES
BONE103_HIT=YES
BONE104_HIT=YES
BONE105_HIT=YES
BONE107_HIT=YES
BONE108_HIT=YES
ALL_NINE_NO_INTERP_RESULT_EQUALS_CURRENT=YES
```

If any `BONE*_HIT=NO` or `ALL_NINE_...=NO`, the visual is not a V5C
result.

User response only: `SAME` / `BETTER` / `FIXED` / `WORSE`.
No recording. No visual measurements.

```text
USER_VISUAL_TEST_REQUIRED: YES
VISUAL_RECORDING_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: user visual V5C test
```
