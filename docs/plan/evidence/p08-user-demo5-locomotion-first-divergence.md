---
title: "P08 — USER_DEMO_5 locomotion first divergence (Kazooie-carries-Banjo / Talon Trot)"
description: "Paired RT64 vs ANGRYLION_CONTROL: first real post-Talon-Trot movement discriminator is camera-derived direction; first camera+0x1C writer difference is CA628 CASE B at glidx 24 (FE14 src2 already divergent)."
status: active
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-23
tags: [p08, user-demo-5, talon-trot, bsbtrot, locomotion, attract, first-divergence]
---

# USER_DEMO_5 locomotion first divergence

```text
BASE_HEAD: 25595a2eb8241ef7ca4aa135e6681e44abcf8598
MODEL:     Grok 4.6 Medium
FIX_IMPLEMENTED: NO
COMMIT: NO
PUSHED: NO
```

This note is a first discriminator, not a production fix. The USER_DEMO_4
`0x800979D8` / BCE84 / P60 / ncbapivot chain is not reopened.

Capture (REFERENCE_ARTIFACT, hashed twice):

```text
CAPTURE_DIR: /tmp/p08-demo5-first-divergence-20260822T173031Z
SHA256_VERIFY: PASS (twice)
```

Diagnostics are env-gated (`TOOIE_UD5_LOCO=1`, `P08_UD5_LOCO=1`). They do not
change production movement.

## 1. Which scene is USER_DEMO_5 — PROVEN

Live RT64 `[ud5-scene]` plus static `D_808003D0` / prior soak agree.

| USER | stage | sub | arrival map | name |
| --- | --- | --- | --- | --- |
| USER_DEMO_4 | 0 | 4 | 235 | WW Haunted Cavern |
| USER_DEMO_5 | 0 | 5 | 296 `0x128` | `MAP_128_HP_ICY_SIDE` (Hailfire Peaks ice side) |
| USER_DEMO_6 | 0 | 6 | 185 | Mayahem Temple |

In-clip `D_80127630+0x10` often reads `0` after arrival, same as other attract
gameplay clips. Identify by `(stage, sub)` plus warp-arrival map.

```text
USER_DEMO5_MAPPING: PROVEN
```

## 2. Technical identity of “Kazooie carries Banjo” — PROVEN as Talon Trot

Tooie overlay **29** is `bsbtrot` (`src/overlays/bs/btrot.c`, ROM `0x01F14F50`).

Runtime on USER_DEMO_5 (RT64):

| glidx (after clip glrecord reset) | bs_cur | anim_id | overlay 29 | meaning |
| ---: | ---: | ---: | --- | --- |
| 1..20 | 1 | 111 | no | idle |
| 21..22 | 7 | 1 | no | crouch (`BS_STATE_7_CROUCH`) |
| 23..34 | 20 | 22 | ep 0 enter @22, then ep 1/3 | Talon Trot **enter** |
| 35+ | 22 | 21 | ep 1 update | Talon Trot **locomotion** |

Enter asm (`bsbtrot_entrypoint_0` @ `0x80800AA0`) plays duration-once via
`func_80800000_bsbtrot(..., a3=0x29F)` and zeroes target horizontal velocity.
Update (`bsbtrot_entrypoint_1` @ `0x80800B3C`) calls `bs_setState`.

```text
KAZOOIE_CARRIES_BANJO_TECHNICAL_NAME: Talon Trot (bsbtrot / overlay 29)
PLAYER_BEHAVIOR_ID:                    20 (enter), 22 (locomote)
PLAYER_SUBSTATE:                       BanjoState.current; next usually 0 on sampled ticks
LOCOMOTION_MODE:                       baphysics type 2 (NORMAL) during enter/locomote; type 3 during crouch
ENTRY_FUNCTION:                        bsbtrot_entrypoint_0
ENTRY_PC:                              0x80800AA0
MOVEMENT_UPDATE_FUNCTION:              bsbtrot_entrypoint_1
MOVEMENT_UPDATE_PC:                    0x80800B3C
ANIMATION_IDS:                         enter 22, locomote 21 (sampled anctrl +0x34)
TALON_TROT_IDENTITY:                   PROVEN
```

`bs_prev` in historical `[demo-sync] bs=` lines is `BanjoState.previous` at
offset 0, not current. This capture logs both.

## 3. Transition join

Clip glrecord resets: RT leftover glidx 1012..1039 then `glidx=0`. Join after
that reset by `stage=0 sub=5` + consumed glidx + pad. Do not join by wall-clock.

```text
TRANSITION_GLIDX_RT: 23 (bs 7→20, first overlay-29 ep 0 @ glidx 22)
TRANSITION_GLIDX_AN: UNKNOWN in dense ticks (CSV sparse); bs 22 already current at first post-gap sample glidx 34
TRANSITION_EVENT:    crouch (bs 7) → bsbtrot enter (bs 20, anim 22) → locomote (bs 22, anim 21)
TRANSITION_JOIN:     PASS for identity/pad on overlapping ticks; PARTIAL density (AN skipped 1..20, 23..33)
```

Pad stream on overlapping rows: **143/143 MATCH**.

Oracle CSV is sparse because `p08_motion_on_glrecord` only emitted when post
changed at a hooked PC; several enter ticks never appeared. That is an
instrumentation limit, not proof that Angrylion skipped those game ticks.

## 4. First overlapping discriminator

After reset:

| glidx | pad | RT | AN |
| ---: | --- | --- | --- |
| 0 | (2,1,0000) MATCH | leftover pose (6.86, 2139.85) yaw 347 camctrl 164.66 | already at ice spawn (4897, 3845) yaw 118 camctrl 30 |
| 21 | (1,1,2000) MATCH | bs 7 anim 1 pos 4897 yaw 118 camctrl 293.5692 | identical |
| 22 | (2,1,2002) MATCH | bs 7 anim 1 pos 4897 yaw 118 camctrl 293.6131 | identical |
| 34 | (-21,21,2000) MATCH | **bs 20 anim 22** camctrl **270.199** tspd 0 | **bs 22 anim 21** camctrl **292.755** tspd 0 |

```text
LAST_FULL_MATCH_GLIDX:     22
FIRST_DIFFERENT_GLIDX:     34   (first overlapping tick after MATCH; 23..33 not in AN CSV)
FIRST_DIFFERENT_FIELD:     bs_cur (20 vs 22) and anim_id (22 vs 21) and camctrl, same sampled tick
FIRST_DIFFERENT_FUNCTION:  bsbtrot enter-complete / bs_setState into 22 (INFERENCE from state IDs; missed call tick)
FIRST_DIFFERENT_PC:        UNKNOWN (AN missed 23..33; RT wrap logs r31=0)
```

glidx 0 pose mismatch is a **one-tick map-entry sampling difference** (RT still
showing previous clip pose). Player pose **reconverges** by glidx 21. Do not
treat glidx 0 leftover as the USER_DEMO_5 locomotion residual.

From glidx 35, AN is already applying Talon Trot target speed (~418) while RT
still has `tspd=0` on the first bs-22 tick; then both accelerate toward 700
with **different yaw/ideal/velocity direction**.

## 5. Ordered first-difference glidx (overlapping samples)

```text
FIRST_BEHAVIOR_DIFF:        34
FIRST_LOCOMOTION_MODE_DIFF: none before 34 (ptype 2 both at 34)
FIRST_ANIMATION_ID_DIFF:    34
FIRST_ANIMATION_PHASE_DIFF: UNKNOWN (not compared bit-wise in paired.csv)
FIRST_TARGET_SPEED_DIFF:    35
FIRST_ACCEL_DIFF:           none in window (2.0 both)
FIRST_TARGET_VELOCITY_DIFF: 35 (tspd; no separate xyz target captured)
FIRST_ACTUAL_VELOCITY_DIFF: 35
FIRST_YAW_DIFF:             37
FIRST_POSITION_DIFF:        35 (after reconverge; glidx 0 leftover ignored)
CAMERA_FIRST_DIFFERENCE_GLIDX: 34 (same sample as behavior; earlier enter ticks missing)
PLAYER_FIRST_DIFFERENCE_GLIDX: 34
CAUSAL_ORDER_CAMERA_VS_PLAYER: UNKNOWN within missing 23..33; at 34 both differ
```

## 6. Transition-entry audit

`bs_setState` @ `0x8009E5C8` wrapped on RT; PC-hooked on AN.

RT wrap often prints `ra=0` (recomp context). AN prints overlay RA
`8020d048` on later locomote ticks with `next=0` (no change). The **20→22**
call itself was not captured on AN.

```text
TRANSITION_FUNCTION_CALL_MATCH: UNKNOWN
TRANSITION_ARGUMENTS_MATCH:     UNKNOWN
TRANSITION_STATE_BEFORE_MATCH:  YES at glidx 22 (pre-enter)
TRANSITION_STATE_AFTER_MATCH:   NO at first post-gap sample glidx 34
```

## 7. Speed / direction (not collapsed)

At glidx 35 (both `bs_cur=22`, anim 21):

| field | RT | AN |
| --- | --- | --- |
| TARGET_SPEED | 0 | 418.37 |
| SPEED_MULTIPLIER | 1 | 1 |
| ACCEL_PARAM | 2 | 2 |
| VELOCITY_XZ | 0 | ~152 |
| yaw | 274.667 MATCH | 274.667 |
| ideal_yaw | 136.60 | 159.15 |

Matching yaw with different target speed is already a failure. Later ticks
also diverge in direction.

## 8. Movement driver / goto

`ptype` is 2 (NORMAL) or 3 (LOCKED_ROTATION during crouch/jump). No
`BA_PHYSICS_C_GOTO` (12) in the USER_DEMO_5 CSV. Motion follows consumed
glrecord stick after Trotting starts.

```text
USER_DEMO5_MOVEMENT_DRIVER: MIXED — INPUT_DRIVEN glrecord after animation-gated Talon Trot enter
TARGET_POSITION_MATCH:      N/A (no goto destination sampled)
```

## 9. Gaps on the executed transition path

Overlay 29 is generated `bsbtrot_*` entrypoints, not a lookup_gap stub.
`bs_setState` is generated guest logic with a diagnostic wrap only.

```text
EXECUTED_GAPS_ON_TRANSITION_PATH: none identified on overlay 29 / bs_setState
```

No A-candidate same-input/different-output helper was isolated (enter call
tick missing on AN).

```text
ROOT_CLASS:        B-BEHAVIOR-TRANSITION / B-ANIMATION-STATE
ROOT_CAUSE_LEVEL:  B
```

AN is **ahead** of RT on completing Talon Trot enter (`bs 20`/`anim 22` →
`bs 22`/`anim 21`). That is the earliest proven overlapping state split.
Whether the cause is animation timer, `anctrl_isStopped`, or an upstream
dt/input into enter is **UNKNOWN**.

## 10. User Talon Trot observation

```text
DOES_FIRST_DIVERGENCE_PRECEDE_KAZOOIE_CARRIES_BANJO: NO
DOES_FIRST_DIVERGENCE_OCCUR_ON_TRANSITION:           YES (enter → locomote)
DOES_FIRST_DIVERGENCE_OCCUR_AFTER_TRANSITION:        NO for first overlapping split
TALON_TROT_CORRELATION:                              CONFIRMED
TALON_TROT_CAUSAL_ROOT:                              NOT_PROVEN
```

Idle/crouch through glidx 22 MATCH. Split is the Talon Trot enter completion,
not a later downhill wander.

## 11. Cheap USER_DEMO_1 cross-check

RT64 with `TOOIE_UD5_LOCO=1` also logged overlay 29 during **sub=0**
(USER_DEMO_1 / Wooded Hollow) at glidx 356 (`ep 0/1/3` then later 4–11).

```text
DOES_USER_DEMO1_USE_SAME_LOCOMOTION_STATE: YES (overlay 29 bsbtrot)
DOES_USER_DEMO1_CALL_SAME_TRANSITION_PATH: YES / INFERENCE (same overlay entrypoints; Demo 1 not pair-traced here)
```

## 12. Phase

```text
P08: ACTIVE / MANUAL_VALIDATION_PENDING
P09: PENDING / NOT_FORMALLY_OPEN
```

## 13. Dense transition arbitration checkpoint (2026-08-22)

The sparse reference above is superseded for the transition window by the
immutable capture below. No production fix was made.

```text
CAPTURE_DIR: /tmp/p08-demo5-talontrot-dense-20260822T181732Z
SHA256_VERIFY: PASS (twice, after all writers were stopped)
DENSE_WINDOW: glidx 20..36, 17/17 rows in RT64 and 17/17 rows in AN
```

The pad stream and player state/animation remain equal through glidx 33 when
the AN pre-state snapshots are joined with the RT64 post-tick rows. The old
glidx-34 split is a sampling-phase artifact: RT64's post row is sampled before
the synchronous setter, while AN's row is sampled after it. Both runtimes
execute the same `bs_setState(22)` event at glidx 34:

| glidx | input | RT64 | AN |
| ---: | --- | --- | --- |
| 34 | `(-21,21,0x2000)` | sample before setter: `bs 7→20`, anim 22 | sample after setter: `bs 20→22`, anim 21 |
| 35 | `(-29,30,0x2000)` | writer sets `tspd=418.370605`; row still samples `0` | row samples `tspd=418.370605` |

AN's exact transition call is captured before the state write:

```text
glidx=34 ra=801dee78 next=22 before prev=7 cur=20 nextf=0
```

RT64 has no `next=22` call in glidx 20..36; it remains in state 20 at the
glidx-34 post-tick sample. Therefore the transition-call arbitration is a
proven **MISMATCH**, not an instrumentation gap. `ra=0x801DEE78` is the
runtime caller return address; the generated `bsbtrot_entrypoint_1` static
callsite is `0x80800BC0` and is not the dynamic caller observed in this
capture.

Static `bsbtrot_entrypoint_1` semantics are precise: it calls
`anctrl_isStopped`; a nonzero result selects animation/state candidate `0x15`
(animation 21), then it calls `anctrl_getAnimTimer` and only enters the
arbitration when `timer > 0.5f`. It calls `func_808007E0_bsbtrot(self,
candidate)` and passes that return to `bs_setState`. The helper's candidate
chain can select `0x16` (state 22), `0x17C`, `0x71`, `0x17`, `0x45`, or retain
its input based on its ordered helper predicates and the `+0x160` flag. This
is the exact gate/arbiter; the capture proves both runtimes take the same
`next=22` result. The RT diagnostic sees the overlay through a relocated
guest address, so static `0x808xxxxx` overlay PCs are only partial evidence;
the resident `bs_setState` and physics-helper PCs are reliable.

Animation roles are numeric and static/dynamic, not guessed names: enter
entrypoint `0x80800AA0` starts animation `0x16` (22) with duration-once
parameters; the update path selects animation `0x15` (21) after the stopped
and timer gate. In this clip, 22 is the Talon Trot enter animation and 21 is
the Talon Trot locomotion animation.

Ordered residuals from the dense paired CSV:

```text
FIRST_PAD_DIFF:              none in glidx 20..36
FIRST_RAW_STATE_SAMPLE_DIFF: 34 (pre/post setter phase)
FIRST_RAW_ANIMATION_DIFF:    34 (pre/post setter phase)
FIRST_RAW_ANIM_TIMER_DIFF:   34 (pre/post setter phase)
FIRST_POSITION_DIFF:         35 (raw row join; not phase-aligned)
FIRST_VELOCITY_DIFF:         35 (raw row join; not phase-aligned)
FIRST_TARGET_SPEED_SAMPLE:   35 (RT write sampled at RT glidx 36)
FIRST_PLAYER_YAW_DIFF:       35 (ideal_yaw; yaw field itself remains equal at 35)
FIRST_CAMERA_CONTROL_DIFF:   25 (camctrl; before the player-state split)
```

Thus the apparent RT/AN player-behavior split at glidx 34 is not a semantic
transition mismatch. Camera-control drift is an earlier, independent residual
at glidx 25; it must not be attributed to the Talon Trot transition without a
separate camera call-order audit. The apparent movement split at glidx 35 is
also a sample-phase artifact: RT64 glidx 36 samples the same write seen by AN
at glidx 35. No production correction is authorized by this checkpoint.

```text
TRANSITION_CALL_MATCH:       YES (same bs_setState(22) semantic event)
TRANSITION_INPUTS_MATCH:     YES through the gate and writer window
RT_NEXT_22_IN_WINDOW:        YES (sample phase differs)
AN_NEXT_22_GLIDX:            34
ROOT_CLASS:                  SAMPLING-PHASE / UPDATE-ORDER ARTIFACT
CAMERA_ROOT:                 SEPARATE / UNRESOLVED
PRODUCTION_FIX:              NONE
```

Artifact manifest and metadata: `/tmp/p08-demo5-talontrot-dense-20260822T181732Z/SHA256SUMS`.

## 14. Exact bs20 → bs22 exit gate (2026-08-22) — PROVEN MATCH at setState

The sparse/dense claim that RT64 **fails** to call `bs_setState(22)` at
glidx 34 is **CORRECTED/SUPERSEDED**. A gate-level recapture proves both
runtimes issue `next=22` on that tick.

```text
CAPTURE_DIR: /tmp/p08-demo5-bs20-exit-gate-20260822T184452Z
SHA256_VERIFY: PASS (sha256sum -c twice after writers stopped)
```

Static reconstruction of `bsbtrot_entrypoint_1` (`0x80800B3C`):

| symbol | PC |
| --- | --- |
| BS20_TO22_BS_SETSTATE_CALL_PC | `0x80800BC0` (`jal bs_setState`; delay `or a1,s0`) |
| BS20_TO22_CALLER_PC | `0x80800B3C` (`bsbtrot_entrypoint_1`) |
| BS20_TO22_IMMEDIATE_GATE_PC | `0x80800BA8` (`bc1fl` skip arbiter if NOT `0.5 < timer`) |
| ANCTRL_ISSTOPPED_CALL_PC | `0x80800B78` (`jal anctrl_isStopped`) |
| ANIM_TIMER_LOAD_PC | `0x80800B8C` (`jal anctrl_getAnimTimer` → `func_8008C0C8(anctrl+0x1C)` loads `anctrl+0x2C`) |
| ANIM_TIMER_COMPARE_PC | `0x80800BA0` (`c.lt.s $f4=0.5, $f0=timer`) |
| ARBITER_CALL_PC | `0x80800BB0` (`jal func_808007E0_bsbtrot`) |
| ARBITER_RETURN_TEST_PC | none; `s0=v0` at `0x80800BB8` then always `bs_setState` |

`anctrl_isStopped` (`0x8008B324`) returns `anctrl_getPlaybackType()==3`
(`lbu +0x1A`). It does **not** read the timer.

`func_808007E0_bsbtrot` first writes candidate `0x16` (state 22) when
`bastick_getZone(self)>0` and `*(player+0x160)==0`. That zone is stick
**distance** (`BaStick.zone.id` at stick+`0x3C`), not camera yaw.

```text
BS20_EXIT_GATE_ORDER:
1. baanim_getAnimCtrlPtr
2. bainput_enable(self,0,1)
3. func_8009D3A8(self,1)
4. anctrl_isStopped → if nonzero s0=0x15
5. anctrl_getAnimTimer
6. if 0.5 < timer: s0 = func_808007E0_bsbtrot(self, s0)
7. bs_setState(self, s0)   # always; next=0 is a no-op
```

Paired gate bits glidx 30..34 (`paired-gates.csv`):

| glidx | timer bits | gt_half | isStopped | zone RT@ep1 | zone AN@first anctrl | setState(22) |
| ---: | --- | ---: | ---: | ---: | ---: | --- |
| 30..33 | identical (`3eeeeef1` at 33 = 0.466667) | 0 | 0 | 0 | 0 | NO / NO |
| 34 | identical `3f044445` = 0.516667 | 1 | 0 | 1 | 0 then 1 later in tick | **YES / YES** |

```text
LAST_ALL_GATE_INPUTS_MATCH_GLIDX: 33
FIRST_GATE_DIFFERENCE_GLIDX:      none on the transition call
FIRST_GATE_DIFFERENCE:            NONE — both call bs_setState(22) at glidx 34
TRANSITION_CALL_DIFFERENCE:       MATCH (this capture)
```

The earlier 698-CSV split (`RT bs_cur=20` vs `AN bs_cur=22` at glidx 34)
is a **sample-phase join**: RT `tooie_demo_sync_tick` on overlay 698 still
sees pre-`ep1` `bs_cur=20` while Angrylion's 698/setState row is already
post-transition. RT `ovl29-ep1` then executes `next=22` on the same glidx.
`ROOT_CLASS B-BEHAVIOR-TRANSITION / B-ANIMATION-STATE` is
**CORRECTED/SUPERSEDED** for the missing-call claim.

The exact target-speed writer is `baphysics_set_target_horizontal_velocity`
at `0x8009B9B0`, with the architectural store at `0x8009B9BC`, writing the
physics object at `player+0xC8`, field `+0x3C`. RT64's `418.370605` write is
glidx 35 / logic 8307 / writer ordinal 4, `0x00000000 → 0x43d12f70`;
AN's corresponding writer is the same semantic event at AN glidx 35.
RT64 glidx 36 samples that write, so the one-row `tspd` offset is not a real
one-tick locomotion delay. `FIRST_MOVEMENT_DIFF` at the phase-aligned join is
not established by this window.

`func_8008ADE4` diagnostic wrap logged every scene AnimCtrl (noisy). Player
timer bits were still identical. Angrylion overlay PCs are heap-relocated
(`bs_setState` RA is in the `0x801xxxxx` heap range), so static `0x808xxxxx`
overlay hooks are partial and not a reliable PC-to-symbol mapping without
relocation metadata. Shared resident helper PCs and semantic event records
remain reliable.

```text
BS20_EXIT_GATE_READS_CAMERA: NO
ARBITER_READS_CAMERA:        NO (first 22-assign is stick zone + +0x160)
CAMERA_CAUSAL_TO_BS20_EXIT:  NO
CAMCTRL_DIFF_GLIDX:          25  PARALLEL_RESIDUAL / NOT_CAUSAL_TO_THIS_TRANSITION
BS20_STATE_TIMER_EXISTS:     NO
RAW_INPUT_MATCH_30_34:       YES
TSPD_OBJECT:                 player +0xC8 physics object
TSPD_OFFSET:                 +0x3C
TSPD_SEMANTIC:               target horizontal velocity
TSPD_SAMPLE_OFFSET:          RT row glidx 36 vs AN row glidx 35
TSPD_ROOT_CLASS:             SAMPLING-PHASE / UPDATE-ORDER ARTIFACT
GAPS_ON_EXACT_EXIT_GATE_PATH: none in shared resident path
NATIVE_OVERRIDES_ON_EXIT_GATE_PATH: diagnostic wraps only
ROOT_CLASS:                  CORRECTED — gate and writer MATCH; no real one-tick delay proven
FIX_IMPLEMENTED:             NO
```

## 15. Post-Talon-Trot writer/sample-phase proof (2026-08-22)

```text
CAPTURE_DIR: /tmp/p08-demo5-post-trot-phase-20260822T161200Z
SHA256_VERIFY: PASS twice after all runtime writers were stopped
```

The phase order used for this decision is:

```text
raw pad/glrecord consumed
  → bsbtrot overlay-29 entry/gate and synchronous bs_setState(22)
  → animation-controller update / state callback
  → baphysics_set_target_horizontal_velocity
       player +0xC8 → physics +0x3C (target horizontal velocity)
  → movement integration and demo-sync post sample
```

`bs_setState` rotates previous/current/next state fields and invokes the old
exit and new entry callbacks synchronously. Therefore the RT64 `bs 7→20` row
at glidx 34 is a pre-setter observation; the AN `bs 20→22` row is a
post-setter observation. Both execute the same `next=22` semantic event.

The target-speed writer is proven at resident PC `0x8009B9B0`, with the
delayed architectural store at `0x8009B9BC`:

```text
TSPD_OBJECT: player + 0xC8
TSPD_FIELD:  physics + 0x3C
TSPD_KIND:   target horizontal velocity (not current velocity)
RT: glidx 35 / logic 8307 / ordinal 4 / 00000000 → 43d12f70
AN: glidx 35 / ordinal 4 / source 43d12f70 (same semantic write)
```

The complete writer inputs relevant to this bounded event match: raw pad
`(-29,30,0x2000)` at glidx 35, state `20→22`, and source bits
`0x43d12f70` (418.370605). The next writes also match:
`0x4408ab0a` (546.672485) and `0x44245d9e` (657.462769). RT64 samples the
first write in its glidx-36 row; AN samples it in its glidx-35 row. This is a
sampling/update-order artifact, not evidence of a real one-tick locomotion
delay. Position/velocity rows remain numerically different after the aligned
write, so full locomotion parity is not claimed here.

Camera control first differs at glidx 25 and the exact exit gate/arbiter does
not read camera state. It is therefore a parallel residual, not a causal
explanation for the `bs_setState(22)` or `tspd` sample phase.

The Oracle's overlay function PCs are heap-relocated and no relocation map is
available in this capture. Static `0x808xxxxx` overlay PCs are consequently
partial/unreliable; shared resident `0x800xxxxx` helper PCs, raw arguments,
state fields, source bits, and semantic event order are the reliable mapping.

Artifacts: `/tmp/p08-demo5-post-trot-phase-20260822T161200Z/`.

## 16. First real post-Talon-Trot movement discriminator (2026-08-22)

```text
CAPTURE_DIR: /tmp/p08-demo5-direction-firstdiff-20260822T201012Z
SHA256_VERIFY: PASS twice after writers stopped
MODEL: Grok 4.6 Medium
FIX_IMPLEMENTED: NO
```

```text
BS20_TO22_MISSING_TRANSITION: CORRECTED/SUPERSEDED
TSPD_ONE_TICK_DELAY:          CORRECTED/SUPERSEDED
TSPD_GLIDX_DIFFERENCE:        SAMPLING_PHASE_ARTIFACT
```

`SEMANTIC_JOIN_SCHEME`: `stage/sub/map` + consumed glidx + writer function +
writer ordinal. Overlay-698 CSV rows are a later sample, not the same phase
as the writer.

### Active Talon Trot movement chain — PROVEN

```text
ACTIVE_TALON_TROT_UPDATE:     bsbtrot_entrypoint_5 @ 0x80800D04
                              (state-22 overlay update) plus core yaw
                              controller func_8009D088 @ 0x8009D088
                              (ytype written by bsbtrot_entrypoint_4 /
                              func_8009FFD8 → func_8009D2D8)
STICK_ZONE_FUNCTION:          bastick_getZone @ 0x8009EF10 /
                              bastick_getZonePosition @ 0x8009EF1C
MOVEMENT_DIRECTION_FUNCTION:  bastick_getAngleRelativeToBanjo @ 0x8009EEB8
                              → func_800A4C48 @ 0x800A4C48 (player+0x184)
                              → func_800CAF00 @ 0x800CAF00
IDEAL_YAW_WRITER:             yaw_setIdeal @ 0x8009C914 store 0x8009C930
                              (yaw object player+0xF8, field +0x4)
PHYS_YAW_WRITER:              baphysics_set_target_yaw @ 0x8009B9C0
                              store 0x8009B9DC (physics +0x40)
TARGET_VELOCITY_WRITER:       func_8009AE08 @ 0x8009AE08 →
                              func_800EEB9C @ 0x800EEB9C
                              (yaw_bits, spd_bits → dest xyz)
ACTUAL_VELOCITY_WRITER:       baphysics type-2 path in func_8009B590
                              copies ideal into target yaw then 8009AE08
POSITION_INTEGRATOR:          physics object player+0xC8, position
                              player+0xE4
```

During bs20 (glidx 22..33) `*(player+0xFC)=2`, so `func_8009D088` only calls
`yaw_update` and does **not** read stick/camera. After `bs_setState(22)`,
enter `bsbtrot_entrypoint_4` sets ytype=1. The first ytype-1 tick is glidx 34.

```text
TALON_TROT_MOVEMENT_READS_CAMERA: YES  (after state22 / ytype=1)
CAMERA_FIELD_CONSUMED:            camera record +0x1C heading
                                  (same field as demo-sync camctrl)
CAMERA_FIELD_OFFSET:              +0x1C
CAMERA_FIELD_SELECTOR:             player+0x184 → func_800A93F8
CAMERA_FIELD_UPSTREAM_WRITER:      func_800CA628 / store 0x800CA65C
                                  (execution in this capture window untraced)
CAMERA_FIELD_FIRST_CONSUMER:      func_800CAF00
                                  wrap(cam+0x1C + stick.angle + 90)
```

### First consumed movement difference — PROVEN

Join: first `rel_angle` / `caf00` / `yaw_setIdeal` after state22
(glidx 34, yaw_ctrl ord 15, rel_angle/caf00/yaw_setIdeal ord 1/1/4).

| input/output | RT64 | ANGRYLION_CONTROL |
| --- | --- | --- |
| stick_ang | `430865b1` | `430865b1` MATCH |
| stick_xy | `be866667,3e800000` | MATCH |
| dist / zone | `3eb9999a` / 1 | MATCH |
| ytype / bs_cur | 1 / 22 | MATCH |
| cam +0x1C | `4387197b` (270.199066°) | `4392609a` (292.754700°) |
| caf00 f0 / ideal src | `430898a8` (136.596313°) | `431f26e4` (159.151917°) |

`d_camctrl = d_ideal = -22.5556°`. Stick MATCH, camera DIFF, direction DIFF.

```text
LAST_FULL_SEMANTIC_MATCH:     glidx 33 yaw_ctrl ord 14 (ytype=2, stick=0;
                              ideal/phys/tspd MATCH; camctrl already differs
                              but is not consumed)
FIRST_REAL_DIFFERENT_EVENT:   func_800CAF00 first post-state22 call
FIRST_REAL_DIFFERENT_GLIDX:   34
FIRST_REAL_DIFFERENT_PHASE:   caf00 / rel_angle (then yaw_setIdeal)
FIRST_REAL_DIFFERENT_FIELD:   camera record +0x1C → desired direction
FIRST_REAL_DIFFERENT_WRITER:  func_800CAF00 (consumer); yaw_setIdeal store
FIRST_REAL_DIFFERENT_PC:      0x800CAF00 (store yaw 0x8009C930)
RAW_INPUT_SEMANTIC_MATCH:     YES (pad MATCH 20..45)
STATE22_SEMANTIC_MATCH:       YES (both next=22 at glidx 34)
STATE22_UPDATE_ORDINAL_MATCH: YES (yaw_ctrl ord 15 both; ytype 1 / bs 22)
ANIMATION_SEMANTIC_MATCH_BEFORE_MOVEMENT_SPLIT: YES
                              (CSV anim 22 vs 21 at glidx 34 is the known
                              698 sample-phase; no anim writer split before
                              caf00)
TARGET_SPEED_SEMANTIC_MATCH:  YES (glidx 35 player sincos spd 43d12f70 both)
TARGET_DIRECTION_SEMANTIC_MATCH: NO
TARGET_VELOCITY_VECTOR_SEMANTIC_MATCH: NO
                              RT yaw 430898a8 vx=438fbcfe vz=c397fabc
                              AN yaw 431f26e4 (same spd)
IDEAL_YAW_SEMANTIC_MATCH:     NO (first write after 22)
IDEAL_YAW_FIELD:              yaw+0x4
IDEAL_YAW_WRITER_RT/AN:       yaw_setIdeal
IDEAL_YAW_STORE_PC:           0x8009C930
IDEAL_YAW_WRITE_EVENT:        glidx 34 ord 4 both
IDEAL_YAW_SOURCE_BITS_RT:     430898a8
IDEAL_YAW_SOURCE_BITS_AN:     431f26e4
IDEAL_YAW_COMPLETE_INPUTS_MATCH: NO (stick MATCH, cam+0x1C DIFF)
PHYS_YAW_FIRST_REAL_DIFF_EVENT: set_target_yaw glidx 35 ord 14
                                src RT 430898a8 vs AN 431f26e4
PLAYER_YAW_FIRST_REAL_DIFF_EVENT: yaw_update inside func_8009D088 after the
                                glidx-34 setIdeal; CSV yaw first splits at 37
CAMERA_INPUT_TO_DIRECTION_RT_BITS: 4387197b
CAMERA_INPUT_TO_DIRECTION_AN_BITS: 4392609a
CAMERA_INPUT_TO_DIRECTION_MATCH: NO
STICK_INPUT_MATCH:            YES
CAMERA_GLIDX25_ROLE:          PARALLEL residual until ytype=1; CAUSAL input
                              to the first state22 direction calculation
EXECUTED_GAPS_ON_FIRST_DIFF_PATH: none (resident 0x800xxxxx helpers)
NATIVE_BOUNDARIES_ON_FIRST_DIFF_PATH: diagnostic wraps only
DEMO1_SAME_DIRECTION_FUNCTION: YES (static: same bastick_getAngleRelativeToBanjo)
DEMO1_SAME_CAMERA_INPUT_PATH: YES (static: same func_800CAF00 +0x1C)
DEMO1_SAME_TARGET_VECTOR_WRITER: YES (static: same 8009AE08/800EEB9C)
ROOT_CLASS:                   B-CAMERA-DERIVED-DIRECTION
ROOT_CAUSE_LEVEL:             B (consumed camera heading already differs;
                              direction helper itself is same-input/same-op)
```

Causal order of first differences (semantic events, not raw CSV glidx):

1. camctrl +0x1C @ glidx 25 yaw_ctrl (not consumed; ytype=2)
2. desired direction / ideal_yaw writer @ glidx 34 caf00/yaw_setIdeal
3. target speed scalar remains matched @ glidx 35
4. target velocity vector @ glidx 35 sincos (same spd, different yaw)
5. phys_yaw writer @ glidx 35 set_target_yaw
6. actual yaw slew / position after that

Do not treat CSV `ideal`/`tspd`/`bs_cur` at glidx 34 as the writer phase:
698 still samples pre-`yaw_setIdeal` / pre-ep1 on RT.

No production fix. Camera writer execution was not traced in this pass.

## 17. Independent camera-record and upstream-writer review (2026-08-22)

The Grok direction result is **CONFIRMED_WITH_CORRECTIONS**. The correction is
that the first camera-field divergence is not merely a demo-sync sampling
artifact: the instrumented semantic `yaw_ctrl` call matches through glidx 24
and first differs at glidx 25. It remains parallel while `ytype=2`; the first
causal consumer is still glidx 34, after the synchronous `bs_setState(22)`.

```text
BASE_HEAD:                         25595a2eb8241ef7ca4aa135e6681e44abcf8598
SHA256_VERIFY_1:                   PASS
SHA256_VERIFY_2:                   PASS
CAPTURE:                           /tmp/p08-demo5-direction-firstdiff-20260822T201012Z/

DIRECTION_EVENT_SEMANTIC_JOIN:    PASS
  stage/sub/map                    0/5/296 in metadata and both logs
  consumed glrecord                glidx 34
  player update ordinal             yaw_ctrl ord 15; rel_angle/caf00 ord 1
  consumer                          0x8009EEB8 -> 0x800CAF00
  downstream call                  yaw_setIdeal ord 4, store 0x8009C930
  phase                             first ytype=1 / bs_cur=22 event

CAMERA_RECORD_SEMANTIC_JOIN:      PASS (active player-control record)
CAMERA_RECORD_1C_SEMANTIC:        PROVEN
CAMERA_RECORD_SELECTION:           player+0x184 -> D_80127708[index]
CAMERA_RECORD_ROLE:               player-control camera record
CAMERA_RECORD_MODE_PARITY:        NOT_PROVEN in this capture; cammode is not
                                   a reliable cross-runtime join key

LAST_CAMERA_1C_SEMANTIC_MATCH:    glidx 24, yaw_ctrl ord 5
FIRST_CAMERA_1C_SEMANTIC_DIFF:    glidx 25, yaw_ctrl ord 6
CAMERA_HEADING_RT_BITS:           4387197b (270.199066 degrees) @ glidx 34
CAMERA_HEADING_AN_BITS:           4392609a (292.754700 degrees) @ glidx 34
CAMERA_HEADING_DELTA:             -22.5556 degrees (RT - AN)

STICK_INPUT_COMPLETE_MATCH:       YES for the consumed fields
  stick_ang                       430865b1
  stick_xy                        be866667,3e800000
  distance / zone                 3eb9999a / 1
  pad/buttons                     (-21,21,0x2000) in the joined sample

CAMERA_TO_DIRECTION_CAUSAL_EDGE:  PROVEN
IDEAL_YAW_RT / AN:                430898a8 / 431f26e4
CAMERA_DIRECTION_TO_IDEAL_YAW:    PROVEN (same call ordinal and writer)
TARGET_SPEED_RT / AN:             418.370605 / 418.370605
TARGET_VECTOR_MATCH:              NO (same magnitude, different yaw/XZ)
GENERIC_SPEED_ROOT:               REFUTED for this first discriminator
YTYPE_BEFORE / AFTER:              2 / 1
CAMERA_CONSUMED_WHEN_YTYPE2:      NO (parallel field divergence only)
CAMERA_CONSUMED_WHEN_YTYPE1:      YES
```

Static source establishes the exact direction formula:

```text
bastick_getAngleRelativeToBanjo @ 0x8009EEB8:
  camera = func_800A4C48(player)
  stick_angle = *( *(player+0x128) + 0x5C )
  return func_800CAF00(camera, stick_angle)

func_800CAF00 @ 0x800CAF00:
  normalized(camera[+0x1C] + stick_angle + 90.0 degrees)

func_800A4C48:
  return func_800A93F8(*(player+0x184))

func_800A93F8:
  return D_80127708[*(player+0x184)]
```

Thus `+0x1C` is proven to be the same semantic active player-control camera
field in both runtimes; only the guest pointer values differ. The first
direction result follows the camera delta with matched stick input. The
shared speed scalar is therefore not the first root.

The best static upstream producer target is:

```text
LIKELY_ACTIVE_CAMERA_1C_WRITER:    func_800CA628
LIKELY_ACTIVE_CAMERA_1C_STORE_PC: 0x800CA65C
WRITE_SEMANTICS:                   copies three destination fields and three
                                   source fields; loop iteration 2 writes
                                   destination +0x1C from the second source
                                   float
CALLER EVIDENCE:                   prior player-control camera differential
                                   identifies func_800CA628/store 0x800CA65C
                                   as the primary camera-control writer
EXECUTED_IN_THIS_WINDOW:          NOT_PROVEN (no writer trace in this capture)
```

Other static `+0x1C` stores are initialization, generic/overlay helpers, or
unrelocated actor code and are not promoted as the active producer without a
runtime pointer/role join. The next trace must instrument `0x800CA65C`, retain
the selected index and destination pointer, and join its last matching write
to the first differing write. No production fix is authorized.

`func_800CAF00`, the camera-relative direction helper, is therefore
**PROVEN_DOWNSTREAM / NOT_ROOT**. The current root boundary is
**B-CAMERA-HEADING-UPSTREAM**.

## 17. Camera +0x1C writer provenance — 2026-08-22

Writer capture (hashed twice after processes stopped):

```text
CAPTURE_DIR: /tmp/p08-demo5-camera1c-writer-20260822T211941Z
SHA256_VERIFY: PASS (twice)
```

Static `func_800CA628` (size `0x40`): `dst=a0`, `src=a1`, `src2=a2`. Three
loop stores. Instruction at `0x800CA65C` is `swc1 $f6, 0x14($v1)` in the
`bne` delay slot. Heading write is loop `$v0==2` (`0-based` iteration 1):
`$f6` loaded from `src2[1]` → `dst+0x1C`. Confirmed dynamically:
`heading_store=1` only when `loop_v0=2` and `store==dst+0x1C`.

```text
CA628_STORE_PC:                 0x800CA65C
CA628_STORE_INSTRUCTION:        swc1 $f6, 0x14($v1)
CA628_STORE_SOURCE_REGISTER:    $f6
CA628_RECORD_BASE:              $a0 / selected D_80127708[player+0x184]
CA628_ACTIVE_LOOP_ITERATION:    1 (0-based); MIPS $v0==2 at heading store
CA628_STORE_SEMANTIC:           dst[+0x1C] = src2[1]
CA628_EXECUTED_RT:              YES
CA628_EXECUTED_AN:              YES
CA628_SEMANTIC_EVENT_JOIN:      PASS (glidx + selected + ord 1..18)
CA628_SELECTED_RECORD_MATCH:    YES (sel=0 both; heap pointers differ)
CA628_LOOP_ITERATION_RT:        1 / $v0==2
CA628_LOOP_ITERATION_AN:        1 / $v0==2
```

Last matching selected write: glidx 23, ord 7 (ord 8 is a same-value echo).
Both `old_1c`/`source`/`new_1c` = `4392c3c2`.

First differing selected write: glidx 24, ord 9.

```text
CA628_OLD_VALUE_RT:             4392c3c2
CA628_OLD_VALUE_AN:             4392c3c2
CA628_SOURCE_BITS_RT:           4392861e
CA628_SOURCE_BITS_AN:           4392b342
CA628_SOURCE_MATCH:             NO
CA628_NEW_VALUE_RT:             4392861e
CA628_NEW_VALUE_AN:             4392b342
CA628_OUTPUT_MATCH:             NO
CA628_CLASSIFICATION:           DOWNSTREAM_WRITER  (CASE B)
```

The store copies `$f6` / `src2[1]` faithfully on both sides. The source is
already different at CA628 enter. Angrylion `ra=8010fe24` proves the first
write is the `func_8010FE14` path with scratch `src2=80045678`, **before**
`func_800CA8B4` on that tick. Static review shows that FE14 itself is only a
thin wrapper: it dereferences `a0` and forwards `a1/a2` to CA628; it neither
computes nor fills the heading scratch.

```text
CA628_SOURCE_SEMANTIC:          src2[1] at 80045678+4, caller stack temp
CA628_SOURCE_PRODUCER:          caller stack vector; FE14 forwards a2 unchanged
CA628_SOURCE_PRODUCER_PC:       0x800EE804 (selected static copy path)
CA628_SOURCE_PRODUCER_OUTPUT_RT: 4392861e
CA628_SOURCE_PRODUCER_OUTPUT_AN: 4392b342
CA8B4_ON_CURRENT_PROVENANCE_PATH: NO  (first discriminator write)
```

`func_800CA8B4` / `func_800E1A58` run **after** ord-9 and echo the already
written heading (`E1A58` identity). Do not treat historical CA8B4 Demo4
results as this event's root. `CA8B4_COMPLETE_INPUTS_MATCH` is not applicable
to the first write; later CA8B4 enter already includes the new `+0x1C` and
also shows a different RT eye (`45817cda,...` vs AN `45818c16,...`).

```text
LAST_CAMERA_WRITER_MATCH:       glidx 23 / CA628 ord 7 / selected +0x1C / 4392c3c2
FIRST_CAMERA_WRITER_DIFF:       glidx 24 / CA628 ord 9 / selected +0x1C
FIRST_CAMERA_WRITER_DIFF_GLIDX: 24
FIRST_CAMERA_WRITER_DIFF_ORDINAL: 9
FIRST_CAMERA_WRITER_DIFF_FUNCTION: func_800CA628
FIRST_CAMERA_WRITER_DIFF_PC:    0x800CA65C
FIRST_CAMERA_WRITER_DIFF_FIELD: record+0x1C
UPSTREAM_TO_CAMERA_1C_EDGE:     PROVEN (source bits → store → new_1c)
CAMERA_1C_TO_DIRECTION_EDGE:    already PROVEN
TALON_TROT_ROLE:                CONSUMER
```

End-of-tick / `yaw_ctrl` last match at glidx 24 is the **pre-write** heading
`4392c3c2`. The first writer difference is on that same tick; the consumer
first sees it at glidx 25 (`yaw_ctrl`) and at glidx 34 (`func_800CAF00`).

Cheap A/B/C at first `caf00` after `bs_setState(22)`:

```text
A_CAMERA_MATCH:     YES  (USER_DEMO_1 trot_ord=1 glidx=51 cam=43490ecb both)
A_DIRECTION_MATCH:  YES  (stick=4334b754 f0=42df8c40 both)
B_CAMERA_MATCH:     NOT_CAPTURED (no second sub=1 state22 in this soak)
B_DIRECTION_MATCH:  NOT_CAPTURED
C_CAMERA_MATCH:     NO   (Demo5 trot_ord=1 glidx=34 4387197b vs 4392609a)
C_DIRECTION_MATCH:  NO   (stick MATCH 430865b1; f0 430898a8 vs 431f26e4)
THREE_CASE_PATTERN: A=MATCH / B=NOT_CAPTURED / C=DIFF
TALON_TROT_REENTRY_REQUIRED: NO
JUMP_LANDING_REQUIRED:       NO
```

```text
FIRST MOVEMENT DISCRIMINATOR:   camera-derived heading (glidx 34 CAF00)
FIRST CAMERA PRODUCER DISCRIMINATOR:
  CA628 CASE B at glidx 24 ord 9; source already supplied as FE14 a2 stack input
ROOT_CLASS:                     B-CALLER-STACK-SOURCE
ROOT_CAUSE_LEVEL:               FE14 input boundary; source-field writer UNKNOWN
FIX_IMPLEMENTED:                NO
```

Manual observations remain separate from automated proof:

```text
USER_DEMO1_TROT1:                  APPEARS_CORRECT_BY_USER
USER_DEMO1_TROT2:                  SEVERE_RESIDUAL_REPRODUCED_BY_USER
USER_DEMO5_TROT1:                  SEVERE_RESIDUAL_REPRODUCED_BY_USER
TALON_TROT_REENTRY_REQUIRED:       REFUTED as a required condition
JUMP_LANDING_REQUIRED:             REFUTED as a required condition
THREE_CASE_CAMERA_TEST_WARRANTED: YES
```

The next three-case capture point is the first active `func_800CAF00` call
after state22/ytype1 in Demo1 Trot 1, Demo1 Trot 2, and Demo5 Trot 1, joining
the same camera index, record pointer role, `+0x1C` bits, stick fields,
`yaw_setIdeal` source, and target-speed writer event. This is warranted but
was not dynamically run here.

## 18. FE14/source provenance review — 2026-08-22

The immutable writer capture was verified twice:

```text
CAPTURE_DIR: /tmp/p08-demo5-camera1c-writer-20260822T211941Z
SHA256_VERIFY: PASS (twice)
```

`func_8010FE14` is `0x8010FE14..0x8010FE30`. Its complete operation is:
`lw a0,0(a0)` in the delay slot of `jal 0x800CA628`, then return. Therefore:

```text
FE14_EXACT_ROLE:                dereference/adaptor into CA628
FE14_CALCULATES_HEADING:       NO
FE14_ONLY_COPIES_HEADING:      NO (CA628 performs the three-float copy)
FE14_CALLERS:                  static sites include 0x801109EC and 0x8011066C;
                                selected event matches the 0x8011066C path
```

At the selected event, CA628 writes `record+0x1C` from `a2[1]`:

```text
SCRATCH_4567C_WRITE_PC:        0x800EE804
SCRATCH_4567C_WRITE_INSTRUCTION: swc1 $f6,0x4($a0)
SCRATCH_4567C_SOURCE_REGISTER:  $f6 loaded from caller source element 1
CA628_CAMERA_WRITE_PC:          0x800CA65C
CA628_CAMERA_WRITE_INSTRUCTION: swc1 $f6,0x14($v1)
FE14_SCRATCH_EVENT_JOIN:       PASS (stage/sub/map + glidx 24 + ord 9 + selected)
FE14_INPUT_RT:                 0x4392861e
FE14_INPUT_AN:                 0x4392b342
FE14_INPUT_MATCH:              NO
FE14_SOURCE_IS_STACK_VALUE:   YES
STACK_SLOT:                    caller sp+0x2C (base sp+0x28 + element 1)
STACK_SLOT_WRITER_PC:          0x800EE804 (swc1 $f6,0x4($a0))
```

The selected static caller prepares that stack vector with
`func_800EE7F8`: at `0x80110630`, `a0=sp+0x28` and `a1=s0+0x80`; the helper
copies element 1 at `0x800EE804`. This narrows the first proven boundary to
the caller-provided stack/source field (`s0+0x80`), but the dynamic capture did
not trace the writer of that structure field. Consequently:

```text
FE14_VALUE_SOURCE_KIND:        caller stack temporary copied from s0+0x80
FE14_VALUE_SOURCE_ADDRESS:     0x80045678 + 4 = 0x8004567C in this event
FE14_VALUE_SOURCE_PRODUCER:    func_800EE7F8 / caller source field s0+0x80
FE14_VALUE_SOURCE_PRODUCER_PC: 0x800EE804
LAST_FE14_SOURCE_MATCH:        glidx 23 ord 7/8, 0x4392c3c2
FIRST_FE14_SOURCE_DIFF:        glidx 24 ord 9, 0x4392861e vs 0x4392b342
FIRST_FE14_SOURCE_DIFF_GLIDX:  24
FIRST_FE14_SOURCE_DIFF_ORDINAL: 9
SOURCE_PRODUCER_READS_PREVIOUS_CAMERA_STATE: UNKNOWN
FEEDBACK_LOOP_PRESENT:         UNKNOWN (source-field writer not captured)
CA8B4_STATUS:                  NOT_ON_PATH / later consumer
ROOT_CLASS:                    B-CALLER-STACK-SOURCE
```

The result is a narrower provenance boundary, not a production root fix:
`s0+0x80 → caller stack sp+0x2C → FE14 a2[1] → CA628 → record+0x1C`.
The next trace should instrument exactly the writer of `s0+0x80`; it should
not modify FE14, CA628, or generated code.

## 18. Upstream Camera Source Field s0+0x80 Writer Isolation — 2026-08-23

Paired writer capture artifact directory (double SHA256 verified):

```text
CAPTURE_DIR:   /tmp/p08-demo5-s0-80-writers-20260823T002233Z
SHA256_VERIFY: PASS (twice)
```

### 18.1 Semantic Identity of Object `s0`

Reconstruction of `func_80110588` (`0x80110588..0x8011069C`) and `func_8010FC38` (`0x8010FC38..0x8010FCBC`):

- **Object `s0` Type:** `CameraMode2` Object (Follow Camera Mode Object).
- **Allocation / Lifecycle:** Allocated dynamically in `func_80110424` via `heap_alloc(D_80137372)`. Held at `camera_controller + 0x4` (`*(0x8010FAC0)`).
- **Field Layout & Semantics:**
  - `+0x00..+0x60`: Array of 25 sub-mode state pointers (`ncbapivot` at index 0, etc.).
  - `+0x64`: Pointer back to parent `camera_record` (`D_80127708[sel]`).
  - `+0x68`: Auxiliary actor pointer / parameter.
  - `+0x6C`: Active sub-mode index (`u32`).
  - `+0x70`: Active / enabled flag (`u8`).
  - `+0x71`: 1P / first-person mode flag (`u8`).
  - `+0x72`: Pause / freeze flag (`u8`).
  - `+0x74`: Eye / camera position vector `(x, y, z)` (`f32[3]`).
  - `+0x80`: Camera orientation / rotation angles `(pitch, yaw/heading, roll)` (`f32[3]`). Offset `+0x80` is pitch, `+0x84` is yaw/heading, `+0x88` is roll.
  - `+0x8C`: Auxiliary vector cleared by the constructor; its smoothed/target
    interpretation is not proven here. It is not read by `func_80110A68`.

### 18.2 Exhaustive Static Writer Enumeration

All store paths capable of writing `s0+0x80` (or `+0x84` / `+0x88`):

1. **`func_80110790` (Setter Function):**
   - Stores at `0x800EE7FC` (pitch), `0x800EE804` (yaw/heading), `0x800EE810` (roll) inside helper `func_800EE7F8`.
   - Instructions: `swc1 $f4, 0x0($a0)`, `swc1 $f6, 0x4($a0)`, `swc1 $f8, 0x8($a0)`.
   - 20 audited static callers (e.g. `ncbapivot_entrypoint_3` @ `0x80800244`, `func_80110528` @ `0x8011056C`).
   - Role: Primary active setter for `CameraMode2` orientation.
2. **`ncba1p_entrypoint_13` (`0x808008B0`):**
   - Calls `func_800EE7F8(s0+0x80, sub_struct+0xC)`. Role: 1P mode orientation copyback.
3. **`func_80110424` (Constructor Initialization):**
   - Store at `0x800CA8E8` inside `func_800CA8B4` called via `func_8010FFB0(camera_record, s0+0x74, s0+0x80)`.
   - Role: Scene / mode init from initial camera record.
4. **Gemini `bzero` candidate — corrected:**
   - `func_80110424` at `0x80110450` performs a `jal 0x8002D6E0`; the
     exact constructor source contains no direct `bzero` store at that PC.
   - No separate allocator/bzero path was observed executing in glidx 20..28.

`GEMINI_STATIC_WRITER_ENUM_VERDICT: COMPLETE_WITH_CORRECTIONS`.
The three real writer classes above are retained: active `func_80110790`,
the inactive 1P copyback, and constructor initialization through
`func_8010FFB0`/`func_800CA8B4`. The reported fourth bzero writer is not
substantiated by the exact source.

### 18.3 Exact `func_80110A68` Semantics — Gemini Arbitration

The exact static call chain is:

```text
ncbapivot_entrypoint_3 @ 0x80800238
  func_80110A68(a0=s0, a1=sp+0x60, a2=sp+0x6C)
    func_800A516C(out=sp+0x60, input=sp+0x6C, position=s0+0x74)
      func_800F18FC(input, position, out)
  func_80110790(a0=s0, a1=sp+0x60) @ 0x80800244
```

`func_800F18FC` subtracts the two input vectors component-wise, forms the
horizontal squared magnitude, calls the two angle helpers, and writes the
resulting three-float angle vector. `func_800A516C` then negates the output
pitch through `0x800136E4` and writes zero to output roll. There is no
interpolation factor, `dt`, timer, persistent field, or read of `s0+0x80` /
`s0+0x8C` in this chain.

```text
FUNC_80110A68_EXACT_ROLE:       output-vector adaptor/orchestration helper
ARITHMETIC_PRODUCER:            func_800A516C -> func_800F18FC
OUTPUT_DESTINATION:             caller stack sp+0x60 (runtime 0x80045638)
OUTPUT_TO_80110790_EDGE:        PASS (same caller buffer)
SMOOTHING_OR_INTEGRATOR:        REFUTED
FEEDBACK_READ_FROM_S0_PLUS_80:  NO
```

The static call edge and the new immutable paired trace are both available.
The trace captures the complete two-vector input pair and output at glidx 23
and glidx 24. It proves that the target `sp+0x6C` remains bit-identical while
the eye `s0+0x74` is the first differing input at glidx 24. This closes
`80110A68` as the first cause, while leaving the internal `80111FD4`
translation discriminator unresolved.

```text
FUNC_80110A68_IS_IMMEDIATE_PRODUCER: YES (call-chain producer; arithmetic is in 800A516C/800F18FC)
CALLER_PC:                         0x80800238 (static ncbapivot call)
GLIDX23_COMPLETE_INPUTS_MATCH:     MATCH (target + eye, 0 ULP)
GLIDX23_OUTPUT_MATCH:              MATCH (0 ULP)
GLIDX24_COMPLETE_INPUTS_MATCH:     DIFF (target matches; s0+0x74 differs)
GLIDX24_OUTPUT_MATCH:              DIFF (out_sp60[1] = 0x439250ca vs 0x43927d8b)
FIRST_80110A68_INPUT_DIFF:         s0+0x74 (camera eye position vector)
FIRST_80110A68_INPUT_DIFF_RT:      (0x45817cda, 0x447db8e2, 0x458200cd / 4143.6064, 1014.8888, 4160.1001)
FIRST_80110A68_INPUT_DIFF_AN:      (0x45818c16, 0x447db8e2, 0x458225c9 / 4145.5107, 1014.8888, 4164.7231)
FIRST_DIFF_INPUT_CLASS:            B (s0+0x74 eye position vector)
FUNC_80110A68_READS_PREVIOUS_ROT:  NO
FEEDBACK_LOOP:                     REFUTED for this chain
FEEDBACK_IS_FIRST_CAUSE:           NO
TARGET_ANGLE_PRODUCER:             func_80111FD4 -> func_80110770 (eye pos); func_80112524 (target pos)
TARGET_INPUT_STATUS:               MATCH at glidx 23 and glidx 24 (0 ULP)
EYE_INPUT_STATUS:                  MATCH at glidx 23; DIFF at glidx 24
```

### 18.4 Dynamic Isolation & Writer Classification

Capture results across USER_DEMO_5 (`stage=0, sub=5, map=296`) glidx 20..28:

```text
LAST_MATCHING_WRITE_GLIDX:       23 (ord 4)
LAST_MATCHING_OBJECT_VALUE:      old_84 = 0x43928610, src_1 = 0x43928610 -> new_84 = 0x43928610 (EXACT MATCH)

FIRST_DIVERGENT_WRITE_GLIDX:     24 (ord 5)
FIRST_DIVERGENT_WRITER:          func_80110790
STORE_PC:                        0x800EE804 (swc1 $f6, 0x4($a0)) inside func_800EE7F8
CALLER_RA:                       ncbapivot_entrypoint_3 @ 0x80800244 (relocated 0x801dab6c)
OBJECT_PRE_VALUE:                0x43928610 (RT64) vs 0x43928610 (ANGRYLION) -> MATCH
WRITER_INPUT_PRE_VALUE:          src_1 = 0x439250ca (RT64) vs 0x43927d8b (ANGRYLION) -> DIFF
OBJECT_POST_VALUE:               new_84 = 0x439250ca (RT64) vs 0x43927d8b (ANGRYLION) -> DIFF

FIRST_ACTIVE_WRITER_CLASS:       CASE B (WRITER INPUTS ALREADY DIFFER)
UPSTREAM_PRODUCER:               func_80110A68 orchestration; arithmetic is in func_800A516C / func_800F18FC
FEEDBACK_PROPERTY:               REFUTED for this chain (80110A68 does not read s0+0x80 or s0+0x8C)
DOWNSTREAM_EDGE_STATUS:          VERIFIED PROVEN
80110A68_INPUT_STATUS:           CAPTURED & ISOLATED
```

### 18.5 Complete Proven Dataflow Chain

```text
func_80111FD4 (glidx 24)
  → enters the active translation path:
      func_801107F0(s0, sp+0x3C) loads the initial eye
      func_80111F70(s0, arg, sp+0x3C, *(s0+0x34)+0x78, *(s0+0x34)+0x88)
        → func_80111DB0 → func_800EF174 can transform the working eye
      func_8011106C(s0, sp+0x30) copies s0+0x8C to sp+0x30; it does not compute a delta
      func_800EF04C(sp+0x3C, sp+0x30) adds the copied vector
  → calls func_80110770(s0, sp+0x3C) @ PC 0x8011208C to store into s0+0x74
ncbapivot_entrypoint_3 (glidx 24)
  → func_80112524 reads *(s0+0x30)+0x4..0xC into caller sp+0x6C (target pos) [MATCHES]
  → func_80110A68(s0, sp+0x60, sp+0x6C) calls func_800A516C(sp+0x60, sp+0x6C, s0+0x74)
  → func_800F18FC computes delta from target (sp+0x6C) to eye (s0+0x74)
    → yaw = func_80013B70(dx, dz) -> 0x439250ca (RT64) vs 0x43927d8b (Oracle)
  → func_80110790 / func_800EE7F8 stores src_1 into s0+0x84 at store PC 0x800EE804 (glidx 24 ord 5)
  → func_80110588 reads s0+0x80..0x88 into scratch sp+0x28 (0x8004567C = s0+0x84)
  → func_8010FE14 forwards scratch pointer to func_800CA628
  → func_800CA628 loop iter 1 stores src2[1] into camera_record+0x1C at store PC 0x800CA65C
  → func_800CAF00 reads camera_record+0x1C + stick_angle + 90° -> ideal_yaw (f0)
  → func_8009C914 stores ideal_yaw into player_yaw_controller->ideal_yaw at store PC 0x8009C930
  → Player locomotion heading diverges
```

## 19. func_80110A68 Complete Input/Output Capture — CONFIRMED; translation source unresolved

Artifact directory: `/tmp/p08-demo5-80110a68-firstdiff-20260823T011500Z`
SHA256 verification: PASS (twice)

### 19.1 Static & Dataflow Contracts

- **Function:** `func_80110A68` (`RecompiledFuncs/funcs_217.c:7133`).
- **Signature:** `$a0 = s0` (`CameraMode2` object), `$a1 = sp+0x60` (output rotation vector buffer), `$a2 = sp+0x6C` (target look-at position vector).
- **Execution:** Arranges `$a0 = sp+0x60` (dst), `$a1 = sp+0x6C` (target), `$a2 = s0 + 0x74` (camera eye position) and calls `func_800A516C`.
- **Pure 3D Look-At:** Computes Euler angles from target `sp+0x6C` to eye `s0+0x74`. Reads NO globals, NO previous rotation output. Pure geometric function.
- **Runtime verdict:** complete inputs and output match at glidx 23; at glidx 24 the target still matches, the eye differs, and the output follows the eye difference.
- **Causal status:** `80110A68` is downstream of the first observed `+0x74` divergence, not its root. The exact internal phase inside `80111FD4` was not captured.

### 19.2 Dynamic Input Isolation at glidx 24

| Input Field | RT64 Hex | Oracle Hex | RT64 Float | Oracle Float | Parity |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Input A: `sp+0x6C` (Target X)** | `0x45992390` | `0x45992390` | 4900.4453 | 4900.4453 | **MATCH (0 ULP)** |
| **Input A: `sp+0x6C` (Target Y)** | `0x441e58b4` | `0x441e58b4` | 633.3860 | 633.3860 | **MATCH (0 ULP)** |
| **Input A: `sp+0x6C` (Target Z)** | `0x4570492c` | `0x4570492c` | 3844.5732 | 3844.5732 | **MATCH (0 ULP)** |
| **Input B: `s0+0x74` (Eye X)** | `0x45817cda` | `0x45818c16` | 4143.6064 | 4145.5107 | **DIFF (-1.9043)** |
| **Input B: `s0+0x74` (Eye Y)** | `0x447db8e2` | `0x447db8e2` | 1014.8888 | 1014.8888 | **MATCH (0 ULP)** |
| **Input B: `s0+0x74` (Eye Z)** | `0x458200cd` | `0x458225c9` | 4160.1001 | 4164.7231 | **DIFF (-4.6230)** |
| **Output: `out_sp60[1]` (Yaw)** | `0x439250ca` | `0x43927d8b` | 292.6312 | 292.9808 | **DIFF (-0.3496°)** |

### 19.3 Immediate Upstream Producer of Differing Input

- Differing input: `s0+0x74` (camera eye position vector).
- Writer function: `func_80110770`.
- Calling function: `func_80111FD4` (`funcs_94.c:921` / PC `0x8011208C`).

```text
FIRST_80110A68_INPUT_DIFF: s0+0x74 (camera eye position vector)
PROVEN_BOUNDARY: Camera eye positioning update in func_80111FD4 -> func_80110770 differs at glidx 24.
ROOT_CAUSE_STATUS: The first differing internal phase before the 0x8011208C setter is NOT_CAPTURED.
```

## 20. USER_DEMO_5 camera-eye translation review — 2026-08-23T01:15Z

The immutable artifact `/tmp/p08-demo5-80110a68-firstdiff-20260823T011500Z/`
passes `sha256sum -c SHA256SUMS` twice. The paired setter trace contains two
`+0x74` commits per tick and joins them by stage/sub/map, ordinal, source
buffer, and caller return PC:

```text
SET74_EVENT_SEMANTIC_JOIN: PASS
GLIDX24_SET74_CALLS_RT:
  ord 9, call 0x8011208C, caller func_80111FD4, source 0x800455CC, DIFF
  ord 10, call 0x801132F0, caller func_8011329C, source 0x80045578, NO-OP after ord 9
GLIDX24_SET74_CALLS_AN:
  ord 9, call 0x8011208C, caller func_80111FD4, source 0x800455CC, MATCH
  ord 10, call 0x801132F0, caller func_8011329C, source 0x80045578, NO-OP
FIRST_SET74_CALL_WITH_DIFFERENT_SOURCE: ord 9 / 0x8011208C
```

The initial `+0x74` value at the first glidx-24 setter matches in both
runtimes, so the pre-update eye boundary matches. The direct return value of
`func_801107F0` was not separately logged, therefore this is a pre-update
boundary proof, not a byte-level `801107F0` exit capture.

Static reconstruction of the active path is corrected as follows:

```text
func_80111FD4
  → func_801107F0(s0, sp+0x3C)
  → func_80111F70(s0, arg, sp+0x3C, *(s0+0x34)+0x78, *(s0+0x34)+0x88)
      → func_80111DB0
          → func_800EF174(working_eye, local_vector, scalar)
      → func_80111E88(working_eye_y, state fields)
  → func_8011106C(s0, sp+0x30)
      → func_800EE7F8(sp+0x30, s0+0x8C)
  → func_800EF04C(sp+0x3C, sp+0x30)
  → func_80110770(s0, sp+0x3C) @ 0x8011208C
```

`func_8011106C` is a three-float copy from `s0+0x8C`; it is not a delta
calculator. `func_800EF04C` is a deterministic three-float add. The static
code shows that `func_80111F70` can modify the working eye through
`80111DB0 → 800EF174`, but the artifact has no P0–P9 snapshots, no complete
`sp+0x30`/`s0+0x8C` pair, and no branch-flow trace. Consequently the exact
first discriminator cannot yet be assigned to `80111F70`, one of its helpers,
the `+0x8C` input to `8011106C`, or the apply step.

```text
GEMINI_A68_VERDICT:                 CONFIRMED
FUNC_80110A68_CLASS:                DOWNSTREAM_LOOKAT_TRANSFORM
GLIDX24_INITIAL_EYE_MATCH:          YES at pre-update +0x74 boundary
LAST_80111FD4_MATCH_PHASE:          initial eye / pre-update setter input
FIRST_80111FD4_DIFF_PHASE:          first observed at setter input to 0x8011208C
FIRST_80111FD4_DIFF_FIELD:          working eye X/Z (+0x3C source; +0x74 commit)
FIRST_80111FD4_DIFF_FUNCTION:       UNKNOWN inside 80111FD4 active path
FIRST_80111FD4_DIFF_PC:             UNKNOWN; 0x8011208C is the first observed commit PC
FUNC_80111F70_MODIFIES_WORKING_EYE: YES (static; dynamic phase NOT_CAPTURED)
FUNC_80111F70_OUTPUT_STATE_MATCH:   NOT_CAPTURED
FUNC_8011106C_EXACT_ROLE:           copy s0+0x8C -> sp+0x30
DELTA_RT:                           NOT_CAPTURED
DELTA_AN:                           NOT_CAPTURED
DELTA_MATCH:                        NOT_CAPTURED
COMPLETE_8011106C_INPUTS_MATCH:     NOT_CAPTURED
TRANSLATION_BRANCH_FLOW_MATCH:      NOT_CAPTURED
FIRST_BRANCH_FLOW_DIFFERENCE:       UNKNOWN
AN_STATIONARY_MECHANISM:            UNKNOWN (zero/skipped/not-applied/cancelled unmeasured)
EF04C_INPUTS_MATCH:                 NOT_CAPTURED
EF04C_OUTPUT_MATCH:                 NOT_CAPTURED
TRANSLATION_TO_EYE_EDGE:            PROVEN at the 0x8011208C setter boundary
EYE_TO_HEADING_EDGE:                PROVEN (complete 80110A68 input/output pair)
ROOT_CLASS:                         B-80111FD4-INTERNAL-TRANSLATION-UNRESOLVED
ROOT_CAUSE_LEVEL:                   upstream camera translation; exact discriminator unknown
```

The next single action is a new paired trace around the same glidx-24
`80111FD4` call: entry, `801107F0` exit, pre/post `80111F70`, pre/post
`8011106C` including `s0+0x8C` and `sp+0x30`, pre/post `800EF04C`, and the
`80110770` commit, plus branch predicates. No production fix is justified.

## 21. Inner translation discriminator — 2026-08-23T03:32Z

Capture `/tmp/p08-demo5-eye-inner-firstdiff-20260823T033246Z/` (`SHA256SUMS`
PASS twice, then `chmod a-w`). Semantic join by stage/sub/map, glidx,
`func_80111FD4` ordinal, and `arg=0x80045620` **PASS**.

Mandatory PCs on the CameraMode2 event:

```text
F80111F70_CALL_PC:     0x80112068
F80111DB0_CALL_PC:     0x80111F88
F800EF174_CALL_PC:     0x80111E5C
F8011106C_CALL_PC:     0x80112070
F800EF04C_CALL_PC:     0x8011207C
F80110770_COMMIT_PC:   0x8011208C
```

`func_800EFB24` is `*a0 = *a1 - *a2`. `func_800EEF94` returns length.
`D_8012655C` is `0x3c23d70a` (0.01). `bc1fl` at `0x80111DEC` skips
`800EF174` and writes 0 to `*a3` (`*(s0+0x34)+0x78`).

```text
S0_8C_TYPE:                 f32[3] at CameraMode2+0x8C/+0x90/+0x94
S0_8C_SEMANTIC:             UNKNOWN as a named velocity; PROVEN as 8011106C copy source
S0_8C_ACTIVE_WRITER:        none nonzero at glidx 23-24 (read/copy of zeros)
F8011106C_CLASS:            DOWNSTREAM_COPY
F800EF04C_CLASS:            DOWNSTREAM_ADD
```

Glidx 23 control: both skip `800EF174`; working eye X/Z unchanged; `s0+0x8C`
zeros; final eye MATCH (`45818c16,447db8e2,458225c9`).

Glidx 24: P0–P4 logged fields MATCH (working eye, arg pointer, threshold,
`a3_0=0`, `s0+0x8C=0`). First difference is `800EEF94` `f0` / skip vs apply:

- AN: `f0=3b200ccc` (0.0024), `c_lt_thr_f0=0`, `BR_SKIP_EF174` @ `0x80111E70`
- RT: `800EF174` runs; `dir=(c0ce6800,b8800000,c17a8800)` scale `3e972b3b`
  (0.2953); working eye becomes `45817cda,447db8e2,458200cd`

Working-eye `a2` MATCH implies `*a1` at `0x80045620` already DIFF (`800EFB24`
algebra). `*a1` was not hex-dumped; RT reconstructs ≈ `(4139.0605, 1014.8887,
4149.0649)` vs AN ≈ eye. This is **B-80111F70-INPUT / B-80111DB0-INPUT**, not
an A-candidate helper. `s0+0x8C` stays zero; 1106C/EF04C follow that zero.

```text
LAST_FULL_MATCH_PHASE:           P4 logged 11DB0 entry fields
FIRST_DIFFERENT_PHASE:           BR_EEF94 / P6
FIRST_DIFFERENT_FIELD:           *a1 @ 0x80045620
FIRST_DIFFERENT_FUNCTION:        800EFB24 / first observed 800EEF94 f0
FIRST_DIFFERENT_PC:              0x80111DEC
AN_STATIONARY_MECHANISM:         SKIPPED_TRANSLATION_BRANCH + S0_8C_ZERO
RT_TRANSLATION_VECTOR:           ~(-1.9043, 0, -4.6230)
AN_TRANSLATION_VECTOR:           (0, 0, 0)
ROOT_CLASS:                      B-80111F70-INPUT / B-80111DB0-INPUT
B_CAMERA_MATCH:                  NOT_CAPTURED
FIX_IMPLEMENTED:                 NO
```

Next: independently review, then follow **one** producer of `*0x80045620`
(`func_80111FD4` `a1`, after `80111A98`/`80111ABC`). No production fix.

## USER_DEMO_5 `a1@0x80045620` branch arbitration — 2026-08-23T03:32:46Z

Immutable capture: `/tmp/p08-demo5-eye-inner-firstdiff-20260823T033246Z/`.
Historical pre-interruption `sha256sum -c SHA256SUMS` passed twice; after
resume the directory is absent, so a fresh recheck is NOT_RECHECKABLE. No
production or generated-C change.

The branch claim is **CONFIRMED_WITH_CORRECTIONS**. `func_800EFB24` is exactly
`out = a1 - a2`; `func_800EEF94` computes the Euclidean length; and
`D_8012655C = 0x3c23d70a = 0.01f`. At `0x80111DEC`, `bc1fl` follows
`c.lt.s f4,f0`: false takes the likely branch, executes the delay-slot zero
of `a3`, and skips `800EF174`; true nullifies the delay slot and runs
translation at `0x80111E5C`.

At glidx 24, AN has `f0=0x3b200ccc` (`~0.0024`) and skips translation. RT
uses `dir=(c0ce6800,b8800000,c17a8800)` and scale `0x3e972b3b` (`~0.2953`),
producing `45817cda,447db8e2,458200cd`. The RT `a1`, reconstructed from the
exact delta and matching eye, is `4581587c,447db8e1,4581a885` ≈
`(4139.0605,1014.8887,4149.0649)`. AN raw `a1` bits were not dumped; only
`|a1-eye|~0.0024` was captured. Thus the branch result is proven, but the
complete dynamic writer join is not.

### Exact `a1` storage and immediate producer

`0x80045620` is `ncbapivot`'s stack-local `sp+0x48`, not a global and not
proven to be the target camera position. The active frame is:

```text
0x808000D0  801107F0(s0, sp+0x48)                         initial copy of s0+0x74
0x808001AC  800EE780(sp+0x48, sp+0x28, sp+0x54)            final writer
0x808001B8  80111FD4(s0, sp+0x48)                         consumer
```

`800EE780` is the immediate producer. Its exact operation is
`dst[i] = input_a1[i] + input_a2[i]`; the final vector combines the copied
target at `sp+0x28` with the processed vector at `sp+0x54`. `80111FD4` stores
the pointer at its own `sp+0x4C` and forwards it unchanged through
`80111F70 -> 80111DB0 -> 800EFB24`.

`80112524` produces the target at `sp+0x6C` from `*(s0+0x30)+0x4`; it is not
the final `a1` producer. `80111A98` and `80111ABC` are off this active event:
they are on the `8010FFD0 == 0` path and write `s0+0x34+0x5C` /
`s0+0x34+0x68`, not `sp+0x48`.

```text
A1_STORAGE_KIND:              STACK_LOCAL (ncbapivot sp+0x48)
A1_SEMANTIC_ROLE:              PROVEN intermediate camera-translation vector
A1_IMMEDIATE_PRODUCER:         func_800EE780 @ 0x808001AC
A1_INITIAL_WRITER:             func_801107F0 @ 0x808000D0
GLIDX23_A1_WRITERS:             STATIC_ONLY; dynamic timeline NOT_CAPTURED
GLIDX24_A1_WRITERS:             STATIC_ONLY; dynamic timeline NOT_CAPTURED
LAST_A1_WRITE_BEFORE_11F70:     800EE780 @ 0x808001AC
FIRST_MATCH_TO_DIFF_WRITE:      NOT_CAPTURED
800EE780_COMPLETE_INPUT_JOIN:   NOT_CAPTURED
ROOT_CLASS:                     B-800EE780-INPUT-SOURCE-UNRESOLVED
```

This supersedes the earlier wording that left `80111A98/80111ABC` as the
candidate producer. The next single diagnostic action is a paired trace of
`800EE780` with `sp+0x28`, `sp+0x54`, and destination `sp+0x48` at glidx 23–24;
no fix is justified by the current artifact.

## USER_DEMO_5 `func_800EE780` complete operand provenance — 2026-08-23T05:35Z

```text
PREVIOUS_RAW_ARTIFACT: /tmp/p08-demo5-eye-inner-firstdiff-20260823T033246Z/
PREVIOUS_RAW_ARTIFACT_STATUS: LOST / NOT_RECHECKABLE
PREVIOUS_LUNA_ARBITRATION: PRESERVED AS DOCUMENTED EVIDENCE
NEW_CAPTURE: /tmp/p08-demo5-ee780-source-20260823T053510Z/
SHA256_VERIFY: PASS twice then again after chmod a-w
```

### Minimum branch revalidation (glidx 24, arg=0x80045620)

```text
WORKING_EYE_A2_MATCH: YES  45818c16,447db8e2,458225c9
A1_MATCH:             NO
A1_RT:                4581587c,447db8e1,4581a885
A1_AN:                45818c1a,447db8e1,458225cc
EFB24_DELTA_RT:       c0ce6800,b8800000,c17a8800  (~-6.4502, -0.0001, -15.6582)
EFB24_DELTA_AN:       3b000000,b8800000,3ac00000  (~0.0020, -0.0001, 0.0015)
EEF94_LENGTH_RT:      ~16.9347  (from delta; RT BR_EEF94 wrap missed ra=0)
EEF94_LENGTH_AN:      3b200ccc (~0.0024) then BR_SKIP_EF174
BRANCH_80111DEC_RT:   TAKE 800EF174 (P6 work=800455cc scale=3e972b3b)
BRANCH_80111DEC_AN:   SKIP 800EF174
NEW_BRANCH_REPRODUCTION: PASS
```

Raw join lines (ncbapivot dest 0x80045620):

```text
RT [ud5-ee780] enter glidx=24 ord=40 pc=800ee780 ra=00000000 a0=80045620 a1=80045600 a2=8004562c a3=44621a8f old=(45818c16,447db8e2,458225c9 / 4145.5107,1014.8888,4164.7231) src_a1=(45992390,441e58b4,4570492c / 4900.4453,633.3860,3844.5732) src_a2=(c43e589d,43bec05a,43983ef2 / -761.3846,381.5027,304.4918)
RT [ud5-ee780] leave glidx=24 ord=40 dest=80045620 out=(4581587c,447db8e1,4581a885 / 4139.0605,1014.8887,4149.0649)
AN [ud5-ee780] enter glidx=24 ord=40 pc=800ee780 ra=801daad4 a0=80045620 a1=80045600 a2=8004562c a3=44621a8f old=(45818c16,447db8e2,458225c9 / 4145.5107,1014.8888,4164.7231) src_a1=(45992390,441e58b4,4570492c / 4900.4453,633.3860,3844.5732) src_a2=(c43cbbb0,43bec05a,43a01366 / -754.9326,381.5027,320.1516)
AN [ud5-ee780] edge_to_11fd4 glidx=24 dest=80045620 out=(45818c1a,447db8e1,458225cc / 4145.5127,1014.8887,4164.7246)
```

Glidx 23 control: same pointer triple; `src_a1`/`src_a2`/`out` MATCH
`45818c13,447db8e2,458225c6`.

### `func_800EE780` classification

```text
F800EE780_EXACT_ROLE:           elementwise f32[3] add  dst = a1 + a2
F800EE780_CLASS:                DOWNSTREAM TRANSFORM/COPY (add)
GLIDX24_EE780_COMPLETE_INPUTS_MATCH: NO
FIRST_EE780_INPUT_DIFF:         src_a2 (sp+0x54 / 0x8004562C)
FIRST_EE780_INPUT_DIFF_COMPONENT: X first (Z also DIFF; Y MATCH 43bec05a)
FIRST_EE780_INPUT_DIFF_RT_BITS: c43e589d
FIRST_EE780_INPUT_DIFF_AN_BITS: c43cbbb0
FIRST_EE780_INPUT_DIFF_SEMANTIC: PROVEN
EE780_OUTPUT_TO_A1_EDGE:        PASS
EE780_PRECALL_BRANCH_FLOW_MATCH: YES
EE780_DIFF_INPUT_WRITER:        func_800EEBF0
EE780_DIFF_INPUT_WRITER_PC:     800EF214 stores (via EEBF0)
ROOT_CLASS:                     B-800EEBF0-INPUT
```

`800EEBF0` scale `a2=44621a8f` MATCH. Its `src_dir` Y already differs
(`4391e610` vs `43927d8b` at sp+0x38). Stop one producer past EE780.

SHA256SUMS:

```text
453e0ac822de45e3cb60ca5eb6b7aeac872b5bc860c593a88da9600827899f0d  metadata.txt
42417b16fc972937a0eaee5e6ee5375c81bbb220fe37872cfe36a94a68209f77  static-ee780.txt
1e584eba54b5aa8431f9f54e5f7000823173b8fd933b5cf72177507ce5871726  rt64.log
616f049d9932f48f5976c7ecebc3895920a264ee62af5eafbe71ff7dde539eaa  oracle.log
8d5657f3efc96e0f252fc8dabe7ea32e4d3481df53e2c7b1da910c0b10bf7a11  paired-ee780.csv
a2fc8f890caead0c43a822d1d87e750c8341593cdc40fd76e305dbfb8a3abd45  glidx23-control.txt
0a9cc5b99c6b26021af2c4ebb32f6ea7d35d2ec2fbfdb147166c6a8378596dbc  glidx24-firstdiff.txt
b71dde7d992ab8fe6374d37dff9b36699429c79e6b27c29cd4b4295648949026  source-producer.txt
9eb0bad59e784eb7bfed6dc98753608f8a150d939e3020eccc4de962b760c9f8  branch-flow.txt
```

FIX_IMPLEMENTED: NO. COMMIT: NO. PUSHED: NO.
