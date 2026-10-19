---
title: "P09 CPU diagnostic shader-semantics repair"
description: "Mechanical CPU clip repair to SPIR-V row-vector WQ; offline synth and static SPIR-V only. No production change, no capture."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-16
tags: [p09, rt64, shader, spirv, matrix, diagnostic, repair]
---

# P09 CPU diagnostic shader-semantics repair

Role: `P09_REPAIR_CPU_DIAGNOSTIC_SHADER_SEMANTICS`.
Bounded objective P09-RES-05: repair CPU diagnostic matrix action to match
compiled Vulkan RSPProcessCS. No new root-cause investigation. No production
policy. No game launch.

Authoritative audit:
[[p09-cpu-shader-matrix-semantics-and-homogeneous-audit]].

## Verdict

**PROVEN (`OTHER`)**: active P09 CPU clip evaluators now implement
`hlslpp::mul(pos4, hlslpp::mul(world, viewProj))`, matching SPIR-V
`OpMatrixTimesMatrix(W,Q)` then `OpVectorTimesMatrix(p,WQ)`. Physical WORLD
is `mul(pos4,WORLD)`. Camera-relative diagnostics are
`mul(pos4,mul(WORLD,VIEW))`. Production HLSL, upload layout, and matrix
storage are unchanged. Game root remains `UNRESOLVED`.

This is a measurement-system repair. Old CPU factorial/pose/WORLD metrics
and stale `VIEW_WORLD_ROOT_CLASS=CROSS_STREAM_TEMPORAL_COHORT_MISMATCH`
claims are `CORRECTED/SUPERSEDED` as shader-equivalent proofs. They are
not replaced by a new root class.

## Provenance and scope

- Tooie HEAD: `ff5b6d992f5bcdc1f9002a2c351fc68d018e3bcf` (dirty; pre-existing).
- RT64 HEAD: `6f1c2d99a4ea571c139f449c326fd176ba8f3496` (dirty; pre-existing).
- RT64 root: `/home/wesley/projetos/N64-Recomp/BanjoRecomp/lib/rt64`.
- Static/CPU artifacts: `/tmp/p09-cpu-shader-semantics-repair-20260916/`.
- `GAME_LAUNCHED=NO`. `GPU_READBACK_USED=NO`.
- Production shader SHA-256 unchanged from the audit:
  `RSPProcessCS.hlsl` `5159782f633dc248eb4002dca48195634a9a49d23f6bd921fd594ebe13ca18e6`.
- Processor after repair:
  `rt64_rsp_processor.cpp` SHA256 `c57975501833ce049aeb23bfd0b8a5dbe2ee0b73185feb07bc7e5fe4265d3c78`.
- Offline test:
  `tests/p09_cpu_shader_clip_semantics.cpp` SHA256 `44d3379de7ac70da2ca79248e8424b1dbb92cb3079aa6adc3e3a2803dd03ce7b`.

## Canonical helper

In `src/render/rt64_rsp_processor.cpp` (anonymous namespace):

```text
clip = hlslpp::mul(pos4, hlslpp::mul(world, viewProj))
Q    = hlslpp::mul(view, projection)   // when V and P are separate
```

`P09_CANONICAL_CLIP_HELPER_SEMANTICS=ROW_VECTOR_TIMES_WORLD_TIMES_VIEWPROJ`.

Status schema: `SCHEMA=P09_RSP_ENDPOINT_BINDING_V2_SHADER_EQUIVALENT`.

## Active CPU clip consumers repaired (14)

All 14 active tfPos/clip evaluators now use the helper or the same
row-vector product:

| Consumer | Location |
| --- | --- |
| `p09FactorialClip` | helper return |
| sidecar PREV / CURRENT | endpoint sidecar |
| witness PREV / CURRENT | CPU witness |
| LIVE clip | live sample |
| PREV / CURRENT / LIVE triangle clips | pairing samples |
| rigid-world clip | rigid WORLD clip |
| `p09SameCohortCompositeClip` | `mul(pos, lerp(W*Q_prev, W*Q_cur))` |
| `p09CohortAlignedLiveClip` | helper |
| `p09MappingCoherentLiveClip` | helper |
| `p09PoseDerivedLiveClip` | helper |

POLICY_A / xf / draw / comp remain snaps of repaired LIVE/CURRENT NDC.

`P09_ACTIVE_CLIP_CONSUMER_COUNT=14`.
`P09_ACTIVE_CLIP_CONSUMERS_REPAIRED=14`.
`P09_ACTIVE_CLIP_CONSUMERS_STILL_OLD_SEMANTICS=0`.

## Retired arms left on old convention (not active)

MODELVIEW (`mul(mul(interpVW, liveProj), livePos)`) and hierarchy
(`mul(mul(Q, hierWorld), pos4)` plus `mul(hierWorld, pos4)` xyz) remain
in-file as `SUPERSEDED` measurement arms. They are not converted and are
not used as current truth. Status keys emit `SUPERSEDED`.
`VIEW_WORLD_ROOT_CLASS=SUPERSEDED_UNRESOLVED` (does not emit
`CROSS_STREAM_TEMPORAL_COHORT_MISMATCH` as current truth).
`CLIP_LERP_NDC_LERP_CLOSED=SUPERSEDED`.

## Physical WORLD and camera-relative

`p09PhysicalWorldPoint` = `mul(pos4, world)`. Affine test is last-column
`(0,0,0,1)`. Affine samples with `w≈1` contribute WORLD JOIN xyz.
Non-affine samples are counted separately and are not used as affine JOIN
proof. No hierarchy production fix.

Camera-relative: `p09CameraRelativePoint` = `mul(pos4, mul(world, view))`.
Diagnostic only; not the production clip evaluator.

## Pose arm (measurement only)

`p09ClassifyRigidCameraPose` reject reasons are separate:
Inverse / Nonaffine / Decompose / Scale / Skew / Perspective / Determinant.
Actual `poseView(0)` vs PREV_VIEW and `poseView(1)` vs CURRENT_VIEW use
scale-aware tolerance (floor `1e-4`, rel `1e-5 * max entry mag`).
Endpoint mismatch does not count as inverse. Gates are not loosened.
`POSE_VIEW_CAUSAL_RESULT=NOT_CLAIMED`.

## Offline synthetic fixtures (independent scalar algebra)

`p09_cpu_shader_clip_semantics_test` (EXCLUDE_FROM_ALL). Independent
`C[r][c]=sum_k A[r][k]*B[k][c]` and row-vector `out[c]=sum_r p[r]*M[r][c]`.

Fixture A: `W=T(10,20,30)`, `Q=I`, `p=(1,2,3,1)` → `(11,22,33,1)`.
Fixture B: `W=mul(mul(S(2,3,4), Rz90), T(10,20,30))` with explicit XY
`Rz(+90)` `[[0,1,0,0],[-1,0,0,0],[0,0,1,0],[0,0,0,1]]` (RT64
`matrixRotationZ` rotates YZ and is not used); `Q=mul(T(-2,5,7), S(0.5,2,1))`
→ `(1,54,49,1)`.
Fixture C: old `mul(mul(Q,W),p)` on A is `(1,2,3,141)`; differs from A.

```text
cmake --build build --target p09_cpu_shader_clip_semantics_test
./build/p09_cpu_shader_clip_semantics_test
```

Exit 0. `p09_cpu_shader_clip_semantics: 9/9 passed`.
`SYNTH_TRANSLATION_MATCH=YES`.
`SYNTH_NONCOMMUTING_MATCH=YES`.
`SYNTH_OLD_AND_NEW_DIFFER_AS_EXPECTED=YES`.

## Static SPIR-V cross-check (no GPU)

Same DXC flags as production / the audit (`-spirv -fspv-target-env=vulkan1.0
-fvk-use-dx-layout -E CSMain -T cs_6_3`; no `-Zpr`):

```sh
RT64_ROOT=/home/wesley/projetos/N64-Recomp/BanjoRecomp/lib/rt64
LD_LIBRARY_PATH="$RT64_ROOT/src/contrib/dxc/lib/x64" rtk proxy \
  "$RT64_ROOT/src/contrib/dxc/bin/x64/dxc-linux" \
  -spirv -fspv-target-env=vulkan1.0 -fvk-use-dx-layout -I"$RT64_ROOT/src" \
  -E CSMain -T cs_6_3 "$RT64_ROOT/src/shaders/RSPProcessCS.hlsl" \
  -Fo /tmp/p09-cpu-shader-semantics-repair-20260916/RSPProcessCS.spv \
  -Fc /tmp/p09-cpu-shader-semantics-repair-20260916/RSPProcessCS.spvasm
```

Exit 0. Live clip SSA: `%139=viewProjTransforms` (Q), `%141=worldTransforms`
(W), `%178=OpMatrixTimesMatrix %141 %139` (W*Q),
`%188=OpVectorTimesMatrix %187 %178` (p*(W*Q)). Endpoint clips use the same
`OpMatrixTimesMatrix(W,Q)` then `OpVectorTimesMatrix(p,WQ)`.

`RSPPROCESS_STATIC_SEMANTICS=OpMatrixTimesMatrix(W,Q)_THEN_OpVectorTimesMatrix(p,WQ)`.
`CPU_HELPER_STATIC_SEMANTICS_MATCH=YES`.

SPV SHA256 `a8030768476991d17a826a4aea2e81b7d75b6614198d4f10092c289331719a8f`.

## Builds and regression tests (no game launch)

```text
cmake -S . -B build                         # exit 0
cmake --build build --target TooieRecompiled  # exit 0; rebuilt rt64_rsp_processor.cpp
./build/window_display_test                 # exit 0; PASS 38/38
./build/tintro_projection_policy_test       # exit 0; PASS 7/7
git diff --check                            # Tooie exit 0
git -C ../BanjoRecomp/lib/rt64 diff --check # RT64 exit 0
```

Status write path, ~1 Hz, mutex, atomic tmp+rename, and bounded accumulators
are unchanged.

## What this does not claim

Do not treat VIEW, WORLD, VIEW×WORLD, hierarchy, or cohort mismatch as the
game root. `ROOT_CAUSE_CLASS=UNRESOLVED`. No production HLSL/upload change.
No user capture is requested by this task.

## Closing checks

- Generated `RecompiledFuncs/**` not edited.
- Pre-existing Tooie/RT64 dirty worktrees preserved (no reset/restore/checkout/clean/stash/pull/rebase).
- No commit, no push.
- Stable specs reviewed; no new production contract.
- Canonical RE: [[../../reverse-engineering/systems/transform-interpolation]].

```text
P09_STATUS_SCHEMA_VERSIONED: YES
P09_CANONICAL_CLIP_HELPER_SEMANTICS: ROW_VECTOR_TIMES_WORLD_TIMES_VIEWPROJ
P09_CANONICAL_CLIP_HELPER_MATCHES_SPIRV: YES
P09_ACTIVE_CLIP_CONSUMER_COUNT: 14
P09_ACTIVE_CLIP_CONSUMERS_REPAIRED: 14
P09_ACTIVE_CLIP_CONSUMERS_STILL_OLD_SEMANTICS: 0
P09_PHYSICAL_WORLD_POINT_HELPER: mul(pos4,WORLD)
P09_CAMERA_RELATIVE_HELPER: mul(pos4,mul(WORLD,VIEW))
P09_REFERENCES_REBUILT_FROM_CORRECTED_CLIPS: YES
P09_PAIRING_RECOMPUTED_AFTER_SEMANTIC_REPAIR: YES
P09_PATHOLOGY_LABELS_RECOMPUTED: YES
WORLD_ATTACHMENT_USES_FULL_ROW_VECTOR_ACTION: YES
P09_INVALID_LEGACY_ARMS_RETIRED_OR_SUPERSEDED: YES
POSE_VIEW_ACTUAL_ENDPOINT_PATH_CHECKED: YES
POSE_VIEW_REJECTION_REASONS_SEPARATED: YES
SYNTH_TRANSLATION_MATCH: YES
SYNTH_NONCOMMUTING_MATCH: YES
SYNTH_OLD_AND_NEW_DIFFER_AS_EXPECTED: YES
CPU_HELPER_STATIC_SEMANTICS_MATCH: YES
OLD_CPU_MATRIX_SEMANTICS_SUPERSEDED: YES
ROOT_CAUSE_CLASS: UNRESOLVED
PRODUCTION_FIX_UPDATED: NO
CLEANUP_READY: NO
COMMIT_READY: NO
USER_CAPTURE_REQUIRED: NO
NEXT_SINGLE_ACTION: return repaired diagnostics + offline validation only; do not request game capture until all acceptance checks pass
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
GAME_LAUNCHED: NO
```
