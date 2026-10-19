---
title: "P09 upstream current-bone-transform semantic forensic"
description: "Guest AE820 compose is parent*child XYZW and internally consistent; additive sway is off; clip/sample time for bone 107's keyed steps remains unresolved."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-08
tags: [p09, kazooie, feather, animation, quaternion, AE820, AE8A8, baanim]
---

# P09 upstream current-bone-transform semantic forensic

Bounded objective: decide whether the **current guest-side bone
transforms** for the 14 visible Kazooie feather DLs can be semantically
wrong or discontinuous during movement, now that the downstream
renderer path is closed. Static generated C plus existing producer
captures only. No game. No presentation/compositing. No production
fix. No GPU readback or added synchronization. No commit. No push.
Do not reopen V7 handoff, Q, RSPProcess, culling, draw-temporal,
depth, alpha, or ST.

Authoritative closed downstream results remain
[[p09-bounded-crest-post-handoff-transferability-audit]],
[[p09-visible-feather-culling-winding-face-visibility-forensic]],
[[p09-post-position-draw-temporal-and-depth-state-forensic]],
[[p09-visible-feather-alpha-coverage-texture-forensic]].
V5B `TOOIE_DIAG_FEATHER_NO_INTERP=1` remains
`REFUTED_FOR_VISIBLE_FEATHER_CHAIN` as a primary cause
([[p09-kazooie-feather-v6-post-matrix-vertex-path]]).

Checkout: Tooie `ff5b6d9` dirty. Generated C in `RecompiledFuncs/`
(not hand-edited). Offline captures:

- `/tmp/tooie-kazooie-feather-producer.csv` (18,210 rows; AE8A8+AE820)
- `/tmp/tooie-kazooie-feather-matrix.csv` (bone 103 composed G_MTX only)

0x62B-like window: 991 frames (guest_logic 619–1609) where bone 103
has parent index 25. All eight dumped family bones present every
frame of that window.

```text
CURRENT_MATRIX_GENERATION_STATUS: CLOSED
SECONDARY_SWAY_STATUS: CLOSED
ANIMATION_TIME_SOURCE_STATUS: RUNTIME_VALUE_REQUIRED
PRODUCTION_FIX_CANDIDATE_FOUND: NO
PRESENTATION_AUDIT_NOW_JUSTIFIED: NO
NEW_CAPTURE_REQUIRED: NO (answered: [[p09-animation-clip-transition-and-authored-key-forensic]])
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
```

## 1. Current-matrix generation dataflow (`PROVEN` `STATIC_MIPS`)

```text
baanim_update (0x8008C894)
  __baanim_update_scaleToHorizontalVelocity (0x8008C2A8)  // clip RATE vs |v_xz|
  0x8008C37C
  0x8008ADE4 x3                                          // sample apply
  → animation records stride 0x28 at *animList + boneAnimID*0x28

func_800AE160 / func_800AE1E8
  a2 = AnimMtxList + 0x50
  a3 = optional additive ctx (AE1E8: list+0xC via 0x800E0A28; else 0)
  func_800AE820          // one call; walks every skeleton bone
    func_800AE8A8        // per-bone: parent slot TRS * record TRS
    func_800AEAA0        // composed quat + scale → 3x3
    func_800AEBA8        // rest+anim translation → 4x4; optional 0x800AF2F0
```

AE820 slot: `list+0x50+(index<<6)`. ADE2C / DCF48 / 19AA0 read that
same slot as the current bone matrix and multiply the character
modelview (`0x8012CE58`). That last multiply is draw-time world, not
a second skeletal compose.

| Stage | Input | Output | Space |
| --- | --- | --- | --- |
| `baanim_update` | player clip, horizontal velocity | 0x28 records (quat, scale, T ptr) | animation track |
| `AE8A8` | record + parent slot | composed XYZW quat + parent_s*local_s | skeleton-composed TRS slot |
| `AEAA0` | slot quat+scale | 3x3 = R(composed) with axes * scale | same slot |
| `AEBA8` | rest+anim T, parent 3x3 | tx,ty,tz at +0x30 | skeleton-composed 4x4 |
| `19AA0` | slot 4x4, char modelview | G_MTX operand | world / modelview |

Rotation representation in the 0x28 record and AE8A8 slot: **XYZW**
(X at +0, W at +0xC). Identity parent is `(0,0,0,1)`. Scale is three
floats at +0x10/14/18. Translation is a pointer at record +0x1C,
stored at slot +0x30.

Diagnostic CSV labels `BASE_QW` at record +0. That label is **XYZW
X**, not W. Do not use the labeled `*_ROTATION_Q` fields to test
Hamilton products. Raw `SLOT_W0..W3` + `BASE_*` as XYZW do compose.

## 2. Local vs world composition (`PROVEN`)

```text
LOCAL_MATRIX_FORMULA:
  q_slot = q_parent_slot ⊗ q_record     // Hamilton, XYZW
  s_slot = s_parent_slot * s_record     // componentwise
  R = quat_to_mat3(q_slot); columns/rows scaled by s_slot
  T = parent_linear * (rest + anim translation)   // AEBA8
PARENT_CHILD_COMPOSITION_ORDER: parent * child (q_parent ⊗ q_local)
WORLD_MATRIX_FORMULA: char_modelview * slot_4x4   // func_80019AA0
COMPOSITION_ORDER_CORRECT: YES
```

Offline check on 991 0x62B frames, raw XYZW:

```text
parent_slot ⊗ child_record  vs  child_slot
107→103 / 103→104 / 104→105 / 107→100 / 100→101 / 101→102
mean angular error ≤ 5e-8 rad; max ≤ 1.6e-7 rad
```

Do not invert RT64 world matrices for this test: AE820 already stores
the skeleton-composed 4x4. `inv(parentWorld)*childWorld` equals
`inv(slot_parent)*slot_child` when both share the same 19AA0 parent.

## 3–4. Existing captures and continuity

```text
EXISTING_CAPTURES_HAVE_SUFFICIENT_CURRENT_MATRICES: PARTIAL
EXISTING_DATA_CAN_CLOSE_CURRENT_MATRIX_CONTINUITY: PARTIAL
```

Present: AE8A8/AE820 rows for 79, 100–105, 107 with local quat, composed
scale, and AE820 4x4. Missing: bone **108** rows (crest parent); clip
id; sample time/frame; 0x28 translation vectors as named channels.

0x62B consecutive-frame maxima (990 steps; hemisphere-aware):

| boneAnimID | MAX_LOCAL_ROTATION_STEP | MAX_FINAL_ROTATION_STEP | MAX_LOCAL_SCALE_STEP | MAX_AE820_T_STEP |
| ---: | ---: | ---: | ---: | ---: |
| 107 | 2.0944 rad (120°) | 2.2009 | 0.2429 | 33.1 |
| 103 | 0.0853 | 2.2016 | 0.2429 (inherited) | 74.5 |
| 104 | 1.7359 | 2.5740 | 0.2429 | 53.9 |
| 105 | 0.6525 | 2.8384 | 0.2483 | 73.1 |
| 100 | 0.1234 | 2.2031 | 0.2429 | 73.0 |
| 101 | 1.7404 | 2.5981 | 0.2429 | 48.3 |
| 102 | 0.6292 | 2.8562 | 0.2483 | 68.3 |
| 79 | 0.0283 | 2.2049 | 0.2429 (inherited) | 59.9 |

```text
CURRENT_LOCAL_TRANSFORM_DISCONTINUITY_OBSERVED: YES
```

Isolated clusters at guest_logic **1548–1552** and **1575–1577**.
107 local steps are 0.524 / 0.873 / 1.047 / 2.094 rad (30° / 50° /
60° / 120°) together with the known uniform scale ramp
(~1.018 → 0.141). Plume **roots** 103 and 100 stay locally smooth.
Crest 79 local is nearly identity (948/991 frames). Distal 104/105/101/102
and all FINAL rotations follow 107.

These steps are **finite, unit-length, quantized**. They are not a
singular-matrix or AE8A8 multiply defect. Bug-versus-authored-key is
**not** decided without clip/time (see §7).

## 5. Quaternion path (`PROVEN` generated C + capture)

```text
QUATERNION_NORMALIZATION_CORRECT: YES (records already unit, min 0.999996; AE8A8 does not renormalize)
QUATERNION_COMPONENT_ORDER_CORRECT: YES (XYZW in memory)
QUATERNION_MULTIPLICATION_ORDER_CORRECT: YES (parent ⊗ child)
QUATERNION_SIGN_FLIP_CAN_CHANGE_RESULT: NO
```

`q` and `-q` are the same rotation. AE8A8 does not force a hemisphere.
Diagnostic `QUATERNION_SIGN_FLIPPED=YES` on 2–3 bone-103 AE820 rows is
the matrix-extraction hemisphere, not a guest compose flip.

## 6. Animation sample source

Each feather bone reads `*animList + boneAnimID*0x28` inside the **same**
AE820 walk as the rest of `0x62B`.

```text
BONE79_ANIMATION_SOURCE:  record 79; parent idx 26 / anim 108; local ≈ identity
BONE100_ANIMATION_SOURCE: record 100; parent idx 25 / anim 107
BONE101_ANIMATION_SOURCE: record 101; parent idx 33 / anim 100
BONE102_ANIMATION_SOURCE: record 102; parent idx 34 / anim 101
BONE103_ANIMATION_SOURCE: record 103; parent idx 25 / anim 107; raw scale (1,1,1)
BONE104_ANIMATION_SOURCE: record 104; parent idx 30 / anim 103
BONE105_ANIMATION_SOURCE: record 105; parent idx 31 / anim 104
```

Clip resource and interpolation mode are **not** in the producer CSV
(`ACTIVE_ANIMATION_RESOURCE: UNKNOWN`, same as V2).

## 7. Frame/time input (`HIGH` remaining seam)

```text
CURRENT_ANIMATION_TIME_SOURCE: baanim_update (0x8008C894) before AE820
CURRENT_ANIMATION_FRAME_SOURCE: UNKNOWN (not dumped; 0x8008ADE4 is the apply candidate)
FRAME_TIME_CAN_ALTERNATE_OR_JUMP: UNKNOWN
SAMPLE_TIME_SHARED_WITH_BODY: YES (one AE820 over the whole skeleton, one anim list)
SAMPLE_TIME_SHARED_WITH_OTHER_SECONDARY_BONES: YES at compose; additive path not entered
SECONDARY_BONE_TIME_SOURCE_DIFFERENT: NO
```

`__baanim_update_scaleToHorizontalVelocity` scales **clip playback
rate** from `|v_xz|`. That is movement-dependent time for the **whole
clip**, not a feather-only clock. It is **not** the bone-107 TRS scale
channel.

## 8. Secondary / sway

All 18,210 producer rows: `ADDITIVE_BYTE8=0`, `ADDITIVE_BYTE9=0`.
AE8A8 therefore does not call `func_800AEDA0` or `func_800AF04C`.
No E0B8C immediate for 79 / 100–105 (prior static). V2 already had
zero AF04C on bone 103.

```text
FEATHER_SECONDARY_TRANSFORM_PATH: none reached
SECONDARY_INPUTS: a3+8 / a3+9 both 0
MOVEMENT_DEPENDENT_INPUTS: base clip + bone 107 scale/rotation channels; clip rate vs |v_xz|
SECONDARY_OUTPUT_BONES: none
COMMON_SECONDARY_SWAY_MECHANISM: UNKNOWN (Mumbo/backpack not on this AE820 dump; backpack is E2180)
```

User-visible “sway while moving” can still be **authored** in the
base clip / 107 channels. It is not the bawobble type-3 additive.

## 9. Right vs left

```text
RIGHT_LEFT_LOCAL_TRANSFORM_SYMMETRY_EXPECTED: YES (mirror)
RIGHT_LEFT_TRANSFORM_RELATION: dumped local 4-vector 103↔100 best match neg-Y-and-Z (mean 0.004 rad)
UNEXPECTED_RIGHT_LEFT_ASYMMETRY: NO as a constant bug
```

104↔101 mean 0.012 rad (max 0.57 at the 107 transition cluster).
105↔102 mean 0.015 (max 0.70). Residual is at the same 1548/1576
keys, not a standing left/right compose mismatch.

## 10. Crest

```text
CREST_SHARES_CURRENT_MATRIX_PRODUCER: YES (same AE820/AE8A8)
CREST_SHARES_TEMPORAL_INPUT_SOURCE: YES (same list, same call)
CREST_SHARES_SECONDARY_SWAY_PATH: YES (same unused additive gates)
```

Crest local is almost identity; visible crest motion is parent 108
(not dumped) plus inherited 107-family scale through the head chain.
108→79 cannot be reconstructed as a full 4x4 pair from this CSV.

## 11. Parent 107 scale

```text
PARENT107_SCALE_SOURCE: animation record 107 +0x10/14/18 (uniform); AE8A8 inherits into 100–105 and also into 79's composed slot in this dump
PARENT107_SCALE_INTENDED_TO_AFFECT_BOTH_PLUMES: YES
SCALE_APPLIED_BEFORE_OR_AFTER_CHILD_ROTATION: scale baked into composed slot, then AEAA0 scales R(q_composed)
SCALE_COMPOSITION_ORDER_CORRECT: YES for uniform scale (commutes with R)
SCALE_ORDER_LOCALIZED_DISTORTION_PLAUSIBLE: NO
```

Do not reopen “low determinant = bad”. Range 0.134–1.018 is the V2
uniform scale, positive.

## 12–13. Writers and buffers

```text
BONE79_CURRENT_MATRIX_WRITERS: func_800AE820 (via AE160/AE1E8)
BONES100_105_CURRENT_MATRIX_WRITERS: func_800AE820
MULTIPLE_CURRENT_WRITERS_PER_FRAME: NO (0 multi-writes in 991-frame window)
LATE_OVERWRITE_POSSIBLE: NO on this path (additive not entered)
CURRENT_GUEST_MATRIX_BUFFER: AnimMtxList+0x50+(index<<6)
PREVIOUS_GUEST_MATRIX_BUFFER_IF_ANY: none on compact DCF48 (E2180 dual-mtx is a different class)
CURRENT_SLOT_CAN_CONTAIN_PREVIOUS_SAMPLE: NO (single slot overwritten each AE820)
FRAME_PARITY_OR_DOUBLE_BUFFERING_PRESENT: NO
```

## 14. Body control

```text
BODY_CONTROL_BONE: Kazooie head idx 26 / anim 108 (structurally); not present in producer CSV. Banjo torso uses the same AE820 walk.
BODY_CONTROL_MATRIX_PRODUCER: func_800AE820
BODY_AND_FEATHER_SHARE_BASE_PATH: YES
WHAT_IS_FEATHER_SPECIFIC: boneAnimIDs 79/100–105, parent 107 scale+rotation channels, GeoCmd DLs. Not the compose function.
```

## 16–19. Root decision

Compose, writers, quaternion order, uniform-scale inheritance, and
the unused additive path are closed. The movement-coupled **inputs**
are bone 107's record (scale + large keyed rotations) and whole-clip
`baanim` rate. Whether those inputs are the intended Tooie clip at
the intended time is **not** in the existing dump.

```text
CURRENT_BEST_ROOT_CLASS: GUEST_COMPOSE_CLOSED_ANIMATION_SAMPLE_AUTHORED
ROOT_CONFIDENCE: HIGH for compose/additive; HIGH that 107 keyed steps are authored (see [[p09-animation-clip-transition-and-authored-key-forensic]])
PRODUCTION_FIX_CANDIDATE_FOUND: NO
BEST_NEXT_WITNESS: CPU-only dump at baanim_update / 0x8008ADE4 of clip id, sample time/frame, and 0x28 TRS for 79,100–105,107,108 plus one body bone, same guest_logic tick as AE8A8
WHY_DECISIVE: separates authored 30/60/120° keys from a stale/wrong sample time
CAN_BE_CPU_COMMAND_STREAM_ONLY: YES
EXPECTED_CAPTURE_COUNT: 1
PRESENTATION_AUDIT_NOW_JUSTIFIED: NO
```

Do not implement that witness in this forensic. Do not start
presentation/compositing until the three statuses are all CLOSED.

## Not this forensic

- RT64 interpolation (V5B already `REFUTED` as primary)
- GPU / command-stream after G_MTX
- Frame presentation / compositing
- Reopening position, culling, depth, alpha, ST
