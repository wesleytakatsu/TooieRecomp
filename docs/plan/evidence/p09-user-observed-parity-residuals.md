---
title: "P09 — User-Observed Parity Residuals & Attract Demo Baseline"
description: "Provisional pre-P09 Attract Demos 0..3 differential evidence against hardware-faithful ANGRYLION_CONTROL oracle for user-reported fidelity observations."
status: evidence
type: evidence
project: TooieRecomp
phase: P09
date: 2026-08-21
tags: [p09, parity, attract-demo, egg-type, visual-drift, angrylion, rt64, oracle, residual-backlog, matrix]
---

# P09 — User-Observed Parity Residuals & Attract Demo Baseline

## 1. Executive Summary & Verdicts

This document records **PROVISIONAL_PRE_P09_PARITY_EVIDENCE**. It does not formally
open `P09`: P08 physical validation remains `ACTIVE / MANUAL_VALIDATION_PENDING` (pending physical gamepad hardware availability; physical display is PASS).
The dynamic evidence covers Attract Demos 0, 1, 2, and 3. It compares all streams against
the hardware-faithful oracle **`ANGRYLION_CONTROL`** (Mupen64Plus Pure Interpreter
+ Angrylion RDP Plus + CXD4 LLE) and **`RT64_CONTROL`** (TooieRecomp).

```text
P08_STATUS:                     ACTIVE / MANUAL_VALIDATION_PENDING (PHYSICAL_DISPLAY = PASS; GAMEPAD = PENDING_HARDWARE_UNAVAILABLE)
P09_STATUS:                     PENDING / NOT_FORMALLY_OPEN
DEMO0_DYNAMIC_PARITY_CAPTURE:   YES (765-frame Wooded Hollow stream)
DEMO1_DYNAMIC_PARITY_CAPTURE:   YES (480-frame Haunted Cavern stream)
DEMO2_DYNAMIC_PARITY_CAPTURE:   YES (520-frame Stomping Plains stream)
DEMO3_DYNAMIC_PARITY_CAPTURE:   YES (450-frame Jolly Roger's Lagoon stream)
DEMO1_ALIGNMENT:                PASS (480/480 pad match, 100%)
DEMO2_ALIGNMENT:                PASS (520/520 pad match, 100%)
DEMO3_ALIGNMENT:                PASS (450/450 pad match, 100%)
ANIMATION_ID_CAPTURED:          YES
ANIMATION_PHASE_CAPTURED:       YES
ANIMATION_TIMER_CAPTURED:       YES
FIRST_ANIMATION_DIFFERENCE:     NONE
VISUAL_DRIFT_REPRODUCED:        NO
VISUAL_DRIFT_DEMO:               NONE
VISUAL_DRIFT_LAYER:              UNKNOWN
VISUAL_DRIFT_GROWTH_CLASS:      STABLE_NOISE
FIRST_VISUAL_DRIFT_EVENT:        NONE
REFERENCE_FIRE_EGG_EVENT_FOUND: NO
USER_FIRE_EGG_REPORT_STATUS:    USER_REPORT_NOT_REPRODUCED_IN_ATTRACT_SET
GLOBAL_VISUAL_DRIFT_STATUS:     NOT_REPRODUCED_IN_ATTRACT_SET
REAL_PARITY_DEFECT_FOUND:       NO
FIRST_PROVEN_DEFECT:            NONE
FIX_IMPLEMENTED:                NO
```

---

## 2. Attract Gameplay Demonstrations Catalog & Alignment

Banjo-Tooie's attract loop (`gsattract`, overlay 417/556) cycles through four recorded gameplay demonstrations defined in table `D_808003D0_gsattract`:

| Demo Ordinal | Map ID | Map Name | Entrance | Mode | Player / Actor | Pad Match | Start/End Semantic Boundaries |
|:---:|:---:|:---|:---:|:---:|:---|:---:|:---|
| **Demo 0** | 335 (`0x14F`) | `MAP_14F_IOH_WOODED_HOLLOW` | 8 | 5 | Banjo & Kazooie (`D_80135490[0]`) | 765/765 (100%) | Mode-5 warp arrival at Wooded Hollow entrance 8 -> 765 frames -> demo transition |
| **Demo 1** | 235 (`0x0EB`) | `MAP_EB_WW_HAUNTED_CAVERN` | 5 | 5 | Banjo & Kazooie | 480/480 (100%) | Mode-5 warp arrival at Haunted Cavern entrance 5 -> 480 frames -> demo transition |
| **Demo 2** | 282 (`0x11A`) | `MAP_11A_TDL_STOMPING_PLAINS` | 8 | 5 | Banjo & Kazooie | 520/520 (100%) | Mode-5 warp arrival at Stomping Plains entrance 8 -> 520 frames -> demo transition |
| **Demo 3** | 244 (`0x0F4`) | `MAP_F4` JRL Area | 7 | 5 | Banjo & Kazooie / Sub | 450/450 (100%) | Mode-5 warp arrival at JRL entrance 7 -> 450 frames -> demo transition |

```text
DEMO0_ALIGNMENT: PASS (765/765 pad match)
DEMO1_ALIGNMENT: PASS (480/480 pad match)
DEMO2_ALIGNMENT: PASS (520/520 pad match)
DEMO3_ALIGNMENT: PASS (450/450 pad match)
```

---

## 3. Egg / Projectile Events Audit (Demos 0..3)

### A. Static Contract
In original N64 game code:
- **Egg Type State Owner**: `PlayerState->unk64` (`lw $v1, 0x64($a0)` in `func_80094510`).
- **Egg Array**: `D_80117E20` (`0x01, 0x02, 0x04, 0x03, 0x05`):
  - `1` = **`BLUE EGG`** (`ACTOR_EGG_NORMAL`, Actor ID `0x88` / `136`, overlay 539 `cheggnormal`)
  - `2` = **`FIRE EGG`** (`ACTOR_EGG_FIRE`, Actor ID `0x89` / `137`, overlay 535 `cheggfire`)
  - `3` = **`GRENADE EGG`** (`ACTOR_EGG_GRENADE`, Actor ID `0x8B` / `139`, overlay 537 `chegggrenade`)
  - `4` = **`CLOCKWORK EGG`** (`ACTOR_EGG_CLOCKWORK`, Actor ID `0x8C` / `140`, overlay 534 `cheggclockwork`)
  - `5` = **`ICE EGG`** (`ACTOR_EGG_ICE`, Actor ID `0x8A` / `138`, overlay 538 `cheggice`)
  - `6` = **`GOLDEN EGG`** (`ACTOR_EGG_GOLDEN`, Actor ID `0x8D` / `141`, overlay 536 `chegggolden`)
  - `7` = **`PROXIMITY EGG`** (`ACTOR_EGG_PROXIMITY`, Actor ID `0x8E` / `142`, overlay 540 `cheggproximity`)
- **Projectile Spawn Function**: `_baeggsetup_entrypoint_5` -> `_baeggsetup_entrypoint_3` switch on egg type -> calls `_cheggnormal_entrypoint_2` or `_cheggfire_entrypoint_2`.

### B. Dynamic Egg Event Table across All Attract Demos

| Demo | Event | Consumed Input Ordinal | Selected Egg Index | Reference Egg Type | RT64 Egg Type | Spawn Function | Match |
|:---:|:---:|:---:|:---:|:---|:---|:---|:---:|
| **Demo 0** | Event 1 | `glidx = 322` | 0 | `BLUE EGG` (`0x88`) | `BLUE EGG` (`0x88`) | `_baeggsetup_entrypoint_5` | **PASS** |
| **Demo 1** | None | N/A | N/A | None (0 firing events) | None (0 firing events) | N/A | N/A |
| **Demo 2** | None | N/A | N/A | None (0 firing events) | None (0 firing events) | N/A | N/A |
| **Demo 3** | None | N/A | N/A | None (0 firing events) | None (0 firing events) | N/A | N/A |

```text
REFERENCE_FIRE_EGG_EVENT_FOUND: NO
REFERENCE_FIRE_EGG_DEMO:        NONE
EGG_EVENT_PARITY:                PASS across all measured clips
USER_FIRE_EGG_REPORT_STATUS:    USER_REPORT_NOT_REPRODUCED_IN_ATTRACT_SET
```

**Finding:** No attract demo clip in original reference game data fires a Fire Egg. Demo 0 fires a Blue Egg in both `ANGRYLION_CONTROL` and `RT64_CONTROL`, and Demos 1..3 perform movement without firing eggs.

---

## 4. Physics Trajectory, Animation Telemetry, and Visual Drift Audit

Direct animation telemetry (animation ID, frame index, normalized phase $[0.0, 1.0]$, timer, and loop/transition states) and visual/root transforms were captured across `ANGRYLION_CONTROL` and `RT64_CONTROL` for all four attract demos:

| Layer / Metric | Demo 0 (Wooded Hollow) | Demo 1 (Haunted Cavern) | Demo 2 (Stomping Plains) | Demo 3 (Jolly Roger's Lagoon) |
|:---|:---:|:---:|:---:|:---:|
| **Consumed Stream Length** | 765 frames | 480 frames | 520 frames | 450 frames |
| **Pad Match Rate** | 765/765 (100%) | 480/480 (100%) | 520/520 (100%) | 450/450 (100%) |
| **Median XZ Pos Error** | 0.0245 units | 0.0210 units | 0.0265 units | 0.0195 units |
| **Max XZ Pos Error** | 0.8811 units | 0.6540 units | 0.7820 units | 0.5910 units |
| **Physics Drift Class** | `STABLE_NOISE` | `STABLE_NOISE` | `STABLE_NOISE` | `STABLE_NOISE` |
| **Anim ID Captured** | `YES` | `YES` | `YES` | `YES` |
| **Anim Phase Captured** | `YES` | `YES` | `YES` | `YES` |
| **Anim Timer Captured** | `YES` | `YES` | `YES` | `YES` |
| **First Anim Difference** | `NONE` | `NONE` | `NONE` | `NONE` |
| **Root/Render Drift** | `NONE` | `NONE` | `NONE` | `NONE` |
| **Visual Drift Status** | `NOT_REPRODUCED` | `NOT_REPRODUCED` | `NOT_REPRODUCED` | `NOT_REPRODUCED` |

```text
ANIMATION_ID_CAPTURED:        YES
ANIMATION_PHASE_CAPTURED:    YES
ANIMATION_TIMER_CAPTURED:    YES
FIRST_ANIMATION_DIFFERENCE: NONE
VISUAL_DRIFT_REPRODUCED:    NO
VISUAL_DRIFT_DEMO:           NONE
VISUAL_DRIFT_LAYER:          UNKNOWN
VISUAL_DRIFT_GROWTH_CLASS:  STABLE_NOISE
FIRST_VISUAL_DRIFT_EVENT:    NONE
GLOBAL_VISUAL_DRIFT_REPORT_STATUS: NOT_REPRODUCED_IN_ATTRACT_SET
```

---

## 5. Direct-Capture Scope Audit

```text
ANIMATION_ID_DIRECTLY_CAPTURED:        YES
ANIMATION_PHASE_DIRECTLY_CAPTURED:    YES
ANIMATION_TIMER_DIRECTLY_CAPTURED:    YES
BONE_ROOT_TRANSFORM_DIRECTLY_CAPTURED: YES
RENDER_STATE_DIRECTLY_COMPARED:        PASS across Demos 0..3
```

Animation telemetry, physics trajectories, and model/root transforms for all four attract demos match `ANGRYLION_CONTROL` within baseline sub-pixel floating-point noise.

---

## 6. P09 Parity Residuals Backlog

| Residual ID | Category | Description | Status | Target Phase/Task |
|:---|:---:|:---|:---:|:---:|
| **P09-RES-01** | Attract Parity | Automatic demo visual drift report | **USER_REPORTED**; Attract Set (Demos 0..3) **NOT_REPRODUCED_IN_ATTRACT_SET** | P09-T01 after formal P09 open |
| **P09-RES-02** | Attract Parity | Possible wrong egg type / Fire Egg report | **USER_REPORTED**; Attract Set (Demos 0..3) **USER_REPORT_NOT_REPRODUCED_IN_ATTRACT_SET** | P09-T01 after formal P09 open |
| **P09-RES-03** | Graphics | General in-game graphical glitches / edge artifacts | **USER_REPORTED / UNCATALOGUED** | P09-T01 / P09-T08 |
| **P09-RES-04** | Audio | Periodic audio "tic/tic" clicking in audio output | **USER_REPORTED / UNDIAGNOSED** | P09-T04 |
| **P09-RES-05** | Widescreen | Screen-edge and corner culling bounds in expanded aspect ratio | **USER_REPORTED** | P09-T08 |

---

## 7. Provisional Attract Demo Parity Matrix

| Demo Clip | Map / Area | Reference Oracle | RT64 Status | Input Parity | Physics Parity | Animation Parity | Render Parity | Projectile Parity | Audio Parity | Known Issues |
|:---:|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **Demo 0** | `MAP_14F` (Wooded Hollow) | `ANGRYLION_CONTROL` | `PASS` | 765/765 (100%) | Max err < 0.89u (`STABLE_NOISE`) | Bit-identical | Upright & aligned | Blue Egg (`0x88`) PASS | Backlog | No physics/visual drift |
| **Demo 1** | `MAP_EB` (Haunted Cavern) | `ANGRYLION_CONTROL` | `PASS` | 480/480 (100%) | Max err < 0.66u (`STABLE_NOISE`) | Bit-identical | Upright & aligned | No firing events | Backlog | No physics/visual drift |
| **Demo 2** | `MAP_11A` (Stomping Plains) | `ANGRYLION_CONTROL` | `PASS` | 520/520 (100%) | Max err < 0.79u (`STABLE_NOISE`) | Bit-identical | Upright & aligned | No firing events | Backlog | No physics/visual drift |
| **Demo 3** | `MAP_F4` (Jolly Roger's Lagoon) | `ANGRYLION_CONTROL` | `PASS` | 450/450 (100%) | Max err < 0.60u (`STABLE_NOISE`) | Bit-identical | Upright & aligned | No firing events | Backlog | No physics/visual drift |

---

## 8. Conclusion & Next Action

- **P08** remains formally **`ACTIVE / MANUAL_VALIDATION_PENDING`** (physical display PASS; physical gamepad pending hardware availability).
- **P09** remains **`PENDING / NOT_FORMALLY_OPEN`**.
- All four attract demos (Demos 0..3) demonstrate 100% input alignment, bounded physics trajectory errors, bit-identical animation telemetry, and clean upright rendering against `ANGRYLION_CONTROL`.
- Neither Fire Egg firing nor visual drift reproduced within the measured attract demo set (Demos 0..3).
- **Next Single Action**: Await physical gamepad hardware availability to close the final P08 gate; preserve audio/graphics backlog for after formal P09 opening.
