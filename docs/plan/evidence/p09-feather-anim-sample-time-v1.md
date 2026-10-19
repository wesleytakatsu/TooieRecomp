---
title: "P09 feather animation sample-time witness V1"
description: "Default-off CPU witness for baanim clip/time and raw 0x28 TRS before AE8A8; captured; classified in authored-key forensic."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-09-08
tags: [p09, kazooie, feather, animation, baanim, sample-time, AE8A8]
---

# P09 feather animation sample-time witness V1

Bounded objective: decide whether large bone-107 current-transform
steps are authored animation keys sampled correctly, or a
clip/time/sampler discontinuity. Diagnostic only. No game this turn.
No production fix. No GPU readback or added synchronization. No
commit. No push. Downstream renderer stages stay closed.

Prior forensic: [[p09-upstream-current-bone-transform-semantic-forensic]].

```text
DIAGNOSTIC_SCHEMA: P09_FEATHER_ANIM_SAMPLE_TIME_V1
DIAGNOSTIC_VERSION: 1
MAX_ROWS: 384
SELF_DIAGNOSING_STATUS_IMPLEMENTED: YES
RAW_RECORD_CAPTURED_BEFORE_COMPOSE: YES
BONE107_KEY_PROVENANCE_COMPLETE: NO
INDEPENDENT_SAMPLE_RECONSTRUCTION_AVAILABLE: NO
BODY_CONTROL_BONE: parent of 107 (runtime boneAnimID; expected skel index 24)
PRODUCTION_FIX_CANDIDATE_FOUND: NO
USER_MANUAL_TEST_REQUIRED_NOW: NO
NEW_CAPTURE_REQUIRED: NO (captured; classified in [[p09-animation-clip-transition-and-authored-key-forensic]])
GPU_READBACK_USED: NO
GPU_SYNCHRONIZATION_ADDED: NO
NO_COMMIT: YES
NO_PUSH: YES
```

## Sampler sources (`PROVEN` `STATIC_MIPS`)

`baanim_update` is `0x8008C894`. `a0` is PLAYER. State = `*(player+4)`.
Mode = `*(state+8)` selects `D_80124AB0`: case 1 `func_8008ADE4`,
case 2 `__baanim_update_scaleToHorizontalVelocity` (`0x8008C2A8`) then
ADE4, case 4 ADE4 twice. ADE4 only advances the AnimCtrl timer; it
does not write 0x28 bone records.

| Field | Source | Address |
| --- | --- | --- |
| ACTIVE_CLIP_ID_SOURCE | `lh` Animation `+0x18` | `func_8008C0C0`; clip = `anctrl+0x1C` |
| SAMPLE_TIME_SOURCE | `lwc1` Animation `+0x10` | `func_8008C0C8` / write `func_8008C1C0` |
| SAMPLE_FRAME_SOURCE | derived `trunc(sample_time)` plus optional asset header start/end | no integer frame register proven |
| CLIP_DURATION_OR_FRAME_COUNT_SOURCE | `anctrl+0xC` seconds/rate | `anctrl_setDuration` / `0x8008B1BC` |
| WRAP_MODE | `anctrl+0x1A` | 1=ONCE 2=LOOP 3=STOPPED 4=SUBRANGE |
| INTERPOLATION_MODE | UNKNOWN at ADE4 | 0x28 records observed as AE8A8 input |
| KEY_LOOKUP_FUNCTION | NOT_IDENTIFIED_IN_ADE4 | Animation.apply pointer at `clip+0`; pose fill is not ADE4 |
| CURRENT_KEY_INDEX / NEXT_KEY_INDEX / KEY_FRACTION | RUNTIME_VALUE_REQUIRED | not in AnimCtrl; not invented |

`__baanim_update_scaleToHorizontalVelocity` remaps `|v_xz|` from
`func_8009BB24` (`*(player+0xC8)+0x10`) into `anctrl_setDuration`.
That is clip **rate**, not bone-107 TRS scale. The witness dumps
`sqrt(vx*vx+vz*vz)` from that actor vector without calling
`8009BB24`, and dumps post-update `anctrl+0xC` as ANIMATION_RATE.

## Identity

Rows join `guest_logic_tick` from `func_800151BC` (not `ctx->r31`).
`baanim_update` a0 is saved before the original call. AE8A8 dump is
**before** `__real_func_800AE8A8`. Player filter is `D_80135490`.

## Target bones

79, 100, 101, 102, 103, 104, 105, 107, 108, plus the parent of 107.

BODY_CONTROL_BONE: parent of 107.
WHY: 107 is the common plume parent (107→100 left, 107→103 right);
its parent is the nearest ordinary body bone on the same update.

Raw 0x28 before compose: quat XYZW at `+0/+4/+8/+C` (X at +0, W at
+0xC), scale `+0x10/14/18`, translation pointer `+0x1C`.

## Classification design (after capture)

| Class | Meaning |
| --- | --- |
| A | time continuous, raw 107 equals authored interpolation → `BONE107_STEPS_AUTHORED_AND_CORRECT` |
| B | sample time/frame jumps unexpectedly → `ANIMATION_SAMPLE_TIME_BUG` |
| C | time correct, key/fraction wrong → `ANIMATION_KEY_SELECTION_OR_INTERPOLATION_BUG` |
| D | keys correct, 0x28 disagrees with reconstructed sample → `BAANIM_RECORD_GENERATION_BUG` |
| E | 107 smooth under true sampler data → re-evaluate the world-discontinuity assumption |

Do not classify A–E until the capture exists. Key provenance is
PARTIAL: clip/time/frac/playback are recorded; prev/next key index
and authored key times are not, because the pose writer was not
identified. Independent reconstruction is therefore NO.

## Files

- Env: `TOOIE_DIAG_FEATHER_ANIM_SAMPLE_TIME=1` (default off)
- Status: `/tmp/tooie-feather-anim-sample-time-status.txt` (eager INIT)
- CSV: `/tmp/tooie-feather-anim-sample-time.csv` (max 384 rows)
- Hooks: after `baanim_update` in `boot_hw_stubs.cpp`; before
  `func_800AE8A8` in `backpack_lookup_diag.cpp`

CSV reserves first 8 control ticks, clip changes, expected wraps,
unexpected time jumps, and large 107/108/79/body steps. Counters
always increment.

## Validation

```text
BUILD: cmake --build build --target TooieRecompiled window_display_test tintro_projection_policy_test -j2
BUILD_RESULT: PASS
WINDOW_DISPLAY_TEST: PASS 38/38
PROJECTION_TEST: PASS 7/7
TOOIE_DIFF_CHECK: PASS
RT64_DIFF_CHECK: PASS (no Tooie-authored RT64 change this turn)
```

HEAD Tooie `ff5b6d9` dirty. Pre-existing dirty files were not folded
into this witness.

## Capture authorization

Authorized: clip/time/playback/duration are observable; raw 0x28 is
captured before compose; body control exists; 108/79 included;
status is self-diagnosing; bounded; build/tests/diff pass. Key
indices remain UNKNOWN; capture still distinguishes A vs B via time
continuity vs authored-looking 107 steps.

USER_TEST_COMMAND:

```sh
TOOIE_DIAG_FEATHER_ANIM_SAMPLE_TIME=1 ./build/TooieRecompiled
```

Enter combined `0x62B` Kazooie gameplay and walk so bone 107 moves.
Do not set `TOOIE_DIAG_SECONDARY_SWAY`. After exit, keep
`/tmp/tooie-feather-anim-sample-time-status.txt` and
`/tmp/tooie-feather-anim-sample-time.csv`.

NEXT_SINGLE_ACTION: one user capture, then GPT-5.6 Luna classifies
A–E from the status/CSV. Do not GPU-readback. Do not production-fix.
Do not start presentation/compositing.
