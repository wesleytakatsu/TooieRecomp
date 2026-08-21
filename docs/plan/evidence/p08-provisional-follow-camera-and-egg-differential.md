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
USER_DEMO4_SEMANTIC_JOIN:       PASS

USER_DEMO_1_RT64_PROJECTILE:    cheggfire overlay 535   (baeggsetup a1=2 FIRE)
USER_DEMO_1_ANGRYLION_PROJECTILE: cheggfire overlay 535
USER_DEMO4_RT64_SELECTED_EGG:   1 / BLUE
USER_DEMO4_RT64_EGG_ENUM:       1
USER_DEMO4_RT64_SPAWN:          cheggnormal overlay 539
USER_DEMO4_RT64_ACTOR_ID:       overlay-implied BLUE; packed +0x6C at spawn enter untrusted
USER_DEMO4_REFERENCE_SELECTED_EGG: 2 / FIRE
USER_DEMO4_REFERENCE_EGG_ENUM:  2 (INFERENCE from spawn overlay 535; baeggsetup ep5 a1 not visible as a syscall on N64)
USER_DEMO4_REFERENCE_SPAWN:     cheggfire overlay 535
USER_DEMO4_REFERENCE_ACTOR_ID:  overlay-implied FIRE; packed +0x6C at spawn enter untrusted
REFERENCE_EVENT_FOUND:          YES
EGG_PARITY_DEFECT:              PROVEN   (Case A: REFERENCE=FIRE, RT64=BLUE)
FIRST_EGG_STATE_DIFFERENCE:     egg enum / projectile overlay at first fire (1/539 vs 2/535). Inventory D_80117E20=01020403 MATCHES both oracles. ps+0x64 slot untrusted. Equipped enum at clip start NOT captured.
FIRST_EGG_DIVERGENT_STORE_OR_WRITER: baeggsetup_entrypoint_5 (VRAM off +0x42C): sb a1 -> *(s0+0x70)+0x24 then func_80086D30(0x80084A58, *(s0+0x184))

FOLLOW_CAMERA_MODE:             ncbapivot (overlay 781)
FOLLOW_CAMERA_OBJECT:           D_80127708[ps+0x184]  (sel=0 this clip)
FOLLOW_CAMERA_UPDATE_FUNCTION:  ncbapivot_entrypoint_3
PLAYER_MOVEMENT_CAMERA_SOURCE:  selected camera record +0x1C (camctrl); historically written by func_800CA628
PAD_STREAM_MATCH:               YES (877/877 joined glidx, 100%)
FIRST_FOLLOW_CAMERA_NUMERIC_DIFFERENCE: glidx 41 camctrl 306.0417 vs 306.1269 (0.085 deg); eye Δ 2.01
FIRST_FOLLOW_CAMERA_PERSISTENT_DIFFERENCE: glidx 43 camctrl Δ 1.29 deg (3 consecutive samples over 0.05)
FIRST_CAMERA_USED_FOR_MOVEMENT_DIFFERENCE: glidx 41 camctrl
FIRST_IDEAL_YAW_DIFFERENCE:     glidx 42 (0.087 deg same-index)
FIRST_PHYS_YAW_DIFFERENCE:      glidx 39 same-index is 1-sample yaw_setIdeal vs glrecord phase; after AN[n]~RT[n+1] pos align, phys/pos stay together until ~43-47
FIRST_PLAYER_POSITION_PERSISTENT_DIFFERENCE: ~glidx 47 under +1 physics alignment (pos Δ>2); same-index already 19u at 41
CAMERA_DIVERGENCE_REPRODUCED:   YES
CAMERA_DIVERGENCE_GROWTH:       FEEDBACK_ACCUMULATION (camctrl 0.085° @41 → 12° @50 → 113° @100; eye 2 → 181 → 818; xz 19 → 27 → 829)
PLAYER_TRAJECTORY_DIVERGENCE_REPRODUCED: YES
CAMERA_PRECEDES_PLAYER_DIVERGENCE: YES (after 1-sample physics correction: camctrl @41 before persistent pos @47)
SCRIPTED_CAMERA_PARITY:         UNKNOWN_NUMERIC (no Angrylion mode-3 CSV). Warp-in uses ncpod 783 / ncpoducam 785, not ncbapivot. User visual PASS on Angrylion nightly. Idle-to-walk camctrl identical at glidx 38 (303.985).

ANGRYLION_NIGHTLY_VISUAL:       USER_CONFIRMED PASS (fire egg AND previously-broken demo behavior)
b042865_PHYSICS_PASS_AS_REFUTATION: DISPUTED (Demo 0 Wooded Hollow R-MOTION-ORACLE does not cover USER_DEMO_4)

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

Semantic join (do not use wall-clock):

```text
USER_DEMO4_SEMANTIC_JOIN: PASS
  ordinal=4  map=235 Haunted Cavern  stage=0 sub=4
  start pad=(2,2,0x0000) pos=(-84,0,-1855) yaw=108 camctrl=30  (both oracles)
  C-up landmark glidx=176 pad=(10,78,0x2008)  (both oracles; player already diverged)
  PAD_STREAM_MATCH: 877/877 at same glidx (100%)
```

RT64 (TOOIE_DIAG_SHOT via ATTRACT; 455s soak reused, not rerun):

```text
baeggsetup ep8 a1=00000001
baeggsetup ep5 a1=00000001 BLUE a2=80 a3=120
cheggnormal ovl=539 ep=2 act=80345908
```

Angrylion (Pure Interpreter; `tools/p08_attract_egg_hook.h` + yaw_setIdeal `0x8009C914` motion + `func_80086D30` spawn dispatcher):

```text
ovl=126 ep=10 ... sub=4 post=179
func_80086D30 a0=80084A58 ... sub=4 post=188
ovl=535 ep=2 cheggfire ... sub=4 post=188   (repeated; never 539 on this clip)
egg_tbl D_80117E20 = 01020403 at boot and at fire (MATCH vs RT64)
baeggsetup ep5 a1: not a separate N64 syscall (overlay already resident; PC 0x8080042C missed because overlay 126 is relocated off 0x80800000)
```

```text
USER_DEMO4_REFERENCE_EGG_EVENT_FOUND: YES
USER_DEMO4_REFERENCE_SELECTED_EGG:    FIRE
USER_DEMO4_REFERENCE_EGG_ENUM:        2
USER_DEMO4_REFERENCE_SPAWN:           cheggfire
USER_DEMO4_REFERENCE_ACTOR_ID:        overlay-implied FIRE (packed +0x6C at ep2 enter is float garbage)
USER_DEMO4_RT64_SELECTED_EGG:         BLUE
USER_DEMO4_RT64_EGG_ENUM:             1
USER_DEMO4_RT64_SPAWN:                cheggnormal
USER_DEMO4_RT64_ACTOR_ID:             overlay-implied BLUE (packed +0x6C untrusted)
EGG_PARITY_DEFECT:                    PROVEN
ROOT_CAUSE_LEVEL:                     B (gameplay selected-egg / spawn overlay; not a render-only color)
```

Ordered egg-state check (stop at first proven difference; no fix):

1. attract/demo init inventory `D_80117E20=01020403`: **MATCH**
2. selected slot `ps+0x64`: **UNTRUSTED** (garbage in RT64 probe)
3. egg enum at first fire: **DIFFERS** (RT64 `a1=1` BLUE vs reference spawn 535 FIRE)
4. egg-cycle: not reached
5. projectile dispatch: **DIFFERS** (`func_80086D30` → overlay 539 vs 535)
6. spawned actor overlay: **DIFFERS** (`cheggnormal` vs `cheggfire`)

```text
FIRST_EGG_STATE_DIFFERENCE:          egg enum / spawn overlay at first fire (1/539 vs 2/535)
FIRST_EGG_DIVERGENT_STORE_OR_WRITER: baeggsetup_entrypoint_5 sb a1, 0x24(*(s0+0x70)); jal func_80086D30
```

Equipped enum at USER_DEMO_4 glidx 1 was **not** captured. Pad including C-up matches, so the fire button is the same recorded sample; the selected type at that sample differs. Whether the equipped egg was already BLUE at clip start on RT64 remains **UNKNOWN**. Camera/player already diverged at glidx 41, before C-up at 176, so a downstream world-state effect is not ruled out. Egg investigation **stops** here per task.

`PlayerState+0x64` via `D_80135490[0]` dumped garbage in RT64; do not use that field from this probe.

---

## 6. Follow camera (USER_DEMO_4)

Follow path (distinct from scripted / display / Rice Z-buffer):

```text
FOLLOW_CAMERA_MODE:            ncbapivot overlay 781
FOLLOW_CAMERA_OBJECT:          D_80127708[sel]  sel=0
FOLLOW_CAMERA_UPDATE_FUNCTION: ncbapivot_entrypoint_3  (stub 0x80089488; RA 0x8011061C)
PLAYER_MOVEMENT_CAMERA_SOURCE: camctrl at selected camera +0x1C
                               (Demo 0 writer: func_800CA628; not re-traced this session)
```

Warp-in / predetermined cameras on this map use `ncpod` 783 and `ncpoducam` 785 during mode=3. Overlay enters in the Haunted window also include `ncbaspline` / `ncbadolly` / `ncbafixpos` / `ncba1p`. Those are **not** the accumulating gameplay follow path.

Capture: RT64 `[demo-sync]` at glrecord (1011 samples). Angrylion `yaw_setIdeal` `0x8009C914` when `post_glidx` changes (878 samples; idle glidx 1-37 omitted because yaw_setIdeal does not run while `bs=0`). Join by same glidx; pad 877/877.

Sampling note (PROVEN, same as Demo 0): Angrylion position/phys_yaw at yaw_setIdeal is one physics step ahead of RT64 glrecord (`AN[n].pos == RT[n+1].pos` through glidx 42, Δ<0.02). camctrl is same-index aligned (bit-identical 303.985 at glidx 38).

Causal chain (same recorded PAD):

| layer | first difference |
|---|---|
| PAD | none (877/877) |
| camctrl (movement camera) | glidx 41, 0.085 deg |
| follow eye | glidx 41, Δ 2.01 |
| ideal_yaw | glidx 42, 0.087 deg |
| phys_yaw / xz (after +1 align) | persistent ~glidx 47 |
| player xz (same index) | glidx 39 (1-sample phase) then grows |

```text
PAD_STREAM_MATCH:                          YES
FIRST_FOLLOW_CAMERA_NUMERIC_DIFFERENCE:    glidx 41 camctrl 0.085 deg
FIRST_FOLLOW_CAMERA_PERSISTENT_DIFFERENCE: glidx 43 camctrl 1.29 deg
FIRST_CAMERA_USED_FOR_MOVEMENT_DIFFERENCE: glidx 41 camctrl
FIRST_IDEAL_YAW_DIFFERENCE:                glidx 42
FIRST_PHYS_YAW_DIFFERENCE:                 glidx 39 same-index (phase); ~43 after +1
FIRST_PLAYER_POSITION_PERSISTENT_DIFFERENCE: ~glidx 47 (+1 align)
CAMERA_YAW_ERROR_GROWTH:   0.00@38  0.085@41  12.0@50  112.7@100  113.8@176
CAMERA_EYE_ERROR_GROWTH:   0@38  2.0@41  181@50  818@100  1124@176
PLAYER_XZ_ERROR_GROWTH:    0@38  19@41  27@50  829@100  2120@176
MOVEMENT_ANGLE_ERROR_GROWTH: ideal 0@38  0.087@42  10.8@50  111@100
GROWTH_CLASS:              FEEDBACK_ACCUMULATION
CAMERA_DIVERGENCE_REPRODUCED: YES
PLAYER_TRAJECTORY_DIVERGENCE_REPRODUCED: YES
CAMERA_PRECEDES_PLAYER_DIVERGENCE: YES
SCRIPTED_CAMERA_PARITY:    UNKNOWN_NUMERIC; user visual PASS on Angrylion nightly;
                           warp-in ncpod/ncpoducam distinct from ncbapivot;
                           camctrl identical at walk-start glidx 38
```

This is **not** the Demo 0 Wooded Hollow R-MOTION-ORACLE result and does **not** reopen Rice Z-buffer artifacts. No camera constants were tuned. No production fix.

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

Temporary Tooie diagnostics are env-gated (`TOOIE_DIAG_ATTRACT` implies SHOT + DEMO_SYNC). Angrylion hook lives in the local mupen core plus `tools/p08_attract_egg_hook.h` (syscall + `yaw_setIdeal` `0x8009C914` + `func_80086D30`). No production behavior change. Generated C was not edited. The 455s RT64 soak was **not** rerun; USER_DEMO_4 CSV was parsed from `/tmp/p08-attract-rt64/stderr.log`.

---

## 8. Classification of prior claims

| Claim | New class |
|---|---|
| USER_DEMO_4 = zero-based Demo 3 | **REFUTED** |
| Demo 0..3 table = complete attract set | **DISPUTED** / incomplete slice |
| REFERENCE_FIRE_EGG_EVENT_FOUND = NO | **CORRECTED/SUPERSEDED** (FIRE in USER_DEMO_1 both oracles; FIRE vs BLUE in USER_DEMO_4) |
| REAL_PARITY_DEFECT_FOUND = NO | **CORRECTED/SUPERSEDED** (`EGG_PARITY_DEFECT: PROVEN`) |
| GLOBAL_VISUAL_DRIFT_STATUS = NOT_REPRODUCED_IN_ATTRACT_SET | **CORRECTED/SUPERSEDED** (USER_DEMO_4 follow-camera + player trajectory **REPRODUCED**; FEEDBACK_ACCUMULATION) |
| p09 Demo 0 projectile = BLUE | **CORRECTED/SUPERSEDED** (FIRE) |
| p09 Demo 1 / Haunted = no firing | **REFUTED** (`HAUNTED_CAVERN_NO_FIRE_EVENT`) |
| overlay 417/556 = gsattract | **CORRECTED/SUPERSEDED** (overlay **731** `.gsattract`) |

---

## 9. Next single action

Egg investigation stopped at the first proven egg-state difference (enum / spawn overlay). No production fix.

Follow-camera **visible** first split remains `camctrl` at USER_DEMO_4 demo-sync glidx 41 (0.085°). **Bit-level** first difference is live eye `node+0x74` written by faithful `func_80110770` ordinal 2 on the mode=2 bind `ncbapivot_entrypoint_3` (enter `n74` MATCH; source already differs). Canonical: [[p08-user-demo4-live-eye-writer]]. Writer of camctrl remains `func_800CA628` ([[p08-user-demo4-follow-camera-first-discriminator]]). Do not reopen Rice Z-buffer. Do not formally open P09.

```text
NEXT_SINGLE_ACTION: instruction-join first ncbapivot_entrypoint_3 (mode=2 glidx=0); no fix
```
