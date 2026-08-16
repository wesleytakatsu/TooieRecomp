---
title: "P08 — forced/cinematic actor facing diagnosis"
description: "One-actor diagnosis of USER_BUG_B: Wooded Hollow sucoaster chdiggerground walks the return path with an exact 180° yaw freeze. Diagnosis only; no fix."
status: evidence
phase: P08
updated: 2026-08-15
tags: [tooie-recomp, p08, facing, cinematic, sucoaster, chdiggerground, user-bug-b]
---

# P08 — FORCED / CINEMATIC FACING DIAGNOSIS

```text
BASE_HEAD:                         2412ea51a7743442982699c84cb45701f6daff74
MODEL:                             Grok 4.6 HIGH
MODE:                              NARROW DYNAMIC ROOT-CAUSE DIAGNOSIS
FIX_IMPLEMENTED:                   NO
TRANSFORM_TAGGING_IMPLEMENTED:     NO
DEMO_MOVEMENT_RESIDUAL:            DEFERRED_PENDING_SPECIFIC_VISUAL_SEGMENT
USER_BUG_A_WASD_RUNTIME:           RESOLVED
USER_BUG_A_FULL_CONTROLLER:        PENDING
USER_BUG_B_STATUS:                 DIAGNOSED_ONE_ACTOR (Level B)
```

Do **not** reopen: WASD directional collapse, `func_80013B70`, first Wooded
Hollow cardinal straight, first analog transition `glidx 74..91`,
`func_80013ABC`, first-straight ~510 slope scale, matrix submission, or
global original timing / 4:3.

## 1. Selected actor

```text
SELECTED_SCENE:                    Wooded Hollow gsattract mode-5
                                   sucoaster ground-digger return
MAP:                               MAP_14F_IOH_WOODED_HOLLOW (335)
                                   (probe map field often 0 during mode-5)
OVERLAY:                           219 chdiggerground
ACTOR_TYPE:                        chdiggerground
                                   (_chdiggerground_entrypoint_0 = 0x80085C98)
ACTOR_POINTER:                     0x801c5d1c then 0x801c5afc
                                   (same logical instance; realloc)
START_EVENT:                       first reverse sample after outbound walk
                                   tick=56026 pos=(-2548.28, -326.75, 980.36)
VISIBLE_FAILURE:                   walks back toward the burrow at heading
                                   294.21° while Actor.rotation.y stays 114.21°
```

This is **not** the recorded-pad Banjo. Control is `sucoaster` + `subaddie`,
not `bastick` / `glrecord`.

Rejected nearby leads (not this diagnosis):

| lead | why not selected |
| --- | --- |
| Wooded Hollow Banjo `Actor.rotation.y=0` | player yaw lives in `PlayerState`; recorded-pad path already PASS |
| `chbadshad` (0x80085768) | blob shadow under diggers; facing of a disc is not the user clip |
| `chintrochar` look-at snaps | first move matches `func_80013B70` world-delta; idle look-at, not a walk |
| `chintrochar` teleports | scene cuts, not a facing-while-moving defect |
| later `cheggfire` / `chhoney` | second-scene leftovers; stop after one actor |

## 2. Four headings (not conflated)

At the first reverse sample (`tick=56026`, `act=801c5d1c`):

```text
MOVEMENT_HEADING:                  294.21°
  atan2(dx, dz) of the live position delta
  dpos = (-18.24, +8.20)

TARGET_HEADING:                    114.21°
  Actor.unk54 / sucoaster path-yaw
  = heading from actor position toward the still-forward path node
  (func_8010CD50 → func_800F1DF4(actor.pos, path_point))

ACTOR_LOGICAL_YAW:                 114.21°
  Actor.rotation.y at +0x48

MODEL_RENDER_YAW:                  114.21°
  same field; no distinct submitted yaw
```

`unk54` and `rotation.y` stay equal for the whole reverse. This is not a
logical-vs-render split. Matrix submission is not reopened.

## 3. Primary failure class

```text
PRIMARY_FAILURE_CLASS:             D
```

Movement heading is the reverse path (correct translation). The scripted
look-at / path-yaw target itself stays on the **outbound** node, so desired
yaw is 180° from travel. Logical yaw faithfully follows that desired value.

Not A (movement is not wrong). Not B in the “yaw writer wrote a garbage
angle” sense — there is no new incompatible store; the last outbound yaw is
reused because the target node did not flip. Not C (render). Not E (yaw is
frozen; no long interpolation).

## 4. Per-tick window (gfx ticks; TOOIE_DIAG_FORCED_FACING=1)

Live run on this HEAD, log `/tmp/tooie-p08-facing2.log` (not committed).
`TOOIE_AUTO_START=1 TOOIE_DIAG_FORCED_FACING=1`, ~99 s, later-attract
SIGSEGV (historical map-314-era crash; **not** used as facing evidence).

Same logical digger, outbound then reverse. Sibling `chdiggerground`
`801c5c34` on another path keeps facing its movement in the same window.

| tick | actor | pos xz | move_head | yaw | unk54 | move_vs_yaw |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 55530 | 801c5d1c | -2745.8, 1060.4 | 114.25 | 114.25 | 114.25 | 0.00 |
| 55592 | 801c5d1c | -2724.2, 1050.7 | 114.25 | 114.25 | 114.25 | 0.00 |
| 55654 | 801c5d1c | -2702.7, 1041.4 | 113.55 | 113.55 | 113.55 | 0.00 |
| 55716 | 801c5d1c | -2672.6, 1028.2 | 113.55 | 113.55 | 113.55 | 0.00 |
| 55779 | 801c5d1c | -2642.2, 1015.5 | 112.65 | 112.65 | 112.65 | 0.00 |
| 55839 | 801c5d1c | -2605.7, 1000.3 | 112.65 | 112.65 | 112.65 | 0.00 |
| 55917 | 801c5d1c | -2570.4, 987.1 | 110.54 | 110.54 | 110.54 | 0.00 |
| 55965 | 801c5d1c | -2530.0, 972.2 | 110.28 | 114.21 | 114.21 | 3.93 |
| **56026** | **801c5d1c** | **-2548.3, 980.4** | **294.21** | **114.21** | **114.21** | **180.00** |
| 56150 | 801c5afc | -2621.2, 1013.2 | 294.21 | 114.21 | 114.21 | 180.00 |
| 56210 | 801c5afc | -2657.7, 1029.6 | 294.21 | 114.21 | 114.21 | 180.00 |
| 56273 | 801c5afc | -2694.2, 1046.0 | 294.21 | 114.21 | 114.21 | 180.00 |
| 56336 | 801c5afc | -2714.7, 1055.2 | 294.21 | 114.21 | 114.21 | 180.00 |
| 56398 | 801c5afc | -2733.0, 1063.4 | 294.21 | 114.21 | 114.21 | 180.00 |
| 56461 | 801c5afc | -2751.2, 1071.6 | 294.21 | 114.21 | 114.21 | 180.00 |
| 56523 | 801c5afc | -2756.9, 1074.2 | 294.21 | 114.21 | 114.21 | 180.00 |

Control group, same ticks: `801c5c34` `chdiggerground` heading tracks yaw
(deltas ≤ 3°). `chdiggerfly` (0x80085CA8) also faces movement.

## 5. Intended facing

```text
INTENDED_FACING_SOURCE:            sucoaster path-follow contract:
                                   this actor type faces travel
INTENDED_YAW:                      294.21°
WHY:                               outbound of the same instance faces
                                   movement (110–114°); other
                                   chdiggerground instances in the same
                                   scene face movement continuously
```

A deliberate “back into the burrow” pose is **not** proven. If original
plays a reverse/burrow animation here, this classification would need to
be REFUTED. The live walk is a long (~200 unit) reverse with frozen yaw,
which matches the user-visible “~180° reversed” report.

`func_800F18FC` is **not** on this actor during the event. Early intro
`func_80013B70` world-delta calls belong to `chintrochar` look-at and are
not this residual. `func_80013B70` remains accepted.

## 6. Yaw write chain

```text
sucoaster_entrypoint_2 (per-frame)
  → func_8080027C / sucoaster_entrypoint_10
    → func_8080264C_sucoaster
         func_80802760  → path node index (or -1)
         if index == -1:  f0 = Actor.rotation.y   (keep)
         else:            func_8010CD50(actor, index)
                            func_800F5A00(index) → path point
                            func_800F1DF4(actor.pos, path_point)
                              → heading toward that point
    → store f0 at Actor.unk54 (+0x54)
  Actor.rotation.y (+0x48) tracks the same value on this clip
```

Generated bodies of `func_8080264C_sucoaster` (`RecompiledFuncs/funcs_241.c`)
and `func_8010CD50` (`RecompiledFuncs/funcs_414.c`) match original MIPS
structure (delay slots present; not empty). READ ONLY. The defect is the
**node / travel-direction convention**, not a smashed 13B70-style body.

```text
FIRST_YAW_WRITER_PC:               0x808006F8  (swc1 f0, 0x54(s0)
                                               in func_8080027C)
FIRST_YAW_WRITER_FUNCTION:         func_8080264C_sucoaster
FIRST_BAD_STAGE:                   path-node look-at on reverse travel
FIRST_BAD_FUNCTION:                func_8080264C + func_80802760 /
                                   func_800F5A00 node selection
                                   (func_8010CD50 look-at is faithful
                                   given the node it is handed)
FIRST_BAD_GUEST_PC:                0x80802678  (jal func_8010CD50)
                                   or 0x80802760 (node == -1 keep)
EXPECTED_VALUE:                    294.21°
ACTUAL_VALUE:                      114.21°
```

First incompatible value: `TARGET_HEADING` / `unk54` stays on the outbound
node (114.21°) after position integration has reversed.

Remaining discriminator to reach Level A: on the first reverse tick, does
`func_80802760` return `-1` (keep old yaw) or a live index whose
`func_800F5A00` point still sits at the outbound end?

## 7. 180° / sideways / 360°

```text
ERROR_OFFSET_DEG:                  180.00
EXACT_180_PATTERN:                 YES
MOVEMENT_VS_YAW_DELTA:             180.00   (backward)
RAW_YAW_DELTA:                     0.00
WRAPPED_YAW_DELTA:                 0.00
TURN_DIRECTION:                    none (frozen)
SHORTEST_PATH_USED:                NA
```

Not an X/Z swap (that is 90°). Not a 13B70 quadrant error. Not a global
model-forward +180 (outbound would also be 180° off). Not a 350°→−10°
interpolation miss — yaw does not interpolate on the reverse.

Hypotheses that fit the exact 180°:

- target minus actor vs actor minus target on the stale outbound node
  (look-at the node now behind the actor);
- missing reverse-travel +180 after `func_8010CD50`.

Do **not** apply a global +180.

## 8. func_800F18FC

```text
FUNC_800F18FC_USED_BY_SELECTED:    NO
```

Not inspected as a writer for this event.

## 9. Player / bot control group

```text
SHARES_PLAYER_YAW_WRITER:          NO
SHARES_BOT_YAW_WRITER:             NO
```

Player recorded-pad / WASD uses `bastick` → `func_80013B70` → `yaw_setIdeal`.
Bots also enter `yaw_setIdeal` from AI. This digger never does.

## 10. Logical vs render

```text
LOGICAL_VS_RENDER:                 logical path-yaw and Actor.rotation.y
                                   are both 114.21°; visible reverse is
                                   that logical facing
```

Do not reopen matrix submission.

## 11. Root cause

```text
ROOT_CAUSE_LEVEL:                  B
ROOT_CAUSE:                        sucoaster chdiggerground faces the
                                   heading to its current path node.
                                   On reverse, position travels 294.21°
                                   but the node/look-at stays the
                                   outbound end, so desired yaw, unk54,
                                   and rotation.y remain 114.21°.
FIX_OWNER_CANDIDATE:               TOOIE_NATIVE (only after the remaining
                                   node-index discriminator; do not edit
                                   RecompiledFuncs)
MINIMAL_FIX_FILES:                 TBD: native override of the sucoaster
                                   path-yaw helper if original MIPS
                                   flips travel direction and generated
                                   code does not; otherwise a Tooie
                                   travel-direction adjustment at
                                   func_8080264C
FIX_IMPLEMENTED:                   NO
```

Minimal fix sketch (not implemented): when sucoaster `dt` / travel flag is
reverse, either hand `func_8010CD50` the previous node or wrap the look-at
by 180°. Confirm original MIPS first. No global +180. No 13B70 change.

## 12. Instrumentation retained

```text
TOOIE_DIAG_FORCED_FACING=1
```

Independent of `TOOIE_DIAG`. Compact `[facing]` lines for
`chintrochar` / `chdiggerground` / `chdiggerfly` only. Skips `chbadshad`
and the Banjo `Actor.rotation.y=0` slot.

```text
[facing] tick= actor= sys= state= pos= dpos= move_head=
         yaw= unk54= pitch= raw_delta= wrapped_delta= move_vs_yaw=
```

Files: `include/tooie_diag.h`, `src/game/tooie_diag.cpp`,
`src/game/syscall_handler.cpp`.

## 13. Validation

```text
STICK_ANGLE_TEST:                  PASS
MATRIX_SUBMISSION_TEST:            PASS
INPUT_DIRECTION_TEST:              PASS
BCF28_TEST:                        PASS
CORE1_RA_TEST:                     PASS
TOOIE_BUILD:                       PASS
```

The diagnostic run ended SIGSEGV (~99 s) in a later attract overlay load.
First Wooded Hollow reverse samples were already complete. That crash is
not facing evidence.

## 14. Next single action

Independent review of this facing defect before any implementation.
If review wants Level A: watch `func_80802760` `$v0` and the
`func_800F5A00` point on tick 56026 only.

## 15. Independent review pointer / correction status

**CORRECTED/SUPERSEDED (2026-08-16):** The Level B and `TOOIE_NATIVE`
classification above is preserved as historical Grok evidence. The
independent review did not establish the original return-facing contract and
did not capture the selected actor's node index discriminator. See
[[p08-forced-cinematic-facing-independent-review]]. No production fix was
implemented.
