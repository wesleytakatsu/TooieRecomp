---
title: "P08 — demo movement precision diagnosis"
description: "Narrow differential of the small residual gameplay-demo path error after the accepted func_80013B70 fix. WASD directional collapse is resolved; attract-bypass claim is corrected."
status: evidence
phase: P08
updated: 2026-08-15
tags: [tooie-recomp, p08, attract, glrecord, bastick, demo-path, diagnosis]
---

# P08 — DEMO MOVEMENT PRECISION DIAGNOSIS

```text
BASE_HEAD:                         6a0a6960c9b7d12de05225cf69c182c5dc1f7999
MODEL:                             Grok 4.6 HIGH
MODE:                              NARROW DIFFERENTIAL DIAGNOSIS ONLY
USER_BUG_A_FUNCTION_LEVEL:         RESOLVED
USER_BUG_A_WASD_RUNTIME:           RESOLVED
USER_BUG_A_FULL_CONTROLLER:        PENDING
FIX_IMPLEMENTED:                   NO
TRANSFORM_TAGGING_IMPLEMENTED:     NO
```

Manual current-HEAD evidence (WASD): live player no longer collapses every
direction toward screen-right; WASD directions appear correct; neutral
behavior remains normal. That closes the previously pending runtime gate
for USER_BUG_A. Analog/gamepad is **not** marked validated.

Do **not** reopen the solved directional-collapse bug.

## 1. Selected clip

```text
SELECTED_DEMO:                     gsattract first gameplay clip
DEMO_SCENE_TYPE:                   recorded-pad gameplay demonstration
MAP:                               MAP_14F_IOH_WOODED_HOLLOW (335), entrance 8
ACTOR:                             Banjo / player 0 (D_80135490[0])
START_EVENT:                       mode=5 first grounded sample after intro
END_EVENT:                         end of first map=0 grounded walk
                                   (before map 278 / later attract scenes)
EXPECTED_VISIBLE_PATH:             Banjo walks the IOH path from the spawn
                                   near (1861, -131, -166) along the grass
                                   in the same world direction as original
                                   Tooie (user side-by-side reference)
```

This is the early stable sequence already used by
[[p08-visible-demo-movement-diagnosis]]. Later attract maps (278, 314, 395)
were not diagnosed.

## 2. Attract / demo control mechanisms (CORRECTED)

Previous notes claimed attract/demo bypassed bastick entirely
([[p08-player-direction-facing-diagnosis]] §4, [[p08-visible-demo-movement-diagnosis]] §9).

New manual evidence: gameplay-demo movement improved after the
`func_80013B70` fix. Runtime capture on this HEAD shows why.

Tooie has **more than one** demo/attract control path:

| scene class | control source | bastick | 13B70 |
| --- | --- | --- | --- |
| **Gameplay demonstration (selected clip)** | `glrecord` recorded `OSContPad` samples injected in `func_8001608C`, then remapped into `D_80079AF8` | YES | YES (`bastick_update`) |
| Live player (WASD / pad) | host SDL → `osContGetReadData` → same remapper | YES | YES |
| Cinematic / forced actor facing | script / `func_800F18FC` world-delta heading / drone-goto | usually NO | sometimes (`func_800F18FC`, `subaddiejoy`) |
| Bots | AI → `yaw_setIdeal` | NO | NO (bot yaw path) |

```text
OLD_ATTRACT_BYPASS_CLAIM:          CORRECTED/SUPERSEDED
  previous wording treated all attract/demo as "stick=0, bastick unused"
  that is true of some cinematic/scripted actors and of idle frames
  it is FALSE for the Wooded Hollow gameplay-demo player walk
```

### 2.1 Gameplay-demo player (PROVEN)

`func_800A9CAC` returns 1 when attract byte `D_80127760 != 0`. During
mode-5 Wooded Hollow that byte is 2, so `func_8001608C` calls
`_glrecord_entrypoint_0`.

`glrecord_entrypoint_0` reads 6-byte samples from `D_8012D5B0` and writes
them onto `OSContPad` (`stick_x`, `stick_y`, buttons). The remapper then
stores floats at `D_80079AF8+8/+C`. `bastick_update` consumes those floats
and calls `func_80013B70`.

Live `OSContPad` at dump time is usually `(0,0)` because the host
controller is untouched and a later `osContGetReadData` overwrites the
guest pad after the remapper has already consumed the recorded sample.

```text
PLAYER_CONTROL_SOURCE:             recorded pad (glrecord) → remapper
                                   → bastick (not live pad, not goto target)
BASTICK_USED:                      YES
FUNC_80013B70_USED:                YES
GOTO_TARGET_USED:                  NO (phys+0x64 = 0,0,0 during the walk)
LIVE_OSCONTPAD_DURING_WALK:        almost always (0,0)
REMAP_AND_BASTICK_MATCH:           YES (0 mismatches / 294 moving samples)
```

### 2.2 Why the old stick=0 observation happened

The earlier `TOOIE_DIAG_MOVEMENT` dump sampled at 80–200 ms and often
caught the pre-walk idle (`stick=0`, `xyz=1861,-131,-166`). Denser
`TOOIE_DIAG_DEMO_PATH` sampling on this run shows the walk immediately
drives remapped stick `(0,1)` / angle `90°`.

```text
OLD_STICK_ZERO_WHILE_WALKING:      CORRECTED/SUPERSEDED for this clip
                                   (idle/coarse-sample artifact, not the walk)
```

## 3. Error type (game-internal, not invented original numbers)

Capture: `TOOIE_AUTO_START=1 TOOIE_DIAG_DEMO_PATH=1`, 130 s budget,
log `/tmp/tooie-p08-demopath.log` (not committed). 648 path samples,
294 moving samples on the first grounded map=0 interval.

Do **not** treat the user's side-by-side original as a frame-perfect
numeric ground truth the agent measured. Compare against Tooie's own
commanded values.

### 3.1 First straight (stick UP hold)

```text
START_POSITION:                    1861.000, -131.000, -166.000  yaw=276.00
                                   (idle spawn; gfx=49706)
FIRST_MOVE:                        1849.922, -132.137, -166.431
                                   gfx=50257  stick=(0,1) ang=90
                                   ideal=267.70  yaw=270.96  head=267.77
END_OF_FIRST_GROUNDED_WALK:        701.225, -497.268, 2644.458
                                   gfx=69667  (later analog stick, not the
                                   first UP hold)
FIRST_STRAIGHT_HEADING:            267.4° (head = yaw = ideal after ramp)
TICK_COUNT:                        not a guest logic counter; host gfx span
                                   50257→50850 ≈ 593 gfx for the first 10
                                   moving samples (~0.6 s at ~1 kHz present)
```

First-tick angular error versus the game's own ideal yaw:

```text
POSITION_ERROR_X:                  not vs original; first move is -11.1 from idle
POSITION_ERROR_Z:                  not vs original; first move is -0.43 from idle
HEADING_ERROR_DEGREES:             0.07  (head 267.77 vs ideal 267.70)
YAW_ERROR_DEGREES:                 3.26  (yaw still catching ideal; 1–2 samples later 0.1°)
DISTANCE_ERROR_PERCENT:            +2.0  after ramp (horiz |vel| ≈ 510 vs tspd 500)
```

On the first UP hold, heading tracks ideal to ~0.1°. There is **no**
first-tick angular writer error and **no** heading accumulation on that
straight.

Steady horiz speed after the accel ramp is consistently ~510 against
`baphysics` target `tspd=500` (ratio 1.02). That is a game-internal
speed discrepancy, not an invented original-world number.

```text
PRIMARY_RESIDUAL:                  SPEED
  ANGLE on the first straight is internally consistent (≤0.1°)
  START_POSITION is the same idle spawn previously measured
  TIMING baseline is not reopened (P08-T03 already validated)
  visual residual vs original may still mix a small speed scale
  with later analog-stick / camera-relative turns
```

### 3.2 First-tick vs accumulated

```text
FIRST_DIVERGENCE_TICK:             first moving sample
FIRST_TICK_HEADING_ERROR:          0.07° vs ideal (not the residual)
FIRST_TICK_SPEED:                  222 vs tspd 500 (accel ramp; expected)
ERROR_PER_TICK (heading, first straight): ~0
ERROR_ACCUMULATION:                NONE on first-straight heading
                                   LINEAR ~+2% on horiz speed vs tspd
                                   after the ramp
```

## 4. Recorded-input pipeline (selected clip)

```text
recorded 6-byte sample (D_8012D5B0)
  → glrecord_entrypoint_0 writes OSContPad
  → func_80015D60 (lb, scale 7 / 0x3D) → D_80079AF8 floats
  → bakey_getStick / bastick_update
  → func_80013B70(stick_y, stick_x, distance)
  → camera-relative desired yaw
  → baphysics target speed + yaw
  → func_800EEB9C (sin/cos degrees) → X/Z velocity
  → position integration
```

Checked and **not** reopened:

- `func_80013B70` cardinals 0/90/180/270 and diagonals (accepted independently).
- First-straight stick `(0,1)` → angle `90.00` exactly.
- Remapper `lb` of the `lbu`/`sb` glrecord bytes is bit-preserving.
- Host SDL / `convert_to_n64_range` is **not** on this recorded path.
- `func_800EEB9C` uses the existing native `sin/cos` helpers
  (`vel.x = sin(yaw)*speed`, `vel.z = cos(yaw)*speed`). First-straight
  heading matches that convention. Helpers are not reopened.

`func_8001608C` (which consumes one glrecord sample) is called from the
per-frame game update `func_800123F4`, and also from `func_800A759C` /
`func_800A9924`. Double-consume in one frame would be a **large** speed
error, not the small residual the user sees. Not proven on this run.

## 5. Speed contract (first straight, after ramp)

```text
BASE_SPEED:                        tspd = 500  (baphysics+0x3C)
ANIMATION_SPEED_FACTOR:            UNKNOWN (not isolated)
STICK_MAGNITUDE_FACTOR:            1.0 (distick=1.0, zone full)
STATE_SPEED_MULTIPLIER:            UNKNOWN
DELTA/TICK_FACTOR:                 UNKNOWN; P08-T03 cadence validated
FINAL_WORLD_SPEED:                 |vel_xz| ≈ 510  (1.02 × tspd)
```

Do **not** reopen global timing. The local question is why integrated
horiz speed sits ~2% above the physics target.

## 6. Angle contract

```text
raw stick:                         (0, 1) during the first straight
func_80013B70:                     90.00
camera-relative ideal:             267.4 after one or two samples
actor yaw:                         tracks ideal (catch-up then match)
movement-vector heading:           267.4 (atan2(dx,dz), 0° = +Z)
first angular difference:          0.07° (not 1 / 5 / 90 / 180)
```

`func_80013B70` participates and is **not** the residual. Its
cardinal/general semantics remain accepted.

## 7. Bots (control group only)

```text
BOT_MOVEMENT_SPEED_PATH_SHARED:    UNKNOWN (may share baphysics)
BOT_DIRECTION_MATH_SHARED:         NO
FIRST_PLAYER_DEMO_VS_BOT_DIVERGENCE:
  player demo enters bastick_update via remapped glrecord stick;
  bots set yaw->ideal from AI and do not use bastick
```

Bots looking plausible is compatible with a player-only recorded-stick
or player-only +2% walk-speed residual.

## 8. Forced / cinematic facing — characterize only

USER_BUG_B remains **OPEN_SEPARATE**.

One observed nearby actor during the first walk
(`act=801ca174`, pos ≈ `-775,-300,-57`, `rot.y=201`, not translating)
has a large yaw while the player movement heading is 267°. Player
heading and player yaw agree. That actor is not on the bastick path.

Early intro also calls `func_80013B70` with **world-unit** `(dx,dz)`
magnitudes (70–126), matching `func_800F18FC` heading-from-delta, and
those angles appear in other actors' `rot.y`. That path is a different
mechanism from the recorded-pad walk.

```text
LIKELY_SEPARATE_FACING_BUG:        YES
```

Stop. Do not diagnose the full cinematic facing bug here.

## 9. Root cause

```text
ROOT_CAUSE_LEVEL:                  C
ROOT_CAUSE:
  The selected gameplay demo is recorded-pad playback through bastick.
  After the accepted 13B70 fix, first-straight heading matches the
  game's own desired yaw. The remaining internal residual is a ~2%
  horiz-speed overshoot versus baphysics target 500 (measured 510).
  Visual miss versus original may still include later analog-stick
  turns or camera-relative yaw; those are not first-straight angle
  errors. Exact writer of the 2% scale is not proven.

FIRST_BAD_STAGE:                   post-ramp horiz integration vs tspd
FIRST_BAD_FUNCTION:                unproven
                                   candidates: baphysics integrator,
                                   animation root-motion add, local dt
EXPECTED_VALUE:                    |vel_xz| = tspd (500) once ramped
ACTUAL_VALUE:                      |vel_xz| ≈ 510

FIX_OWNER_CANDIDATE:               TOOIE_NATIVE (only if a later
                                   discriminator proves a Tooie-local
                                   writer; do not touch RecompiledFuncs)
MINIMAL_FIX_FILES:                 TBD after the remaining discriminator
FIX_IMPLEMENTED:                   NO
```

Single remaining discriminator: on the first UP hold, dump per logic
tick `tspd`, anim displacement, physics dt, and `|vel_xz|` until the
ramp finishes, and count `glrecord` index steps versus game-frame
updates. If `|vel_xz|/tspd` is already 1.02 with dt matching P08-T03,
the writer is a local speed multiplier. If glrecord advances more than
once per game frame, the residual is sample cadence instead.

## 10. Instrumentation retained

```text
TOOIE_DIAG_DEMO_PATH=1
```

Independent of `TOOIE_DIAG`. Implies movement sampling.

- `note_13b70` in `src/game/boot_hw_stubs.cpp` (throttled)
- denser mode-5 player sample in `src/game/syscall_handler.cpp`
  (tick/pos/delta/speed/heading/yaw/ideal/stick/remap/pad/tspd/goto)

No massive 400k-line logs on a clean run. Temporary `/tmp` log is not
committed.

## 11. Validation

```text
STICK_ANGLE_TEST:                  PASS
MATRIX_SUBMISSION_TEST:            PASS
INPUT_DIRECTION_TEST:              PASS
BCF28_TEST:                        PASS
CORE1_RA_TEST:                     PASS
TOOIE_BUILD:                       PASS
```

The diagnostic run ended SIGSEGV (~98 s) during a later attract overlay
load (map 314 era). That crash is **not** used as evidence for the
Wooded Hollow residual. First-clip samples were already complete.

## 12. Next single action

If a later task implements a fix: implement **only** the proven
demo-movement precision writer after the remaining discriminator.
Do not implement cinematic facing, transform tagging, or P08-T10
automation.
