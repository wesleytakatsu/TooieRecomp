---
title: "P09 visible-feather alpha/coverage/texture forensic"
description: "Plume G_AC_THRESHOLD/G_TT_RGBA16 and ST are closed as local mismatch; they do not cover crest and have no movement-only material driver."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-08
tags: [p09, kazooie, feather, alpha, coverage, texture, G_AC_THRESHOLD, G_TT_RGBA16]
---

# P09 visible-feather alpha/coverage/texture forensic

Bounded objective: decide whether alpha compare, coverage, or
texture-coordinate/texture state can make the Kazooie plumes look
geometrically distorted during movement after position, culling, depth,
and draw-temporal association are closed. Static RT64 source plus
existing V3 / Q captures and `STATIC_ROM` DLs only. No game. No new
instrumentation. No production fix. No GPU readback or added
synchronization. No commit. No push. Do not reopen a position writer,
culling, depth pairing, or GameFrame association.

Authoritative closed results remain
[[p09-bounded-crest-post-handoff-transferability-audit]] (position
path `CLOSED`),
[[p09-visible-feather-culling-winding-face-visibility-forensic]]
(culling status `A`), and
[[p09-post-position-draw-temporal-and-depth-state-forensic]]
(draw-temporal and depth `CLOSED`).

Checkout: Tooie `ff5b6d9` dirty. BanjoRecomp `ec85963`. RT64
`6f1c2d99` protected diagnostics. Asset `/tmp/asset_062B.bin`
(78200 bytes). Captures used offline:

- V3: `/tmp/tooie-kazooie-feather-v7-cpu-handoff-status.txt` +
  `/tmp/tooie-kazooie-feather-v7-cpu-handoff.csv`
- Q: `/tmp/tooie-kazooie-feather-postprocess-q-v1-status.txt`

The V7 CSV has no `otherMode` / `blendColor` / tile-hash columns.

```text
ALPHA_COVERAGE_STATUS: CLOSED
TEXTURE_COORDINATE_STATUS: CLOSED
TEXTURE_RESOURCE_ASSOCIATION_STATUS: CLOSED
PRODUCTION_FIX_CANDIDATE_FOUND: NO
NEW_CAPTURE_REQUIRED: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```

## 1. Material/pixel state (`PROVEN` `STATIC_ROM`)

All twelve plume DLs share one command skeleton (own vs join only
changes `G_VTX`/`G_POPMTX`/`G_TRI*`):

```text
GEO and=0xC0FFFB (clears FOG|LIGHTING|TEXGEN|TEXGEN_LINEAR|LOD|SHADING_SMOOTH|TEXTURE_ENABLE)
GEO or =G_SHADE|G_SHADING_SMOOTH
G_TEXTURE on=1 tile=0 s=t=0x8000
SETTIMG RGBA16 0x02001420          // TLUT
SETTILE tile6 tmem=256            // LOADTLUT dest
LOADTLUT pal16 (lrs=0x3C)
SETOTHERMODE_H sft=16 size=2 data=0x8000  // G_TT_RGBA16 at bit 14
SETTIMG CI 0x02001440             // load as 16b words
SETTILE tile7 CI/16b LOADBLOCK
LOADBLOCK 512 texels dxt=512      // 64x32 CI4
SETTILE tile0 CI/4b line=4 tmem=0 pal=0 cms=cmt=CLAMP masks=6 maskt=5 shifts=0
SETTILESIZE 64x32 (uls/ult=0 lrs=252 lrt=124)
SETCOMBINE FC129804 3F15FFFF      // RGB (TEXEL0-PRIM)*ENV+PRIM; A TEXEL0*SHADE
SETOTHERMODE_L sft=30 size=2 data=1       // G_AC_THRESHOLD at bit 0
G_DL dest=0x03000040
G_VTX / (join: POPMTX + parent G_VTX) / G_TRI*
SETOTHERMODE_H restore 0          // G_TT_NONE
SETOTHERMODE_L restore 0          // G_AC_NONE
ENDDL
```

F3DEX2 `setOtherModeH/L`: `off = 32 - sft - size`. TEXTLUT
`sft=16,size=2` → `off=14`, data `0x8000` = `G_TT_RGBA16`. Alpha
compare `sft=30,size=2` → `off=0`, data `1` = `G_AC_THRESHOLD`.

Crest `0x4A6`/`0x4B0` and head `0x48F`:

```text
same GEO pair
G_TEXTURE on=0 s=t=0x8000
SETCOMBINE FC62FE04 3F15F9FF      // RGB (1-PRIM)*ENV+PRIM; A SHADE
G_DL dest=0x03000010
G_VTX / tris
no SETTIMG / SETTILE / LOADTLUT / LOADBLOCK / SETOTHERMODE
```

Torso `0xBC`: CI4 16x16 from `0x02000F80`/`0x02000FA0`, `G_TT_RGBA16`
during the textured batch, **no** `G_AC_THRESHOLD`, then `G_TEXTURE on=0`
and the same untextured combine as head/crest.

None of the 16 DLs contain `G_SETBLENDCOLOR`, `G_SETPRIMCOLOR`,
`G_SETENVCOLOR`, `G_SETPRIMDEPTH`, `G_MODIFYVTX`, `G_LOADTILE`, or
`G_RDPSETOTHERMODE`. A scan of Gfx from `model+0x2670` to EOF found
**zero** `G_SETBLENDCOLOR`.

```text
RIGHT_EFFECTIVE_MATERIAL_STATE:
  tex on=1 scale=0x8000/0x8000 tile0
  CI4 64x32 + pal16 RGBA16 TLUT (local load)
  G_TT_RGBA16 during tris, restored
  combine (TEXEL0-PRIM)*ENV+PRIM / TEXEL0*SHADE
  G_AC_THRESHOLD during tris, restored
  blend/prim/env/cycle/filter/persp/cvg/blender: inherited, not set
LEFT_EFFECTIVE_MATERIAL_STATE: identical command class; same images
CREST_EFFECTIVE_MATERIAL_STATE:
  tex on=0; no local tile/TMEM/TLUT; no SOM
  combine (1-PRIM)*ENV+PRIM / SHADE
  nested 0x03000010 before G_VTX (segment 3 RUNTIME_REQUIRED)
HEAD_CONTROL_EFFECTIVE_MATERIAL_STATE: same class as crest
BODY_CONTROL_EFFECTIVE_MATERIAL_STATE:
  CI4 16x16 + pal16, G_TT_RGBA16, no G_AC_THRESHOLD
  then untextured SHADE batch like head
```

`G_ZBUFFER` / cull bits: unchanged from the depth/culling forensics.

## 2. `G_AC_THRESHOLD` semantics (`PROVEN` source)

```text
G_AC_THRESHOLD_DECODE_PATH:
  F3DEX2 G_SETOTHERMODE_L 0xE2
    -> GBI_F3DEX2::setOtherModeL (size=p0(0,8)+1, off=32-sft-size)
    -> RSP::setOtherModeL mask/or into otherMode.L
    -> RDP::setOtherMode
    -> DrawAttribute::OtherMode -> loadDrawState copies DrawCall.otherMode
    -> RasterPS otherMode.alphaCompare()
RT64_ALPHA_COMPARE_IMPLEMENTATION:
  RasterPS.hlsl: combiner alpha vs blendColor.a; return false discards
ALPHA_COMPARE_THRESHOLD_VALUE_SOURCE: instanceRDPParams.blendColor.a
ALPHA_COMPARE_CAN_DISCARD_PIXEL: YES (shader)
ALPHA_COMPARE_APPLIES_ONLY_TO_PLUMES: YES among the 16 DLs
```

Default RDP `blendColor` is `{0,0,0,0}`. Compare is
`alphaCompareValue < blendColor.a`. If `blendColor.a==0`, the test never
discards. No `0x62B` Gfx sets blend color; the live threshold is
whatever a **caller** GameCall left in the RDP stack. That value is not
in V7/Q CSVs.

This is **not** promoted to `RUNTIME_VALUE_REQUIRED` as a next witness:
even a typical Rare cutout (`blendColor.a≈8/255` against 1-bit TLUT
alpha) is binary transparency, does not rewrite position, and does not
run on crest/head.

Coverage: `resultCvg = (8/cvgRange) * (cvgXAlpha ? combiner.a : 1)`.
Plumes patch only AC bits 0–1 of otherMode.L; `cvgXAlpha` /
`alphaCvgSel` / blender stay inherited and are therefore shared with
later/earlier body draws on the same otherMode.

## 3. `G_TT_RGBA16` (`PROVEN` `STATIC_ROM` + source)

```text
G_TT_RGBA16_ROLE: TLUT decode of CI indices as RGBA16 (not alpha compare)
PLUME_TEXTURE_USES_CI_FORMAT: YES (sample tile CI/4b; load tile CI/16b)
TLUT_FORMAT: RGBA16 pal16 (LOADTLUT lrs=0x3C)
TLUT_SOURCE: SETTIMG 0x02001420 then LOADTLUT tile6 tmem=256
TLUT_IDENTITY_OR_HASH: model+0x1420 if seg2==model; sha1-16 d2b85cb3f900dcf1
  (32 bytes). Seg2 base is the usual model texture bank (STRONG_INFERENCE).
RIGHT_LEFT_SHARE_TLUT: YES (same 0x02001420)
TLUT_STATE_CAN_BE_STALE_OR_MISASSOCIATED: NO for the 12 plume DLs —
  they LOADTLUT in-DL before triangles. Nested 0x03000040 could overwrite
  TMEM; V6C produced 0 triangles from that call; its bytes remain
  RUNTIME_REQUIRED and are not a movement driver if the setup DL is static.
```

Torso uses a **different** pair `0x02000F80`/`0x02000FA0`. Crest/head
do not load a TLUT.

## 4. ST provenance (`PROVEN` source + `STATIC_ROM`)

```text
PLUME_ST_DATAFLOW:
  ROM Vtx.tc (s10.5) at G_VTX seg1
    -> RSP::setVertices: tc = (s * textureState.sc) / (65536*32)
       (texgen off because GEO cleared G_LIGHTING|G_TEXTURE_GEN)
    -> Workload.tcFloats
    -> RSPProcessCS: tc -= tcVel*(1-curFrameWeight); tcVel=0 on G_VTX
       (lookAt ENABLED bit is 0, so computeTextureGen is skipped)
    -> RasterVS oUV = iUV (no extra transform)
    -> RasterPS ddx/ddy + tile shift/clamp/mask -> sampleTexture
PLUME_HAS_G_MODIFYVTX_ST: NO
ST_CAN_CHANGE_AFTER_G_VTX: NO in the 14 DLs
ST_CAN_CHANGE_FRAME_TO_FRAME_FOR_SAME_VERTEX: NO
  (static ROM * static 0x8000 scale; modify ST absent; texgen off; vel 0)
```

```text
RIGHT_HAS_MODIFY_ST: NO
LEFT_HAS_MODIFY_ST: NO
CREST_HAS_MODIFY_ST: NO
```

`G_MODIFYVTX` is absent on all 14 plus `0x48F`/`0xBC`. Nested
`0x03000040` had zero tracked triangles (V6C); hypothesized
`Gfx[0]+0x40` leftover-tri mapping remains `REFUTED` as the live
target.

## 5. Texture identity (`PROVEN` `STATIC_ROM`)

```text
RIGHT_TEXTURE_IDENTITY: CI4 64x32 0x02001440 + pal16 0x02001420
LEFT_TEXTURE_IDENTITY: same
CREST_TEXTURE_IDENTITY: none locally; G_TEXTURE on=0; combiner ignores TEXEL0
RIGHT_LEFT_SHARE_TEXTURE: YES
TEXTURE_IS_STATIC: YES
TEXTURE_OR_TLUT_CONTENT_CHANGES_DURING_ANIMATION: NO in these DLs
ANIMATED_TEXTURE_PATH_PRESENT: NO
```

If `seg2==model`, CI4 bytes start at `model+0x1440`
(sha1-16 `6e12584f734922cb` over 1024 bytes). No per-frame SETTIMG
rewrite.

## 6. Tile / TMEM lifetime (`PROVEN` `STATIC_ROM` + source)

```text
PLUME_TILE_STATE_SET_LOCALLY: YES
PLUME_TMEM_CONTENT_SET_LOCALLY: YES
PLUME_TLUT_SET_LOCALLY: YES
PLUME_RELIES_ON_INHERITED_TEXTURE_STATE: NO for tiles/TMEM/TLUT
  (nested 0x03000040 is the only possible later mutator)
STATE_RESTORED_AFTER_PLUME: YES for G_TT and G_AC only; tiles/TMEM not restored
CAN_RIGHT_LEFT_OR_CREST_SEE_DIFFERENT_STALE_TMEM_STATE: NO among the 12
  (each reloads the same images). Crest does not sample that CI4.
```

`loadDrawState` copies `rdp->tiles[]` and `otherMode.textLUT()` into
the **current** Workload `callTiles`/`rdpTiles` at the same moment as
the GameCall. `tmemHashOrID` is hashed from that TMEM against
`workload.submissionFrame`.

```text
TEXTURE_STATE_OWNED_BY_SAME_GAMECALL: YES
TEXTURE_RESOURCE_CAN_REFER_TO_DIFFERENT_WORKLOAD_GENERATION: NO
  for the image hash stored on the call. GameFrame tile matching lerps
  tile descriptors only when tmemHashOrID matches; identical static
  tiles lerp to themselves.
MATERIAL_STATE_CAN_BE_STALE_WHILE_GEOMETRY_IS_CURRENT: NO
  (OtherMode/Combine/tiles snapshotted onto the same DrawCall)
TEXTURE_DESCRIPTOR_RING_OR_CACHE_LIFETIME_SAFE: YES
  (hash + submissionFrame; no added sync; same barrier as geometry)
```

## 7. Pixel shader / movement drivers (`PROVEN` source)

Plume RasterPS path is the ordinary textured raster variant:
sample CI via TLUT, combiner, optional AC_THRESHOLD, coverage, blender.

```text
PLUME_PIXEL_SHADER_VARIANT: RasterPS textured, G_AC_THRESHOLD, G_TT_RGBA16
ALPHA_VALUE_FORMULA: combiner A = TEXEL0.a * SHADE.a (cycle-0 muxes)
ALPHA_TEST_OR_DISCARD_LOCATION: RasterPS after combiner
  (AC_THRESHOLD vs blendColor.a; also coverage < 1/cvgRange)
COVERAGE_FORMULA: (8/cvgRange)*(cvgXAlpha ? combiner.a : 1)
BLEND_FORMULA: Blender::run(inherited otherMode.L blender bits)
ALPHA_RESULT_DEPENDS_ON_SCREEN_POSITION_OR_DERIVATIVES:
  NO for the AC compare itself
  YES for LOD/filter footprint (ddx/ddy of UV) and coverage rasterization
```

LOD only if inherited `G_TL_LOD`. Plumes do not set textLOD. Filter /
perspective bits are also inherited (TEXTLUT is the only H field they
patch).

```text
ALPHA_TEXTURE_MOVEMENT_DRIVER_TABLE:
  perspective-correct ST interpolation | WEAK | ST static; UV swims on
    a moving mesh as hardware perspective; also hits any textured mesh
  texture LOD / derivatives            | WEAK | only if G_TL_LOD inherited;
    shared otherMode.H except TEXTLUT
  sample footprint / filter            | WEAK | edge shimmer, not broken bones
  AC_THRESHOLD at cutout edges         | WEAK | plume-only; binary TLUT alpha;
    does not cover crest; blendColor.a may be 0 (no-op)
  animated TLUT/texture                | REFUTED | static ROM, in-DL load
  stale material association           | REFUTED | same GameCall snapshot
  coverage / sample mask               | WEAK | inherited cvg bits; not plume-local
  G_MODIFYVTX ST                       | REFUTED | absent
  texgen ST                            | REFUTED | GEO clears LIGHTING|TEXGEN
```

## 8. Can this look geometric? (`STRONG_INFERENCE`)

```text
ALPHA_CUTOUT_CAN_MIMIC_GEOMETRY_DISTORTION: YES as holes/flicker at
  silhouette and CI transparent texels; NOT as articulated-bone
  misplacement. Signature: sparkling missing pixels, stable mesh silhouette.
TEXTURE_COORDINATE_CORRUPTION_CAN_MIMIC_GEOMETRY_DISTORTION: YES as
  swimming/sliding texture that reads as a bent plume. Signature: UVs
  moving on an otherwise rigid triangle. Not observed as a pipeline
  write: ST is static.
TLUT_CORRUPTION_CAN_MIMIC_GEOMETRY_DISTORTION: NO as geometry. Signature:
  wrong colors / whole-texel transparency. Shared right/left; crest unused.
```

## 9. Right / left / crest discrimination (`PROVEN` `STATIC_ROM`)

```text
ALPHA_THRESHOLD_HYPOTHESIS_COVERS_RIGHT: YES (mechanism present)
ALPHA_THRESHOLD_HYPOTHESIS_COVERS_LEFT: YES
ALPHA_THRESHOLD_HYPOTHESIS_COVERS_CREST: NO
HEAD_USES_ALPHA_THRESHOLD: NO
BODY_USES_ALPHA_THRESHOLD: NO
HEAD_USES_SAME_TEXTURE_FORMAT: NO (head untextured combine)
BODY_USES_SAME_TEXTURE_FORMAT: PARTIAL (body CI4 but 16x16 other image, no AC)
PLUME_HAS_UNIQUE_ALPHA_TEXTURE_PATH: YES
```

Crest and head use `G_TEXTURE on=0` and a SHADE-alpha combine that does
not read TEXEL0. A plume-only AC/CI4 bug cannot move or hide crest
triangles. Prior visual-scope work treats crest as part of the
user-visible “head feathers” set
([[p09-visual-geometry-identity-and-scope-forensic]]). That is a
**limitation of the AC/TT hypothesis as a 14-DL root**, not a hardware
refutation of plume cutout.

## 10. ST ranges (`PROVEN` `STATIC_ROM`)

Tile 64×32, s10.5 → one wrap is S∈[0,2048], T∈[0,1024]. `masks=6`
`maskt=5` match those sizes. `shifts=shiftt=0`. `cms=cmt=G_TX_CLAMP`.

Own-mesh examples: `0x534` S=[-10496,1536] T=[-5,4097]. Authored ST
leaves the 64×32 rectangle; clamp+mask is the N64 rule, not a decoder
mismatch.

Join tables contain the same class of large authored ST (and large
object-space Y). Do not reopen those as a position bug (V7 closed
handoff of the same Vtx bytes).

```text
RIGHT_ST_RANGE: authored s10.5; own 0x534 S=[-10496,1536] T=[-5,4097]
LEFT_ST_RANGE: authored; own 0x5C0 S=[-9792,1552] T=[-26620,4614]
EXPECTED_TEXTURE_COORD_RANGE: 64x32 tile, s10.5 wrap 2048 x 1024
OUT_OF_RANGE_ST_OBSERVED: YES vs one tile (authored clamp/wrap)
TILE_SHIFT_OR_MASK_MISMATCH: NO (mask 6/5, shift 0, size 64x32)
LEFT_RIGHT_ST_SYMMETRY: same tile/TLUT/combine; distinct vertex banks
```

## 11. Existing captures

V7/Q close **geometry** association, not material fields.

```text
EXISTING_CAPTURES_CAN_CLOSE_DRAW_TEMPORAL_ASSOCIATION: YES (prior)
EXISTING_CAPTURES_CAN_CLOSE_MATERIAL_ASSOCIATION: PARTIAL
  GameCall exclusivity implies one DrawCall.otherMode per target DL;
  they do not record blendColor.a or tmemHashOrID
GENERATION_MISMATCH_COUNT: 0 (geometry; unchanged)
DRAW_RANGE_OWNERSHIP_MISMATCH_COUNT: 0
FRAME_SELECTION_MISMATCH_COUNT: 0
```

## 12. Closure

No RT64 implementation defect was isolated. Plume AC/TT is real
localization, but it is the **wrong shape** for a movement-only
14-DL geometric root: static images, static ST, crest excluded,
threshold possibly a no-op.

```text
ALPHA_COVERAGE_STATUS: CLOSED
TEXTURE_COORDINATE_STATUS: CLOSED
TEXTURE_RESOURCE_ASSOCIATION_STATUS: CLOSED
CURRENT_BEST_ROOT_CLASS: VISIBLE_FEATHER_POSITION_PIPELINE_CLOSED_ROOT_ELSEWHERE
ROOT_CONFIDENCE: HIGH (this layer); overall visual root remains elsewhere
PRODUCTION_FIX_CANDIDATE_FOUND: NO
BEST_NEXT_WITNESS: none
CAN_BE_CPU_COMMAND_STREAM_ONLY: n/a
EXPECTED_CAPTURE_COUNT: 0
NEXT_VISUAL_ASSUMPTION: frame presentation / compositing / final-image
  association; or the user-visible symptom is not geometric distortion
  of these 14 DLs (cutout/texture misread vs still-unidentified geometry)
```

Do not continue linearly through remaining RDP bits. Do not add an
AC/ST/TLUT witness. Do not implement a production fix. Do not GPU-readback.
