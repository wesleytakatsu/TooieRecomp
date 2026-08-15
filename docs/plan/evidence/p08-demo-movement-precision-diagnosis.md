---
title: "P08 — demo movement precision diagnosis"
description: "Narrow differential of the small residual gameplay-demo path error after the accepted func_80013B70 fix. WASD directional collapse is resolved; attract-bypass claim is corrected. Per-logic-tick speed contract discriminator added (DeepSeek V4 Flash HIGH). Post-physics velocity writer discriminator: 510 is original slope-scaled physics integration (Grok 4.6 HIGH)."
status: evidence
phase: P08
updated: 2026-08-15
tags: [tooie-recomp, p08, attract, glrecord, bastick, demo-path, diagnosis, speed-contract]
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

---

## 13. PER-LOGIC-TICK SPEED CONTRACT DISCRIMINATOR — DeepSeek V4 Flash HIGH

```text
BASE_HEAD:                          fafd26b68022bb121ea670bd5aa23783a1bedeec
MODEL:                              DeepSeek V4 Flash HIGH
MODE:                               DYNAMIC SINGLE-DISCRIMINATOR DIAGNOSIS
SELECTED_DEMO:                      Wooded Hollow gsattract recorded-pad UP hold
LOGIC_TICK_BOUNDARY:                glrecord overlay 698 ep 0 dispatch
                                    (the TRUE recorded-input consumer; each
                                    dispatch consumes one 6-byte sample from
                                    D_8012D5B0 and writes OSContPad)
GLRECORD_ADVANCES_PER_LOGIC_TICK:   1 (adv=1, count=766, no double-consume,
                                    no skipped sample)
DOUBLE_CONSUME:                     NO
SKIPPED_SAMPLE:                     NO
SAMPLE_CADENCE_CORRECT:             YES
LOGIC_TICK_HZ_DURING_CLIP:          30.0 Hz (fc=2, gs=D_8012C760=0.03333
                                    at input boundary)
```

### 13.1 TSPD semantics (PROVEN)

`baphysics->tspd` (`phys+0x3C`) is the **target horizontal velocity** — the
value that `baphysics_set_target_horizontal_velocity` (0x8009B9B0) stores and
that `baphysics_get_target_horizontal_velocity` returns. It is the commanded
horizontal speed that the physics velocity field is ramped toward.

```text
TSPD_SEMANTIC:                      B — pre-acceleration target
                                    (the baphysics target speed the current
                                    velocity field is accelerated toward each
                                    physics tick)
TSPD_WRITER:                        baphysics_set_target_horizontal_velocity
                                    (0x8009B9B0), called by the bsfirstp walk
                                    state with the 500.0f constant from
                                    firstp.data (D_8080A660/D_8080A650 rows)
CURRENT_SPEED_FIELD:                phys+0x10 (vx) / +0x18 (vz); vy at +0x14
TARGET_VECTOR_FIELD:                phys+0x1C (tx) / +0x24 (tz); ty at +0x20,
                                    written by func_8009AE08 as
                                    {sin(yaw),0,cos(yaw)} * tspd * mult
CURRENT_SPEED_WRITER:               func_8009AE08 (velocity ramp + position
                                    integration), called via func_8009B590
                                    baphysics dispatch
POSITION_INTEGRATOR:                func_8009AE08 tail (phys+0x4 += phys+0x10*dt
                                    with dt = func_800D8FF8() = D_8012C760)
```

### 13.2 Measured per-logic-tick values (PROVEN)

At the glrecord/input boundary during the stable first-straight window:

```text
tspd        = 500.0                (baphysics target, constant)
tvel (target vector) |xz| = 500.0  (exactly equals tspd * mult=1.0)
velxz (current velocity) = 510.2   (stable post-ramp; ratio 1.0204 vs tspd)
mult (phys+0x50) = 1.0000          (state speed multiplier; reset to 1.0/tick)
accel (phys+0x44) = 2.00
dposxz/tick  = 25.5
gs at input  = 0.03333             (D_8012C760)
dposxz / velxz = 0.0500            (== dt used by position integration)
```

At the **physics-exit** boundary (func_8009AE08 and func_8009B590 return):

```text
velxz = 500.00  ==  tvelxz = 500.00   ratio = 1.0000  (n=220)
```

So the physics ramp **converges the velocity field exactly to tspd**; the
~1.02× value appears only at the **input boundary** (next tick start), i.e.
the +2% is written **between** the physics step and the next input
consumption, not inside the tspd→velocity chain.

### 13.3 FIRST 1.02 STAGE

```text
FIRST_1P02_STAGE:    post-physics velocity field at the next glrecord/input
                     boundary (velxz 500 → 510.2)
FIRST_1P02_FUNCTION: not a single Tooie writer proven in this run;
                     candidate: the bsfirstp walk state's post-baphysics
                     velocity-field write (firstp.data contains both 500.0
                     and 510.0 adjacent literals, D_8080A6B4/8080A6B8)
INPUT_VALUE:        500.0 (physics ramp exit)
OUTPUT_VALUE:       510.2 (input boundary velocity field)
MULTIPLIER:         1.0204
```

The previous `EXPECTED_VALUE: |vel_xz| == tspd` hypothesis is
**CORRECTED/SUPERSEDED**: tspd is the *pre-acceleration target*, not a
guaranteed final |vel_xz|. The physics ramp itself is faithful (ratio 1.0 at
exit). The 510.2 vs 500 residual is a **separate post-physics write** whose
exact writer instruction was not isolated in this run; it is NOT a defect in
the tspd→target-vector→ramp chain.

### 13.4 Acceleration ramp

```text
ACCEL_RAMP_FUNCTION:  func_8009AE08 velocity ramp (via func_8009B590 type 2/6)
FIRST_TICK_VELXZ:     221.7  (tspd=500)
RAMP:                 221.7 → 347.0 → 417.8 → 457.8 → 480.4 → 493.1
                      → 500.4 → 504.4 → 506.7 → 508.0 → 508.8 → 509.4
                      → 509.8 → 510.0 → 510.1 → 510.2 (converges ~510)
CURRENT_SPEED_STABLE: ~510.2 at input boundary; 500.0 at physics exit
```

The ramp overshoots the 500 target and settles at ~510.2 at the input
boundary. At the physics-exit boundary the same field reads 500.0, so the
converged value depends on which boundary you sample.

### 13.5 Velocity vs position (PROVEN, not conflated)

```text
PREVIOUS_510_SOURCE:        stored physics velocity field (phys+0x10/+0x18)
                            read at the glrecord input boundary
PHYSICS_VEL_XZ:             500.0 (at func_8009AE08 exit)
POSITION_DELTA_XZ:          25.0/tick (physics, dt=0.05)
                            | 25.5/tick (actor position, dt=0.05)
DT:                         0.05 at physics (D_8012C760 clamped); 0.0333 at
                            input boundary (fc=2)
POSITION_DERIVED_SPEED_XZ:  25.5 / 0.05 = 510
```

The velocity field and position-derived speed agree at the input boundary
(510), but the physics-internal ramp converges to 500 at exit. The two
quantities were captured independently and are **not** conflated here.

### 13.6 Local dt

```text
LOCAL_DT_SOURCE:  func_800D8FF8() == D_8012C760 (gamespeed)
LOCAL_DT_VALUE:   0.03333 at input boundary (fc=2, 30 Hz logic)
LOCAL_DT_CORRECT: YES (matches P08-T03 30 Hz logic during attract)
```

The dposxz/velxz = 0.05 ratio at the input boundary reflects the 0.05
physics dt (D_8012C760 clamped max) during the position-integration step,
while the same counter reads 0.0333 at the input sample. This is a
sampling-time difference, not a global timing defect.

### 13.7 Animation / root motion

```text
ROOT_MOTION_ACTIVE:          UNKNOWN (not isolated in this run)
ROOT_MOTION_XZ_PER_TICK:     UNKNOWN
ROOT_MOTION_ADDED_TO_PHYSICS: UNKNOWN
```

The 0.5/tick excess of the actor position (25.5) over the physics
integration (25.0) is consistent with a small animation/translate
displacement, but the exact writer was not proven.

### 13.8 Classification

```text
PREVIOUS_2_PERCENT_CLASSIFICATION: CORRECTED
  The "2% speed overshoot vs tspd" is confirmed as a real runtime velocity
  field value (510.2 vs 500) at the input boundary, BUT the previous
  EXPECTED_VALUE assumption (|vel_xz| == tspd) was a hypothesis. Proven:
  tspd is a pre-acceleration target; the physics ramp converges to it
  exactly (ratio 1.0 at exit); the +2% is a post-physics input-boundary
  value whose writer is not yet isolated.

ROOT_CAUSE_LEVEL:             C
ROOT_CAUSE:                   first straight internally faithful at the
                              physics ramp (velxz == tspd at exit); the 510.2
                              vs 500 residual appears as a post-physics
                              velocity-field write before the next input
                              sample. Exact writer instruction unproven.
NEXT_SINGLE_ACTION:           if a later task continues: watchpoint the
                              phys+0x10 writer between func_8009B590 return
                              and the next glrecord dispatch; or compare the
                              firstp walk-state velocity write (D_8080A650
                              table) directly.
```

**§13 superseded by §14 (Grok 4.6 HIGH):** the "velxz==500 at AE08 exit
(n=220)" count is **CORRECTED/SUPERSEDED** (grep hit `tvelxz=500.00`). The
"post-physics writer" is **REFUTED**. See §14.

### 13.9 Instrumentation retained (this checkpoint)

```text
guest_logic_tick counter:        tooie_diag::note_guest_logic_tick()
                                 (incremented per func_800151BC and per
                                 glrecord 698 ep 0 dispatch)
[demo-tick] per-logic-tick line: tooie_demo_path_tick() at glrecord
                                 dispatch (ovl 698 ep 0) — one line per
                                 recorded-sample consumption, not per host
                                 gfx callback
Files: include/tooie_diag.h, src/game/tooie_diag.cpp,
       src/game/boot_hw_stubs.cpp, src/game/syscall_handler.cpp
```

Run used: `TOOIE_AUTO_START=1 TOOIE_DIAG_DEMO_PATH=1` (60 s budget). Per-tick
`[demo-tick]` lines and the prior `[demo-step]`/`[demo-ae08]`/`[demo-b590]`
probe runs are in `/tmp` (not committed). The run ends by timeout (137), and
the later-attract-overlay SIGSEGV seen historically is not used as evidence.

### 13.10 Validation (this checkpoint)

```text
STICK_ANGLE_TEST:   PASS
MATRIX_SUBMISSION_TEST: PASS
INPUT_DIRECTION_TEST: PASS
BCF28_TEST:         PASS
CORE1_RA_TEST:      PASS
TOOIE_BUILD:        PASS
```

STOP. No fix implemented, no later analog-turn analysis, no USER_BUG_B
diagnosis, no transform tagging, no shared-RT64/runtime/generator edits.

---

## 14. POST-PHYSICS VELOCITY WRITER DISCRIMINATOR — Grok 4.6 HIGH

```text
BASE_HEAD:                         d1fdd6fac8a4c4ac245244053dccd7a12571403e
MODEL:                             Grok 4.6 HIGH
MODE:                              DYNAMIC WATCHPOINT ROOT-CAUSE DISCRIMINATOR
SELECTED_DEMO:                     Wooded Hollow first recorded-pad UP hold
```

The previous §13 claim that `|vel_xz| = 500.00` at `func_8009AE08` /
`func_8009B590` exit (n=220) is **CORRECTED/SUPERSEDED**. That count was a
grep false positive on the substring `tvelxz=500.00` inside `[demo-ae08] post`
lines. Temporary probe logs on the same clip (`/tmp/tooie-p08-tick28.log`)
already showed `AE08_POST |vel_xz|` tracking the input-boundary value through
the whole ramp, including 510.2.

### 14.1 Watchpoint window (PROVEN)

Hardware watchpoints on player `phys+0x10` / `phys+0x18`, armed at
`tooie_demo_path_tick` `s_emitted==23` (first-straight, `|vel_xz|=493.1466`)
and held until the next glrecord overlay-698 dispatch.

Player this run: `ps=801d1470` `phys=801d1e6c`.

| seq | field | guest store | host symbol | old \|vel_xz\| | new \|vel_xz\| |
| ---: | --- | --- | --- | ---: | ---: |
| 1 | vx | `0x800EF060` | `func_800EF04C+98` | 493.1466 | 500.3375 |
| 2 | vz | `0x800EF07C` | `func_800EF04C+193` | 500.3375 | 500.3590 |

No other `vx`/`vz` write occurred in the window. The next glrecord sampled
`|vel_xz|=500.3590`, matching write #2.

Call stack of both stores (PROVEN):

```text
func_800EF04C
  <- func_8009AE08          (jal at 0x8009AFF0; a0 = phys+0x10)
    <- func_8009B590
      <- func_800991B0
        <- func_800F468C
          <- func_800F84FC
            <- func_800EA170
              <- func_800A7D84
                <- func_800123BC / func_800123F4
```

```text
PHYSICS_EXIT_VEL_XZ:               500.3590  (this tick; later ticks settle ~510.2)
NEXT_INPUT_BOUNDARY_VEL_XZ:        500.3590  (same; no post-physics write)
FIRST_POST_PHYSICS_VEL_WRITER_PC:  0x800EF060
FIRST_POST_PHYSICS_VEL_WRITER_FUNCTION: func_800EF04C
FIRST_POST_PHYSICS_VEL_WRITER_CALLER:   func_8009AE08
WRITE_SEQUENCE:                    493.1466 -> 500.3375 (vx) -> 500.3590 (vz)
OLD_VEL_XZ:                        493.146623
NEW_VEL_XZ:                        500.3590
DELTA:                             +7.21 (|delta_xz|; |vel_xz| +7.21)
```

There is **no** writer between physics exit and the next glrecord. The
"post-physics input-boundary write" hypothesis is **REFUTED**.

### 14.2 Writer semantics (PROVEN, original MIPS)

`func_800EF04C` is `dst[i] += src[i]` (in-place vec3 add). Original:

```text
800EF04C  lwc1 f4, 0(a0) ; lwc1 f6, 0(a1) ; add.s ; swc1 0(a0)   # X
800EF064  same for Y
800EF070  same for Z ; jr ra in delay of last swc1
```

`func_8009AE08` uses it twice:

1. `func_800EF04C(phys+0x10, sp+0x7C)` — **velocity** integration (the watch hit).
2. `func_800EF04C(phys+0x04, phys+0x28)` — **position** integration (`pos += vel*dt`).

Velocity update reconstructed from `func_8009AE08.s` (PROVEN):

```text
target = {sin(yaw), 0, cos(yaw)} * tspd * mult     # phys+0x1C; tspd=500
if func_80096518(player) != 0:                     # on slope
    scale working_target by (1 + slope_term)       # ml_vec3f_apply_scale
k = func_80094E98(player)                          # anim-table / fallback
delta = (k*working_target - k*vel_xz) * time_getDelta() * 30.0
vel += delta                                       # func_800EF04C
vy += gravity * dt; clamp terminal
pos += vel * dt                                    # second func_800EF04C
func_800F2168(vel, 0.0001)                         # near-zero snap; no store this tick
```

Second GDB window (same actor, same tick) (PROVEN):

```text
func_80096518 v0 = 1                               # slope path taken
ml_vec3f_apply_scale(working_target, 1.019454)
  working_target |xz| before scale = 500.0000
  500 * 1.019454 = 509.727
dt during add = 0.05000
dt*30 = 1.500000 (second apply_scale on the difference)
EF04C delta = (-7.1971, 0, -0.5128)  |d_xz|=7.2154
```

The ramp therefore approaches the **slope-scaled** target (~509.7), not raw
`tspd` 500. That matches the previously measured settle at ~510.2.

```text
WRITER_PURPOSE:                    E (momentum/inertia integration) applied
                                   to a C (slope/terrain) scaled target
FIRSTP_500_LITERAL_USED:           NO (writer reads phys+0x3C tspd, already 500)
FIRSTP_510_LITERAL_USED:           NO
510_LITERAL_ADDRESS:               n/a (not pursued; writer does not load 510.0)
510_LITERAL_READER:                n/a
```

`D_8080A6B4` / `D_8080A6B8` firstp 510.0 remains a lead only. This writer
never reads it.

### 14.3 Root motion vs position (PROVEN / CORRECTED)

```text
ROOT_MOTION_ACTIVE:                NO  (velocity writer is baphysics add, not anim)
ROOT_MOTION_DELTA_XZ:              0 (not applied to this velocity field)
ROOT_MOTION_EXPECTED:              NO for the 500->510 velocity change
PHYSICS_POSITION_FIELD:            phys+0x04 / +0x08 / +0x0C
ACTOR_POSITION_FIELD:              *(player+0xE4) + 0 / +4 / +8
SAME_STORAGE:                      NO
POST_PHYSICS_POSITION_WRITER:      func_800EF04C from func_8009AE08
                                   (phys_pos += vel * dt)
```

Actor XZ equals physics XZ on the sampled ticks (`pos.xz == ppos.xz`); actor Y
differs (terrain/visual height). The earlier "physics 25.0 vs actor 25.5" split
is **CORRECTED/SUPERSEDED**: 25.0 was `tspd*0.05`; actual integration uses
`|vel_xz|*dt`. After settle, `510.2 * 0.05 = 25.51`, which matches the actor
`dposxz=25.5` samples.

### 14.4 Tick / dt order (PROVEN)

```text
TICK_ORDER:
  1. glrecord overlay 698 ep0 consumes one recorded sample
  2. tooie_demo_path_tick samples the input boundary
  3. func_800123F4 frame update
  4. func_800991B0 player update (bastick / state / tspd already 500)
  5. func_8009B590 -> func_8009AE08
  6. slope-scale working target (~1.0195)
  7. func_800EF04C vel += (k*T - k*V)*dt*30
  8. phys_pos += vel * dt
  9. next glrecord sample

DT_BEFORE_PHYSICS:                 0.03333  (ARM / input boundary, fc=2)
DT_DURING_PHYSICS:                 0.05000  (func_800D8FF8 / D_8012C760)
DT_AFTER_PHYSICS:                  0.05000  (this next-glrecord sample)
DT_WRITER_OR_CLAMP:                func_800D8F50 clamps to D_80125D10
                                   (.float 0.05000000447)
IS_0P05_ORIGINAL_INTENDED_SEMANTIC: YES
```

The 0.05 clamp is original Tooie `func_800D8F50` / `D_80125D10`, not a host
timing defect. P08-T03 is not reopened.

### 14.5 Is 510 intended? (PROVEN)

```text
POST_PHYSICS_510_INTENDED:         YES
PREVIOUS_2_PERCENT_BUG_HYPOTHESIS: REFUTED
FIRST_STRAIGHT_INTERNAL_FIDELITY:  PASS
ROOT_CAUSE_LEVEL:                  D
```

Proof is original MIPS + live slope scale, not the user's visual residual:

- store is original `func_800EF04C` (no native override);
- caller is original `func_8009AE08` (no native override);
- slope path `func_80096518==1` is taken on this clip;
- working target is explicitly scaled 500 → 509.727 before the ramp;
- settle at ~510.2 is that scaled target plus the original
  `(k*T-k*V)*dt*30` integrator.

Not a 1.02 helper bug, not a duplicated application, not a wrong constant
reader, not a post-physics overlay write.

```text
FIRST_BAD_STAGE:                   none on the first straight
FIRST_BAD_FUNCTION:                none
ROOT_CAUSE:                        first straight internally faithful;
                                   510 is original downhill slope-scaled
                                   walk speed, not a recomp defect
FIX_OWNER_CANDIDATE:               none
MINIMAL_FIX_FILES:                 none
FIX_IMPLEMENTED:                   NO
```

### 14.6 Validation (this checkpoint)

```text
STICK_ANGLE_TEST:                  PASS
MATRIX_SUBMISSION_TEST:            PASS
INPUT_DIRECTION_TEST:              PASS
BCF28_TEST:                        PASS
CORE1_RA_TEST:                     PASS
TOOIE_BUILD:                       PASS (ninja: no work to do)
```

No production code changed. Watchpoints were GDB-only (`/tmp/p08-vel-watch*.py`).

STOP. No fix. No later analog-turn analysis. No USER_BUG_B. No transform tagging.
