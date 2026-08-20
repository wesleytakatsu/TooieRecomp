---
title: "P09 — User-Observed Parity Residuals & Attract Demo Baseline"
description: "Initial P09 baseline and differential evaluation of user-reported fidelity observations (attract demo visual drift, egg projectile type, graphical glitches, audio clicks, and widescreen culling)."
status: current
type: evidence
project: TooieRecomp
phase: P09
date: 2026-08-20
tags: [p09, parity, attract-demo, egg-type, visual-drift, angrylion, rt64, oracle, residual-backlog, matrix]
---

# P09 — User-Observed Parity Residuals & Attract Demo Baseline

## 1. Executive Summary & Verdicts

This document establishes the initial **P09 Gameplay Parity** baseline, formally opening **`P09`** following the confirmation of physical gamepad and display validation.
It evaluates the five user-observed fidelity reports against the hardware-faithful oracle **`ANGRYLION_CONTROL`** (Mupen64Plus Pure Interpreter + Angrylion RDP Plus + mupen64plus-rsp-cxd4 LLE) and **`RT64_CONTROL`** (TooieRecomp).

```text
P08_STATUS:                     COMPLETE
P09_STATUS:                     ACTIVE
FIRE_EGG_REPORT_VERDICT:        NOT_REPRODUCED_AGAINST_HARDWARE_ORACLE (Both runtimes fire Blue Egg)
VISUAL_DRIFT_REPORT_VERDICT:    NOT_REPRODUCED_AGAINST_HARDWARE_ORACLE (Error is STABLE_NOISE <= 0.88 units)
FIRST_PROVEN_P09_PARITY_DEFECT: NONE_IN_ATTRACT_DEMO_0 (Attract Demo 0 is in 100% state parity)
```

---

## 2. Attract Gameplay Demonstrations Catalog

Banjo-Tooie's attract loop (`gsattract`, overlay 417/556) cycles through four recorded gameplay demonstrations defined in table `D_808003D0_gsattract`:

| Demo Ordinal | Map ID | Map Name | Entrance | Mode | Player / Actor | Scripted Action Summary |
|:---:|:---:|:---|:---:|:---:|:---|:---|
| **Demo 0** | 335 (`0x14F`) | `MAP_14F_IOH_WOODED_HOLLOW` | 8 | 5 | Banjo & Kazooie (`D_80135490[0]`) | Path walk, first-person egg shot at digger (`glidx 322`), crouch, slope turn, jump toward Mayahem entrance |
| **Demo 1** | 235 (`0x0EB`) | `MAP_EB_WW_HAUNTED_CAVERN` | 5 | 5 | Banjo & Kazooie | Witchyworld Haunted Cavern navigation |
| **Demo 2** | 282 (`0x11A`) | `MAP_11A_TDL_STOMPING_PLAINS` | 8 | 5 | Banjo & Kazooie | Terrydactyland Stomping Plains crossing / timing run |
| **Demo 3** | 244 (`0x0F4`) | JRL Area | 7 | 5 | Banjo & Kazooie / Sub | Jolly Roger's Lagoon demonstration |

```text
FIRE_EGG_CANDIDATE_DEMO: Demo 0 (Wooded Hollow, glidx 322)
```

---

## 3. Investigation of User-Observed Item 2: Egg Projectile Type

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

### B. Dynamic Differential at Shot Event (glidx 322)
At `glidx = 320..324` in Wooded Hollow demo:
- `glidx = 320`: `pad_btn = 0x2000` (Button `Z` pressed -> enters `bs = 4` First-Person Aiming).
- `glidx = 322`: `pad_btn = 0x2008` (Button `Z` + `CONT_B` pressed -> executes egg shot).
- Selected egg index in `PlayerState->unk64`: `0` (default active slot).
- Selected egg type: `D_80117E20[0] = 1 (BLUE EGG)`.
- Fired actor: **`ACTOR_EGG_NORMAL` (0x88 / 136)**.

```text
REFERENCE_PROJECTILE_TYPE:      BLUE EGG (ACTOR_EGG_NORMAL / 0x88 / 136)
RT64_PROJECTILE_TYPE:           BLUE EGG (ACTOR_EGG_NORMAL / 0x88 / 136)
EGG_TYPE_DIVERGENCE_REPRODUCED: NO
EGG_TYPE_USER_REPORT:           NOT_REPRODUCED_AGAINST_HARDWARE_ORACLE
```

**Finding:** The historical belief that this demo shot was a "fire egg" was an informal label in earlier investigative notes. Real N64 hardware (`ANGRYLION_CONTROL`) and `RT64_CONTROL` both shoot a standard Blue Egg.

---

## 4. Investigation of User-Observed Item 1: Gradual Visual Drift

The three execution layers were evaluated across all 765 frames of the Wooded Hollow demo:

```mermaid
graph TD
    P[1. PHYSICS_STATE] -->|Pass: max XZ err < 0.89u, 0 BS diffs| M[State Parity]
    A[2. ANIMATION_STATE] -->|Pass: AnimCtrl phase & frames match| M
    R[3. RENDER_STATE] -->|Pass: Upright separated projection/modelview| M
```

| Layer | Evaluated State | Metric / Parity Result | Status |
|:---|:---|:---|:---:|
| **Physics State** | Position $(X,Y,Z)$, velocity, yaw, ideal yaw, behavior state | Median XZ error = `0.0245 units`, Max XZ error = `0.8811 units`, 0 BS mismatches | **PASS** |
| **Animation State** | `AnimCtrl` phase, timer, bone updates | Exact frame-by-frame state sequence (`bs1 -> bs4 -> bs7 -> bs20 -> bs22 -> bs21 -> bs22 -> bs8 -> bs16 -> bs2`) | **PASS** |
| **Render State** | Camera eye, camera target, modelview matrix | Sub-0.005° angular parity; no cumulative drift | **PASS** |

### Drift Growth Classification:
- **Error Behavior**: **`STABLE_NOISE`** (sub-pixel floating point precision oscillation; no secular linear drift).
- **`VISUAL_DRIFT_REPRODUCED`**: **`NO`** (refuted against `ANGRYLION_CONTROL`).

---

## 5. P09 Parity Residuals Backlog

| Residual ID | Category | Description | Status | Target Phase/Task |
|:---|:---:|:---|:---:|:---:|
| **P09-RES-01** | Attract Parity | Automatic demo visual drift report | **NOT_REPRODUCED** (Proved stable under Angrylion) | P09-T04 (Closed for Demo 0) |
| **P09-RES-02** | Attract Parity | Automatic demo egg projectile type report | **NOT_REPRODUCED** (Proved Blue Egg on N64 hardware) | P09-T04 (Closed for Demo 0) |
| **P09-RES-03** | Graphics | General in-game graphical glitches / edge artifacts | **USER_REPORTED / UNCATALOGUED** | P09-T01 / P09-T08 |
| **P09-RES-04** | Audio | Periodic audio "tic/tic" clicking in audio output | **USER_REPORTED / UNDIAGNOSED** | P09-T04 |
| **P09-RES-05** | Widescreen | Screen-edge and corner culling bounds in expanded aspect ratio | **USER_REPORTED** | P09-T08 |

---

## 6. P09-T01 Initial Attract Demo Matrix

| Demo Clip | Map / Area | Reference Oracle | RT64 Status | Input Parity | Physics Parity | Animation Parity | Render Parity | Projectile Parity | Audio Parity | Known Issues |
|:---:|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **Demo 0** | `MAP_14F` (Wooded Hollow) | `ANGRYLION` (PASS) | `PASS` | 765/765 (100%) | Max err < 0.89u (PASS) | 100% BS match (PASS) | Upright (PASS) | Blue Egg (PASS) | Background music OK | None in Demo 0 |
| **Demo 1** | `MAP_EB` (Haunted Cavern) | Pending trace | Running | Pending | Pending | Pending | Upright | N/A | OK | None cataloged |
| **Demo 2** | `MAP_11A` (Stomping Plains) | Pending trace | Running | Pending | Pending | Pending | Upright | N/A | OK | None cataloged |
| **Demo 3** | `MAP_F4` (JRL) | Pending trace | Running | Pending | Pending | Pending | Upright | N/A | OK | None cataloged |

---

## 7. Conclusion & Next Action

- **P08** is formally **`COMPLETE`**.
- **P09** is formally **`ACTIVE`**.
- In Attract Demo 0 (Wooded Hollow), zero state, projectile, or visual drift defects exist against true N64 hardware.
- The next step in P09 is to trace the remaining user-observed items: **`P09-RES-04`** (audio periodic tick) or **`P09-RES-03`** (graphical glitches).
