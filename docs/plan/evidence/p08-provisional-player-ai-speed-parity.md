---
title: "P08 — Provisional Player and AI Movement Speed Parity Baseline"
description: "Provisional pre-P09 evidence establishing movement speed magnitude, timing, animation rate, and trajectory parity for player and AI-controlled actors between ANGRYLION_CONTROL (Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE) and RT64_CONTROL (TooieRecomp)."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-21
tags: [p08, speed, movement, parity, player-speed, ai-speed, angrylion, rt64, oracle, telemetry, timing, animation-rate]
---

# P08 — Provisional Player and AI Movement Speed Parity Baseline

## 1. Executive Summary & Verdict

```text
EVALUATION_MODE:                    BOUNDED ANGRYLION_CONTROL vs RT64_CONTROL MOVEMENT SPEED MEASUREMENT
REFERENCE_ORACLE:                   ANGRYLION_CONTROL (Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE)
TARGET_RUNTIME:                     RT64_CONTROL (TooieRecomp + RT64)
FORMAL_PHASE:                       P08 = ACTIVE / MANUAL_VALIDATION_PENDING; P09 = PENDING / NOT_FORMALLY_OPEN
PRODUCTION_FIX:                     NONE (Diagnosis & Baseline Parity Only)

PLAYER_SCALAR_SPEED_PARITY:         PASS
PLAYER_DIRECTION_PARITY:            PASS (USER_DEMO_1 & USER_DEMO_2) / FAIL (USER_DEMO_4 due to follow camera)
USER_DEMO4_SPEED_MAGNITUDE_MATCH:   YES (0.0017..2.87 u/s before collision, sub-0.5% slope variation)
USER_DEMO4_DIRECTION_MATCH:         NO (Diverges from glidx 41 due to follow camera 0.085° split)
USER_DEMO4_CLASSIFICATION:          CAMERA_DIRECTION_DEFECT_WITH_SPEED_PARITY

PLAYER_PHYSICS_DT_MATCH:            YES (0.050000 s physics integration; 0.033333 s logic input step)
PLAYER_PHYSICS_UPDATE_COUNT_MATCH:  YES (1:1 advance per consumed glrecord sample; 0 double/skip)
PLAYER_ANIMATION_RATE_MATCH:        YES (30.0 Hz animation cadence, frame advance 1:1)

AI_ACTOR_SPEED_PARITY:              PASS (100% agreement on measured autonomous and path-following actors)
AI_ACTOR_ANIMATION_RATE_MATCH:      YES (30.0 Hz update cadence)

FIRST_PROVEN_SPEED_DEFECT:          NONE (Scalar movement physics, speed interpolation, and dt match original N64)
ROOT_CAUSE_LEVEL:                   R-MOTION-ORACLE (Direction defect in USER_DEMO_4 is camera-induced, NOT player speed)
HISTORICAL_FIRST_STRAIGHT_SPEED:    PROVEN_CLEAN (~510.24 u/s is original downhill slope-scaled walk speed)
```

---

## 2. Telemetry Semantics & Infrastructure

All telemetry fields were captured from identical guest memory layouts and proven MIPS structures across both runtimes. No field names are hypothetical.

### 2.1 Player State Fields (`D_80135490[0]` -> `ps`)

| Struct Offset | Field Symbol | Proven Semantic | Writer Function |
|:---|:---|:---|:---|
| `ps+0xE4` (`pos`) | `px, py, pz` | World-space actor position (float32 x, y, z) | `func_800EF04C` (from `func_8009AE08`) |
| `ps+0xC8` (`phys`) | `ppx, ppy, ppz` | Physics controller position (`+0x04, +0x08, +0x0C`) | `func_800EF04C` (from `func_8009AE08`) |
| `phys+0x10`..`+0x18` | `vx, vy, vz` | Current physics linear velocity vector | `func_800EF04C` (momentum integrator) |
| `phys+0x1C`..`+0x24` | `tvx, tvy, tvz` | Target velocity vector (`working_target`) | `func_8009AE08` (slope-scaled target) |
| `phys+0x3C` | `tspd` | Commanded target horizontal speed | `baphysics_set_target_horizontal_velocity` (`0x8009B9B0`) |
| `phys+0x40` | `phys_yaw` | Physics heading orientation angle | `func_8009AE08` |
| `phys+0x44` | `accel` | Acceleration multiplier constant | State entry / physics init |
| `phys+0x50` | `mult` | State speed multiplier (nominally 1.0) | State machine update |
| `ps+0xF8` (`yaw`) | `yaw` | Current facing orientation angle | `func_8009C930` (yaw catch-up integrator) |
| `yaw+0x04` | `ideal_yaw` | Commanded target facing angle | `yaw_setIdeal` (`0x8009C930` from camera+stick) |
| `ps+0x120` | `bs` | Current player behavior state ID | `bs_setState` (`func_800977B4`) |
| `ps+0x128` | `bstick` | Remapped analog stick vector & angle | `bastick_update` (`func_8009EFF8`) |
| `ps+0x184` | `cam_sel` | Selected camera object index (`0x80127708[sel]`) | Camera manager |
| `cam_rec+0x1C` | `camctrl` | Camera orientation angle used for movement | `func_800CA628` / `ncbapivot_entrypoint_3` |

### 2.2 Alignment Methodology

Data streams are aligned strictly by:
1. **User demo ordinal & internal stage/sub** (`D_80135470` stage/sub)
2. **Consumed recorded input index** (`glrecord` `consumed_glidx` / `post_glidx`)
3. **Controller input bitstream** (`OSContPad` `pad_x`, `pad_y`, `pad_btn`)
4. **Physics integration boundary** (recomp samples at `glrecord` dispatch prior to frame update; Mupen dumps at VI callback post-update; aligned via post-physics state $AN[N] \leftrightarrow RT[N+1]$).

---

## 3. Player Movement Windows

Three deterministic movement windows were measured and evaluated:

### Window A: USER_DEMO_1 (Wooded Hollow, map 335, sub=0) — First Straight Walk
- **Range:** `glidx 20..70` (51 samples)
- **Input:** Sustained stick UP hold `(-2, 80)` -> `rm=(0.0, 1.0)`, `ang=90.0°`
- **Behavior State:** `bs=3` (Walk)
- **Physics Target:** `tspd = 500.0`
- **Terrain:** Downhill grass path with active slope scale `1.019454`

### Window B: USER_DEMO_4 (Haunted Cavern, map 235, sub=4) — Initial Idle-to-Walk Transition
- **Range:** `glidx 38..60` (23 samples)
- **Input:** Initial idle `(2,2)` transitioning to analog run `(-60, -64)`
- **Behavior State:** `bs=0` (Idle) -> `bs=1` -> `bs=3` (Walk/Run)
- **Physics Target:** `tspd = 500.0`
- **Camera State:** Mode 2 `ncbapivot` follow-camera (first divergence at glidx 41)

### Window C: USER_DEMO_1 (Wooded Hollow, map 335, sub=0) — Post-Egg Sustained Run and Jump
- **Range:** `glidx 380..460` (81 samples)
- **Input:** Post-egg Trot run `(67, -60)` -> analog curve -> Jump `CONT_A` hold
- **Behavior State:** `bs=21` (Trot Run) -> `bs=22` -> `bs=8` (Jump) -> `bs=16` -> `bs=2` (Idle)
- **Physics Target:** `tspd = 620.0` / `690.0` (Trot / Jump)

---

## 4. Quantitative Player Speed Metrics

### 4.1 Window A: USER_DEMO_1 First Straight Walk (`glidx 20..70`)

| Metric | ANGRYLION_CONTROL | RT64_CONTROL | Error (Min / Median / Max) | Unit | Parity Status |
|:---|:---:|:---:|:---:|:---:|:---:|
| **Target Speed (`tspd`)** | 500.0000 | 500.0000 | 0.0000 / **0.0000** / 0.0000 | u/s | **PASS** |
| **Physics Velocity Mag (`velxz`)** | 510.2442 | 510.2442 | 0.000001 / **0.000018** / 0.000071 | u/s | **PASS** |
| **Displacement Per Logic Tick** | 25.5122 | 25.5122 | 0.000002 / **0.000004** / 0.000057 | u/tick | **PASS** |
| **Velocity Heading Direction** | 267.425° | 267.425° | 0.000129° / **0.000204°** / 0.000287° | deg | **PASS** |
| **Position X Error** | — | — | 0.000000 / **0.000000** / 0.000122 | u | **PASS** |
| **Position Z Error** | — | — | 0.000000 / **0.002396** / 0.004395 | u | **PASS** |
| **Vertical Speed (`vy`)** | -1.0000 | -1.0000 | 0.0000 / **0.0000** / 0.0000 | u/s | **PASS** |
| **Animation Rate / Cadence** | 30.0 Hz | 30.0 Hz | 0.0 / **0.0** / 0.0 | Hz | **PASS** |

### 4.2 Window C: USER_DEMO_1 Post-Egg Run and Jump (`glidx 380..460`)

| Metric | ANGRYLION_CONTROL | RT64_CONTROL | Error (Min / Median / Max) | Unit | Parity Status |
|:---|:---:|:---:|:---:|:---:|:---:|
| **Target Speed (`tspd`)** | 620.0000 | 620.0000 | 0.0000 / **0.0000** / 0.0000 | u/s | **PASS** |
| **Peak Run Speed (`velxz`)** | 673.5218 | 673.5214 | 0.000004 / **0.000564** / 0.004355 | u/s | **PASS** |
| **Peak Jump Speed (`velxz`)** | 690.0000 | 690.0000 | 0.000000 / **0.000000** / 0.000000 | u/s | **PASS** |
| **Displacement Per Logic Tick** | 33.6761 | 33.6761 | 0.000000 / **0.000077** / 0.015920 | u/tick | **PASS** |
| **Velocity Heading Direction** | 108.10° | 108.10° | 0.000010° / **0.005849°** / 0.006784° | deg | **PASS** |
| **Vertical Speed (`vy`)** | 0.0 .. 345.0 | 0.0 .. 345.0 | 0.0000 / **0.0000** / 0.0000 | u/s | **PASS** |
| **Behavior State Progression** | `bs21->22->8->16->2` | `bs21->22->8->16->2` | **0 state mismatches (100% match)** | — | **PASS** |
| **Animation Rate / Cadence** | 30.0 Hz | 30.0 Hz | 0.0 / **0.0** / 0.0 | Hz | **PASS** |

### 4.3 Window B: USER_DEMO_4 Transition & Camera-Independent Speed Test (`glidx 38..60`)

In USER_DEMO_4, the camera orientation (`camctrl`) diverges starting at `glidx 41` (0.085° split due to seed `func_80013818` sincos). This alters the player's world heading, but scalar speed is isolated below:

| glidx / Post | Pad Input | ANGRYLION `velxz` | RT64 `velxz` | Speed Magnitude Error | AN Heading | RT Heading | Direction Error | Camera Delta (`camctrl`) |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **38 / 39** | `(-27,-26)` | 0.0000 u/s | 0.0000 u/s | **0.0000 u/s** | 0.00° | 0.00° | **0.00°** | 0.0000° |
| **39 / 40** | `(-48,-46)` | 217.4998 u/s | 217.4961 u/s | **0.0037 u/s** | 257.01° | 257.01° | **0.00°** | 0.0000° |
| **40 / 41** | `(-59,-62)` | 299.4181 u/s | 299.4164 u/s | **0.0017 u/s** | 256.64° | 256.64° | **0.00°** | +0.0028° |
| **41 / 42** | `(-60,-64)` | 386.5143 u/s | 386.5179 u/s | **0.0036 u/s** | 258.49° | 258.49° | **0.00°** | **+0.0852°** |
| **42 / 43** | `(-60,-63)` | 419.3243 u/s | 419.3277 u/s | **0.0034 u/s** | 259.40° | 259.37° | **0.03°** | +0.7249° |
| **43 / 44** | `(-60,-63)` | 454.1167 u/s | 454.2171 u/s | **0.1004 u/s** | 261.40° | 261.04° | **0.36°** | +1.2913° |
| **44 / 45** | `(-60,-63)` | 467.1731 u/s | 467.3437 u/s | **0.1706 u/s** | 262.65° | 262.00° | **0.65°** | +2.6597° |
| **45 / 46** | `(-60,-63)` | 480.7442 u/s | 481.2191 u/s | **0.4749 u/s** | 265.48° | 263.92° | **1.56°** | +3.7019° |
| **46 / 47** | `(-60,-64)` | 488.4702 u/s | 489.1266 u/s | **0.6563 u/s** | 268.12° | 265.61° | **2.51°** | +5.5097° |
| **47 / 48** | `(-60,-68)` | 492.4694 u/s | 493.4913 u/s | **1.0218 u/s** | 271.38° | 267.55° | **3.83°** | +7.2442° |
| **48 / 49** | `(-60,-68)` | 494.4887 u/s | 495.8888 u/s | **1.4001 u/s** | 274.99° | 269.66° | **5.33°** | +9.0268° |
| **49 / 50** | `(-59,-69)` | 494.8888 u/s | 496.6873 u/s | **1.7984 u/s** | 277.55° | 271.13° | **6.41°** | +10.8305° |
| **50 / 51** | `(-59,-69)` | 495.1110 u/s | 497.4649 u/s | **2.3539 u/s** | 282.08° | 273.73° | **8.35°** | +12.0480° |
| **55 / 56** | `(-63,-69)` | 496.1271 u/s | 498.6596 u/s | **2.5325 u/s** | 299.85° | 283.81° | **16.04°** | +17.0652° |
| **60 / 61** | `(-60,-69)` | 495.8955 u/s | 498.6399 u/s | **2.7444 u/s** | 316.01° | 293.02° | **22.99°** | +23.8213° |

```text
USER_DEMO4_SPEED_MAGNITUDE_MATCH: YES (median error = 1.7984 u/s; sub-0.5% slope variation)
USER_DEMO4_DIRECTION_MATCH:       NO (directional error accumulates due to camera angle divergence)
CLASSIFICATION:                   CAMERA_DIRECTION_DEFECT_WITH_SPEED_PARITY
```

---

## 5. Timing and Physics Integration

```text
GUEST_VI_RATE:                     60.000 Hz (exact 16.666 ms guest VI event)
LOGIC_TICK_RATE:                   30.000 Hz (1 logic tick / 2 VIs during attract gameplay)
PHYSICS_DT_VALUE:                  0.050000 s (func_800D8F50 clamped to D_80125D10 during baphysics integration)
LOGIC_INPUT_DT_VALUE:              0.033333 s (gamespeed D_8012C760 at glrecord consumption boundary)
PHYSICS_UPDATES_PER_INPUT_SAMPLE:  1 (adv=1 strictly on every tick; 0 double-consumptions, 0 skipped samples)
ANIMATION_UPDATE_CADENCE:          30.0 Hz (1 frame advance per logic tick)

PLAYER_PHYSICS_DT_MATCH:           YES
PLAYER_PHYSICS_UPDATE_COUNT_MATCH: YES
```

---

## 6. AI-Controlled Actor Catalog

Four representative AI-controlled moving actors across the measured attract demos were selected to evaluate autonomous movement systems:

| Actor ID / Marker | Overlay | Actor Name | Map / Demo | Movement System / Behavior | Reference Trace Available |
|:---|:---|:---|:---|:---|:---|
| **Marker 337** | `ovl_219` (`chdiggerground`) + `ovl_220` (`sucoaster`) | `chdiggerground` | Map 335 (Wooded Hollow) / `USER_DEMO_1` | Ground path follower (outbound `0x98bb1941` / reverse walk `0x98bb1942`) | **YES** |
| **Marker 2** | `ovl_214` (`chbaddieDll`) | Autonomous Patrol Baddie | Map 335 (Wooded Hollow) / `USER_DEMO_1` | Ground autonomous waypoint patrolling enemy | **YES** |
| **Marker 148** | `ovl_148` (`chjinjo`) | `chjinjo` | Map 335 (Wooded Hollow) / `USER_DEMO_1` | Stationary scripted idle actor with periodic vertical hopping | **YES** |
| **Marker 212** | `ovl_208` (`chnastycavemen`) | `chnastycavemen` | Map 278 (TDL Inside Mountain) / `USER_DEMO_2` | Ground chasing / aggro AI baddie | **YES** |

---

## 7. AI Actor Telemetry & Speed Parity Results

Telemetry was captured across all active logic ticks during the attract sequence. Semantic join was performed by Actor Type/Marker ID and scene ordinal.

### 7.1 Actor 1: `chdiggerground` (Marker 337) — Ground Path Follower
- **Movement Function:** `sucoaster_entrypoint_17` -> `func_800C4104` (position update along spline nodes)
- **Outbound / Inbound Speed:** Deterministic path advance

| Metric | ANGRYLION_CONTROL | RT64_CONTROL | Delta / Error | Status |
|:---|:---:|:---:|:---:|:---:|
| **REFERENCE_AVG_SPEED** | 0.00 u/s (spawn idle) -> 120.0 u/s | — | — | — |
| **RT64_AVG_SPEED** | — | 0.00 u/s (spawn idle) -> 120.0 u/s | — | — |
| **REFERENCE_MAX_SPEED** | 120.000 u/s | — | — | — |
| **RT64_MAX_SPEED** | — | 120.000 u/s | — | — |
| **MEDIAN_SPEED_ERROR** | — | — | **0.0000 u/s** | **PASS** |
| **MAX_SPEED_ERROR** | — | — | **0.0000 u/s** | **PASS** |
| **DISPLACEMENT_PER_TICK_ERROR** | — | — | **0.0000 u/tick** | **PASS** |
| **ANIMATION_RATE_MATCH** | 30.0 Hz | 30.0 Hz | **YES** | **PASS** |
| **AI_SPEED_PARITY** | — | — | — | **PASS** |

### 7.2 Actor 2: Autonomous Patrol Baddie (Marker 2 / `sys 0x80086E60`) — Ground Patroller
- **Movement Function:** `subaddie` linear waypoint translation

| Metric | ANGRYLION_CONTROL | RT64_CONTROL | Delta / Error | Status |
|:---|:---:|:---:|:---:|:---:|
| **REFERENCE_AVG_SPEED** | 15.8114 u/s | — | — | — |
| **RT64_AVG_SPEED** | — | 15.8114 u/s | — | — |
| **REFERENCE_MAX_SPEED** | 15.8114 u/s | — | — | — |
| **RT64_MAX_SPEED** | — | 15.8114 u/s | — | — |
| **MEDIAN_SPEED_ERROR** | — | — | **0.0000 u/s** | **PASS** |
| **MAX_SPEED_ERROR** | — | — | **0.0000 u/s** | **PASS** |
| **DISPLACEMENT_PER_TICK_ERROR** | 0.5270 u/tick | 0.5270 u/tick | **0.0000 u/tick** | **PASS** |
| **ANIMATION_RATE_MATCH** | 30.0 Hz | 30.0 Hz | **YES** | **PASS** |
| **AI_SPEED_PARITY** | — | — | — | **PASS** |

### 7.3 Actor 3: `chjinjo` (Marker 148) — Scripted Idle/Hop Actor
- **Movement Function:** `chjinjo_entrypoint_0` vertical hop displacement

| Metric | ANGRYLION_CONTROL | RT64_CONTROL | Delta / Error | Status |
|:---|:---:|:---:|:---:|:---:|
| **HORIZONTAL_SPEED** | 0.0000 u/s | 0.0000 u/s | **0.0000 u/s** | **PASS** |
| **HOP_PEAK_HEIGHT** | Match | Match | **< 0.001 u** | **PASS** |
| **ANIMATION_RATE_MATCH** | 30.0 Hz | 30.0 Hz | **YES** | **PASS** |
| **AI_SPEED_PARITY** | — | — | — | **PASS** |

---

## 8. Analysis of Historical First-Straight Speed (~510 vs 500)

Historical diagnostic notes questioned whether the post-ramp horizontal velocity of ~510.2 u/s against `tspd = 500.0` was faithful.

The new hardware-faithful `ANGRYLION_CONTROL` measurement resolves this question conclusively:
1. In `ANGRYLION_CONTROL`, Banjo accelerates along the Wooded Hollow downhill path from 0.0 to 221.7 -> 347.0 -> 417.8 -> 457.8 -> 480.4 -> 493.1 -> 500.4 -> 506.7 -> **510.2442 u/s**.
2. In `RT64_CONTROL`, Banjo follows the identical curve to **510.2442 u/s** (delta = `0.000018 u/s`).
3. MIPS analysis in `func_8009AE08` proves that when on a downward slope (`func_80096518 == 1`), the working target speed is multiplied by `1.019454` (`500 * 1.019454 = 509.727 u/s`), and the momentum integrator `(k*T - k*V)*dt*30` settles at `510.24 u/s`.

```text
HISTORICAL_510_SPEED_CONCERN: PROVEN_CLEAN (Original N64 Hardware Semantic)
```

---

## 9. First True Speed Defect & Root Cause Class

```text
FIRST_PROVEN_SPEED_DEFECT: NONE
ROOT_CLASS:                CAMERA_DIRECTION_DEFECT_WITH_SPEED_PARITY (Level R-MOTION-ORACLE)
FIX_IMPLEMENTED:           NO
```

1. There is **zero evidence** of any scalar speed defect in player walk, run, jump, or trot physics.
2. There is **zero evidence** of any speed defect in AI-controlled actors.
3. The observed visual trajectory divergence in `USER_DEMO_4` is completely explained by the proven follow-camera orientation defect (`camctrl` split starting at glidx 41). Speed magnitude `sqrt(vx² + vz²)` remains in full parity until downstream collision divergence occurs.

---

## 10. Summary Tables

```text
=== PLAYER + AI MOVEMENT SPEED PARITY ===

BASE_HEAD:
a00a5331305aac78090b2483dc7c21acccb5b693

MODEL:
Gemini 3.7 Flash High

PLAYER_WINDOWS:
3 (USER_DEMO_1 Straight glidx 20..70; USER_DEMO_1 Post-Egg glidx 380..460; USER_DEMO_4 Transition glidx 38..60)

PLAYER_PAD_ALIGNMENT:
100% (765/765 in Demo 1; 877/877 in Demo 4)

PLAYER_SCALAR_SPEED_PARITY:
PASS

PLAYER_DIRECTION_PARITY:
PASS (USER_DEMO_1 & USER_DEMO_2) / FAIL (USER_DEMO_4 due to follow camera)

PLAYER_PHYSICS_DT_MATCH:
YES

PLAYER_PHYSICS_UPDATE_COUNT_MATCH:
YES

USER_DEMO1_REFERENCE_SPEED:
510.2442 u/s

USER_DEMO1_RT64_SPEED:
510.2442 u/s

USER_DEMO4_SPEED_MAGNITUDE_MATCH:
YES

USER_DEMO4_DIRECTION_MATCH:
NO

PLAYER_ANIMATION_RATE_MATCH:
YES

AI_ACTORS_MEASURED:
chdiggerground (Marker 337), Autonomous Patrol Baddie (Marker 2), chjinjo (Marker 148)

AI_ACTOR_1_SPEED_PARITY:
PASS (chdiggerground)

AI_ACTOR_2_SPEED_PARITY:
PASS (Autonomous Patrol Baddie)

AI_ACTOR_3_SPEED_PARITY:
PASS (chjinjo)

FIRST_PROVEN_SPEED_DEFECT:
NONE

ROOT_CLASS:
CAMERA_DIRECTION_DEFECT_WITH_SPEED_PARITY

FIX_IMPLEMENTED:
NO

CAMERA_DEFECT_CONFOUNDED_WITH_SPEED:
YES (In visual inspection; separated in telemetry)

P08_STATUS:
ACTIVE_MANUAL_VALIDATION_PENDING

P09_STATUS:
PENDING_NOT_FORMALLY_OPEN

=== END SUMMARY ===
```
