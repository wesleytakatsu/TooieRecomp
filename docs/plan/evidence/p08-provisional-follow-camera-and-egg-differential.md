---
title: "P08 — Attract cycle reconstruction and USER_DEMO_4 egg/camera differential"
description: "User-continuous attract numbering, one incomplete-but-measured attract slice, and the proven USER_DEMO_4 FIRE-vs-BLUE egg parity defect against ANGRYLION_CONTROL."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-21
tags: [p08, attract, user-demo, egg, fire-egg, blue-egg, follow-camera, angrylion, rt64, parity, gsattract]
---

# P08 — Attract cycle reconstruction and USER_DEMO_4 egg/camera differential

## 1. Verdict

```text
USER_NUMBERING:                 CONTINUOUS_ACROSS_INTRO_BLOCKS (not zero-based Demo 0..3)
USER_DEMO_4:                    FIRST gameplay scene after first return to intro/music following USER_DEMO_1..3
USER_DEMO_4_MAP:                235 / 0x0EB / MAP_EB_WW_HAUNTED_CAVERN entrance 5
USER_DEMO_4_INTERNAL_SUB:       stage=0 sub=4

USER_DEMO_1_RT64_PROJECTILE:    cheggfire overlay 535   (baeggsetup a1=2 FIRE)
USER_DEMO_1_ANGRYLION_PROJECTILE: cheggfire overlay 535
USER_DEMO_4_RT64_SELECTED_EGG:  BLUE (baeggsetup a1=1)
USER_DEMO_4_RT64_PROJECTILE:    cheggnormal overlay 539
USER_DEMO_4_REFERENCE_EGG_EVENT_FOUND: YES
USER_DEMO_4_REFERENCE_SELECTED_EGG: FIRE (spawn overlay 535; baeggsetup a1 not captured)
USER_DEMO_4_REFERENCE_PROJECTILE: cheggfire overlay 535
USER_DEMO_4_EGG_PARITY:         FAIL
EGG_PARITY_DEFECT:              PROVEN

FOLLOW_CAMERA_USER_DEMO_4:      RT64 uses ncbapivot (follow) mixed with scripted cameras
FOLLOW_CAMERA_PARITY_NUMERIC:   UNKNOWN (no pad-aligned Angrylion camera stream this session)
ANGRYLION_NIGHTLY_VISUAL:       USER_CONFIRMED PASS (fire egg AND previously-broken demo behavior)
b042865_PHYSICS_PASS_AS_REFUTATION: DISPUTED (cannot refute USER_DEMO_4 follow-camera until that clip is mapped and compared)

FULL_ATTRACT_GAMEPLAY_SCENE_COUNT: 12 per D_808003D0 pointer (INFERENCE from static 6-byte table; 8 measured live)
FULL_ATTRACT_SEQUENCE_REPEAT_PROVEN: NO

P08_STATUS:                     ACTIVE / MANUAL_VALIDATION_PENDING
P09_STATUS:                     PENDING / NOT_FORMALLY_OPEN
FIX_IMPLEMENTED:                NO
```

b042865 claims now **DISPUTED** / **CORRECTED/SUPERSEDED** as complete-attract conclusions:

- `REFERENCE_FIRE_EGG_EVENT_FOUND = NO`
- `GLOBAL_VISUAL_DRIFT_STATUS = NOT_REPRODUCED_IN_ATTRACT_SET`
- `REAL_PARITY_DEFECT_FOUND = NO`
- "all attract demos 0..3" as the complete attract set

Retained Demo 0..3 rows may remain valid **for those exact clips if later re-proven**, but they are **not** the complete attract cycle and must **not** map USER_DEMO_4 onto zero-based "Demo 3".

---

## 2. User numbering vs zero-based Demo 0..3

The user numbers **every** gameplay demonstration continuously across repeated intro/music blocks:

```text
INTRO / MUSIC
  -> USER_DEMO_1
  -> USER_DEMO_2
  -> USER_DEMO_3
INTRO / MUSIC AGAIN
  -> USER_DEMO_4     << primary egg + follow-camera target
  -> USER_DEMO_5
  -> USER_DEMO_6
INTRO / MUSIC AGAIN
  -> USER_DEMO_7 ...
```

Zero-based Demo 0..3 in [[p09-user-observed-parity-residuals]] selected only `flags=0x01` maps (335, 235, 282, 244) and skipped TDL 278 and CCL 314. That catalog is a **partial slice**, not the attract state machine.

---

## 3. Static attract table (PROVEN)

Overlay `.gsattract` (index 2882 / overlay 731), VRAM `0x80800000`, size `0x4B0`, ROM `0x02129FF0` in `build/bt.n64.us.1.0.z64`.

`D_808003D0` is **6-byte** records: `u16be map`, `u8 entrance`, `u8 flags`, `u16 extra`.

`D_80800490` is a **12-byte** stage table of pointers:

- stage 0 -> `0x808003D0` (this session's live slice)
- stage 1 -> `0x80800430` (same maps, different entrances; not reached live)

First pointer (16 records = 4 groups of 3 gameplay + intro):

| i | map | name | entrance | flags | role |
|---:|---:|---|---:|---:|---|
| 0 | 335 `0x014F` | Wooded Hollow | 8 | `01` | gameplay |
| 1 | 278 `0x0116` | TDL Inside Mountain | 7 | `00` | gameplay |
| 2 | 314 `0x013A` | CCL Central Cavern | 8 | `00` | gameplay |
| 3 | 395 `0x018B` | CS Startup Intro | `0xFF` | `00` | intro |
| 4 | 235 `0x00EB` | WW Haunted Cavern | 5 | `01` | gameplay |
| 5 | 296 `0x0128` | MAP_296 | 8 | `00` | gameplay |
| 6 | 185 `0x00B9` | Mayahem Temple | 7 | `00` | gameplay |
| 7 | 395 `0x018B` | CS Startup Intro | `0xFF` | `00` | intro |
| 8 | 282 `0x011A` | TDL Stomping Plains | 8 | `01` | gameplay |
| 9 | 264 `0x0108` | MAP_264 | 8 | `00` | gameplay |
| 10 | 230 `0x00E6` | MAP_230 | 8 | `00` | gameplay |
| 11 | 395 | CS Startup Intro | `0xFF` | `00` | intro |
| 12 | 244 `0x00F4` | MAP_F4 JRL | 7 | `01` | gameplay |
| 13 | 276 `0x0114` | MAP_276 | 6 | `00` | gameplay |
| 14 | 423 `0x01A7` | MAP_423 | 8 | `00` | gameplay |
| 15 | 395 | CS Startup Intro | `0xFF` | `00` | intro |

Runtime `D_80135470+1` (`sub`) indexes this table. During a mode-5 clip, `D_80127630+0x10` (map) often reads `0`; identify clips by `(stage, sub)` plus the warp-arrival map.

---

## 4. Live attract slice (RT64, TOOIE_DIAG_ATTRACT=1)

Command (exit 0 at 455s; soak did not wrap the 16-record pointer):

```text
TOOIE_AUTO_START=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 TOOIE_DIAG_ATTRACT=1 \
timeout -k 2s 480s ./build/TooieRecompiled
HEAD: b042865e9d2f6c4e86e46cfc12ebf3043197c98e + env-gated attract diagnostics
```

Live sequence (mode-5 gameplay counted continuously; intro = map 395 and/or mode-3 map 0):

| USER_ORDINAL | INTERNAL_ATTRACT_INDEX | MAP | ENTRANCE | MODE | INPUT_ASSET | PRECEDING_INTRO_OCCURRENCE |
|---:|---|---|---:|---:|---|---:|
| 1 | stage=0 sub=0 | 335 Wooded Hollow | 8 | 5 (map often 0 in-clip) | glrecord ovl 698, 765 samples / cnt=766 | 1 (title intro) |
| 2 | stage=0 sub=1 | 278 TDL Inside Mountain | 7 | 5 | glrecord, 504 samples / cnt=476 | 1 |
| 3 | stage=0 sub=2 | 314 CCL Central Cavern | 8 | 5 | glrecord, 766 samples / cnt=738 | 1 |
| — | stage=0 sub=3 | 395 CS Startup Intro then T_INTRO | 1 / 0 | 5 then 3 | n/a (intro/music) | 1 -> 2 |
| **4** | **stage=0 sub=4** | **235 WW Haunted Cavern** | **5** | **5** | **glrecord, 1011 samples / cnt=1012** | **2** |
| 5 | stage=0 sub=5 | 296 MAP_296 | 8 | 5 | glrecord, 1151 samples / cnt=1123 | 2 |
| 6 | stage=0 sub=6 | 185 Mayahem Temple | 7 | 5 | glrecord, 935 samples / cnt=907 | 2 |
| — | stage=0 sub=7 | 395 then T_INTRO | 1 / 0 | 5 then 3 | n/a | 2 -> 3 |
| 7 | stage=0 sub=8 | 282 TDL Stomping Plains | 8 | 5 | glrecord, 1244 samples / cnt=1245 | 3 |
| 8 | stage=0 sub=9 | 264 MAP_264 | 8 | 5 | truncated (63 samples) | 3 |

```text
USER_DEMO_1: map 335 entrance 8, sub=0, FIRE egg (both oracles)
USER_DEMO_2: map 278 entrance 7, sub=1
USER_DEMO_3: map 314 entrance 8, sub=2
USER_DEMO_4: map 235 entrance 5, sub=4, AFTER first intro return
USER_DEMO_5: map 296 entrance 8, sub=5
USER_DEMO_6: map 185 entrance 7, sub=6
```

`FULL_ATTRACT_SEQUENCE_REPEAT_PROVEN: NO` — the soak reached sub=9 of 16 and did not return to Wooded Hollow / wrap stage 1.

---

## 5. Egg events

Egg type is the projectile overlay actually entered, plus `baeggsetup` overlay 126 ep 5 `a1` when captured:

- `a1=1` / overlay 539 `cheggnormal` = BLUE
- `a1=2` / overlay 535 `cheggfire` = FIRE

### USER_DEMO_1 (sub=0, Wooded Hollow)

RT64: `baeggsetup` ep5 `a1=2` then `cheggfire` ep2. Angrylion: `cheggfire` ep2. User reported this scene looks correct. **Do not inherit p09 "Demo 0 = BLUE".** Measured type is FIRE.

### USER_DEMO_4 (sub=4, Haunted Cavern) — primary discriminator

RT64 (TOOIE_DIAG_SHOT via ATTRACT):

```text
baeggsetup ep5 a1=00000001 BLUE   (repeated)
cheggnormal ovl=539 ep=2          (repeated projectile spawns)
```

Angrylion (Pure Interpreter syscall hook, `tools/p08_attract_egg_hook.h`):

```text
[egg-sys] ovl=731 ep=0 ... sub=4
[egg-sys] ovl=126 ep=10 ... sub=4
[egg-sys] ovl=535 ep=2 ... sub=4    << cheggfire, repeated
```

```text
USER_DEMO_4_REFERENCE_EGG_EVENT_FOUND: YES
USER_DEMO_4_REFERENCE_SELECTED_EGG:    FIRE
USER_DEMO_4_REFERENCE_PROJECTILE:      cheggfire / overlay 535
USER_DEMO_4_RT64_SELECTED_EGG:         BLUE
USER_DEMO_4_RT64_PROJECTILE:           cheggnormal / overlay 539
USER_DEMO_4_EGG_PARITY:                FAIL
EGG_PARITY_DEFECT:                     PROVEN
```

No other egg events were pursued after this discriminator, per task scope.

`PlayerState+0x64` via `D_80135490[0]` dumped garbage in RT64; do not use that field from this probe. Spawn overlay + `baeggsetup` `a1` are the measured selectors.

---

## 6. Follow camera (USER_DEMO_4)

RT64 USER_DEMO_4 camera overlays after Haunted Cavern arrival include `ncbapivot` (follow), plus scripted `ncbaspline` / `ncbadolly` / `ncbafixpos` / `ncba1p`. Recorded C-up (`0x2008`) begins around glidx 176.

RT64 `camctrl` (ps selector `D_80127708[sel]+0x1C`) moves from ~307° early in the clip to ~331° at glidx 1011; player XZ travels from `(-84, -1855)` to `(611, -2365)`. `cammode` at `+0x16` in this probe is **untrusted** (wild s16 values).

```text
FOLLOW_CAMERA_DIFFERENTIAL:     UNKNOWN (numeric)
CAMERA_FIRST_THEN_YAW_THEN_POS: UNKNOWN
ANGRYLION_CAMERA_STREAM:        NOT_CAPTURED_THIS_SESSION
ANGRYLION_NIGHTLY_VISUAL:       USER_CONFIRMED PASS
```

The user confirmed that **this Angrylion RDP Plus nightly-build run** showed correct behavior for **both**:

- fire egg instead of blue egg;
- the demo behavior that was broken in TooieRecomp.

That is `USER_CONFIRMED` visual PASS on `ANGRYLION_CONTROL`. Combined with the syscall spawn (`535` vs `539`), egg type is **PROVEN**. The broken-demo / follow-camera visual is **USER_CONFIRMED** on Angrylion vs **USER_REPORTED** on RT64; the pad-aligned first-difference chain remains **UNKNOWN**.

b042865 "physics PASS" on a four-row Demo 0..3 table does **not** refute the user's accumulating follow-camera report on USER_DEMO_4.

---

## 7. Environment

```text
RT64:         ./build/TooieRecompiled  TOOIE_DIAG_ATTRACT=1
ANGRYLION:    mupen64plus --emumode 0  + Angrylion RDP Plus + cxd4 LLE
MUPEN_CORE:   6dca4c15370ac3e2171ce7b31426695f8f39b460
ANGRYLION:    9c8b9ed3e7d7f00dff8bc872ccdd3fba1a3673fc
CXD4:         00906a92641c540a64e4e7505012157315314810
ROM_SHA1:     af1a89e12b638b8d82cc4c085c8e01d4cba03fb3
CPU_MODE:     PURE_INTERPRETER
```

Temporary Tooie diagnostics are env-gated (`TOOIE_DIAG_ATTRACT` implies SHOT + DEMO_SYNC). Angrylion hook lives in the local mupen core plus `tools/p08_attract_egg_hook.h`. No production behavior change. Generated C was not edited.

---

## 8. Classification of prior claims

| Claim | New class |
|---|---|
| USER_DEMO_4 = zero-based Demo 3 | **REFUTED** |
| Demo 0..3 table = complete attract set | **DISPUTED** / incomplete slice |
| REFERENCE_FIRE_EGG_EVENT_FOUND = NO | **CORRECTED/SUPERSEDED** (FIRE in USER_DEMO_1 both oracles; FIRE vs BLUE in USER_DEMO_4) |
| REAL_PARITY_DEFECT_FOUND = NO | **CORRECTED/SUPERSEDED** (`EGG_PARITY_DEFECT: PROVEN`) |
| GLOBAL_VISUAL_DRIFT_STATUS = NOT_REPRODUCED_IN_ATTRACT_SET | **DISPUTED** (USER_CONFIRMED Angrylion PASS vs RT64 broken-demo report; numeric follow-camera still UNKNOWN) |
| p09 Demo 0 projectile = BLUE | **CORRECTED/SUPERSEDED** (FIRE) |
| p09 Demo 1 / Haunted = no firing | **CORRECTED/SUPERSEDED** (USER_DEMO_4 fires; types disagree) |
| overlay 417/556 = gsattract | **CORRECTED/SUPERSEDED** (overlay **731** `.gsattract`) |

---

## 9. Next single action

Locate the **first egg-state difference** on USER_DEMO_4 (`baeggsetup` `a1` / selected egg before spawn) that makes Angrylion enter overlay 535 and RT64 enter overlay 539. Do not hunt other egg scenes. Follow-camera pad-aligned dump remains residual **UNKNOWN**.

```text
NEXT_SINGLE_ACTION: trace USER_DEMO_4 selected-egg state (a1 / owner) at first fire; Angrylion FIRE vs RT64 BLUE
```
