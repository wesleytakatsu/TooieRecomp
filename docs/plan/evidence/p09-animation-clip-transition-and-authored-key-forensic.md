---
title: "P09 animation clip-transition and authored-key forensic"
description: "Sample-time resets are clip changes; clip22 bone107/108 raw TRS match ROM keys; prior 30/120° steps are authored walk↔idle interpolations."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-08
tags: [p09, kazooie, feather, animation, baanim, clip-transition, authored-keys]
---

# P09 animation clip-transition and authored-key forensic

Bounded objective: decide whether the V1 sample-time resets and
bone-107/108 raw TRS steps are correct authored animation or a
clip/sampler/key bug. Static generated C plus ROM assets plus the
existing `P09_FEATHER_ANIM_SAMPLE_TIME_V1` capture. No game. No new
witness. No production fix. No GPU readback or added synchronization.
No commit. No push. Downstream renderer and guest AE8A8 compose stay
closed.

Capture: `/tmp/tooie-feather-anim-sample-time-status.txt` and `.csv`
(`RAW_BAANIM_UPDATES=1451`, `TARGET_SAMPLE_ROWS=384`). Prior producer:
`/tmp/tooie-kazooie-feather-producer.csv` frames 1548–1577.

```text
INTRA_CLIP_SAMPLE_TIME_CONTINUOUS: YES
SAMPLE_TIME_PROGRESSION_BUG_OBSERVED: NO
CLIP_TRANSITION_TIME_RESET_COUNT: 9
CLIP_TRANSITION_TIMING_BUG: REFUTED
CLIP22_BONE107_TRACK_FOUND: YES
BONE107_RAW_RECORD_MATCHES_AUTHORED_ROTATION: YES
BONE107_RAW_RECORD_MATCHES_AUTHORED_SCALE: YES
BONE107_RAW_RECORD_MATCHES_AUTHORED_TRANSLATION: UNKNOWN
BONE107_SCALE_RAMP_AUTHORED: YES
PRIOR_107_LARGE_ANGLE_DISCREPANCY_EXPLAINED: YES
CLIP22_BONE108_TRACK_FOUND: YES
BONE108_LARGE_STEPS_AUTHORED: YES
ANIMATION_SAMPLER_STATUS: C
CURRENT_BEST_ROOT_CLASS: AUTHORED_ANIMATION_BEHAVIOR
ROOT_CONFIDENCE: HIGH (PROVEN for clip22 keys vs raw 0x28; STRONG for transition timing)
EXISTING_CAPTURE_PLUS_STATIC_KEYS_SUFFICIENT: YES
NEW_CAPTURE_REQUIRED: NO
PRODUCTION_FIX_CANDIDATE_FOUND: NO
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

Do not reopen V7 handoff, Q, RSPProcess, culling, depth, draw-temporal,
alpha, ST, or AE8A8 compose. Additive sway remains off on this path.

## 1. Reclassify sample time (`PROVEN` capture counters + CSV)

Status over 1451 `baanim_update` ticks:

| Counter | N |
| --- | ---: |
| SAMPLE_TIME_FORWARD | 1314 |
| SAMPLE_TIME_SAME | 0 |
| SAMPLE_TIME_BACKWARD | 7 |
| SAMPLE_TIME_EXPECTED_WRAP | 129 |
| SAMPLE_TIME_UNEXPECTED_JUMP | 5 |
| CLIP_ID_CHANGE | 9 |

The bounded CSV has seven clip-change ticks. Every negative
`SAMPLE_TIME_DELTA` in that CSV is either an expected same-clip wrap
(`prev≈0.9x → ~0.0x`, playback=2) or a clip-id change. The five
Luna `UNEXPECTED_JUMP` rows are all clip changes.

```text
INTRA_CLIP_SAMPLE_TIME_FORWARD_COUNT: 1312  (status FORWARD minus CSV clip-change FORWARD 624,657)
INTRA_CLIP_SAMPLE_TIME_BACKWARD_COUNT: 0
INTRA_CLIP_SAMPLE_TIME_UNEXPECTED_JUMP_COUNT: 0
CLIP_TRANSITION_TIME_RESET_COUNT: 9
EXPECTED_LOOP_WRAP_COUNT: 129
INTRA_CLIP_SAMPLE_TIME_CONTINUOUS: YES
SAMPLE_TIME_PROGRESSION_BUG_OBSERVED: NO
```

Do not treat a decrease across a clip change as
`ANIMATION_SAMPLE_TIME_BUG`. Normalized time is 0–1 per clip
(`func_800E0414`: `frame = start + t*(end-start)`).

## 2. Clip transitions (`PROVEN` CSV + `baanim_play*` static)

Dump is **after** `baanim_update`, so a start-at-0 clip shows
`t ≈ (1/30)/duration` on the first accepted tick.

| GUEST_LOGIC | OLD_CLIP | NEW_CLIP | TRANSITION_CALLER | TRANSITION_REASON | EXPECTED_FROM_GAME_STATE | NEW_TIME_EXPECTED | ACTUAL_NEW_TIME | STATUS |
| ---: | ---: | ---: | --- | --- | --- | ---: | ---: | --- |
| 624 | 111 | 3 | `baanim_playForDuration_loop` family | idle → short loop | speed 0, pb=2 | 0 then one ADE4 step | 0.116 (dur 0.43; two ticks of clip 3 already in CSV) | expected |
| 626 | 3 | 12 | loop + mode 2 | start walk | spd 212 | 1/30/0.545=0.0612 | 0.06119 | expected |
| 655 | 12 | 111 | `baanim_playForDuration_once` | walk → idle | spd 0, pb=1, dur 5.5 | 1/30/5.5=0.00606 | 0.00606 | expected |
| 657 | 111 | 12 | loop + mode 2 | idle → walk | loc 46012 | one ADE4 step | 0.0536 | expected |
| 691 | 12 | 1 | `..._once` | one-shot | spd 499, pb=1, dur 0.5, loc 40156 | 1/30/0.5=0.06667 | 0.06667 | expected |
| 697 | 1 | 22 | `..._once` | walk-start one-shot | pb=1, dur 1.0 | 1/30/1.0=0.03333 | 0.03333 | expected |
| 714 | 22 | 21 | `..._loop` then `|v_xz|` rate | enter walk loop | pb=2, mode 2 | 1/30/0.569=0.0585 | 0.05853 | expected |

`CLIP_ID_UNEXPECTED_CHANGE=1` is a diagnostic label, not semantic
proof. Two further clip changes after CSV `MAX_ROWS` (`7:18` then
back toward `111` in `CLIP_ID_COUNTS`) are ordinary.

```text
TRANSITION_626_GAMEPLAY_SEMANTICS: short loop (clip 3) → locomotion loop clip 12 at spd 212
TRANSITION_655_GAMEPLAY_SEMANTICS: clip 12 → idle clip 111 (110 frames, 5.5 s, speed 0)
TRANSITION_691_GAMEPLAY_SEMANTICS: clip 12 → 28-frame one-shot clip 1 at high speed / new locomotion
TRANSITION_697_GAMEPLAY_SEMANTICS: clip 1 → 30-frame walk-start one-shot clip 22 (107 scale 0.31→1, 108 nod)
TRANSITION_714_GAMEPLAY_SEMANTICS: clip 22 → looping walk clip 21 (60 frames; `|v_xz|` remaps duration)
CLIP_TRANSITION_TIMING_BUG: REFUTED
```

## 3. Clip-change API (`PROVEN` `STATIC_MIPS`)

```text
CLIP_CHANGE_FUNCTION: baanim_playForDuration_once (0x8008CDB8) / _loop / _onceSmooth / _loopSmooth / *StartingAt
CLIP_ID_WRITE_FUNCTION: anctrl_setIndex 0x8008AED4 (AnimCtrl+0x18 hword); func_8008C1B8 (Animation+0x18)
SAMPLE_TIME_RESET_FUNCTION: func_8008C1C0 (Animation+0x10); anctrl_start 0x8008B064; func_8008C1E4 writes Animation+0x14=0
DEFAULT_NEW_CLIP_START_TIME: 0 (once/loop); first dumped t is one ADE4 step
CAN_NEW_CLIP_INHERIT_OLD_CLIP_TIME: YES via *StartingAt variants; NOT used on the five listed events
CAN_CLIP_CHANGE_RESET_TO_NONZERO_START: YES (StartingAt / func_8008C1C0)
```

`baanim_playForDuration_once`: `anctrl_reset` → `anctrl_setIndex` →
`anctrl_setDuration` → playback=1 → `anctrl_start`.
`8008BD94` loads `func_800D674C(lh Animation+0x18)` so the captured
clip ids **are** cart resource ids (flag `0x16`).

## 4. Animation.apply (`PROVEN` `STATIC_MIPS`)

`clip+0 = 0x8008C4DC` is `__baanim_applyBottlesBonus`, a wrapper that
patches bonus bones then `jalr` a nested pointer. It is **not** the
key sampler.

Per-frame pose fill:

```text
ANIMATION_APPLY_FUNCTION: func_800E0170
FUNCTION_ADDRESS: 0x800E0170
INPUTS: a0=D674C(clip_id) asset, a1=sample_time (Animation+0x10), a2=Animation+0x1E, a3=dest from 8008BD1C
OUTPUT: 0x28 TRS records via func_800E084C (dest + bone*(32+8) = bone*0x28 after +8)
KEY_LOOKUP_FUNCTION: func_800E04C0
TIME_TO_FRAME: func_800E0414  frame = start + t*(end-start)
```

`8008ADE4` still only advances the timer. `8008C1CC` stores
`Animation+0x1E`. `8008C038` → `8008BD94` → `800E0170` applies keys.

## 5. Track format (`PROVEN` ROM + `800E0170`)

```text
ANIMATION_TRACK_FORMAT: header s16 start,end,count,unk at +0; 4-byte dirents then packed keys
KEY_TIME_FORMAT: u16 at key+0; frame = bits 0–13; bit14=0x4000 cubic-prev; bit15=0x8000 cubic-next
ROTATION_KEY_FORMAT: s16 Euler degrees*64 (linear) or cubic *1/128; channels 0/1/2 = EX/EY/EZ
SCALE_KEY_FORMAT: same s16 units; channels 3/4/5 = SX/SY/SZ; default 1
TRANSLATION_KEY_FORMAT: same s16 units; channels 6/7/8 = TX/TY/TZ; default 0
INTERPOLATION_RULE: linear if no cubic flags; cubic (MIPS 800E04C0) if 0x4000/0x8000; channel>=9 wraps 0xB400/360°
```

Dirent `u16 = (boneAnimID<<4) | channel`. `func_800E084C` converts
Euler degrees × 0.5 through `sincos` (degree) into XYZW.

## 6–8. Clip 22 bone 107 (`PROVEN` ROM vs CSV)

Cart `0x16` / clip 22: start 0, end 30, 102 tracks, 3516 bytes.

```text
CLIP22_BONE107_TRACK_FOUND: YES
CLIP22_BONE107_ROTATION_KEYS: EX frames 0:0.0, 5:-6.6875, 30:6.0938 deg (cubic flags on 0 and 30)
CLIP22_BONE107_SCALE_KEYS: SX=SY=SZ 0:0.3125, 5:1.0, 30:1.0
CLIP22_BONE107_TRANSLATION_KEYS: TY 0:-1.25, 5:0, 30:0; TX/TZ default 0
```

Independent `800E04C0` cubic with **no** loop wrap (`a2=0`) at captured
times, then half-angle degree `sincos` for EX:

| t | frame | authored S | CSV S | authored qX | CSV qX |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0.0333333351 | 1 | 0.417000 | 0.416999996 | -0.008871 | -0.00887068454 |
| 0.0666666701 | 2 | 0.571000 | 0.57099998 | -0.021941 | -0.0219413918 |
| 0.100000009 | 3 | 0.741500 | 0.74150002 | -0.036408 | -0.0364090316 |
| 0.13333334 | 4 | 0.895500 | 0.895500004 | -0.049469 | -0.0494689532 |
| 0.166666672 | 5 | 1.000000 | 1.0 | -0.058326 | -0.0583270267 |

```text
BONE107_AUTHORED_SAMPLE_RECONSTRUCTION_AVAILABLE: YES
BONE107_RAW_RECORD_MATCHES_AUTHORED_ROTATION: YES
BONE107_RAW_RECORD_MATCHES_AUTHORED_SCALE: YES
BONE107_RAW_RECORD_MATCHES_AUTHORED_TRANSLATION: UNKNOWN  (CSV dumps +0x1C as T_PTR; authored TY lives at +0x20)
MAX_ROTATION_ERROR: ~1e-6 rad (q and -q equivalent; sign matches)
MAX_SCALE_ERROR: ~1e-8
MAX_TRANSLATION_ERROR: not comparable from this CSV
BONE107_SCALE_RAMP_AUTHORED: YES
```

The 0.417→1.0 ramp is the authored cubic between 0.3125 and 1.0 over
frames 0–5. Not a bug.

## 9. Prior 30/50/60/120° (`PROVEN` producer CSV)

```text
PRIOR_107_LARGE_ANGLE_MEASUREMENT_SOURCE: /tmp/tooie-kazooie-feather-producer.csv AE8A8 BASE_* / LOCAL_ROTATION_Q (raw 0x28 XYZW; BASE_QW is X)
PRIOR_ANGLE_WAS_RAW_LOCAL_RECORD: YES
PRIOR_ANGLE_WAS_COMPOSED_WORLD_OR_PARENT_RELATIVE: LOCAL record; FINAL also large because children inherit
PRIOR_ANGLE_CROSSED_CLIP_TRANSITION: YES
PRIOR_QUATERNION_DELTA_CALCULATION_CORRECT: YES (hemisphere-aware 2*acos(|dot|))
PRIOR_107_LARGE_ANGLE_DISCREPANCY_EXPLAINED: YES
```

Producer frames 1548–1552 (walk → idle):

| FRAME | raw XYZW | scale | step |
| ---: | --- | ---: | --- |
| 1548 | (-0.060, 0, 0, 0.998) | 1.018 | — |
| 1549 | (-0.057, 0.259, 0, 0.964) | 0.869 | ~30° |
| 1550 | (-0.046, 0.643, 0, 0.765) | 0.626 | ~50° |
| 1551 | (-0.025, 0.906, 0, 0.422) | 0.384 | ~50° |
| 1552 | (0, 1, 0, 0) | 0.141 | ~50° |

1575 `(0,1,0,0)` s=0.141 → 1576 `(0, 0.866, 0, 0.500)` ~60° → 1577
`(0,0,0,1)` s=0.156 ~120°, then scale 0.156→0.397… same family as
clip 22's ramp. Idle clips 111/3/12/1 hold 107 at `(0,1,0,0)` s≈0.14
(`PROVEN` V1 CSV). Walk clips 21/22 hold near identity + small EX.

The old 30/60/120° numbers are **authored** interpolations of a 180°
Y rest vs identity walk, across a few transition frames. They are not
an intra-clip sampler discontinuity and not a quaternion-math artifact.

Clip 22 after t=0.033 is already past that Y unwind (q ≈ small EX).
The V1 CSV therefore shows ≤~0.05 rad 107 rotation during stable
clip 22, which matches the ROM EX keys.

## 10. Crest / bone 108 (`PROVEN`)

```text
CLIP22_BONE108_TRACK_FOUND: YES  (EX only: 0:47.69°, 5:-34.5°, 30:43.80°; all cubic)
BONE108_LARGE_STEPS_AUTHORED: YES
BONE108_RAW_RECORD_MATCHES_AUTHORED_SAMPLE: YES  (same five times, error ~1e-6)
BONE79_HAS_LARGE_SAMPLE_STEPS: NO  (no clip22 tracks; identity 0,0,0,1)
CREST_SAMPLER_BEHAVIOR_MATCHES_PLUMES: NO  (108 has a large authored nod; 79 is empty; 107 is scale+small EX)
```

Captured 108 steps 0.365 / 0.389 / 0.310 rad at gl 698–700 are the
cubic nod, ~22° max, **authored**.

## 11. Body control 106 (`PROVEN`)

```text
BODY106_CLIP22_TRACK_FOUND: NO  (no dirents; apply defaults identity)
BODY106_SMOOTHNESS_MATCHES_AUTHORED_TRACK: YES
BODY_CONTROL_SAMPLE_TIME_SAME: YES (shared Animation+0x10)
BODY_CONTROL_SHOWS_COMPARABLE_LARGE_STEPS: NO
```

Same clip/time as 107/108. Smooth because the track is missing, not
because a global clock skipped 106.

## 12–13. Root

```text
ANIMATION_SAMPLER_STATUS: C
CURRENT_BEST_ROOT_CLASS: AUTHORED_ANIMATION_BEHAVIOR
ROOT_CONFIDENCE: HIGH
```

A is `REFUTED` (no intra-clip time bug). B is `REFUTED` for clip 22
(apply matches keys). D is `REFUTED` as “wrong delta math”; the old
angles were real raw records of authored transition poses. E is not
required for the sampler question.

`clip+0` Bottles wrapper is not the sampler. Independent
reconstruction used `800E04C0`/`800E084C` semantics on cart `0x16`.

## 14–15. Capture / fix

```text
EXISTING_CAPTURE_PLUS_STATIC_KEYS_SUFFICIENT: YES
NEW_CAPTURE_REQUIRED: NO
PRODUCTION_FIX_CANDIDATE_FOUND: NO
EXACT_BUG: none proven
```

Remaining optional TY compare would need the float at record `+0x20`,
not a new gameplay capture. Do not production-fix authored keys.

## Not this forensic

- RT64 / RSPProcess / ProjectionProcessor / Raster / presentation
- AE8A8 compose
- GPU readback
- Implementing another witness
