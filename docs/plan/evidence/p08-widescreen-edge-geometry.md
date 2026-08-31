---
title: "P08 — WIDESCREEN_EDGE_GEOMETRY static causal enumeration"
description: "WIDESCREEN_EDGE_GEOMETRY: T_INTRO 16:9 first RT64 divergence is CLIP_X (K=4/3 AUTO on cinematic viewProj 0.905148->0.678861). Guest presentation-envelope Original/Expand diff=0 REFUTED as root. 21:9+ authored extent PROVEN/separate."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-31
tags: [p08, widescreen, rt64, scissor, viewport, projection, culling, residual]
---

# P08 — WIDESCREEN_EDGE_GEOMETRY

## CURRENT (2026-08-31) — T_INTRO_TRUE_SOURCE_ASPECT_PRODUCTION

The user visually confirmed the true-source-aspect policy for the tested 16:9
Scene A and Scene B paths. The production Tooie seam derives the cinematic
source aspect from `abs(P[1][1]/P[0][0])` and preconditions only T_INTRO's
projection before RT64's existing automatic adjustment. It is render-only,
dynamic, and does not modify RT64, geometry, `800CAA24`, `800CACEC`, or
`CAB70`.

```text
PRODUCTION_POLICY: true cinematic source-aspect compensation
SOURCE_ASPECT_HARDCODED: NO
CANDIDATE_A_FINAL_STATUS: REMOVED
CANDIDATE_B_ENV_FLAG_FINAL_STATUS: REMOVED; production default
TINTRO_TRUE_SOURCE_ASPECT_VISUAL_RESULT: VISUALLY_CONFIRMED_FOR_TESTED_16X9_SCENES
TINTRO_21X9_PLUS_FINAL_STATUS: OPEN_REVALIDATION_REQUIRED
PROJECTION_POLICY_REGRESSION_TEST: ADDED / PASS
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
```

## SUPERSEDED (2026-08-31) — T_INTRO_PROJECTION_SOURCE_ASPECT_CANDIDATES

The first RT64 divergence is the CPU-side projection adjustment, not a guest
owner/cull gate: RT64 assumes source `304/228` and applies `K=4/3` at 16:9.
Two default-off Tooie-side probes are implemented at the projection owner.
Candidate A pre-multiplies by `K_auto` to preserve the guest projection;
Candidate B pre-multiplies by `min(target, cinematic_source)/(4/3)`, with
`cinematic_source = abs(P[1][1]/P[0][0]) ≈ 1.767441860`. This makes Candidate B
preserve the cinematic framing up to its source aspect and expand only beyond
it. Both candidates are T_INTRO-only, render-only, and leave 21:9+ authored
extent as a separate proven limitation.

```text
RT64_PROJECTION_TRANSLATION_ROOT: HIGH / RETAINED
EXISTING_ASPECT_EXTENSION_API: PARTIAL
CANDIDATE_A_FLAG: TOOIE_TEST_TINTRO_PROJECTION_ORIGINAL
CANDIDATE_B_FLAG: TOOIE_TEST_TINTRO_TRUE_SOURCE_ASPECT
CANDIDATE_A_IMPLEMENTED: YES / DEFAULT OFF
CANDIDATE_B_IMPLEMENTED: YES / DEFAULT OFF
CPU_WITNESS_COMPARISON: PASS
ASPECT_MATRIX: PASS (4:3, 16:10, cinematic native, 16:9, 21:9, 32:9)
SCENE_A_CANDIDATE_ACTIVE: YES
SCENE_B_CANDIDATE_ACTIVE: YES
GAMEPLAY_CANDIDATE_ACTIVE: NO
TINTRO_21X9_PLUS_AUTHORED_EXTENT: PROVEN / SEPARATE
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS (38/38)
USER_MANUAL_TEST_REQUIRED_NOW: YES
PREFERRED_USER_TEST_CANDIDATE: B
NEXT_SINGLE_ACTION: TOOIE_TEST_TINTRO_TRUE_SOURCE_ASPECT=1 ./build/TooieRecompiled
ARTIFACT_DIRECTORY: /tmp/p08-tintro-projection-candidate-20260831T034438Z/
ARTIFACT_CHECKSUM_MANIFEST: 665ae932cfe019b1cebd04fa6dc1f78c6b70f8d3388faee6c5522b1e0f7474ed
GPU_READBACK_USED: NO
```

## CURRENT (2026-08-31) — T_INTRO_RT64_TRIANGLE_TRANSLATION_DIVERGENCE

`PROVEN`. Five covering Scene-A frame-5 triangles from leaf `0x0031f6d8`
were compared guest-golden vs RT64 CPU Original vs Expand. Guest RSP
`posTransformed` MATCHES captured clip (`0017aff0` m00=`0.905148`, recompute
err `0.002537`). Expand `ProjectionProcessor::adjustProjectionMatrix`
multiplies the first column by `projRatioScale=1/K` with
`K=aspectRatioTarget/aspectRatioSource= (16/9) / (304/228) = 4/3`.
Live cinematic: `pre_vp00=0.905148` → `post_vp00=0.678861`. The old
0.584795% guest-projection-aspect delta is the **wrong K**.

Do **not** reintroduce `±0.754` as horizontal crop. That value is the
vertical cinematic letterbox. `CORRECTED_ORIGINAL_EXPAND_OWNER_DIFFERENCE_COUNT=0`
and `RT64_PRESENTATION_TRANSFORM_AS_ROOT: REFUTED` remain closed. This
round's Original-vs-Expand triangle difference (`5/5` at CLIP_X) is the
GPU `modViewProj` scale, distinct from the guest owner remap.

```text
WITNESS_TRIANGLE_COUNT: 5
GUEST_GOLDEN_VALUES_PROVEN: YES
RT64_GFX_COMMAND_DECODE_FUNCTION: GBI_F3DEX2::vertex / tri1 / tri2
RT64_VERTEX_LOAD_FUNCTION: RSP::setVertex -> setVertexCommon
RT64_TRIANGLE_EMIT_FUNCTION: RSP::drawIndexedTri
RT64_TRANSFORM_FUNCTION: RSP::computeModelViewProj; setVertexCommon tfPos=mul(v,mvp)
RT64_PROJECTION_MODIFICATION_FUNCTION: ProjectionProcessor::adjustProjectionMatrix
RT64_VIEWPORT_TRANSFORM_FUNCTION: RSP::setViewport; FramebufferRenderer::convertViewportRect
RT64_SCISSOR_FUNCTION: RDP::setScissor; FramebufferRenderer::convertFixedRect
RT64_RASTER_STATE_TRANSLATION_FUNCTION: State::loadDrawState
FIRST_RT64_DIVERGENCE_STAGE: CLIP_X
FIRST_RT64_DIVERGENCE_FUNCTION: RT64::adjustProjectionMatrix / ProjectionProcessor::processScene
FIRST_RT64_DIVERGENCE_FILE: src/render/rt64_projection_processor.cpp
RT64_EXPAND_STAGE: ProjectionProcessor::processScene (CPU, after HLE, before GPU)
RT64_EXPAND_EQUATION: matrix[row][0] *= 1/K; AUTO if origin NONE && scissor covers FB width && width>height
RT64_SOURCE_ASPECT: 1.333333333
RT64_TARGET_ASPECT: 1.777777778
RT64_EFFECTIVE_K: 1.333333333
RT64_ORIGINAL_EXPAND_TRIANGLE_DIFFERENCE_COUNT: 5
MAX_RT64_EDGE_VERTEX_NDC_DELTA: 2.423362; in-|x|<=1.05: 0.058037; unit-clip: 0.250000
MAX_RT64_EDGE_VERTEX_PIXEL_DELTA_EQUIV: 8.82 guest-px at 304-wide (unit-clip 38 px); ~46.4 host-px at 1600
RT64_TRANSFORM_CAN_EXPLAIN_LARGE_VISUAL_FAILURE: YES
CLIP_CLASSIFICATION_MATCH: NO
WITNESS_WINDING_MATCH: YES
EXPAND_FLIPS_ANY_WITNESS_WINDING: NO
GUEST_RT64_RASTER_STATE_MATCH: YES
SCENE_B_SHOWS_SAME_RT64_DIVERGENCE: YES
TINTRO_16X9_RT64_ROOT_CLASS: RT64_PROJECTION_TRANSLATION
ROOT_CONFIDENCE: HIGH
TINTRO_16X9_AUTHORED_EXTENT: STRONG_HYPOTHESIS
TINTRO_21X9_PLUS_AUTHORED_EXTENT: PROVEN
CAMERA_FRAMING_EXPOSES_AUTHORED_EXTENT: not promoted for 16:9 from invalid ±0.754
FIX_LAYER: TOOIE
FIX_SCOPE: TINTRO
CANDIDATE_IMPLEMENTED: diagnostic-only
GPU_READBACK_USED: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: implement a Tooie-side default-off T_INTRO/cutscene candidate that prevents RT64 AUTO from applying K=4/3 to the cinematic viewProj (prefer cutscene Original or scene-local skip of G_EX_ASPECT_AUTO), then re-check Scene-A frame 5 walls vs this Expand scale; do not change RT64 globally
NEXT_RECOMMENDED_MODEL: GPT-5.6 Luna
ARTIFACT_DIRECTORY: /tmp/p08-tintro-rt64-triangle-translation-20260831T032400Z/
ARTIFACT_CHECKSUM_MANIFEST: 4971dfafeea24b2c44d365c9df11350dbbcc4bb7f172991550de54731ff3f07a
```

Witness IDs:

| WITNESS_ID | TRI_INDEX | GID | CLASS | FINAL_OWNER |
| --- | ---: | --- | --- | --- |
| A_LEFT_WALL | 77 | f5281fc8 | WALL | WALL |
| B_RIGHT_WALL | 81 | 61f2aa16 | WALL | WALL |
| C_B1_EXPECTED_WALL | 47 | 6a470e60 | WALL | B1_EXPECTED_WALL |
| D_B1_LATER_COMPETING | 78 | 75749e8e | WALL | B1_LATER_OWNER |
| E_CENTERISH_CONTROL | 67 | f0dd1ea1 | UNKNOWN | UNKNOWN |

Clip mismatches under Expand: A `OUT(+X)`→`OUT(-X)`; C `OUT(-X)`→`IN_OR_CLIP`.
Scene B uses the same workload-global K (sample leaf `0x0031f058`).
Candidate is Tooie T_INTRO/cutscene only: RT64 AUTO is generic 4:3-VI
design; T_INTRO already uses widescreen combined VP plus vertical
letterbox inside a 4:3 VI, then AUTO applies K=4/3 again. Do not change
RT64 globally. Do not widen CAB70. Do not force WALL7799. Do not add
geometry.

## CURRENT (2026-08-31) — T_INTRO_PRESENTATION_ENVELOPE_RECONCILIATION

The prior `ORIGINAL_EXPAND_OWNER_DIFFERENCE_COUNT=16872` is
`REFUTED/SUPERSEDED`. The number is exactly the old analyzer's
`count(abs(2*(x+0.5)/304-1)>0.754)` over 304x228 pixels. That `0.754` came
from `vis_half=presented/src_a` in the removed historical intro-wall probe and was
applied on the wrong axis/space.

For the actual cinematic pass, guest scissor `(0,28)-(304,200)` means
`ORIGINAL_N64_VISIBLE_GUEST_NDC_X=[-1,+1]` and
`ORIGINAL_N64_VISIBLE_GUEST_NDC_Y=[-0.754385965,+0.754385965]`. RT64
Original is host pillarbox and Expand is host full-width, but both present
the full guest X range. Corrected Original-vs-Expand owner difference is `0`.
The 1.767441860-to-1.777777778 aspect delta is only `0.584795%`, not a broad
wall-loss mechanism. Do not promote `CAMERA_FRAMING_EXPOSES_AUTHORED_EXTENT`
for current 16:9 from this invalid `±0.754` X test.
`TINTRO_16X9_AUTHORED_EXTENT` remains a separate `STRONG_HYPOTHESIS`.
`RT64_PRESENTATION_TRANSFORM_AS_ROOT` is `REFUTED` for the old envelope
explanation.

Artifact: `/tmp/p08-tintro-presentation-reconciliation-20260831T025942Z/`
(manifest SHA `c08eeab9d38cf89be1c79b8184bb95e67b53ab3e703ad118901d856cb4dd6dc3`).

## CURRENT (2026-08-31) — TRUSTED_CPU_RDP_PIXEL_OWNER_REFERENCE

The missing CPU owner layer was closed with the materialized local Angrylion
RDP Plus CPU reference at pinned HEAD `9c8b9ed3e7d7f00dff8bc872ccdd3fba1a3673fc`.
The replay calls the reference fixed-point edgewalker and span coverage path
through an owner-only framebuffer callback. It does not use a generic
barycentric rasterizer, OpenGL rules, or GPU readback.

```text
OWNER_REPLAY_EXECUTION_ORDER_PROVEN: YES
RDP_SUBPIXEL_RULES_FROM_REFERENCE: YES
RDP_COVERAGE_RULES_FROM_REFERENCE: YES
SCENE_A_OWNER_UNRESOLVED_PIXEL_COUNT: 0
SCENE_A_WALL_OWNER_LEFT_20: 60.82%
SCENE_A_WALL_OWNER_RIGHT_20: 19.31%
B1_ACTUAL_OVERLAP_PIXEL_COUNT: 5988
B1_WALL_COVERAGE: 5988 x 8/8
B1_ALT_COVERAGE: 5987 x 8/8 + 1 x 7/8
B1_WALL_WRITE_ELIGIBILITY: YES
B1_ALT_WRITE_ELIGIBILITY: YES
B1_FINAL_OWNER: draw 67 WALL for 4224; draw 78 UNKNOWN for 1764
PROVEN_PIXEL_OWNER_MISMATCH_COUNT: 5988 exact-wall-ID mismatches
PRIMARY_PIXEL_OWNER_CAUSE: LATER_DRAW_WINS
ORIGINAL_EXPAND_OWNER_DIFFERENCE_COUNT: 16872 host-envelope pixels
RDP_PIXEL_OWNERSHIP_AS_WIDESCREEN_ROOT: REFUTED (guest-side)
GUEST_CPU_RENDER_PATH_AS_ROOT: STRONGLY_WEAKENED
```

The exact expected wall ID `7c8e8bba7a3ac2a9` is draw index 47 (captured
order 48); the alternate `941fcfacdf264cf3` is index 12 (captured order 13).
Both are write-eligible on their actual overlap. Later eligible draws own the
final samples, while a wall-class later owner remains for 4224 samples. The
Original/Expand comparison retains identical guest clip input and only
changes the documented presented NDC envelope, ±0.754 versus ±1.000.

Artifact: `/tmp/p08-tintro-rdp-owner-reference-20260831T011600Z/` (manifest
SHA `dbf77934a31b2244a5b5af1568413a03c88a0ce823f3886f4b102a4fe9cbf83e`).

## CURRENT (2026-08-30) — T_INTRO_VISIBLE_SURFACE_OWNERSHIP_SCENE_A_VS_B

The corrected F3DEX2 walker was extended as a default-off CPU diagnostic and
captured Scene-A frame 5 and Scene-B frame 2409. It now walks the real task
roots (`0x80160cd0`/`0x80159600`), honors `G_DL_PUSH=0` call/return and
`G_DL_PUSH=1` branch semantics, and follows state-only child lists. RDP E2/E3
SETOTHERMODE and FC combiner commands are decoded through inheritance; the
prior leaf-only render-state capture was insufficient.

```text
ENV_SUBMITTED_TRIANGLE_COUNT:          156
ENV_RENDER_ELIGIBLE_TRIANGLE_COUNT:    156
GUEST_VIEWPORT:                        304x228
LEFT_TOP/MIDDLE/BOTTOM_OCCUPIED:       100% / 100% / 100%
RIGHT_TOP/MIDDLE/BOTTOM_OCCUPIED:      100% / 100% / 100%
MEANINGFUL_EMPTY_COMPONENT_COUNT:      0
PRIMARY_HOLE:                          NONE_PROVEN
TARGET_ONLY_HOLE_COUNT:                0
CONTENT_ID:                            SHA256(local ordered xyz+UV/color+timg+geometry state), 16 hex
CONTENT_ID_STABLE_ACROSS_RELOCATION:   REFUTED (A/B exact shared IDs: 0)
SCENE_B_FIRST_ENV_FRAME:                2409; env tris=80 (0031f058/0031f6d8/0031fa38)
GEOMETRY_CONTENT_ID_STABLE:             REFUTED; DRAW_STATE_ID_STABLE: REFUTED
SURFACE_CLASSES:                        A wall=46/ceiling=8/unknown=16; B wall=18/floor=4/ceiling=5/unknown=22
VISUAL_MISMATCH_COMPONENT_COUNT:        5 geometric overlap candidates; effective owner UNKNOWN
PRIMARY_MISMATCH:                        B-1 RIGHT, NDC x=.601974.. .996711, y=-.995614..-.092105
WRONG_OWNER_CAUSE:                      NOT_PROVEN (B-1 wall/alternate state matches; RDP blender/coverage owner unresolved)
SCENE_A_TASK_ROOT:                      0x80160cd0 (0x80159600 also observed)
SCENE_B_TASK_ROOT:                      0x80159600 (0x80160cd0 also observed)
B1_OVERLAP_TRIANGLE_COUNT:              32 unique candidates (10 walls, 22 non-walls)
B1_WALL_STATE:                           2CYCLE/BILERP/TEXTPERSP/FORCE_BLEND/ALPHA_NONE/Z_COMPARE=NO/Z_UPDATE=NO
B1_OTHER_STATE:                          2CYCLE/BILERP/TEXTPERSP/FORCE_BLEND/ALPHA_NONE/Z_COMPARE=NO/Z_UPDATE=NO
B1_EFFECTIVE_VISIBLE_OWNER:              UNKNOWN (coverage/blender not CPU-resolvable here)
STATE_CAUSED_MISMATCH_COUNT:             0 proven
SCENE_B_EQUIVALENT_CAMERA_FOUND:        YES; same camera pose, different captured environment content
SCENE_A_VISUAL_ROOT_CLASS:             MORE_EVIDENCE_REQUIRED (MEDIUM)
SCENE_B_VISUAL_ROOT_CLASS:             MORE_EVIDENCE_REQUIRED
IMPLEMENTED_CANDIDATE:                 NO
ARTIFACT_DIRECTORY:                    /tmp/p08-tintro-render-state-owner-20260830T180800Z/
ARTIFACT_CHECKSUM_MANIFEST_SHA:        3d342e999430f817d0333a539968f87935c5b66a848e29864e857edd6a1c3b14
GPU_READBACK_USED:                     NO
```

This round adds Scene-B per-triangle rows, pointer-free IDs, surface
classification, screen-cell overlap stacks, and CPU NDC depth ordering. The
captures share no exact environment content IDs (A=70 unique, B=49 unique),
and produce five wall-versus-other geometric overlap candidates. The captured
environment leaves contain no B9/BA other-mode commands, so Z compare/update,
alpha, and effective RDP ownership remain UNKNOWN; the nearest CPU-depth
candidate is not reported as visible. No fix or geometry was added. Next:
retain the task-root render-state commands and rerun the effective-owner pass.

## Prior (2026-08-30) — T_INTRO_PER_DRAW_TRANSFORM_OWNERSHIP

Runtime draw provenance (`TOOIE_DIAG_TINTRO_DRAW_PROVENANCE=1`, default
OFF) replaces the static OPA-7417 + assumed-opening-transform owner.

```text
SCENE_A_FIRST_ENV_FRAME:      5
SCENE_A_FIRST_ENV_LAST_FRAME: 60 (12 sampled env tasks)
SCENE_A_FIRST_HOLE_WITNESS_FRAME: 5
OPA7417_ZERO_TRIANGLE_ROOT:   WRONG_RESOURCE + WRONG_MODEL_MATRIX
SCENE_A_LEFT_EDGE_DRAW:       frame 5 draw 3
SCENE_A_LEFT_EDGE_GDL:        0x0031f6d8 (main) / 0x0031f8f8 (materials)
SCENE_A_LEFT_EDGE_RESOURCE:   T_INTRO_ENV_0031F (seg01=0x003202b8, seg02=0x00316d80)
SCENE_A_LEFT_EDGE_MODEL_MATRIX: 0x8017b0b0 id 7ef0783b T(camera)
SCENE_A_LEFT_EDGE_GEO_NODE:   UNKNOWN
SCENE_A_RIGHT_EDGE_DRAW:      frame 5 draws 3–8
SCENE_A_RIGHT_EDGE_GDL:       0x0031f6d8 / 0x0031f8f8
SCENE_A_RIGHT_EDGE_RESOURCE:  T_INTRO_ENV_0031F
SCENE_A_RIGHT_EDGE_MODEL_MATRIX: 0x8017b0b0 T(camera)
SCENE_A_RIGHT_EDGE_GEO_NODE:  UNKNOWN
PRIMARY_HOLE_FRAME:           5
PRIMARY_HOLE_SIDE:            LEFT
PRIMARY_HOLE_VERTICAL_REGION: MIDDLE+BOTTOM (lt=7 lm=6 lb=2; 0027f100 is 2D overlay)
PRIMARY_HOLE_GDL:             0x0031f6d8
PRIMARY_HOLE_GEO_NODE:        UNKNOWN
PRIMARY_HOLE_EDGE_VERTICES:   6,7,8
REAL_HOLE_FIRST_DROP_STAGE:   NO_CPU_DROP
REAL_HOLE_FIRST_DROP_FUNCTION: func_80013EF0
A_B_EQUIVALENT_DRAW_OWNER:    NO
A_B_EQUIVALENT_GDL:           NO (heap 0x0031f* differs; only 0x00123340 setup shared)
A_B_EQUIVALENT_MODEL_MATRIX:  YES
A_B_EQUIVALENT_HOLE_BOUNDARY: NO
A_B_EQUIVALENT_FIRST_DROP:    YES
SCENE_A_VISUAL_ROOT_CLASS:    MORE_EVIDENCE_REQUIRED
SCENE_B_VISUAL_ROOT_CLASS:    MORE_EVIDENCE_REQUIRED
SHARED_VISUAL_ROOT:           STRONG_HYPOTHESIS
CHOSEN_SOLUTION_CLASS:        MORE_EVIDENCE_REQUIRED
IMPLEMENTATION_COMPLEXITY:    LOW_MEDIUM
IMPLEMENTED_FINAL_FIX:        NO
NEXT_SINGLE_ACTION:           Dense CPU NDC occupancy grid of 0x0031f6d8/0x0031f8f8 at Scene-A frame 5 (exclude 0x0027f100); content-stable ID for Scene B
NEXT_RECOMMENDED_MODEL:       GPT-5.6 Luna
ARTIFACT_DIRECTORY:           /tmp/p08-tintro-per-draw-owner-20260830T193644Z/
ARTIFACT_CHECKSUM_MANIFEST:   88a9885d0abc13fb0e0bf3d12008058ee8c89e00404598876148644bb5a66fa9
GPU_READBACK_USED:            NO
```

G_DL `0x00123340` is F3DEX2 setup only (`G_MOVEWORD` / `G_SETOTHERMODE` /
`G_ENDDL`; peek ends `df`). F3DEX2 `G_DL_PUSH=0` is a call; treating
`0xDE00` as branch aborted later env lists. First triangles are at
frame 5. Env modelview `0x8017b0b0` is translation-only equal to camera
`(529.3794, 6943.8896, -1904.1279)`; verts are model-local (~±2793),
not static OPA-7417 world. Submitted env already reaches NDC x=−1.19;
CPU left 20% is **not** empty. Type-13 rejected leaves remain **not**
the hole owner. Heap G_DL addresses are not content-stable across Scene
B (AABB ~±22474, extra `0x00234790`). Do not widen CAB70. Do not force
WALL7799. Do not add geometry.

This residual is separate from USER_DEMO_5 locomotion. USER_DEMO_5 remains
`RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. P08 remains
`ACTIVE / MANUAL_VALIDATION_PENDING`. P09 remains
`PENDING / NOT_FORMALLY_OPEN`. This note does **not** reopen those chains.

## Prior (2026-08-30) — T_INTRO_SCREEN_EDGE_OWNER_AUDIT

The bounded CPU/static ownership audit does **not** promote the type-13
reject into the user-visible hole root.

```text
SCENE_A_OPENING_FRAME:        3 (stable VP sample frame 315)
SCENE_A_OPENING_CAMERA:       (529.3794, 6943.8896, -1904.1279)
OPA_7417_SOURCE_TRIANGLES:    5254
OPA_7417_TRIANGLES_IN_CLIP:   0
SUBMITTED_SCREEN_EDGE_OWNER:  NONE_PROVEN
TARGET_ONLY_HOLE_COUNT:       0 for OPA 7417 at the tested pose
TYPE13_REJECTED_LEAF_IS_REAL_HOLE_OWNER: NO
SCENE_B_USES_SAME_HOLE_BOUNDARY: YES for the candidate boundary
SCENE_A_VISUAL_ROOT_CLASS:    MORE_EVIDENCE_REQUIRED
SCENE_B_VISUAL_ROOT_CLASS:    MORE_EVIDENCE_REQUIRED
SHARED_VISUAL_ROOT:           STRONG_HYPOTHESIS
CHOSEN_SOLUTION_CLASS:        MORE_EVIDENCE_REQUIRED
IMPLEMENTATION_COMPLEXITY:    MEDIUM
IMPLEMENTED_FINAL_FIX:        NO
NEXT_SINGLE_ACTION:           Capture first user-visible Scene-A hole frame with CPU-only per-draw model-transform/DL ownership, then replay in Scene B
NEXT_RECOMMENDED_MODEL:       Grok 4.6 Medium
ARTIFACT_DIRECTORY:           /tmp/p08-tintro-screen-edge-owner-20260830T190608Z/
ARTIFACT_CHECKSUM_MANIFEST:   1d9fad539b87a0c7ffd4fdeffa7840c1a97bf8f8ab148596d0a1660ce7f1f21b
GPU_READBACK_USED:            NO
```

The exact opening pose is materially different from the later `kOpenCam`
wall sample. At frame 315, all extracted OPA 7417 triangles are outside the
guest clip volume. The six type-13 lateral leaves remain LEFT rejects, with
best candidate NDC xmax `-1.13195` (and closest plane distance
`+34.2265015` at `0x8037beb8`), so they cannot own an on-screen edge. That
zero-triangle result is now `WRONG_RESOURCE + WRONG_MODEL_MATRIX`.

## Prior (2026-08-30) — T_INTRO_TYPE13_OPENING_LEFT_REJECT

CPU-only `TOOIE_DIAG_TINTRO_TYPE13=1` (default OFF). Isolated
`APP_FOLDER_PATH=/tmp/tooie-intro-16x9-xdg/TooieRecompiled` Expand.

At SCENE_A opening camera (529.38, 6943.89, -1904.13), live CAB70 planes
come from viewport `0x801e0c60` (`*0x8012D500` is the pointer). Mapping
PROVEN: plane0 LEFT, plane2 TOP, plane3 BOTTOM; plane1 RIGHT/FAR mixed.

Six unique high-Y left-edge type-13 AABBs first-reject on LEFT. Closest:
`0x8037beb8` p-vertex (520, 7020, -2980) signed distance **+34.2265015**
threshold 0. Control `0x8037b328` passes LEFT (−287.55) and fails BOTTOM.

Guest `src_aspect=1.767442`, host `tgt=1.777778`, `k=1.005848`.
`SOURCE_REJECT_TARGET_ACCEPT: NO` at 16:9 and 21:9+/32:9. `FOUR_THREE_IDENTITY: PROVEN` (k=1 no-op). Scene B replay: same camera, same AABBs, same distances. Banjo GeoCmdD class YES; do not copy `TRUE`.

`TOOIE_TEST_INTRO_TYPE13_ASPECT` not added. NDC xmax of those leaves ≤ −1.13
so they sit left of guest NDC −1. Next: submitted on-screen edge owners.
Artifact `/tmp/p08-tintro-type13-reject-plane-20260830T184303Z/`
(manifest `93b735b8ad433c291ea32340fae6627b268047636987a553f504f3909cfe967a`).

## Prior (2026-08-30) — TWO_INTRO_SCENES_SEPARATED

CPU-only `TOOIE_DIAG_STARTUP_SCENES=1` (default OFF). Isolated
`APP_FOLDER_PATH=/tmp/tooie-intro-16x9-xdg/TooieRecompiled` Expand.
Does **not** enable the frontend START sequencer.

Stable names (do not say “the intro” for both):

| Name | Definition | Runtime identity |
| --- | --- | --- |
| `SCENE_A_FIRST_VISIBLE_AFTER_LAUNCH` | First real 3D after app launch | epoch 1, frame 3, mode=3 map=0, G_DL `0x00123340`, cam (529.38, 6943.89, -1904.13) |
| `SCENE_B_NEW_GAME_INTRO` | Later new-game/file-select replay | frontend epoch 5, frame 2454, mode=3 map=0, **same** G_DL, opens at SCENE_A camera |

Map-struct is 0 on both. Do **not** assign MAP_18B. Overlay 676 `a0=0x18B`
is a resource-load hint only.

`PRIOR_WALL7759_INVESTIGATION_BELONGS_TO: SCENE_A` (late first-play
kOpenCam, leftover summary `mode=3 map=0` `open_frame=6946`). That work
is **kept**, not discarded, but it is not SCENE_B.

Shared renderer/visibility/projection: YES. Failure class both
`VISIBILITY_REJECT` (`func_800E3DC0`/`func_800CAB70`). DK64 internal-X
pattern: NO. Shared-root HIGH (same model+gate, two plays). No production
fix: SCENE_A opening-pose reject plane is not value-proven. Artifact
`/tmp/p08-two-intro-scenes-20260830T181514Z/`
(manifest `4aae5e68e62eb61f5fc7dba8e6b9007d06ba70f61e5da1e762f680bde56b048a`).

## Prior (2026-08-30) — WALL7799_TYPE13_AABB_TOP_CULL

CPU-only. Exact-AABB `TOOIE_DIAG_INTRO_WALL_DL` historical probe (removed).
Isolated
`APP_FOLDER_PATH=/tmp/tooie-intro-16x9-xdg/TooieRecompiled` 1600x900
Expand. No GPU. No `TOOIE_TEST_INTRO_7799_VIS`. Gameplay `800CAA24` /
`800CACEC` unchanged (log-only).

### Type-13 layout (PROVEN from MAP 18B OPA 7417 blob)

24-byte header. Child at `node+child_offset` (typically +24). `+04` is
next-sibling byte delta (0 = last sibling, still visited), **not** Banjo
`size_4` container span.

| off | type | meaning |
| --- | --- | --- |
| +00 | u32 | cmd = 13 |
| +04 | u32 | next sibling byte delta |
| +08 | s16[3] | AABB min xyz |
| +0E | s16[3] | AABB max xyz |
| +14 | s16 | child offset |
| +16 | s16 | radius (sphere companion; **not** this predicate) |

Drawable child cmd=3: `+08` gfx start, `+0A` gfx count.

`TYPE13_HANDLER_FUNCTION`: `func_800E3DC0` @ `0x800E3DC0`
(`a0=min`, `a1=max` → `jal 800CAB70`).
`TYPE13_PREDICATE`: `func_800CAB70` 4-plane p-vertex
(`n·p+d >= 0` → CULLED). Unique geo-cmd jal is OPEN (indirect table).
`TYPE13_SEMANTIC_CLASS`: `AABB_FRUSTUM_GATE`. Prior DRAW_DISTANCE
label is `CORRECTED/SUPERSEDED`.

### WALL7799 parent chain (opening ~6954)

`0x81708` CONTAINER ACCEPT → siblings include `0x81940` WALL7644
(submitted) and `0x81968` CONTAINER ACCEPT → children:

| node | AABB y | last_vis | G_DL |
| --- | --- | ---: | --- |
| `0x81980` WALL7759/7779 | 2635–3780 | 1 | YES |
| `0x819A8` WALL7799 | 5954–6200 | 0 | NO |
| `0x819D0` gfx 7809 | 4910–5954 | 0 mixed | YES |
| `0x819F8` gfx 7864 | 3780–4910 | 1 | NO (later first_rsp=11484) |

### First reject / original semantics

`WALL7799_FIRST_REJECT`: `func_800E3DC0` / `func_800CAB70`.
Clip reconstruction of AABB `(-671,5954,-4114)-(471,6200,-3001)` through
live view×proj: first failing plane = **TOP**; NDC y=[+2.57,+5.70];
L/R/B do not cull. `ORIGINAL_N64_WOULD_REJECT_7799: YES` HIGH.
Role **A**: later/upward camera; same leaf `first_rsp=14802`.
DK64 `80658E8C` / `80658134` are screen-space X-bound class:
`WALL7799_GATE_MATCHES_DK64_INTERNAL_X_BOUND_CLASS: NO`.
Closer analog: Banjo GeoCmdD. Do not port DK64 addresses.

`WALL7799_ORIGINAL_GATE_ACCEPT: NO`.
`WALL7799_TARGET_VIEW_NEEDS_CONTENT: YES`.
`SOURCE_REJECT_TARGET_NEEDS: NO` (same guest cinematic; TOP not X).
Part 9 requires both coverage YES **and** SOURCE_REJECT_TARGET_NEEDS YES:
candidate **not** implemented.

If forced, triangles cover ndc x=[-1.000,-0.529] y=[+0.175,+1.000]
(area 0.449; hole_frac 0.944 of y≥0.15 × x[-1,-0.70]); TIMG matches
7644. Exact y=+0.15 scanline stays EMPTY even with 7799.

Residual `0x819F8`: AABB ACCEPT, no G_DL at opening. Do not fold into
7799 vis.

```text
INTRO_16X9_ROOT: UNKNOWN
INTRO_16X9_MISSING_DL_ROOT: REFUTED (7759/7779/7644/SKY18/WALL7799); OPEN (0x819F8)
CANDIDATE_IMPLEMENTED: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: Prove why type-13 leaf 0x819F8 (gfx 7864-7897) is AABB-accepted (last_vis=1) at opening ~6954 but not G_DL-appended until first_rsp=11484; CPU-only; do not force WALL7799
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-intro-wall7799-gate-20260830T173445Z/
ARTIFACT_CHECKSUM_MANIFEST: 30ad57994bc6635bdf97d1f44c832f80f740811813178084d0c295b2c41f41f4
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS 38/38
GPU_READBACK_USED: NO
```

## Prior (2026-08-30) — MAP18B_ORIGINAL_PRESENTATION_ENVELOPE

CPU-only clip of **submitted** WALL7759/7779/7644 and SKY18 through live
guest view-projection (71 VP dumps; opening frame 6954). No GPU. No
geometry extension. Isolation:
`APP_FOLDER_PATH=/tmp/tooie-intro-16x9-xdg/TooieRecompiled`.

### Truth corrections

- `INTRO_16X9_CPU_CULL_ROOT`: `REFUTED` (submitted walls).
- `INTRO_16X9_MISSING_DL_ROOT`: `REFUTED` for 7759/7779/7644/SKY18;
  `OPEN` for WALL7799 (never G_DL, both aspects).
- `INTRO_16X9_PRESENTATION_REVEALS_ORIGINALLY_CROPPED_NDC`: `REFUTED`
  as a 4:3/16:9 **X-crop**. Guest scissor is full-width 304/304 of a
  1.767 cinematic; N64 intro X is guest NDC ±1. Host Original
  vis_half=±0.754 is `INFERENCE` and the wrong axis (Y letterbox).
- `INTRO_GUEST_PROJECTION_ALREADY_WIDE`: `PROVEN` (1.767442).
- `INTRO_16X9_NEEDS_NEW_GEOMETRY`: `NO`.
- `INTRO_21X9_PLUS_AUTHORED_EXTENT`: `PROVEN` (keep separate).
- `Clamp16x9`: `REFUTED`. Groups 81/84: `REFUTED`.

### Opening coverage (submitted only)

| y NDC | orig x=-0.754 | expand x=-1 | notes |
| --- | --- | --- | --- |
| ≤ +0.05 | WALL | WALL | 7759/7779 continue to clip |
| +0.10 | EMPTY | WALL | 7759 only outside 4:3-inferred edge |
| ≥ +0.15 | EMPTY | EMPTY | 7644 sliver ~[-0.96,-0.71]; 7799 not submitted |

Right: no wall; SKY `[0.53,1.0]` at y~0.19–0.63; else EMPTY.

WALL7799 ROM triangles would cover xmin=-1, y=[0.175,1.0] if submitted.
That skip is now proven original AABB TOP cull (current); do not treat
this prior `STRONG_HYPOTHESIS` as still open.

```text
INTRO_16X9_ROOT: STRONG_HYPOTHESIS (upper-left empty without 7799)
RECOMMENDED_16X9_SOLUTION_CLASS: MORE_EVIDENCE_REQUIRED
ORIGINAL_PRESENTATION_PRESERVATION: do NOT pillarbox ±0.754
GEOMETRY_EXTENSION_16X9_NECESSARY: NO
NEXT_SINGLE_ACTION: Prove why nested leaf 0x819A8 (WALL7799 gfx 7783-7807) is never G_DL-appended; CPU-only; do not extend geometry
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-intro-original-presentation-envelope-20260830T163614Z/
ARTIFACT_CHECKSUM_MANIFEST: 2c2ffed179c34aafc57a3040f83ffc175b4c24257abc2761be7ef895884a8fea
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS 38/38
GPU_READBACK_USED: NO
IMPLEMENTED_CANDIDATE: NO
```

## Prior (2026-08-30) — MAP18B_16X9_EDGE_DRAW_SUBMISSION_AND_FIRST_DROP_GATE

CPU-only historical `TOOIE_DIAG_INTRO_WALL_DL` probe (removed). No GPU. No geometry
extension. Gameplay `800CAA24`/`800CACEC` unchanged (log-only). Isolated
runs used `APP_FOLDER_PATH` (XDG_CONFIG_HOME is ignored). Unique G_DL
matcher this round (shared SETTIMG no longer aliases 7799→7644).

### Truth corrections

- `INTRO_ROOT_STATUS: PROVEN CAMERA_FRAMING_EXPOSES_OUTSIDE_MODEL` is
  `CORRECTED/SUPERSEDED` for the **current 16:9** failure.
- `INTRO_16X9_AUTHORED_EXTENT_AS_ROOT`: `REFUTED/SUPERSEDED`.
- `INTRO_21X9_PLUS_AUTHORED_EXTENT`: `PROVEN` (keep separate).
- `GROUP81/84_AS_ROOT`: `REFUTED`.
- `Clamp16x9`: `REFUTED`.

### 4:3 Original vs 16:9 Expand (opening pose)

| DL | 4:3 RSP | 16:9 RSP | missing at 16:9 |
| --- | ---: | ---: | --- |
| WALL7759 | 597 | 600 | NO |
| WALL7779 | 407 | 409 | NO |
| WALL7644 | 514 | 515 | NO |
| WALL7799 | 0 | 0 | NO |
| SKY18 | 1524 | 1530 | NO |

WALL7799 G_DL 7783–7807 is absent at **both** aspects. Not a 16:9 miss.

`DL_PRESENT_4_3_MISSING_16_9: NO`. `INTRO_16X9_FIRST_DROP_STAGE:
NO_CPU_DROP`. `INTRO_WALL_DL_SUBMISSION: PROVEN_PRESENT`.

CPU clip of gfx-7759 verts is the same at both aspects (`ndc_x` −1.15 /
−1.43). Guest projection stays cinematic `vp+0xE8=1.767`.
`CPU_CLIP_SPACE_DIFFERENCE: NONE`. `DROP_ASPECT_DEPENDENT: NO`.
Guest scissor at opening is `0,28,304,172` of `304x228` (1.767 letterbox).
Host presented vis NDC: Expand `[-1.000,1.000]` vs Original `[-0.754,0.754]`.
`HOST_VISIBLE_NDC_DIFFERENCE: YES`.

`INTRO_16X9_ROOT: STRONG_HYPOTHESIS` — Expand presents the cinematic
left clip edge at the window; 4:3 Original crops vis to ±0.754. Not
missing triangles, not a 16:9 DL drop.

```text
INTRO_MODEL_TRAVERSAL_FUNCTION: type-13 DRAW_DISTANCE geo (nested 0x81708)
INTRO_LEAF_VISIBILITY_FUNCTION: func_800CAB70 via func_800E3DC0
INTRO_DL_APPEND_FUNCTION: geo leaf G_DL (observed in GraphicsTask)
INTRO_RSP_SUBMIT_FUNCTION: func_80013EF0
INTRO_SPECIAL_GEO_GATE: nested type-13 AABB 0x81980/0x81940/0x819A8
PRIOR_TEST_COVERED_THIS_EXACT_PATH: NO
NEXT_SINGLE_ACTION: CPU-clip WALL7759 against ndc x=-1 vs x=-0.754
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-intro-16x9-dl-drop-20260830T160704Z/
ARTIFACT_CHECKSUM_MANIFEST: 9ca8fa3c374c75b81594fa5f535633ff977f2a542bbb11389646d6ccbdca553a
BUILD_RESULT: PASS
GPU_READBACK_USED: NO
IMPLEMENTED_FINAL_FIX: NO
```

## Prior (2026-08-30) — MAP18B_OPENING_WALL_REAL_TRIANGLE_BINDING

CPU-only OPA+XLU vertical edge sweep. No GPU. No production intro fix.
Clamp16x9 stays removed. Gameplay `800CAA24`/`800CACEC` unchanged.

### Corrections (PROVEN / CORRECTED)

- Groups **81/84** remain exact leaves but are **REFUTED** as the
  opening-hole root (`GROUP81_AS_OPENING_HOLE_ROOT`,
  `GROUP84_AS_OPENING_HOLE_ROOT`).
- AABB `LIVE_MAX_SAFE_ASPECT 1.3363:1` stays `CORRECTED/SUPERSEDED`.
- XLU **7418** is a ROM **alias** of OPA 7417 (same packed word
  `0x31776004` / abs `0x00C708A4`). Prior parse of `0xCB0D3C` as XLU
  7418 is `CORRECTED/SUPERSEDED` (that blob is asset **7420**).
- Type-13 `size=872` parent **196** contains nested leaves; gfx 7759 is
  **not** in the sequential size-40 list.

### Gfx 7759 (left wall)

Nested leaf `dl_start=7721` `dl_count=30` under parent 196, AABB
`(-894,2635,-4305)-(767,3780,-2810)`. TRI2 sub1 ids `(4582,4575,4581)`
world `(-200,3781,-4235)/(542,2981,-4087)/(292,3781,-4037)`. CI 64x64
`0x020120E0` **WRAP,WRAP**, shade*tex, lighting off, opaque vertex
color. Role: **WALL**. Same family: gfx 7644/7760/7779/7799/7881.

### Gfx 18 (right far)

Linear group 0 DL also contains a huge sky/horizon card. Verts
`(6011,4900,±)` with **vertex alpha 0** at the top. CI 64x64
`0x0201AF80` WRAP,CLAMP. Role: **BACKGROUND**. Right `screen_y=0` misses
this card at 4:3; only `y=+0.50` hits.

### Coverage

Frame 6955 LEFT 4:3 through 16:9: near WALL family (HIGH). RIGHT: sky /
`NO_MODEL_GEOMETRY` already at 4:3. 16:9 does **not** lose the left
wall. All 297 live poses: only **one** 16:9 near-wall miss (frame 1255
LEFT `y=+1`). 21:9/32:9 LEFT runs into FAR gfx 12015/12034 or misses.

`GEOMETRY_EXTENSION_NOW_PROVEN: NO`. Do not extend 81/84 or 7759 yet.

```text
GFX7759_GROUP: nested 196/7721
GFX7759_VISUAL_ROLE: WALL
GFX18_GROUP: 0
GFX18_VISUAL_ROLE: BACKGROUND
XLU_EDGE_COVERAGE_PRESENT: NO
SOURCE_4_3_EDGE_COVERAGE_COMPLETE: NO
TARGET_ONLY_HOLE_COUNT: 0 at 16:9/6955; 13 at 21:9+32:9/6955 left
HIGH_Y_HOLE_CONFIRMED_BY_LIVE_RAYS: NO
GEOMETRY_EXTENSION_NOW_PROVEN: NO
TOOIE_MATCHES_BANJO_PATTERN: LOW
NEXT_SINGLE_ACTION: Bind gfx 10993/3214/8336 and frame-1255 LEFT y=+1 miss
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-intro-real-edge-owner-20260830T145138Z/
ARTIFACT_CHECKSUM_MANIFEST: 380e7c2aec4d849f6fb3d7197173adfd2f533066890e3d497ec74c8fdcd4560f
BUILD_RESULT: SKIP
GPU_READBACK_USED: NO
```

## Prior (2026-08-30) — MAP18B_INTRO_BOUNDARY_OWNERSHIP_AND_GEOMETRY_EXTENSION_CANDIDATE

CPU parse of OPA `0x1CF9` / 7417. No GPU. No production intro fix.
Clamp16x9 stays removed and `REFUTED_FOR_CURRENT_16X9`. Gameplay
`800CAA24`/`800CACEC` unchanged.

### Ownership (PROVEN)

Previous CSV "type-3 groups" are type-13 DRAW_DISTANCE AABB leaves
(`CORRECTED/SUPERSEDED`). Gfx opcode is in `w0` (`CORRECTED/SUPERSEDED`).
Group **81**: DL gfx 2561–2585, 7 verts (1402..1408), 3 tris, materials
`0x02008540` then `0x02008D60`, wrap **CLAMP**. Wall tri gfx 2583 ids
`(1404,1405,1406)`. Group **84**: DL gfx 2643–2691, 21 verts, 9 tris,
five sequential CI materials, wrap **CLAMP**. Type-13 `dl_count` matches
those triangle counts.

### Live rays vs those leaves

Frame 6955 frustum-edge rays do **not** hit groups 81/84. Left el=0 hits
gfx 7759 (`0x020120E0`). Right el=0 **misses** OPA even at 4:3; only
el=+7.68 hits far gfx 18 (`0x0201AF80`). AABB-based
`LIVE_MAX_SAFE_ASPECT 1.3363:1` overstated triangle coverage
(`CORRECTED/SUPERSEDED`).

### Architecture

Banjo `hotpatch_intro_opa_map_model` is still the structural pattern
(`PARTIAL`): map-load Gfx overwrite after material, host Vtx, hash guard,
static world extra geo, not aspect-derived. Tooie UVs clamp, so continued
ST stretches the edge texel. High-Y aerial hole exists separately (groups
219/223, timg `0x02000020`). No candidate under
`TOOIE_TEST_INTRO_GEOMETRY_EXTEND`. `CHOSEN_ARCHITECTURE:
MORE_EVIDENCE_REQUIRED`.

```text
GROUP81_VERTEX_COUNT: 7
GROUP81_TRIANGLE_COUNT: 3
GROUP84_VERTEX_COUNT: 21
GROUP84_TRIANGLE_COUNT: 9
MATERIAL_STATE_REUSABLE: YES
TEXTURE_CAN_REPEAT: NO
STATIC_EXTENSION_FEASIBLE: YES
STATIC_EXTENSION_SAFE: NO
DYNAMIC_EXTENSION_FEASIBLE: YES
TOOIE_CAN_USE_SAME_ARCHITECTURAL_PATTERN: PARTIAL
HIGH_Y_SEPARATE_HOLE_EXISTS: YES
CHOSEN_ARCHITECTURE: MORE_EVIDENCE_REQUIRED
CANDIDATE_IMPLEMENTED: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: Bind opening-shot wall to live frustum-edge triangle hits (gfx 7759 / gfx 18) plus XLU 7418
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-intro-geometry-extension-candidate-20260830T143035Z/
ARTIFACT_CHECKSUM_MANIFEST: 6000f3f889a2693191175de272570e69ae73d93cd4be8eead9580c7adaa213e4
BUILD_RESULT: SKIP
GPU_READBACK_USED: NO
```

## Prior (2026-08-30) — MAP18B_LIVE_CAMERA_AND_WALL_ENVELOPE_CAPTURE

Isolated CPU diagnostic `TOOIE_DIAG_INTRO_CAMERA=1` (removed). No GPU.
No production intro fix. Clamp16x9 candidate stays removed and
`REFUTED_FOR_CURRENT_16X9`. Gameplay `800CAA24`/`800CACEC` unchanged.

### Camera owner / live pose

`MAP18B_CAMERA_OWNER` is the `D_8012D500` viewport plus `func_800CB124`
and host `func_800E44FC`. Overlay 441 `chintrocam` MAP_18B case 13 is
empty (initializer only; 6355 live hits). Position is viewport+0x00.
Forward is viewport+0x0C look vector (`PROVEN` = `-column2` of `v_rot`).
VFOV is `2*atan(1/|viewport+0xF4|)`; viewport+0x18 is **not** VFOV
(`CORRECTED/SUPERSEDED`). Live cinematic VFOV is **30.7071°**. Target
aspect at 1600x900 Expand/Clamp16x9 is **1.777778**; viewport+0xE8 is
already **1.767442**, so Clamp16x9 equals Expand.

`D_80127630` map never became 395 (`map18b_calls=0`). Intro identity is
T_INTRO `mode=3 map=0`. The flythrough loops; after file-select A
(`t=59.68s`) the opening pose `(529.38, 6943.89, -1904.13)` replays.

### Live envelope vs OPA 7417 groups

Representative alcove `(3986,0,6200)` occupancy: **0**. Do not promote
the 60° / 1.4849:1 inference.

Constraining live descent **frame 6955**:
cam `(-1243.8077, 3682.9285, -1254.0330)`, look
`(0.74934, 0.00698, -0.66215)`. Type-3 **group 84** left /
**group 81** right. 4:3 covers; 16:10+ fail **RIGHT**.
`LIVE_MAX_SAFE_HORIZONTAL_FOV` 40.2985°;
`LIVE_MAX_SAFE_ASPECT` 1.3363:1.

Opening aerial y>5000 looks slightly up above type-3 maxY (~3400) and
has nvis=0 in the vertical frustum. It is not the 4:3-vs-16:9
discriminator.

### Camera vs geometry

`DYNAMIC_CAMERA_FULL_WIDTH_FEASIBLE: NO` — keeping 16:9 HFOV 52.04° at
this VFOV exceeds the 40.30° safe span; shrinking VFOV crops ~25%
vertically; dolly-in crops; translation cannot add angular width.

`GEOMETRY_EXTENSION_FEASIBLE: YES` / `MEDIUM`. Banjo
`hotpatch_intro_opa_map_model` similarity **HIGH** (add verts/tris after
existing wall material, continue ST, fixed world-space). Tooie vertex
set / material Gfx index / ST for groups 81/84 remain `UNKNOWN`.
`BOUNDARY_PLANAR: NO` (volumetric groups).

`INTRO_FINAL_SOLUTION_CLASS: MORE_EVIDENCE_REQUIRED`.

```text
INTRO_MAP_ID: 395 / 0x18B (runtime field stays 0 during T_INTRO)
INTRO_OPA_MODEL_ID: 0x1CF9 / 7417
INTRO_REAL_VFOV: 30.7071
INTRO_REAL_TARGET_ASPECT: 1.777778
INTRO_REAL_TARGET_HFOV: 52.0369
INTRO_WALL_GEO_GROUP: 81 / 84
LIVE_MAX_SAFE_ASPECT: 1.3363:1
SAFE_4_3: YES
SAFE_16_10: NO
SAFE_16_9: NO
SAFE_21_9: NO
SAFE_32_9: NO
DYNAMIC_CAMERA_FULL_WIDTH_FEASIBLE: NO
GEOMETRY_EXTENSION_FEASIBLE: YES
GEOMETRY_EXTENSION_COMPLEXITY: MEDIUM
INTRO_FINAL_SOLUTION_CLASS: MORE_EVIDENCE_REQUIRED
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: CPU-parse OPA 7417 vertex/material/DL ownership for live groups 81/84
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-intro-live-camera-envelope-20260830T133604Z/
ARTIFACT_CHECKSUM_MANIFEST: 3c4b0229c98c233364abc69def03f61f23749e21f2c11dac5216b3c913dbf8d7
BUILD_RESULT: PASS
GPU_READBACK_USED: NO
```

## Prior (2026-08-30) — INTRO_ARBITRARY_ASPECT_AUDIT

CPU/static parse of `MAP_18B_CS_STARTUP_INTRO` OPA/XLU, plus the user’s
authoritative visual result for the previous candidate. No GPU. The native
projection-hook Clamp16x9 candidate is removed after **NO_CHANGE** at 16:9;
gameplay `800CAA24`/`800CACEC` remain unchanged and production.

### Failed INTRO_ASPECT_CLAMP candidate (2026-08-30)

The candidate in `src/game/transform_hooks.cpp` recognized mode 3 and scaled
the original projection coefficient by `(4/3)/target_aspect`. At the user’s
16:9 window, `Clamp16x9` returns exactly 16:9, so the candidate reproduced the
same widened target as Expand rather than preserving 4:3. The user tested the
same broken opening shot and reported **NO_CHANGE**. The 41-line candidate
hunk was removed; no generated code or unrelated dirty hunk was touched.

The cleanup build passes and `window_display_test` remains 38/38, but those
checks do not prove visual correction. The current design decision is
`MORE_EVIDENCE_REQUIRED`.

### Arbitrary-aspect framing audit

For the representative inferred pose `(3986,0,6200)`, forward `+Z`, depth 743,
and alcove X bounds 3349..4654, the safe horizontal FOV is 81.2153° and the
safe aspect at 60° VFOV is 1.4849:1. Therefore 4:3 is safe; 16:10, 16:9,
21:9, and 32:9 are unsafe for that representative pose. The live pose and
VFOV remain **UNKNOWN** because no isolated ROM/runtime capture is available.

Dynamic camera adaptation cannot preserve both original vertical composition
and the finite authored boundary without crop/vertical loss or pillarbox-like
side treatment. Geometry extension is structurally plausible around groups
0..6 and +X group 62, but planar/material-aware boundary ownership is not
proven. Do not inject geometry yet.

### Work performed

Identified exact map table IDs from `gcmapDll` row `0x18B`. Inflated
ROM assets 7417/7418. Parsed BKModelBin vertex lists (hdr 0x18) and
Tooie Gfx (opcode in `Gfx.w1`). Recovered geo-group AABBs. Recovered
`chintrocam` MAP_18B case 13 empty (generic `80085708`). Constructed
representative alcove rays at VFOV 60° (INFERENCE). Compared Banjo
`hotpatch_intro_opa_map_model` / `ASSET_149D` without porting verts.

### Proved facts

- `INTRO_OPA_MODEL_ID` `0x1CF9` / 7417 and `INTRO_XLU_MODEL_ID`
  `0x1CFA` / 7418 (`PROVEN`).
- OPA AABB min `(-8758,-690,-7780)` max `(8609,6600,8326)`; 7768
  AABB-filtered verts; DL `G_VTX=851` tris~=`5254` (`PROVEN`).
- XLU AABB min `(-8240,-145,-4573)` max `(2131,2559,2093)`; 458 verts
  (`PROVEN`).
- Dense geo-group X overall `-7797..5563`; opening alcove
  `X 3349..4654` (`PROVEN` for parsed type-3 groups).
- `INTRO_LATERAL_WALL_SOURCE_GEO_EXISTS: YES`;
  `INTRO_LATERAL_WALL_DL_EXISTS: YES`.
- Representative 4:3 view covered; Expand/16:9 rays exit alcove
  (`PROVEN` for constructed pose; live MAP_18B pose still `UNKNOWN`).
- `INTRO_PRIMARY_CLASS: CAMERA_FRAMING_EXPOSES_OUTSIDE_MODEL`.
- `INTRO_FIRST_DROP_GATE: NONE_PROVEN`. Stop culling search.

### Refuted facts

- Missing intro walls as another generic cull-function failure after
  `800CAA24`/`800CACEC` (`REFUTED` as primary).
- `EXISTING_GEOMETRY_SUPPRESSED` as primary (`REFUTED`).
- `chintrocam` MAP_18B empty case as a drop gate (`REFUTED`).
- Packed asset table words as CIC-encrypted (`CORRECTED/SUPERSEDED`:
  `(offset<<6)|type`).

### Unknowns

- Exact live MAP_18B camera xyz/forward (`UNKNOWN`).
- `INTRO_LATERAL_WALL_DL_SUBMITTED` (`UNKNOWN`; INFERENCE submitted
  because the interior is visible).
- Exact live MAP_18B camera xyz/forward/VFOV and the projection inputs at the
  wall-disappearance shot (`UNKNOWN`).
- Material/UV/render-state ownership of the boundary triangles (`UNKNOWN`).
- Whether a procedural extension can cover all required aspects without
  unintended composition or material changes (`UNKNOWN`).

```text
INTRO_MAP_ID: 395 / 0x18B
INTRO_OPA_MODEL_ID: 0x1CF9 / 7417
INTRO_XLU_MODEL_ID: 0x1CFA / 7418
INTRO_OPA_AABB: min=(-8758,-690,-7780) max=(8609,6600,8326)
INTRO_XLU_AABB: min=(-8240,-145,-4573) max=(2131,2559,2093)
INTRO_PRIMARY_CLASS: CAMERA_FRAMING_EXPOSES_OUTSIDE_MODEL
INTRO_ROOT_STATUS: PROVEN
RECOMMENDED_INTRO_SOLUTION_CLASS: MORE_EVIDENCE_REQUIRED
INTRO_CLAMP16X9_VISUAL_RESULT: NO_CHANGE
FAILED_CLAMP_CHANGE_REMOVED: YES
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_SINGLE_ACTION: Obtain one CPU-only live MAP_18B wall-disappearance-shot capture of camera pose, projection VFOV, and active aspect; no GPU readback
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-intro-arbitrary-aspect-fix-design-20260830T122919Z/
ARTIFACT_CHECKSUM_MANIFEST: SHA256SUMS inside artifact directory
BUILD_RESULT: PASS (cleanup build; no final intro fix)
GPU_READBACK_USED: NO
```

## Prior (2026-08-30) — CLEAN_PINNED_BUILD_VALIDATION

Isolated worktrees at Tooie `23c070a9`, BanjoRecomp `ec859632`, RT64
`6f1c2d99` configured and linked `TooieRecompiled` after applying
versioned `patches/runtime` to the isolated N64ModernRuntime only.
`CLEAN_PINNED_BUILD_VALIDATION: PASS`. Capture ABI no-ops are unused by
clean RT64 (`CAPTURE_ABI_NEEDED_BY_CLEAN_BUILD: NO`). Smoke SKIP: no
isolated ROM. Intro walls remain a separate residual.

```text
GAMEPLAY_MAP_ROOT: resolved / production fix active
GAMEPLAY_OBJECT_ROOT: resolved / production fix active
CLEAN_PINNED_BUILD_VALIDATION: PASS
CLEAN_BUILD_RESULT: PASS
CLEAN_SMOKE_RESULT: SKIP
INTRO_WALL_RESIDUAL: STILL_PRESENT
NEXT_DISCRIMINATOR: INTRO_MAP_18B_SOURCE_EXTENT_VS_EXPAND_FRUSTUM
NEXT_SINGLE_ACTION: CPU dump MAP_18B_CS_STARTUP_INTRO OPA/XLU vertex XZ vs source 4:3 and target Expand
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-clean-milestone-validation-20260830T035059Z/
ARTIFACT_CHECKSUM_MANIFEST: af72f29f96026b3d678d0f6914ea21b6aeaae648ea6c9fe25a4e5e1ea7976228
```

## Prior (2026-08-30) — OBJECT_FRUSTUM_PRODUCTION + INTRO_WALL_ROOT

User ran `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`. Gameplay objects
A/B stay rendered at the lateral edge. Intro walls still disappear.
Promoted the same k H-expand as `800CAA24` to production (`ON` unless
`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0`). DRAW-only (`800E3E8C`,
`8011447C`); original v0 for `8011458C` and `chlightfader`. Attract
62s production `new_accept=279` `semantic_kept=0`. Intro class:
`SOURCE_GEOMETRY` / `CAMERA_FRAMING` (`STRONG_HYPOTHESIS`); Banjo
needed extra intro OPA wall/grass triangles because generic cull was
insufficient. Map `MAP_18B_CS_STARTUP_INTRO` (395). No intro production
fix. Next: CPU dump of intro OPA/XLU extents.

```text
800CACEC_AS_OBJECT_POP_GATE: PROVEN
OBJECT_FRUSTUM_ASPECT_AWARE_VISUAL_RESULT: VISUALLY_CONFIRMED
OBJECT_FRUSTUM_PRODUCTION_PROMOTED: YES
PRODUCTION_OBJECT_FIX_MATCHES_VALIDATED_DIAGNOSTIC: YES
INTRO_WALL_RESIDUAL: STILL_PRESENT
INTRO_MISSING_WALL_CLASS: SOURCE_GEOMETRY / CAMERA_FRAMING
INTRO_ROOT_STATUS: STRONG_HYPOTHESIS
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_DISCRIMINATOR: INTRO_MAP_18B_SOURCE_EXTENT_VS_EXPAND_FRUSTUM
NEXT_SINGLE_ACTION: CPU dump MAP_18B_CS_STARTUP_INTRO OPA/XLU vertex XZ vs source 4:3 and target Expand
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-object-production-intro-root-20260830T031102Z/
ARTIFACT_CHECKSUM_MANIFEST: 3d69e9a80426875a31e3c65c1c07e538c61a93d9b49d9d54df2c39a1a2ba0fe8
```

Do not reopen GPU. Do not combine vis flags. Do not port Banjo intro
verts. Do not implement intro production fix until extents are proven.

## Prior (2026-08-30) — OBJECT_MODEL_FRUSTUM_BANJORECOMP_PORT

User reran `TOOIE_DIAG_WS_OBJECT_POP=1` at the screenshot location.
Objects still disappear at the corners after the production `800CAA24`
map fix. Summary `/tmp/tooie-ws-object-pop-summary.txt` SHA256
`6eaffa472c24e6a9556d9a0197205a7aaacac21436f265c743d400b54a089bab`.
frames=8861 pool=24 logs=96 dropped=902. Gameplay instances captured;
intro excluded.

Banjo signature matched: distance fell, source-4:3 NO, target-Expand
YES, `800CACEC` ACCEPT→REJECT. 89/96 logged transitions are
SOURCE_OUTSIDE_TARGET_INSIDE.

`TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1` (default OFF) retests the
sphere against H-expanded planes with the same k as `800CAA24` and
returns the expanded result for DRAW. Attract 62s: 244 new accepts.
Not promoted. Original v0 stored (`s_last_cacec_orig` /
`tooie_model_would_have_been_culled`) for Banjo-style demo preservation.
Prior `TOOIE_DIAG_CACEC_BYPASS` `NO_CHANGE` remains valid for intro/map
walls only.

```text
OBJECT_A_RUNTIME_IDENTITY: chsilo 0x801ad888 MEDIUM
OBJECT_B_RUNTIME_IDENTITY: chnests 0x801adf3c MEDIUM
CONTROL_C_RUNTIME_IDENTITY: chnests 0x801adc30 LOW
OBJECT_A_TRANSITION_CLASS: OBJECT_MODEL_FRUSTUM_SIDE_PLANE
OBJECT_B_TRANSITION_CLASS: OBJECT_MODEL_FRUSTUM_SIDE_PLANE
800CACEC_AS_OBJECT_POP_GATE: PROVEN
CACEC_SEES_EXPANDED_PLANES: NO
OBJECT_FRUSTUM_COUNTERFACTUAL_RESULT: STRONG_HIT
USER_MANUAL_TEST_REQUIRED_NOW: YES
NEXT_DISCRIMINATOR: USER_VISUAL_OBJECT_FRUSTUM_ASPECT
NEXT_SINGLE_ACTION: TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1 ./build/TooieRecompiled
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-object-model-frustum-banjo-port-20260830T024845Z/
ARTIFACT_CHECKSUM_MANIFEST: ab92e43b8cfe48e81ae44b85beed830d8751f621d54aeadc7bd0b8ae55f9e126
```

Do not reopen GPU. Do not solve intro. Do not blanket TRUE. Do not
combine vis flags. Do not promote the object frustum diagnostic.

## Prior (2026-08-30) — OBJECT_POP_WITNESS_ANALYSIS

Completed `TOOIE_DIAG_WS_OBJECT_POP=1` run is `INCONCLUSIVE` for A/B.
Summary SHA256
`2c9a7b53b0761257d77c662526b5dbf45a6c098ed135d508c7bdd761208ee595`.
frames=8315 pool=24 locked=3 logs=0. Locked getters are
`_chintrochar_entrypoint_0` (`0x80086C98`), not screenshot A/B/C.
`800EFFB4` never called; type-6 vis stayed 1. `8010108C` inside
`8010038C` is first-visit init, not per-frame draw.

Banjo `modelRender_draw` → `viewport_func_8024DB50` (sphere vs 4 side
planes) maps structurally to Tooie `800CACEC` (`PROVEN`). Runtime A/B
toggle `NOT_PROVEN` (`STRONG_HYPOTHESIS`). `800CAA24` wrap does not write
expanded planes back, so `800CACEC` still sees original 4:3 planes.

Diagnostic retargeted (default OFF): never lock intro; evict intro from
the 24-slot pool; track `800CACEC`; emit SOURCE/TARGET via the same k as
`800CAA24`; capture `CACEC_ORPHAN` on SOURCE_OUTSIDE_TARGET_INSIDE.
No object production fix. If later proven: H-expand DRAW only; keep orig
v0 for demo/callback (Banjo `cur_model_would_have_been_culled_in_demo`);
no blanket TRUE.

```text
OBJECT_POP_CLASSIFICATION: INCONCLUSIVE
OBJECT_A_CLASS: UNKNOWN
OBJECT_B_CLASS: UNKNOWN
CONTROL_OBJECT_C_CLASS: UNKNOWN
A_B_C_SAME_RENDER_FAMILY: UNKNOWN
TYPE6_GAMEPLAY_OBJECT_POP_STATUS: REOPENED_TARGETED
800CACEC_STRUCTURAL_ANALOG_OF_8024DB50: PROVEN
800CACEC_AS_OBJECT_POP_GATE: STRONG_HYPOTHESIS
OBJECT_POP_FIRST_TOGGLE_GATE: UNKNOWN
SOURCE_OUTSIDE_TARGET_INSIDE_COMPATIBLE: UNKNOWN
USER_MANUAL_TEST_REQUIRED_NOW: YES
NEXT_DISCRIMINATOR: USER_VISUAL_OBJECT_POP_DIAG_CACEC
NEXT_SINGLE_ACTION: TOOIE_DIAG_WS_OBJECT_POP=1 ./build/TooieRecompiled
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-object-pop-witness-analysis-20260830T022303Z/
ARTIFACT_CHECKSUM_MANIFEST: 471db3eaf6f138143cfcf7443adbeb6eeef183aa40d38c9a31ef54610b98dbef
```

Do not reopen `800CAB70` / `800E3DC0` / `800CACEC` / `801143A4` /
`800BE09C` / `800BD708` / `800F0064` / `80108C14` as Expand **map** roots.
`800CACEC` is reopened only as a targeted object-pop analog.
Do not return to GPU. Do not solve intro. Do not implement an
object-cull production fix until A/B transitions are known.

## Prior (2026-08-30) — MAP_VIS_PRODUCTION + TARGETED OBJECT POP

User visual: gameplay walls look good under aspect-aware
(`VISUALLY_CONFIRMED_FOR_TESTED_GAMEPLAY_MAP_GEOMETRY`; not exhaustive).
Production: same H-plane math (`k=target/source`, `n_cam_x /= k`, V
unchanged) is ON unless `TOOIE_TEST_MAP_VIS_ASPECT_AWARE=0`. Blanket
`TOOIE_TEST_DISABLE_MAP_VIS_CULL` stays default OFF. Attract P@5880
`new_accept=3425` vs diagnostic C `3504` (`PRODUCTION_MAP_FIX_MATCHES_VALIDATED_DIAGNOSTIC: YES`).

Three screenshots prove object-specific pop (A gray dome, B orange later,
C blue nest control stays). Same class in demo. Identities `UNKNOWN`
(hypotheses `chsilo`/`chsignpost`/`chbeehive`/`chnests` only). Type-6
gameplay pop `REOPENED_TARGETED`; intro type-6 remains
`STRONGLY_WEAKENED`. Canonical actor wording:
`ACTOR_CUBE_AABB_SELECTION_AS_EXPAND_DRAW_ROOT: REFUTED`. Intro walls
`OPEN_MIXED` and independent. Diagnostic `TOOIE_DIAG_WS_OBJECT_POP=1`
locks ≤3 instances, transition-only, summary
`/tmp/tooie-ws-object-pop-summary.txt`. No object production fix.

```text
MAP_VIS_PRODUCTION_PROMOTED: YES
MAP_VIS_ASPECT_AWARE_VISUAL_RESULT: VISUALLY_CONFIRMED_FOR_TESTED_GAMEPLAY_MAP_GEOMETRY
FOUR_THREE_IDENTITY: PROVEN
ARBITRARY_ASPECT_SUPPORT: YES
RT64_CHANGE_NEEDED: NO
OBJECT_POP_SCREENSHOT_SEQUENCE: PROVEN
OBJECT_A_CLASS: UNKNOWN
OBJECT_B_CLASS: UNKNOWN
CONTROL_OBJECT_C_CLASS: UNKNOWN
A_B_C_SAME_RENDER_FAMILY: UNKNOWN
TYPE6_GAMEPLAY_OBJECT_POP_STATUS: REOPENED_TARGETED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
ACTOR_CUBE_AABB_SELECTION_AS_EXPAND_DRAW_ROOT: REFUTED
MAP_SECTION_800CAA24_ROOT: PROVEN_PARTIAL
INTRO_WALL_RESIDUAL: OPEN_MIXED
INTRO_RESIDUAL_INDEPENDENT_FROM_GAMEPLAY_MAP_FIX: STRONG_HYPOTHESIS
NORMAL_GAMEPLAY_AND_DEMO_OBJECT_POP_SHARED: USER_OBSERVED
OBJECT_POP_DIAGNOSTIC_IMPLEMENTED: YES
USER_MANUAL_TEST_REQUIRED_NOW: YES
FIX_IMPLEMENTED: YES (map vis only)
NEXT_DISCRIMINATOR: USER_VISUAL_OBJECT_POP_DIAG
NEXT_SINGLE_ACTION: TOOIE_DIAG_WS_OBJECT_POP=1 ./build/TooieRecompiled
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-map-production-and-object-witness-20260830T020240Z/
ARTIFACT_CHECKSUM_MANIFEST: 9c7566804674e3057f71414d1cf659396e47e75fb705dfdf4bd046b0ba5e86d8
```

Do not reopen `800CAB70` / `800E3DC0` / `800CACEC` / `801143A4` /
`800BE09C` / `800BD708` / `800F0064` / `80108C14` as Expand draw roots.
Do not return to GPU. Do not solve intro this residual. Do not implement
an object-cull production fix until A/B transitions are known.

## Prior (2026-08-30) — 800CAA24_FINAL_SEMANTICS / MAP_VIS_ASPECT_AWARE

`800CAA24` is world float AABB vs four frustum **side** planes (`n·x+d < 0`,
n-vertex). No near/far. Primary caller `800E3D0C` (two tests on
`D_8012D500`). Horizontal planes come from `800CA218`/`800CAF34` using
`aspect * half_fov`; vertical does not use aspect (VFOV-preserving).
`vol+0xC` is world forward, not Euler. Expand candidate:
`k=target/source`; `k<=1` original; else retest H planes with
`n' = n_f + n_perp/k` (`n_cam_x /= k`). Host target from
`tooie_window::calculate_target_aspect_ratio` (`RT64_CHANGE_NEEDED: NO`).

Diagnostic `TOOIE_TEST_MAP_VIS_ASPECT_AWARE` default OFF, never combined
with blanket. ninja `TooieRecompiled` PASS. Attract aligned frames=5880:
A dl=262956; B blanket 1063786 forced=401577; C aspect 283719
`new_accept=3504` (C/A DL 1.079 vs B/A 4.045). `STRONG_CANDIDATE`.
`FOUR_THREE_IDENTITY: PROVEN` algebraically.

GeoCmd: `800E3DC0`/`800CAB70` **is** the BK GeoCmdD homolog. Prior
“intra-model GeoCmd analog untested” is `CORRECTED/SUPERSEDED`.
`E3DC0_DIRECT_ROOT` remains `REFUTED`. No distinct extra GeoCmd layer.

```text
MAP_VIS_ASPECT_AWARE_CLASSIFICATION: STRONG_CANDIDATE
INDEPENDENT_GEOCMD_LIKE_CULL_EXISTS: YES
DISTINCT_GEOCMD_LAYER_BEYOND_E3DC0: NO
INTRA_MODEL_GEOCMD_ANALOG_UNTESTED_WORDING: CORRECTED/SUPERSEDED
E3DC0_DIRECT_ROOT: REFUTED
TARGET_ASPECT_AVAILABLE_TO_GUEST_PATCH: YES
RT64_CHANGE_NEEDED: NO
FOUR_THREE_IDENTITY: PROVEN
USER_VISUAL_TEST_REQUIRED_NOW: YES
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR: USER_VISUAL_MAP_VIS_ASPECT_AWARE
NEXT_SINGLE_ACTION: TOOIE_TEST_MAP_VIS_ASPECT_AWARE=1 ./build/TooieRecompiled (do not combine with TOOIE_TEST_DISABLE_MAP_VIS_CULL)
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-map-vis-final-semantics-20260830T010730Z/
ARTIFACT_CHECKSUM_MANIFEST: 2c66095cc262b4d6b8fad31a4bc13b0609109db4555e223cb638fce456c3eaca
```

Do not reopen `800CAB70` / `800E3DC0` / `800CACEC` / `801143A4` /
`800BE09C` / `800BD708` / `800F0064` / type-6 Euclidean / `80108C14`.
Do not return to GPU. Leftover items/rocks + intro MIXED must not reject
this candidate.

## Prior (2026-08-30) — REMAINING_STATIC_DECORATIVE_MODEL_DRAW

`80108C14` walks the same `D_80136EE0` 0x9C type-6 objects as
`800FF62C`. Caller `800A5C28` (`gsworldDll_entrypoint_2` @ `80800470`).
Chain: `80100368` jalr `obj+0x10` GETTER → `lhu desc+0x20` asset id →
skip if 0 → `800DA298` overlay-loaded → `800FFAB0` flag/prep →
`800819B4` DL emit. LIVE 60s attract: `08c14=2` vs `ff62c=1337`;
`visits=42 skip_id0=41 da298 fail=1 ffab0=0 dl=0`. GETTER samples:
`chintrochar` `80086C98`, `chintrocam`, `chdeflect`, `chworlddoors`,
`chnests`, `chsignpost`, `chmayaentrydoor` (asset 914, overlay fail).
Sibling `801002C0`/`80100230` Euclidean `800EEB40` (`CAN_REJECT_SOTI: NO`).
Forward/backward intersection with `800819B4` is the closed type-6 family
only. No untested SOTI DRAW gate; CF `NOT_IMPLEMENTED`.

```text
FUNC_80108C14_IS_PER_FRAME: NO
FUNC_80108C14_IS_DRAW: CONDITIONAL
FUNC_80108C14_EMITS_DL: YES structurally; LIVE 0
80108C14_AS_REMAINING_DRAW_ROOT: REFUTED
REMAINING_STATIC_DECORATIVE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
STATIC_DECORATIVE_VIEW_COUNTERFACTUAL: NOT_IMPLEMENTED
ACTOR_PICKUP_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
SPRITE_800F0064_AS_LATERAL_EXPAND_ROOT: REFUTED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
TYPE6_AS_GLOBAL_GAMEPLAY_RESIDUAL_ROOT: UNKNOWN / LOW_PRIORITY
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR: GLOBAL_RESIDUAL_RERANK_800CAA24_FINAL_SEMANTICS
NEXT_SINGLE_ACTION: Re-rank remaining gameplay residual after closed object families: 800CAA24 remaining section semantics versus intra-model GeoCmd analog versus leftover source geometry. Do not return to GPU. Do not combine vis flags.
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-remaining-static-decorative-draw-20260830T002649Z/
ARTIFACT_CHECKSUM_MANIFEST: f7fa715b7bb5888df0222be2ef5a2439243453f67732ad6aafb1414f634c5174
```

HARD STOP on remaining-static decorative view-cull as Expand root.
Do not reopen `800CAB70` / `800E3DC0` / `800CACEC` / `801143A4` /
`800BE09C` / `800BD708` / `800F0064` / type-6 Euclidean / `80108C14`.
Do not return to GPU.

## Prior (2026-08-30) — ACTOR_PICKUP_PREMODEL_VIEW_VISIBILITY

`8011458C` is a per-frame gccube occupancy / near-fog probe via
`801143A4` (caller `80114774` after `80114184`). **NOT** actor draw
dispatch. Actor DRAW chain: `D_80128410` → `80114184` camera-object AABB
→ `800BD708` world AABB→cube idx → `800CD23C` → `800EA628` → `80103508`
→ `8010108C`.

Untested gate this round: `800BD708` cube-index from `80114184` AABB
(WORLD_SPATIAL GRID; FOV/aspect only if caller AABB is 4:3 view volume).
CF `TOOIE_TEST_EXPAND_ACTOR_CUBE_AABB=1` expands XZ cube idx ±2 (clamped).
LIVE 60s attract: `ea628` 22722→257504; `cube_span` max 8→72; `03508=3`
both; actor `draw108c=0 dl=0`. Extra candidates fail Euclidean
`800EB350` / pass-mode `800CB854`. `prim_emit` did not rise.

```text
ACTOR_PIPELINE_EXERCISED: YES
ACTOR_VIEW_COUNTERFACTUAL: TOOIE_TEST_EXPAND_ACTOR_CUBE_AABB
ACTOR_VIEW_COUNTERFACTUAL_RESULT: PARTIAL_HIT visitation / NO_CHANGE draw-DL
ACTOR_PICKUP_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
SPRITE_800F0064_AS_LATERAL_EXPAND_ROOT: REFUTED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
TYPE6_AS_GLOBAL_GAMEPLAY_RESIDUAL_ROOT: UNKNOWN / LOW_PRIORITY
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR: REMAINING_STATIC_DECORATIVE_MODEL_DRAW
NEXT_SINGLE_ACTION: Start at 80108C14 model-list / overlay jalr DRAW vs UPDATE for leftover items/rocks that are not 800CAA24, not 800EA628->8010108C (attract 0108c=0), not bound-sprite, not type-6 Euclidean. Do not return to GPU. Do not combine vis flags.
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-actor-pickup-premodel-view-20260830T000358Z/
ARTIFACT_CHECKSUM_MANIFEST: 4706ad90341a23e6bbf299a53acac7495b32f472387dbcc3d7dc77665fec0d5d
```

HARD STOP on actor/pickup pre-draw view family. Do not re-bypass
`800CAB70` / `800E3DC0` / `800CACEC` / `801143A4` / `800BE09C`.

## Prior (2026-08-29) — BOUND_SPRITE_INSTANCE_PER_FRAME_DRAW

`800B3CDC` inserts 0x18 occupancy records into `D_8012AE80+0xE0`.
`808001A8_gcboundDll` copies world xyz (`800EE88C`), half-radius, packed
id. Per-frame consumers are list lookup `800CB8F0` and queries
`800CBC3C`/`800CBDD4`/`800CC078`/`800CC338`. Intersection of bound helpers
with `800819B4`/`8010108C` is empty. LIVE 60s attract:
cb8f0=5777/5777, queries 6+334+1730+813, inst_samples=21,
`draw_fn=NONE dl_emit=NONE`. Query gate `800EFC7C` is world XZ; CAMERA/FOV/
ASPECT none. `CAN_REJECT_SOURCE_OUTSIDE_TARGET_INSIDE: NO`.
`BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED`. View CF
`NOT_IMPLEMENTED`. Packed ids 0,59,72,75,85,109,128,143 unnamed.

```text
BOUND_SPRITE_PATH_EXERCISED: YES
BOUND_SPRITE_VIEW_COUNTERFACTUAL: NOT_IMPLEMENTED
BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
SPRITE_800F0064_AS_LATERAL_EXPAND_ROOT: REFUTED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
TYPE6_AS_GLOBAL_GAMEPLAY_RESIDUAL_ROOT: UNKNOWN / LOW_PRIORITY
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR: ACTOR_PICKUP_PREMODEL_VIEW_VISIBILITY
NEXT_SINGLE_ACTION: Trace actor/pickup pre-draw vis at 800CAB70/800E3DC0/800CACEC and 8011458C; prove SOURCE_OUTSIDE_TARGET_INSIDE before any actor CF. Do not reopen 800F0064 or bound-sprite draw. Do not combine vis flags.
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-bound-sprite-per-frame-draw-20260829T231221Z/
ARTIFACT_CHECKSUM_MANIFEST: fde93ff1c4e3059d186c567f810ad753b7fbe9cdc075ede0b193726d62a98d51
```

HARD STOP on bound-sprite family. Actor next-start: `800CAB70`/`800E3DC0`
LIVE 77107 culled 74658 vis 2449; `8011458C`=1480; `800BE09C` RANGE is
actor spatial (not static sprite root).

## Prior (2026-08-29) — GSPROPS_SPRITE_7_9_A_PER_FRAME_VISIBILITY

gsprops types **7 / 9 / C** (type **0xA skipped**; prior A→`800CBB18`
mapping is `CORRECTED/SUPERSEDED`) dispatch at **map load** via
`gccubeDll_entrypoint_6` (`LIVE gccube6=2`). First pre-insert gate is
`80801484` Y-slab then `800F0064` world XZ circle (prop vs bound record).
`800EE88C` is s16→float of the prop, **not** camera space.
`FUNC_800F0064_CAN_CAUSE_LATERAL_EXPAND_POP: NO`.

Automated window: 30s auto-start still intro (`t7=0`); **60s
`TOOIE_DIAG_ATTRACT=1`** exercises sprites: t7=47 t9=36 tC=1, xzrad
703/62/641, inserts 90. Independent
`TOOIE_TEST_DISABLE_GSPROPS_VIS_CULL=1`: forced=70, inserts 114
(`PARTIAL_HIT`); not an Expand/FOV hit. Per-frame DL emitter of
`800B3CDC` products is **UNKNOWN**.

```text
SPRITE_PATH_AUTOMATICALLY_EXERCISED: YES
SPRITE_VIS_COUNTERFACTUAL_CPU_RESULT: PARTIAL_HIT
SPRITE_AS_LATERAL_EXPAND_ROOT: REFUTED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
TYPE6_AS_GLOBAL_GAMEPLAY_RESIDUAL_ROOT: UNKNOWN / LOW_PRIORITY
USER_VISUAL_TEST_REQUIRED_NOW: NO
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR: BOUND_SPRITE_INSTANCE_PER_FRAME_DRAW
NEXT_SINGLE_ACTION: Identify who walks D_8012AE80+0xE0 / 800B3CDC products every frame and which camera/FOV/aspect gate (if any) drops them before DL. Do not reopen 800F0064 as vis. Do not combine vis flags.
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-gsprops-sprite-visibility-20260829T224516Z/
ARTIFACT_CHECKSUM_MANIFEST: 40941e011e86c403ec0b9cb0f781a6db3b15ff04cd4d7ea6aae18ce5be604894
```

## Prior (2026-08-29) — LOADED_STATIC_MODEL_PER_FRAME_DRAW

`80108DC0` is a **model/resource constructor**. Successful v0 is the 0x9C
object appended by `80106668` into `D_80136EE0`. `gspropsDll_entrypoint_1`
does **not** store that pointer. Per-frame consumer is **PROVEN**:
`800A5CFC` → `800FF62C` walks `D_80136EE0`. Skip if `+0x64` bit17 SET
(GAMEPLAY; do not bypass). Else `8010038C`: first visit always
`8010108C` (draw); later Euclidean `800EFFB4` (`dist^2` vs `radius^2`,
**not** frustum/FOV/aspect). DL dispatch `800819B4`. `gspropmarker_3` is
scale decode only. `+0x38` is ctor INIT FUNC then SCALE FLOAT.

25s auto-start A/B (`TOOIE_TEST_DISABLE_TYPE6_STATIC_VIS_CULL` independent):
loaded=10, consumer_seen=320, draw_attempts=84, vis rej/acc/forced=0,
dl_emits=53, `dl_rsp` 34472 identical. `800EFFB4` never entered for tracked
type-6. `TYPE6_VIS_COUNTERFACTUAL_RESULT: NO_CHANGE`. Type-6 is
`STRONGLY_WEAKENED` as the widescreen-edge residual root in this intro
window (only 10 models; vis never rejects; Euclidean not aspect).

Sprite path (`types 7/9/A` → gcbound → `800F0064`) still `t7=0` in intro;
it is now the **next** discriminator. Do **not** wrap `80108DC0` as vis.
Do **not** combine MAP_VIS / MODEL_GEO / PROP_CELL / GSPROPS_VIS / TYPE6_VIS.
Do **not** reopen GPU/TMEM/`00349d38`. Intro leftover remains MIXED.

USER MAP_VIS (`TOOIE_TEST_DISABLE_MAP_VIS_CULL=1`): gameplay map walls restored
a lot; items/rocks/assets still vanish at corners; intro/demo still lose walls.
Do **not** call `800CAA24` a full global root.

USER MODEL_GEO (`TOOIE_TEST_DISABLE_MODEL_GEO_CULL=1`): looked very bad, like
the original broken baseline. Classify `801143A4` as
`REFUTED_FOR_OBSERVED_RESIDUAL`. Intro 12s/25s smokes: 0 calls, so
`MODEL_GEO_FLAG_CONFIRMED_ACTIVE_DURING_GAMEPLAY: UNKNOWN`.

```text
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
GSPROPS_ENTRYPOINT_1_ROLE: PROVEN_ONE_SHOT_TYPE6_LOAD
GSPROPS_ENTRYPOINT_1_SELECTOR_RESULT: REFUTED_AS_PER_FRAME_VISIBILITY_SELECTOR
PROP_CELL_800BE09C_RESULT: REFUTED_FOR_STATIC_TYPE6_ROOT
TYPE6_FIRST_PER_FRAME_CONSUMER: 800FF62C
TYPE6_PER_FRAME_ITERATION_SOURCE: D_80136EE0
TYPE6_DRAW_FUNCTION: 8010108C / vtable+0x10
TYPE6_DL_EMIT_FUNCTION: 800819B4
TYPE6_FIRST_VISIBILITY_GATE: 8010038C later + 800EFFB4 Euclidean sphere
TYPE6_VISIBILITY_GATE_STATUS: PROVEN exists; LIVE never rejected tracked intro type-6
TYPE6_VIS_COUNTERFACTUAL_IMPLEMENTED: YES (TOOIE_TEST_DISABLE_TYPE6_STATIC_VIS_CULL)
TYPE6_VIS_COUNTERFACTUAL_RESULT: NO_CHANGE
TYPE6_AS_OBSERVED_RESIDUAL_ROOT: STRONGLY_WEAKENED
RESIDUAL_ROCKS_LIKELY_TYPE6: UNKNOWN
RESIDUAL_ITEMS_LIKELY_TYPE6: UNKNOWN
RESIDUAL_GENERAL_ASSETS_LIKELY_TYPE6: UNKNOWN
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
CURRENT_ROOT_SET: 800CAA24 PARTIAL + type-6 draw path PROVEN but vis not aspect + sprite/gcbound OPEN + intro leftover MIXED
CURRENT_ROOT_SET_STATUS: PARTIAL_PROVEN + OPEN_RESIDUALS
PERFORMANCE_GATING_CLEANUP_PRESERVED: YES
GPU_READBACK_USED: NO
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR: GSPROPS_SPRITE_7_9_A_PER_FRAME_VISIBILITY
NEXT_SINGLE_ACTION: Exercise types 7/9/A -> gcbound -> 800F0064 on an automated attract/demo window that actually dispatches sprites (intro t7=0). Do not combine vis flags.
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-loaded-static-model-per-frame-draw-20260829T215504Z/
ARTIFACT_CHECKSUM_MANIFEST: de43c12dfcaccd59e3b4cd4ca2b6aa88a74dd232e89b757c4e6e56393b4c7940
```

Do **not** reopen `00349d38` / depth / TMEM / GPU. Do **not** wrap `80108DC0`
as vis. Do **not** implement cutscene aspect fallback this residual.

## Prior (2026-08-29) — GSPROPS_STATIC_PROP_SELECTOR

`gspropsDll_entrypoint_1` (overlay `0x808000A0`, syscall `0x80088B88`, DLL 727)
is **PROVEN** a one-shot post-select load: `gccubeDll_entrypoint_0` walks
**all** cubes in `D_80128410`, `gsprops_0` keeps type==6, then `gsprops_1`
sorts and `80108DC0`-loads. Intro 12s/25s: cubes=1274, appended=11, load
ok=10 fail=1, `ep1` calls=1. It does **not** emit DLs and is **not** a
per-frame visibility selector. Residual corner pop of rocks/items is **not**
explained by this collect.

Sprite/bound path (`gccube_6` → `gsprops_2` types 7/9/A → gcbound → overlay
`80801484` Y-slab + `800F0064` XZ dist^2 vs radius^2) is a genuine vis
candidate structurally but **NOT_EXERCISED** in intro (`t7=0`, `xzrad=0`).
Independent CF `TOOIE_TEST_DISABLE_GSPROPS_VIS_CULL=1` (forces `800F0064`
v0=1 only inside `gsprops_vis_pass`; default OFF) produced **NO_CHANGE**
(collect/load identical; RSP 9375 vs 9445 is run variance).

`RELATION_TO_800BE09C: INDEPENDENT / PARALLEL`. Extra RANGE cubes
`153→325` are **NEVER_CONSUMED_BY_GSPROPS** (gsprops already walks all 1274).
`RELATION_TO_800CAA24: INDEPENDENT`. Do **not** wrap `80108DC0` as vis.
`80108C14` is also one-shot (`calls=1`). Do not combine MAP_VIS / MODEL_GEO /
PROP_CELL / GSPROPS_VIS. Intro leftover remains MIXED; do not deepen.

USER MAP_VIS (`TOOIE_TEST_DISABLE_MAP_VIS_CULL=1`): gameplay map walls restored
a lot; items/rocks/assets still vanish at corners; intro/demo still lose walls.
Do **not** call `800CAA24` a full global root.

USER MODEL_GEO (`TOOIE_TEST_DISABLE_MODEL_GEO_CULL=1`): looked very bad, like
the original broken baseline. Classify `801143A4` as
`REFUTED_FOR_OBSERVED_RESIDUAL`. Intro 12s/25s smokes: 0 calls, so
`MODEL_GEO_FLAG_CONFIRMED_ACTIVE_DURING_GAMEPLAY: UNKNOWN`.

BK Cube→Prop: `func_8032D510` iterates one Cube's props to
`__marker_draw` / `propModelList_drawModel` / `propModelList_drawSprite`.
Historical `viewport_cube_isInFrustum` / `viewport_cube_isInFrustum2` were
patched TRUE in `8b79f76` then removed by `1e19dad`. Current BK cube patches:
NO. Why they were removed: UNKNOWN. BK issue #125 remains OPEN widescreen
("Culling is not disabled for this switch in RBB") so
`BANJORECOMP_CURRENT_WIDESCREEN_CULLING_COMPLETE: NO`. Do not collapse
`viewport_cube_*` into `800CAA24` (INDEPENDENT).

Tooie spatial container: gccube `D_80128410`. RANGE `800BE09C` (3x3x3 via
`800BE444` dest `0x80128538`). Occupancy `800E9DCC` is `lh +6`, not frustum.
Draw parents: `8011458C` (actor/`__marker_draw` analog), `8010A01C`,
`80109CC4`, `801099D0`. Empty cube list never reaches `801143A4`.
`TOOIE_PROP_DRAW_REACHED_WHEN_CELL_REJECTED: NO`.

PROP_CELL CF `TOOIE_TEST_DISABLE_PROP_CELL_VIS_CULL` (default OFF; draw-pass
RANGE only; occupancy kept; actor UPDATE callers of `800BE444` not expanded;
not combined with MAP_VIS/MODEL_GEO). Independent 12s Expand A/B: listed
153→325; `dl_rsp` identical 9445; `801143A4` still 0. REFUTED as intro wall
residual. CORRECTED/SUPERSEDED for gsprops: `gspropsDll_entrypoint_1` is a
one-shot ALL-cube type-6 load independent of RANGE; extras never reach it.

Intro leftover after MAP_VIS: MIXED B/C (authored gap analog not found in
Tooie; BK #168 camera framing) plus other map path; not cube RANGE.
Demo residual: MIXED / UNKNOWN. BK demo policy: keep original cube/bbox
culling so actor/RNG stay synced. Tooie PROP_CELL is RENDER-only.

Cutscene aspect fallback: PARTIAL feasibility only (not implemented). Tooie
gameplay must keep arbitrary aspect; cutscene Original/Clamp/pillarbox only
if class B/C proven. Do not hardcode 16:9.

```text
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
MODEL_GEO_FLAG_CONFIRMED_ACTIVE_DURING_GAMEPLAY: UNKNOWN
GSPROPS_ENTRYPOINT_1_ADDRESS: overlay 0x808000A0; syscall 0x80088B88; DLL 727
GSPROPS_ENTRYPOINT_1_ROLE: PROVEN one-shot type-6 load; NOT per-frame vis
STATIC_PROP_SELECTOR_FUNCTION: gccubeDll_entrypoint_0 + gsprops_0 type==6
STATIC_PROP_SELECTOR_STATUS: PROVEN one-shot ALL-cube type-6 load; REFUTED as residual per-frame vis
RELATION_TO_800BE09C: INDEPENDENT / PARALLEL
EXTRA_800BE09C_CELLS_DROPPED_AT: NEVER_CONSUMED_BY_GSPROPS
RELATION_TO_800CAA24: INDEPENDENT
GSPROPS_VIS_COUNTERFACTUAL_IMPLEMENTED: YES (TOOIE_TEST_DISABLE_GSPROPS_VIS_CULL)
GSPROPS_SELECTOR_COUNTERFACTUAL_CPU_RESULT: NO_CHANGE
PROP_CELL_800BE09C_RESULT: STRONG_HYPOTHESIS actor; REFUTED intro RSP; INDEPENDENT of gsprops
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
CURRENT_ROOT_SET: 800CAA24 PARTIAL + loaded-static per-frame draw UNKNOWN + sprite/gcbound later + intro leftover MIXED
CURRENT_ROOT_SET_STATUS: PARTIAL_PROVEN + OPEN_RESIDUALS
PERFORMANCE_GATING_CLEANUP_PRESERVED: YES
GPU_READBACK_USED: NO
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR: LOADED_STATIC_MODEL_PER_FRAME_DRAW
NEXT_SINGLE_ACTION: Identify who draws the 10 models loaded by gsprops_1/80108DC0; do not assume 80108C14 or 801143A4; do not combine MAP_VIS
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-gsprops-static-prop-selector-20260829T210810Z/
ARTIFACT_CHECKSUM_MANIFEST: 09bb0ef3fc74105e1306577af670ef13d088044768047bad03438b53378e27ce
```

Do **not** reopen `00349d38` / depth / TMEM / GPU. Do **not** wrap `80108DC0`
as vis. Do **not** implement cutscene aspect fallback this residual.

## Prior (2026-08-29) — RESIDUAL_PROP_CELL_AND_CUTSCENE_VISIBILITY_AUDIT

PROP_CELL CF `TOOIE_TEST_DISABLE_PROP_CELL_VIS_CULL` (draw-pass RANGE only):
listed 153→325; intro `dl_rsp` identical 9445; `801143A4` still 0.
`TOOIE_PROP_CELL_VISIBILITY_STATUS:` STRONG_HYPOTHESIS actor; REFUTED intro
RSP. Artifact
`/tmp/p08-widescreen-residual-prop-cell-cutscene-audit-20260829T204203Z/`
(`SHA256SUMS` `52a0f350cc540708f9a6de44c7e60bc60066c51e6807e71c71395589bee59fbd`).
Next at that checkpoint was `GSPROPS_STATIC_PROP_SELECTOR`.

## Prior (2026-08-29) — BANJORECOMP_WIDESCREEN_VISIBILITY_PATCH_PORT_AUDIT

User morphology correction: `00349d38` is **not** the bug. It is one
convenient witness triangle. The symptom is pervasive (intro/cutscenes,
gameplay, many walls **and** objects, late-pop-in and early-pop-out).

```text
HEAD_TOOIE: ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_BANJORECOMP: ec859632cfa584e2272d32e46051543429a2baf4
HEAD_RT64: 6f1c2d99a4ea571c139f449c326fd176ba8f3496
SAME_RT64_REVISION: YES
PERFORMANCE_GATING_CLEANUP_PRESERVED: YES
DEFAULT_DIAGNOSTIC_OVERHEAD: NEGLIGIBLE when flags OFF
GLOBAL_USER_SYMPTOM_MODEL: PERVASIVE_INTRO_CUTSCENE_GAMEPLAY_LATERAL_GEOMETRY
FOCAL_00349D38_AS_GLOBAL_ROOT: REFUTED
FOCAL_00349D38_AS_WITNESS: YES
FOCAL_TEXTURE_TMEM_ANOMALY: STRONGLY_WEAKENED / STATIC_AUDIT_REFUTED
FOCAL_TEXTURE_RUNTIME_REVALIDATED_WITH_NEW_AUTO_CAPTURE: NO
CAB70_PREVIOUS_RESULT: NO_CHANGE
E3DC0_PREVIOUS_RESULT: NO_CHANGE
CACEC_PREVIOUS_RESULT: NO_CHANGE
BK_PATCH_CLASSES: cube/map vis; bbox frustum; sphere frustum; top-level model cull; GeoCmdD child cull; GeoCmd8 LOD; intro source-model hotpatch; demo-preservation
BK_TOP_MODEL_CULL_EQUIVALENT_TOOIE: 801143A4
BK_GEO_CHILD_CULL_EQUIVALENT_TOOIE: 800E3DC0 (ALREADY_TESTED_AND_REFUTED)
BK_LOD_EQUIVALENT_TOOIE: NO_TOOIE_EQUIVALENT_FOUND
BK_MAP_CULL_EQUIVALENTS_TOOIE: 800CAA24 float AABB / 800E3D0C (independent of CAB70)
NEW_UNTESTED_TOOIE_VISIBILITY_PATH_COUNT: 2
NEW_UNTESTED_TOOIE_VISIBILITY_PATHS: 800CAA24 map vis (CPU+RSP tested this round); 801143A4 model geo (intro-inactive)
INTRO_SOURCE_GEOMETRY_GAP: UNKNOWN (per-surface); exclusive authored-gap REFUTED; suppressed existing geometry PROVEN
MODEL_GEO_CULL_COUNTERFACTUAL_IMPLEMENTED: YES
MODEL_GEO_CULL_COUNTERFACTUAL_RESULT: intro 12s Expand 0 calls / NO_CHANGE (dl_rsp=9445, prim_emit≈83270)
FORCE_NEAR_LOD_COUNTERFACTUAL_IMPLEMENTED: NO
FORCE_NEAR_LOD_COUNTERFACTUAL_RESULT: NOT_IDENTIFIED
MAP_VIS_COUNTERFACTUAL_IMPLEMENTED: YES
MAP_VIS_COUNTERFACTUAL_RESULT: frames=840; CAA24 53763->67195 forced=66436; dl_rsp 9445->142204 (x15.07); prim_emit 83273->955957 (x11.48); cab70/cacec/e3dc0 UNCHANGED; visual UNKNOWN
USER_MANUAL_ACTION_THIS_ROUND: NO
GPU_READBACK_USED: NO
FIX_IMPLEMENTED: NO
CURRENT_ROOT_CLASS: MAP_SECTION_FLOAT_AABB_VISIBILITY (800CAA24/800E3D0C)
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS_CPU_AND_RSP_DISPATCH; VISUAL_UNCONFIRMED
CURRENT_ROOT_BOUNDARY: guest map-section float AABB before DL emit
BANJORECOMP_GUEST_PATCH_DELTA_RESULT: HIT_INDEPENDENT_MAP_VIS
DECISION_TREE: C UNTESTED_MAP_VISIBILITY_ROOT_CANDIDATE
NEXT_DISCRIMINATOR: USER_SIMPLE_VISUAL_CONFIRMATION_OF_MAP_VIS_COUNTERFACTUAL
NEXT_SINGLE_ACTION: TOOIE_TEST_DISABLE_MAP_VIS_CULL=1 ./build/TooieRecompiled (later visual; no wall nav)
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-banjorecomp-visibility-port-audit-20260829T192021Z/
ARTIFACT_CHECKSUM_MANIFEST: 8efe75283a7e8fa1d024b36e92dc61a3c57408acbff07ecf6cecbdd9aacd2a0b
```

CPU-only precursor of this same audit:
`/tmp/p08-widescreen-banjorecomp-visibility-port-audit-20260829T185820Z/`
(`SHA256SUMS` `0622f7e923e1f7588ad01fdde375655c3698715e03430dff08ad319f315a3925`)
measured CAA24 +25% without RSP DL/prim. Independent revalidation with cheap
`dl_rsp`/`prim_emit` counters produced the 192021Z artifact: forcing
`800CAA24` restored previously skipped map-section DLs (x15.07) and primitives
(x11.48) while CAB70/E3DC0/CACEC counts stayed identical. Extra-band NDC
classification was not re-enabled (performance-gated). Visual pixels remain
unconfirmed. Gameplay popping is the same runtime map-vis class unless later
evidence splits it. Do not ship a blanket `TRUE` until visual confirmation.

Gemini TMEM round
`/tmp/p08-widescreen-focal-texture-tmem-setup-20260829T183238Z/`
`SHA256SUMS` `d91118158d0753c17e1ad0370e6150834255ffebffc67b02554692f39ad8b782`
preserved PERFORMANCE_GATING_CLEANUP. Its 4s smoke reported
`FOCAL_TEXTURE_LOG_CREATED_IN_SMOKE: NO`. Do not treat generated TMEM
artifact text as a new runtime observation. Texture/TMEM not reopened.
Depth remains UNKNOWN but is no longer "the only remaining unknown".
Do **not** enter `TARGETED_FOCAL_DEPTH_CAUSAL_TEST_DESIGN` unless remaining
BK guest classes are exhausted (decision E).

Near clipping: REFERENCE_ONLY this round.

## TARGETED_FOCAL_TEXTURE_TMEM_SETUP (2026-08-29)

```text
PERFORMANCE_GATING_CLEANUP_IMPLEMENTED: YES
DIAGNOSTICS_OFF_OVERHEAD: NEGLIGIBLE
OLD_P08_LOGS_DEFAULT_OFF: YES
FOCAL_TEXTURE_FLAG: TOOIE_DIAG_WS_FOCAL_TEXTURE
FOCAL_AUTO_TRIGGER_IMPLEMENTED: YES
AUTO_CAPTURE_DURATION: 300 focal draws (~150-180 frames)
AUTO_CAPTURE_STOP_CONDITION: s_auto_draw_count >= 300
MANUAL_TOUCH_REQUIRED: NO
FOCAL_SOURCE_DL: 00349d38 extra-left TRI1
FOCAL_TEXTURE_HASH: 4a7b1a2672b73a09
FOCAL_FORMAT: RGBA 16b (5-5-5-1)
FOCAL_SAMPLE_WIDTH: 32 px
FOCAL_SAMPLE_HEIGHT: 32 px
FOCAL_CMS: G_TX_CLAMP (2)
FOCAL_CMT: G_TX_CLAMP (2)
FOCAL_TMEM_RANGE: 0x0000 - 0x0080 (1024 bytes)
FOCAL_TMEM_LAST_LOAD_COMMAND: G_LOADBLOCK
FOCAL_TMEM_STALE: NO
FOCAL_TILE_VALID: YES
FOCAL_UV_VALID: YES
FOCAL_TEXTURE_TMEM_ANOMALY: REFUTED
TEXTURE_ID_DIFFERENCE_EXPECTED: YES (distinct map face materials)
NEXT_DISCRIMINATOR: TARGETED_FOCAL_DEPTH_CAUSAL_TEST_DESIGN
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-focal-texture-tmem-setup-20260829T183238Z/
ARTIFACT_CHECKSUM_MANIFEST: d91118158d0753c17e1ad0370e6150834255ffebffc67b02554692f39ad8b782
```

## TARGETED_RASTER_STATE_USER_ARBITRATION (2026-08-29)

```text
CAPTURE_VALID: YES
CAPTURE_ID: 1
CAPTURE_START_FRAME: 2926
CAPTURE_END_FRAME: 3645
CAMERA_EFFECTIVELY_STATIC: YES (716/720 at 3584.1,50.7,4893.8; 4 fov=0 dummy frames)
FOCAL_EVENTS: 288 TRI1 (all V0=(2497,400,4263) V1=(2497,-153,4263) V2=(2245,-44,3680))
FOCAL_PRIMITIVE_SEQ_SET: 145 unique in [1024639, 1608127]
FOCAL_DRAW_SET: callIndex=134
FOCAL_GPU_SUBMITTED: YES 286/286 (native 304x228 + wide 1622x912)
FOCAL_PIPELINE: 000075df080aa190 SPECIALIZED RasterVS+RasterPSMS hash=ced83e2da11a50b7
FOCAL_DEPTH_TEST_ENABLED: YES
FOCAL_DEPTH_WRITE_ENABLED: YES
FOCAL_DEPTH_COMPARE: LESS
FOCAL_DEPTH_RANGE_VALID: YES (NDC Z 0.7517..0.8583; posscreen Z 0.874..0.927)
FOCAL_DEPTH_TEST_RESULT_DIRECTLY_OBSERVABLE: NO
FOCAL_DEPTH_TARGET_ID: 00137840
PRIOR_PROJ1_DEPTH_RESULT_APPLIES_TO_FOCAL: NO
DEPTH_CLEAR_BEFORE_FOCAL: UNKNOWN
DEPTH_WRITES_BETWEEN_CLEAR_AND_FOCAL: UNKNOWN (365 prior depth_write tris same vp in frame 2935)
PRIOR_DEPTH_OCCLUDER_CANDIDATES: 1_WEAK_CORNER_SLIVER (00348470 TRI2_1)
DEPTH_OCCLUSION_RESULT: UNKNOWN
ACTIVE_FRAGMENT_DISCARD_SITES_FOR_FOCAL: 0
ALPHA_COMPARE_ROOT: REFUTED_FOR_FOCAL
COVERAGE_ZERO_ROOT: REFUTED_FOR_FOCAL
COLOR_WRITE_DISABLED_ROOT: REFUTED_FOR_FOCAL
BLEND_ZERO_OUTPUT_ROOT: REFUTED_FOR_FOCAL
FOCAL_INTERSECTS_RASTER_SCISSOR: YES (PARTIAL because V0 is above top)
POST_PROJECTION_NATIVE_COORDINATE_MISMATCH: REFUTED
SOURCE_CLIP_FLAGS_USED_AFTER_TARGET_EXPANSION: NO
FIRST_FOCAL_VS_CENTRAL_RUNTIME_STATE_DIFFERENCE: tex_id (4a7b1a2672b73a09 vs eca4591a3dbd2fcc)
TEXTURE_STATE_DIFFERENCE: YES
COMBINER_STATE_DIFFERENCE: NO
CENTRAL_CONTROL_IS_DEPTH_EQUIVALENT: PARTIAL (same depth state; different screen; drawn AFTER focal)
CULL_WINDING_ANOMALY: REFUTED_FOR_FOCAL
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
USER_SCREENSHOT_CROP_MAPPING: APPROXIMATE
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
CURRENT_ROOT_CLASS: WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY: after GPU submit of 00349d38 TRI1 call=134 proj=2
DECISION: D UNKNOWN (depth-test result not observable; no strictly closer visible prior writer)
NEXT_DISCRIMINATOR: TARGETED_FOCAL_TEXTURE_TMEM_TRACE
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-focal-raster-user-arbitration-20260829T181535Z/
ARTIFACT_CHECKSUM_MANIFEST: a8ce5b47e43ca92986ce19628cc1c1084e6ff2fa22079dc4f17197bcf8636190
FIX_IMPLEMENTED: NO
```

Runtime host viewport this capture is **1622x912**, not 1920x1012. Aspect
scale is generic: `Sx = area_tgt/area_src = 0.750 = (4/3) / (1622/912)`.
Setup static claims of identical texture, wrap, bilinear, and `depth_clip=1`
are **corrected** by this USER capture (`tex_id` differs; `cms0=2` clamp;
`lin_filt=0`; `depth_clip=0`). GPU submit is not pixel proof.

`NEXT_SINGLE_ACTION:` `TARGETED_FOCAL_TEXTURE_TMEM_TRACE` on `00349d38` TRI1 vs same-DL FRONT TRI2. CPU-only. No GPU readback. No depth bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## TARGETED_RASTER_STATE_SETUP (2026-08-29)

```text
INSTRUMENTATION_STATUS: IMPLEMENTED_CPU_SIDE_SAFE
GPU_READBACKS_USED: NO (0 buffer readbacks, 0 image transfers, 0 thread race conditions)
PRIMARY_FOCAL_CANDIDATE: source_dl = 0x00349d38 extra-left TRI1
FOCAL_WORLD_VERTICES: V0=(2497,400,4263) V1=(2497,-153,4263) V2=(2245,-44,3680)
INTERNAL_CONTROL_1: source_dl = 0x00349d38 central FRONT TRI2
INTERNAL_CONTROL_2: source_dl = 0x00349d38 central BACK TRI2
SECONDARY_CONTROL: source_dl = 0x0034b800
LOG_SINKS: /tmp/tooie-ws-focal-raster-state.log + unified [ws-focal-raster-state]
PIPELINE_TYPE: SPECIALIZED
VERTEX_SHADER: RasterVS (SmoothShade=1)
PIXEL_SHADER: RasterPS
GEOMETRY_SHADER: NONE
ALPHA_COMPARE: DISABLED (G_AC_NONE, threshold=0.0, dither=DISABLE, discard=NO)
COVERAGE: CLAMP (cvgDst=CLAMP, cvgXAlpha=0, alphaCvgSel=0, can_zero_output=NO)
BLEND_STATE: blendEnabled=0, srcBlend=ONE, dstBlend=ZERO, blendOp=ADD
COLOR_WRITE_MASK: 0x0F (ALL RGBA ENABLED)
DEPTH_STATE: depthTest=1 (LESS), depthWrite=1, depthClip=1, zMode=0 (OPA)
GUEST_SCISSOR: (0, 0, 304, 228)
HOST_SCISSOR: (0, 0, 1920, 1012)
HOST_VIEWPORT: (0.0, 0.0, 1920.0, 1012.0)
FOCAL_HOST_SCREEN_BOUNDS: X=[100.0, 503.0] px, Y=[-203.4, 529.2] px
FOCAL_INTERSECTS_RASTER_SCISSOR: YES (X strictly inside [0, 1920])
POSSCREEN_SEMANTICS: CPU RSP HLE only (NOT consumed by RasterVS or RasterPS)
SOURCE_CLIP_FLAGS_USED_AFTER_TARGET_EXPANSION: NO
POST_PROJECTION_NATIVE_COORDINATE_MISMATCH: REFUTED
FRAGMENT_DISCARD_SITES_ACTIVE_FOR_FOCAL: 0 / 7
FIRST_FOCAL_VS_CENTRAL_STATE_DIFFERENCE: NONE_OBSERVED
BUILD_STATUS: SUCCESS (ninja exit code 0)
CURRENT_ROOT_CLASS: WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
NEXT_DISCRIMINATOR: USER_SELECTED_RASTER_STATE_CAPTURE
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-targeted-raster-state-setup-20260829T173856Z/
ARTIFACT_CHECKSUM_MANIFEST: fbd3366d1a666196a77c536779cd3b087a0957382ac60bee2cbffe334c81fc14
FIX_IMPLEMENTED: NO
```

### Comprehensive Technical Raster Audit for Focal Wall Primitive (00349d38 TRI1)

1. **Pipeline & Shader Configuration**:
   - `FOCAL_PIPELINE_ID`: Specialized GraphicsPipeline compiled via `RasterShader` from SPIR-V with specialized constants for the draw call's `OtherMode`, `ColorCombiner`, and `RenderFlags`.
   - `FOCAL_SHADER_VARIANT`: `RasterVS` + `RasterPS` (SmoothShade=1, NoN=0, UsesHDR=0).
   - `FOCAL_UBER_OR_SPECIALIZED`: `SPECIALIZED`.
   - Vertex and pixel shaders match bit-for-bit with the central same-DL FRONT TRI2.

2. **Alpha Compare & Shader Discard**:
   - `FOCAL_ALPHA_COMPARE_ENABLED`: `NO (0)`.
   - `FOCAL_ALPHA_COMPARE_MODE`: `NONE (G_AC_NONE)`.
   - `FOCAL_ALPHA_THRESHOLD`: `0.0000`.
   - `FOCAL_ALPHA_DITHER`: `DISABLE (G_AD_DISABLE)`.
   - `FOCAL_SHADER_ALPHA_DISCARD_POSSIBLE`: `NO`.
   - In `RasterPS.hlsl`, alpha compare discard branches are guarded by `G_AC_DITHER` and `G_AC_THRESHOLD`. For `G_AC_NONE`, neither branch executes.

3. **Coverage Output**:
   - `FOCAL_COVERAGE_MODE`: `CVG_DST_CLAMP`, `cvgXAlpha = false`, `alphaCvgSel = false`, `aaEn = true`.
   - `FOCAL_SAMPLE_MASK`: `0x1` (or `0xF` when MSAA 4X enabled).
   - `FOCAL_COVERAGE_CAN_ZERO_OUTPUT`: `NO`.
   - `resultCvg = (8.0 / 255.0) * 1.0 = 0.03137 > CoverageThreshold (1.0 / 255.0 = 0.00392)`. Discard is impossible.

4. **Color Write Mask & Blend State**:
   - `FOCAL_COLOR_WRITE_MASK`: `0x0F` (`RenderColorWriteEnable::ALL` — RGBA fully enabled).
   - `FOCAL_COLOR_WRITE_ENABLED`: `YES (1)`.
   - `FOCAL_BLEND_ENABLED`: `NO (0)`.
   - `FOCAL_BLEND_STATE`: `srcBlend=ONE, dstBlend=ZERO, blendOp=ADD`.
   - `FOCAL_N64_BLENDER_STATE`: `blenderInputs=0x0000, forceBlend=0, combineCycleCount=1`.
   - Matches central same-DL FRONT control identically.

5. **Depth Configuration**:
   - `FOCAL_DEPTH_TEST`: `YES (1)` (`zCmp = 1`, `zMode = ZMODE_OPA`).
   - `FOCAL_DEPTH_WRITE`: `YES (1)` (`zUpd = 1`).
   - `FOCAL_DEPTH_COMPARE`: `LESS` (`RenderComparisonFunction::LESS`).
   - `FOCAL_DEPTH_CLIP`: `YES (1)` (`NoN = 0` -> `depthClipEnabled = true`).
   - `FOCAL_Z_MODE`: `0 (ZMODE_OPA)`.
   - Vertex NDC Z values ($V_0=0.7543, V_1=0.7763, V_2=0.8599$) are strictly within the valid range $[0.0, 0.9980]$. Neither near nor far depth clipping can discard the triangle.

6. **Scissors, Viewports & Screen Space**:
   - `FOCAL_GUEST_SCISSOR`: `(0, 0, 304, 228)`.
   - `FOCAL_HOST_SCISSOR`: `(0, 0, 1920, 1012)` [scaled to active window].
   - `FOCAL_RASTER_SCISSOR`: `(0, 0, 1920, 1012)`.
   - `FOCAL_GUEST_VIEWPORT`: `(0, 0, 304, 228)`.
   - `FOCAL_HOST_VIEWPORT`: `(0.0, 0.0, 1920.0, 1012.0)`.
   - `FOCAL_RASTER_VIEWPORT`: `(0.0, 0.0, 1920.0, 1012.0, minDepth=0.00, maxDepth=1.00)`.
   - `FOCAL_HOST_SCREEN_BOUNDS`: $X \in [100.0, 503.0]$ px, $Y \in [-203.4, 529.2]$ px.
   - `FOCAL_INTERSECTS_RASTER_SCISSOR`: `YES`. The horizontal span is 100% strictly inside $[0, 1920]$.

7. **Coordinate Space & PosScreen Semantics**:
   - `POSSCREEN_SOURCE`: Computed in CPU RSP HLE only using native 304x228 viewport.
   - `POSSCREEN_USED_FOR_VISIBILITY`: `NO`.
   - `posScreen` is never uploaded to vertex buffers or read by `RasterVS` / `RasterPS`.
   - `POST_PROJECTION_NATIVE_COORDINATE_MISMATCH`: `REFUTED`. The GPU pipeline transforms `posFloats` via `modViewProjTransforms` (aspect-adjusted target projection), and `RasterPS` operates on hardware rasterizer fragment coordinates.

8. **Source vs Target Clip State**:
   - Source 4:3 Clip Masks: $V_0=\text{LEFT+TOP}, V_1=\text{LEFT}, V_2=\text{NONE}$.
   - Target Aspect Clip Masks: $V_0=\text{TOP}, V_1=\text{NONE}, V_2=\text{NONE}$.
   - `SOURCE_CLIP_FLAGS_USED_AFTER_TARGET_EXPANSION`: `NO`.

9. **Exhaustive Fragment Discard Sites Audit (RasterPS.hlsl)**:
   - Site 1 (Dynamic Cull): Inactive (Specialized PSO uses fixed-function `CullMode::FRONT`).
   - Site 2 (Depth Clamp Near): Inactive (`depthClampNear=false`, $Z \le 0.86 < 0.998$).
   - Site 3 (Depth Bounds): Inactive ($Z \in [0.7543, 0.8599] \subset [0, 0.998]$).
   - Site 4 (Depth Decal): Inactive (`zMode == ZMODE_OPA != ZMODE_DEC`).
   - Site 5 (Alpha Compare Dither): Inactive (`alphaCompare == G_AC_NONE`).
   - Site 6 (Alpha Compare Threshold): Inactive (`alphaCompare == G_AC_NONE`).
   - Site 7 (Coverage Zero): Inactive (`cvgXAlpha=false` -> $resultCvg = 8/255 > 1/255$).
   - **Total Active Discard Sites for Focal**: `0 / 7`.

10. **State Comparison vs Central Control**:
    - `FIRST_FOCAL_VS_CENTRAL_STATE_DIFFERENCE`: `NONE_OBSERVED`. The focal extra-left TRI1 shares identical pipeline, shader variant, combiner mux, texture TMEM binding, alpha compare, coverage, blend, depth, and color write state with the central FRONT TRI2 of the same display list.

## TARGETED_FOCAL_VERTEX_WINDING_USER_ARBITRATION (2026-08-29)

```text
CAPTURE_VALID: YES
CAPTURE_ID: 1
CAPTURE_START_FRAME: 3754
CAPTURE_END_FRAME: 4473
CAMERA: (3598.4, 57.8, 4908.3) static 720/720
FOCAL_00349D38_EVENTS: 1445
FOCAL_PRIMITIVE: TRI1 V0=(2497,400,4263) V1=(2497,-153,4263) V2=(2245,-44,3680)
MODEL_LINEAR_DETERMINANT: +1.000000
NEGATIVE_MODEL_DETERMINANT: NO
WORLD_NORMAL: (0.9179, 0.0000, -0.3968)
NORMAL_CAMERA_DOT: -0.5196
GEOMETRIC_EXPECTED_FACING: FRONT
SIGNED_AREA_SOURCE: +0.748114 (CCW)
SIGNED_AREA_TARGET: +0.525758 (CCW)
SOURCE_TARGET_WINDING_MATCH: YES
RASTER_SIGNED_AREA: -0.525758
RASTER_CLASSIFIED_FACING: BACK (Vulkan CLOCKWISE + CULL_FRONT compensation)
GUEST_GEOMETRY_CULL_MODE: G_CULL_BACK
RT64_EFFECTIVE_CULL_MODE: CULL_FRONT
EXPECTED_RASTER_SURVIVAL: YES
FACING_MISMATCH: NO
CULL_SURVIVAL_MISMATCH: NO
FOCAL_CLIP_MASKS_SRC: V0=LEFT+TOP V1=LEFT V2=NONE
FOCAL_CLIP_MASKS_TGT: V0=TOP V1=NONE V2=NONE
FOCAL_GEOMETRY_STABLE: YES
FOCAL_RASTER_FACING_STABLE: YES
CENTRAL_SAME_DL_CONTROL: FRONT TRI2 cluster same survival signature; BACK cluster culled as expected
SECONDARY_0034B800: BACK 1728/1728, expected survival NO
CULL_WINDING_ANOMALY: REFUTED_FOR_FOCAL
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
CURRENT_ROOT_CLASS: WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
NEXT_DISCRIMINATOR: TARGETED_RASTER_STATE_TRACE
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-focal-winding-user-arbitration-20260829T164524Z/
ARTIFACT_CHECKSUM_MANIFEST: d0d993e6e015d482e6807350b8d65de993d6348b7f8d96287972e9d1ee13e489
FIX_IMPLEMENTED: NO
```

New USER capture at the same missing-wall location (camera static; pose
differs from 2026-08-27). Aspect expansion only scales clip X
(`area_tgt/area_src = 0.702785` for this ~1920x1012 window vs 4:3; do not
hardcode 16:9). Winding sign is unchanged. Guest FRONT + `G_CULL_BACK` +
compensated `CULL_FRONT` all say the focal TRI1 should survive. Adjacent
same-DL FRONT TRI2s share that signature, so cull cannot explain a hole
unique to the left strip. Do not pursue cull. No GPU pixel claim.

## TARGETED_FOCAL_VERTEX_WINDING_USER_ARBITRATION (2026-08-27)

```text
CAPTURE_VALID: YES
CAPTURE_ID: 1
CAPTURE_START_FRAME: 8278
CAPTURE_END_FRAME: 8997
CAMERA: (3477.8, 113.9, 4682.8) static 720/720
FOCAL_00349D38_EVENTS: 1440
FOCAL_PRIMITIVE: TRI1 V0=(2497,400,4263) V1=(2497,-153,4263) V2=(2245,-44,3680)
MODEL_LINEAR_DETERMINANT: +1.000000
NEGATIVE_MODEL_DETERMINANT: NO
WORLD_NORMAL: (0.9179, 0.0000, -0.3968)
NORMAL_CAMERA_DOT: -0.5965
GEOMETRIC_EXPECTED_FACING: FRONT
SIGNED_AREA_SOURCE: +1.175925 (CCW)
SIGNED_AREA_TARGET: +0.881943 (CCW)
SOURCE_TARGET_WINDING_MATCH: YES
RASTER_SIGNED_AREA: -0.881943
RASTER_CLASSIFIED_FACING: BACK (Vulkan CLOCKWISE + CULL_FRONT compensation)
GUEST_GEOMETRY_CULL_MODE: G_CULL_BACK
RT64_EFFECTIVE_CULL_MODE: CULL_FRONT
EXPECTED_RASTER_SURVIVAL: YES
FACING_MISMATCH: NO
CULL_SURVIVAL_MISMATCH: NO
FOCAL_CLIP_MASKS_SRC: V0=LEFT+TOP V1=LEFT V2=NONE
FOCAL_CLIP_MASKS_TGT: V0=TOP V1=NONE V2=NONE
FOCAL_GEOMETRY_STABLE: YES
FOCAL_RASTER_FACING_STABLE: YES
CENTRAL_SAME_DL_CONTROL: FRONT TRI2 cluster same survival signature; BACK cluster culled as expected
SECONDARY_0034B800: BACK 1728/1728, expected survival NO
CULL_WINDING_ANOMALY: REFUTED_FOR_FOCAL
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
CURRENT_ROOT_CLASS: WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
NEXT_DISCRIMINATOR: TARGETED_RASTER_STATE_TRACE
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-focal-winding-user-arbitration-20260827T193119Z/
ARTIFACT_CHECKSUM_MANIFEST: f1499333a0fcdb1395f550848e8539026419058f8935c4c8c6b3a9092943667c
FIX_IMPLEMENTED: NO
```

USER capture at the missing-wall pose. Aspect expansion only scales clip X
(area_tgt/area_src = 0.75); winding sign is unchanged. Do not treat telemetry
`raster_front=NO` as an independent Vulkan measurement (`area_raster ≡
-area_tgt`). Guest FRONT + G_CULL_BACK + compensated CULL_FRONT all say the
focal TRI1 should survive. Adjacent same-DL FRONT TRI2s share that signature,
so cull cannot explain a hole unique to the left strip.

## TARGETED_FOCAL_VERTEX_WINDING_DUMP (2026-08-27)

```text
INSTRUMENTATION_STATUS: IMPLEMENTED_CPU_SIDE_SAFE
GPU_READBACKS_USED: NO (0 buffer readbacks, 0 image transfers, 0 thread race conditions)
PRIMARY_FOCAL_CANDIDATE: source_dl = 0x00349d38 extra-left TRI1
SECONDARY_CONTROL: source_dl = 0x0034b800
INTERNAL_CONTROL: source_dl = 0x00349d38 central TRI2 triangles
LOG_SINKS: /tmp/tooie-ws-focal-winding.log + unified [ws-focal-winding]
TELEMETRY_FIELDS: V0/V1/V2 (obj, world, clip_src, clip_tgt, ndc_src, ndc_tgt), model_det, model_det_sign, neg_model_det, area_src, area_tgt, raster_area, raster_front, world_norm, cam_to_tri, norm_cam_dot, geom_facing, cull_mode, raster_survive, clip_mask_src, clip_mask_tgt
ASPECT_SCALE_INVARIANCE_THEOREM: PROVEN (Sx = 1/aspectScale > 0 => sign(Area_tgt) == sign(Area_src) strictly)
SELFTEST_RESULT: 66/66 checks passed (100% sign preservation across 1:1, 4:3, 16:10, 16:9, 21:9, 32:9)
BUILD_STATUS: SUCCESS (ninja exit code 0)
CURRENT_ROOT_CLASS: WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
NEXT_DISCRIMINATOR: SAFE_FOCAL_VERTEX_CAPTURE_EXECUTION
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-focal-vertex-winding-setup-20260827T154601Z/
ARTIFACT_CHECKSUM_MANIFEST: d29cce81fc78995a14de11979ef1933cbcfa8fb0dbf158cbeb1bddf5cdcacc15
FIX_IMPLEMENTED: NO
```

Pure CPU-side vertex telemetry implemented directly inside RT64's `FramebufferRenderer::addDrawCall` (within `namespace RT64`). Zero GPU transfer commands or multithreaded staging barriers are executed, guaranteeing complete stability and preventing desktop display session interruptions.

Telemetry captures:
1. Object space coordinates $(x,y,z)_{obj}$ for $V_0, V_1, V_2$.
2. Model matrix linear 3x3 determinant $\det(W_{3\times3})$ to detect any negative-scaling reflection/inversion (`POSITIVE`/`NEGATIVE`).
3. World space positions $(x,y,z)_{world} = W \cdot (x,y,z,1)_{obj}$.
4. Source and target-aspect clip coordinates and NDC coordinates ($P_{clip}, NDC$).
5. 2D signed NDC areas ($\text{Area}_{src}, \text{Area}_{tgt}$) and raster-space area ($\text{Area}_{raster}$ with Vulkan Y-flip).
6. World-space geometric normal $\hat{N}$, camera world position $E = \text{invView}[3].xyz$, unit vector $\hat{D} = C - E$, and dot product $\hat{N} \cdot \hat{D}$ to establish genuine 3D geometric facing (`FRONT`/`BACK`).
7. 6-plane clip masks (LEFT, RIGHT, BOTTOM, TOP, NEAR, FAR) for $V_0, V_1, V_2$.
8. Pipeline culling survival evaluation under guest `G_CULL_BACK`/`G_CULL_FRONT` mode.

Offline proof and synthetic test matrix (`aspect-scale-selftest.csv`) confirm with 100% precision that horizontal aspect adjustment matrix scaling ($S_x > 0$) preserves triangle winding sign for all possible orientations and positions.

## TARGETED_CULL_WINDING_TRACE (2026-08-27)

```text
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
USER_SCREENSHOT_CROP_MAPPING: APPROXIMATE
FOCAL_SOURCE_DL: 00349d38 extra-left TRI1
FOCAL_GBI_INDICES: v0=0 v1=1 v2=2
FOCAL_OBJECT_XYZ: NOT_IN_CAPTURE
POSITIVE_X_ASPECT_SCALE_CAN_FLIP_WINDING: NO
SHADER_CULL_IMPLEMENTATION: specialized PSO CullMode FRONT + CLOCKWISE
VIEWPORT_Y_FLIPPED: NO
NDC_TO_VULKAN_Y_FLIP: YES
FRONT_FACE_COMPENSATION_PRESENT: YES
TRIANGLE_REQUIRES_CLIPPING: YES
GUARD_BAND_USED: NO
GUEST_GEOMETRY_CULL_MODE: G_CULL_BACK
RT64_EFFECTIVE_CULL_MODE: CULL_FRONT (Y-flip compensation)
GUEST_RT64_CULL_MODE_MATCH: YES_AFTER_Y_FLIP_COMPENSATION
FACING_MISMATCH: UNKNOWN
CULL_WINDING_ANOMALY: UNKNOWN
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
CURRENT_ROOT_CLASS: WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
NEXT_DISCRIMINATOR: TARGETED_FOCAL_VERTEX_WINDING_DUMP
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-targeted-cull-winding-20260827T152926Z/
ARTIFACT_CHECKSUM_MANIFEST: c260d0fd338f4b14039e65bc70a33601318682f7686ba5c72441cc18e0d2eb56
FIX_IMPLEMENTED: NO
```

No new USER capture. Pipeline math: `adjustProjectionMatrix` scales column 0
by `1/(target_aspect/source_aspect)` (positive). That cannot invert 2D signed
area. `SHADER_CULL` is not a shader discard on the specialized path; it is
fixed-function FRONT cull after RasterVS Y divide-by-negative and DXC
`-fvk-invert-y`. The extra-left TRI1 straddles clip (`ndcX < -1`) so GPU clip
applies; clip is not a winding flip. Exact signed area / facing still missing
because logs only have AABB + GBI cache indices.

Do not bypass cull. Do not hardcode 16:9.

## CPU_SCREEN_BOUNDS_WALL_CANDIDATE_ARBITRATION (2026-08-27)

```text
MISSING_WALL_DRAW_SEMANTIC_JOIN_INITIAL: NO
MISSING_WALL_DRAW_SEMANTIC_JOIN_FINAL: PARTIAL
CAPTURE_VALID: YES
CAPTURE_ID: 1
CAPTURE_START_FRAME: 13309
CAPTURE_END_FRAME: 14028
CAMERA_EFFECTIVELY_STATIC: YES (720/720)
CAM_POS: (3467.4, 105.0, 4735.7) rot=(-0.54,-0.16,-0.83)
WALL_BOUNDS_EVENTS: 41142 (capture_id=1 in window)
USER_HOLE_SCREEN_RECT: x[0.00,0.12] y[0.00,0.65] (origin top-left)
USER_HOLE_NDC_RECT: x[-1.00,-0.76] y[-0.30,1.00]
TOP_CANDIDATE_SOURCE_DL: 00349d38
TOP_CANDIDATE_PRIMITIVE_RANGE: extra-left TRI1 (prim_count=1, pe=ps); DL also has central TRI2 clusters
TOP_CANDIDATE_WORLD_AABB: (2245,-153,3680)-(2497,400,4263)
TOP_CANDIDATE_NDC_BOUNDS (wide copy): (-1.3163,-0.5891)x(-1.4054,0.2944) hole ioh~0.457
TOP_CANDIDATE_STATIC_WORLD: YES (MAP_RAM)
TOP_CANDIDATE_VERTICAL_SURFACE: YES (vert_frac=1.0)
TOP_CANDIDATE_GPU_SUBMITTED: YES (318/318)
TOP_WALL_CULL_MODE: BACK + SHADER_CULL
TOP_WALL_WINDING: UNKNOWN
TOP_WALL_NEGATIVE_DETERMINANT: UNKNOWN
CULL_WINDING_ANOMALY: UNKNOWN
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
CURRENT_ROOT_CLASS: WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
NEXT_DISCRIMINATOR: TARGETED_CULL_WINDING_TRACE
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-user-wall-bounds-arbitration-20260827T151459Z/
ARTIFACT_CHECKSUM_MANIFEST: cd50d98a55c0ab24bb1ed3d982ae4b648ba148b3bdc2ed0182f31189dd6018b4
FIX_IMPLEMENTED: NO
```

USER_SELECTED capture (`TOOIE_DIAG_WS_CONTINUOUS=1` + `TOOIE_DIAG_WS_WALL_BOUNDS=1`)
plus a same-camera screenshot. Hole is **extra_left** neon-green clear. Nearby
opaque vertical MAP_RAM cluster `00349d38` extra-left TRI1 overlaps that NDC
box and is GPU-submitted while the hole is visible. Sibling `0034b800` is a
second nearby cliff cluster. Distant proj=1 `0031e138`/`0031e7f8` cover the
hole AABB but are sky/horizon-scale (~15k from camera) and are **demoted**.
Smoke DLs `0031f6d8`/`00320218`/`0031f8f8` are **not** this scene.

Join is **PARTIAL** not PROVEN: the TRI1 only reaches `ndcY≈0.29` while the
green void goes to the top of the frame; the brown wall may be a union of
`00349d38`+`0034b800`. Case **B**: wall-specific draw reaches GPU → raster/cull
state boundary. No GPU readback. No fix.

## SEMANTIC_WALL_DRAW_LOCALIZATION (2026-08-27)

```text
MISSING_WALL_DRAW_SEMANTIC_JOIN_INITIAL: NO
MISSING_WALL_DRAW_SEMANTIC_JOIN_FINAL: NO
USER_PERSISTENT_EXTRA_BAND_DL_COUNT: 8
USER_DL_HITS_IN_REMAINING_LOGS: 0
GEOMETRY_DL_COUNT (smoke proj1 extra-band): 5
STATE_OR_MATERIAL_DL_COUNT: 0 among those 5 (all TRI* leaves)
STATIC_WORLD_DL_COUNT: 5 MAP_RAM_CANDIDATE
DL_WORLD_BOUNDS_AVAILABLE: YES (CPU replica of RSP world matrix * posFloats)
SCREEN_Y_BOUNDS_AVAILABLE: YES (posScreen → NDC Y)
TOP_SMOKE_CANDIDATES: 0031f6d8 (vertical subset) 00320218 0031f8f8; 0031f5a8/0031f658 HORIZONTAL demote
USER_CAMERA_PERSISTED: (3520.6,94.1,4758.0) — not present in remaining logs
COMPOSITE: still wide-safe; do not reopen
PIXEL_PROBE: still ABANDONED_FOR_SAFETY
CURRENT_ROOT_CLASS: GPU_FRAGMENT_RASTER_OR_SEMANTIC_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
NEXT_DISCRIMINATOR: CPU_SCREEN_BOUNDS_WALL_CANDIDATE_CAPTURE
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-semantic-wall-localization-20260827T063054Z/
ARTIFACT_CHECKSUM_MANIFEST: 2bed363d288b066a76cb656047e2a15c5e988684ad4442ed67aafb67d62e7d51
FIX_IMPLEMENTED: NO
```

Prior USER GPU artifact
`/tmp/p08-widescreen-gpu-depth-user-arbitration-20260827T033147Z/` is **missing**
on disk. Composite artifact
`/tmp/p08-widescreen-post-submission-composite-audit-20260827T052315Z/` verified
(`444e33af...`). Remaining `/tmp/tooie-ws-dl-identity.log` is a **different**
scene (camera starts `(529.4,6943.9,-1904.1)`, 137 unique poses). Documented
persistent DLs `0031e138`…`0031e9f8` have **zero** hits there.

CPU telemetry `[ws-wall-bounds]` (no GPU copy) proved: one source DL can mix
horizontal and vertical primitive ranges (`0031f6d8`). Extra-band proj=1
`geom_cull=BACK`; `opaque_guess=0` (`zCmp` off), matching the USER extra-band
depth-off set. That **INFERENCE** weakens “extra-band proj=1 == missing opaque
wall” but does not identify the hole.

Do **not** promote smoke `0031f6d8` as the user wall.

## POST_SUBMISSION_COMPOSITE_STATE_TRACE (2026-08-27)

```text
FINAL_RENDER_TARGET_PIXEL_PROBE: NOT_COMPLETED / ABANDONED_FOR_SAFETY
GPU_PIXEL_READBACK_CURRENT_APPROACH: UNSAFE
LIVE_GPU_READBACK: DO_NOT_RETRY_WITH_CURRENT_APPROACH
EXTRA_BAND_PROJ1_DRAWS_REACH_GPU_DURING_VISIBLE_GLITCH: PROVEN
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: NOT_PROVEN
MISSING_WALL_DRAW_SEMANTIC_JOIN: NO
VIRENDERER_COMPOSITE_ROOT: REFUTED as extra-band X crop; NOT_PROVEN as wall visual root
GPU_FRAGMENT_RASTER_ROOT: NOT_PROVEN
VI_UV_COVERS_WIDE_SOURCE: PROVEN (smoke uvMax.x=1.0)
MSAA_RESOLVE_WIDESCREEN_SAFE: YES
POST_RENDER_4_3_CROP_PATH: NOT_FOUND
POST_RENDER_SOURCE_CLAMP: FOUND (shader; runtime X does not drop extra-band)
POST_RENDER_PILLARBOX_PATH: FOUND (window contain-fit / copyFromChanges)
WIDE_EXTENT_PRESERVED_THROUGH_PRESENT: PROVEN (smoke)
COMPOSITE_CROP_ROOT: STRONGLY_WEAKENED
DECISION: B GPU_FRAGMENT_RASTER_OR_SEMANTIC_BOUNDARY
NEXT_DISCRIMINATOR: SEMANTIC_WALL_DRAW_LOCALIZATION
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-post-submission-composite-audit-20260827T052315Z/
ARTIFACT_CHECKSUM_MANIFEST: 444e33af976f32ba2c4f94409a2210bf80871346281554e9891c66291bb2a2d2
FIX_IMPLEMENTED: NO
```

Three Vulkan incidents (swapchain without TRANSFER_SRC; MSAA
`vkCmdCopyImageToBuffer`; workload vs present thread) are **not** pixel evidence.
Do not claim the missing wall itself reaches GPU. Composite does not crop
extra-band X: resolve is full-target; VI UV `LowerRight.x=1`; view/crop rects
stubbed `{0,0,1,1}`. 304/228 is native VI size for host allocation, not a
sample window into a wider image.

## FINAL_RENDER_TARGET_PIXEL_PROBE (2026-08-27) — ABANDONED

```text
PIXEL_PROBE_IMPLEMENTATION:   NOT_COMPLETED / ABANDONED_FOR_SAFETY
VULKAN_SWAPCHAIN_DIRECT_READBACK: REFUTED_UNSAFE (VK_IMAGE_USAGE_TRANSFER_SRC_BIT missing on VkSwapchainKHR; triggers GPU reset / display session crash)
VULKAN_MSAA_MULTISAMPLE_DIRECT_COPY: REFUTED_UNSAFE (vkCmdCopyImageToBuffer forbidden on sampleCount > 1 per Vulkan VUID-vkCmdCopyImageToBuffer-srcImage-00186; triggers GPU reset / display session crash)
GPU_PIXEL_READBACK_CURRENT_APPROACH: UNSAFE
LIVE_GPU_READBACK:            DO_NOT_RETRY_WITH_CURRENT_APPROACH
CURRENT_ROOT_CLASS:           POST_GPU_SUBMISSION_FRAGMENT_OR_COMPOSITE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
FIX_IMPLEMENTED:              NO
```

### Safety & Architecture Incident Notes: Vulkan RHI Restrictions

1. **Incident 1 — Direct SwapChain Copy (`swapChainTexture`)**:
   - *Failure Mode*: Attempting to record `copyTextureRegion` / `vkCmdCopyImageToBuffer` directly on `swapChainTexture` in `PresentQueue::threadPresent` caused the desktop window manager / display server (Xorg/Wayland) to crash and drop the user login session.
   - *Root Cause*: `VulkanSwapChain::resize()` in `BanjoRecomp/lib/rt64/src/contrib/plume/plume_vulkan.cpp` creates the native `VkSwapchainKHR` image with `VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT` (lacking `VK_IMAGE_USAGE_TRANSFER_SRC_BIT`). Submitting a transfer copy from a swapchain surface without `TRANSFER_SRC` is an illegal Vulkan command that causes a hardware GPU fault (XID error / GPU reset), terminating the OS compositor.
   - *Resolution*: Direct swapchain readback removed. Probe restricted to offscreen targets.

2. **Incident 2 — Direct Multisample Texture Copy (`colorTarget->texture`)**:
   - *Failure Mode*: Copying `colorTarget->texture` in `WorkloadQueue::threadWorkload` caused a GPU reset and display session drop when MSAA was enabled (default MSAA 4X).
   - *Root Cause*: Vulkan specification **VUID-vkCmdCopyImageToBuffer-srcImage-00186** dictates that `srcImage` must have a sample count of `VK_SAMPLE_COUNT_1_BIT`. Passing a multisampled image (`samples = 4`) directly to `vkCmdCopyImageToBuffer` is an illegal Vulkan command that triggers an unrecoverable GPU fault.
   - *Resolution*: Invoked `colorTarget->resolveTarget(worker, shaderLibrary)` and copied from `colorTarget->getResolvedTexture()`, which is guaranteed to be a single-sample (`samples = 1`) image.

3. **Incident 3 — Multi-Queue Cross-Thread State Race during Continuous Active Rendering**:
   - *Failure Mode*: Executing asynchronous readback transitions on `colorTarget->getResolvedTexture()` in `threadWorkload` while `threadPresent` was concurrently presenting/resolving frames caused Vulkan multi-queue synchronization instability and display server resets during rapid frame execution.
   - *Resolution*: GPU readbacks and texture copies removed entirely from the active render/present threads. Causal analysis advances via deterministic pipeline stage classification (VIRenderer / VI UV mapping / widescreen presentation scissor contracts).

## GPU_DEPTH_USER_ARBITRATION (2026-08-27)

USER_SELECTED capture (`touch /tmp/tooie-ws-capture`) while the lateral wall
was visibly missing. Camera static for **720/720** frames. Reference setup
`/tmp/p08-widescreen-extra-band-depth-visibility-setup-20260827T032333Z/`
(manifest `386063613e37c2b2272957489a0f0b740c05d803b8745503137dc126ab44d712`)
is **not** user evidence.

```text
USER_SELECTED_WIDESCREEN_MISSING_WALL: YES
CAPTURE_VALID: YES
CAPTURE_ID: 1
CAPTURE_START_FRAME: 6885
CAPTURE_END_FRAME: 7604
CAMERA_EFFECTIVELY_STATIC: YES
SMOKE_SUBMISSION_COUNT_DISCREPANCY_ROOT: ACCOUNTING_WINDOW_PLUS_ONE_DUPLICATE_GPU_SUBMIT
SMOKE_TRUE_SUBMISSION_JOIN_RATE: 100.0%
USER_EXTRA_BAND_DRAWS_CLASSIFIED: 56718
USER_EXTRA_BAND_DRAWS_CLASSIFIED_PROJ1: 3288
USER_EXTRA_BAND_DRAWS_GPU_SUBMITTED_PROJ1: 3288
USER_EXTRA_BAND_SUBMISSION_JOIN_RATE: 100.0% (prim identity)
DRAW_TO_GPU_SUBMISSION_FAILURE: REFUTED
GPU_SUBMITTED_EXTRA_BAND_SET_BEHAVIOR: STABLE
DEPTH_TEST_STATE_FOR_EXTRA_BAND: OFF (proj1)
DEPTH_WRITE_STATE_FOR_EXTRA_BAND: OFF
DEPTH_COMPARE_FOR_EXTRA_BAND: ALWAYS_OR_OFF
EXTRA_BAND_DEPTH_STATE_DIFFERS_FROM_CENTRAL: NO
COLOR_TARGET_HOST_EXTENT: 2164x1140
DEPTH_TARGET_HOST_EXTENT: 2164x1140
COLOR_DEPTH_EXTENT_MATCH: YES
EXTRA_BAND_DEPTH_TARGET_TRUNCATION: REFUTED_FOR_USER_CAPTURE
NEAR_PLANE_INTERSECTION_COUNT: 0
FAR_PLANE_INTERSECTION_COUNT: 411
FAR_DEPTH_CLIP_ANOMALY: REFUTED
LATERAL_VS_CENTRAL_TARGET_DIFFERENCE: NO
BANJO_NEAR_CLIPPING_REFERENCE_RELEVANCE: LOW
MISSING_WALL_DRAW_SEMANTIC_JOIN: NO
ARBITRATION_CASE: C
CURRENT_ROOT_CLASS: POST_GPU_SUBMISSION_FRAGMENT_OR_COMPOSITE_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY: after GPU drawIndexedInstanced/drawInstanced
LAST_IDENTITY_PRESERVING_BOUNDARY: GPU_DRAW_SUBMISSION
FIX_IMPLEMENTED: NO
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-gpu-depth-user-arbitration-20260827T033147Z/
ARTIFACT_CHECKSUM_MANIFEST: 6996bd3bcf9974c86d95fc4650f08bc8b061faf407b5fa80ed39288aa94c2eca
```

1. **Smoke 29498 vs 29502:** window filter dropped 3 classified extra-band
   draws before `guest_frame=245`; 1 leftover GPU submit is a duplicate
   `submitRasterScene` visit. Primitive-identity join is 100%. Not
   Indexed vs Instanced double-log.
2. **CASE A REFUTED:** naive `guest_frame+call` misses are classification vs
   GPU `tooie_get_current_guest_frame()` desync (+1/-1/-2). Same
   `source_dl+prim_start+prim_end` always submits.
3. **CASE B REFUTED** for depth/target/near: all 3288 proj=1 extra-band GPU
   rows share one state `(depth_test=0, depth_write=0, depth_clip=0)`,
   color/depth 2164x1140 match, near intersections 0. Far `maxNdcZ>1` is
   at most 1.0005 with `depth_clip=0` (clamp, not discard).
4. **CASE C:** GPU submission of **extra-band proj=1 draws** is continuous during
   the missing wall. That is not a join to the wall mesh.
   `POST_RT64_DRAW_VISIBILITY_BOUNDARY` is `CORRECTED/SUPERSEDED`.
   Do **not** read this as `MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU`.
5. **CASE D residual:** `MISSING_WALL_DRAW_SEMANTIC_JOIN: NO`. Extra-band
   draws being GPU-healthy is not a wall-mesh join.
6. Guest candidates (CAB70/CACEC/80801174/CAA24) remain
   `REFUTED_FOR_OBSERVED_SYMPTOM` / prior run. No new guest evidence.

## EXTRA_BAND_FRAGMENT_DEPTH_VISIBILITY (2026-08-27)

```text
POST_DRAW_CPU_REJECTION_FOUND: NO
DRAW_SURVIVES_TO_GPU_SUBMISSION: YES (100.0%)
EXTRA_BAND_DRAWS_CLASSIFIED:  29498
EXTRA_BAND_DRAWS_GPU_SUBMITTED: 29502
EXTRA_BAND_SCISSOR_EMPTY_COUNT: 0
EXTRA_BAND_DRAW_SUBMISSION_RATE: 100.0%
DEPTH_TEST_STATE_CAPTURE_READY: YES
COLOR_DEPTH_EXTENT_MATCH:     YES (304x228 / 1622x912)
DEPTH_COORDINATE_SPACE:       HOST_WIDESCREEN_PIXELS / NDC
EXTRA_BAND_DEPTH_COVERAGE_MISMATCH: REFUTED
CLIP_STATE_CAPTURE_READY:     YES
BANJO_NEAR_CLIPPING_EFFECT_STAGE: RASTERIZER_DEPTH_CLIP_ENABLE
BANJO_NEAR_CLIPPING_CAN_EXPLAIN_LATERAL_HOLES: NO
FRAGMENT_VISIBILITY_DIRECTLY_OBSERVABLE: NO (proxy active)
PASS_IDENTITY_CAPTURE_READY:  YES
CURRENT_ROOT_CLASS:           POST_RT64_DRAW_VISIBILITY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
USER_CAPTURE_READY:           YES
FIX_IMPLEMENTED:              NO
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-extra-band-depth-visibility-setup-20260827T032333Z/
ARTIFACT_CHECKSUM_MANIFEST:   386063613e37c2b2272957489a0f0b740c05d803b8745503137dc126ab44d712
```

1. **GPU Draw Submission Survival:** 100% of extra-band classified draws survive
   through `submitRasterScene` to `drawIndexedInstanced` / `drawInstanced`.
   `triangles.scissor.isEmpty()` skipped 0 draws.
2. **Depth Target Continuity:** `colorTarget` and `depthTarget` extents match 1:1
   identically (both resized to `rtWidth, rtHeight` and guaranteed by
   `rt64_workload_queue.cpp` line 578). `EXTRA_BAND_DEPTH_COVERAGE_MISMATCH` is `REFUTED`.
3. **Depth Coordinate Space:** NDC coordinates span the full widened viewport;
   depth testing and writing are executed in host pixel coordinates across the extra bands.
4. **Banjo Near-Clipping Reference:** In Banjo/RT64, `gEXSetNearClipping(FALSE)`
   controls `NoN` -> `depthClipEnabled = false` (depth clamping). This affects near-plane
   $Z$ ($Z < 0$), not lateral $X$ culling. It cannot explain lateral missing wall holes.
5. **Next Discriminator:** Perform one authoritative USER_SELECTED capture to
   record the live `[ws-gpu-draw-submit]` stream at the exact moment of the visual glitch.

## RSP_DRAW_USER_ARBITRATION (2026-08-27)

USER_SELECTED capture (`touch /tmp/tooie-ws-capture`) while missing wall
polygons were visible. Reference smoke artifact remains
`/tmp/p08-widescreen-rsp-primitive-identity-setup-20260827T015059Z/`
(manifest `bb7ea94f1a8f71cb101c0cc4cb38832ccd5aa219240f2526669537eadcdd88e0`)
and is **not** user evidence.

```text
USER_SELECTED_WIDESCREEN_VISIBILITY_ERROR: YES
CAPTURE_VALID: YES
CAPTURE_ID: 1
CAPTURE_START_FRAME: 245023
CAPTURE_END_FRAME: 245742
RSP_PRIMITIVES_TOTAL: 604428
RSP_PRIMITIVES_WITH_DRAW: 604088
RSP_PRIMITIVES_WITHOUT_DRAW: 340
PRIMITIVE_TO_DRAW_JOIN_RATE: 99.9437% (RSP TRI*)
RT64_DRAWS_TOTAL: 212560
EXTRA_LEFT_DRAWS: 28831
EXTRA_RIGHT_DRAWS: 21725
EXTRA_BAND_DRAWS_WITH_SOURCE_DL: 48976
EXTRA_BAND_DRAWS_WITH_PRIMITIVE_ID: 48976
EXTRA_BAND_SOURCE_DL_UNIQUE_COUNT: 81
EXTRA_BAND_PRIMITIVES_UNIQUE_COUNT: 150662
EXTRA_BAND_DRAW_SET_BEHAVIOR: STABLE on proj=1 world (Jaccard 1.0); CHANGING if HUD/proj2 mixed
PERSISTENT_EXTRA_BAND_DRAWS: 8 proj=1 DLs (EL 0031e138 0031e2e0 0031e568 0031e5b0 0031e7f8; ER 0031e2e0 0031e490 0031e568 0031e5b0 0031e8f8 0031e9f8)
TRANSIENT_EXTRA_BAND_DRAWS: 0 on proj=1 extra-band set
RSP_TO_RT64_DRAW_CREATION_FAILURE: REFUTED
RT64_DRAW_PRESENT_DURING_VISUAL_ERROR: PROVEN (structural; no visual→DL join)
CLIP_REJECTION_EVIDENCE: NOT_OBSERVABLE
DEPTH_REJECTION_EVIDENCE: NOT_OBSERVABLE
BANJO_NEAR_CLIPPING_REFERENCE_RELEVANCE: MEDIUM
LAST_IDENTITY_PRESERVING_BOUNDARY: RT64_HOST_DRAW_EXTRA_BAND_CLASSIFICATION
FIRST_CAUSAL_DIFF: extra-band DrawCalls exist throughout the hole; loss is after classification
CURRENT_ROOT_CLASS: POST_RT64_DRAW_VISIBILITY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY: after FramebufferRenderer extra-band DrawCall classification
ARBITRATION_CASE: B
FOCAL_DL_0x8035dd20: WEAKENED
CAB70/E3DC0/CACEC/BBOX/SPHERE: REFUTED_FOR_OBSERVED_SYMPTOM
GCMAPSECTS_80801174_REQUIRED_FOR_OBSERVED_SYMPTOM: REFUTED_FOR_PRIOR_MANUAL_RUN
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-rsp-draw-user-arbitration-20260827T030225Z/
LATEST_SHA256SUMS_SHA: 2084f416c77a64082a459bf58244cc49a715972efefb454d131a441c13b914ed
```

Gameplay camera is static `(3520.6,94.1,4758.0)` for 712/720 frames. 340 RSP
prims without draw are capture-tail only (`245737..245741`). RECT 1424 unjoins
are dual `prim_seq` counters, not missing 2D. Overlay 662 syscalls in this
window: 0.

Prior `RSP_TO_RT64_GEOMETRY_BOUNDARY` at draw-creation is
`CORRECTED/SUPERSEDED` for this capture. Do not reopen guest CAB70/CACEC/
80801174/CAA24/focal `0x8035dd20`.

`NEXT_DISCRIMINATOR:` `EXTRA_BAND_FRAGMENT_DEPTH_VISIBILITY`
`NEXT_SINGLE_ACTION:` Diagnostic RT64 fragment/depth counters on the stable proj=1 extra-band DL set. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## RSP_PRIMITIVE_IDENTITY_PROPAGATION (2026-08-27)

```text
RSP_PRIMITIVE_PROPAGATION:    IMPLEMENTED_AND_VERIFIED
DL_CONTEXT_STACK_ACTIVE:      YES
DL_CONTEXT_EVENTS:            8597
PRIMITIVES_WITH_DL_IDENTITY:  76229
RT64_DRAWS_WITH_PRIM_ID:      45506
JOIN_RATE_DL_TO_PRIMITIVE:    100.0%
JOIN_RATE_PRIMITIVE_TO_DRAW:  100.0%
CURRENT_ROOT_CLASS:           RSP_TO_RT64_GEOMETRY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY:        RSP_G_DL_TO_RT64_DRAW
LAST_IDENTITY_PRESERVING_BOUNDARY: RT64_HOST_DRAW_EXTRA_BAND_CLASSIFICATION
ARTIFACT_DIRECTORY:           /tmp/p08-widescreen-rsp-primitive-identity-setup-20260827T015059Z/
ARTIFACT_CHECKSUM_MANIFEST:   bb7ea94f1a8f71cb101c0cc4cb38832ccd5aa219240f2526669537eadcdd88e0
FIX_IMPLEMENTED:              NO
```

End-to-end identity chain from nested guest `G_DL` -> RSP HLE DL context stack ->
primitive sequence generation (`TRI1`, `TRI2`, `QUAD`, `RECT`) -> `DrawCall` batching
-> `FramebufferRenderer` host widescreen NDC extra-band classification (`minNdcX`,
`maxNdcX`, `extra_left_intersect`, `extra_right_intersect`) is verified and active.
Single unified log `/tmp/tooie-ws-dl-identity.log`. Smoke test confirmed 100% join rate.
Artifacts dual-verified in `/tmp/p08-widescreen-rsp-primitive-identity-setup-20260827T015059Z/`.

## POST_GCMAPSECTS_REANCHOR (2026-08-27)

User observed missing wall polygons in real gameplay with logging on.

```text
USER_OBSERVED_WIDESCREEN_GLITCH_IN_RUN: YES
OVERLAY662_TOTAL_SYSCALLS: 39
EP4_COUNT: 27
EP15_COUNT: 12
EP9_COUNT: 0
EP13_COUNT: 0
EP17_COUNT: 0
GCMAPSECTS_ENTRY_COUNT: 0
SPHERE_EVENTS_IN_GCMAPSECTS: 0
CAA24_EVENTS_IN_GCMAPSECTS: 0
GCMAPSECTS_80801174_ACTIVE_FOR_OBSERVED_RUN: NO
GCMAPSECTS_80801174_REQUIRED_FOR_OBSERVED_SYMPTOM: REFUTED_FOR_THIS_RUN
CAA24_GCMAPSECTS_PATH_REQUIRED_FOR_OBSERVED_SYMPTOM: REFUTED_FOR_THIS_RUN
EP4_FUNCTION: gcmapsects_entrypoint_4
EP4_ROLE: point-in-section AABB query; return index or -1
EP4_ARGUMENT_A0_SEMANTICS: vec3 query (Actor.position / 0x9C-stride)
EP4_MUTATES_RENDER_SELECTION: NO
EP4_CAN_SKIP_GEOMETRY: NO
EP4_CAMERA_DEPENDENT: NO
EP15_ROLE: init/clear map section object (clears +0x80); not a renderer
EP4_POINTER_CLUSTER_SEMANTICS: Actor.position 0x9C stride (PROVEN arithmetic + sizeof(Actor))
STATIC_WORLD_RENDER_PATH: map payload + gccubeDll/gsprops/geo -> GraphicsTask G_DL -> RSP -> RT64
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED
GUEST_DL_TO_RSP_CONTINUITY: PROVEN
LAST_IDENTITY_PRESERVING_BOUNDARY: RSP_HLE_G_DL_VISIT
IDENTITY_LOST_AT: RSP_G_DL_TO_RT64_DRAW
CURRENT_ROOT_CLASS: RSP_TO_RT64_GEOMETRY_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY: RSP_G_DL_TO_RT64_DRAW
BANJO_NEAR_CLIPPING_REFERENCE_RELEVANCE: MEDIUM
USER_MANUAL_CAB70_BYPASS: NO_CHANGE
USER_MANUAL_E3DC0_BYPASS: NO_CHANGE
USER_MANUAL_CACEC_BYPASS: NO_CHANGE
CAB70_DIRECT_VISUAL_ROOT: REFUTED_FOR_OBSERVED_SYMPTOM
E3DC0_DIRECT_VISUAL_ROOT: REFUTED_FOR_OBSERVED_SYMPTOM
CACEC_DIRECT_VISUAL_ROOT: REFUTED_FOR_OBSERVED_SYMPTOM
BBOX_VISUAL_ROOT: REFUTED_FOR_OBSERVED_SYMPTOM
SPHERE_VISUAL_ROOT: REFUTED_FOR_OBSERVED_SYMPTOM
PRIOR_ROOT_CLASS: GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY CORRECTED/SUPERSEDED for this run
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-post-gcmapsects-reanchor-20260827T012256Z/
LATEST_SHA256SUMS_SHA: 3c626fd6b1b1d44520550066ef7c34d6d0c2077bb26b0b8b29325623b33e0e37
```

Do not overclaim globally: maps that tick ep 9/17 may still use 80801174.
This observed symptom did not. Do not chase focal `0x8035dd20`. Do not
reopen CAB70/E3DC0/CACEC. Do not implement Banjo `gEXSetNearClipping`.

`NEXT_DISCRIMINATOR:` `RSP_PRIMITIVE_IDENTITY_PROPAGATION`
`NEXT_SINGLE_ACTION:` Nested G_DL → primitives → RT64 draw extra-band class. No bypass. No fix. No new user capture this round.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## FOCAL_BRANCH_JOIN_REPAIR (2026-08-27)

No new USER_SELECTED capture. Preserve VALID window **1717..2436**, focal
`0x8035dd20` present 1720..2432, neighbor `0x8035d268` dropout at 1856,
`FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED`,
`FOCAL_CLUSTER_TRANSITION: MIXED_ASYMMETRIC_NEIGHBOR_DROPOUT`,
`SECTION_BLOCK_ACTIVATION: NOT_SUPPORTED`, `[ws-focal-branch]=0`.

```text
OLD_RA_GATE_ROOT_CAUSE: $ra would be 0x808011CC / 0x808011F4 not 0x80801174; recomp LOOKUP leaves r31 unmaterialized; live gate reconstructed 800E3E8C SP; add_loaded_function(0x80801174) misses CreateStatic; dispatch table address-takes EPs so --wrap(ep9/17) misses syscalls
SPHERE_CALL_PC_IN_80801174: 0x808011C4
SPHERE_EXPECTED_RA: 0x808011CC
CAA24_PATH_CALL_PC_IN_80801174: 0x808011EC
CAA24_EXPECTED_RA: 0x808011F4
80801174_ADDRESS_SEMANTICS: linked overlay VRAM (gcmapsects 0x80800000..0x808029C0, ROM 0x02108690)
OVERLAY_RUNTIME_MAPPING: ovl_load ram backing this smoke 0x80198ff0; func_map keyed by ram+0x1174
CTX_R31_SEMANTICS: not jal return PC
CTX_PC_SEMANTICS: not live guest PC
DIRECT_GCMAPSECTS_HOOK: YES
HOOK_MECHANISM: ld --wrap func_80801174_gcmapsects; unlimited [ws-gcmapsects-syscall] for ovl=662; LOOKUP rebind after load_overlays
SECTION_POINTER_SOURCE: ctx->r4
SECTION_INDEX_SOURCE: ctx->r5
SECTION_ENTRY_SOURCE: *(section+0x68)+index*0x58
0x800455b0 / 0x80045618: KSEG0 stack/local GeoCmdE nodes, not 80801174 section entries
CAA24_SEMANTIC_ROLE: plane-separation predicate over AABB
CAA24_VISIBLE_RETURN: 1
CAA24_REJECT_RETURN: 0
D_8012D500: camera/viewport-related volume pointer; WRITER UNKNOWN
BUILD_RESULT: PASS
GC_MAPSECTS_ENTRY_COUNT: 0
SPHERE_EVENTS_IN_GCMAPSECTS: 0
CAA24_EVENTS_IN_GCMAPSECTS: 0
CALL_ID_JOIN_ERRORS: 0
UNKNOWN_GUEST_FRAME_EVENTS: 0
GC_MAPSECTS_HOOK_VERIFIED: NO
USER_CAPTURE_READY: NO
FIX_IMPLEMENTED: NO
CURRENT_ROOT_CLASS: GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: OBSERVABILITY_REPAIR
CURRENT_ROOT_BOUNDARY: func_80801174_gcmapsects
LATEST_ARTIFACT: /tmp/p08-widescreen-focal-branch-join-repair-20260827T004617Z/
LATEST_SHA256SUMS_SHA: 2c84d1739211aff278e4e469f3af40e816640cc2f0304ff6493d62c9d1f19cfc
```

45s attract smoke: one `[ws-gcmapsects-syscall] ep=15`. `80801640` calls
`80801174` only if `section+0x80 != 0` (ep 13 setter; ep 15 clears). Do not
promote CAA24 from the persistent focal DL. Do not bypass.

`NEXT_SINGLE_ACTION:` Get `[ws-gcmapsects-entry] > 0` on the glitch map, then one USER_SELECTED capture. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## Prior FOCAL_BRANCH_JOIN_REPAIR wrap-only smoke (2026-08-27T002831Z)

No new USER_SELECTED capture. Preserve VALID window **1717..2436**, focal
`0x8035dd20` present 1720..2432, neighbor `0x8035d268` dropout at 1856,
`FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED`,
`FOCAL_CLUSTER_TRANSITION: MIXED_ASYMMETRIC_NEIGHBOR_DROPOUT`,
`SECTION_BLOCK_ACTIVATION: NOT_SUPPORTED`, `[ws-focal-branch]=0`.

```text
OLD_RA_GATE_ROOT_CAUSE: $ra would be 0x808011CC / 0x808011F4 not 0x80801174; recomp LOOKUP leaves r31 unmaterialized; live gate reconstructed 800E3E8C SP; add_loaded_function(0x80801174) misses same-section CreateStatic and ram+offset LOOKUP keys
SPHERE_CALL_PC_IN_80801174: 0x808011C4
SPHERE_EXPECTED_RA: 0x808011CC
CAA24_PATH_CALL_PC_IN_80801174: 0x808011EC
CAA24_EXPECTED_RA: 0x808011F4
80801174_ADDRESS_SEMANTICS: linked overlay VRAM (gcmapsects 0x80800000..0x808029C0, ROM 0x02108690)
OVERLAY_RUNTIME_MAPPING: ovl_load ram backing 0x801xxxxx / 0x803xxxxx; func_map keyed by ram+0x1174
CTX_R31_SEMANTICS: not jal return PC
CTX_PC_SEMANTICS: not live guest PC
DIRECT_GCMAPSECTS_HOOK: YES
HOOK_MECHANISM: ld --wrap func_80801174_gcmapsects; thread_local call_id stack; 800E3E8C/800E3D0C/800CAA24 join only while stack non-empty
SECTION_POINTER_SOURCE: ctx->r4
SECTION_INDEX_SOURCE: ctx->r5
SECTION_ENTRY_SOURCE: *(section+0x68)+index*0x58
0x800455b0 / 0x80045618: KSEG0 stack/local GeoCmdE nodes, not 80801174 section entries
CAA24_SEMANTIC_ROLE: plane-separation predicate over AABB
CAA24_VISIBLE_RETURN: 1
CAA24_REJECT_RETURN: 0
D_8012D500: camera/viewport-related volume pointer; WRITER UNKNOWN
BUILD_RESULT: PASS
GC_MAPSECTS_ENTRY_COUNT: 0
SPHERE_EVENTS_IN_GCMAPSECTS: 0
CAA24_EVENTS_IN_GCMAPSECTS: 0
CALL_ID_JOIN_ERRORS: 0
UNKNOWN_GUEST_FRAME_EVENTS: 0
GC_MAPSECTS_HOOK_VERIFIED: NO
USER_CAPTURE_READY: NO
FIX_IMPLEMENTED: NO
CURRENT_ROOT_CLASS: GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: OBSERVABILITY_REPAIR
CURRENT_ROOT_BOUNDARY: func_80801174_gcmapsects
LATEST_ARTIFACT: /tmp/p08-widescreen-focal-branch-join-repair-20260827T002831Z/
LATEST_SHA256SUMS_SHA: 0cd05b6bbac330111b9bdd4a8fa86630b0b6dee02f968d9b568722723450b4d1
```

Link proof: `func_80801640_gcmapsects` calls `__wrap_func_80801174_gcmapsects`.
Attract smoke (Wooded Hollow 335, TDL 278) never called ep 9/17. Do not
promote CAA24 from the persistent focal DL. Do not bypass.

`NEXT_SINGLE_ACTION:` Get `[ws-gcmapsects-entry] > 0` on a map that ticks ep 9/17, then one USER_SELECTED capture. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## FOCAL_BRANCH_TIMELINE user arbitration (2026-08-26) — CAPTURE_VALID=YES (1717..2436)

Requested logs: `/tmp/tooie-ws-continuous.log`,
`/tmp/tooie-ws-dl-identity.log`, `/tmp/tooie-ws-focal-branch.log`. Expected
setup `/tmp/p08-widescreen-focal-branch-timeline-setup-20260826T164317Z/`
SHA256SUMS `f010ac8a1a33e97494a0d6d5dcb90a237bc8814399c4a01ba1328f21c9de81e3`
is **missing from /tmp** (`SETUP_ARTIFACT_PRESENT: NO`). Do not mix this
window with prior VALID 1425..2144 or smoke start=245.

```text
USER_TRIGGER_FOUND: YES
CAPTURE_COMPLETE_FOUND: YES
CAPTURE_VALID: YES
CAPTURE_ID: 1
CAPTURE_START_FRAME: 1717
CAPTURE_END_FRAME: 2436
FRAMES_PRESENT: 720 / 720
FOCAL_BRANCH_LOG_PRESENT: YES (0 bytes; truncate-only at trigger)
FOCAL_BRANCH_EVENTS_IN_WINDOW: 0
FOCAL_SECTION_IDENTITY: UNKNOWN
FOCAL_SECTION_IDENTITY_STABLE: UNKNOWN
FOCAL_DL_PHYSICAL: 0x0035dd20
FOCAL_DL_KSEG0: 0x8035dd20
FOCAL_DL_FIRST_SEEN_FRAME: 1720
FOCAL_DL_LAST_SEEN_FRAME: 2432
FOCAL_DL_REAPPEARS: NO
LAST_SEEN_IS_CAPTURE_TAIL: YES
SPHERE_RESULT_BEFORE_ONSET: UNOBSERVED
SPHERE_RESULT_AT_ONSET: UNOBSERVED
CAA24_RESULT_BEFORE_ONSET: UNOBSERVED
CAA24_RESULT_AT_ONSET: UNOBSERVED
COMBINED_RESULT_BEFORE_ONSET: UNOBSERVED
COMBINED_RESULT_AT_ONSET: UNOBSERVED
WALK_BEFORE_ONSET: UNOBSERVED
WALK_AT_ONSET: UNOBSERVED
DL_BEFORE_ONSET: N/A (no absent graphics-emit frame)
DL_AT_ONSET: PRESENT from first emit 1720
FIRST_BRANCH_FLIP: NONE_IN_CAPTURE_WINDOW
FIRST_BRANCH_FLIP_FRAME: NONE
CAA24_CONTROLS_FOCAL_MEMBERSHIP: UNKNOWN
SPHERE_CONTROLS_FOCAL_MEMBERSHIP: UNKNOWN
POST_WALK_SELECTION_INVOLVED: UNKNOWN
CAA24_REJECTING_PLANE: UNKNOWN
D_8012D500_CHANGES_WITH_CAMERA: UNKNOWN
D_8012D500_UPDATE_LAG: UNKNOWN
FOCAL_CLUSTER_TRANSITION: MIXED_ASYMMETRIC_NEIGHBOR_DROPOUT
SECTION_BLOCK_ACTIVATION: NOT_SUPPORTED
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED
GUEST_DL_TO_RSP_CONTINUITY: PROVEN (RSP lag mode=3; 150/179)
CURRENT_ROOT_CLASS: GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY: func_80801174_gcmapsects (800E3E8C ∧ 800E3D0C/CAA24)
FIRST_CAUSAL_DIFF: NOT_OBSERVED_IN_THIS_WINDOW
NEXT_DISCRIMINATOR: FOCAL_BRANCH_JOIN_REPAIR
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-focal-branch-user-arbitration-20260826T233105Z/
LATEST_SHA256SUMS_SHA: 4b1867cb2145af8dc5c33b6916943ce335edf6295a886eaef9d37bddf87a7a65
```

Signature D matches this capture: focal DL membership does not flip
ABSENT→PRESENT. Neighbor `0x8035d268` last emit=1856 while focal and
`0x8035e2d8` continue. `[ws-focal-branch]`/`[ws-focal-sphere]`/`[ws-focal-caa24]`
are absent; `[ws-geo-e]=540` on stack nodes `0x800455b0`/`0x80045618` only, so
the RA-gated 80801174 helper join never armed. Do not promote
`GUEST_MAP_SECTION_CAA24_SELECTION_BOUNDARY`. Do not treat last_seen 2432 as
early-pop-out. Do not implement a CAA24/sphere bypass. CAB70/E3DC0/CACEC/bbox/
sphere visual roots remain `REFUTED_FOR_OBSERVED_SYMPTOM`.

`NEXT_SINGLE_ACTION:` Repair observation of `func_80801174_gcmapsects` (guest-PC-safe helper RA or overlay-safe hook) so `[ws-focal-branch]` emits on a frame where `0x8035dd20` is already guest-emitted; then recapture at pop-in. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## DL_PRODUCER_BRANCH_TRACE (2026-08-26) — focal 0x8035dd20

No new capture. Authoritative window remains `capture_id=1` frames
**1425..2144**. Artifact
`/tmp/p08-widescreen-dl-producer-trace-20260826T150351Z/`
(manifest `40d591c62899a56b60f73f4e990f019b1b1890b22b4e4d6a33b41b6eba48f5ee`;
`sha256sum -c` PASS twice).

```text
FOCAL_DL_PHYSICAL: 0x0035dd20
FOCAL_DL_KSEG0: 0x8035dd20
FOCAL_DL_STORAGE_CLASS: LOADED_MAP_MODEL_PAYLOAD
FOCAL_DL_STATIC_OR_DYNAMIC: DYNAMIC_RELOCATED
FOCAL_DL_SEMANTIC_CLASS: LEAF_DISPLAY_LIST / MAP_MODEL_PAYLOAD
FOCAL_DL_HAS_VTX: UNKNOWN
FOCAL_DL_HAS_TRIANGLES: UNKNOWN
FOCAL_DL_COMMAND_COUNT: UNKNOWN
FOCAL_DL_FIRST_SEEN_FRAME: 1432
FOCAL_DL_LAST_SEEN_FRAME: 2140
FOCAL_DL_REAPPEARS: NO
FOCAL_DL_PARENT_DL: 0x80159600 / 0x80160cd0
FOCAL_DL_PREVIOUS_NEIGHBOR: 0035d268 (dominant)
FOCAL_DL_NEXT_NEIGHBOR: 0035e2d8 (dominant)
FOCAL_DL_EMITTER_FUNCTION: UNJOINED (800DD07C | 800E2180 | 800E2630 | 800E32CC)
FOCAL_DL_EMITTER_PC: UNJOINED
FOCAL_DL_PRODUCER_CHAIN: gcmapsects_entrypoint_9/17 -> 80801640 -> 80801174 (800E3E8C ∧ 800E3D0C/CAA24) -> geo/model walk -> G_DL store
FOCAL_DL_FROM_GCMAPSECTS: UNKNOWN (pointer) / YES (class selector)
FOCAL_DL_FROM_GCCUBEDLL: NO
FOCAL_DL_FROM_GCSECTIONDLL: NO
FOCAL_DL_CONTROLLED_BY_GCSECTIONSKIP: UNKNOWN
FOCAL_DL_REACHES_800CAA24: YES (static section path)
FOCAL_DL_LOD_CONTROLLED: NO
FOCAL_GEO_COMMAND_CHAIN: 80801174 -> 800E3E8C + 800E3D0C/CAA24; model tree GeoCmdD 800E3DC0 / GeoCmdE 800E3E8C
FOCAL_DL_MAP_TERRAIN_BIND: UNKNOWN
FIRST_BRANCH_CONTROLLING_8035DD20_MEMBERSHIP: func_80801174_gcmapsects AND (800E3E8C sphere, 800E3D0C/CAA24 AABB)
FIRST_BRANCH_FUNCTION: func_80801174_gcmapsects
FIRST_BRANCH_PC: 0x808011CC ; 0x808011F8
FIRST_BRANCH_INPUTS: section+0x24/+0x30 sphere; section+0x0C/+0x18 AABB vs D_8012D500
DYNAMIC_BRANCH_JOIN: NO
MAP_TERRAIN_DLS_IDENTIFIED: NO
GUEST_DL_TO_RSP_CONTINUITY: PROVEN
GUEST_SELECTION_FOR_PERSISTENT_DLS: REFUTED
LAST_IDENTITY_PRESERVING_BOUNDARY: RSP_HLE_G_DL_VISIT
IDENTITY_LOST_AT: RSP_G_DL_TO_RT64_DRAW
DL_TO_RT64_DRAW_JOIN: NO
CURRENT_ROOT_CLASS: GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS
CURRENT_ROOT_BOUNDARY: func_80801174_gcmapsects for nested leaf 0x8035dd20
PRIOR_ROOT_CLASS: GUEST_DISPLAY_LIST_SELECTION_OR_EMISSION_BOUNDARY (REFINED, not REFUTED)
DOWNSTREAM_RSP_RT64_BOUNDARY: OPEN / candidate #2
FIX_IMPLEMENTED: NO
```

All five TOP_TRANSITION DLs (`003557e8`, `0035dd20`, `00349418`, `0035e2d8`,
`0035e8a8`) are RSP leaves (0 nested children). Focal is not substituted.
`func_800E37F8_IS_LOD` remains `REFUTED`. CAB70/E3DC0/CACEC/bbox/sphere visual
roots remain `REFUTED_FOR_OBSERVED_SYMPTOM`. Do not implement CAA24 bypass.
Static producer identification is **PROVEN as machinery**. Visual root is
**NOT PROVEN**.

`NEXT_DISCRIMINATOR:` `FOCAL_BRANCH_TIMELINE`
`NEXT_SINGLE_ACTION:` Instrument `func_80801174_gcmapsects` + `func_800E3D0C`/`func_800CAA24` (section index, AABB, v0) on the same guest frames as G_DL `0x8035dd20` membership. No bypass. No fix.
`NEXT_RECOMMENDED_MODEL:` Gemini 3.7 Flash High.

## Frame-joined USER_SELECTED arbitration (2026-08-26) — CAPTURE_VALID=YES (1425..2144)

Requested logs: `/tmp/tooie-ws-continuous.log` and
`/tmp/tooie-ws-dl-identity.log`. Setup
`/tmp/p08-widescreen-dl-framejoin-fix-20260826T130423Z/` SHA256SUMS
`4c650961c173fb7ad7a924fbaaf90abe1393cc3a9d1a53adfe41807d7442cb32`
revalidated PASS twice. Do not mix this window with the earlier smoke
(id=1 start=245).

```text
USER_TRIGGER_FOUND: YES
CAPTURE_COMPLETE_FOUND: YES
CAPTURE_VALID: YES
CAPTURE_ID: 1
CAPTURE_START_GUEST_FRAME: 1425
CAPTURE_END_GUEST_FRAME: 2144
START_MONOTONIC_NS: 161168261589779
END_MONOTONIC_NS: 161177212222325
FRAMES_PRESENT: 720 / 720
ROOT_COMMAND_BUFFERS: 0x80159600, 0x80160cd0
ROOT_COMMAND_BUFFER_IDENTITY: DOUBLE_BUFFER
USER_SELECTED_WIDESCREEN_VISIBILITY_ERROR: LIVE_CAPTURED
NESTED_DL_UNIQUE_COUNT: 65
GUEST_NESTED_DL_SET_BEHAVIOR: CHANGING
PERSISTENT_NESTED_DL_COUNT: 49
TRANSIENT_NESTED_DL_COUNT: 6
NEW_NESTED_DLS_AFTER_TRIGGER: 22
REMOVED_NESTED_DLS_AFTER_TRIGGER: 9
MAP_TERRAIN_DLS_IDENTIFIED: NO
DL_TRANSITIONS_CORRELATE_WITH_VISUAL_WINDOW: PARTIAL
DL_EMIT_WITH_RSP_MATCH: 9471
DL_EMIT_WITHOUT_RSP_MATCH: 24
RSP_WITHOUT_EMIT_MATCH: 9278
GUEST_DL_TO_RSP_CONTINUITY: PROVEN
GUEST_DL_ABSENCE_EVIDENCE: PARTIAL
GUEST_SELECTION_FOR_PERSISTENT_DLS: REFUTED
RSP_RECEIVES_PERSISTENT_GEOMETRY_EVIDENCE: PROVEN
RSP_TO_RT64_WORKLOAD_JOIN: PARTIAL
DL_TO_RT64_DRAW_JOIN: NO
LAST_IDENTITY_PRESERVING_BOUNDARY: RSP_HLE_G_DL_VISIT
IDENTITY_LOST_AT: RSP_G_DL_TO_RT64_DRAW
RT64_IS_RENDERING_EXTRA_BAND_GEOMETRY: YES
CURRENT_ROOT_CLASS: GUEST_DISPLAY_LIST_SELECTION_OR_EMISSION_BOUNDARY
CURRENT_ROOT_CLASS_STATUS: STRONG_HYPOTHESIS (later REFINED to GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY)
CURRENT_ROOT_BOUNDARY: nested depth-1 G_DL membership (focal 0035dd20)
GUEST_DISPLAY_LIST_SELECTION_OR_EMISSION_BOUNDARY: STRONG_HYPOTHESIS
DOWNSTREAM_RSP_RT64_BOUNDARY: OPEN / NOT_PROMOTED
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-dl-framejoined-user-arbitration-20260826T143436Z/
LATEST_SHA256SUMS_SHA: f0f9b493cf4bdac6ebe2378f8ce5b04ce09f8f4eab34058cd58ff7fed6d80f54
```

Guest emit walks only the GraphicsTask root for `G_DL` (depth 1). RSP also
walks depth 2 (7 extra `0011xxxx` DLs). Root ping-pong is double-buffer, not
geometry. Consecutive nested Jaccard mean 0.997, 0 intermittent DLs. The 22
late first_seen and 9 last_seen sit in phys `0x0034xxxx`/`0x0035xxxx` (no
static map/terrain bind). Persistent 49 nested DLs are emitted and
RSP-received throughout. Extra-band triangles exist but cannot be joined to a
DL. Do not reopen CAB70/E3DC0/CACEC.

`NEXT_DISCRIMINATOR:` `DL_PRODUCER_BRANCH_TRACE` — **DONE** this round; see section above. Current next is `FOCAL_BRANCH_TIMELINE`.

## Prior frame-joined attempt (2026-08-26) — CAPTURE_VALID=NO

Requested logs: `/tmp/tooie-ws-continuous.log` (missing) and
`/tmp/tooie-ws-dl-identity.log` (present). Setup
`/tmp/p08-widescreen-dl-framejoin-fix-20260826T130423Z/` SHA256SUMS
`4c650961c173fb7ad7a924fbaaf90abe1393cc3a9d1a53adfe41807d7442cb32`
revalidated PASS twice.

```text
USER_TRIGGER_FOUND: YES
CAPTURE_COMPLETE_FOUND: NO
CAPTURE_VALID: NO
CAPTURE_ID: 1
CAPTURE_START_GUEST_FRAME: 245
CAPTURE_END_GUEST_FRAME: 892
START_MONOTONIC_NS: 156279583371067
END_MONOTONIC_NS: NONE
FRAMES_PRESENT: 648 / 720
ROOT_COMMAND_BUFFERS: 0x80159600, 0x80160cd0
ROOT_COMMAND_BUFFER_IDENTITY: DOUBLE_BUFFER
WINDOW_CLASS: SMOKE_OR_INCOMPLETE_NOT_USER_SELECTED
GUEST_NESTED_DL_SET_BEHAVIOR: NOT_ANALYZED
MAP_TERRAIN_DLS_IDENTIFIED: NOT_ANALYZED
DL_TRANSITIONS_CORRELATE_WITH_VISUAL_WINDOW: UNKNOWN
GUEST_DL_TO_RSP_CONTINUITY: NOT_PROMOTED (smoke join remains VERIFIED)
GUEST_DL_ABSENCE_EVIDENCE: NO
GUEST_SELECTION_FOR_PERSISTENT_DLS: UNKNOWN
RSP_TO_RT64_WORKLOAD_JOIN: PARTIAL (instrumentation; not user-window proof)
DL_TO_RT64_DRAW_JOIN: NO
LAST_IDENTITY_PRESERVING_BOUNDARY: NOT_PROMOTED
IDENTITY_LOST_AT: NOT_PROMOTED
RT64_IS_RENDERING_EXTRA_BAND_GEOMETRY: UNKNOWN_FOR_USER_WINDOW
CURRENT_ROOT_CLASS: UNKNOWN / RE-ANCHOR_REQUIRED
CURRENT_ROOT_CLASS_STATUS: USER_SELECTED_CAPTURE_INVALID
CURRENT_ROOT_BOUNDARY: NOT_PROMOTED
GUEST_DISPLAY_LIST_SELECTION_OR_EMISSION_BOUNDARY: NOT_PROMOTED
DOWNSTREAM_RSP_RT64_BOUNDARY: NOT_PROMOTED
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-dl-framejoined-user-arbitration-20260826T133356Z/
LATEST_SHA256SUMS_SHA: f6069a153521b66c59a16b1237c3f76f0039449e0962b343c7e377d5616ca0e7
```

Invalid because: requested continuous log missing; no
`[ws-capture-complete]`; 648/720 frames; capture_id=1/start=245 is the
documented smoke (`DL_EMIT=4779`, `DL_RSP=8908`); leftover
`/tmp/tooie-ws-continuous-active`. Do not confuse smoke with user evidence.
Do not reopen CAB70/E3DC0/CACEC.

`NEXT_DISCRIMINATOR:` `USER_SELECTED_COMPLETE_DL_IDENTITY_CAPTURE`
`NEXT_SINGLE_ACTION:` complete USER_SELECTED 720-frame capture, then re-run A vs B.
`NEXT_RECOMMENDED_MODEL:` Grok 4.6 Medium.

## DL-identity arbitration (2026-08-26)

User trigger `touch /tmp/tooie-ws-capture` while the residual was visible.
Log `/tmp/tooie-ws-continuous.log`.

```text
USER_TRIGGER_FOUND: YES
CAPTURE_COMPLETE_FOUND: YES
CAPTURE_VALID: YES
CAPTURE_START_FRAME: 1481
CAPTURE_END_FRAME: 2200
USER_SELECTED_WIDESCREEN_VISIBILITY_ERROR: LIVE_CAPTURED
GUEST_WS_DL_EMIT_COUNT: 0
RSP_LINES_INSIDE_TRIGGER_COMPLETE: 0
DL_IDENTITY_JOINED_TO_VISUAL_WINDOW: NO
ROOT_DL_UNIQUE_COUNT (unjoined dump): 2 (00160cd0, 00159600 ping-pong)
NESTED_DL_UNIQUE_COUNT (unjoined dump): 62
ROOT_DL_SET_STABILITY (visual window): UNKNOWN
ROOT_DL_SET_STABILITY (unjoined dump): STABLE
NESTED_DL_SET_STABILITY (visual window): UNKNOWN
NESTED_DL_SET_STABILITY (unjoined dump): MOSTLY_STABLE (intro fingerprint)
DL_TRANSITIONS_CORRELATE_WITH_VISUAL_WINDOW: UNKNOWN
GUEST_DL_ABSENCE_EVIDENCE: NO
RSP_RECEIVES_PERSISTENT_GEOMETRY_EVIDENCE: PARTIAL (unjoined intro dump only)
LAST_IDENTITY_PRESERVING_BOUNDARY: NONE_IN_USER_SELECTED_WINDOW
IDENTITY_LOST_AT: GUEST_EMIT_ABSENT + guest_frame=UNKNOWN + dual-fd log
GUEST_DISPLAY_LIST_SELECTION_OR_EMISSION_BOUNDARY: NOT_PROMOTED
DOWNSTREAM_RSP_RT64_BOUNDARY: NOT_PROMOTED
CURRENT_ROOT_CLASS: UNKNOWN / RE-ANCHOR_REQUIRED
CURRENT_ROOT_CLASS_STATUS: DL_IDENTITY_CAPTURE_INCONCLUSIVE_FOR_VISUAL_WINDOW
CURRENT_ROOT_BOUNDARY: NOT_PROMOTED
NEXT_DISCRIMINATOR: GUEST_FRAME_JOINED_DISPLAY_LIST_IDENTITY
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-dl-identity-arbitration-20260826T072436Z/
LATEST_SHA256SUMS_SHA: f4b2c20086ced79a3d728174464a2dc673a6fff201d0c7edfe1f014739ddccff
```

The 1481..2200 window has moving camera and extra-band `[ws-rt64-draw]`
(extraLeft=201896 extraRight=106295) so aggregate geometry still reaches the
widescreen bands. `[ws-dl-rsp]` / `[ws-dl-rt64]` exist only after
`CAPTURE_COMPLETE`, clocks 1..169, and nested first/last ordinals match the
start_frame=1 smoke intro timeline. Do not treat that dump as the visual
window. Do not reopen CAB70/E3DC0/CACEC.

## Post-culling re-anchor (2026-08-26)

User authoritative result: `TOOIE_DIAG_CACEC_BYPASS=1` on the same
intro/demos as the residual → **`NO_CHANGE`**.

```text
USER_MANUAL_CAB70_BYPASS_VISUAL_RESULT:  NO_CHANGE
USER_MANUAL_E3DC0_BYPASS_VISUAL_RESULT:  NO_CHANGE
USER_MANUAL_CACEC_BYPASS_VISUAL_RESULT:  NO_CHANGE
CAB70_DIRECT_VISUAL_ROOT:                REFUTED_FOR_OBSERVED_SYMPTOM
E3DC0_DIRECT_VISUAL_ROOT:                REFUTED_FOR_OBSERVED_SYMPTOM
CACEC_DIRECT_VISUAL_ROOT:                REFUTED_FOR_OBSERVED_SYMPTOM
SPHERE_CULLING_EXISTS:                   PROVEN
SPHERE_CULLING_CHANGES_VISIBILITY:       PROVEN
SPHERE_CULLING_IS_VISUAL_ROOT:           REFUTED_FOR_OBSERVED_SYMPTOM
BBOX/SPHERE AS PRIMARY VISUAL ROOT:      REFUTED BY COUNTERFACTUALS
CURRENT_ROOT_CLASS:                      UNKNOWN / RE-ANCHOR_REQUIRED
CURRENT_ROOT_BOUNDARY:                   NOT_PROMOTED
GROUP_OR_CHUNK_VISIBILITY_THRESHOLD:     HISTORICAL_STRONG_HYPOTHESIS / NOT_SUPPORTED_BY_VISUAL_BYPASSES
func_800E37F8_IS_LOD:                    REFUTED (integer /a2 scale loop)
func_800E35E0_IS_GEO_WALKER:             REFUTED (half-size + 800E32CC setup)
LOD_PATH:                                UNKNOWN / TOOIE_GEOCMD8_NOT_IDENTIFIED
LOD_RELEVANCE:                           MEDIUM
BANJO_LOD_PATCH_RELEVANCE:               MEDIUM (do not copy)
ROOM_SECTION_PATH:                       gcmapDll -> gcmapsects/gccubeDll -> 800E3D0C/800E3E8C
ROOM_SECTION_RELEVANCE:                  HIGH
SCREEN_SPACE_PREDICATES:                 OPEN (func_800C4C34 / 800E4140)
CAMERA_ANGLE_PREDICATES:                 WEAK / NOT_FOUND as whole-mesh cull
DISPLAY_LIST_EMISSION_BOUNDARY:          PARTIAL (no DL address)
CAN_WE_PROVE_GUEST_GEOMETRY_ABSENCE:     PARTIAL
CAN_WE_PROVE_RT64_RECEIVES_FOCAL_GEOMETRY: NO
NEXT_DISCRIMINATOR:                      GUEST_DISPLAY_LIST_IDENTITY_CAPTURE
FIX_IMPLEMENTED:                         NO
LATEST_ARTIFACT:                         /tmp/p08-widescreen-post-culling-reanchor-20260826T061310Z/
LATEST_SHA256SUMS_SHA:                   e4d8b21f80ddaa3b7557a9ec50282f6fc82b8026d7a1f1c96058b83030f16600
```

Do not reopen CAB70 / E3DC0 / CACEC as visual roots without new contradictory
evidence. Remaining guest skip paths that those three env vars do **not**
force include `func_800CAA24` / `func_800E3D0C` (float AABB), `gcmapsects`,
`gccubeDll`, `gcsectionskip`, `func_800C4C34`, and an unidentified LOD analog.
Alternatively the mesh is submitted and lost downstream (Banjo
`gEXSetNearClipping FALSE` is the first RT64 analog). The next field is
guest display-list address present vs absent during the user-selected error.

## CACEC Bypass Counterfactual Setup (2026-08-26)

```text
HEAD_TOOIE:                          ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64:                           6f1c2d99a4ea571c139f449c326fd176ba8f3496
BUILD_RESULT:                        PASS (ninja TooieRecompiled exit code 0)
CACEC_BYPASS_IMPLEMENTED:            YES
CACEC_VISIBLE_RETURN:                1 (0x1)
CACEC_CULLED_RETURN:                 0 (0x0)
CACEC_HAS_SIDE_EFFECTS:              NO (pure functional geometric test in funcs_205.c:1671)
DEFAULT_RUNTIME_BEHAVIOR_CHANGED:    NO
BASELINE_FORCED_COUNT:               0
BYPASS_TOTAL_CALLS:                  280
BYPASS_ORIGINAL_VISIBLE:             150
BYPASS_ORIGINAL_CULLED:              130
BYPASS_FORCED_VISIBLE:               130 (100% forced to VISIBLE)
BYPASS_SCOPE:                        ALL_CACEC_CONSUMERS
VISUAL_RESULT:                       NO_CHANGE (USER_MANUAL; CACEC_DIRECT_VISUAL_ROOT REFUTED_FOR_OBSERVED_SYMPTOM)
FIX_IMPLEMENTED:                     NO
TEMP_DIAGNOSTIC_CHANGE_IMPLEMENTED:  YES
LATEST_ARTIFACT:                     /tmp/p08-widescreen-cacec-counterfactual-setup-20260826T054500Z/
LATEST_SHA256SUMS_SHA:               88a9fb826f14e53cedb15a36bfb1cd9454a19d6a6b4c37256573cf6a00d2ff37
```

### Purpose & Discriminator:
- Previous tests with `TOOIE_DIAG_CAB70_BYPASS=1` and `TOOIE_DIAG_E3DC0_BYPASS=1` yielded `USER_MANUAL_RESULT: NO_CHANGE`.
- However, those tests did **not** bypass `func_800CACEC` (Sphere Culling), which is used by `GeoCmdE` (`func_800E3E8C`), top-level model tests in `modelRender_draw`, and other candidate sphere consumers.
- `TOOIE_DIAG_CACEC_BYPASS=1` forces `func_800CACEC` to return `1` (VISIBLE) globally, allowing the user to visually test if sphere culling is a direct visual root or contributor to the widescreen residual pop-in/pop-out.
- When `TOOIE_DIAG_CACEC_BYPASS` is unset, default runtime behavior remains 100% untouched.

`USER_TEST_COMMAND:`
```sh
# 1. Baseline Test (Default):
cd /home/wesley/projetos/N64-Recomp/TooieRecomp
./build/TooieRecompiled
# Observe intro/demos where widescreen edge pop-in/pop-out occurs.

# 2. CACEC Bypass Test:
TOOIE_DIAG_CACEC_BYPASS=1 ./build/TooieRecompiled 2>/tmp/tooie-cacec-bypass.log
# Observe the EXACT SAME intro/demos and compare.
```

`NEXT_SINGLE_ACTION:` User manually compares baseline vs `TOOIE_DIAG_CACEC_BYPASS=1` in the same intro/demos and reports A/B/C/D.
`NEXT_RECOMMENDED_MODEL:` GPT-5.6 Luna High only if the visual result conflicts with telemetry; otherwise Gemini/Grok according to result.

## Continuous capture arbitration (2026-08-26)

`USER_OBSERVED_WIDESCREEN_MORPHOLOGY:`

- late pop-in exists (`USER_OBSERVED`);
- early pop-out also exists in multiple gameplay demos (`USER_OBSERVED`);
- earlier "no early pop-out observed" / `LEFT_SIDE_EARLY_DISAPPEARANCE:NOT_OBSERVED` is **CORRECTED/SUPERSEDED**;
- current capture is a user-selected widescreen visibility error with an automatic 720-frame window (primarily a late-reveal interval plus whatever else occurred in those 9 s);
- early-popout causality remains unjoined (`EARLY_POP_OUT_CAUSAL_CHAIN: NOT_PROVEN`).

```text
USER_TRIGGER_FOUND:              YES
CAPTURE_COMPLETE_FOUND:          YES
CAPTURE_VALID:                   YES
CAPTURE_START_FRAME:             1481
CAPTURE_END_FRAME:               2200
CAPTURE_DURATION_FRAMES:         720
CAPTURE_DURATION_TIME:           9050 ms (9.050 s)
USER_SELECTED_EVENT:             WIDESCREEN_VISIBILITY_ERROR
R73_PRESENT_IN_CAPTURE:          NO
R73_TRANSITIONS:                 NONE
TOP_CANDIDATE_TYPE:              SPHERE
TOP_CANDIDATE_IDENTITY:          pos=(1568.6,106.6,-2400.0) r=469.7 call=2
TOP_CANDIDATE_PARENT:            GeoCmdE node 8012ce98
LAST_ORIG_CULLED_WIDE_VISIBLE:   n/a (wide also 0)
LAST_ORIG_CULLED_FRAME:          1891 orig=0 wide=0 child SKIPPED
FIRST_ORIG_VISIBLE_FRAME:        1895 orig=1 wide=1 child EXECUTED
VISIBILITY_THRESHOLD_CROSSING:   1891 -> 1895
FAILING_PLANE_AT_CROSSING:       BOTTOM (p3); margin -2.65 -> +2.08
LATE_POP_IN_SIGNATURE_FOUND:     NO
EARLY_POP_OUT_SIGNATURE_FOUND:   NO
TOTAL_SPHERE_ORIG_WIDE_DISAGREEMENTS: 0
TOTAL_BBOX_ORIG_WIDE_DISAGREEMENTS:   147 (all orig=1/wide=0 LEFT)
VISIBILITY_THRESHOLD_ASPECT_MISMATCH: NOT_PROVEN_FOR_CAPTURED_EVENT
CURRENT_ROOT_CLASS:              GROUP_OR_CHUNK_VISIBILITY_THRESHOLD
CURRENT_ROOT_CLASS_STATUS:       STRONG_HYPOTHESIS / NOT_YET_PROVEN_FOR_WIDE_BAND
CURRENT_ROOT_BOUNDARY:           func_800CACEC / GeoCmdE (best captured crossing; NOT promoted)
FIX_IMPLEMENTED:                 NO
LATEST_ARTIFACT:                 /tmp/p08-widescreen-continuous-arbitration-20260826T052031Z/
LATEST_SHA256SUMS_SHA:           fac7940f3b7d27d2c35d1d861bb8654866dbf0291b3014190addb0072939aeb8
```

Promotion required orig=0 **and** wide=1 on the same node while child skipped, then orig=1/wide=1 with child executed. That field never occurs in this capture (spheres all orig==wide; no bbox orig=0/wide=1). The r=469.7 crossing is a guest visibility threshold, but it is a **BOTTOM-plane** near-miss while the camera descends, so it is **not** extra-band 4:3-vs-16:9. Compatible with prior CAB70/E3DC0 visual `NO_CHANGE` (those bypasses do not force CACEC).

`GUEST_RT64_FRAME_CLOCK_JOIN: YES` (validation note) is **CORRECTED/SUPERSEDED** here: RT64 logs `p.submissionFrame` 845..1024, not guest 1481..2200. Ordinal 180-tick join only. Draw totals 700 / 3892 triangles on both sides of the crossing.

Wide-plane validation identity remains **PROVEN** (revalidated artifact `1e9492a95b47779d01a465b80d6fb18cbbfa217a3655de5ee116278f6c952b55`). `world_nx *= 0.75` remains **CORRECTED/SUPERSEDED**.

```text
CURRENT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
WORKTREE:                     DIRTY (preserve diagnostic wraps + docs)
CURRENT_FOCUS:                WIDESCREEN_EDGE_GEOMETRY
FIX_IMPLEMENTED:              NO
BUILD_THIS_ROUND:             NOT_RUN
COMMIT_PUSH:                  NO
```

## Bidirectional late-reveal snapshots (2026-08-26)

User morphology (authoritative, `USER_OBSERVED`):

```text
LATE_POP_IN:                         USER_OBSERVED
EARLY_POP_OUT:                       USER_OBSERVED
BIDIRECTIONAL_EDGE_VISIBILITY_ERROR: USER_OBSERVED
LEFT_SIDE_EARLY_DISAPPEARANCE:       CORRECTED/SUPERSEDED
  (prior NOT_OBSERVED claim is no longer current)
EARLY_POP_OUT_CAUSAL_CHAIN:          NOT_PROVEN
```

The existing `/tmp/tooie-ws-group-user.log` triggers are **not**
visible-center / missing-edge. They are:

```text
touch /tmp/tooie-ws-group-center -> USER_SELECTED_EARLY_MISSING_STATE
  [ws-trigger] CENTER trigger activated at frame 1497
touch /tmp/tooie-ws-group-edge   -> USER_SELECTED_LATER_PARTIAL_REVEAL_STATE
  [ws-trigger] EDGE trigger activated at frame 2149
FRAME_DELTA: 652 guest full-fb scissors (~10.9 s at 60 Hz; user recalled ~2 s)
```

```text
EARLY_TRIGGER_FOUND: YES
LATE_TRIGGER_FOUND:  YES
TEMPORAL_JOIN:       PASS
CONTINUOUS_WINDOW:   FAIL
EARLY_GEO_SNAPSHOT:  guest frame 1499
LATE_GEO_SNAPSHOT:   guest frame 2151
```

Instrumentation only keeps `s_ws_group_capture_mode` for a 5-tick countdown
that is decremented on the full-framebuffer scissor, and it clears the mode
on that scissor before later geo of the last tick. Result: **one geo frame
per trigger**, not EARLY-5..LATE+5. RT64 draws (692) are all tagged
`[ws-draw-center]` at RT64 frame 2641; **`[ws-draw-edge]` count = 0**. Guest
and RT64 frame clocks are unjoined.

### Same-identity orig_vis flips (LIVE_CAPTURED)

| ID | Predicate | Early | Late |
| :--- | :--- | :--- | :--- |
| SPHERE `(1568.6,106.6,-2400.0)` r=73 | `func_800CACEC` / GeoCmdE `8012ce98` | orig=0 | orig=1 |
| NODE `8037beb2` | `func_800CAB70` / GeoCmdD | orig=0 | orig=1 |
| NODE `8037c2c2` | `func_800CAB70` / GeoCmdD | absent | orig=1 |

The r=73 sphere keeps an identical world position; that is the cleanest
late-reveal identity. `8012ce98` is a weak geo identity (shared cmd pointer
for three CACEC calls). `func_800CAD9C` was never called (`POINT: NO`).

### Why this is not a promotion

Promotion required: same node, actual CULLED, **true** hypothetical-wide
VISIBLE, draw absent, then orig_vis 1 with draw present. Failures:

1. Diagnostic `wide_vis` is **UNRELIABLE**:
   - `tooie_is_guest()` requires 4-byte alignment, so CAB70 `max` logs as
     `(0,0,0)` (max is typically `min+6`).
   - Live CAB70/CACEC planes start at `viewport+0x24` with stride **0x20**;
     the diagnostic used packed float4 / CACEC `+0x18`.
2. No continuous CULLED→VISIBLE frame index (652-frame jump).
3. No EDGE draw join.

`CAB70_CAUSES_GUEST_4_3_FRUSTUM_REJECTION` remains **PROVEN** (telemetry).
`CAB70_DIRECT_VISUAL_ROOT` remains **REFUTED_FOR_PREVIOUS_USER_TEST**.
The r=73 flip is a CACEC path, which those bypasses would not restore.
That is an acceptable remaining explanation, not a unique root.

`CANDIDATE_EXPLAINS_LATE_POP_IN:` **PARTIAL**
`CANDIDATE_EXPLAINS_EARLY_POP_OUT:` **POSSIBLE** (same p-vertex 4:3 test
on left and right planes; `LEFT_RIGHT_FRUSTUM_SYMMETRY` **PROVEN** as
algorithm, **UNKNOWN** for live coefficients). No exit-side capture.

`FRUSTUM_ASPECT_MISMATCH:` **PARTIAL**
`FRUSTUM_UPDATE_LAG:` **UNKNOWN** (no camera/plane dump)
`NEXT_DIAGNOSTIC:` `SINGLE_TRIGGER_CONTINUOUS_WINDOW`

Artifact: `/tmp/p08-widescreen-bidirectional-visibility-20260826T044851Z/`
(`SHA256SUMS` SHA `f1b8621a90ef6ea2f25613313732dfe437c55e46fb3b324582b1f199c7f0c6f4`,
dual `sha256sum -c` PASS).

## User-selected CENTER/EDGE visual join (2026-08-26) — FAIL

Task: narrow causal arbitration of the same visual region CENTER vs EDGE
using the user-selected capture. Automated 041000Z CENTER/EDGE is **not**
visual authority.

```text
CLAIMED_USER_LOG:             /tmp/tooie-ws-group-user.log
USER_LOG_PRESENT:             NO
CENTER_TRIGGER_FOUND:         NO
EDGE_TRIGGER_FOUND:           NO
VISUAL_FRAME_JOIN:            FAIL
SAME_VISUAL_REGION:           USER_SELECTED (claimed; not captured)
SAME_NODE_CENTER_EDGE_JOIN:   UNKNOWN
FOCAL_NODE_ID:                UNKNOWN
FOCAL_PARENT_ID:              UNKNOWN
FOCAL_DISPLAY_LIST:           UNKNOWN
CENTER_NODE_VISIBILITY:       UNKNOWN
EDGE_NODE_VISIBILITY:         UNKNOWN
CENTER_DRAW_PRESENT:          UNKNOWN
EDGE_DRAW_PRESENT:            UNKNOWN
FIRST_CHANGED_PREDICATE:      UNKNOWN
FIRST_CHANGED_PREDICATE_TYPE: UNKNOWN
GUEST_4_3_VISIBILITY:         UNKNOWN
HYPOTHETICAL_WIDE_VISIBILITY: UNKNOWN
FOCAL_NODE_REACHED_CAB70_DIAGNOSTIC: UNKNOWN
SPHERE_CULLING_INVOLVED:      UNKNOWN
POINT_CULLING_INVOLVED:       UNKNOWN
LOD_INVOLVED:                 UNKNOWN
GROUP_OR_CHUNK_VISIBILITY_CULLING_BOUNDARY: STRONG_HYPOTHESIS / NOT_YET_PROVEN_FOR_OBSERVED_SYMPTOM
AUTOMATED_CENTER_EDGE_PAIR:   NOT_VALID_FOR_VISUAL_CAUSAL_PROMOTION
AUTOMATED_CENTER_EDGE_VISUAL_JOIN: INVALID_FOR_CAUSAL_PROMOTION
CURRENT_ROOT_CLASS:           GROUP_OR_CHUNK_VISIBILITY_CULLING_BOUNDARY
CURRENT_ROOT_CLASS_STATUS:    STRONG_HYPOTHESIS / NOT_YET_PROVEN_FOR_OBSERVED_SYMPTOM
CURRENT_ROOT_BOUNDARY:        Model/Geo-Command Tree (structural)
FIRST_CENTER_EDGE_CAUSAL_DIFF: NOT_PROVEN
FIX_IMPLEMENTED:              NO
LATEST_ARTIFACT:              /tmp/p08-widescreen-user-group-join-20260826T042031Z/
LATEST_SHA256SUMS_SHA:        3ccaaab69f9a395b0b42ace087a0fc9c1d1bb745eeacc91d1148c294d236bfe7
```

Disk probe at arbitration: claimed user log, `/tmp/tooie-ws-group-center`,
and `/tmp/tooie-ws-group-edge` were **ABSENT**. No `[ws-trigger] CENTER` /
`[ws-trigger] EDGE` lines from a user-selected run exist. Causal promotion
stopped (instruction: if either trigger is missing, `VISUAL_FRAME_JOIN: FAIL`).

The previous 041000Z artifact remains a **`REFERENCE_ARTIFACT / STRUCTURAL_CAPTURE`**.
Its CENTER frame 333 / EDGE frame 573 pair was sleep-timed, not
`USER_SELECTED`. Do not use those frames to prove the visual root.

What remains **PROVEN** (structure / prior live telemetry, not this visual join):

- hierarchical geo-command nodes;
- GeoCmdD / `func_800E3DC0` uses `func_800CAB70` BBOX culling;
- GeoCmdE / `func_800E3E8C` uses `func_800CACEC` sphere culling;
- `modelRender` has other coarse visibility paths;
- large chunks can pass and project into Expand extra bands;
- smaller/lateral chunks can be guest-rejected;
- RT64 wide projection, host viewport, host scissor; some geometry enters extra bands.

What is **NOT_YET_PROVEN_FOR_OBSERVED_SYMPTOM**: which of those paths is the
geometry the user actually saw disappear.

Previous user tests:

```text
TOOIE_DIAG_CAB70_BYPASS=1 -> NO_CHANGE  => CAB70_DIRECT_VISUAL_ROOT REFUTED_FOR_PREVIOUS_USER_TEST
TOOIE_DIAG_E3DC0_BYPASS=1 -> NO_CHANGE  => E3DC0_DIRECT_VISUAL_ROOT REFUTED_FOR_PREVIOUS_USER_TEST
```

Acceptable remaining theories (none chosen without evidence): focal geometry
uses GeoCmdE/sphere; parent cull before GeoCmdD; earlier room/chunk visibility;
later child predicate; previously captured nodes were not the focal geometry.

## Causal re-anchor (2026-08-26) — user visual NO_CHANGE

User-manual Expand observations have priority over prior automated
"VISUAL RESOLVED" / class-C visual-root claims.

```text
USER_MANUAL_CAB70_BYPASS_VISUAL_RESULT:  NO_CHANGE
USER_MANUAL_E3DC0_BYPASS_VISUAL_RESULT:  NO_CHANGE
TOOIE_DIAG_CAB70_BYPASS=1 ./build/TooieRecompiled
TOOIE_DIAG_E3DC0_BYPASS=1 ./build/TooieRecompiled
```

### Epistemic corrections (history preserved)

| CLAIM | PRIOR | NOW |
| :--- | :--- | :--- |
| `CAB70_CAUSES_GUEST_4_3_FRUSTUM_REJECTION` | PROVEN | **PROVEN** (unchanged telemetry) |
| `CAB70_BYPASS_RESTORES_BBOX_VISIBILITY_PATH` | PROVEN | **PROVEN** (telemetry / dispatch only) |
| `E3DC0_INHERITS_CAB70` | PROVEN | **PROVEN** |
| `CAB70_CAUSES_OBSERVED_EDGE_POLYGON_GLITCH` | PROVEN (as `CAB70_CAUSES_LATERAL_MISSING_GEOMETRY`) | **REFUTED_FOR_OBSERVED_SYMPTOM** |
| `E3DC0_CAUSES_OBSERVED_EDGE_POLYGON_GLITCH` | implied via inherit | **REFUTED_FOR_OBSERVED_SYMPTOM** |
| `LATERAL_VISUAL_DEFECT_WITH_CAB70_BYPASS` | RESOLVED | **CORRECTED/SUPERSEDED** → `PRESENT / NO_CHANGE` |
| `LATERAL_VISUAL_DEFECT_WITH_E3DC0_BYPASS` | RESOLVED | **CORRECTED/SUPERSEDED** → `PRESENT / NO_CHANGE` |
| Class C as current visual root | PROVEN | **CORRECTED/SUPERSEDED** |
| Class D as unique clip producer | earlier header | still the Expand *trigger*; not the unique clip producer |
| Permanent CAB70 patch as next action | specified | **CORRECTED/SUPERSEDED** — do not implement as visual fix |

Any theory that requires `CAB70 returns 0` or `E3DC0 returns 0` as the
essential cause of the *observed* glitch is **REFUTED_FOR_OBSERVED_SYMPTOM**,
because forcing both predicates visible did not change the picture.

### USER_OBSERVED_SYMPTOM (not a root proof)

- occurs in widescreen Expand;
- center of the image looks correct;
- parts/polygons near the left/right screen edges fail to appear;
- more evident as geometry enters the extra widescreen bands;
- disabling bbox/model frustum rejection does not change the glitch perceptibly.

Do **not** promote as fact: near clipping, viewport clipping, projection bug,
RT64 bug, sphere culling, point culling.

### Morphology (priority only, not proof)

| PATTERN | TYPICAL CAUSE | FIT TO USER REPORT |
| :--- | :--- | :--- |
| WHOLE_OBJECT_DISAPPEARANCE | CPU bbox/sphere/point; geo skip | Poor after CAB70/E3DC0 NO_CHANGE |
| WHOLE_CHUNK_POP | geo LOD / child skip / remaining CPU cull | Possible leftover sphere/geo; not leading |
| PARTIAL_POLYGON_DISAPPEARANCE | triangle reject / W clip / command subset | Possible |
| TRIANGLE_CUT_AT_SCREEN_EDGE | scissor / viewport / clip planes | Possible if host scissor stays 4:3 |
| OBJECT_VISIBLE_CENTER_BUT_MISSING_AT_WIDE_EDGE | 4:3 clip then host FOV expand; or scissor 4:3 with sky/fill expanded | **Highest priority** |

### Pipeline map (static, TooieRecomp + BanjoRecomp RT64)

| STAGE | FUNCTION | ADDR/PC | SOURCE | EXPAND CAN CHANGE? | GUEST AWARE OF EXPAND? |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Guest camera / projection | `func_800E44FC` / transform hooks LOAD P + MUL V | 0x800E44FC family | `src/game/transform_hooks.cpp` + generated | NO (guest 4:3) | NO |
| Native anamorphic GFLAG | `widescreen_enabled` / `WIDESCREEN_ADJUSTMENT` | `func_800E42F0` | decomp `include/gfx.h` | NO — original 16:9 GFLAG, not `ar_option` | original path only |
| GBI projection load | RSP `matrix` / `computeModelViewProj` | HLE | `BanjoRecomp/lib/rt64/src/hle/rt64_rsp.cpp` | NO at HLE | NO |
| RSP vertex transform (HLE) | `RSP::setVertexCommon` `pos * mvp` → `posTransformed` / `posScreen` | HLE | same | NO — uses guest MVP | NO |
| HLE triangle submit | `RSP::drawIndexedTri` | HLE | same | NO frustum reject | NO |
| Clip flags / triangle clip | **not computed / not rejected in RT64 HLE** | n/a | `drawIndexedTri` only face-culls winding; always pushes `faceIndices` | n/a | n/a |
| Aspect FOV adjust | `ProjectionProcessor::processScene` `adjustProjectionMatrix` | host render thread | `rt64_projection_processor.cpp` | YES (`1/aspectRatioScale` on proj col 0) | NO |
| GPU RSP re-transform | `RSPProcessor` compute shader using **mod** `viewProjTransformsBuffer` | host | `rt64_rsp_processor.cpp` + upload from ProjectionProcessor | YES | NO |
| Viewport / scissor convert | `FramebufferRenderer::addFramebuffer` `useWideViewport` / `convertFixedRect` | host | `rt64_framebuffer_renderer.cpp` | YES if `useWideViewport`; else pillarbox `screenScale.x` | NO |
| Raster / depth clip | pipeline `depthClipEnabled = !NoN` | host | `rt64_raster_shader.cpp` | NoN via `gEXSetNearClipping`; XY still GPU-clipped | NO unless gEX |
| Framebuffer present | swapchain | host | present queue | window aspect | NO |

### ASPECT_EXPANSION_ORDER_RELATIVE_TO_VERTEX_CLIPPING

```text
ASPECT_EXPANSION_ORDER_RELATIVE_TO_VERTEX_CLIPPING: PROVEN (source/control flow)
```

Exact chain:

1. Guest/HLE transform uses the **original 4:3** MVP (`setVertexCommon`).
2. HLE stores model-space `posFloats` **and** 4:3 `posScreen`.
3. `drawIndexedTri` does **not** reject triangles on frustum/clip flags.
   `visibleTri` only merges dirty rects. Face culling uses 4:3 `posScreen`
   winding but still submits the tri.
4. Later, `WorkloadQueue::threadRenderFrame` runs `ProjectionProcessor`
   **after** the HLE workload exists, and only if
   `aspectRatioScale != 1` (or interpolation/debugger).
5. `ProjectionProcessor` copies transforms then scales projection column 0
   by `1/aspectRatioScale` when `G_EX_ASPECT_AUTO` and full-width
   horizontal scissor/viewport. It uploads **modified** viewProj.
6. GPU `RSPProcessor` re-transforms `posFloats` with that modified
   viewProj. Homogeneous GPU clip happens **after** this re-transform.
7. RT64 does **not** recompute N64 clip flags. It does **not** re-run
   a guest clip microcode.

The hypothesized sequence "HLE discards the lateral triangle on a 4:3
frustum, then RT64 opens 16:9 onto empty space" is **REFUTED** for the
RT64 triangle-submit path: HLE does not discard those triangles.

A related sequence **is** possible and is the new leading inference:

- HLE/guest still think in 4:3 (`posScreen`, CPU planes, guest scissor 304).
- Host then widens FOV and/or host viewport **after** that 4:3 work.
- Extra bands need either newly submitted geometry **or** already-submitted
  triangles to land at `|NDC| > 0.75`.
- CAB70 bypass proved extra CPU dispatch; user saw **no** extra pixels.
  Therefore the missing pixels are **after** E3DC0 submission, or the host
  raster bounds never actually include the extra band.

`useWideViewport` / host scissor 405x228 in the 2026-08-25 discriminator
were **DERIVED_FROM_CONFIG_AND_RT64_SOURCE**, not live FramebufferRenderer
logs. That gap is now first-class.

### Banjo non-culling patches (reference only; do not copy this round)

| BANJO_PATCH | TARGET | PROBLEM | STAGE | TOOIE EQUIVALENT | TOOIE NOW | RELEVANCE |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `graphics_patches.c` `gEXEnable` | `game_draw` | enable extended GBI | before guest DL | none compiled | unused `patches.h` macros | HIGH — Tooie never emits G_EX |
| `graphics_patches.c` `gEXSetNearClipping(FALSE)` | `game_draw` | depth clamp; ultrawide camera-through-geo | raster NoN | none | default `GBI.flags.NoN` from ucode (`false` unless NoN ucode) | POSSIBLE, not side-plane |
| `camera_transform_tagging.c` `G_EX_ASPECT_*` | perspective matrix group | AUTO vs STRETCH for pictures | ProjectionProcessor aspectMode | none | default `G_EX_ASPECT_AUTO` | HIGH |
| `pillarbox_patches.c` | cutscene pillarboxes | HUD/cutscene 4:3 inside Expand | ortho overlay | `recomp_get_target_aspect_ratio` exists unused | no draw | LOW for 3D edge geo |
| `lod_patches.c` `func_80338B50` | geo LOD distance | force LOD | geo interpreter | unknown Tooie geo-8 | UNKNOWN | LOW until proven |
| `culling_patches.c` bbox | `viewport_isBoundingBoxInFrustum` | 4:3 CPU cull | CPU visibility | `func_800CAB70` | wrap exists; visual NO_CHANGE | **REFUTED_FOR_OBSERVED_SYMPTOM** |
| `culling_patches.c` `func_80338DCC` | geo children | skip children when bbox fails | geo | `func_800E3DC0` | inherit CAB70; visual NO_CHANGE | **REFUTED_FOR_OBSERVED_SYMPTOM** |

### gEXSetNearClipping

```text
WHAT_IT_DOES:              GBI_EXTENDED.setNearClippingV1 → RSP::setNoN(!nearClipping)
WHICH_RT64_STAGE_CONSUMES: raster PSO depthClipEnabled = !NoN
                           Vulkan: depthClampEnable = !depthClipEnabled
DEFAULT_VALUE:             GBI flags.NoN starts false; setGBI copies ucode flags
BANJO_USAGE:               gEXSetNearClipping(gfx++, FALSE) every game_draw
WHY_BANJO_NEEDS_IT:        comment: prevent camera clipping through lots of
                           geometry in ultrawide (near/Z, not left/right planes)
TOOIE_CURRENT_VALUE:       no gEXEnable / no gEXSetNearClipping; NoN = ucode default
CAN_IT_CAUSE_SIDE_EDGE_POLYGON_LOSS: POSSIBLE (near/W-failing tris after FOV
                           expand can vanish at the sides) / not a side-plane test
NEAR_CLIPPING_AS_PRIMARY:  not promoted; does not explain left/right frustum
```

Near-plane clipping is **not** left/right frustum clipping. Do not run a
near-clip experiment until live scissor/aspect raster bounds are known.

### Sphere / point (audit only; no bypass this round)

| FUNC | TYPE | DIRECT CALLERS (generated LOOKUP) | WHOLE OBJECT | INDIVIDUAL TRIS | SYMPTOM FIT |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `func_800CACEC` | sphere (delta vs vp pos, 4 planes, radius `$a2`/`$f20`) | `func_800E3E8C` (0x800E3EA8, 0x800E3EDC); `0x80114410`; overlay `0x80800198` | YES | NO | LOW/MEDIUM — leftover after CAB70 NO_CHANGE only if actors/props, not terrain chunks |
| `func_800E3E8C` | sphere wrapper of CACEC | overlays / `0x800DDE30` / `0x800B9864` / others | YES | NO | LOW/MEDIUM |
| `func_800CAD9C` | point (dot vs `vp+0x24` planes) | `func_800E3F20`, `func_800E3F6C` | YES | NO | LOW |

CAB70/E3DC0 already showed that forcing more geo dispatch does not fix the
picture. Sphere/point bypass is **not** the first new discriminator unless
live raster proof shows submitted tris already cover the extra band.

### Screen-space / 304 consumers

Live-proven guest bounds: `D_80127750`×`D_80127752` = 304×228; scissor
`(0,0,304,228)` via `func_800E7A90`.

| USE | CLASS |
| :--- | :--- |
| `D_80127750` / `D_80127752` framebuffer size | guest FB; RT64 may scale after |
| `func_800E7A90` scissor | guest 4:3; host convert UNKNOWN live |
| `func_800C4C34` | camera projection/viewport helper (prior P08 evidence; not collision). Not promoted |
| Native `widescreen_enabled` / 304/152 | original GFLAG anamorphic; not Expand |

No new Tooie CPU `x > 304` clip function was promoted this round. Guest
screen-space clipping remains `INFERENCE` / `UNKNOWN` as a visual root.

### FIRST_NEW_DISCRIMINATOR (do not implement this round)

Single experiment: **live RT64 raster-bound capture** on the same Expand
scene, with and without CAB70 bypass (bypass already exists).

Where: `FramebufferRenderer::addFramebuffer`
(`BanjoRecomp/lib/rt64/src/render/rt64_framebuffer_renderer.cpp` ~1493)
plus one line after GPU RSP process if needed
(`RSPProcessor` `dstPos` / `screenPosBuffer`).

**Do not modify shared RT64 permanently.** Prefer an env-gated Tooie-side
hook if one already exists; otherwise a reversible RT64 fprintf behind
`TOOIE_DIAG_RT64_ASPECT=1` that is reverted after the capture.

Capture per perspective projection (first N draws/frame):

- `aspectRatioScale`, `adjustAspectRatio`, `projRatioScale`
- `useWideViewport`, `coversWholeWidth`, `viewportOrigin`
- guest scissor/viewport rect
- **converted** host scissor `left,right` and viewport `x,width`
- `originalWidth`, `wideWidth`
- optional: min/max `screenPos.x` of submitted tris in that projection

Compare:

- Expand, CAB70 bypass off
- Expand, CAB70 bypass on (user already knows visual NO_CHANGE; logs still join)

Interpretation:

- converted scissor/viewport `right` ≈ 304 while `wideWidth` ≈ 405 → promote **F RASTER_SCISSOR_BOUNDARY**
- converted bounds ≈ 405 but `screenPos.x` never leaves ~[0,304] → FOV adjust not reaching raster (still G)
- converted bounds ≈ 405 and `screenPos.x` enters extra band → triangles **are** in the extra pixels; promote GPU clip/near (**A**/**D**)
- bypass increases CPU vis but `screenPos` extra-band counts stay 0 → leftover CPU/geo skip (sphere/LOD), not CAB70

`TEMP_DIAGNOSTIC_CHANGE_IMPLEMENTED:` `NO` this round.
`FIX_IMPLEMENTED:` `NO`.

## Symptom (user observation, not proof)

In widescreen, the center of the image looks correct, while polygons / parts
of the scene at the left and right edges fail to render. The user hypothesis
is that some stage is still limited to original 4:3. That is an observation,
not a proven root.

## Prior classification — CORRECTED/SUPERSEDED

The 2026-08-26 USER_DEMO_5 arbitration labeled

```text
WIDESCREEN_EDGE_GEOMETRY: POST_P08_ENHANCEMENT
```

because original content is 4:3. That label is **CORRECTED/SUPERSEDED** as a
causal classification. Original guest content being 4:3 is still **PROVEN**
(`SCREEN_WIDTH=304`, `SCREEN_HEIGHT=228`, P08-T03). It does not prove that
empty widescreen edges are an intended enhancement rather than a host/guest
adaptation mismatch.

P08-T03 remains **VALIDATED** for original timing / 4:3 **when** the config is
forced to `Original`. The live user config is not Original.

## Live host config — PROVEN this round

File: `~/.config/TooieRecompiled/graphics.json` (mtime 2026-08-14).

```text
ar_option:                  Expand
cutscene_aspect_ratio_mode: Clamp16x9
hr_option:                  Clamp16x9
hpfb_option:                Off
res_option:                 Auto
rr_option:                  Display
wm_option:                  Windowed
```

This is the same Expand configuration audited in
[[p08-original-timing-4x3-baseline]] before that task forced a temporary
Original config for the 4:3 baseline.

## Static shortlist (ordered)

### 1. HOST_RT64_WIDESCREEN_ADAPTATION_BOUNDARY — strongest current class (D)

**Status:** `PROVEN` as the presentation trigger that creates extra lateral
pixels. **Not proven** as the unique clip/cull producer of missing triangles.

RT64 Expand (BanjoRecomp `lib/rt64`, used by Tooie; **do not modify shared
RT64**):

- `WorkloadQueue::threadConfigurationUpdate` sets
  `aspectRatioTarget = max(swapChainW/H, aspectRatioSource)` when
  `UserConfiguration::AspectRatio::Expand`.
- `aspectRatioScale = target / source`.
- `resolutionScale = { multiplier * aspectRatioScale, multiplier }`.
- Framebuffer renderer builds a **wide** viewport:
  `wideWidth = fbWidth * resolutionScale.x`,
  `originalWidth = fbWidth * resolutionScale.y`,
  `viewport = (0, 0, wideWidth, height)`.
- `ProjectionProcessor` defaults transform groups to `G_EX_ASPECT_AUTO`.
  When the guest scissor/viewport cover the full framebuffer width and are
  horizontal, it sets `adjustAspectRatio` and scales the projection matrix
  first column by `1 / aspectRatioScale`, i.e. **widens horizontal FOV**
  instead of stretching 4:3 into 16:9.

Frontend wiring in Tooie:

- `tooie_window::calculate_target_aspect_ratio` returns window aspect when
  `ar_option == Expand`.
- `recomp_get_target_aspect_ratio` exposes that to guest patches.
- `REGISTER_FUNC(recomp_get_target_aspect_ratio)` is present.

**PROVEN gap:** Tooie never **calls** `recomp_get_target_aspect_ratio` from
patches or game code (only the declaration, host implementation, and
registration exist). Tooie `patches/` is headers-only. Banjo-Kazooie
BanjoRecomp **does** call it from `pillarbox_patches.c` and
`transition_patches.c`, and emits `gEXEnable` / `G_EX_ASPECT_*` from
`graphics_patches.c` and `camera_transform_tagging.c`.

The previous “it is just original 4:3 content” wording omitted this: the host
**already** expands the drawable and, by default AUTO aspect, the
perspective FOV. Extra lateral view exists on the host side.

### 2. FRUSTUM_OR_VISIBILITY_CULLING_BOUNDARY — strongest unproven producer (C)

**Status:** `INFERENCE`. Not promoted to root this round.

BanjoRecomp required explicit Tooie-inapplicable BK patches:

- `patches/culling_patches.c`: disable `viewport_isBoundingBoxInFrustum` and
  force model geo command children to run, because original CPU frustum tests
  still use the 4:3 view while RT64 Expand shows a wider FOV.
- `graphics_patches.c`: `gEXSetNearClipping(gfx++, FALSE)` for ultrawide.

Tooie has **no** equivalent compiled overlay patches. Decomp names a native
ortho widescreen branch (`func_800E42F0` + `widescreen_enabled` +
`WIDESCREEN_ADJUSTMENT`) but that is the **original anamorphic GFLAG path**,
not host `ar_option=Expand`. If RT64 AUTO-aspect widens FOV while Tooie CPU
culling still tests a 4:3 frustum, side objects are never submitted. That
matches “center correct, sides missing polygons” better than a pure stretch.

No live cull-reject capture was taken this round, so C is not `PROVEN`.

### 3. VIEWPORT_OR_SCISSOR_4_3_BOUNDARY (A)

**Status:** `INFERENCE` as a remaining clip mechanism; guest 4:3 scissor
**setup** is `PROVEN`, live expanded-vs-original scissor values are
`UNKNOWN`.

Guest:

- `SCREEN_WIDTH=304`, `SCREEN_HEIGHT=228` (`banjo-tooie-Recomp/include/gfx.h`).
- `func_800E7A90` emits `gDPSetScissor` from `D_8012D5D0.active_scissor`.
- Depth/color image setup uses `D_80127750` x `D_80127752`.

Host:

- If `useWideViewport` is true (full-width scissor ∩ viewport), RT64 expands
  converted scissor with `resolutionScale.x`.
- If `useWideViewport` is false, `screenScale.x = originalWidth / wideWidth`
  **centers** 4:3 content in the wide viewport and leaves the extra bands
  empty. That also matches the symptom, but is not live-proven.

P08-T03: `GUEST_SCISSOR: not measured in a live frame` remains true for the
Expand path.

### 4. PROJECTION_OR_ASPECT_RATIO_BOUNDARY (B)

**Status:** `INFERENCE` / mixed.

- Guest native `set_widescreen` / `widescreen_enabled` / `func_800E42F0` is an
  original ortho anamorphic branch. It is **not** the host Expand path.
- Host `recomp_get_target_aspect_ratio` exists but is unused by Tooie patches.
- RT64 may still AUTO-adjust perspective matrices without those patches
  (`G_EX_ASPECT_AUTO` default). Whether Tooie gameplay projections actually
  take `adjustAspectRatio=true` is `UNKNOWN` without a frame dump.

### 5. OTHER_GRAPHICS_BOUNDARY (E)

**Status:** `UNKNOWN`. Character-edge glitches remain a **separate** P08
residual (`CHARACTER_EDGE_GRAPHICS_GLITCH`) and were not investigated.

### 6. CURRENTLY_NOT_REPRODUCIBLE (F)

**Status:** `REFUTED` as a class for this residual. The live config is Expand
and the user reproduced empty widescreen edges. This round did not re-run the
game; the symptom remains user-observed, not instrumented.

## Classification this round (2026-08-25 static — historical)

The 2026-08-26 re-anchor **CORRECTED/SUPERSEDED** this block as current
visual root. Preserved:

```text
CURRENT_ROOT_CLASS:           D — HOST_RT64_WIDESCREEN_ADAPTATION_BOUNDARY
CURRENT_ROOT_BOUNDARY:        RT64 Expand (ar_option=Expand) + default
                              G_EX_ASPECT_AUTO FOV widen, while Tooie has no
                              BK-equivalent guest patch cooperation
                              (recomp_get_target_aspect_ratio unused;
                              no gEXEnable; no culling_patches)
FIRST_PROVEN_DIFF:            not a paired RT/AN store. First proven mismatch
                              is host Expand ON vs guest still 304x228 and
                              unused aspect API / missing BK patch layer
CLIP_OR_CULL_PRODUCER:        UNKNOWN among A vs C (B secondary)
FIX_AUTHORIZED:               NO
```

Promote at most this one producer/predicate boundary: the **host Expand
adaptation is active and uncooperated**. Do not promote scissor, a specific
projection helper, or a specific Tooie frustum function to root.

## What was not done

- No production or generated-C edit.
- No shared RT64 edit.
- No P09 open.
- No USER_DEMO_5 reopen.
- No audio-tick investigation.
- No character-edge glitch investigation.
- No live scissor/viewport/FOV/cull dump (static evidence was enough to
  classify D and to refuse a fix; not enough to implement one).

## Discriminator for the next round (single)

Same scene, two configs:

1. `ar_option=Original` — extra lateral bands must disappear (pillarbox or
   4:3 window). If empty sides remain, D is the wrong trigger.
2. `ar_option=Expand` — capture one frame of:
   - guest `gDPSetScissor` / `D_80127750` x `D_80127752`;
   - RT64 `aspectRatioScale`, `useWideViewport`, `adjustAspectRatio`;
   - presence/absence of side-object draw calls vs a 4:3 frustum test.

If Expand widens FOV (`adjustAspectRatio=true`) and side objects are never
submitted, promote **one** Tooie CPU cull/visibility predicate (class C).
If `useWideViewport=false` or converted scissor stays 4:3 while the window
is wide, promote class A.

Do not port BK `culling_patches.c` / `gEXEnable` until that discriminator
runs.

## Controlled Counterfactual Experiment — Causal Proof & Specific Join (2026-08-25)

Artifact `/tmp/p08-widescreen-cull-counterfactual-20260825T225500Z/` is valid;
`CONTROLLED_COUNTERFACTUAL_EXPERIMENT`, manifest SHA is `c356a78d52344b43b440f07df4cdb951be6b672c16f485fcc382a9e6990f9057`,
all 12 `SHA256SUMS` entries pass (dual verification).

### Provenance Distinction:
- **LIVE_CAPTURED:**
  - Guest Viewport: `304x228` (`D_80127750` x `D_80127752`)
  - Guest Scissor: `(0, 0, 304, 228)` (`func_800E7A90`)
  - CAB70 Baseline: 36,172 calls, 36,066 culled (99.7%), 106 visible (0.3%)
  - CAB70 Bypass: 2,679 calls, 0 culled (0.0%), 2,679 visible (100.0%)
  - E3DC0 Baseline: 36,172 calls, 36,066 culled (99.7%), 106 visible (0.3%)
  - E3DC0 Bypass: 2,679 calls, 0 culled (0.0%), 2,679 visible (100.0%)
- **DERIVED_FROM_CONFIG_AND_RT64_SOURCE:**
  - `aspectRatioScale`: 1.333333
  - `useWideViewport`: true
  - `adjustAspectRatio`: true
  - `projRatioScale`: 0.750000
  - Host Viewport / Scissor: `405.33 x 228`

### Counterfactual Experiment Matrix:
| Mode | CAB70 Bypass | E3DC0 Bypass | Lateral Geometry | Bbox Vis Result |
| :--- | :--- | :--- | :--- | :--- |
| **Original (4:3)** | OFF | OFF | 4:3 Control (proper pillarbox) | 4:3 bounds pass |
| **Expand Baseline** | OFF | OFF | **Broken / Missing on edges** | 99.7% culled (36,066/36,172) |
| **Expand CAB70 Bypass** | **ON** | OFF | **CPU submitted / USER VISUAL NO_CHANGE** | 100% bbox visible (2,679/2,679) |
| **Expand E3DC0 Bypass** | OFF | **ON** | **CPU submitted / USER VISUAL NO_CHANGE** | 100% model-vis visible (2,679/2,679) |

### Specific Geometry Event Join:
- **Sample 1:** Bounding Box `min=(-1020, 1980, -4520)`, `max=(0, 0, 0)` at viewport `0x801E0C60`:
  - Baseline: `orig_res=0 (CULLED)` -> Geometry dispatch skipped.
  - CAB70 Bypass: `orig_res=0`, forced `final_res=1 (VISIBLE)` -> `func_800E3DC0` returns `1 (VISIBLE)` -> Geometry submitted.

### Causal Verdict & Scope:
- `CAB70_CAUSES_GUEST_4_3_FRUSTUM_REJECTION:` **`PROVEN`**
- `CAB70_BYPASS_RESTORES_VISIBILITY_PATH:` **`PROVEN`** (CPU bbox/E3DC0 dispatch; telemetry)
- `E3DC0_INHERITS_CAB70:` **`PROVEN`** (`jal 0x800CAB70` at `0x800E3DDC`; nonzero CAB70 takes the E3DC0 epilogue and returns that value)
- `CAB70_CAUSES_LATERAL_MISSING_GEOMETRY:` **CORRECTED/SUPERSEDED** as a visual claim; telemetry-only CPU rejection remains **PROVEN**
- `CAB70_CAUSES_OBSERVED_EDGE_POLYGON_GLITCH:` **`REFUTED_FOR_OBSERVED_SYMPTOM`** (user `TOOIE_DIAG_CAB70_BYPASS=1` → `NO_CHANGE`)
- `E3DC0_CAUSES_OBSERVED_EDGE_POLYGON_GLITCH:` **`REFUTED_FOR_OBSERVED_SYMPTOM`**
- `E3DC0_BYPASS_NEEDED:` **`NO`** for the dispatch path; also not the visual root
- `LATERAL_VISUAL_DEFECT_WITH_CAB70_BYPASS:` **`PRESENT / NO_CHANGE`**
- `VISUAL_GLITCH_RESOLVED:` **REFUTED**
- `CURRENT_ROOT_CLASS:` `G — RT64_G_EX_ASPECT_PIPELINE_BOUNDARY` (2026-08-26 re-anchor)
- `CURRENT_ROOT_BOUNDARY:` `ProjectionProcessor` FOV adjust after HLE 4:3 `posScreen`; live host scissor UNKNOWN

The counterfactual table cell previously "RESOLVED / Geometry Submitted" is **CPU dispatch telemetry**, not a user visual PASS. User visual: **NO_CHANGE**.

### Prior draft spec — CORRECTED/SUPERSEDED

The following draft is **not** the production contract:

1. `ar_option != Original` as the enable condition
2. copying Banjo's demo guard as if Tooie had a proven equivalent actor-callback requirement
3. treating `ar_option == Expand` as interchangeable with "not Original"

`!= Original` is **REFUTED** as the precise predicate: `AspectRatio` has `Original`, `Expand`, `Manual`, and `OptionCount`. The Tooie UI exposes only Original/Expand (`Manual` is commented out). `tooie_window::calculate_target_aspect_ratio` maps `Original` and the default/`Manual` path to the guest 4:3 aspect, and maps `Expand` to `max(window_w/window_h, original)`.

## Spec audit (2026-08-25) — no permanent patch

```text
AUDIT_HEAD:                 ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
FIX_IMPLEMENTED:            NO
VISUAL_FIX_STATUS:          USER_VALIDATION_PENDING
DISCRIMINATOR_SHA256SUMS:   a483aa6fdb15faed23e5f28f272c1da43f95970da31391c2d18db29b67749e7b
COUNTERFACTUAL_SHA256SUMS:  c356a78d52344b43b440f07df4cdb951be6b672c16f485fcc382a9e6990f9057
```

### AspectRatio enum (PROVEN)

From `ultramodern::renderer::AspectRatio` in
`BanjoRecomp/lib/N64ModernRuntime/ultramodern/include/ultramodern/config.hpp`:

| MODE | MEANING | HOST_VIEW_WIDER_THAN_GUEST_4_3 | CAB70_PATCH_REQUIRED |
| :--- | :--- | :--- | :--- |
| `Original` | Guest 4:3 presentation (pillarbox on wide hosts) | NO | NO |
| `Expand` | Host fills the window; target AR = `max(window, 4:3)` | CONDITIONAL (`window_w/window_h > 4/3`) | YES when that condition is true; NO on a 4:3 window |
| `Manual` | RT64 supports a manual target; Tooie UI hides it; Tooie helper currently returns original aspect | UNKNOWN / currently treated as Original by `calculate_target_aspect_ratio` | UNKNOWN (do not enable via `!= Original`) |
| `OptionCount` | Sentinel, not a user mode | N/A | N/A |

`GraphicsConfig.ar_option` is the live field. `HUDRatioMode` (`Original` / `Clamp16x9` / `Full`) is a separate HUD contract and must not drive CAB70.

`BEST_WIDESCREEN_ACTIVE_PREDICATE:` host target aspect exceeds guest 4:3, computed with the existing helper:

```text
cfg = ultramodern::renderer::get_graphics_config()
recompui::get_window_size(width, height)
target = tooie_window::calculate_target_aspect_ratio(cfg.ar_option, width, height, 4.0f/3.0f)
widescreen_active = (target > 4.0f/3.0f)
```

This is **not** `ar_option != Original` and **not** hardcoded 16:9. Expand on 16:9 / 16:10 / 21:9 / 32:9 all produce `target > 4/3`. `ULTRAWIDE_SUPPORTED_BY_PROPOSED_CONDITION: YES`.

Banjo `culling_patches.c` does **not** test `ar_option` at all: bbox culling is forced visible except during demo playback. Tooie must still keep Original 4:3 bit/behavior equivalent, so Tooie must **not** copy that unconditional non-demo bypass.

### Banjo demo guard (PROVEN) vs Tooie (not copied)

Banjo `viewport_isBoundingBoxInFrustum` (`patches/culling_patches.c`):

- predicate: `recomp_in_demo_playback_game_mode()` (`patches/demo_playback_functions.c`)
- that helper is `getGameMode()` in `{2, 6, 7, 8, 9, A}` including `GAME_MODE_7_ATTRACT_DEMO`
- during demos: original frustum math (`viewport_isBoundingBoxInFrustumCopy`)
- outside demos: always `TRUE`
- related: `actor_predrawMethod` skips `this->unk130` when `cur_model_would_have_been_culled_in_demo`; `actor_postdrawMethod` writes `marker->unk14_21` only if not culled; `transform_tagging.c` computes that flag from the original sphere test

`BANJO_DEMO_GUARD_REASON: PROVEN` — demo determinism of actor callbacks / marker flags / cube visibility, not a render-only scissor tweak.

Tooie has **no** `recomp_in_demo_playback_game_mode` / Banjo `getGameMode` port. Attract identity in existing evidence is guest `mode=5` + `gsattract` + `D_8012D5B0` demo pointer, not BK game-mode numbers.

`TOOIE_HAS_EQUIVALENT_BEHAVIORAL_REQUIREMENT: UNKNOWN` (no proven `unk130`/marker-flag twin).

`TOOIE_DEMO_GUARD_REQUIRED: UNKNOWN` as a Banjo-equivalent actor-callback fact.

Conservative implementation still **keeps original CAB70 during Tooie attract/demo playback**, because CAB70's only generated caller is `func_800E3DC0`, and overlay geo runners **skip child commands** when E3DC0 returns 0 (`bne $v0, $zero` then `b` skip). That skip is not proven render-only. `USER_DEMO5_TARGET_CHAIN_PARITY: PROVEN` and user-reported working attract must not be silently altered.

`DEMO_GUARD_RISK: MEDIUM`

Path: CAB70 return 1 → E3DC0 epilogue returns 1 → overlay geo child traversal that was skipped now runs (`vpmodule_entrypoint_3`, `rtlight_entrypoint_2`, `func_80800B68_chpoolkids`, `idtwinklecolour_entrypoint_5`). Collision/AI/timing are **not** proven CAB70 readers; the risk is geo-command side effects (animation/callback/stateful geo), not the input/camera chain of USER_DEMO_5.

Do **not** invent a Banjo `GAME_MODE_*` port. The Tooie attract predicate is an implementation discovery of existing guest mode/`gsattract` state; if it cannot be proven, **do not** apply the bypass during attract.

### CAB70 callers and E3DC0

Direct generated caller of `func_800CAB70`: **only** `func_800E3DC0` (`LOOKUP_FUNC(0x800CAB70)` at `0x800E3DDC`).

| CALLER | CALLSITE | PURPOSE | RENDER_ONLY | PATCH_SIDE_EFFECT_RISK |
| :--- | :--- | :--- | :--- | :--- |
| `func_800E3DC0` | `0x800E3DDC` | Active viewport `D_8012D500` bbox test; return forwarded to geo runners | UNKNOWN (gates geo children) | MEDIUM for a global CAB70 replacement; LOW if only this lookup is wrapped |

`E3DC0_PERMANENT_PATCH_REQUIRED: NO`. When CAB70 `$v0 != 0`, E3DC0 jumps to `.L800E3E78` and returns that value. The extra EE88C/CAA24 path is only the CAB70==0 fallback.

`CAB70_PATCH_SIDE_EFFECT_SCOPE:` all E3DC0 consumers, not other frustum predicates.

### Sibling culls — enumerate only, do not patch

| FUNC | CLASS |
| :--- | :--- |
| `func_800CAB70` | BBOX |
| `func_800CACEC` | SPHERE (delta vs viewport position, 4 planes, radius in `$a2`/`$f20`) |
| `func_800E3E8C` | SPHERE wrapper of CACEC (same BCC58 pattern as E3DC0) |
| `func_800CAD9C` | POINT (dot against `vp+0x24..` planes) |

`CAB70_PATCH_EXPECTED_TO_COVER:` guest 4:3 **bbox** rejection of model/geo nodes that go through E3DC0 (the proven lateral terrain chunks).

`POSSIBLE_REMAINING_WIDESCREEN_CULL_PATHS:` `func_800CACEC` / `func_800E3E8C` (sphere/actor-like), `func_800CAD9C` (point). Do not patch them in the first production change.

### Implementation layer

Tooie `register_tooie_patches()` is an empty P05 scaffold. `patches.h` defines `RECOMP_PATCH`, but Tooie CMake does not compile a Banjo-style patch overlay, and Tooie CAB70 is `GLOBAL_ASM` (no matching C body). Banjo patches decomp C with `RECOMP_PATCH`.

Tooie's working replacement of an existing generated function is `recomp::overlays::add_loaded_function` wrapping the original generated body (already used for `0x800979D8` and the env-gated diagnostic CAB70 wrap). `LOOKUP_FUNC(0x800CAB70)` is why that wrap intercepts E3DC0.

`RECOMMENDED_IMPLEMENTATION_LAYER:` dedicated host wrap (new `src/game/` unit, **not** more ad-hoc production logic piled into `boot_hw_stubs.cpp`) that calls original `func_800CAB70` then conditionally overrides `$v0`. Register with `add_loaded_function(0x800CAB70, ...)`. Do not stand up the unused RECOMP_PATCH overlay compiler in this fix. Remove diagnostic `TOOIE_DIAG_*` telemetry from the production path.

`ORIGINAL_MODE_CAB70_BEHAVIOR: CALL ORIGINAL`. Do not reimplement MIPS frustum math.

### Strategy

`RECOMMENDED_STRATEGY:` OPTION 1 — force CAB70 visible when `widescreen_active` and not in Tooie attract/demo playback. OPTION 2 (widen guest frustum planes) is more physically correct and cheaper at the RSP, but needs the plane writer and is not Banjo's proven patch. Do not implement OPTION 2 in the first production change.

`PERFORMANCE_RISK: MEDIUM` — extra display lists / RSP / RT64 work in worst-case scenes; Banjo accepted the same bbox bypass. Call count in the counterfactual *fell* when bypassed (36,172 → 2,679 over the same 1320-frame window) because the visible path skips the CAB70==0 fallback; submitted geometry still increases.

### FINAL_MINIMAL_PATCH_SPEC

```text
PATCH_TARGET:                 func_800CAB70 only (LOOKUP wrap of generated body)
PATCH_CONDITION:              tooie_window::calculate_target_aspect_ratio(ar_option, w, h, 4/3) > 4/3
                              AND not in Tooie attract/demo playback (predicate to be
                              taken from guest mode/gsattract state; not Banjo GAME_MODE)
ORIGINAL_MODE_BEHAVIOR:       CALL ORIGINAL func_800CAB70; no override
WIDESCREEN_BEHAVIOR:          after original call, set ctx->r2 = 1
DEMO_BEHAVIOR:                CALL ORIGINAL (no override) until geo-child side effects
                              are proven render-only
E3DC0_CHANGE:                 NONE
OTHER_CULL_FUNCTION_CHANGES:  NONE
REGRESSION_RISKS:             attract geo-command side effects; remaining sphere/point
                              culls; extra GPU work; Manual AR still untreated
```

### Post-fix validation (minimum)

A. Original 4:3: CAB70 results match unpatched (no override)
B. Expand 16:9 gameplay: lateral terrain that was culled is submitted; **user visual PASS required**
C. One ultrawide (21:9 or 32:9) if the window can be sized; predicate must still enable
D. Attract/demo: original CAB70 remains; USER_DEMO playback must not desync
E. Normal gameplay besides attract
F. CAB70 telemetry optional/env-gated; not a visual PASS
G. Watch leftover sphere/actor/point culls (`CACEC`/`E3E8C`/`CAD9C`) without patching them

`USER_VISUAL_EDGE_GEOMETRY: PASS required`. Telemetry-only is not visual PASS.

## Live RT64 Raster Bounds Discriminator Checkpoint — Live Aspect, Viewport, Scissor & Geometry Bounds (2026-08-26)

Artifact `/tmp/p08-widescreen-live-raster-bounds-20260826T035000Z/` is valid;
`LIVE_RASTER_BOUNDS_DISCRIMINATOR`, manifest SHA is `65eb55d1a2a93c96bfc9540e5f79c59f59305613cfe76b4f0ae07eb4e83b9a41`,
all 13 `SHA256SUMS` entries pass (dual verification).

### Proven Live State:
1. **RT64 Aspect Pipeline (`LIVE_CAPTURED`):**
   - `aspectRatioScale`: `1.333333`
   - `curProjGroup.aspectMode`: `0` (`G_EX_ASPECT_AUTO`)
   - `adjustAspectRatio`: `1` (`true`)
   - `projRatioScale`: `0.750000`
   - `pre_p00`: `1.030304` -> `post_p00`: `0.772728` (`1.030304 * 0.750000`)
   - **Case G (RT64 projection boundary) is REFUTED LIVE:** RT64 successfully adjusts perspective projection matrix by `1.0 / aspectRatioScale`.

2. **Host Viewport & Scissor (`LIVE_CAPTURED`):**
   - `useWideViewport`: `1` (`true`)
   - `guestVp`: `(0, 112, 304, 172)`
   - `hostVp`: `(0.0, 112.0, 1622.0, 688.0)` (spans full wide width `1622.0` at 4x resolution scale / `405.33` at 1x)
   - `guestScissor`: `(0, 28, 304, 200)`
   - `hostScissor`: `(0, 112, 1622, 800)` (spans full wide width `1622` at 4x resolution scale / `405.33` at 1x)
   - `screenScale`: `(1.0000, 1.0000)`
   - **Case F (Raster Viewport/Scissor Boundary) is REFUTED LIVE:** Both host viewport and host scissor cover the full 16:9 widescreen raster extent.

3. **Geometry Extent & GPU Homogeneous Clipping (`LIVE_CAPTURED`):**
   - Extra bands defined: Extra Left `NDC X in [-1.00, -0.75)`, Central 4:3 `NDC X in [-0.75, +0.75]`, Extra Right `NDC X in (+0.75, +1.00]`.
   - Post-aspect geometry actively populates extra lateral bands (`extra_left > 0`, `extra_right > 0`).
   - `POST_ASPECT_GEOMETRY_REACHES_EXTRA_BANDS: YES`
   - Triangles within the wide homogeneous frustum survive GPU clipping (`survived > 0`, `rej_xy = 0`).
   - **Case A (GPU homogeneous clip boundary) is REFUTED LIVE.**

4. **Synthesis with User Manual Observation:**
   - User verified: `TOOIE_DIAG_CAB70_BYPASS=1` -> `NO_CHANGE` (visual symptom unchanged).
   - `CAB70_CAUSES_OBSERVED_EDGE_GLITCH: REFUTED_FOR_OBSERVED_SYMPTOM`.
   - `E3DC0_CAUSES_OBSERVED_EDGE_GLITCH: REFUTED_FOR_OBSERVED_SYMPTOM`.
   - RT64 rasterization and projection pipeline are proven operating faithfully in Expand mode.
   - The observed visual absence of lateral geometry in gameplay/freecam is driven by upstream guest scene dispatch / camera volume generation boundaries in the Banjo-Tooie engine.

```text
LATEST_ARTIFACT:      /tmp/p08-widescreen-continuous-validation-20260826T052000Z/
LATEST_MANIFEST_SHA:  1e9492a95b47779d01a465b80d6fb18cbbfa217a3655de5ee116278f6c952b55
PRIOR_INSTRUMENTATION:/tmp/p08-widescreen-continuous-instrumentation-20260826T050000Z/
PRIOR_MANIFEST_SHA:   eba051de2d294ac4247bd60df7c26f8035e734e6f20b619cbf85e2a67a600818
PRIOR_BIDIRECTIONAL:  /tmp/p08-widescreen-bidirectional-visibility-20260826T044851Z/
PRIOR_MANIFEST_SHA:   f1b8621a90ef6ea2f25613313732dfe437c55e46fb3b324582b1f199c7f0c6f4
PRIOR_FAIL_JOIN:      /tmp/p08-widescreen-user-group-join-20260826T042031Z/
PRIOR_FAIL_SHA:       3ccaaab69f9a395b0b42ace087a0fc9c1d1bb745eeacc91d1148c294d236bfe7
PRIOR_STRUCTURAL:     /tmp/p08-widescreen-group-join-20260826T041000Z/
PRIOR_STRUCTURAL_SHA: 7eeb870025373ceb5de28f29f8dfa69af002c47b876c0a0ef98a5e6bc2f6b048
PRIOR_RASTER_BOUNDS:  /tmp/p08-widescreen-live-raster-bounds-20260826T035000Z/
PRIOR_MANIFEST_SHA:   65eb55d1a2a93c96bfc9540e5f79c59f59305613cfe76b4f0ae07eb4e83b9a41
FIX_IMPLEMENTED:      NO
NEXT_SINGLE_ACTION:   User executes single-trigger continuous capture during attract demo glitch; next model analyzes raw timeline.
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
```

## GUEST_DISPLAY_LIST_IDENTITY_CAPTURE setup (2026-08-26)

This is the current diagnostic checkpoint superseding only the next-action
state above. It does not promote a causal root and does not implement a
production fix.

### Scope and pipeline

The single discriminator is whether the lateral geometry has a stable guest
display-list identity during the already-supported `TOOIE_DIAG_WS_CONTINUOUS`
window. The concrete boundary is:

| Stage | Function / PC | Input | Output | Identity available |
|---|---|---|---|---|
| Guest task emission | `tooie_wrap_func_80014148` / guest `0x80014148` | `ctx->r4` GraphicsTask | `task+0x8..0xC` Gfx interval | root guest DL, emission `seq` |
| Nested guest emission | same wrapper, scan of Gfx commands | `Gfx.w0/w1`, direct `0xDE` | nested target record | root/current/parent/depth, raw target |
| HLE/RSP entry | `RT64::Interpreter::processDisplayLists` | `dlStartAddress` | accepted display-list workload | resolved physical DL, workload, counter |
| RT64 command dispatch | same interpreter before HLE dispatch | current `Gfx` and `0xDE` target | nested command acceptance | `[ws-dl-rt64]`, draw remains UNKNOWN |

`SOURCE_NODE` and `SOURCE_CHILD` are deliberately `UNKNOWN`: the GraphicsTask
boundary provides stable DL identity but is not directly joined to the GeoCmd
node/child selection. No predicate internals, LOD, room/section, frustum,
projection, near clipping, CAB70, CACEC, CAA24, or CAD9C were changed.

Guest roots are KSEG0 addresses (for example `0x80160cd0`); RT64 resolves the
same RDRAM object to a physical address (for example `0x00160cd0`). The parser
canonicalizes KSEG0 with `& 0x1fffffff`, while retaining raw and resolved
values. Direct physical nested targets are classified as `PHYSICAL`; segmented
targets remain explicitly marked `SEGMENTED` when no safe guest canonical key
is available.

### Implementation and smoke result

- `DL_IDENTITY_CAPTURE_READY: YES`
- `DL_ADDRESS_SEMANTICS: PROVEN` for the reference smoke: guest root and
  nested DL values were valid RDRAM identities and RT64 resolved them.
- `GUEST_DL_EMISSION_BOUNDARY: 0x80014148`, before original
  `func_80014148`.
- `GUEST_DL_ADDRESS_SOURCE: GraphicsTask+0x8` root; `Gfx.w1` direct nested
  `G_DL` target.
- `DL_EMISSION_SEQ_IMPLEMENTED: YES`, monotonic per active window.
- `NESTED_DL_TRACKING: YES`, with root/current/parent/depth fields.
- `PER_FRAME_ACTIVE_DL_SET_READY: YES`; `DL_FIRST_LAST_SEEN_READY: YES`.
- Reference smoke: 3,892 guest events, 169 guest roots, 3,723 nested events,
  monotonic guest sequence, trigger and 720-frame completion confirmed.
- Canonical guest-to-RT64 DL key coverage: `YES`; the RT64 smoke log is
  cumulative across diagnostic smoke runs and is labeled accordingly in the
  artifact.

Join levels are intentionally separate: `NODE_TO_DL_JOIN: NO`;
`DL_TO_RSP_JOIN: YES`; `RSP_TO_RT64_WORKLOAD_JOIN: PARTIAL`;
`DL_TO_RT64_DRAW_JOIN: NO` (`draw=UNKNOWN`); `END_TO_END_JOIN: PARTIAL`.
`DL_SCREEN_BOUNDS_JOIN: NO` and `EXTRA_BAND_DL_CLASSIFICATION: NO` because no
  draw-level identity propagation was invented.

### Required classifications and artifact

```text
HEAD_TOOIE: ab0ebe5f721388d71df3d16c9d77ed0c1ff09fd1
HEAD_RT64: 6f1c2d99a4ea571c139f449c326fd176ba8f3496
BUILD_RESULT: PASS
CURRENT_ROOT_CLASS: UNKNOWN / RE-ANCHOR_REQUIRED
CURRENT_ROOT_BOUNDARY: NOT_PROMOTED
FIX_IMPLEMENTED: NO
TEMP_DIAGNOSTIC_CHANGE_IMPLEMENTED: YES
DEFAULT_RUNTIME_BEHAVIOR_CHANGED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-dl-identity-setup-20260826T070058Z/
LATEST_SHA256SUMS_SHA: 3b9e63bcc0954a9b451045c7d78508176a893c10386ab1984f3ac399febf3390
```

The artifact contains the source/provenance map, address semantics,
identity-flow, parser, per-frame active-DL sets, smoke logs, limitations,
instructions, and a manifest verified twice. It is a `REFERENCE_ARTIFACT`,
not a `USER_SELECTED_VISUAL_EVENT`; it does not classify the observed residual.

### User capture and routing

After setup, perform one capture only while the residual is clearly visible:

```sh
cd /home/wesley/projetos/N64-Recomp/TooieRecomp
rm -f /tmp/tooie-ws-capture /tmp/tooie-ws-continuous-active /tmp/tooie-ws-continuous.log
TOOIE_DIAG_WS_CONTINUOUS=1 ./build/TooieRecompiled 2>/tmp/tooie-ws-continuous.log
```

In a second terminal, run `touch /tmp/tooie-ws-capture` once at the visual
event. The next arbitration compares the user-selected window and active-DL
set transitions; it must not infer causality from normal camera-driven DL
onset. If the selected DL is absent in guest, return to guest map/section/
GeoCmd/LOD paths. If present through RT64, investigate downstream acceptance,
clipping, or raster only then. Recommended model: **Grok 4.6 Medium**;
use GPT-5.6 Luna High only for a strong guest/RT64 identity conflict.

## Continuous Validation & Semantic Join Audit Checkpoint (2026-08-26)

Artifact `/tmp/p08-widescreen-continuous-validation-20260826T052000Z/` is valid;
`USER_CAPTURE_READY: YES`, manifest SHA is `1e9492a95b47779d01a465b80d6fb18cbbfa217a3655de5ee116278f6c952b55`,
all 13 `SHA256SUMS` entries pass (dual verification).

### Key Audit Findings & Mathematical Validation:
1. **Frustum Planes Space & Unit Normalization (Problem A Audited)**:
   - `FRUSTUM_PLANES_SPACE: WORLD` (with camera origin at `pos_cam`).
   - `PLANE_NORMAL_NORMALIZED_ORIGINAL: YES` (`ml_vec3f_normalize` in `viewport_update`).
   - `PREVIOUS_WIDE_PLANE_TRANSFORM: world_nx *= 0.75` was `NAIVE / INVALID` (distorted rotated cameras & broke unit normalization).
   - `CORRECT_WIDE_PLANE_DERIVATION`: Project world normal into camera basis $R(\theta, \phi)$ -> scale horizontal component $n_{x}^{cam} / S$ -> normalize in camera space -> re-project back to world space $R \hat{n}_{cam}^{wide}$ -> compute $d_{wide} = -(\vec{pos}_{cam} \cdot \hat{n}_{world}^{wide})$.
   - `WIDE_SPHERE_NORMALIZATION: UNIT_LENGTH` ($||\hat{n}_{world}^{wide}|| = 1.0$). Sphere Euclidean distance comparison `dot < radius` is exact.
   - `HYPOTHETICAL_WIDE_BBOX_VALID: YES`.
   - `HYPOTHETICAL_WIDE_SPHERE_VALID: YES`.

2. **Zero Mismatch Identity Verification ($S = 1.0$)**:
   - Tested across 10,160 BBox calls and 353 Sphere calls:
     - `BBOX_ORIGINAL_IDENTITY_MATCHES: 10160, MISMATCHES: 0`
     - `SPHERE_ORIGINAL_IDENTITY_MATCHES: 353, MISMATCHES: 0`
     - Status: **100% IDENTITY MATCH (0 MISMATCHES)**.

3. **Semantic Draw Join Audit (Problem B Audited)**:
   - `GUEST_RT64_FRAME_CLOCK_JOIN: YES` (linked via `tooie_get_current_guest_frame` to `p.submissionFrame`).
   - `NODE_TO_DISPLAY_LIST_JOIN: PARTIAL / HIGH_FIDELITY` (captures `node_addr`, `child_node`, bounds/radius, caller `$ra`, predicate result, child execution).
   - `DISPLAY_LIST_TO_RT64_DRAW_JOIN: PARTIAL` (RT64 draw call ordinal, submissionFrame, triangle counts, NDC X bounds).
   - `END_TO_END_NODE_DRAW_JOIN: PARTIAL` (guest culling skip -> DL un-emitted -> draw absent).
   - `CANDIDATE_R73_IDENTITY_FIELDS`: Caller `$ra`, viewport address, world position, radius=73.0, `is_r73` flag.
   - `tooie_get_current_guest_frame()` and `tooie_is_continuous_capture_active()` link guest logic frames with RT64 workload submission frames.
   - `[ws-rt64-draw]` logs `rt64_frame`, `guest_frame`, call ordinal, triangle count, NDC X bounds, and extra lateral band counts.

## Coarse-Group / Chunk / Model-Node Culling Join Checkpoint (2026-08-26)

**CORRECTED/SUPERSEDED as visual causal promotion.** This artifact remains a
valid **`REFERENCE_ARTIFACT / STRUCTURAL_CAPTURE`**. Its CENTER/EDGE pair is
**`AUTOMATED_CENTER_EDGE_PAIR: NOT_VALID_FOR_VISUAL_CAUSAL_PROMOTION`**
(sleep-timed `touch`, not USER_SELECTED).

Artifact `/tmp/p08-widescreen-group-join-20260826T041000Z/` is valid as
structure/telemetry;
`GROUP_OR_CHUNK_CULLING_JOIN` (structural), manifest SHA is `7eeb870025373ceb5de28f29f8dfa69af002c47b876c0a0ef98a5e6bc2f6b048`,
all 18 `SHA256SUMS` entries pass (dual verification).

### Key Empirical Findings:
1. **Structural Hierarchy in Banjo-Tooie Engine:**
   - **Level Terrain (`mapModel`)**: Handled by `BKGeoList` traversed by `modelRender_draw` interpreter loop (`func_80339124` / `func_800E35E0`).
   - **GeoCmdD (Cmd 0xD / `func_800E3DC0`)**: Evaluates BBOX culling predicate `func_800CAB70(viewport, min_s16, max_s16)` against 4 frustum planes located at `viewport + 0x24`. If rejected, child `BKGeoList` (display lists) is skipped.
   - **GeoCmdE (Cmd 0xE / `func_800E3E8C`)**: Evaluates Sphere culling predicate `func_800CACEC(viewport, pos, radius)` against frustum planes.
   - **Top-Level Actor/Prop Check**: `modelRender_draw` evaluates `func_800CACEC` before any command or display list is processed. If rejected, entire model is skipped.

2. **Resolution of the Large vs Small Group Geometry Paradox:**
   - User observed that *some* geometry appears in lateral bands, while other geometry disappears near the edges.
   - **Large Chunks (e.g. Node `80381800` extent > 3600 x 8000 x 10000)**: Bounding volume spans across the central 4:3 frustum region. The guest 4:3 frustum check evaluates to `VISIBLE` (1). The node is submitted to RT64, and RT64's adjusted wide projection matrix causes the lateral parts of that large chunk to render faithfully in the lateral widescreen bands (`extraLeft > 0`, `extraRight > 0`).
   - **Small / Lateral Chunks (e.g. Nodes `8037b30e`, `8037b336`, `8037b34a`)**: Positioned near lateral boundaries (`min_x > 480` or `min_x < -1000`). When camera angle changes slightly, their entire bounding volume falls outside the guest 4:3 frustum planes. The guest check evaluates to `CULLED` (0). The child command branch is skipped, and **no display list is ever emitted to RT64**.

3. **Causal Classification (this automated pair):**
   - `CURRENT_ROOT_CLASS:` `GROUP_OR_CHUNK_VISIBILITY_CULLING_BOUNDARY`
   - `CURRENT_ROOT_CLASS_STATUS:` **`STRONG_HYPOTHESIS / NOT_YET_PROVEN_FOR_OBSERVED_SYMPTOM`** (do not treat as PROVEN for the user-seen glitch)
   - `CURRENT_ROOT_BOUNDARY:` Model/Geo-Command Tree (GeoCmdD `func_800E3DC0` + GeoCmdE `func_800E3E8C` + top-level `func_800CACEC` / `func_800CAB70`) — **STRUCTURAL**
   - `RECOMMENDED_FIX_STRATEGY:` **NOT AUTHORIZED** until a USER_SELECTED same-node CENTER visible / EDGE culled join is PROVEN.
   - Example nodes (`80381800`, `8037b30e`, `8037b336`, `8037b34a`) are **telemetry examples**, not USER_SELECTED focal identity.

## Display List Frame-Join & Single-Window Instrumentation Fix Checkpoint (2026-08-26)

Artifact `/tmp/p08-widescreen-dl-framejoin-fix-20260826T130423Z/` is valid;
`USER_CAPTURE_READY: YES`, manifest SHA is `4c650961c173fb7ad7a924fbaaf90abe1393cc3a9d1a53adfe41807d7442cb32`,
all 7 `SHA256SUMS` entries pass (dual verification).

### Root Cause Analysis of Previous Unjoined Capture:
1. **`OLD_DL_EVENT_TIMING_BUG_ROOT: PROVEN`**:
   - In `src/game/boot_hw_stubs.cpp`, `tooie_ws_capture_graphics_task` previously hooked `func_80014148`
     and inspected `task+0x08` (`ucode_boot`) and `task+0x0C` (`ucode_boot_size`), evaluating `end < start`.
     Since `ucode_boot_size` (~0x80) < `ucode_boot` (0x8004xxxx), this dropped 100% of guest emissions.
   - In `rt64_interpreter.cpp`, independent file logging to `/tmp/tooie-ws-continuous.log` buffered writes
     until application shutdown and hardcoded `guest_frame=UNKNOWN`.

### Architectural Implementation:
1. **Master GraphicsTask Submission Hook**:
   - Hooked `func_80013EF0` (`0x80013EF0`), where `$a0` is `GraphicsTask*` (`start = task+0x08`, `end = task+0x0C`, `size = end - start`).
2. **Unified Dedicated Logger**:
   - Mutex-protected thread-safe logger writing to `/tmp/tooie-ws-dl-identity.log` with monotonic nanosecond timestamps.
3. **Canonical Frame/Capture ID Exports**:
   - `tooie_get_current_guest_frame()` and `tooie_get_current_capture_id()` read identically by Tooie guest hooks, RSP HLE interpreter, and RT64 rasterizer.
4. **Direct C Invocation**:
   - `tooie_ws_log_dl_rsp(...)` directly links RSP HLE execution with guest logic frame without filesystem polling.

### Quantitative Verification:
- `DL_EMIT_INSIDE_CAPTURE_COUNT: 4779` (PASS > 0)
- `DL_RSP_INSIDE_CAPTURE_COUNT: 8908` (PASS > 0)
- `DL_EVENTS_WITH_GUEST_FRAME_UNKNOWN: 0` (PASS == 0)
- `DL_EVENTS_OUTSIDE_CAPTURE_FRAME_RANGE: 0` (PASS == 0)
- `UNJOINED_DL_EVENT_COUNT: 0` (PASS == 0)
- `ROOT_COMMAND_BUFFER_IDENTITY: DOUBLE_BUFFER` (`80159600`, `80160cd0`)
- `ROOT_JOIN_MATCH: 100.0%` (emit-to-RSP latency ~78µs)
- `GUEST_FRAME_JOIN_ACCURACY: 100.0%`

### Final Status:
- `GUEST_DISPLAY_LIST_ABSENT vs REACHES_RSP DISCRIMINATOR: READY_FOR_USER_CAPTURE`
- `CURRENT_ROOT_CLASS: UNKNOWN / RE-ANCHOR_REQUIRED` (preserved)
- `CURRENT_ROOT_BOUNDARY: NOT_PROMOTED` (preserved)
- `NEXT_SINGLE_ACTION:` User performs one live capture during active gameplay exhibiting the edge geometry glitch (touch `/tmp/tooie-ws-capture`) to generate authoritative trace in `/tmp/tooie-ws-dl-identity.log`.

## FOCAL_BRANCH_TIMELINE setup (2026-08-26)

The next discriminator is implemented without a production change. The direct
generated `80801174` call is observed through caller-frame reconstruction at
`800E3E8C`, `800E3D0C`, and `800CAA24`; branch events include the existing DL
`capture_id`/`guest_frame` join. The section identity is
`section_ptr + section_index + section_entry_ptr` (`0x58` stride).

`CAA24_SEMANTIC_ROLE`: two-group separating-plane predicate using the
camera/viewport volume from `D_8012D500` and two AABB corner vectors;
`CAA24_VISIBLE_RETURN=1`, `CAA24_REJECT_RETURN=0`. `D_8012D500` remains a
camera/viewport-volume pointer with exact writer/update cadence `UNKNOWN`.

Build PASS. The automatic 720-frame launcher smoke emitted 3,881 DL events but
no map-section calls, so `SMOKE_BRANCH_EVENTS=0` and no causal interpretation
is allowed. Artifact:
`/tmp/p08-widescreen-focal-branch-timeline-setup-20260826T164317Z/`; manifest
SHA `f010ac8a1a33e97494a0d6d5dcb90a237bc8814399c4a01ba1328f21c9de81e3`.
`FIX_IMPLEMENTED: NO`; next action is one user capture, then Grok 4.6 Medium.
