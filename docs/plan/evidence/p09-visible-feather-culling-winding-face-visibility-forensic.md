---
title: "P09 visible-feather culling / winding / face-visibility forensic"
description: "Pre-append triangle rejection is absent on all 14 visible feather DLs; GPU face cull is inherited and body-shared, not a local mismatch."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-07
tags: [p09, kazooie, feather, culling, winding, face-visibility]
---

# P09 visible-feather culling / winding / face-visibility forensic

Bounded objective: decide whether triangle culling, winding, or face
visibility can explain the movement-only Kazooie visible-feather glitch
now that the full position pipeline is `CLOSED`. Static code plus
existing V3 / Q captures only. No game. No new instrumentation. No
production fix. No GPU readback or added synchronization. No commit.
No push. Do not reopen a position writer.

Authoritative closed result remains
[[p09-bounded-crest-post-handoff-transferability-audit]]: V7 VERSION=3
CPU handoff `CORRECT` for all 14 exact targets; Q / RSPProcess /
RSPModify / RasterVS closed for right, left, and crest.

Checkout: Tooie `ff5b6d9` dirty. Asset `/tmp/asset_062B.bin` (78200
bytes). Captures used offline:

- V3: `/tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt` + `.csv`
- shaders / HLE: `BanjoRecomp/lib/rt64/src/hle/rt64_rsp.cpp`,
  `rt64_state.cpp`, `rt64_raster_shader.cpp`,
  `gbi/rt64_gbi_f3dex2.cpp`, `gbi/rt64_gbi_f3dex.cpp`,
  `shaders/RasterVS.hlsl`, `shaders/RSPProcessCS.hlsl`

```text
CULLING_VISIBILITY_STATUS: A
CULLING_VISIBILITY_STATICALLY_CLOSED: YES
TARGET_TRIANGLE_CAN_BE_REJECTED_BEFORE_V7_APPEND: YES (cull-both only)
MISSING_TRIANGLES_OBSERVED: NO
CULLING_ENABLED_FOR_FEATHER_TRIANGLES: INHERITED (not set in the 14 DLs)
PRODUCTION_FIX_CANDIDATE_FOUND: NO
NEW_CAPTURE_REQUIRED: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```

## 1. Triangle submission dataflow (`PROVEN` source)

F3DEX2 Tooie path:

```text
G_TRI1 / G_TRI2
  GBI_F3DEX2::tri1 / tri2
    -> RSP::drawIndexedTri(slotA, slotB, slotC, rawGlobal=false, "TRI1"|"TRI2_*")
      -> V6C hook (pre-cull, pre-append)
      -> if (geometryMode & cullBothMask) == cullBothMask: return
      -> optional projection flush / loadDrawState
      -> if (geometryMode & cullBothMask) == cullFrontMask: swap A<->C
      -> faceIndices.push_back(globalIndices[0..2])
      -> V7 APPEND hook (post-append)
      -> CPU signed-area on posScreen (scissor merge only; does not pop)
      -> drawCall.triangleCount++
```

`FACEINDICES_APPEND_LOCATION`:
`BanjoRecomp/lib/rt64/src/hle/rt64_rsp.cpp` `RSP::drawIndexedTri`
`faceIndices.push_back` (after the cull-both return and the
`G_CULL_FRONT` index swap).

`ALL_PRE_APPEND_EARLY_RETURNS`:

| Reason | Location | Inputs | Depends on position | Depends on geomode | Frame/movement | CPU/GPU |
| --- | --- | --- | --- | --- | --- | --- |
| Copy-cycle assert | `drawIndexedTri` | `otherMode.cycleType` | NO | NO | NO | CPU assert |
| **Cull both** | `geometryMode & cullBothMask == cullBothMask` | geomode | NO | YES | only if geomode changes | CPU return |
| Front-face **swap** (not reject) | `== cullFrontMask` | geomode + slot order | NO | YES | NO | CPU reorder |

No degenerate test before append. No clip-plane test before append.
No signed-area reject before append.

```text
CULLING_OCCURS_BEFORE_FACEINDICES_APPEND: YES (only G_CULL_BOTH)
WINDING_OR_FRONTFACE_LOGIC_OCCURS_BEFORE_APPEND: YES (index swap iff G_CULL_FRONT)
DEGENERATE_TRIANGLE_REJECTION_BEFORE_APPEND: NO
TARGET_TRIANGLE_CAN_BE_REJECTED_BEFORE_V7_APPEND: YES
```

V7 APPEND therefore proves surviving-triangle **identity/handoff**, not
GPU visibility. The only pre-append drop is cull-both.

## 2. Front-face / GPU cull (`PROVEN` source)

```text
RT64_FRONT_FACE_DEFINITION: RenderFrontFace::CLOCKWISE (PSO default)
FRONT_FACE_DECIDED_FROM: GEOMETRY_MODE selects whether GPU culling is on;
  G_CULL_FRONT additionally swaps two indices on the CPU
INDEX_SWAP_OR_FRONTFACE_SWAP_EXISTS: YES
SWAP_LOCATION: drawIndexedTri before faceIndices.push_back
SWAP_OCCURS_BEFORE_OR_AFTER_V7_APPEND: BEFORE
CAN_SWAP_DEPEND_ON_DYNAMIC_TRANSFORM_STATE: NO
```

When any cull bit is set, `State::fullSync` sets `flags.culling` and the
raster PSO uses `RenderCullMode::FRONT` (`rt64_raster_shader.cpp`).
`RasterVS` still only maps screenPos → NDC (Y divide by
`-resolution.y/2`); it has **no** extra position or facing logic.

CPU signed area **after** append, used only to merge the scissor/draw
rect, **not** to drop `faceIndices`:

```text
U = posScreen[i1] - posScreen[i0]
V = posScreen[i2] - posScreen[i0]
N = cross(V, U)
visibleTri = (N.z >= 0)   // if usesCulling
```

```text
SIGNED_AREA_FORMULA: N.z = V.x*U.y - V.y*U.x with U=p1-p0, V=p2-p0
COORDINATE_SPACE: SCREEN (CPU posScreen); GPU cull is clip/NDC after RasterVS
Y_AXIS_SIGN_OR_VIEWPORT_FLIP_ACCOUNTED_FOR: YES in RSPProcess (divide Y by -w)
  and again in RasterVS (divide Y by -res.y/2)
NEAR_ZERO_AREA_HANDLING: none on CPU; visibleTri stays true if culling off
AREA_EPSILON: none
```

## 3. Geometry mode on the 14 DLs (`PROVEN` `STATIC_ROM`)

F3DEX2 `G_GEOMETRYMODE` is `geomode = (geomode & w0_low24) | w1`
(`modifyGeometryMode`). `w0_low24` is `~clear`, not the clear mask.
`G_CULL_FRONT=0x200`, `G_CULL_BACK=0x400`, `G_CULL_BOTH=0x600`.

Every one of the 14 feather DLs, plus body controls `0x48F` and `0xBC`,
starts with the same pair:

```text
G_GEOMETRYMODE and=0xC0FFFB set=0x00000000   // preserves G_CULL_* and G_ZBUFFER
G_GEOMETRYMODE and=0xFFFFFF set=0x00200004   // sets G_SHADE | G_SHADING_SMOOTH
```

`0xC0FFFB` **keeps** bits 9–10 (cull). Contrast: 27 other DLs in the
same `0x62B` asset use `and=0xC0F9FB`, which **clears** `G_CULL_BOTH`.
The whole asset contains **zero** commands that *set* `G_CULL_FRONT` or
`G_CULL_BACK`.

So the 14 DLs do not choose a local cull mode. They keep whatever the
caller/engine left in the RSP stack. RSP `reset()` starts at
`G_CLIPPING` only. P08 map geometry used guest `G_CULL_BACK`
(`STRONG_INFERENCE` for player-model inheritance, not a new proof).

Nested `G_DL dest=0x03000040` (plumes) / `0x03000010` (crest, head,
torso) has no `G_GEOMETRYMODE` and no `G_CULLDL`. Segment-3 base stays
`RUNTIME_REQUIRED`; the Gfx[0] hypothesis also has no cull opcode.

```text
RIGHT_CULL_MODE: INHERITED (DL preserves G_CULL_*; does not set/clear)
LEFT_CULL_MODE: INHERITED (same pair)
CREST_CULL_MODE: INHERITED (same pair)
CULL_MODE_SAME_ACROSS_ALL_14: YES
CULLING_ENABLED_FOR_FEATHER_TRIANGLES: INHERITED (not encoded in the 14 DLs)
BODY_CULL_MODE_SAME_AS_FEATHERS: YES (0x48F and 0xBC use the same pair)
```

No left/right/crest visibility-state asymmetry. Texture/combine and the
nested `0x10` vs `0x40` call differ; cull bits do not.

## 4. G_CULLDL (`PROVEN` source + `STATIC_ROM`)

F3DEX2 maps `0x03` to `GBI_F3DEX::cullDl`, whose body is `// TODO`
(empty). None of the 14 DLs contain `G_CULLDL`. Nested hypothesized
walks also have zero.

```text
TARGET_PATH_HAS_G_CULLDL: NO
CULLDL_INPUT_VERTEX_RANGE: N/A
CULLDL_CAN_REJECT_WHOLE_FEATHER_SUBTREE: NO (unimplemented + absent)
MOVEMENT_ONLY_COMPATIBILITY: a working G_CULLDL would hide a whole DL
  when a vertex range is off-screen; that is not the reported
  movement-only distortion, and RT64 would currently never hide it.
```

## 5. Static triangle count vs V3 APPEND (`PROVEN_FOR_CAPTURE`)

Status `APPEND_EXACT_TARGET_MATCH` is the full (unsampled) count.
`MAX_APPEND_ROWS_PER_TARGET=128` only limits the CSV. All 14 DLs share
the same invocation count **1258** with remainder **0**:

| DL | Region | Kind | STATIC_TRI | EXACT_MATCH | EXACT/STATIC |
| --- | --- | --- | ---: | ---: | ---: |
| `0x534` | RIGHT | OWN | 8 | 10064 | 1258 |
| `0x54C` | RIGHT | JOIN | 4 | 5032 | 1258 |
| `0x564` | RIGHT | JOIN | 4 | 5032 | 1258 |
| `0x57C` | RIGHT | OWN | 4 | 5032 | 1258 |
| `0x592` | RIGHT | JOIN | 4 | 5032 | 1258 |
| `0x5AA` | RIGHT | OWN | 4 | 5032 | 1258 |
| `0x5C0` | LEFT | OWN | 8 | 10064 | 1258 |
| `0x5D8` | LEFT | JOIN | 4 | 5032 | 1258 |
| `0x5F0` | LEFT | JOIN | 4 | 5032 | 1258 |
| `0x608` | LEFT | OWN | 4 | 5032 | 1258 |
| `0x61E` | LEFT | JOIN | 4 | 5032 | 1258 |
| `0x636` | LEFT | OWN | 4 | 5032 | 1258 |
| `0x4A6` | CREST | OWN | 3 | 3774 | 1258 |
| `0x4B0` | CREST | JOIN | 6 | 7548 | 1258 |

CSV rows for generations 828–843 are `static_tri * 16` for every target
(128-row cap binds only `0x534`/`0x5C0` at exactly 128).

```text
STATIC_VS_APPEND_COUNT_COMPLETE: YES
MISSING_TRIANGLES_OBSERVED: NO
```

Cull-both never fired on these 14 DLs across 1258 invocations.
`G_CULL_FRONT` swap, if it occurred, still appends three indices.

## 6. Offline V3 area / determinant (`PROVEN_FOR_CAPTURE` CPU recon)

V3 has P, worldIndex, W (first-of-batch 4x4s), viewProjIndex, Q[4], and
face slots. That is enough for a CPU reconstruction. It is **not** GPU
screenPos and **not** interpolated W'.

World-space triangle `|N|` (cross of transformed edges):

| Region | n | min \|N\| | max \|N\| |
| --- | ---: | ---: | ---: |
| RIGHT | 448 | 52.0 | 1195 |
| LEFT | 448 | 52.0 | 2436 |
| CREST | 144 | 7.81 | 102 |

JOIN `|N|` is larger than OWN in every region (not thinner in world
area). Crest OWN is the smallest, still not near-zero.

Local XY signed area of both plumes is identically 0 because those
vertices share local Y=94 (plane-on-Y; a projection artifact, not a
missing triangle). Crest local XY is nonzero.

Clip/NDC signed area from `Q*W*P` is numerically tiny (1e-19..1e-12)
under this CPU convention; do **not** treat that as GPU degeneracy.
Two right clip-sign mixed identities in 16 generations are consistent
with that noise.

World-Nz facing mixed identities exist (JOIN/articulation): crest 1,
right 4, left 2 of 9/28/28 slot-identities. That is expected geometry
for mixed parent/child joins, not a false-cull proof.

3x3 determinants of captured world matrices for wi 115–116 and
124–139: all **positive** (~0.9999). No sign change across the 16
generations.

```text
V3_CONTAINS_SUFFICIENT_DATA_FOR_CPU_SIGNED_AREA: YES
RIGHT_AREA_MIN_ABS: 52.0 (world |N|; 16 gens)
LEFT_AREA_MIN_ABS: 52.0
CREST_AREA_MIN_ABS: 7.81
RIGHT_SIGN_FLIP_COUNT: 0 reliable (clip recon noise only)
LEFT_SIGN_FLIP_COUNT: 0 reliable
CREST_SIGN_FLIP_COUNT: 0 reliable
NEAR_DEGENERATE_TRIANGLE_COUNT: 0 (world |N|)
FEATHER_AREA_SIGN_CAN_CHANGE_DURING_ARTICULATION: YES (JOIN mixed W; world-Nz)
IS_SIGN_CHANGE_EXPECTED_GEOMETRY_OR_POSSIBLE_FALSE_CULL: expected geometry;
  cannot reject before V7 append; GPU FRONT cull would be shared with body
NEGATIVE_DETERMINANT_TARGET_MATRIX_OBSERVED: NO
DETERMINANT_SIGN_CHANGES_DURING_ANIMATION: NO
CULL_LOGIC_ACCOUNTS_FOR_TRANSFORM_HANDEDNESS: NOT_APPLICABLE (no negative det)
JOIN_GEOMETRY_MORE_WINDING_SENSITIVE: YES (structural; mixed worldIndex)
OWN_TRIANGLE_CULL_RISK: low (same W, det>0, world |N|>=52 plume / 7.8 crest)
JOIN_TRIANGLE_CULL_RISK: higher facing change, not pre-append drop
FEATHER_TRIANGLES_MORE_AREA_SENSITIVE_THAN_BODY: UNKNOWN (body not in V3 APPEND)
```

## 7. Plausibility (`STRONG_INFERENCE` + `PROVEN`)

Separate mechanisms:

```text
FACE_CULL_PLAUSIBILITY: WEAK
  GPU FRONT cull only if inherited G_CULL_* is set. Same GEO pair as
  body 0x48F/0xBC. No 14-DL asymmetry. No pre-append loss.
CLIP_REJECTION_PLAUSIBILITY: WEAK
  no CPU clip reject; prior crest clip.w not near-zero
DEGENERATE_REJECTION_PLAUSIBILITY: NONE
  no CPU degenerate test; world |N| not near-zero
WHOLE_DL_CULL_PLAUSIBILITY: NONE
  G_CULLDL absent and unimplemented
```

Movement-only ranking (high → low), given the actual symptom:

1. **not this layer** — winding sign flip as a *bug* (body-shared cull,
   no pre-append drop, det>0)
2. near-degenerate area threshold — no CPU threshold; world areas healthy
3. wrong front/back state — would also hit body; 14 DLs do not set FRONT
4. G_CULLDL — absent
5. clip-plane rejection — not pre-append; not local
6. static missing triangle — `REFUTED` (1258×N exact)

```text
MOVEMENT_ONLY_VISIBILITY_RANKING: static-missing REFUTED; G_CULLDL NONE;
  degenerate NONE; clip WEAK; inherited GPU FRONT cull WEAK as a
  feather-local mismatch; remaining glitch is not a proven cull/winding
  defect
```

## 8. Decision

```text
CULLING_VISIBILITY_STATUS: A
TOP_VISIBILITY_MECHANISM: NONE
CURRENT_BEST_ROOT_CLASS: VISIBLE_FEATHER_POSITION_PIPELINE_CLOSED_ROOT_ELSEWHERE
ROOT_CONFIDENCE: HIGH for pre-append/static cull-winding; GPU inherited
  G_CULL_BACK is STRONG_INFERENCE and body-shared, not a remaining local root
BEST_NEXT_WITNESS: none (status A)
CAN_BE_CPU_COMMAND_STREAM_ONLY: N/A
EXPECTED_CAPTURE_COUNT: 0
NEXT_NON_POSITION_ASSUMPTION: depth / occlusion / draw temporal association
PRODUCTION_FIX_CANDIDATE_FOUND: NO
```

Do not call GPU bug. Do not add another position witness. Do not treat
join 0–4/27–31 as a culling reason: V7 already resolved those vertices
to global/worldIndex, and every issued G_TRI of those DLs reached APPEND.

Status A does **not** prove GPU raster coverage. It proves there is no
credible **mismatch** in the CPU triangle-cull/winding path that V7
observes, and no 14-DL-local cull encoding that would spare the body.

## 9. Next single action

Bounded static / command-stream audit of **depth / occlusion / draw
temporal association** on the 14 visible feather DLs. Do not add a
position witness. Do not GPU-readback. Do not implement in this turn.
