---
title: "P08 — attract movement residuals after the 0x800979D8 checkpoint"
description: "User-observed residuals in attract scenes preserved separately from the validated USER_DEMO_4 helper fix."
status: active
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-22
tags: [p08, attract, movement, residual, user-observation, demo1, demo5]
---

# Attract movement residuals

This note records manual observations after the `0x800979D8` checkpoint. They
are product/parity evidence, not automated proofs and not a new movement fix.

## CURRENT (2026-08-30) — OBJECT_FRUSTUM_PRODUCTION + INTRO_WALL_ROOT

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

Gameplay object-pop is production-fixed (`800CACEC` H-expand). Intro
walls remain `STILL_PRESENT` and independent. Next: CPU dump of
`MAP_18B_CS_STARTUP_INTRO` extents.

```text
OBJECT_FRUSTUM_PRODUCTION_PROMOTED: YES
INTRO_WALL_RESIDUAL: STILL_PRESENT
INTRO_MISSING_WALL_CLASS: SOURCE_GEOMETRY / CAMERA_FRAMING
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEXT_DISCRIMINATOR: INTRO_MAP_18B_SOURCE_EXTENT_VS_EXPAND_FRUSTUM
NEXT_SINGLE_ACTION: CPU dump MAP_18B OPA/XLU vertex XZ vs 4:3 and Expand
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-object-production-intro-root-20260830T031102Z/
```

## Prior (2026-08-30) — OBJECT_MODEL_FRUSTUM_BANJO_PORT

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

Gameplay object-pop CPU witness: `OBJECT_MODEL_FRUSTUM_SIDE_PLANE`.
Attract 62s with `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1`: 244
source-reject/target-accept (`STRONG_HIT`). Visual A/B still required.
Intro walls `MIXED` / independent.

```text
OBJECT_POP_CLASSIFICATION: OBJECT_MODEL_FRUSTUM_SIDE_PLANE
800CACEC_AS_OBJECT_POP_GATE: PROVEN
MAP_VIS_PRODUCTION_PROMOTED: YES
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: YES
NEXT_DISCRIMINATOR: USER_VISUAL_OBJECT_FRUSTUM_ASPECT
NEXT_SINGLE_ACTION: TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=1 ./build/TooieRecompiled
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-object-model-frustum-banjo-port-20260830T024845Z/
```

## Prior (2026-08-30) — OBJECT_POP_WITNESS_ANALYSIS

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

Object-pop CPU witness is `INCONCLUSIVE`: the completed diagnostic locked
`chintrochar` (`0x80086C98`) with `logs=0`. `800CACEC` is the Banjo
`viewport_func_8024DB50` analog (`STRONG_HYPOTHESIS`). Diagnostic
retargeted. Next: `TOOIE_DIAG_WS_OBJECT_POP=1 ./build/TooieRecompiled`.

```text
OBJECT_POP_CLASSIFICATION: INCONCLUSIVE
TYPE6_GAMEPLAY_OBJECT_POP_STATUS: REOPENED_TARGETED
800CACEC_STRUCTURAL_ANALOG_OF_8024DB50: PROVEN
MAP_VIS_PRODUCTION_PROMOTED: YES
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: YES
NEXT_DISCRIMINATOR: USER_VISUAL_OBJECT_POP_DIAG_CACEC
NEXT_SINGLE_ACTION: TOOIE_DIAG_WS_OBJECT_POP=1 ./build/TooieRecompiled
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
ARTIFACT_DIRECTORY: /tmp/p08-widescreen-object-pop-witness-analysis-20260830T022303Z/
ARTIFACT_CHECKSUM_MANIFEST: 471db3eaf6f138143cfcf7443adbeb6eeef183aa40d38c9a31ef54610b98dbef
```

## Prior (2026-08-30) — WIDESCREEN_EDGE_GEOMETRY 800CAA24_FINAL_SEMANTICS

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

`800CAA24` aspect-aware H-expand is **production ON** (user: gameplay
walls look good). Screenshot object pop is `PROVEN` and shared with demo;
do not classify as intro-only. `TYPE6_GAMEPLAY_OBJECT_POP_ROOT` is
`REOPENED_TARGETED`. `ACTOR_CUBE_AABB_SELECTION_AS_EXPAND_DRAW_ROOT`
remains `REFUTED` (not a global actor/pickup-cull refutation). Intro
walls remain `OPEN_MIXED` and independent of the gameplay map fix.
Next: `TOOIE_DIAG_WS_OBJECT_POP=1 ./build/TooieRecompiled`.

```text
SAME_RT64_REVISION: YES (6f1c2d99a4ea571c139f449c326fd176ba8f3496)
FOCAL_00349D38_AS_GLOBAL_ROOT: REFUTED
FOCAL_00349D38_AS_WITNESS: YES
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
ACTOR_PICKUP_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
FUNC_800F0064_CAN_CAUSE_LATERAL_EXPAND_POP: NO
SPRITE_800F0064_AS_LATERAL_EXPAND_ROOT: REFUTED
BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
BOUND_SPRITE_PATH_EXERCISED: YES
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
TYPE6_GAMEPLAY_OBJECT_POP_STATUS: REOPENED_TARGETED
ACTOR_CUBE_AABB_SELECTION_AS_EXPAND_DRAW_ROOT: REFUTED
80108C14_AS_REMAINING_DRAW_ROOT: REFUTED
REMAINING_STATIC_DECORATIVE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: YES
MAP_VIS_PRODUCTION_PROMOTED: YES
INTRA_MODEL_GEOCMD_ANALOG_UNTESTED_WORDING: CORRECTED/SUPERSEDED
PERFORMANCE_GATING_CLEANUP_PRESERVED: YES
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR: USER_VISUAL_MAP_VIS_ASPECT_AWARE
NEXT_SINGLE_ACTION: TOOIE_TEST_MAP_VIS_ASPECT_AWARE=1 ./build/TooieRecompiled (do not combine with TOOIE_TEST_DISABLE_MAP_VIS_CULL)
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
LATEST_ARTIFACT: /tmp/p08-widescreen-map-vis-final-semantics-20260830T010730Z/
LATEST_SHA256SUMS_SHA: 2c66095cc262b4d6b8fad31a4bc13b0609109db4555e223cb638fce456c3eaca
```

## Prior (2026-08-30) — WIDESCREEN_EDGE_GEOMETRY ACTOR_PICKUP_PREMODEL_VIEW_VISIBILITY

Actor/pickup pre-draw vis: `8011458C` is occupancy/near, not actor draw.
Cube AABB `800BD708` expand CF is `PARTIAL_HIT` visitation / `NO_CHANGE`
actor model draw/DL (`03508=3` `0108c=0`).
`ACTOR_PICKUP_VIEW_CULL_AS_EXPAND_ROOT: REFUTED`. Next was
`REMAINING_STATIC_DECORATIVE_MODEL_DRAW`. Artifact
`/tmp/p08-widescreen-actor-pickup-premodel-view-20260830T000358Z/`
`4706ad90341a23e6bbf299a53acac7495b32f472387dbcc3d7dc77665fec0d5d`.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY BOUND_SPRITE_INSTANCE_PER_FRAME_DRAW

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

User clarification: missing walls/objects are pervasive in intro/cutscenes
and gameplay, not specific to `00349d38`. That triangle is a witness only.

Sprite types 7/9/C dispatch at **map load** (`gccube_ep6` LIVE=2). Type 0xA
is skipped; `800CBB18` is type 0xC (`CORRECTED/SUPERSEDED`). `800F0064` is
world XZ membership, not camera/FOV/aspect (`REFUTED` as Expand pop).
`800B3CDC` products are occupancy records in `D_8012AE80+0xE0`; per-frame
consumers are queries (`800CB8F0` LIVE 5777; `draw_fn=NONE`).
`BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED`. Type-6 remains
`STRONGLY_WEAKENED` intro / `UNKNOWN / LOW_PRIORITY` global. Next:
actor/pickup pre-draw vis.

```text
SAME_RT64_REVISION: YES (6f1c2d99a4ea571c139f449c326fd176ba8f3496)
FOCAL_00349D38_AS_GLOBAL_ROOT: REFUTED
FOCAL_00349D38_AS_WITNESS: YES
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
FUNC_800F0064_CAN_CAUSE_LATERAL_EXPAND_POP: NO
SPRITE_800F0064_AS_LATERAL_EXPAND_ROOT: REFUTED
BOUND_SPRITE_VIEW_CULL_AS_EXPAND_ROOT: REFUTED
BOUND_SPRITE_PATH_EXERCISED: YES
BOUND_SPRITE_VIEW_COUNTERFACTUAL: NOT_IMPLEMENTED
TYPE6_AS_INTRO_RESIDUAL_ROOT: STRONGLY_WEAKENED
TYPE6_AS_GLOBAL_GAMEPLAY_RESIDUAL_ROOT: UNKNOWN / LOW_PRIORITY
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
PERFORMANCE_GATING_CLEANUP_PRESERVED: YES
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR: ACTOR_PICKUP_PREMODEL_VIEW_VISIBILITY
NEXT_SINGLE_ACTION: Trace actor/pickup pre-draw vis at 800CAB70/800E3DC0/800CACEC and 8011458C; prove SOURCE_OUTSIDE_TARGET_INSIDE before any actor CF. Do not reopen 800F0064 or bound-sprite draw. Do not combine vis flags.
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
LATEST_ARTIFACT: /tmp/p08-widescreen-bound-sprite-per-frame-draw-20260829T231221Z/
LATEST_SHA256SUMS_SHA: fde93ff1c4e3059d186c567f810ad753b7fbe9cdc075ede0b193726d62a98d51
```

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY GSPROPS_SPRITE_7_9_A_PER_FRAME_VISIBILITY

60s attract LIVE: t7=47 t9=36 tC=1, tests 703 acc 62 rej 641, inserts 90.
Independent `TOOIE_TEST_DISABLE_GSPROPS_VIS_CULL=1` inserts 114
(`PARTIAL_HIT`) and is `REFUTED` as Expand pop. Artifact
`/tmp/p08-widescreen-gsprops-sprite-visibility-20260829T224516Z/`
`SHA256SUMS` `40941e011e86c403ec0b9cb0f781a6db3b15ff04cd4d7ea6aae18ce5be604894`.

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY LOADED_STATIC_MODEL_PER_FRAME_DRAW

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

User clarification: missing walls/objects are pervasive in intro/cutscenes
and gameplay, not specific to `00349d38`. That triangle is a witness only.

Type-6 per-frame consumer is `800FF62C` over `D_80136EE0` (PROVEN). Vis is
Euclidean `800EFFB4`, not aspect/FOV. Independent
`TOOIE_TEST_DISABLE_TYPE6_STATIC_VIS_CULL=1` produced `NO_CHANGE`
(loaded=10, draw_attempts=84, dl_emits=53, vis rej=0). Type-6 is
`STRONGLY_WEAKENED` as the observed residual root in the intro window.
Next: sprite types 7/9/A (`t7=0` in intro).

```text
SAME_RT64_REVISION: YES (6f1c2d99a4ea571c139f449c326fd176ba8f3496)
FOCAL_00349D38_AS_GLOBAL_ROOT: REFUTED
FOCAL_00349D38_AS_WITNESS: YES
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
GSPROPS_ENTRYPOINT_1_ROLE: PROVEN_ONE_SHOT_TYPE6_LOAD
TYPE6_FIRST_PER_FRAME_CONSUMER: 800FF62C
TYPE6_VIS_COUNTERFACTUAL_RESULT: NO_CHANGE
TYPE6_AS_OBSERVED_RESIDUAL_ROOT: STRONGLY_WEAKENED
INTRO_RESIDUAL_CLASS: MIXED
USER_VISUAL_TEST_REQUIRED_NOW: NO
PERFORMANCE_GATING_CLEANUP_PRESERVED: YES
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR: GSPROPS_SPRITE_7_9_A_PER_FRAME_VISIBILITY
NEXT_SINGLE_ACTION: Exercise types 7/9/A -> gcbound -> 800F0064 on an automated attract/demo window that actually dispatches sprites; do not combine vis flags
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
LATEST_ARTIFACT: /tmp/p08-widescreen-loaded-static-model-per-frame-draw-20260829T215504Z/
LATEST_SHA256SUMS_SHA: de43c12dfcaccd59e3b4cd4ca2b6aa88a74dd232e89b757c4e6e56393b4c7940
```

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY GSPROPS_STATIC_PROP_SELECTOR

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

`gspropsDll_entrypoint_1` is a one-shot ALL-cube type-6 load (11 collected,
10 loaded in intro), not per-frame vis. Independent
`TOOIE_TEST_DISABLE_GSPROPS_VIS_CULL=1` produced `NO_CHANGE`. Extra
`800BE09C` RANGE cells are never consumed by gsprops.

```text
SAME_RT64_REVISION: YES (6f1c2d99a4ea571c139f449c326fd176ba8f3496)
GSPROPS_ENTRYPOINT_1_ROLE: PROVEN one-shot type-6 load; NOT per-frame vis
GSPROPS_SELECTOR_COUNTERFACTUAL_CPU_RESULT: NO_CHANGE
NEXT_DISCRIMINATOR: LOADED_STATIC_MODEL_PER_FRAME_DRAW
LATEST_ARTIFACT: /tmp/p08-widescreen-gsprops-static-prop-selector-20260829T210810Z/
LATEST_SHA256SUMS_SHA: 09bb0ef3fc74105e1306577af670ef13d088044768047bad03438b53378e27ce
```

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY RESIDUAL_PROP_CELL_AND_CUTSCENE_VISIBILITY_AUDIT

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

User clarification: missing walls/objects are pervasive in intro/cutscenes
and gameplay, not specific to `00349d38`. That triangle is a witness only.

USER MAP_VIS (`TOOIE_TEST_DISABLE_MAP_VIS_CULL=1`): gameplay map walls restored
a lot; items/rocks/assets still vanish at corners; intro/demo still lose walls.
USER MODEL_GEO (`TOOIE_TEST_DISABLE_MODEL_GEO_CULL=1`): looked like the original
broken baseline.

```text
SAME_RT64_REVISION: YES (6f1c2d99a4ea571c139f449c326fd176ba8f3496)
FOCAL_00349D38_AS_GLOBAL_ROOT: REFUTED
FOCAL_00349D38_AS_WITNESS: YES
FOCAL_TEXTURE_TMEM_ANOMALY: STRONGLY_WEAKENED / STATIC_AUDIT_REFUTED
FOCAL_TEXTURE_RUNTIME_REVALIDATED_WITH_NEW_AUTO_CAPTURE: NO
CAB70/E3DC0/CACEC: USER NO_CHANGE (do not rerun)
MAP_SECTION_800CAA24_VISUAL_RESULT: PROVEN_PARTIAL_FOR_GAMEPLAY_MAP_GEOMETRY
MODEL_GEO_801143A4_VISUAL_RESULT: REFUTED_FOR_OBSERVED_RESIDUAL
MODEL_GEO_FLAG_CONFIRMED_ACTIVE_DURING_GAMEPLAY: UNKNOWN (intro 12s/25s calls=0)
TOOIE_PROP_CELL_VISIBILITY_FUNCTION: 800BE09C 3x3x3 RANGE via 800BE444
PROP_CELL_COUNTERFACTUAL_IMPLEMENTED: YES (TOOIE_TEST_DISABLE_PROP_CELL_VIS_CULL; draw-pass only)
PROP_CELL_COUNTERFACTUAL_CPU_RESULT: listed 153->325; dl_rsp identical 9445; 801143A4 still 0
TOOIE_PROP_CELL_VISIBILITY_STATUS: STRONG_HYPOTHESIS actor gate; REFUTED intro RSP; UNKNOWN gsprops
INTRO_RESIDUAL_CLASS: MIXED
DEMO_RESIDUAL_CLASS: MIXED / UNKNOWN
USER_VISUAL_TEST_REQUIRED_NOW: NO
PERFORMANCE_GATING_CLEANUP_PRESERVED: YES
FIX_IMPLEMENTED: NO
NEXT_DISCRIMINATOR: GSPROPS_STATIC_PROP_SELECTOR
NEXT_SINGLE_ACTION: Identify the selector that fills gspropsDll_entrypoint_1; do not combine MAP_VIS
NEXT_RECOMMENDED_MODEL: Grok 4.6 Medium
LATEST_ARTIFACT: /tmp/p08-widescreen-residual-prop-cell-cutscene-audit-20260829T204203Z/
LATEST_SHA256SUMS_SHA: 52a0f350cc540708f9a6de44c7e60bc60066c51e6807e71c71395589bee59fbd
```

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY BANJORECOMP_WIDESCREEN_VISIBILITY_PATCH_PORT_AUDIT

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

User clarification: missing walls/objects are pervasive in intro/cutscenes
and gameplay, not specific to `00349d38`. That triangle is a witness only.

```text
SAME_RT64_REVISION: YES (6f1c2d99a4ea571c139f449c326fd176ba8f3496)
FOCAL_00349D38_AS_GLOBAL_ROOT: REFUTED
FOCAL_00349D38_AS_WITNESS: YES
FOCAL_TEXTURE_TMEM_ANOMALY: STRONGLY_WEAKENED / STATIC_AUDIT_REFUTED
FOCAL_TEXTURE_RUNTIME_REVALIDATED_WITH_NEW_AUTO_CAPTURE: NO
CAB70/E3DC0/CACEC: USER NO_CHANGE (do not rerun)
MAP_VIS_CF: TOOIE_TEST_DISABLE_MAP_VIS_CULL (800CAA24) CPU +25%; RSP dl_rsp x15.07 / prim_emit x11.48 intro
MODEL_GEO_CF: TOOIE_TEST_DISABLE_MODEL_GEO_CULL (801143A4) intro 0 calls
LOD_CF: NOT_IMPLEMENTED
INTRO_SOURCE_GEOMETRY_GAP: UNKNOWN (per-surface); exclusive authored-gap REFUTED
PERFORMANCE_GATING_CLEANUP_PRESERVED: YES
FIX_IMPLEMENTED: NO
USER_MANUAL_ACTION_THIS_ROUND: NO
NEXT_DISCRIMINATOR: USER_SIMPLE_VISUAL_CONFIRMATION_OF_MAP_VIS_COUNTERFACTUAL
LATEST_ARTIFACT: /tmp/p08-widescreen-banjorecomp-visibility-port-audit-20260829T192021Z/
LATEST_SHA256SUMS_SHA: 8efe75283a7e8fa1d024b36e92dc61a3c57408acbff07ecf6cecbdd9aacd2a0b
```

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY TARGETED_RASTER_STATE_USER_ARBITRATION

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

```text
CAPTURE_VALID: YES
CAPTURE_ID: 1
CAPTURE_START_FRAME: 2926
CAPTURE_END_FRAME: 3645
CAM_POS: (3584.1, 50.7, 4893.8)
FOCAL: 00349d38 TRI1 V0/V1/V2 exact extra-left
FOCAL_GPU_SUBMITTED: YES
PRIOR_PROJ1_DEPTH_RESULT_APPLIES_TO_FOCAL: NO
FOCAL_DEPTH_TEST_ENABLED: YES (LESS, proj=2)
FOCAL_DEPTH_TEST_RESULT_DIRECTLY_OBSERVABLE: NO
DEPTH_OCCLUSION_RESULT: UNKNOWN
FIRST_FOCAL_VS_CENTRAL_RUNTIME_STATE_DIFFERENCE: tex_id
FRAGMENT_DISCARD_SITES_ACTIVE: 0 / 7
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
WIDESCREEN_EDGE_GEOMETRY_CLASS: WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
WIDESCREEN_EDGE_GEOMETRY_CLASS_STATUS: STRONG_HYPOTHESIS
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-focal-raster-user-arbitration-20260829T181535Z/
LATEST_SHA256SUMS_SHA: a8ce5b47e43ca92986ce19628cc1c1084e6ff2fa22079dc4f17197bcf8636190
```

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY TARGETED_RASTER_STATE_SETUP

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

```text
INSTRUMENTATION_STATUS: TARGETED_RASTER_STATE_TRACE_IMPLEMENTED
PRIMARY_FOCAL_CANDIDATE: 00349d38 TRI1
FRAGMENT_DISCARD_SITES_ACTIVE: 0 / 7
POST_PROJECTION_NATIVE_COORDINATE_MISMATCH: REFUTED
SOURCE_CLIP_FLAGS_USED_AFTER_TARGET_EXPANSION: NO
FIRST_FOCAL_VS_CENTRAL_STATE_DIFFERENCE: NONE_OBSERVED
WIDESCREEN_EDGE_GEOMETRY_CLASS: WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
WIDESCREEN_EDGE_GEOMETRY_CLASS_STATUS: STRONG_HYPOTHESIS
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-targeted-raster-state-setup-20260829T173856Z/
LATEST_SHA256SUMS_SHA: fbd3366d1a666196a77c536779cd3b087a0957382ac60bee2cbffe334c81fc14
```

## Prior (2026-08-29) — WIDESCREEN_EDGE_GEOMETRY TARGETED_FOCAL_VERTEX_WINDING_USER_ARBITRATION

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

```text
CAPTURE_VALID: YES
CAPTURE_ID: 1
CAPTURE_START_FRAME: 3754
CAPTURE_END_FRAME: 4473
CAM_POS: (3598.4, 57.8, 4908.3)
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
TOP_CANDIDATE_SOURCE_DL: 00349d38 TRI1 (prior extra-left AABB; identical V0/V1/V2)
USER_SCREENSHOT_CROP_MAPPING: APPROXIMATE
POSITIVE_X_ASPECT_SCALE_CAN_FLIP_WINDING: NO
CULL_WINDING_ANOMALY: REFUTED_FOR_FOCAL
WIDE_EXTENT_PRESERVED_THROUGH_PRESENT: PROVEN
COMPOSITE_CROP_ROOT: STRONGLY_WEAKENED
WIDESCREEN_EDGE_GEOMETRY_CLASS: WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
WIDESCREEN_EDGE_GEOMETRY_CLASS_STATUS: STRONG_HYPOTHESIS
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-focal-winding-user-arbitration-20260829T164524Z/
LATEST_SHA256SUMS_SHA: d0d993e6e015d482e6807350b8d65de993d6348b7f8d96287972e9d1ee13e489
```

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY TARGETED_FOCAL_VERTEX_WINDING_USER_ARBITRATION

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

```text
CAPTURE_VALID: YES
CAPTURE_ID: 1
CAPTURE_START_FRAME: 8278
CAPTURE_END_FRAME: 8997
CAM_POS: (3477.8, 113.9, 4682.8)
MISSING_WALL_DRAW_SEMANTIC_JOIN: PARTIAL
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: STRONGLY_SUPPORTED
TOP_CANDIDATE_SOURCE_DL: 00349d38 TRI1 (prior extra-left AABB)
USER_SCREENSHOT_CROP_MAPPING: APPROXIMATE
POSITIVE_X_ASPECT_SCALE_CAN_FLIP_WINDING: NO
CULL_WINDING_ANOMALY: REFUTED_FOR_FOCAL
WIDE_EXTENT_PRESERVED_THROUGH_PRESENT: PROVEN
COMPOSITE_CROP_ROOT: STRONGLY_WEAKENED
WIDESCREEN_EDGE_GEOMETRY_CLASS: WALL_SPECIFIC_GPU_RASTER_STATE_BOUNDARY
WIDESCREEN_EDGE_GEOMETRY_CLASS_STATUS: STRONG_HYPOTHESIS
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-focal-winding-user-arbitration-20260827T193119Z/
LATEST_SHA256SUMS_SHA: f1499333a0fcdb1395f550848e8539026419058f8935c4c8c6b3a9092943667c
```

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY POST_SUBMISSION_COMPOSITE_STATE_TRACE

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

```text
FINAL_RENDER_TARGET_PIXEL_PROBE: NOT_COMPLETED / ABANDONED_FOR_SAFETY
GPU_PIXEL_READBACK_CURRENT_APPROACH: UNSAFE
EXTRA_BAND_PROJ1_DRAWS_REACH_GPU_DURING_VISIBLE_GLITCH: PROVEN
MISSING_WALL_SPECIFIC_DRAW_REACHES_GPU: NOT_PROVEN
MISSING_WALL_DRAW_SEMANTIC_JOIN: NO
WIDE_EXTENT_PRESERVED_THROUGH_PRESENT: PROVEN (CPU smoke)
COMPOSITE_CROP_ROOT: STRONGLY_WEAKENED
WIDESCREEN_EDGE_GEOMETRY_CLASS: POST_GPU_SUBMISSION_FRAGMENT_OR_COMPOSITE_BOUNDARY
WIDESCREEN_EDGE_GEOMETRY_CLASS_STATUS: STRONG_HYPOTHESIS
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-post-submission-composite-audit-20260827T052315Z/
LATEST_SHA256SUMS_SHA: 444e33af976f32ba2c4f94409a2210bf80871346281554e9891c66291bb2a2d2
```

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY GPU_DEPTH_USER_ARBITRATION

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

```text
USER_SELECTED_WIDESCREEN_MISSING_WALL: YES
CAPTURE_ID:                   1
CAPTURE_START_FRAME:          6885
CAPTURE_END_FRAME:            7604
CAMERA_EFFECTIVELY_STATIC:    YES
DRAW_TO_GPU_SUBMISSION_FAILURE: REFUTED
ARBITRATION_CASE:             C
WIDESCREEN_EDGE_GEOMETRY_CLASS: POST_GPU_SUBMISSION_FRAGMENT_OR_COMPOSITE_BOUNDARY
WIDESCREEN_EDGE_GEOMETRY_CLASS_STATUS: STRONG_HYPOTHESIS
COLOR_DEPTH_EXTENT_MATCH:     YES (2164x1140)
FAR_DEPTH_CLIP_ANOMALY:       REFUTED
MISSING_WALL_DRAW_SEMANTIC_JOIN: NO
FIX_IMPLEMENTED:              NO
LATEST_ARTIFACT:              /tmp/p08-widescreen-gpu-depth-user-arbitration-20260827T033147Z/
LATEST_SHA256SUMS_SHA:        6996bd3bcf9974c86d95fc4650f08bc8b061faf407b5fa80ed39288aa94c2eca
```

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY RSP_DRAW_USER_ARBITRATION

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

```text
USER_SELECTED_WIDESCREEN_VISIBILITY_ERROR: YES
CAPTURE_VALID: YES (id=1 frames 245023..245742)
ARBITRATION_CASE: B
RSP_TO_RT64_DRAW_CREATION_FAILURE: REFUTED
WIDESCREEN_EDGE_GEOMETRY_CLASS: POST_RT64_DRAW_VISIBILITY_BOUNDARY
WIDESCREEN_EDGE_GEOMETRY_CLASS_STATUS: STRONG_HYPOTHESIS
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-rsp-draw-user-arbitration-20260827T030225Z/
LATEST_SHA256SUMS_SHA: 2084f416c77a64082a459bf58244cc49a715972efefb454d131a441c13b914ed
```

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY POST_GCMAPSECTS_REANCHOR

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

```text
USER_OBSERVED_WIDESCREEN_GLITCH_IN_RUN: YES
OVERLAY662_TOTAL_SYSCALLS: 39 (ep4=27 ep15=12 ep9/13/17=0)
GCMAPSECTS_ENTRY_COUNT: 0
GCMAPSECTS_80801174_REQUIRED_FOR_OBSERVED_SYMPTOM: REFUTED_FOR_THIS_RUN
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED
WIDESCREEN_EDGE_GEOMETRY_CLASS: RSP_TO_RT64_GEOMETRY_BOUNDARY
WIDESCREEN_EDGE_GEOMETRY_CLASS_STATUS: STRONG_HYPOTHESIS
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-post-gcmapsects-reanchor-20260827T012256Z/
LATEST_SHA256SUMS_SHA: 3c626fd6b1b1d44520550066ef7c34d6d0c2077bb26b0b8b29325623b33e0e37
```

## Prior (2026-08-27) — WIDESCREEN_EDGE_GEOMETRY FOCAL_BRANCH_JOIN_REPAIR

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
locomotion residuals with widescreen missing-side geometry. Canonical:
[[p08-widescreen-edge-geometry]].

```text
CONTINUOUS_CAPTURE_VALID: YES (1717..2436; prior 1425..2144 historical)
FOCAL_BRANCH_EVENTS_IN_WINDOW: 0
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED
OLD_RA_GATE_ROOT_CAUSE: proven (RA semantics + CreateStatic + helper SP + address-taken EPs)
DIRECT_GCMAPSECTS_HOOK: YES (ld --wrap + 662 syscall log)
GC_MAPSECTS_HOOK_VERIFIED: NO
USER_CAPTURE_READY: NO
FIX_IMPLEMENTED: NO
WIDESCREEN_EDGE_GEOMETRY_CLASS: GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
WIDESCREEN_EDGE_GEOMETRY_CLASS_STATUS: OBSERVABILITY_REPAIR
LATEST_ARTIFACT: /tmp/p08-widescreen-focal-branch-join-repair-20260827T004617Z/
LATEST_SHA256SUMS_SHA: 2c84d1739211aff278e4e469f3af40e816640cc2f0304ff6493d62c9d1f19cfc
```

## Prior (2026-08-26) — WIDESCREEN_EDGE_GEOMETRY FOCAL_BRANCH_TIMELINE user arbitration

USER_DEMO_5 remains `RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`. Do not mix
this file's locomotion residuals with the widescreen missing-side geometry
bug. Canonical classification:
[[p08-widescreen-edge-geometry]].

```text
USER_OBSERVED_LATE_POP_IN: YES
USER_OBSERVED_EARLY_POP_OUT: YES
BIDIRECTIONAL_EDGE_VISIBILITY_ERROR: USER_OBSERVED
LEFT_SIDE_EARLY_DISAPPEARANCE_PRIOR: CORRECTED/SUPERSEDED
USER_MANUAL_CAB70_BYPASS_VISUAL_RESULT: NO_CHANGE
USER_MANUAL_E3DC0_BYPASS_VISUAL_RESULT: NO_CHANGE
USER_MANUAL_CACEC_BYPASS_VISUAL_RESULT: NO_CHANGE
CAB70_DIRECT_VISUAL_ROOT: REFUTED_FOR_OBSERVED_SYMPTOM
E3DC0_DIRECT_VISUAL_ROOT: REFUTED_FOR_OBSERVED_SYMPTOM
CACEC_DIRECT_VISUAL_ROOT: REFUTED_FOR_OBSERVED_SYMPTOM
SPHERE_CULLING_IS_VISUAL_ROOT: REFUTED_FOR_OBSERVED_SYMPTOM
CONTINUOUS_CAPTURE_VALID: YES (capture_id=1 frames 1717..2436 complete; prior 1425..2144 still VALID historically)
DL_IDENTITY_JOINED_TO_VISUAL_WINDOW: YES
FOCAL_BRANCH_EVENTS_IN_WINDOW: 0
FOCAL_DL_FIRST_SEEN_FRAME: 1720
FOCAL_DL_LAST_SEEN_FRAME: 2432
LAST_SEEN_IS_CAPTURE_TAIL: YES
FOCAL_DL_8035DD20_AS_CAUSAL_ANCHOR: WEAKENED (present on all emit frames this window)
CAA24_CONTROLS_FOCAL_MEMBERSHIP: UNKNOWN
SPHERE_CONTROLS_FOCAL_MEMBERSHIP: UNKNOWN
GUEST_NESTED_DL_SET_BEHAVIOR: CHANGING
GUEST_DL_TO_RSP_CONTINUITY: PROVEN
GUEST_DL_ABSENCE_EVIDENCE: PARTIAL
GUEST_SELECTION_FOR_PERSISTENT_DLS: REFUTED
LATE_POP_IN_SIGNATURE_FOUND: NOT_IN_THIS_WINDOW (focal already present at 1720)
EARLY_POP_OUT_SIGNATURE_FOUND: NOT_USED (last_seen is capture tail)
VISIBILITY_THRESHOLD_ASPECT_MISMATCH: NOT_PROVEN_FOR_CAPTURED_EVENT
FOCAL_DL_PHYSICAL: 0x0035dd20
FOCAL_DL_KSEG0: 0x8035dd20
FOCAL_DL_STORAGE_CLASS: LOADED_MAP_MODEL_PAYLOAD
FOCAL_DL_MAP_TERRAIN_BIND: UNKNOWN
FOCAL_DL_FROM_GCMAPSECTS: UNKNOWN (pointer) / YES (class selector)
FOCAL_DL_REACHES_800CAA24: YES (static section path)
FOCAL_DL_LOD_CONTROLLED: NO
DYNAMIC_BRANCH_JOIN: NO
WIDESCREEN_EDGE_GEOMETRY_CLASS: GUEST_MAP_SECTION_DL_SELECTION_BOUNDARY
WIDESCREEN_EDGE_GEOMETRY_CLASS_STATUS: STRONG_HYPOTHESIS
PRIOR_CLASS_GUEST_DL_SELECTION: REFINED (not REFUTED)
GROUP_OR_CHUNK_VISIBILITY_THRESHOLD: HISTORICAL_STRONG_HYPOTHESIS / NOT_SUPPORTED_BY_VISUAL_BYPASSES
AUTOMATED_CENTER_EDGE_PAIR: NOT_VALID_FOR_VISUAL_CAUSAL_PROMOTION
PRIOR_G_RT64_ASPECT_AS_VISUAL_ROOT: CORRECTED/SUPERSEDED as current visual root
PRIOR_C_CPU_FRUSTUM_AS_VISUAL_ROOT: CORRECTED/SUPERSEDED
PRIOR_D_HOST_RT64_LABEL: CORRECTED/SUPERSEDED as unique clip producer
PRIOR_POST_P08_ENHANCEMENT_LABEL: CORRECTED/SUPERSEDED as a causal class
NEXT_DISCRIMINATOR: FOCAL_BRANCH_JOIN_REPAIR
FIX_IMPLEMENTED: NO
LATEST_ARTIFACT: /tmp/p08-widescreen-focal-branch-user-arbitration-20260826T233105Z/
LATEST_SHA256SUMS_SHA: 4b1867cb2145af8dc5c33b6916943ce335edf6295a886eaef9d37bddf87a7a65
LATEST_PRODUCER_ARTIFACT: /tmp/p08-widescreen-dl-producer-trace-20260826T150351Z/
LATEST_PRODUCER_SHA: 40d591c62899a56b60f73f4e990f019b1b1890b22b4e4d6a33b41b6eba48f5ee
LATEST_ARBITRATION_ARTIFACT: /tmp/p08-widescreen-dl-framejoined-user-arbitration-20260826T143436Z/
LATEST_ARBITRATION_SHA: f0f9b493cf4bdac6ebe2378f8ce5b04ce09f8f4eab34058cd58ff7fed6d80f54
```

```text
USER_DEMO_1_MANUAL_STATUS:  RESIDUAL_REPRODUCED_BY_USER
USER_DEMO_2_MANUAL_STATUS:  APPEARS_CORRECT_BY_USER / NOT_AUTOMATED_PROOF
USER_DEMO_3_MANUAL_STATUS:  APPEARS_CORRECT_BY_USER / NOT_AUTOMATED_PROOF
USER_DEMO_4_MANUAL_STATUS:  APPEARS_CORRECT_BY_USER / bounded paired evidence
USER_DEMO_5_MANUAL_STATUS:  RESIDUAL_REPRODUCED_BY_USER
USER_DEMO_6_MANUAL_STATUS:  APPEARS_CORRECT_BY_USER / NOT_AUTOMATED_PROOF
GLOBAL_MOVEMENT_PARITY:     NOT_PROVEN
GLOBAL_ATTRACT_PARITY:      NOT_PROVEN
GLOBAL_ANIMATION_PARITY:    NOT_PROVEN
```

Observed detail:

- Demo 1 is correct for roughly 80% of the scene, then follows the wrong
  trajectory / gets lost.
- Demo 5 starts correctly, then gets lost very early in the icy/snow scene.
- Demo 4 now looks correct in manual play and its bounded paired capture
  validates the `0x800979D8 → P60 → ncbapivot` event; this does not close the
  remaining attract set.

## Hypothesis

```text
TALON_TROT_OR_LOCOMOTION_TRANSITION_HYPOTHESIS:
INFERENCE / USER_OBSERVED_CORRELATION
```

The suspected correlation is a transition between normal Banjo movement and
the Kazooie-carries-Banjo/Talon-Trot state. It is not causal proof. Possible
future discriminators include behavior/substate, locomotion mode, animation
ID/phase/timer/rate/transition, root motion, position, velocity, target speed,
acceleration, yaw, ground state, physics `dt`, and movement-target/script
state.

## USER_DEMO_5 `800EEBF0` angular-input arbitration — 2026-08-23

The immutable capture `/tmp/p08-demo5-ee780-source-20260823T053510Z/` passed
`sha256sum -c SHA256SUMS` twice. `800EE780` is confirmed downstream
elementwise addition at `0x808001AC`; its glidx-24 `sp+0x54` input is already
different. `800EEBF0`/`800EF214` reconstruct Cartesian output from degree-based
pitch/elevation, yaw/heading, and radius:

```text
x = r*cos(pitch)*sin(yaw)
y = r*sin(pitch)
z = r*cos(pitch)*cos(yaw)
```

At glidx 24, input0 and radius match, while input1 differs:
`RT=4391e610` (291.797363°), `AN=43927d8b` (292.980804°). The angle is stored
at ncbapivot `sp+0x38` (`0x80045610`) by the caller add at `0x80800194`,
stored at `0x808001A0`, immediately before `jal 800EEBF0` at `0x8080019C`.
That add combines `80012C3C` and `800125E8`; its complete operand join was
not captured, so the root remains `B-ANGLE-SOURCE-PRECALL-COMBINATOR`. See
[[p08-user-demo5-locomotion-first-divergence]].

The prior artifact metadata label `Grok 4.6 Medium` is stale; the actual
capture is documented as `Grok 4.6 High`. The immutable artifact was not
rewritten. No production fix, commit, or push.

## Next single action

Trace `USER_DEMO_5` from scene start to its first divergence, recording those
state fields and testing the Talon-Trot correlation without assuming it is
causal. `USER_DEMO_1` is the second target. No implementation change belongs
to this checkpoint.

P08 remains `ACTIVE / MANUAL_VALIDATION_PENDING`; P09 remains
`PENDING / NOT_FORMALLY_OPEN`.

## USER_DEMO_5 p60 mode-state arbitration — 2026-08-23

Independent reconstruction proves `func_80112C3C` reads the integer selector
at `*(node+0x60)+0x8`: mode 0 bypasses to the incoming yaw; modes 1/2/5 call
`800F1738`; modes 3/4 call `800F1DCC`. The paired capture shows the first
semantic p60+8 value split at glidx 23 (`RT=5`, `AN=0`); glidx 24 is the first
split that changes the returned yaw. The immediate static writer boundary is
`func_80112D08`, store PC `0x80112D14`, called by `func_80112A84` at
`0x80112C20`. Dynamic first-write provenance remains pending because RT64
could not create a Vulkan presentation window in the current headless run.
No production or stable-spec change.

## USER_DEMO_5 `func_801111D0` D90 call-presence boundary — 2026-08-23

Static control flow fixes the exact predicate to
`(80112E84(a0)==1) || (8011349C(a0,&sp+0x40)!=0)`, with branches at
`0x80111230` and `0x80111270` before `jal 80112D90` at `0x80111294`.
At glidx 23 `p60+0xC` is zero before the function in both runtimes; RT takes
the second gate and calls D90 with `a1=0x43780C20`, `a2=0x42340000`, `a3=1`,
while Oracle skips it. The raw pad record `(2,1,2002)` matches, but the
`8011349C` return was not captured. Classification:
`B-801111D0-PREDICATE-INPUT`, call presence proven, helper provenance pending;
this is not a direct C_RIGHT test. No fix.

## USER_DEMO_5 `0x80097B94` semantic-action arbitration — 2026-08-23

`0x80097B94` is a thin `C_RIGHT` wrapper (`a1=0xD`, `a2=1`) over the
published `0x800979D8` helper. Oracle joins the Gemini record
`pad=(2,1,2002)`, `stage=0/sub=5`, glidx 23 and records
`prev_state=0`, BCE84 `ring=0/raw=0/masked=0`, final result `0`. RT64's focused
window also returns `0`, but its raw pad is `(-13,-15,0xF1F1)`, so it is not
the same event. The Gemini `RT=2/AN=0` split is therefore not proven; no root
move or production fix is authorized. See the immutable artifact named in the
companion USER_DEMO_5 evidence note.

## USER_DEMO_5 `0x80097B70` / action-`0xA` arbitration — 2026-08-24

`0x80097B70` is the camera-left wrapper for action `0xA` (`C_LEFT`) and input
id `0`, calling the shared `0x800979D8` helper. Its caller branches on the
return at `0x800A4928` and calls `0x80110EFC(node, 0)` at `0x800A4930`.

The paired artifact joins the visible record `(stage=0, sub=5, map=296,
pad=(2,1,2002))` at glidx 22/23/24. The earlier paired run has RT64
`0/1/1` versus Oracle `0/0/0`; thus RT64 alone activates `0x80110EFC` at the
target glidx 23/24 in that run. Oracle's record-23 state is
`prev=0, ring=1, raw=0xF3, masked=1, bakey=1`, with input 0 disabled before
the helper. The earlier RT64 state had `bakey=1` and input 0 enabled. A fresh
RT64 run with the same visible record produced `ring=0, raw=0x72C, masked=0,
bakey=0` and returned 0, so the pad does not determine the result by itself.

Classification: `B-ACTION-0A-HISTORY-STATE`. The published helper fix covers
the shared implementation contract; the upstream writer of the divergent
history/enabled state is still unresolved. No production change.

## Corrected USER_DEMO_5 camera re-anchor (2026-08-24)

The correctly paired replay re-anchors the first camera-control divergence at
record 24 MATCH -> record 25 DIFF, not record 23. The Talon Trot input-0 enable
transition and action-`0xA` result are matches in both runtimes:
`0xFFFFFFBE -> 0xFFFFFFBF`, bit 0 set, and `0x80097B70` returns 1. Therefore
the earlier `0x80097B94 RT=2/AN=0`, `RT97B70=1/AN=0`, RT-only
`0x80110EFC`/`0x801111D0`/`0x800979D0`,
and action-`0xA` causal claims are superseded; `B-TROT-ENABLE-RECOVERY` is not
the root.

The authoritative camera sample is `selected_record+0x1C`, selected through
`0x80127708[ps+0x184]`. Static `func_800CA628` identifies its store PC as
`0x800CA65C`, from source2[1]. Record 24 remains `293.5294` /
`0x4392C3C2`; record 25 is RT64 `293.0478` versus Oracle `293.4005` /
`0x4392B342`. Oracle's writer event is captured; RT64's direct writer/raw bits
are not, so the join is incomplete and classification is
`B-CAMERA-WRITER-OR-SOURCE-INCOMPLETE-JOIN`. The old record-23 p60 split is
not promoted; p60 must be rechecked only on the record-24->25 path. Static
camera-chain semantics remain valid, but causal role is unknown.

Artifact: `/tmp/p08-demo5-record25-camera-firstdiff-20260824T024500Z/`.
`P08 ACTIVE / MANUAL_VALIDATION_PENDING`; `P09 PENDING / NOT_FORMALLY OPEN`.
No production change.

### Independent map296 join and first camera/CA628 boundary (2026-08-24)

The new map296 artifact independently passes the raw-record gate: RT=28, Oracle=28, raw6=28/28, one epoch, stage=0/sub=5/map=296, epoch-entry mode=5, and zero duplicate record keys. D_8012D5B0+0x0 is the demo base, +0x8/D_8012D5B8 is the post-consume counter, and +0xC/D_8012D5BC is the count; camera rec_ord is post-consume context and is not the logical record ordinal. func_800151BC is the proven logic-tick boundary.

The claimed R24 call sequence is not reproduced: EFA4C is RT=66 versus AN=73, FE14 and CA628 are 1/1 and 2/2, and A9318 is absent from both raw event streams. Static 0x800A9318 is a separate pass, not a proven exit of 0x800A8E9C; CAMERA_UPDATE_BOUNDARY_VERDICT=REFUTED and R24_CALL_SEQUENCE_MATCH=NO.

The first comparable camera event is logical record 0 (capture rec_ord=1 at the same logic tick). CA628 call 1 has matching old state 0x4324BF6C, but source bits differ (RT=0x4324B542, AN=0x4324B132). The current boundary is B-FIRST-CA628-SOURCE; R24 old/source are already divergent and CA628 is downstream. The old FE34/10970/EFA4C and node+0x3C causal claims stay CORRECTED/SUPERSEDED.

Derived artifact: /tmp/p08-demo5-first-camera-diff-from-joined-run-20260824T074500Z/ (d1139defdfa6903041cef31fd2e4db98ca328bd964e702a4e1dca6683a7cff3f). No production change.

### Independent first-writer arbitration

The newer direct CA628 capture supersedes the earlier “RT64 raw writer
missing” wording. The first record-25 CA628 event is RT64 ordinal `10329`
(caller RA `0x8010FE24`): old field `0x4392861E`, source2[1]
`0x43922BE8`, new field `0x43922BE8`. Oracle's paired event is ordinal `103`:
old `0x4392B342`, source `0x4392A91A`, new `0x4392A91A`. Thus CA628 is the
first observed writer but Case B downstream; its RT64 old field and source are
already divergent on entry. `CA8B4` only copies the selected record's
`+0x18/+0x1C/+0x20`, and E1A58 preserves the captured heading. The causal
pre-CA628 writer remains unobserved and is not promoted.

### Camera heading field identity correction (2026-08-24)

The latest `/tmp/p08-demo5-node34-write-watch-20260824T040749Z/` artifact is
not a node+0x34 watch. Its decisive store `func_800EFA4C:0x800EFA5C` writes
`stack_angles+0x4`; the source is already divergent at `active_node+0x3C`,
loaded by `func_8010FE34:0x8010FEA8`. `active_node+0x34` is not part of this
FE34→10970→EFA4C→FE14→CA628 path, and `selected_record+0x1C` remains only the
downstream copy target.

Static enumeration found the direct camera-field writer candidate
`func_8010FDEC:0x8010FDFC`, called at `0x808004A0` by
`plcamera_entrypoint_13`. No semantic effective-address watch captured that
writer for the paired record-24 event. The correct boundary is therefore
`B-NODE3C-TEMPORAL-BOUNDARY-CORRECTED`; do not promote EFA4C, FE34, FE14, CA8B4,
or E1A58 as the node+0x3C producer.

### USER_DEMO_5 EFA4C heading provenance (2026-08-24)

EFA4C writes `stack_angles+0x4` from incoming `a2`/`f14`. FE34 is a consumer
load if the `0x29` path runs. FDEC is a static `+0x3C` writer but
`REFUTED_FOR_TARGET`. The new recapture did not reproduce the Luna EFA4C
heading bits and did not execute 10970/FE34. `CASE E - CORRECTED/SUPERSEDED`.
Do not reintroduce node3C claims from that failed join.

### Record24 camera-path conflict arbitration (2026-08-24)

The provenance artifact is **DISPUTED/INCOMPLETE**, not a replacement join:
its `sub=5` window contains two RT remapped-`glidx=24` slices, the required
record/context and paired call-order tables are absent, and Oracle has no
matching FC38/FE14/CA628 semantic events. `FC38 -> FE14 -> CA628` is therefore
not proven as one event. The old FE34/10970/EFA4C claim remains
`CORRECTED/SUPERSEDED`; current camera classification is `JOIN-UNRESOLVED`,
with `CONFLICT_WINNER: NEITHER — NEW CAPTURE REQUIRED`.

### Independent map296 join and first camera/CA628 boundary (2026-08-24)

The new map296 artifact independently passes the raw-record gate: RT=28, Oracle=28, raw6=28/28, one epoch, stage=0/sub=5/map=296, epoch-entry mode=5, and zero duplicate record keys. D_8012D5B0+0x0 is the demo base, +0x8/D_8012D5B8 is the post-consume counter, and +0xC/D_8012D5BC is the count; camera rec_ord is post-consume context and is not the logical record ordinal. func_800151BC is the proven logic-tick boundary.

The claimed R24 call sequence is not reproduced: EFA4C is RT=66 versus AN=73, FE14 and CA628 are 1/1 and 2/2, and A9318 is absent from both raw event streams. Static 0x800A9318 is a separate pass, not a proven exit of 0x800A8E9C; CAMERA_UPDATE_BOUNDARY_VERDICT=REFUTED and R24_CALL_SEQUENCE_MATCH=NO.

The first comparable camera event is logical record 0 (capture rec_ord=1 at the same logic tick). CA628 call 1 has matching old state 0x4324BF6C, but source bits differ (RT=0x4324B542, AN=0x4324B132). The current boundary is B-FIRST-CA628-SOURCE; R24 old/source are already divergent and CA628 is downstream. The old FE34/10970/EFA4C and node+0x3C causal claims stay CORRECTED/SUPERSEDED.

Derived artifact: /tmp/p08-demo5-first-camera-diff-from-joined-run-20260824T074500Z/ (d1139defdfa6903041cef31fd2e4db98ca328bd964e702a4e1dca6683a7cff3f). No production change.

### USER_DEMO_5 Record 0 CA628 source provenance & writer trace (2026-08-24)

- **Artifact Directory:** `/tmp/p08-demo5-r0-ca628-source-firstdiff-20260824T144859Z/`
- **Replay State:** `stage=0, sub=5, map=296, mode=5`, 28/28 bit-perfect record join.
- **Map Entry Checkpoint:** `sub+0x84 = 0x43240367` (164.013290), `node+0x50 = 0x3f3c04a3` (0.734446), `node+0x58 = 0x3f800000` (1.000000) bit-identical in BOTH runtimes. Refutes Case E (Map Init).
- **First Divergent Writer:** `func_80116218` at `0x80116380` (`swc1 $f18, 0x4($s1)`).
- **Target Memory Address:** `$sp + 0x2C` (`0x8004567C`, caller stack frame of `func_80110588`).
- **Target Lifetime:** `STACK` (ephemeral stack-allocated vector `sp+0x28`).
- **Values Stored ($f18):** RT64: `0x4324b542` (164.708038) vs Angrylion: `0x4324b132` (164.692169).
- **Downstream Consumer:** `func_800CA628` Call 1 at `0x8010FE1C / 0x8010FE24` copies `src2[1]` (`*(sp+0x2C)`) into `dst+0x1C` (old value matched `0x4324bf6c`).
- **Classification:** `Case A — Writer input diff / upstream angular math evaluation in func_80116218 / func_80116120`.
- **Operational Boundary:** `B-R0-CA628-SOURCE-WRITER-PROVEN`.

### Independent pre-0x80116380 arbitration: first cam_sub+0x84 transition (2026-08-24)

The preceding `CA628` boundary is retained only as a downstream operational boundary and is superseded as the root. The first observed transition is `func_800EE7F8:0x800EE804`, writing `cam_sub+0x84` from `0x80045638`: RT `0x4324036A`, AN `0x4324036D`. Its immediate caller is not captured.

- **CURRENT_STATUS:** `P08 ACTIVE / MANUAL_VALIDATION_PENDING`; `P09 NOT FORMALLY OPEN`.
- **CURRENT_ROOT_BOUNDARY:** `B-R0-FIRST-CAMSUB84-WRITER-SOURCE` — `func_800EE7F8:0x800EE804`; caller unknown.
- **LATEST_PROVEN_FACTS:** joined 28/28 records are collision-free; `func_80110588` entry base `0x43240367` and node+`0x50` `0x3F3C04A3` match; the earlier persistent write diverges; `0x80110634` already copies the differing bits to stack `sp+0x2C`; node+`0x50` first differs at `0x801161E0`; `0x80116380` and `CA628` are downstream.
- **REFUTED_OR_SUPERSEDED_CLAIMS:** Gemini's `80116380` root and the prior `B-R0-CA628-SOURCE-WRITER-PROVEN` root. Earlier FE34/10970/EFA4C/node+3C roots remain superseded. Map 296: Hailfire Peaks — Icy Side / `MAP_128_HP_ICY_SIDE`; Mayahem is corrected, while the current TOML lacks a human-readable table.
- **UNRESOLVED_QUESTIONS:** exact nested caller; conditional `0x800893E0` execution/value; complete `func_80116120` inputs.
- **ARTIFACT_PATHS:** `/tmp/p08-demo5-r0-pre-80116380-firstdiff-20260824T082500Z/` plus the joined/source raw directories named above.
- **ARTIFACT_MANIFEST_SHA256:** `6c793196c0457e162bcf6ebd1ff8ed2f4e764f1a72d544cf09232edd0148c29b`.
- **WORKTREE_STATE:** intentionally dirty and preserved; documentation-only changes this run; target code diff none.
- **CODE/BUILD/TEST_STATUS:** no production fix; no build/test; `git diff --check` pending.
- **WHAT_WAS_DONE_THIS_RUN:** independent first-transition arbitration and immutable evidence checkpoint.
- **WHAT_REMAINS_PENDING:** exact caller of the persistent `EE7F8` write.
- **NEXT_SINGLE_ACTION:** paired Record 0 capture with RA/callsite, `f71`, and post-write stack checkpoint only.
- **NEXT_RECOMMENDED_MODEL:** GPT-5.6 Luna High / Terra equivalent.

## USER_DEMO_5 `800EE780 → 800EEBF0 src_dir[1]` arbitration (2026-08-24)

- **PROJECT_PHASE:** P08; **P08_STATUS:** `ACTIVE / MANUAL_VALIDATION_PENDING`; **P09_STATUS:** `PENDING / NOT_FORMALLY OPEN`.
- **CURRENT_HEAD:** `25595a2eb8241ef7ca4aa135e6681e44abcf8598`.
- **WORKTREE_STATUS:** intentionally dirty/preserved; docs-only change; no production or generated-function edit.
- **CURRENT_ROOT_CLASS:** `B-800EEBF0-INPUT` — `PROVEN`.
- **CURRENT_ROOT_BOUNDARY:** `func_800EEBF0` consumes `sp+0x34` (`src_dir[0] = pitch`, `src_dir[1] = yaw`) and radius `a2`, producing `sp+0x54`. At glidx 24, its `src_dir[1]` (yaw) is the first divergent input (RT: `0x4391E610` vs AN: `0x43927D8B`), causing its Cartesian output `sp+0x54` to diverge. `func_800EE780` is an elementwise vector adder (`sp+0x48 = sp+0x28 + sp+0x54`) whose input A is bit-identical and whose output divergence is strictly inherited from `sp+0x54`.
- **LAST_PROVEN_MATCH:** glidx 23 (Control): `800EEBF0` and `800EE780` are bit-identical across all inputs and outputs between RT64 and ANGRYLION.
- **FIRST_PROVEN_DIFF:** glidx 24 (Target): `800EEBF0` input pitch and scale match, input yaw `src_dir[1]` differs (RT: `0x4391E610` vs AN: `0x43927D8B`). `800EE780` input A matches (`0x45992390, 0x441E58B4, 0x4570492C`), input B differs (from `800EEBF0`).
- **LATEST_PROVEN_CAUSAL_CHAIN:** `func_800EEBF0(input_yaw DIFF)` → `sp+0x54 (DIFF)` → `func_800EE780(a1 MATCH, a2 DIFF)` → `sp+0x48 (DIFF)`.
- **REFUTED_OR_SUPERSEDED_CHAINS:** bounded USER_DEMO_4 and older generic movement/camera roots stay superseded.
- **LATEST_ARTIFACT / CAPTURE_DIR:** `/tmp/p08-demo5-eebf0-ee780-20260824T182550Z/`.
  - **MANIFEST_SHA:** `1c6c3313b1c9d657f294a0b089e993730c0ab0b2120b899417e0eba96b8508f5`.
  - **SHA256_VERIFY:** PASS (verified twice; read-only `0444`/`0555`).
- **CURRENT_UNKNOWNS:** upstream evaluation producing `sp+0x38` yaw sum at `ncbapivot_entrypoint_3` line 1430 (`add.s $f16, $f10, $f0`).
- **PENDING_WORK:** GPT-5.6 Luna High arbitration of the new immutable paired capture before advancing the causal chain.
- **NEXT_SINGLE_ACTION:** GPT-5.6 Luna High independently arbitrates the new immutable paired `func_80112D90` call-boundary capture (`/tmp/p08-demo5-80112d90-call-boundary-20260825T040600Z/`) where the call presence difference (`RT64=YES` vs `ANGRYLION=NO`) is proven to originate upstream at branch `0x800A4928` (`beq $v0, $zero`) due to `func_80097B70` returning `1` in RT64 vs `0` in ANGRYLION.
- **NEXT_RECOMMENDED_MODEL:** GPT-5.6 Luna High.
- **FIX_IMPLEMENTED:** NO. **COMMIT:** NONE. **PUSHED:** NO.

## Current arbitration checkpoint — USER_DEMO_5 func_80112D90 Call Presence Boundary glidx 22->23

**PROJECT_PHASE:** P08. **P08_STATUS:** ACTIVE. **P09_STATUS:** PENDING / NOT FORMALLY OPEN.

The current artifact is `/tmp/p08-demo5-80112d90-call-boundary-20260825T040600Z/`; all manifest entries verify, with **MANIFEST_SHA** `6d1ec2322e9b9022b5600a3c54e74bc2bb09c6454af2773fe91793ad63bc7610`.

Raw ordinals are local counters (`RT64`: wrapper trace namespace; `ANGRYLION_CONTROL`: per-helper hook namespace), hence **RAW_ORDINALS_CROSS_RUNTIME_COMPARABLE=NO**. The semantic join is **PROVEN** using normalized `(stage=0, sub=5, map=296, mode=5, glidx, calling_region=func_800A4878, controlling_branch=0x800A4928, candidate_callee=func_80112D90)`; both runtimes match across the glidx 22->23 transition.

**LAST_COMMON_CONTROL_FLOW_EVENT:** glidx 22 (where `func_80097B70` returns `0` in both runtimes, and neither calls `func_80110EFC`/`func_801111D0`/`func_80112D90`; `p60+0xC = 0`).
**FIRST_DIFFERENT_CONTROL_INPUT:** Return value `$v0` of `func_80097B70` (`bainput_should_rotate_camera_left`) at glidx 23: RT64 `$v0 = 1` vs ANGRYLION `$v0 = 0`.
**FIRST_DIFFERENT_BRANCH:** `0x800A4928: beq $v0, $zero, L_800A4940` (NOT TAKEN in RT64 -> invokes `func_80110EFC` -> `func_80110E00` -> `func_801111D0` -> `func_80112D90`; TAKEN in ANGRYLION -> bypasses camera rotation dispatch).
**CALL_PRESENCE:**
- RT64 (glidx 23): `YES` (executed at seq 2565 with `$a0=node`, `$a1=0x43780C20`, `$a2=0x42340000`, `$a3=1`, `5th=2`).
- ANGRYLION (glidx 23): `NO` (`NOT_CALLED / N/A`).
**P60+0xC STATE UPDATE:**
- RT64: `OLD_P60_C = 0` -> `func_80112D90` writes `SOURCE_A3 = 1` at `0x80112E2C` -> `NEW_P60_C = 1`.
- ANGRYLION: `BEFORE = 0` -> no call -> `AFTER = 0`.
**SUBSEQUENT_CONSUMPTION:**
- RT64: reads `p60+0xC = 1` at `0x80112A9C` -> branch `0x80112AA0` NOT TAKEN -> prepares active mode 5 bundle (`sp+0x6C=5`, `sp+0x68=target`, `sp+0x64=step`).
- ANGRYLION: reads `p60+0xC = 0` at `0x80112A9C` -> branch `0x80112AA0` TAKEN -> selects default zero bundle (`sp+0x6C=0`, `sp+0x68=0`, `sp+0x64=0`).
**ARGUMENT_BUNDLE_DOWNSTREAM_CONFIRMED:** `YES`.

**CURRENT_ROOT_CLASS:** `B-ROTATE-CAMERA-LEFT-INPUT` / `B-P60-C-PREDICATE-STATE`.
**CURRENT_ROOT_BOUNDARY:** Branch decision at `0x800A4928` based on `func_80097B70` return value `$v0` (and upstream `func_800979D8` button evaluation).

**NEXT_SINGLE_ACTION:** GPT-5.6 Luna High causal arbitration of the `func_80112D90` call-boundary artifact. **NEXT_RECOMMENDED_MODEL:** GPT-5.6 Luna High for causal arbitration. **FIX_IMPLEMENTED:** NO. **COMMIT:** NONE. **PUSHED:** NO.

## Superseding input-helper correction (2026-08-25)

Artifact `/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/` verifies with
manifest `650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd`
and provenance `NEW_RAW_CAPTURE`. Primary logs correct the earlier
`held RT=1 / AN=0` claim: at joined glidx 23 both have `bakey_held=1` and
enabled bit 0 `1`; `800979D8` itself returns `RT=1` versus `AN=0`.

`80097B70` forwards the result. `bakey_held` reads `self+0x40`, the key
previous-state byte, and the key state pointer, then calls `800BCE84`; the
complete AN nested mapped-byte/table read-set is not captured. Thus Case B is
current, `800BCE84` is not promoted, and **CURRENT_ROOT_BOUNDARY** is the
`800979D8` return at glidx 23. **NEXT_SINGLE_ACTION:** one dual-runtime
capture through `bakey_held` and `800BCE84`, stopping at `800BCE84`.
**NEXT_RECOMMENDED_MODEL:** Gemini 3.7 Flash High. No production change.

## Superseding narrow arbitration — `800979D8` post-`isEnabled` body (2026-08-25)

`/tmp/p08-demo5-80097b70-800979d8-20260825T170358Z/` has manifest
`650d68632e1bfca8ec94f5d1e7d500242fcb9717951f4e568061d951d538b9cd` and
passes all checksums. The corrected target is `bakey_held RT=1/AN=1`,
enabled bit 0 `RT=1/AN=1`, and final `800979D8 RT=1/AN=0`.

## Canonical Checkpoint — Exact MIPS Recovery & Static Implementation Mismatch in `func_800979D8` (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/` (manifest SHA `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`, dual `sha256sum -c` PASS).

1. **Exact MIPS Recovered:**
   - Body `0x800979D8 .. 0x80097A4C` fully disassembled.
   - `0x800979FC` calls `bainput_isEnabled(self, input_id)` (`0x80098024`), which reads `*(input + 0x0) & (1 << input_id)`.
   - When held == 1 and enabled != 0, `0x80097A04: bnez` branches to `0x80097A18`, which executes `0x80097A1C: jal 0x80097978`.
   - `func_80097978` reads and returns `*(input + 0x4) & (1 << input_id)`!
   - Unheld path (`0x80097A2C`): calls `func_80097990(self, input_id, 1)` and returns `$v0 = 0`.

2. **Divergence Mechanism:**
   - Original MIPS: `func_80097978` reads `*(input + 0x4) = 0xFFFFFFFE` -> bit 0 is 0 -> returns `$v0 = 0`.
   - RT64 host replacement (`tooie_bainput_func_800979D8` in `src/game/boot_hw_stubs.cpp`): incorrectly calls `bainput_isEnabled` (reading `*(input + 0x0) = 0xFFFFFFBF` -> bit 0 is 1) a second time -> returns `ctx->r2 = 1`.

3. **Classification:**
   - `STATIC_IMPLEMENTATION_MISMATCH_800979D8`: **PROVEN**.
   - `CURRENT_ROOT_CLASS`: `B-800979D8-STATIC-HOST-IMPLEMENTATION-MISMATCH`.
   - `CURRENT_ROOT_BOUNDARY`: Callee selection in `func_800979D8` at `0x80097A1C` (`func_80097978` on `input+0x4` vs `bainput_isEnabled` on `input+0x0`).
   - `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High independently arbitrate the exact original-vs-host semantic mismatch before any production fix.
   - `NEXT_RECOMMENDED_MODEL`: GPT-5.6 Luna High. No fix, commit, or push.

### Corrected/Superseded — `800979D8` bug versus current `input+0x4` root (2026-08-25)

The valid artifact is `/tmp/p08-demo5-800979d8-post-enabled-20260825T184847Z/`
with manifest `4c75f328cfaa978174f373884472395cf708eeaf413f096f1808af8c913d705c`;
all `SHA256SUMS` entries pass. Original MIPS `800979D8..80097A4C` and the
post-`isEnabled` body are complete and proven.

The host mismatch is real: original `80097990` writes `input+0x4` and
`80097978` reads it at `80097984`; host substitutes `bainput_enable` and a
second `bainput_isEnabled`, respectively. `STATIC_IMPLEMENTATION_MISMATCH_800979D8`
remains **PROVEN** and should eventually be fixed.

It is not the current parity root. At glidx23, `input+0=ffffffbf` matches,
but `input+4=ffffffff` (RT64, bit0=1) versus `fffffffe` (Angrylion, bit0=0)
differs. The metadata/summary complete-input match claim is
**CORRECTED/SUPERSEDED** to **NO**. glidx22 `input+4=ffffffff` in both is the
last proven match; glidx23 is the first proven difference.

Corrected-host counterfactual: RT64 would return `1`, original Angrylion
returns `0`; `COUNTERFACTUAL_RETURN_MATCH=NO` and
`WOULD_FIXING_800979D8_ALONE_RESTORE_GLIDX23_PARITY=NO`.

`PRODUCTION_CORRECTNESS_BUG`: proven host replacement mismatch.
`CURRENT_PARITY_CAUSAL_ROOT`: divergent `input+0x4` active/triggered bit.
`CURRENT_ROOT_CLASS`: `B-BAINPUT-ACTIVE-MASK-INPUT-PLUS4-STATE`.
`CURRENT_ROOT_BOUNDARY`: bit0 for input 0 immediately before original
`80097A1C -> 80097978` / read PC `80097984`.

`FIX_800979D8_NOW=NO`. Next single action: statically enumerate immediate
writers/callsites, starting with `80097990`, then plan one RT64/Angrylion
capture of old/new word, source, writer PC, callsite, call presence, and
semantic occurrence. No new instrumentation or upstream walk in this round.
Next model: **Gemini 3.7 Flash High**; DeepSeek V4 Flash for mechanical
candidate enumeration; Grok 4.6 Medium only for store-order ambiguity.

## Canonical Checkpoint — `input+0x4` First Temporal Writer & Host `800979D8` Causal Proof (2026-08-25)

Artifact `/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/` is valid;
`NEW_RAW_CAPTURE`, manifest SHA is `3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`,
all 10 `SHA256SUMS` entries pass (dual verification).

1. **First Temporal Writer Event Identified:**
   - At `glidx 22`, occurrence `func_800979D8(self, button_id=10, input_id=0)`:
     - `held == 1` and `enabled == 0` in BOTH runtimes.
     - `*(input + 0x4)` is `0xFFFFFFFF` (bit 0 = 1) in BOTH runtimes before call.
     - **AN (Original MIPS):** Follows `0x80097A04` branch fallthrough and executes `0x80097A10: jal func_80097990(self, 0, 0)`. `func_80097990` executes CLEAR store at `0x800979CC`, transforming `*(input + 0x4)` from `0xFFFFFFFF` (bit 0 = 1) to `0xFFFFFFFE` (bit 0 = 0).
     - **RT (Host C++ Replacement):** `tooie_bainput_func_800979D8` calls `bainput_enable(self, 0, 0)` at `0x8009817C` (modifying `input + 0x0`) instead of `func_80097990` (modifying `input + 0x4`). Therefore, `*(input + 0x4)` in RT64 is never modified and remains `0xFFFFFFFF` (bit 0 = 1).
   - This single host defect at `glidx 22` is the exact causal producer of the `input+0x4` state divergence.

2. **Downstream Target Divergence at `glidx 23`:**
   - At `glidx 23`, `bainput_enable(self, 0, 1)` updates `*(input + 0x0)` to `0xFFFFFFBF` in both runtimes.
   - When `func_800979D8(self, button_id=10, input_id=0)` is called:
     - `held == 1`, `enabled == 1`.
     - **AN:** Executes `0x80097A1C: jal func_80097978(self, 0)`, reads `*(input + 0x4) = 0xFFFFFFFE` (bit 0 = 0) -> returns `$v0 = 0`. Controlling branch `0x800A4928` branches over camera rotation left (`p60+0xC = 0`).
     - **RT:** Host calls `bainput_isEnabled(self, 0)` on `*(input + 0x0) = 0xFFFFFFBF` (bit 0 = 1) -> returns `ctx->r2 = 1`. Controlling branch `0x800A4928` falls through to `func_80112D90`, setting `p60+0xC = 1` (unwanted camera rotation left).

3. **Classification & Verification:**
   - `FIRST_INPUT_PLUS4_DIFFERING_EVENT`: `glidx 22`, `func_800979D8(self, btn=10, in_id=0)`.
   - `FIRST_INPUT_PLUS4_WRITER_RT`: `NO_WRITE_OBSERVED` (host called `bainput_enable` on `input+0`, omitting write to `input+4`).
   - `FIRST_INPUT_PLUS4_WRITER_AN`: `func_80097990` at `0x800979CC` (CLEAR bit 0).
   - `FIRST_INPUT_PLUS4_WRITER_EVENT_MATCH`: `ABSENT_IN_ONE_RUNTIME`.
   - `FIRST_INPUT_PLUS4_OLD_MATCH`: `YES` (`0xFFFFFFFF`, bit0=1).
   - `FIRST_INPUT_PLUS4_SOURCE_OPERATION_MATCH`: `NO`.
   - `FIRST_INPUT_PLUS4_NEW_MATCH`: `NO` (`0xFFFFFFFE` [bit0=0] vs `0xFFFFFFFF` [bit0=1]).
   - `HOST_800979D8_MISMATCH_CAUSES_INPUT_PLUS4_FIRST_DIFF`: **`PROVEN`**.
   - `CURRENT_ROOT_CLASS`: `B-800979D8-HOST-REPLACEMENT-WRONG-SIDE-EFFECT`.
   - `BAINPUT_INIT_EXECUTES_IN_WINDOW_RT`: `NO`.
   - `BAINPUT_INIT_EXECUTES_IN_WINDOW_AN`: `NO`.
   - `NEXT_SINGLE_ACTION`: GPT-5.6 Luna High arbitrate the first input+4 divergence and the causal proof that the host 800979D8 mismatch produces it.
   - `NEXT_RECOMMENDED_MODEL`: GPT-5.6 Luna High. No fix, commit, or push.

## Final corrected root — USER_DEMO_5 `input+4` (2026-08-25)

**CORRECTED/SUPERSEDES:** The new raw artifact
`/tmp/p08-demo5-inputplus4-first-writer-20260825T220500Z/` verifies with
manifest SHA `3acb82b7a27b9dfa90da7a782e0618caebd6ef9daa574340261007703417de46`.
The semantic glidx22 join is **PROVEN** (`stage=0/sub=5/map=296/mode=5`,
player self, button 10, input 0), and both calls are present. Held/enabled
are `1/1` and `0/0`; pre-write `input+4` is `ffffffff/ffffffff`.

The first difference is produced within the event: AN calls
`func_80097990(self,0,0)` and clears to `fffffffe`; RT calls
`bainput_enable(self,0,0)` on `input+0` and leaves `input+4=ffffffff`.
Thus `PRE_WRITE_STATE_GLIDX22` matches, `POST_WRITE_STATE_GLIDX22` differs,
and `HOST_800979D8_MISMATCH_CAUSES_INPUT_PLUS4_FIRST_DIFF=PROVEN`.
Correct raw PCs are CLEAR `0x800979CC`, SET `0x800979B0`; the internal
callsite is `0x80097A10`, delay slot `0x80097A14`, and RA `0x80097A18`.
At glidx23 the original `func_80097978` reads `input+4` and returns 0,
while RT's second `bainput_isEnabled` reads `input+0` and returns 1;
`80097B70` and `0x800A4928` are downstream consequences.

`CURRENT_ROOT_CLASS=B-800979D8-HOST-REPLACEMENT-WRONG-SIDE-EFFECT`.
`ROOT_CAUSE_FOUND=YES`; `FIX_IMPLEMENTED=NO`. The write-helper and
read-helper substitutions are both proven and require one replacement fix;
a query-only fix is insufficient. A full faithful replacement is expected
to restore the chain, but this checkpoint is not a runtime PASS. Next:
implement the minimal semantic fix, run dual-runtime A/B parity validation,
then run broader attract regressions.

## Post-Fix Validation Checkpoint — `tooie_bainput_func_800979D8` Faithful Fix (2026-08-25)

Artifact `/tmp/p08-demo5-800979d8-fix-validation-20260825T235800Z/` is valid;
`NEW_RAW_CAPTURE_POST_FIX`, manifest SHA is `9a977b6e1c4c276dd2b6616008723d4b3c7139b9d1c4bd6c519c2f23ae3373da`,
all 16 `SHA256SUMS` entries pass (dual verification).

- **`ROOT_CAUSE_FOUND:`** `YES`
- **`ROOT_CAUSE_FIX_VALIDATED:`** `YES`
- **`FIX_IMPLEMENTED:`** `YES`
- **`FIX_SCOPE:`** `src/game/boot_hw_stubs.cpp` (`tooie_bainput_func_80097990` on `input+4`, `tooie_bainput_func_80097978` on `input+4`, and corrected `tooie_bainput_func_800979D8`)
- **`HOST_WRITE_HELPER_FIXED:`** `YES`
- **`HOST_READ_HELPER_FIXED:`** `YES`

**Validation Results (100% Parity with ANGRYLION_CONTROL):**
- `GLIDX21_CONTROL_PARITY:` `PASS` (held=0, enabled=1, input+4=0xFFFFFFFF, v0=0)
- `GLIDX22_HELD_PARITY:` `PASS` (held=1/1)
- `GLIDX22_ENABLED_PARITY:` `PASS` (enabled=0/0)
- `GLIDX22_INPUT4_OLD_PARITY:` `PASS` (0xFFFFFFFF/0xFFFFFFFF)
- `GLIDX22_OPERATION_PARITY:` `PASS` (func_80097990 CLEAR store at 0x800979CC)
- `GLIDX22_INPUT4_NEW_PARITY:` `PASS` (0xFFFFFFFE/0xFFFFFFFE)
- `GLIDX23_INPUT4_PARITY:` `PASS` (0xFFFFFFFE/0xFFFFFFFE)
- `GLIDX23_800979D8_RETURN_PARITY:` `PASS` (v0=0/0)
- `GLIDX23_80097B70_RETURN_PARITY:` `PASS` (v0=0/0)
- `GLIDX23_800A4928_PARITY:` `PASS` (TAKEN/TAKEN, skipping camera rotation left)
- `GLIDX23_80112D90_CALL_PARITY:` `PASS` (NO/NO, camera rotation left completely collapsed)
- `GLIDX23_P60C_PARITY:` `PASS` (p60+0xC = 0/0)
- `POST_FIX_EEBF0_PARITY:` `PASS` (yaw=270.0 in both runtimes)
- `POST_FIX_EE780_PARITY:` `PASS` (a3=0 in glidx 21..23, 3fa21681 in glidx 24, 3fe2ec4e in glidx 25)
- `BTN13_NEARBY_REGRESSION:` `PASS` (input+4 bit1 SET, v0=0 in both runtimes)

**Status & Next Steps:**
- The entire USER_DEMO_5 locomotion/camera divergence chain is **RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX**.
- P08 remains **ACTIVE / MANUAL_VALIDATION_PENDING** pending broader physical validation gates.
- `NEXT_SINGLE_ACTION:` GPT-5.6 Luna High independently arbitrate the post-fix validation artifact and determine the remaining P08 work.
- `NEXT_RECOMMENDED_MODEL:` GPT-5.6 Luna High. No commit/push.

## Arbitragem final pós-fix P08 / USER_DEMO_5 — 2026-08-26 (GPT-5.6 Luna High)

Este bloco **CORRIGE/SUPERSEDE** o checkpoint anterior sem remover evidência
histórica.

Artifact: `/tmp/p08-demo5-800979d8-fix-validation-20260825T235800Z/`;
`MANIFEST_SHA=9a977b6e1c4c276dd2b6616008723d4b3c7139b9d1c4bd6c519c2f23ae3373da`;
`SHA256SUMS`: 16/16 `OK`.

`RT_RAW_PROVENANCE: NEW_RAW_CAPTURE_POST_FIX`;
`AN_RAW_PROVENANCE: REFERENCE_ORACLE_CAPTURE_REUSED`;
`REFERENCE_ORACLE_REUSE_VALID: YES`: `oracle.log` coincide byte a byte com a
captura Oracle de referência, tomada antes do fix e após a instrumentação.

O fix host preserva a semântica MIPS de `800979D8` (SET/CLEAR/query em
`input+4`, enabled em `input+0`, shift low-5-bit e retorno sign-extended).
`USER_DEMO5_TARGET_CHAIN_PARITY: PROVEN` somente para o chain focal até
`yaw/EEBF0/EE780`; `GLOBAL_GAME_PARITY: NOT_PROVEN`.

Gates do chain são `DIRECTLY_SUPPORTED` para estados/operações input+4,
retornos, branch, p60+0xC, yaw, EEBF0/EE780 e btn13; `DERIVED_BUT_PROVEN` para
a junção causal e ausência do dispatch de rotação esquerda; `UNSUPPORTED` para
parity global, long playthrough e Windows/controle físico.

Status corrigido: `P08_STATUS: ACTIVE / MANUAL_VALIDATION_PENDING`;
`P09_STATUS: PENDING / NOT_FORMALLY_OPEN`;
`USER_DEMO5_STATUS: RESOLVED_BY_800979D8_HOST_REPLACEMENT_FIX`.
`AUDIO_STARTUP_TICK: P08_RESIDUAL`; `WIDESCREEN_EDGE_GEOMETRY: UPSTREAM_ENGINE_SCENE_OR_CAMERA_GEOMETRY_BOUNDARY`
([[p08-widescreen-edge-geometry]]; RT64 wide raster/projection proven faithful; user CAB70/E3DC0 bypass visual `NO_CHANGE`); `CHARACTER_EDGE_GRAPHICS_GLITCH: P08_RESIDUAL`;
`LONG_PLAYTHROUGH_VALIDATION: MANUAL_VALIDATION_PENDING`;
`WINDOWS_GAMEPAD_VALIDATION: BLOCKING_P08 / MANUAL_VALIDATION_PENDING`.

Remaining P08: upstream guest scene/camera world generation audit; checklist
Windows/controle físico; long playthrough; triagem independente de
áudio/character-edge. Fix focal `800979D8`
`CURRENT_FIX_READY_FOR_COMMIT: YES`. Widescreen:
`FIX_IMPLEMENTED: NO`. Nenhum commit/push.

`NEXT_SINGLE_ACTION: Grok 4.6 Medium audit upstream guest engine scene/chunk dispatch and camera parameters for Banjo-Tooie widescreen world generation.`

## P08 display-list identity discriminator ready (2026-08-26)

The upstream-vs-downstream discriminator is prepared without reopening the
refuted CAB70/E3DC0/CACEC visual roots. `GUEST_DISPLAY_LIST_IDENTITY_CAPTURE`
logs root DL identity at GraphicsTask `0x80014148`, direct nested `Gfx.w1`,
monotonic `DL_EMISSION_SEQ`, root/parent/depth, and RT64 HLE/RSP acceptance.
Node/child, draw ordinal, screen bounds, and extra-band classification remain
explicitly unknown/unjoined.

`DL_IDENTITY_CAPTURE_READY: YES`; `BUILD_RESULT: PASS`;
`DL_ADDRESS_SEMANTICS: PROVEN`; `NODE_TO_DL_JOIN: NO`;
`DL_TO_RSP_JOIN: YES`; `RSP_TO_RT64_WORKLOAD_JOIN: PARTIAL`;
`DL_TO_RT64_DRAW_JOIN: NO`; `END_TO_END_JOIN: PARTIAL`;
`PER_FRAME_ACTIVE_DL_SET_READY: YES`; `DL_FIRST_LAST_SEEN_READY: YES`.
The reference smoke completed 720 frames with 3,892 guest DL events. Current
root is still `UNKNOWN / RE-ANCHOR_REQUIRED`, boundary `NOT_PROMOTED`;
`FIX_IMPLEMENTED: NO`; default runtime behavior unchanged.

Artifact: `/tmp/p08-widescreen-dl-identity-setup-20260826T070058Z/`;
`SHA256SUMS` SHA `3b9e63bcc0954a9b451045c7d78508176a893c10386ab1984f3ac399febf3390`.
Next single action is the user's single-trigger capture at the visible
residual, followed by Grok 4.6 Medium arbitration of guest absence versus
downstream presence.

### FOCAL_BRANCH_TIMELINE setup (2026-08-26)

Added diagnostic-only capture for the map-section selection boundary through
caller-frame-reconstructed `800E3E8C → 800E3D0C → 800CAA24` wrappers. Original returns
and stable identity `section_ptr + section_index + section_entry_ptr` are
joined to DL `capture_id`/`guest_frame`. `CAA24` returns `1` for pass and `0`
for separating-plane rejection; `D_8012D500` remains a camera/viewport-volume
pointer with exact writer/update cadence `UNKNOWN`.

Build PASS. The 720-frame launcher smoke emitted 3,881 DL events but no
map-section branch events, so no causal claim is made. Artifact:
`/tmp/p08-widescreen-focal-branch-timeline-setup-20260826T164317Z/`;
`SHA256SUMS` SHA
`f010ac8a1a33e97494a0d6d5dcb90a237bc8814399c4a01ba1328f21c9de81e3`.
`FIX_IMPLEMENTED: NO`; next action is one live user capture.
