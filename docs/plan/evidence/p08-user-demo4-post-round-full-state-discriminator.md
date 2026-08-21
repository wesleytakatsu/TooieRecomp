---
title: "P08 — USER_DEMO_4 post-ROUND.W.S first full-state discriminator"
description: "Bounded glidx 0..45 ANGRYLION_CONTROL vs RT64_CONTROL full-state join after the bind-tick round.w.s fix. Idle follow-cam is now bit-identical through dump glidx 38; the first remaining difference is ncbapivot_entrypoint_3 output on the first walking-player follow tick (dump glidx 39)."
status: current
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-21
tags: [p08, user-demo-4, full-state, discriminator, ncbapivot, camctrl, round-w-s, angrylion, rt64]
---

# P08 — USER_DEMO_4 post-ROUND.W.S first full-state discriminator

## 1. Verdict

```text
ROUND_W_S_ROOT_CAUSE:                              A / PROVEN
ROUND_W_S_FIX_VALIDATED_AT_ORIGINAL_DISCRIMINATOR: YES
ROUND_W_S_FULL_CAMERA_FIX:                         NO
CAMERA_FEEDBACK_ACCUMULATION:                      STILL_PRESENT

IDLE_FOLLOW_CAM_BIT_IDENTITY_GLIDX_1_38:           YES  (CA8B4 + CA628)
PRE_FIX_GLIDX_1_CA8B4_SEED:                        CORRECTED/SUPERSEDED (now MATCH)

FIRST_BIT_LEVEL_DIFFERENCE:      dump glidx 39 ncbapivot_entrypoint_3 output
                                 eye x/z + record+0x1C camctrl
                                 AN eye_in=c43e5a36,43c4a193,c4a62d54
                                 RT eye_in=c43e67de,43c4a193,c4a629cd
                                 AN src2_in=43abb181,43987693,00000000
                                 RT src2_in=43abb233,43987635,00000000
                                 Y MATCH (43c4a193); camctrl Δ 0.002869°
FIRST_NUMERIC_DIFFERENCE:        same tick; 0.002869° camctrl / ~0.21u eye X
FIRST_SEMANTIC_STATE_DIFFERENCE: follow-camera eye/camctrl on first walking-player ncbapivot
FIRST_GAMEPLAY_CAUSAL_DIFFERENCE: that ncbapivot output; demo-sync prints it as
                                 glidx 40 (0.0029°) then 41 (0.0853°)

FIRST_DIVERGENCE_PRODUCER:       ncbapivot_entrypoint_3 (overlay 781 ep 3)
CA628_CA8B4_ROLE:                identity copy of already-different record
A_CANDIDATE:                     YES  (captured inputs MATCH; complete
                                 ncbapivot executed read set this tick
                                 not dumped on RT64 — FIRST_TICK gate)
FIX_IMPLEMENTED:                 NO (diagnosis only; ROUND.W.S preserved)
```

Do not reopen bind-tick `round.w.s`. That discriminator is closed and bit-identical.

Do not treat demo-sync same-glidx player `bs`/position as a 1-tick physics bug: Mupen CSV is post-update at syscall 698, RT64 `demo-sync` is post-698 and pre-remaining-frame physics. Physics-aligned `AN[N] ↔ RT[N+1]` still MATCHES through the first camera split.

Egg remains FIRE vs BLUE. Independence from this remaining camera/demo split is **UNKNOWN**.

---

## 2. Environment

```text
BASE_HEAD:          897611341e3db0876e8b17895022f9e7f7acbc3b
ROUND_FIX_COMMIT:   5854872c79a7cab1c9395ddcd173cece449ffe11
WORKTREE:           ROUND.W.S architectural fix committed; post-round evidence checkpoint
MODEL:              Grok 4.6 High
RT64:          TOOIE_DIAG_ATTRACT=1 TOOIE_AUTO_START=1  (postfix USER_DEMO_4 log)
               /tmp/p08-roundws-postfix-ud4-full.stderr
ANGRYLION:     Mupen Pure Interpreter + Angrylion RDP Plus nightly-build + CXD4 LLE
               ORACLE ONLY; TooieRecomp remains RT64
               CA8B4/CA628 bits: /tmp/p08-ud4-an-init.log  (glidx 1..42)
               motion CSV:       /tmp/p08-angrylion-user_demo4.csv
JOIN:          stage/sub + consumed glrecord index + pad bytes (not wall-clock)
WINDOW:        USER_DEMO_4 Haunted Cavern stage=0 sub=4, glidx 0..45
PRODUCTION_FIX_THIS_TASK: NONE
```

RT64 `tooie_ud4_ncbapivot_ep3` logs only when `TOOIE_UD4_FIRST_TICK` is set (`tooie_ud4_writer_window`). The postfix run therefore has CA8B4/CA628/rec/yawideal/fc38/fe14 for glidx 1..42, not live `ncbapivot` node+0x74 dumps. CA8B4 remains an identity snapshot of the follow-cam record after ncbapivot.

---

## 3. Bind-tick remainder (closed)

Live FIRST_TICK_STOP replay already proved:

```text
yaw f12:     4399bcf1 MATCH
table index: 006d5328 MATCH
sin:         bf4b29cc MATCH
sp+0x48:     c43efa3b,43c49665,c4a6b798 MATCH
node+0x74:   c43efa3b,43c49665,c4a6b798 MATCH
```

Angrylion first USER_DEMO_4 ncbapivot enter still carries that n74. The pre-fix glidx-1 CA8B4 seed (`c43efa34` / `c4a6b794` / `4399bb16`) is **gone**.

---

## 4. Levels of divergence

| Level | First event | Notes |
|---|---|---|
| Bit | dump glidx 39 CA8B4/CA628 record after ncbapivot | idle 1..38: **0** mismatches |
| Numeric | 0.002869° camctrl; eye X Δ≈0.213, Z Δ≈0.110; Y MATCH | not display-rounded noise; 2+ words |
| Semantic | follow-camera eye/camctrl on first moved-player tick | player pos/yaw/ideal still MATCH |
| Gameplay-causal | that output is the seed of demo-sync 0.0853° @41 → 12° @50 → 113° @100 | walk-start amplification of **this** tick, not the closed idle ULP |

Tiny idle ULP is **not** the remaining cause. It no longer exists.

---

## 5. Demo stream / action state

PAD matching used consumed glrecord index + pad bytes.

```text
DEMO_PAD_STREAM_MATCH:            YES
  full-demo 877/877 (prior postfix replay)
  overlapping 0..45 samples (AN CSV 0,38..45; RT 1..45): all pad x/y/buttons MATCH
DEMO_ACTION_STREAM_MATCH:         YES after dump-phase alignment
FIRST_DEMO_ACTION_DIFFERENCE:     NONE
DEMO_CAMERA_MODE_REQUEST_MATCH:   YES (ncbapivot overlay 781 both; sel=0)
```

Same-glidx `bs` is 1 tick ahead on Angrylion CSV (`38: AN=1 RT=0`, `39: AN=3 RT=1`) because the CSV samples later in the frame than RT `demo-sync`. Physics-aligned: `AN38 bs=1 ↔ RT39 bs=1`, `AN39 bs=3 ↔ RT40 bs=3`. Not a demo-script defect.

`cammode` in `demo-sync` is an unproven halfword (garbage signed values). Do not join on it. Weapon/egg selection is inactive in this window (`egg_enum=-1`).

---

## 6. Player behavior / physics / animation

Physics-aligned (`AN[N] ↔ RT[N+1]`) through the first camera split:

| glidx map | pos | vel xz | tspd | phys_yaw | bs |
|---|---|---|---|---|---|
| AN38 ↔ RT39 | spawn −84,0,−1855 | 0 | 0 | 108 | 1 |
| AN39 ↔ RT40 | −94.5967,0,−1857.4445 | (−211.93,−48.89) | 500 | 257.01 | 3 |
| AN40 ↔ RT41 | −104.3071,0,−1859.7510 | (−291.31,−69.20) | 500 | 256.24 | 3 |
| AN41 ↔ RT42 | −123.244,0,−1863.608 | (−378.74,−77.14) | 500 | 259.93 | 3 |

At CA8B4 dump glidx 39 (the first camera-split tick) **same-dump** player fields MATCH both oracles:

```text
pos_bits:   c2bd3182,36800000,c4e82e39
yaw_bits:   43265555   (166.333328)
ideal_bits: 43801ef0   (256.241699)
```

```text
PLAYER_BEHAVIOR_STATE_MATCH:           YES (physics-aligned; bs 0→1→3)
FIRST_PLAYER_BEHAVIOR_DIFFERENCE:      NONE before camera split
PLAYER_POSITION_MATCH_UNTIL:           dump glidx 39 (same-dump bits); physics-aligned through 41
PLAYER_VELOCITY_MAGNITUDE_MATCH_UNTIL: physics-aligned through 41 (then camera-driven)
PLAYER_VELOCITY_DIRECTION_MATCH_UNTIL: physics-aligned through 41
PLAYER_TARGET_SPEED_MATCH_UNTIL:       500 both at first walk samples
PLAYER_ACCELERATION_MATCH_UNTIL:       accel=2.00 / mult=1.0000 through the window
PLAYER_PHYS_YAW_MATCH_UNTIL:           physics-aligned through 41
PLAYER_IDEAL_YAW_MATCH_UNTIL:          dump glidx 39 bits 43801ef0 MATCH; first DIFF dump glidx 40 (downstream of camctrl)
FIRST_PLAYER_PHYSICS_DIFFERENCE:       dump glidx 41 pos_bits (after camera split)
PLAYER_SPEED_CODE_CHANGED:             NO
```

Same scalar speed + later different vx/vz is **not** a speed defect.

Animation: this window has **no** anim ID / phase / timer / rate / root-motion dump. Prior baseline only proved 30 Hz cadence in measured windows. Cadence ≠ full playback-state parity.

```text
PLAYER_ANIMATION_ID_MATCH:     NOT_CAPTURED
PLAYER_ANIMATION_PHASE_MATCH:  NOT_CAPTURED
PLAYER_ANIMATION_TIMER_MATCH:  NOT_CAPTURED
PLAYER_ANIMATION_RATE_MATCH:   CADENCE_ONLY (prior 30 Hz); not re-proven as full state here
PLAYER_ROOT_MOTION_MATCH:      INACTIVE / NOT_CAPTURED
FIRST_PLAYER_ANIMATION_DIFFERENCE: NOT_CAPTURED
```

Animation is **not** the first remaining discriminator: player pos/yaw/bs already MATCH at the tick whose camera output differs.

---

## 7. Camera

Idle follow slew is now bit-identical:

```text
CA8B4 first-enter glidx 1..38:  0 field mismatches (eye_in, src2_in, pos_bits, yaw_bits, ideal_bits)
CA628 first-enter glidx 1..38:  src2_1_bits + eye MATCH
CAMERA_RECORD_INIT glidx 1:     45226124,453b8006,458c9fee / src2 c1f00000,41f00000,00000000 MATCH
```

First remaining camera difference — CA8B4 ord=1 after ncbapivot at **dump glidx 39**:

| field | ANGRYLION | RT64 | |
|---|---|---|---|
| eye x | `c43e5a36` −761.409546 | `c43e67de` −761.622925 | DIFF |
| eye y | `43c4a193` 393.262299 | `43c4a193` 393.262299 | MATCH |
| eye z | `c4a62d54` −1329.416504 | `c4a629cd` −1329.306274 | DIFF |
| src2[0] | `43abb181` | `43abb233` | DIFF |
| src2[1] camctrl | `43987693` 304.926361 | `43987635` 304.923492 | DIFF 0.002869° |
| player pos/yaw/ideal | MATCH | MATCH | |

Angrylion ncbapivot **enter** glidx 39 still holds the matched idle record:

```text
rec eye_in=c43efa3b,43c4968f,c4a6b798
src2_in=43abcd39,43980d52,00000000
pos_bits=c2bd3182,36800000,c4e82e39
yaw_bits=43265555 ideal_bits=43801ef0
```

Those enter inputs equal both oracles' CA8B4 glidx 38 output + the matched walking player. ncbapivot then writes a different record. CA628/CA8B4 copy it.

```text
CAMERA_STATE_MATCH_UNTIL:           dump glidx 38 (bit)
FIRST_CAMERA_STATE_DIFFERENCE:      dump glidx 39 ncbapivot output (eye x/z, camctrl)
FIRST_CAMERA_DIFFERENCE_PRODUCER:   ncbapivot_entrypoint_3
DEMO_SYNC_FIRST_PRINTED_SPLIT:      glidx 41 camctrl 306.0416 vs 306.1269 (0.0853°)
                                    (dump glidx 39 write is sampled as demo-sync ~40)
```

`func_800E1A58` remains identity in this clip. Do not reopen C4C34 collision.

---

## 8. Causal order (dump glidx 39)

Observed RT64 order at this glidx (`seq` in postfix log):

```text
yaw_setIdeal          a1=256.241699 bits=43801ef0 MATCH
func_8010FC38         (fc38)
ncbapivot_entrypoint_3   ← FIRST DIFF (output; not logged this run)
func_8010FE14         (fe14)
func_800CA628         identity of already-different record
func_800CA8B4         identity
func_800E1A58         identity
func_800CA628         identity again
```

Player physics for this walking pose is already visible at ncbapivot/CA8B4 enter (pos −94.60 MATCH). So:

```text
FIRST_DIVERGENCE_EXECUTION_ORDER:
  glrecord consume (pad MATCH)
  → demo/yaw_setIdeal (ideal MATCH)
  → player behavior/physics (pos/yaw/bs MATCH at camera enter)
  → ncbapivot_entrypoint_3  FIRST DIFF (follow eye/camctrl)
  → CA628/CA8B4 identity
  → later ticks: ideal_yaw / movement direction use divergent camctrl
```

This is **camera → later ideal_yaw/direction**, not animation→camera and not physics-scalar→camera. Player is an **input** to this ncbapivot (moved) but a **matching** input.

---

## 9. Environment / AI

```text
ENVIRONMENT_INPUTS_MATCH_BEFORE_FIRST_DIVERGENCE: NOT_READ
```

No terrain/collision query is in the executed CA8B4/CA628/ncbapivot-enter read set used here. The old C4C34 hypothesis stays **REFUTED** as this discriminator.

```text
ACTIVE_AI_ACTORS_IN_WINDOW:  scenery/static markers only (demo-sync act2=0, digger=0 for glidx 1..45)
AI_STATE_MATCH_BEFORE_PLAYER_CAMERA_SPLIT: NO_RELEVANT_ACTOR
FIRST_AI_DIFFERENCE:         NONE / NOT_APPLICABLE
```

Guardrail only. No moving AI is required to explain the glidx-39 camera split.

---

## 10. First-difference table (glidx 0..45)

Dump-phase: Angrylion motion CSV is later in the frame than RT `demo-sync`. Camera bits below use CA8B4/CA628 (same pipeline point both oracles). Player physics rows use `AN[N]↔RT[N+1]` where noted.

| Channel | LAST_MATCHING_GLIDX | FIRST_DIFFERENT_GLIDX | DIFFERENCE_MAGNITUDE/BITS | PRODUCER | CAUSAL_STATUS |
|---|---|---|---|---|---|
| DEMO_ACTION | 45 (pad); bs aligned | NONE | pad 100%; bs 0→1→3 aligned | glrecord 698 | MATCH |
| PLAYER_BEHAVIOR | aligned 41 | none before camera | bs 1 then 3 | `bs_setState` | MATCH until camera |
| PLAYER_ANIMATION | n/a | NOT_CAPTURED | ID/phase/timer absent | — | NOT_ROOT |
| PLAYER_POSITION | dump 39 bits; aligned 41 | dump 41 pos_bits | then grows | physics using camctrl | DOWNSTREAM of camera |
| PLAYER_VELOCITY_MAG | aligned 41 | after 41 | mag still ~tspd 500 | physics | NOT_ROOT (scalar PASS) |
| PLAYER_VELOCITY_DIRECTION | aligned 41 | after 41 | heading follows camctrl | physics + camctrl | DOWNSTREAM |
| PLAYER_TARGET_SPEED | 45 window | NONE | 0 then 500 | baphysics | MATCH |
| PLAYER_ACCELERATION | 45 window | NONE | accel=2 / mult=1 | physics init | MATCH |
| PLAYER_YAW | dump 39 `43265555` | dump 41 `435876d9` vs `43586ea8` | after camctrl | yaw catch-up | DOWNSTREAM |
| PLAYER_IDEAL_YAW | dump 39 `43801ef0` | dump 40 `4381f692` vs `4381f634` | 2 ULP then grows | `yaw_setIdeal` from camctrl | DOWNSTREAM |
| CAMERA_EYE | dump 38 `c43efa3b,43c4968f,c4a6b798` | dump 39 x/z | X/Z DIFF, Y MATCH | ncbapivot ep3 | **FIRST BIT** |
| CAMCTRL | dump 38 `43980d52` | dump 39 `43987693` vs `43987635` | 0.002869° then 0.0853° @ demo-sync 41 | ncbapivot → CA628 identity | **FIRST BIT** |
| CAMERA_INTERNAL_STATE | dump 38 record | dump 39 record | six-word follow record | ncbapivot ep3 | **FIRST BIT** |
| ENVIRONMENT_INPUT | n/a | n/a | not read | — | NOT_READ |
| AI_STATE | n/a | n/a | no relevant mover | — | NO_RELEVANT_ACTOR |

---

## 11. Root search

Earliest causally relevant difference: **ncbapivot_entrypoint_3 output at dump glidx 39**.

Captured inputs MATCH (previous follow record, player pos, yaw, ideal). Complete executed read set (node+0x74/+0x80, tgt, globals, FPRs) is **not** on the RT64 postfix log. Same-inputs/different-FPU is therefore **not fully closed**, but it is the live A-candidate.

```text
ROOT_CLASS:     B-NCBAPIVOT-WALKSTART-OUTPUT
                (A-CANDIDATE for instruction join; not a new demo/player/anim/speed bug)
A_CANDIDATE:    YES
NEXT_MODEL:     GPT-5.6 Terra
```

Do **not** use Sol. Do **not** reopen bind-tick `round.w.s`. A new half-even at this later angle would already be covered by the global mapping; remaining split after that mapping is a **different** event.

---

## 12. Egg / phase

```text
EGG_PARITY_DEFECT:                         PROVEN
EGG_INDEPENDENT_FROM_ROUND_W_S:            YES
EGG_INDEPENDENT_FROM_REMAINING_CAMERA_STATE: UNKNOWN
```

No egg trace or egg fix in this task. Do not call the egg independent of the remaining camera/demo split.

```text
P08: ACTIVE / MANUAL_VALIDATION_PENDING
     only formal gate = PHYSICAL_GAMEPAD_VALIDATION
P09: PENDING / NOT_FORMALLY_OPEN
ROUND_W_S_SPECIAL_VALUE_CONTRACT: PARTIAL
```

Hardware-faithful reference is ANGRYLION_CONTROL, not a physical N64.

---

## 13. What this does not authorize

- No camera, animation, physics, speed, demo-state, or egg production fix.
- Do not remove the ROUND.W.S implementation because another bug remains
  (`5854872c79a7cab1c9395ddcd173cece449ffe11`).
- `CAMERA_FEEDBACK_ACCUMULATION` remains `STILL_PRESENT`; this evidence
  checkpoint does not claim `CAMERA_FIXED`.
