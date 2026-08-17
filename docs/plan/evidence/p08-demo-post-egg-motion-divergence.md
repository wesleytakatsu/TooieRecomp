---
description: "Focused MEASURED_ORIGINAL vs MEASURED_RECOMP walkback of the first persistent post-egg movement divergence."
phase: P08
tags: [tooie-recomp, p08, measured-original, measured-recomp, post-egg, movement, writer-walkback]
---

# P08 — Post-Egg Motion Divergence

## Scope and capture contract

This is a narrow differential only. No production fix was implemented.

```text
BASE_HEAD:                    c82986ad8a558ae43e5285699e4132a96864fbfa
PHASE_CORRECTED_ALIGNMENT:   MEASURED_ORIGINAL[N] vs MEASURED_RECOMP[N+1]
ORIGINAL_TRACE:               FRESH official mupen64plus 2.6.0 Pure Interpreter
                              + instrumented Rice dump (store-tracer core rebuilt
                              locally; no sudo); /tmp/p08-original-store-trace.csv
RECOMP_TRACE:                 REPRODUCED at current HEAD, first Wooded Hollow run
RECOMP_TRACE_ROWS:            793 first run; first 765 rows compared
PAD_BYTES_MATCH:              765/765 at input glidx N
FIX_IMPLEMENTED:              NO
```

The original capture remains the official mupen64plus 2.6.0 Pure Interpreter
trace documented in the full-motion note (core `6dca4c1`, Rice `038882d`,
ROM SHA-1 match). The user-local source was rediscovered at `/tmp/p08-m64p`
and the core/plugins were rebuilt locally (no sudo) with a narrow store tracer
in `pure_interp.c` plus a camctrl extension in the Rice VI dump; both are
diagnostic-only and live outside the TooieRecomp tree.

## Control and first split

The aggregate control region `glidx 1..365` gives `xz median=0.0224` and
`yaw median=0.0000°` under `+1`; the listed landmarks are near identity except
for transition-boundary samples 322 and 354–356. At glidx 322, the same-index
shot-stop position is `0.051` units and `recomp[323].bs=7`; the `+1` position
comparison crosses a repeated/advancing capture sample and is not gameplay
evidence. The input bytes still match exactly by glidx.

```text
CONTROL_REGION_XZ_MEDIAN:       0.0224 units (1..365, +1)
CONTROL_REGION_YAW_MEDIAN:      0.0000 degrees (1..365, +1)
CONTROL_STATE_MATCH:            364/365 (one capture-boundary sample)
PAD_BYTES_MATCH_SELECTED:       765/765 = 100%

FIRST_PHASE_CORRECTED_EXACT_DIFFERENCE:
  glidx 385, ideal_yaw: original 99.38°, recomp[386] 99.31°
FIRST_PHASE_CORRECTED_MEANINGFUL_DIFFERENCE:
  glidx 392, position XZ error 2.997 units
FIRST_BAD_PHYSICS_FIELD:        ideal_yaw (before velocity/position)
```

The first meaningful split is therefore not `bs=21`, `tspd`, or the slope
multiplier. At glidx 385, `bs=21`, `tspd=700.00`, and `mult=1.0000` still
match. The observed sequence is:

```text
ideal_yaw small delta @385 → phys_yaw delta @386 → yaw delta @387
→ speed delta >0.05 @388 → position >2.0 @392
```

The first persistent growth is a turn drift that becomes a jump-trajectory
drift, not a one-time state offset.

## State-machine audit

```text
ORIGINAL:    bs1(350-355) → bs7(356-365) → bs20(366) → bs22(367-371)
             → bs21(372-408) → bs22(409-424) → bs8(425-458)
             → bs16(459) → bs2(460)

RECOMP[+1]:  bs1(350-354) → bs4(355) → bs7(356-365) → bs20(366)
             → bs22(367-371) → bs21(372-408) → bs22(409-424)
             → bs8(425-460)

FIRST_STATE_SEQUENCE_DIFFERENCE: capture-boundary bs4 at 355;
                                 later bs16/bs2 versus bs8 at 459-460
POST_EGG_STATE_DIFFERENCE:       NONE through the persistent split;
                                 bs21 and the subsequent bs8 transition match
```

The retained trace does not expose animation ID/normalized phase, state timer,
movement timer, floor normal, or a direct ground-contact bit:

```text
ANIMATION_PHASE_MATCH_AT_SPLIT: UNKNOWN
STATE_TIMER_MATCH_AT_SPLIT:     UNKNOWN
GROUND_STATE_MATCH:             UNKNOWN
FLOOR_NORMAL_MATCH:             UNKNOWN
SLOPE_MULT_MATCH:               YES (1.0000/1.0000)
```

The first bad field is yaw-intention state, so this missing timer/animation
telemetry is not needed to place the first numerical split. It remains a
follow-up discriminator if a writer-level review is requested.

## Narrow table: meaningful split -10 through +20

Rows are `original[N]` versus `recomp[N+1]`; `pad[N]` is shown once because
the input bytes match 100% at the same consumed glidx. `substate`, animation
ID, and animation phase were not emitted by the retained probe and are `—`.

```text
glidx pad[N] bs O/R  pos O -> R (x,y,z)                         err  speed O/R   tspd O/R  yaw O/R       ideal O/R       mult O/R
382   67,-60,2000 21/21 (-4868.5,-433.2,964.4)->(-4868.5,-433.2,964.4) 0.05 635.28/635.28 700/700 122.92/122.92 111.61/111.61 1/1
383   67,-60,2000 21/21 (-4839.9,-431.3,948.9)->(-4839.9,-431.3,949.0) 0.06 650.52/650.52 700/700 116.98/116.98 107.08/107.08 1/1
384   67,-60,2000 21/21 (-4820.1,-430.0,939.7)->(-4820.1,-430.0,939.7) 0.05 656.00/656.00 700/700 113.38/113.38 102.59/102.59 1/1
385   67,-60,2000 21/21 (-4788.9,-428.0,928.6)->(-4788.9,-428.0,928.7) 0.05 661.92/661.92 700/700 108.13/108.10 99.38/99.31 1/1
386   67,-60,2000 21/21 (-4767.6,-426.6,922.3)->(-4767.6,-426.6,922.4) 0.06 664.55/664.52 700/700 104.84/104.83 94.99/95.00 1/1
387   67,-60,2000 21/21 (-4734.9,-424.5,915.7)->(-4734.9,-424.5,915.8) 0.07 667.28/667.28 700/700 100.08/100.32 92.14/92.82 1/1
388   67,-60,2000 21/21 (-4712.9,-423.2,912.3)->(-4712.9,-423.2,912.3) 0.01 668.73/668.99 700/700 96.99/97.60 87.73/89.41 1/1
389   67,-55,2000 21/21 (-4690.7,-421.8,910.2)->(-4690.7,-421.8,909.9) 0.25 669.12/669.91 700/700 94.60/95.65 87.41/89.82 1/1
390   70,-38,2000 21/21 (-4657.1,-419.7,909.0)->(-4657.1,-419.7,907.9) 1.09 671.72/672.74 700/700 95.27/97.17 96.38/99.71 1/1
391   72,-26,2000 21/21 (-4634.7,-418.4,907.8)->(-4634.8,-418.3,905.9) 1.86 673.27/673.54 700/700 96.63/99.40 100.71/106.08 1/1
392   73,-15,2000 21/21 (-4612.4,-417.0,905.6)->(-4612.6,-417.0,902.6) 3.00 673.59/672.51 700/700 99.44/102.96 107.88/113.63 1/1
393   75,-7,2000 21/21 (-4579.4,-414.9,899.3)->(-4580.3,-414.9,894.0) 5.42 671.89/669.52 700/700 103.68/108.12 110.74/116.72 1/1
394   77,-3,2000 21/21 (-4557.6,-413.5,894.0)->(-4559.1,-413.6,886.9) 7.25 671.86/669.51 700/700 103.35/108.22 102.39/108.44 1/1
395   78,-2,2000 21/21 (-4535.8,-412.2,888.8)->(-4537.8,-412.2,879.8) 9.24 673.74/672.19 700/700 101.79/106.82 97.11/102.64 1/1
396   78,-1,2000 21/21 (-4513.8,-410.8,884.3)->(-4516.3,-410.8,873.3) 11.29 674.29/673.30 700/700 99.33/104.27 91.93/96.63 1/1
397   78,-1,2000 21/21 (-4491.6,-409.4,880.9)->(-4494.6,-409.4,868.0) 13.27 673.59/672.63 700/700 95.92/100.91 85.69/90.83 1/1
398   78,-1,2000 21/21 (-4458.0,-407.3,879.1)->(-4461.4,-407.4,863.2) 16.24 671.65/671.00 700/700 89.86/95.14 79.76/85.53 1/1
399   78,-1,2000 21/21 (-4424.5,-404.1,880.7)->(-4427.9,-404.1,861.7) 19.34 670.68/670.49 700/700 83.20/88.66 72.12/77.85 1/1
400   78,4,2000 21/21 (-4402.7,-400.6,883.4)->(-4405.9,-400.5,862.3) 21.39 659.62/659.02 700/700 78.61/84.15 64.82/70.60 1/1
401   73,31,2000 21/21 (-4371.4,-395.6,891.8)->(-4374.0,-395.4,867.5) 24.42 648.42/647.04 700/700 80.57/86.21 83.85/89.65 1/1
402   69,44,2000 21/21 (-4350.4,-392.3,896.4)->(-4352.6,-392.0,870.0) 26.50 645.73/644.32 700/700 82.42/88.09 87.94/93.72 1/1
403   66,56,2000 21/21 (-4318.6,-387.2,900.8)->(-4320.6,-386.7,871.2) 29.67 642.20/640.99 700/700 86.05/91.77 92.12/97.90 1/1
404   64,67,2000 21/21 (-4286.6,-382.0,902.7)->(-4288.7,-381.4,869.9) 32.88 640.06/639.18 700/700 87.49/93.23 89.88/95.66 1/1
405   64,69,2000 21/21 (-4254.6,-376.7,903.9)->(-4256.7,-376.5,867.9) 36.09 641.12/640.34 700/700 86.63/92.39 85.21/90.98 1/1
406   63,69,2000 21/21 (-4222.5,-372.0,905.7)->(-4224.4,-372.3,866.4) 39.31 642.22/646.42 700/700 84.52/90.29 81.00/86.81 1/1
407   56,69,2000 21/21 (-4190.3,-367.8,909.0)->(-4192.0,-368.0,866.4) 42.56 647.10/649.27 700/700 82.46/88.25 79.03/84.83 1/1
408   17,76,2000 21/21 (-4158.1,-363.6,913.5)->(-4159.5,-363.8,867.7) 45.82 650.12/651.07 700/700 92.05/97.84 108.03/113.83 1/1
409   6,81,a000 22/22 (-4127.6,-332.0,898.3)->(-4130.6,-332.1,849.4) 48.92 682.38/683.36 700/700 101.24/107.02 116.55/122.33 1/1
410   6,81,a000 22/22 (-4097.0,-303.3,883.0)->(-4101.6,-303.5,831.2) 52.07 684.23/685.10 700/700 106.98/112.76 116.55/122.33 1/1
411   6,81,a000 22/22 (-4066.3,-277.6,867.8)->(-4072.5,-277.8,812.9) 55.26 685.87/686.65 700/700 110.57/116.35 116.55/122.33 1/1
412   6,81,a000 22/22 (-4035.4,-254.9,852.7)->(-4043.3,-255.1,794.7) 58.48 687.33/688.03 700/700 112.81/118.59 116.55/122.33 1/1

substate/animation ID/animation phase: — for all rows (not sampled).
```

## Writer walkback boundary

The measured owner chain is:

```text
identical pad[N] → player-control camera angle → bastick / ideal_yaw writer
→ ideal_yaw → phys_yaw/yaw → velocity → position
```

The exact runtime writer and its input are now PROVEN by the
[[p08-demo-ideal-yaw-writer-trace|writer store trace]] (2026-08-17): the
original Pure-Interpreter store tracer and the recomp GDB writer capture both
hit `yaw_setIdeal` (store guest PC `0x8009C930`), and the value entering that
writer **already differs at glidx 385**.

```text
ORIGINAL_IDEAL_WRITER_PCS:       {0x8009C930}  (yaw_setIdeal, entry 0x8009C914)
RECOMP_IDEAL_WRITER_PCS:         {0x8009C930}  (same generated function)
WRITER_PC_MATCH:                 YES
IDEAL_YAW_WRITER_FUNCTION:       yaw_setIdeal (0x8009C914)
IDEAL_YAW_STORE_PC:              0x8009C930  (swc1 $f0, 0x4($t7))
CALLER_FUNCTION:                 func_8009D088 (dispatch case 1; jal at 0x8009D0EC,
                                 RA 0x8009D0F4)
WRITER_INPUT_ALREADY_DIFFERS:    YES — glidx 385 a1 = 99.3756 vs 99.3081 (-0.0675°)
FIRST_DIVERGENT_WRITER_INPUT:    player-control camera angle
                                 D_80127708[*(ps+0x184)*4] + 0x1C
FIRST_UPSTREAM_DIVERGENT_GLIDX:  385
ROOT_CAUSE_LEVEL:                B  (identical writer; first divergent input =
                                 player-control camera angle; stick path refuted)
```

The stick path is NOT the first divergent input: pad, remap, bastick magnitude
and bastick angle match exactly (e.g. `bang` = 315.7252° in both at glidx 385).
The ideal_yaw writer computes `a1 = wrap(camctrl + bang + 90)`; `camctrl`
(player-control camera angle) first differs at glidx 385. See the writer-trace
note for the full per-glidx 382..392 table.

```text
RAW_PAD_MATCH:                   YES
REMAP_MATCH:                     YES
STICK_MAG_MATCH:                 YES
STICK_ANGLE_MATCH:               YES
PLAYER_CONTROL_CAMERA_MATCH:     NO (first differs glidx 385)
PRE_WRAP_IDEAL_MATCH:            NO (first differs glidx 385)
WRAP_RESULT_MATCH:               NO (first differs glidx 385)
CAMERA_DIVERGENCE_RELATIVE_TO_PLAYER: UPSTREAM_OF_PLAYER  (player-control camera
                                 feeds the writer; the DISPLAY camera field
                                 D_8012D500, traced previously, departs at 386)
```

This is sufficient to close the broad capture/state/physics hypotheses and the
`bastick_update → func_80013B70` stick-angle hypothesis. No speed, dt, camera,
collision, state, or generated-C change is authorized by this note.

## Controls and residual relationship

The player-control camera angle (`D_80127708[*(ps+0x184)*4] + 0x1C`) is an
**upstream input** to the ideal-yaw writer and first diverges at glidx 385. The
**display** camera (`D_8012D500`, the field previously traced) departs at
glidx 386, after the player field — it is presentation/downstream, not the
player-control source. This **CORRECTS/SUPERSEDES** the earlier
`CAMERA_DIVERGENCE_RELATIVE_TO_PLAYER=AFTER_PLAYER` label for the player-control
source. The captured actor scan still reports digger `(0,0,0)` in both traces,
so autonomous-actor parity is `UNKNOWN` rather than a false match.

```text
CAMERA_DIVERGENCE_RELATIVE_TO_PLAYER: UPSTREAM_OF_PLAYER (player-control camera;
                                   display camera D_8012D500 is AFTER_PLAYER)
PLAYER_CONTROL_CAMERA_SOURCE:      D_80127708[*(ps+0x184)*4] + 0x1C
SAME_AS_PREVIOUS_DISPLAY_CAMERA_FIELD: NO
CONTROL_ACTOR_MATCH:                  UNKNOWN
POST_EGG_SPLIT_AFTER_FIRST_SHOT:      YES
CAN_CAUSE_FIRST_SHOT_MISS:             NO
DRIFT_CLASS:                           TURN_DRIFT → JUMP_TRAJECTORY_DRIFT
```

The first egg was already fired at glidx 322; the later movement split cannot
cause that already-fired shot to miss. Egg collision remains closed.

## Independent camera-provenance review — GPT-5.6 Luna (2026-08-17)

The writer review accepts the camera-control provenance **WITH CORRECTIONS**.
The same `yaw_setIdeal` store (`0x8009C930`) is confirmed in both runtimes and
the aligned writer input differs at the corresponding glidx 385 event. The
classification is therefore `PROVEN_UPSTREAM_DIVERGENT_VALUE`, not
`PROVEN_FIRST_CAUSAL_VALUE`: the selector `ps+0x184`, the selected
`D_80127708` pointer, and internal camera state were not recorded in the
committed writer artifacts. The next discriminator is the aligned tuple
`(ps, ps+0x184, D_80127708[index], selected+0x1C)` plus complete-width store
tracing to `selected+0x1C`.

`D_8012D500` remains a distinct display-camera object; its later departure does
not prove a direct dependency from or to the player-control source.

## Camera-control divergence — resolved (2026-08-17)

The selector/store-set/internal-state discriminator was executed
([[p08-demo-camera-control-divergence]]). The selector `ps+0x184` is **0 in
both runtimes at every sample**, the selected `D_80127708[0]` record is the
same logical camera-control object, the camctrl store set is complete
(`{0x800CA65C}` = `func_800CA628` only, no alias/block write), and the
camctrl writer input first diverges at glidx 385 (`-0.0676` deg). The camera
delta maps 1:1 onto the ideal_yaw delta (`d_camctrl == d_ideal` to 0.0001),
confirming the player drift chain.

```text
SELECTOR_MATCH:                 YES  (ps+0x184 == 0, both runtimes)
TABLE_SLOT_MATCH:               YES  (D_80127708[0])
CAMCTRL_WRITER_MATCH:           YES  (func_800CA628 / 0x800CA65C)
CAMCTRL_STORE_SET_COMPLETE:     YES
FIRST_INTERNAL_DIVERGENT_FIELD: camctrl +0x1C at glidx 385
CAMCTRL_WRITER_INPUT_DIFFERS:   YES  (glidx 385, -0.0676 deg)
GLIDX385_CLASS_REFINED:         FIRST_CAUSAL_CAMERA_STATE
PLAYER_DRIFT_CHAIN_CONFIRMED:   YES
ROOT_CAUSE_LEVEL:               B  CORRECTED/SUPERSEDED by
                                   [[p08-demo-camera-mode-producer-differential]]
                                   (mode 2 / 80110588 PROVEN; 80110790 a1
                                   already differs at 384)
```
