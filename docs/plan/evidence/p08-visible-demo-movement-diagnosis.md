---
title: "P08 — visible demo movement diagnosis"
description: "First visible attract/gameplay demo reproduced after the core1 RA-wrapper fix. Character movement is the first fidelity divergence. Diagnosis only; no gameplay fix."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, attract, visible-demo, movement, diagnosis]
---

# P08 — VISIBLE DEMO MOVEMENT DIAGNOSIS

```text
BASE_HEAD:                    5c03c658c18043576e3e160f7b2bcfa570a5968a
MODEL:                        Grok 4.6 HIGH
MODE:                         RUNTIME CHARACTERIZATION / DIAGNOSIS ONLY
CORE1_RA_FIX:                 RESOLVED (do not reopen)
CORE1_RA_TEST:                PASS
BCF28_TEST:                   PASS
TOOIE_BUILD:                  PASS
USER_VISIBLE_DEMO_REPORT:     YES
VISIBLE_DEMO_REPRODUCED:      YES
OLD_AUTOMATED_NO_DEMO:        PRESERVED (historical)
OLD_AUTOMATED_NO_DEMO_EXPLAINED: YES
ROOT_CAUSE_LEVEL:             C
FIX_IMPLEMENTED:              NO
```

## 1. Two observations that must both be kept

### 1.1 Manual user observation (NEW)

After commit `5c03c658c18043576e3e160f7b2bcfa570a5968a` the user ran the
game by hand and saw:

```text
VISIBLE ATTRACT / GAMEPLAY DEMO: YES
character movement visibly incorrect
other visual/gameplay issues also present (not diagnosed here)
```

### 1.2 Historical automated implementation run (PRESERVED)

The Grok RA-wrapper implementation run on the same HEAD reported:

```text
VISIBLE_ATTRACT_DEMO: NO
opening probe: mode=3 / map=0 / warp=0 / attr=0
duration: 150s
command:
  TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
  VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
  timeout -k 2s 150s ./build/TooieRecompiled
```

Evidence: [[p08-core1-ra-wrapper-fix]] §6–§8. That result is **historical
run evidence**. It is not erased. It is also not the current product
milestone.

## 2. Environment comparison

| | Historical 150s automated | This diagnosis Run A | This diagnosis Run B |
| --- | --- | --- | --- |
| HEAD | `5c03c65` | `5c03c65` | `5c03c65` + movement probe |
| `TOOIE_AUTO_START` | 1 | 1 | 1 |
| `TOOIE_DISABLE_LAUNCHER_MUSIC` | 1 | 1 | 1 |
| `VK_ICD_FILENAMES` | radeon_icd.json | radeon_icd.json | radeon_icd.json |
| `TOOIE_DIAG` | **1** | **unset** | **unset** |
| `TOOIE_DIAG_MOVEMENT` | unset | unset | **1** |
| Duration | 150s | 180s | 180s |
| Window capture | no (probe only) | yes (root + window) | yes (window every 5s) |
| Visible PRESS START / 3D world | not recorded | YES ~70–80s | YES ~60s |
| Mode-5 in probe | not observed | n/a (no probe) | YES |
| Map 335 in probe | not observed | n/a | YES (brief, then mode-5) |

**RUN_ENVIRONMENT_DIFFERENCE (PROVEN):** the historical “no demo” run
enabled `TOOIE_DIAG=1` and trusted the overlay-gated opening probe. The
user-visible and Run A/B reproductions did **not** set `TOOIE_DIAG`.
Diagnostics are not assumed behavior-neutral. In addition the historical
run never captured the game window, so a title/attract 3D scene could have
been on screen while the probe still printed `mode=3 / map=0`.

**OLD_AUTOMATED_NO_DEMO_EXPLAINED: YES**

1. Probe addresses themselves are **not** wholly stale: Run B shows they
   do change (`mode=3 map=335`, then `mode=5`, later `map=278/314/395`).
2. During visible Wooded Hollow gameplay the probe often prints
   `mode=5 map=0`. The map field is **not** a reliable “are we in a 3D
   world?” bit. A `map=0` line does not mean “no demo”.
3. The historical 150s `TOOIE_DIAG=1` window can remain in intro
   (`mode=3 map=0`) for the entire sample if overlay timing is perturbed
   or if only early/last intro lines were summarized.
4. Independent no-`TOOIE_DIAG` runs reach PRESS START / mode-5 in ~70–90s.

## 3. Product milestone (PROVEN)

```text
intro
 → Jiggy / title logo
 → attract transition
 → C2AB8 crossed
 → BCF28 crossed
 → core1 RA wrappers crossed
 → VISIBLE GAMEPLAY DEMO
```

Do **not** claim normal gameplay, menu, or file-select completion.

### Run A (no DIAG, no new probe)

- Command: `TOOIE_AUTO_START=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1`
  `VK_ICD=radeon` `timeout -k 2s 180s ./build/TooieRecompiled`
- Exit 137 (timeout). No `LOOKUP_FUNC(0)`, no crash string.
- PATCH 4: `guest preemption: ON (ULTRAMODERN_GUEST_PREEMPTION=<unset>)`.
- `gsattract` (`ovl_load idx=731`) loaded 77 times.
- Window capture: Rareware → N64 → Dolby → title/Jiggy → **PRESS START
  over a 3D cave/grotto** → intro logos loop.

### Run B (`TOOIE_DIAG_MOVEMENT=1`, `TOOIE_DIAG` unset)

Probe (`D_80127630` / `D_80127760` / `D_80135470` / `D_80135490[0]`):

```text
mode 0 → 2 → 3 (intro)
mode=3 map=335 entr=8 warp=0 then warp=3     MAP_14F_IOH_WOODED_HOLLOW
mode=2 (transition)
mode=5 map=0                                 VISIBLE Wooded Hollow
mode=5 map=278                               MAP_116_TDL_INSIDE_MOUNTAIN
mode=5 map=314                               MAP_13A_CCL_CENTRAL_CAVERN
mode=5 map=395                               MAP_18B_CS_STARTUP_INTRO
loop back to mode=3, then a second mode-5 cycle
```

```text
VISIBLE_ATTRACT_DEMO: YES
CURRENT_MAP:          335 Wooded Hollow (first demo scene), then 278 / 314 / 395
CURRENT_MODE:         5 during visible demo
CURRENT_WARP:         3 consumed on the 335 and 278 transitions
ATTRACT_STATE:        gsattract ep1 running; attr 2/0/0/0 then 2/2/1/*; stage 0
GAMEPLAY_MILESTONE:   intro → Jiggy → C2AB8 → BCF28 → RA wrappers → visible demo
```

## 4. Other visible issues (inventory only)

```text
ISSUE_1  movement / orientation / camera attach   ACTIVE
ISSUE_2  camera clips inside character meshes     not diagnosed
ISSUE_3  later attract scenes drop Banjo to Y≈-2800 / underwater camera
ISSUE_4  opening-probe map field often 0 during visible mode-5
```

Only ISSUE_1 is diagnosed below.

## 5. Symptom classification

Window captures during the first mode-5 scene (Wooded Hollow):

- ~60s: grassy IOH path; **green Jinjo lying on its back** in the
  foreground; Banjo visible further up the path; camera at grass height.
- ~65s: Banjo standing/walking on grass against a wall (upright).
- ~70–75s: camera staring into a water-filled grotto (next scene).
- ~80s: camera **inside a character mesh** (silhouette cutouts) looking
  at a pink Jinjo in `MAP_116_TDL_INSIDE_MOUNTAIN`.

```text
PRIMARY_MOVEMENT_SYMPTOM:
  C + G + H — actor/camera orientation and attach transform are wrong.
  Characters (first: green Jinjo) rest on their backs or the camera sits
  inside a model. This is not “Banjo’s XZ integrator is a random walk”.
```

Refuted as the *first* divergence:

- B speed-only: Banjo XZ during Wooded Hollow is a smooth path, not a
  uniform time-scale error.
- I attract stick stream: `stick=0,0` while position still advances
  (scripted/attract driver).
- core1 RA-wrapper RA mechanism: already resolved; outputs finite and
  plausible (see §9).

## 6. Selected actor

```text
SELECTED_ACTOR:   Banjo / player 0   (traceable)
VISUAL_WITNESS:   green Jinjo        (clearest wrong pose)
ACTOR_ID:         D_80135490[0]      (PlayerState*)
ACTOR_TYPE:       ba player (Banjo); Jinjo is a separate chjinjo instance
MAP:              MAP_14F_IOH_WOODED_HOLLOW (335), entrance 8
SPAWN/INSTANCE:   player table slot 0; position pointer at PlayerState+0xE4
```

Banjo is the actor with a complete simulation trace. The green Jinjo is
the first *unmistakable* wrong pose in the same deterministic scene.
They are treated as one shared-transform complaint, not two bugs.

## 7. Movement telemetry (Run B, ~5–10 s of first mode-5)

Player 0, `func_8009C128` source (`PlayerState+0xE4` → `vec3`):

| gfx (approx) | mode/map | xyz | yaw |
| --- | --- | --- | --- |
| 49697 | 5 / 0 | 1861.000, -131.000, -166.000 | 276.00 |
| 51032 | 5 / 0 | 2042.001, -111.381, -771.106 | 157.43 |
| 53290 | 5 / 0 | 2770.636, 66.226, -1944.962 | 130.08 |
| 54761 | 5 / 0 | 2835.985, 80.619, -1907.337 | 118.07 |
| 57702 | 5 / 0 | 3152.070, 119.566, -1530.008 | 76.50 |
| 59954 | 5 / 0 | 3416.009, 31.228, -931.065 | 68.44 |
| 63642 | 5 / 0 | 2159.944, -94.510, -1587.664 | 227.00 |
| 73380 | 5 / 0 | 2816.081, 77.175, -1929.605 | 118.02 |
| 74071 | 5 / 278 | 2816.362, 77.272, -1929.799 | 118.02 |
| 75053 | 5 / 0 | **-308.000, -2800.000, -398.000** | 45.00 |

Stick during the Wooded Hollow walk: `0,0` angle≈0 distance=0.
`gsattract` ep1 is live. Attract timer `t` advances (~seconds).

Later Y≈-2800 is a **subsequent** scene drop (ISSUE_3), not the first
Wooded Hollow divergence.

## 8. Simulation vs render

```text
SIMULATION_STATE_WRONG:  NO   (Banjo XZ/Y during first Wooded Hollow walk
                               is a grounded path; yaw tracks the path)
RENDER_TRANSFORM_WRONG:  YES  (Jinjo supine on grass; camera later inside
                               a character mesh; camera height at turf)
```

If world coordinates of Banjo are a plausible walk, do not debug
movement AI/physics as the *first* fix. The first visible error is the
model/camera transform.

## 9. Input, timebase, core1 math

```text
MOVEMENT_DRIVER:          scripted attract / gsattract (not analog stick)
INPUT_STREAM_PLAUSIBLE:   YES   (stick zero while path still moves)
GAME_TIMEBASE:            UNKNOWN-leaning-CORRECT
```

`update_gfx` / `tooie_diag::tick` runs ~900–1000 Hz on this host
(unlocked present). Guest attract `t` still advances on a ~seconds
scale across scenes, so movement integration is **not** obviously
applied once per host frame. Not proven 60 VI exactly.

Core1 wrappers (sampled, `TOOIE_DIAG_MOVEMENT=1`):

| name | calls | representative | verdict |
| --- | --- | --- | --- |
| `func_800137F4` | 1033 | `in=20 out=0.34202` (= sin 20°) | plausible |
| `func_800137D4` | 8402 | finite in [-1, 1] | plausible |
| `func_80013A7C` | 176 | `in=0.995 out=5.69`; `in=-0.47 out=118` (acos-degrees-like) | plausible |
| `func_80013A5C` | 0 this run | not exercised | n/a |
| `tooie_wrap_degrees` | 75151 | 360→0; negatives wrapped | plausible |
| `tooie_sin/cos_degrees` | 461 / 143 | finite | plausible |

Zero `BAD` (NaN / Inf / |out|>1e6) lines.

```text
CORE1_MATH_OUTPUTS_PLAUSIBLE: YES
```

Do **not** reopen the RA-wrapper mechanism. Degree helpers were not
implicated by Banjo’s walk trace; no broad math audit.

## 10. First proven divergence

```text
FIRST_BAD_FRAME_OR_STATE:
  first mode-5 Wooded Hollow seconds (Run B ~60s wall; gfx≈50k–73k)

EXPECTED_STATE:
  Banjo walks the IOH path; nearby Jinjos stand/idle; camera follows
  Banjo at a normal third-person/attract height.

ACTUAL_STATE:
  Banjo player0 xyz/yaw form a grounded walk (simulation OK);
  green Jinjo is supine on the grass; camera is at turf height;
  next scene places the camera inside a character mesh.

FIRST_PROVEN_DIVERGENCE:
  visual actor/camera transform (orientation / attach), not Banjo’s
  XZ integrator, not the attract stick stream, not LOOKUP_FUNC(0).
```

Guest algorithm consulted (narrow):

- `Actor.position` at `+0x04`, `Actor.rotation` at `+0x44`
  (`banjo-tooie-Recomp/include/common.h`).
- Player pose via `func_8009C128` = copy of `*(PlayerState+0xE4)`.
- `D_80135490[]` player table (`src/core2/1ECE0B0.c`).
- `gsattract` first warp `0x014F0801` = map 335 entrance 8
  (`asm/data/overlays/gs/attract.data.s`).

No full movement-engine static audit.

## 11. Root-cause level

```text
ROOT_CAUSE_LEVEL: C
ROOT_CAUSE:
  Shared presentation-transform / orientation path. Banjo simulation
  pose is a plausible attract walk; at least one other character and
  the camera are visually wrong in the same scene. Exact writer
  (actor rotation vs model matrix vs camera attach) is still open.

REFUTED_CANDIDATES:
  PATCH4 / Hybrid F'
  C2AB8 / BCF28 / 0x800BCCC0 / 0x800BCCD8
  func_80013A7C LOOKUP_FUNC(0) / core1 RA cluster
  NaN/Inf from the four RA math wrappers
  attract analog-stick stream (stick is zero)
  Banjo XZ integration as the first Wooded Hollow error
```

One discriminator remains: dump `Actor.rotation[3]` + camera for Banjo
and the first Jinjo over the same 5–10 s and compare to the guest
rotation writer.

## 12. Minimal later fix (NOT implemented)

```text
FIX_OWNER:         TOOIE_NATIVE
FILES:             TBD after the rotation/camera discriminator
                   (likely src/game/* Tooie overlay helper, not
                   RecompiledFuncs / N64Recomp / N64ModernRuntime)
FUNCTIONS:         TBD (actor rotation writer or camera attach)
SEMANTIC_CHANGE:   restore guest orientation / camera-node transform
WHY_THIS_LAYER:    presentation transform is Tooie-specific; do not
                   reopen RA wrappers or preemption
FIX_IMPLEMENTED:   NO
```

### Acceptance test for that later fix

Same attract demo, same HEAD family, `TOOIE_AUTO_START=1`,
`TOOIE_DIAG` unset, `TOOIE_DIAG_MOVEMENT=1`:

1. Reach mode-5 Wooded Hollow (map 335 entrance 8 warp consumed).
2. For ~8 s: Banjo `D_80135490[0]` path stays on ground
   (`|Y|` in the first-scene band, not -2800).
3. Banjo and the first visible Jinjo have upright orientations
   (pitch/roll near guest rest, not ~180° supine).
4. Camera is not inside a character mesh.
5. No regression: C2AB8, BCF28, RA wrappers, visible demo still occur.

“Looks better” is not sufficient by itself.

## 13. Instrumentation retained

`TOOIE_DIAG_MOVEMENT=1` (independent of `TOOIE_DIAG`):

- `include/tooie_diag.h`, `src/game/tooie_diag.cpp`
- opening + player0 dump in `src/game/syscall_handler.cpp`
- sampled math I/O in `src/game/boot_hw_stubs.cpp`

Gated, reversible, Tooie source only. `RecompiledFuncs` untouched.

Raw logs/screenshots live under `/tmp/tooie-p08-runA` and
`/tmp/tooie-p08-runB` (not committed).

## 14. Next single action

Run the single highest-value discriminator: dump Banjo and first Jinjo
`Actor.rotation` + camera transform for the first 8 s of Wooded Hollow
mode-5, then name the exact writer if the rotations are already wrong
in simulation.
