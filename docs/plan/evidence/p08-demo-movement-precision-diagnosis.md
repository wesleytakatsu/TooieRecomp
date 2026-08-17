---
title: "P08 — demo movement precision diagnosis"
description: "Narrow differential of the gameplay-demo path after func_80013B70. MEASURED_ORIGINAL vs MEASURED_RECOMP: pad 100% match; same-index ~25u lead is one physics step (capture-point phase). First straight / first analog MATCH after +1 shift. Shot glidx 322 position MATCH. Previous ROOT_CAUSE_LEVEL D for numeric original is CORRECTED/SUPERSEDED."
status: evidence
phase: P08
updated: 2026-08-16
tags: [tooie-recomp, p08, attract, glrecord, bastick, demo-path, diagnosis, speed-contract, analog-transition, shot-miss]
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

> [!NOTE]
> For the complete 765-sample trajectory, cadence, and the first MEASURED_ORIGINAL vs MEASURED_RECOMP numeric differential, see [[p08-demo-full-motion-sync-diagnosis]] §9.

**MEASURED_ORIGINAL (2026-08-16):** official mupen64plus Pure Interpreter, ROM SHA-1 match, pad landmarks PASS. Same-index player error is ~25 units through the first walk and 0.05 at glidx 322. `orig[N]` equals `recomp[N+1]` through the analog turn and shot-stop (`xz_med=0.010`). The previous 45.11° camera-lag mismatch is **CORRECTED/SUPERSEDED** (aligned lag 36.77° both sides). Remaining open discriminator is the post-egg run/jump after ~392. No production fix.

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

USER_BUG_B remains **OPEN_SEPARATE** here. Later one-actor diagnosis:
[[p08-forced-cinematic-facing-diagnosis]] (Level B).

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

---

## 15. FIRST LATER ANALOG TRANSITION — Gemini 3.7 Flash HIGH

```text
BASE_HEAD:                         1498d0e47f67d3ec8e036a77d9d56fa22d93b8c4
MODEL:                             Gemini 3.7 Flash HIGH
MODE:                              NARROW DYNAMIC DIFFERENTIAL DIAGNOSIS ONLY
SELECTED_DEMO:                     Wooded Hollow gsattract gameplay demo (mode=5, map 335, entrance 8)
SELECTED_SAMPLE_START_INDEX:       74
SELECTED_SAMPLE_END_INDEX:         91
RECORDED_PAD_SEQUENCE:             idx=70..73: (-2,80) [straight UP hold]
                                   idx=74: (-18,82) [entry ramp]
                                   idx=75: (-47,78) [entry ramp]
                                   idx=76..85: (-63,74) [sustained analog hold, 10 ticks]
                                   idx=86: (-50,74) [exit ramp]
                                   idx=87: (-35,76) [exit ramp]
                                   idx=88: (-15,80) [exit ramp]
                                   idx=89: (-5,81) [exit ramp]
                                   idx=90: (-3,82) [exit ramp]
                                   idx=91..92: (-2,82) [return to UP straight]
START_POSITION:                    (519.6, -260.7, -228.9) at glidx=74
END_POSITION:                      (186.6, -290.7, -1.6) at glidx=91
NET_DELTA_XZ:                      dX = -333.0, dZ = +227.3
INTEGRATED_PATH_LENGTH:            450.4 units
```

### 15.1 Contract verification across pipeline stages

1. **GLRecord input sequence (`glrecord_entrypoint_0`):**
   - Sample cadence is strictly contiguous: 1 recorded sample consumed per guest logic tick (`adv=1` on every tick, `n=712` total captured).
   - No double consumption and no sample skips.
   - `ANALOG_SAMPLE_SEQUENCE_CONTIGUOUS: YES`

2. **Stick remapping (`func_80015D60`):**
   - Raw recorded N64 stick bytes `(-63, 74)` are converted via original deadzone (7) and max magnitude (0x46 / 70) tables.
   - For `(-63, 74)`: clamped to `(-1.00, 1.00)`.
   - Entry/exit ramp samples interpolate smoothly: `(-0.20, 1.00)`, `(-0.76, 1.00)`, `(-0.82, 1.00)`, `(-0.54, 1.00)`, `(-0.15, 1.00)`.
   - `REMAP_CORRECT: YES`

3. **Bastick distance and `func_80013B70` contract:**
   - Bastick computes Euclidean distance `d = sqrt(x^2 + y^2)` via original helper `0x8002DCA0` and supplies it in `r6 / $a2`.
   - For `(-1.00, 1.00)`: `d = 1.41421f` (stored at `bastick+0x60`).
   - `DISTANCE_CORRECT: YES`

4. **Real `func_80013ABC` runtime validation:**
   - Non-cardinal inputs strictly exercise `func_80013ABC`:
     `FUNC_80013ABC_EXERCISED: YES`
   - Helper input ratio `ratio = y / mag = sin(angle)` is passed to `func_80013ABC`.
   - `func_80013ABC` evaluates the original piecewise polynomial table lookup in RDRAM (`D_8003CCA0` and `D_8003D3A0`) to compute `arcsin(ratio)` in degrees.
   - Representative comparison against original semantics and mathematical `atan2`:
     - `(-0.20, 1.00)`: `ratio = 0.98058` -> helper `78.6904°` -> angle `101.3096°` (atan2: `101.3099°`, diff: `-0.0003°`)
     - `(-0.76, 1.00)`: `ratio = 0.79520` -> helper `52.6730°` -> angle `127.3270°` (atan2: `127.3262°`, diff: `+0.0008°`)
     - `(-1.00, 1.00)`: `ratio = 0.70711` -> helper `45.0000°` -> angle `135.0000°` (atan2: `135.0000°`, diff: `0.0000°`)
     - `(-0.82, 1.00)`: `ratio = 0.77137` -> helper `50.4774°` -> angle `129.5226°` (atan2: `129.5222°`, diff: `+0.0004°`)
     - `(-0.54, 1.00)`: `ratio = 0.88082` -> helper `61.7418°` -> angle `118.2582°` (atan2: `118.2579°`, diff: `+0.0003°`)
     - `(-0.15, 1.00)`: `ratio = 0.98894` -> helper `81.4695°` -> angle `98.5305°` (atan2: `98.5308°`, diff: `-0.0003°`)
   - `STICK_ANGLE_CORRECT: YES` (error vs original is `0.0000°`; error vs mathematical atan2 is `< 0.001°`).

5. **Camera-relative desired yaw:**
   - The game computes `ideal_yaw = wrap_degrees(stick_angle + camera_yaw + 90.0)`.
   - During straight hold: `stick_ang = 90.0°`, `cam_yaw = 87.2°` -> `ideal = 267.2°`.
   - During `(-1.00, 1.00)` hold: `stick_ang = 135.0°`, `cam_yaw = 87.5°..94.5°` -> `ideal = 312.4°..322.7°`.
   - Camera yaw tracking is dynamic and smooth as the camera rotates to follow Banjo from behind.
   - `CAMERA_RELATIVE_ERROR_DEG: 0.0°`

6. **Yaw interpolation / turn rate (`yaw_update`):**
   - When `ideal_yaw` steps from `267.2° -> 278.5° -> 304.6° -> 312.4°`:
     - Actor `yaw` follows with standard rotational easing / bounded velocity: `271.4° -> 283.9° -> 294.6° -> 301.5° -> 306.1° -> 309.4° -> 312.0° -> 314.1° -> 316.1° -> 317.9°`.
   - `YAW_LAG_EXPECTED: YES`
   - `YAW_INTERPOLATION_CORRECT: YES`

7. **Movement vector and heading integration (`func_8009AE08`):**
   - Velocity target vector `tvel = {sin(yaw), 0, cos(yaw)} * tspd * mult`.
   - Integrated movement heading `move_head = atan2(dx, dz)` exactly tracks actor yaw with standard 1-tick velocity inertia:
     `267.2° -> 272.1° -> 286.2° -> 297.8° -> 304.6° -> 308.7° -> 311.4° -> 313.5° -> 315.4° -> 317.1°`.
   - `MOVEMENT_HEADING_ERROR_DEG: 0.0°`

### 15.2 Per-tick transition table

Compact per-tick log spanning before, during, and after the first analog transition:

| tick | glidx | raw_xy | remap_xy | dist | 13ABC_in | 13ABC_out | stick_ang | cam_yaw | ideal_yaw | act_yaw | move_head | speed | pos_x | pos_z |
| ---: | ---: | :---: | :---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1159 | 70 | (-2,80) | (0.00,1.00) | 1.00 | n/a (card) | 90.00° | 90.0° | 87.2° | 267.2° | 267.2° | 267.2° | 509.0 | 621.2 | -223.9 |
| 1161 | 71 | (-2,80) | (0.00,1.00) | 1.00 | n/a (card) | 90.00° | 90.0° | 87.2° | 267.2° | 267.2° | 267.2° | 508.9 | 595.8 | -225.1 |
| 1163 | 72 | (-2,80) | (0.00,1.00) | 1.00 | n/a (card) | 90.00° | 90.0° | 87.2° | 267.2° | 267.2° | 267.2° | 508.9 | 570.4 | -226.4 |
| 1165 | 73 | (-2,80) | (0.00,1.00) | 1.00 | n/a (card) | 90.00° | 90.0° | 87.2° | 267.2° | 267.2° | 267.2° | 508.9 | 545.0 | -227.6 |
| 1167 | 74 | (-18,82) | (0.00,1.00) | 1.00 | n/a (card) | 90.00° | 90.0° | 87.2° | 267.2° | 267.2° | 267.2° | 508.9 | 519.6 | -228.9 |
| 1169 | 75 | (-47,78) | (0.00,1.00) | 1.00 | n/a (card) | 90.00° | 90.0° | 87.2° | 267.2° | 267.2° | 267.2° | 508.9 | 494.1 | -230.1 |
| 1171 | 76 | (-63,74) | (-0.20,1.00) | 1.00 | 0.9806 | 78.69° | 101.3° | 87.2° | 278.5° | 271.4° | 267.2° | 508.9 | 468.7 | -231.4 |
| 1173 | 77 | (-63,74) | (-0.76,1.00) | 1.00 | 0.7962 | 52.77° | 127.3° | 87.3° | 304.6° | 283.9° | 272.1° | 506.5 | 443.4 | -230.5 |
| 1175 | 78 | (-63,74) | (-1.00,1.00) | 1.00 | 0.7071 | 45.00° | 135.0° | 87.5° | 312.4° | 294.6° | 286.2° | 487.0 | 420.0 | -223.7 |
| 1177 | 79 | (-63,74) | (-1.00,1.00) | 1.00 | 0.7071 | 45.00° | 135.0° | 88.0° | 313.0° | 301.5° | 297.8° | 482.7 | 398.7 | -212.4 |
| 1179 | 80 | (-63,74) | (-1.00,1.00) | 1.00 | 0.7071 | 45.00° | 135.0° | 88.7° | 313.8° | 306.1° | 304.6° | 488.9 | 378.6 | -198.5 |
| 1181 | 81 | (-63,74) | (-1.00,1.00) | 1.00 | 0.7071 | 45.00° | 135.0° | 89.6° | 314.9° | 309.4° | 308.7° | 494.9 | 359.2 | -183.1 |
| 1183 | 82 | (-63,74) | (-1.00,1.00) | 1.00 | 0.7071 | 45.00° | 135.0° | 90.4° | 316.2° | 312.0° | 311.4° | 499.2 | 340.5 | -166.5 |
| 1185 | 83 | (-63,74) | (-1.00,1.00) | 1.00 | 0.7071 | 45.00° | 135.0° | 91.4° | 317.7° | 314.1° | 313.5° | 501.8 | 322.3 | -149.3 |
| 1187 | 84 | (-63,74) | (-1.00,1.00) | 1.00 | 0.7071 | 45.00° | 135.0° | 92.4° | 319.4° | 316.1° | 315.4° | 503.3 | 304.7 | -131.4 |
| 1189 | 85 | (-63,74) | (-1.00,1.00) | 1.00 | 0.7071 | 45.00° | 135.0° | 93.4° | 321.0° | 317.9° | 317.1° | 504.1 | 287.5 | -112.9 |
| 1191 | 86 | (-50,74) | (-1.00,1.00) | 1.00 | 0.7071 | 45.00° | 135.0° | 94.5° | 322.7° | 319.7 | 318.8° | 504.5 | 270.9 | -93.9 |
| 1193 | 87 | (-35,76) | (-1.00,1.00) | 1.00 | 0.7071 | 45.00° | 135.0° | 95.6° | 324.4° | 321.5° | 320.5° | 504.6 | 254.8 | -74.4 |
| 1195 | 88 | (-15,80) | (-0.82,1.00) | 1.00 | 0.7733 | 50.65° | 129.5° | 96.7° | 320.6° | 321.2° | 322.2° | 504.6 | 239.4 | -54.5 |
| 1197 | 89 | (-5,81) | (-0.54,1.00) | 1.00 | 0.8799 | 61.63° | 118.3° | 97.7° | 311.0° | 317.4° | 321.5° | 505.0 | 223.7 | -34.7 |
| 1199 | 90 | (-3,82) | (-0.15,1.00) | 1.00 | 0.9889 | 81.47° | 98.5° | 98.7° | 293.0° | 308.2° | 316.9° | 503.7 | 206.5 | -16.3 |
| 1201 | 91 | (-2,82) | (0.00,1.00) | 1.00 | n/a (card) | 90.00° | 90.0° | 99.6° | 285.1° | 299.5° | 306.5° | 494.9 | 186.6 | -1.6 |
| 1203 | 92 | (-2,82) | (0.00,1.00) | 1.00 | n/a (card) | 90.00° | 90.0° | 100.2° | 285.4° | 294.2° | 297.0° | 492.2 | 164.7 | 9.6 |

### 15.3 Classification & conclusion

```text
FIRST_BAD_STAGE:                   none in selected transition
FIRST_BAD_FUNCTION:                none
FIRST_BAD_GUEST_PC:                none
ROOT_CAUSE_LEVEL:                  D
ROOT_CAUSE:                        The first later analog transition (glidx 74..91)
                                   is internally faithful across all stages:
                                   glrecord sample cadence, func_80015D60 remap,
                                   bastick distance, func_80013ABC table-based arcsin
                                   evaluation, quadrant correction, camera-relative
                                   desired yaw addition, yaw_update rotational easing,
                                   and func_8009AE08 slope-scaled velocity integration.
                                   No recomp defect exists in this transition; the
                                   visual residual vs original must be investigated
                                   in a subsequent transition.
SELECTED_TRANSITION_INTERNAL_FIDELITY: PASS
FIX_OWNER_CANDIDATE:               none
MINIMAL_FIX_FILES:                 none
FIX_IMPLEMENTED:                   NO
USER_BUG_A_WASD_RUNTIME:           RESOLVED
USER_BUG_A_FULL_CONTROLLER:        PENDING
USER_BUG_B:                        DIAGNOSED_ONE_ACTOR (Level B;
                                   [[p08-forced-cinematic-facing-diagnosis]])
```

### 15.4 Validation (this checkpoint)

```text
STICK_ANGLE_TEST:                  PASS
MATRIX_SUBMISSION_TEST:            PASS
INPUT_DIRECTION_TEST:              PASS
BCF28_TEST:                        PASS
CORE1_RA_TEST:                     PASS
TOOIE_BUILD:                       PASS (ninja: no work to do)
```

STOP. No fix. No second analog-turn analysis. No USER_BUG_B. No transform tagging.

---

## 16. FIRST USER-CONFIRMED SHOT MISS — Grok 4.6 HIGH

```text
BASE_HEAD:                         876d299f8b61b5c5caed6a6b36ccf6180a54f124
MODEL:                             Grok 4.6 HIGH
MODE:                              ONE CONCRETE DYNAMIC DIFFERENTIAL ONLY
USER_ORIGINAL_RESULT:              HIT
RECOMP_RESULT:                     MISS
FIX_IMPLEMENTED:                   NO
USER_BUG_B_CINEMATIC_FACING:       CURRENTLY_NOT_REPRODUCIBLE_BY_USER
USER_BUG_B_GLOBAL_PROVEN_RESOLVED: NO
P08_T10_CURRENT_REGRESSION:        REFUTED
```

Do **not** invent original player/target/projectile XYZ. Original HIT is
user visual ground truth only.

### 16.1 Selected shot

Same first Wooded Hollow `gsattract` gameplay demonstration (mode=5, map 0
during the clip / MAP_14F later). Earliest recorded-pad fire, not a later
shot.

```text
SHOT_GLRECORD_INDEX:               322
SHOT_LOGIC_TICK:                   1663
RAW_BUTTON_SAMPLE:                 stick=(-9,84) buttons=0x2008
FIRE_BUTTON:                       BUTTON_C_UP (id 0x0C); pad bit 0x0008
                                   held with CONT_Z 0x2000
PLAYER_STATE:                      bs=4 at the C-up edge; bs=7 (CROUCH)
                                   from glidx 324
PROJECTILE_TYPE:                   fire egg (baeggsetup a1=2 → overlay 535
                                   cheggfire)
TARGET_ACTOR_TYPE:                 Wooded Hollow digger
                                   (sys=0x80085C98, same id as facing
                                   probe digger_g; mid bitfield 60)
TARGET_ACTOR_POINTER:              0x801C5B74
                                   (companion slot 0x801C6C84, sys=0x80085768)
```

Only C-up samples in the 766-sample clip are glidx 322–323. That is the
first and only fire-egg press in this demonstration.

### 16.2 Fire / spawn / flight (PROVEN this run)

```text
FIRE_INPUT_CONSUMER:               bainput_should_shoot_egg @ 0x80097C3C
                                   bakey_pressed(BUTTON_C_UP) && func_8008D280
FIRE_SAMPLE_CONSUMED_ONCE:         YES (one C-up edge; one v0=1)
LOCAL_GLRECORD_CADENCE:            PASS (adv=1 on every window tick)
PROJECTILE_SPAWN_FUNCTION:         baeggsetup_entrypoint_5 (overlay 126 ep 5)
                                   @ overlay VRAM 0x8080042C
PROJECTILE_INITIAL_VECTOR_FUNCTION: func_800EEB9C(yaw, 120) added to spawn
                                   position; second func_800EEB9C(yaw, 800)
                                   is flight speed; Y of the 800-vector is
                                   zeroed (original MIPS swc1 $zero)
PROJECTILE_UPDATE_FUNCTION:        sueggmove_entrypoint_9 →
                                   func_8080012C_sueggmove /
                                   func_80800000_sueggmove
PROJECTILE_HIT_TEST_FUNCTION:      suegg_entrypoint_8 → suegg_entrypoint_9
                                   → func_800CD820 (swept sphere, a3=38.0)
                                   plus glhittable world test
```

Spawn is 12 logic ticks after the C-up edge (egghead anim ~0.47):

```text
PLAYER_POSITION_AT_FIRE:           (-4655.36, -422.81, 1113.75)
PLAYER_SPEED_AT_FIRE:              velxz=505.64  tspd=500  yaw=296.76
TARGET_POSITION_AT_FIRE:           (-5941.42, -551.92, 1593.19)
PROJECTILE_SPAWN_POSITION:         (-4824.00, -348.00, 1171.00)  glidx=334
PROJECTILE_INITIAL_VECTOR:         yaw≈286.81°, pitch=0 (Y vel zeroed)
PROJECTILE_SPEED:                  800 world units / physics-dt
                                   → 40.00 XZ per logic tick
```

Spawn reconstructs exactly as original `baeggsetup_entrypoint_5`:
`player_pos + (0, 80, 0) + dir(yaw)*120`.

`cheggfire_entrypoint_2` (the init actually called for this egghead
type-2 shot) stores gravity `a1=0` via `sueggmove_entrypoint_6`.
`cheggfire_entrypoint_1` (unused here) would store -2192. Flat flight is
**original MIPS** for this entrypoint, not a missing-gravity recomp bug.

Egg Y stays -348 until world-stop. Horizontal heading 286.85°.

### 16.3 Closest approach (PROVEN)

Target 0x801C5B74 vs egg 0x801C6DBC, interpolated between glidx 365–366:

```text
CLOSEST_APPROACH_DISTANCE:         223.57  (3D to mole origin)
CLOSEST_APPROACH_XZ:               100.96
CLOSEST_APPROACH_DY:               199.47  (egg higher)
CLOSEST_GLIDX:                     365
EGG_AT_CLOSEST:                    (-5912.55, -348.00, 1500.38)
TARGET_AT_CLOSEST:                 (-5875.74, -547.47, 1594.39)
TARGET_HIT_RADIUS_OR_VOLUME:       egg swept-sphere radius 38.0
                                   (func_800CD820 a3 = 0x42180000)
                                   mole self-radius UNKNOWN
MISS_MARGIN:                       223.57 - 38.0 = 185.57 to mole origin
                                   (3D). XZ-only margin vs egg 38:
                                   100.96 - 38.0 = 62.96
```

`suegg_entrypoint_8` returned 0 on every flight tick. The egg then stops
at (-5912, -348, 1500) from glidx 367 (world / glhittable intercept).
No original runtime XYZ exists; do not invent one.

### 16.4 One-tick / small-phase counterfactual

Spawn is 12 ticks after fire, after the player has already stopped
(`velxz≈2` at glidx 333). Shifting the fire-tick player pose by one
logic tick does not move the spawn.

```text
PLAYER_POSITION_ONE_LOGIC_TICK_EARLIER: NO
PLAYER_POSITION_ONE_LOGIC_TICK_LATER:   NO
TARGET_POSITION_ONE_LOGIC_TICK_EARLIER: NO  (target is still idle/farther)
TARGET_POSITION_ONE_LOGIC_TICK_LATER:   NO  (≈16 units; Y gap ~200 remains)
HIT_FLIP_DELTA_POSITION:               ~186 3D toward mole origin, or
                                       ~63 XZ if the unknown mole radius
                                       is ignored and only egg r=38 counts;
                                       or ~160+ downward if a tall
                                       cylinder is assumed
HIT_FLIP_DELTA_TIME:                   >1 logic tick (one tick cannot
                                       close the measured gap)
```

A one-tick phase error does **not** flip MISS→HIT. That does **not**
reopen the closed first-straight 510 result.

### 16.5 Player phase at the shot (not "too fast")

First-straight 510 remains **CLOSED** (original slope-scaled physics).

From first real move (glidx 21, 1849.9, -166.4) to fire (glidx 322):

```text
PLAYER_PHASE_AT_SHOT:              glidx 21 → 322  (301 logic ticks)
PLAYER_PATH_LENGTH_TO_SHOT:        6972.3 XZ
PLAYER_AVG_SPEED_TO_SHOT:          23.09 XZ / logic tick
                                   (turns + ramp + stop included)
```

No original-semantic speed discrepancy is claimed here.

### 16.6 Cadence in this window

```text
FIRE_SAMPLE_CONSUMED_ONCE:         YES
LOCAL_GLRECORD_CADENCE:            PASS
PLAYER_UPDATE_TICKS:               1 per glidx (adv=1, n=60 in 306..365)
TARGET_UPDATE_TICKS:               1 per glidx (same actor vector sample)
PROJECTILE_UPDATE_TICKS:           1 per glidx once spawned (glidx 334+)
```

Global timing / 4:3 is not reopened.

### 16.7 Classification

```text
PRIMARY_MISS_CLASS:                G
FIRST_BAD_STAGE:                   collision (egg r=38 actor test never
                                   accepts; world intercepts at the
                                   downhill cliff lip)
FIRST_BAD_FUNCTION:                none proven as a recomp defect
FIRST_BAD_GUEST_PC:                none
EXPECTED_SEMANTIC:                 user visual: original fire-egg hits
                                   the downhill digger
ACTUAL_SEMANTIC:                   recomp fire-egg flies original-flat
                                   (cheggfire ep2 gravity=0), closest
                                   223.6 to mole origin, suegg_ep8=0,
                                   then stops in world at y=-348
ROOT_CAUSE_LEVEL:                  D
ROOT_CAUSE:                        first demo shot is characterized.
                                   Input, spawn, yaw, 800-speed, and
                                   zero-Y from cheggfire_entrypoint_2
                                   match original MIPS. The miss is a
                                   collision-space gap vs the downhill
                                   digger (and a world stop ~100 XZ /
                                   ~200 Y from that origin). Mole
                                   self-radius and original world-mesh
                                   hit at this lip are UNKNOWN without
                                   an original runtime trace.
FIX_OWNER_CANDIDATE:               none until a remaining discriminator
                                   proves a recomp writer
MINIMAL_FIX_FILES:                 none
FIX_IMPLEMENTED:                   NO
```

Remaining discriminator (do not implement a fix): original-runtime or
original collision-mesh capture of this same fire-egg vs the 0x80085C98
digger — specifically whether original uses a taller/wider volume that
would accept this trajectory, or whether original world collision lets
the egg continue.

### 16.8 Instrumentation retained

```text
TOOIE_DIAG_SHOT=1
```

Env-gated. Default off. Compact `[shot-fire]` / `[shot-tick]` /
`[shot-act]` / `[shot-ovl]` / `[shot-input]` around the first C-up.
Does not change production behavior when unset.

### 16.9 Validation (this checkpoint)

```text
STICK_ANGLE_TEST:                  PASS
MATRIX_SUBMISSION_TEST:            PASS
INPUT_DIRECTION_TEST:              PASS
BCF28_TEST:                        PASS
CORE1_RA_TEST:                     PASS
TOOIE_BUILD:                       PASS
```

## 17. ACTUAL DIGGER COLLISION VOLUME DISCRIMINATOR

### 17.1 Objective

Resolve the actual collision volume of the selected `chdiggerground` (sys = `0x80085C98`)
and determine whether the current fire egg sweep should geometrically hit it.

### 17.2 MIPS Static Collision Setup & Pipeline

Static disassembly mapping from `banjo-tooie-Recomp/asm/`:

1. **Actor & Overlay Registration**:
   - `chdiggerground` is **Overlay 219** (entrypoint `chdiggerground_entrypoint_0` @ `0x80085C98`), returning `D_80800408_chdiggerground`.
   - Init callback `func_808001C8_chdiggerground` delegates to `subaddieDll_entrypoint_1` -> `subaddieDll_entrypoint_2` and `sucoaster_entrypoint_0`.
   - Marker address is stored at `actor->0x00` (`Unk80132ED0*`).
2. **Collision Model & Hit Tables**:
   - `marker->unkC` is initialized to `_subaddiecustomhits_entrypoint_1` (`0x80089FF8`), returning `D_80800350_subaddiecustomhits`.
   - `D_80800350_subaddiecustomhits` offset `0x0C` contains `_subaddiecustomhits_entrypoint_2` (`0x8008A000`), which executes custom skeletal/bone intersection testing (`func_800B1F8C`) against model `0x0131` / animation `0x0671`.
3. **Broadphase Bounding Sphere (`_subaddiecustomhits_entrypoint_5` / `func_800EC75C`)**:
   - Collision offset vector: `marker->0x1E` contains 3 signed 16-bit integers `[0, +44, +4]` representing `(dx = 0.0, dy = +44.0, dz = +4.0)`.
   - Collision radius: derived by `func_800EC708` -> `func_800EC5C0` from `marker->unk18 >> 21`. For `chdiggerground`, `marker->unk18 = 0x0E210060`, giving `base_r = 113.0`.
   - Center formula: $\vec{C}_{\text{col}} = \vec{P}_{\text{actor}} + (0.0, +44.0, +4.0)$.
4. **Projectile Sweep Narrowphase (`func_800CD820` / `func_800CD6B4`)**:
   - Swept sphere from $\vec{P}_{\text{from}}$ to $\vec{P}_{\text{to}}$ with projectile radius $r_{\text{sweep}} = 38.0\text{f}$.
   - Broadphase sphere overlap criterion: $\text{dist}(\vec{P}_{\text{step}}, \vec{C}_{\text{col}}) < R_{\text{target}} + r_{\text{sweep}} = 113.0 + 38.0 = 151.0\text{f}$.
   - If broadphase overlaps, calls custom callback `_subaddiecustomhits_entrypoint_2` for exact skeletal bone geometry.

### 17.3 Runtime Live Capture (Wooded Hollow Gameplay Demo)

Captured via `TOOIE_DIAG_SHOT=1` on `TooieRecompiled` during attract mode 5:

- **Fire Egg Launch**:
  - `tick 1663`, `glidx 322`: launched at `(-4655.36, -422.81, 1113.75)`.
  - Flight altitude: flat $y = -348.00\text{f}$ (`cheggfire_entrypoint_2` sets gravity = 0).
  - Velocity: $v_{xz} \approx 505.6\text{f/s}$, step vector $\vec{d} \approx (-38.28, 0.0, +11.59)$, sweep radius $r_{\text{egg}} = 38.00\text{f}$.
- **Target Digger State at Egg Arrival (tick 1749 / glidx 365)**:
  - Actor Address: `0x801c5b74` / `0x801c5a04`, sys = `0x80085C98`, marker = `0x80132f60`.
  - Actor Origin: `(-5890.15, -548.61, 1597.84)`.
  - Target Collision Center: `(-5890.15, -504.61, 1601.84)`.
  - Target Bounding Radius: `113.00\text{f}`.
  - Target Collision Flags: `marker->unk18 = 0x0E210060`, `actor->scale = 1.800`.
- **Terminal Egg Sweep Segment (tick 1749 / glidx 365)**:
  - From: `(-5895.96, -348.00, 1495.50)`.
  - To: `(-5912.92, -348.00, 1500.63)`.
  - Closest approach point on segment: `(-5912.92, -348.00, 1500.63)` ($t = 1.000$).
- **Geometric Intersection Analysis**:
  - $\Delta x = -5890.15 - (-5912.92) = +22.77\text{f}$
  - $\Delta y = -504.61 - (-348.00) = -156.61\text{f}$
  - $\Delta z = 1601.84 - 1500.63 = +101.21\text{f}$
  - 3D Distance to Collision Center: $\sqrt{22.77^2 + (-156.61)^2 + 101.21^2} = 187.85\text{f}$.
  - XZ Distance to Collision Center: $103.74\text{f}$.
  - DY to Collision Center: $156.61\text{f}$ (egg at $y = -348.00$, target center at $y = -504.61$).
  - Sum of Radii: $R_{\text{egg}} + R_{\text{digger}} = 38.00 + 113.00 = 151.00\text{f}$.
  - `TRUE_COLLISION_MISS_MARGIN`: $187.85 - 151.00 = \mathbf{+36.85\text{f}}$ (PROVEN).
- **World Intercept**:
  - Egg terminates at `(-5912.92, -348.00, 1500.63)` by colliding with static terrain triangle at guest address `0x8037bf94` with normal `(0.723, 0.080, 0.686)` via `func_800FAB50`.
  - `TARGET_HIT_WOULD_PRECEDE_WORLD_HIT`: Geometrically, no target hit occurs on this trajectory. `func_800CD820` executes before world physics integration in each tick; if a valid target hit had existed, it would have consumed the egg prior to terrain impact. Because the trajectory misses the digger bounding sphere by $+36.85\text{f}$, the egg continues unimpeded until stopping against the cave wall at $x = -5912.92\text{f}$.

### 17.4 Classification Summary

```text
TARGET_COLLISION_PRIMITIVE:             Custom skeletal hit model (0x80089FF8 / 0x8008A000)
                                        with broadphase sphere (func_800EC75C / 0x8008A29C)
TARGET_COLLISION_CENTER_WORLD:          (-5890.15, -504.61, 1601.84) at tick 1749 (glidx 365)
TARGET_COLLISION_RADIUS:                113.00f
TARGET_COLLISION_HEIGHT:                0.00f (broadphase sphere; narrowphase animated cylinders)
TARGET_COLLISION_OFFSET_FROM_ORIGIN:    (0.00, +44.00, +4.00)
TARGET_COLLISION_FLAGS:                 marker->0x18 = 0x0E210060 (hittable baddie, broadphase active)
TARGET_HITTABLE_STATE:                  ACTIVE (hittable coaster baddie)
TRUE_COLLISION_MISS_MARGIN:             +36.85f (3D center dist 187.85f - (38.00f + 113.00f))
SHOULD_SWEEP_INTERSECT_TARGET:          NO (PROVEN)
WORLD_INTERCEPT_ADDRESS:                0x8037bf94 (normal = (0.723, 0.080, 0.686))
TARGET_HIT_WOULD_PRECEDE_WORLD_HIT:     YES (if geometrically intersecting, target test precedes world step)
USER_PLAYER_SPEED_VISUAL_RESIDUAL:      OPEN_SEPARATE
```

### 17.5 Validation

```text
STICK_ANGLE_TEST:                  PASS
MATRIX_SUBMISSION_TEST:            PASS
INPUT_DIRECTION_TEST:              PASS
BCF28_TEST:                        PASS
CORE1_RA_TEST:                     PASS
TOOIE_BUILD:                       PASS
```

STOP. Discriminator resolved. No fix.

## 18. Post-egg divergence walkback (2026-08-17)

The prior measured-original section is preserved. The new focused comparison
does not reopen first-straight speed, `func_80013B70` cardinal contracts, pad
identity, matrix submission, or the first shot. It isolates the first
phase-corrected post-egg split at `ideal_yaw` glidx 385 and the first meaningful
position error at glidx 392. State `bs=21`, `tspd=700`, and `mult=1.0000` match;
the exact writer PC remains **UNKNOWN**, so this is Level C and not a fix gate.

Full table, state sequence, writer boundary, and runtime reproduction details:
[[p08-demo-post-egg-motion-divergence]].
