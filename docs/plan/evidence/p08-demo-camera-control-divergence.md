---
description: "Bounded original/Recomp differential of the player-control camera state: selector (ps+0x184) MATCHES, exact camctrl writer is func_800CA628 (identical in both), and the camctrl writer input first diverges at glidx 385 (-0.0676 deg) with an exact camera->ideal_yaw amplification chain."
phase: P08
tags: [tooie-recomp, p08, measured-original, measured-recomp, camera, selector, camctrl, writer, func_800CA628, func_8010FC38]
---

# P08 — Player-Control Camera State Divergence

## Scope

Narrow differential only. No production fix was implemented.

```text
BASE_HEAD:                    8395cf44f9a0f8fe278283f0ca354d8c32375d1a
ALIGNMENT:                    MEASURED_ORIGINAL vs MEASURED_RECOMP
                              (demo-cam/RECOMP sampled at glrecord dispatch;
                              ORIGINAL sampled at yaw_setIdeal entry — see
                              CAMERA_WRITER_ALIGNMENT_METHOD)
WINDOW:                       glidx 360..405 (post-egg detail 382..392)
MODE:                         mode=5, Wooded Hollow demo, map=0
FIX_IMPLEMENTED:              NO
```

## Environment

Same reused environment as the prior ideal-yaw writer trace.

```text
MUPEN_ENV_REUSED:             YES  (/tmp/p08-m64p)
MUPEN_REBUILT:                YES  (core 6dca4c1 + rice 038882d, local, no sudo;
                              extended store tracer covers SB/SH/SWL/SWR/SW/SDL/SDR/
                              SD/SC/SCD/SWC1/SDC1 plus ps+0x184 and cam+0x1C)
ROM_SHA1:                     af1a89e12b638b8d82cc4c085c8e01d4cba03fb3
ROM_MATCH:                    YES
RECOMP_SOURCE:                build/TooieRecompiled at BASE_HEAD
```

The original Pure-Interpreter store tracer was extended this session to record:

- the selector tuple `(ps, ps+0x184, D_80127708[pidx], selected+0x1C)` at every
  `yaw_setIdeal` (0x8009C914) entry;
- every store of every width that hits `ps+0x184`, the yaw fields, or the live
  selected camera `+0x1C..+0x1F` (including overlapping/alias writes);
- the selected camera record fields `+0x00..+0x40` as floats at writer entry.

The Recomp side was captured in-process (no GDB) via a temporary env-gated
extension of `tooie_demo_sync_tick` that dumped the same selector + camera
fields per demo sample. The extension was reverted before this checkpoint.

## Camera writer alignment

```text
CAMERA_WRITER_ALIGNMENT_METHOD: RECOMP demo-cam samples at the glrecord dispatch
                                (overlay 698 ep0), i.e. AFTER the frame's movement
                                update; the ORIGINAL ideal-yaw ENTRY trace samples
                                at yaw_setIdeal entry, i.e. DURING the movement
                                update. The two sources therefore sit one sample
                                apart in the direction that yields the cleanest
                                pre-divergence match. With the ORIGINAL ENTRY
                                trace, RECOMP[N] camctrl == ORIGINAL[N+1] camctrl
                                to <0.0005 deg for glidx 380..384. The divergence
                                point is robust to this one-sample choice because
                                it is the first sample where the two stop
                                tracking under either alignment.
CAMERA_WRITER_ALIGNMENT_VERDICT: PASS  (pre-divergence camctrl match <0.0005 deg;
                                control-region check below)
```

## ps+0x184 (camera selector)

Static and dynamic characterization:

```text
PS_0X184_FIELD_SIZE:    4 bytes (lw/sw, s32; loaded by func_800A4C48 as index)
PS_0X184_SEMANTIC:      player-control camera slot selector; the value indexes
                        D_80127708 (8-entry pointer table) to pick the
                        player-control camera record consumed by
                        bastick_getAngleRelativeToBanjo -> func_800CAF00
PS_0X184_READERS:       func_800A4C48 (0x800A4C50: lw $a0, 0x184($a0)) is the
                        selector read on the ideal-yaw path; other readers exist
                        in player/actor code
PS_0X184_WRITERS:       - aligned8_memcpy (func_80019E70; store PC 0x80019E8C)
                          SD block copies that pass over ps+0x184 writing zero
                          (keeps the selector at 0); also seen on other objects
                        - func_8001BA84 region (SW at 0x8001BA84) writes pointer
                          values to a different object's +0x184
                        No selector-state machine write of a non-zero selector
                        was observed in the window.
ORIGINAL_SELECTOR_SEQUENCE: 0 at every writer call (glidx 367..405)
RECOMP_SELECTOR_SEQUENCE:    0 at every demo sample (glidx 1..480)
SELECTOR_MATCH_360_405:      YES
FIRST_SELECTOR_DIFFERENCE:   NONE
```

## D_80127708 table and selected record

```text
D_80127708_KIND:          8-entry pointer table (.space 0x20, stride 4) of
                          player-control camera records; adjacent to D_80127728
                          (display-camera pointer table, read by func_800A93E4)
TABLE_SLOT_MATCH:         YES  (index 0 selected by both runtimes at every sample)
SELECTED_RECORD_SEMANTIC_MATCH: YES  (same logical camera-control record: eye
                          position +0x00..+0x08, direction fields +0x0C..+0x18,
                          control yaw +0x1C, derived vectors +0x20..+0x40;
                          host/guest pointer values differ between runtimes but
                          the logical slot and field layout are identical)
CAMCTRL_PLUS_0X1C_SEMANTIC: player-control camera angle (yaw) consumed by
                          bastick_getAngleRelativeToBanjo: a1 = wrap(camctrl+bang+90)
                          -> yaw_setIdeal -> ideal_yaw
```

## Complete store provenance to selected+0x1C

```text
CAMCTRL_DIRECT_STORE_PCS_ORIGINAL: {0x800CA65C}  (swc1 $f6, 0x14($v1) inside
                          func_800CA628, entry 0x800CA628; v1 = dst+0xC so the
                          effective write lands on cam+0x1C)
CAMCTRL_STORE_SET_COMPLETE:  YES  (all widths SB/SH/SWL/SWR/SW/SDL/SDR/SD/SC/SCD/
                          SWC1/SDC1 intercepted; only 0x800CA65C ever touches
                          cam+0x1C..+0x1F in glidx 360..405)
ALIAS_OR_BLOCK_WRITE_RISK: LOW   (aligned8_memcpy overlaps ps+0x184 but never
                          cam+0x1C; no field=6 overlap event on camctrl)
```

The writer is a 3-word copy loop:

```text
func_800CA628(dst, src, src2):        # a0,a1,a2
    dst[0..2] = src[0..2]             # eye position words
    cam+0x1C = src2[2]                # the player-control camera angle
```

`src`/`src2` are produced by the camera-control step:

```text
CAMCTRL_PRIMARY_WRITER_PC:       0x800CA65C  (original) == func_800CA628 (recomp)
CAMCTRL_PRIMARY_WRITER_FUNCTION: func_800CA628 (0x800CA628)
CAMCTRL_PRIMARY_WRITER_CALLER:   func_8010FC38 (camera-control step; JAL 0x8010FCD8,
                                 RA 0x8010FCE0) and func_8010FE14 (thin wrapper;
                                 JAL 0x8010FE1C, RA 0x8010FE24)
CAMCTRL_WRITER_MATCH:            YES  (same generated MIPS in both runtimes)
```

Camera-control chain that computes the written vectors:

```text
func_8010FC38 (per-frame camera-control step, dispatch by mode)
  -> func_800CA8B4(cam, out1, out2)   # out1 = cam+0x00..0x08 (eye), out2 = cam+0x18..0x20
  -> func_800E1A58(out1, out2)        # camera positioning/collision; rewrites out2,
                                      #   out2[2] becomes the player-control yaw
  -> func_800CA628(cam, out1, out2)   # writes cam record + cam+0x1C = out2[2]
```

## Control region

```text
EARLY_CAMERA_WRITER_CONTROL: PASS
  glidx 74..91, camctrl (ORIGINAL VI sample vs RECOMP demo-cam) tracks to
  <0.005 deg after the +1 phase correction; no growth.
```

## Post-egg window

Aligned camctrl (`RECOMP[N]` vs `ORIGINAL[N+1]` for the ORIGINAL ENTRY trace):

```text
glidx | ORIG+1 camctrl | RECOMP camctrl | delta
382   |  65.8862       |  65.8860       | -0.0002
383   |  61.3517       |  61.3514       | -0.0003
384   |  56.8624       |  56.8623       | -0.0001
385   |  53.6505       |  53.5829       | -0.0676   <- first
386   |  49.2668       |  49.2724       | +0.0056
387   |  46.4110       |  47.0943       | +0.6833
388   |  42.0070       |  43.6842       | +1.6772
389   |  39.0007       |  41.4047       | +2.4040
390   |  35.7391       |  39.0694       | +3.3303
391   |  30.0017       |  35.3740       | +5.3723
392   |  25.7069       |  31.4564       | +5.7495
```

The selector and selected camera record match at every sample; the camctrl
(+0x1C) is the first player-consumed camera value that diverges.

## First internal camera divergence

```text
FIRST_INTERNAL_CAMERA_EXACT_DIFFERENCE:     glidx 385 (aligned), camctrl +0x1C
                                            -0.0676 deg AND camera eye +0x00
                                            ~+6.2 units appear together (both are
                                            written by the same func_800CA628 call)
FIRST_INTERNAL_CAMERA_MEANINGFUL_DIFFERENCE: glidx 385, camctrl -0.0676 deg
FIRST_INTERNAL_DIVERGENT_FIELD:              camctrl (+0x1C); eye +0x00 diverges
                                             simultaneously
FIRST_INTERNAL_DIVERGENT_GLIDX:              385
ORIGINAL_VALUE:                              camctrl 53.6505 deg
RECOMP_VALUE:                                camctrl 53.5829 deg
```

A small constant +0.05 unit offset in eye +0x08 exists from glidx 366 onward;
it is constant and consistent with the sampling-phase difference (demo-cam reads
after the movement update), not a growing game divergence, and it does not feed
the yaw writer. The first meaningfully diverging camera field consumed by the
player is camctrl at glidx 385.

## Writer input provenance

The camctrl value written by `func_800CA628` is `src2[2]`, produced by
`func_800E1A58` (camera positioning/collision step) from the camera record and
camera-mode/collision data. Its update is a per-frame integration; the exact
internal register/field that first diverges inside `func_800E1A58` is the
single remaining causal discriminator.

```text
CAMCTRL_WRITER_INPUT_ALREADY_DIFFERS: YES  (glidx 385, -0.0676 deg)
FIRST_BAD_CAMERA_INPUT:              camctrl angle (src2[2] -> cam+0x1C)
FIRST_BAD_CAMERA_INPUT_SOURCE:       func_800E1A58 output out2[2] feeding
                                     func_800CA628; stick path and selector
                                     REFUTED
```

## Same-writer / different-input

Both runtimes execute the identical `func_800CA628` (and identical caller chain);
the value entering the writer already differs at glidx 385. This is
`SAME_WRITER_DIFFERENT_INPUT` with the first divergent input being the
player-control camera angle.

## Control/display camera relationship

```text
CONTROL_DISPLAY_CAMERA_RELATION: SHARED_UPSTREAM_STATE
  The player-control camera (D_80127708[0]+0x1C) feeds the player yaw and first
  departs at glidx 385. The display camera (D_8012D500) is a distinct object that
  departs at glidx 386. No direct control->display (or display->control) write
  dependency is proven; both are camera systems driven by shared upstream
  player/camera state. The one-sample display lag is temporal evidence only.
```

## 0.0676 degree precursor

```text
GLIDX385_CLASS: FIRST_CAUSAL_CAMERA_STATE
  The camctrl at the aligned sample 385 is the first player-consumed camera
  value that diverges and is the direct input to the yaw writer; it is not FPU
  noise (the writer input itself differs) and not a downstream precursor (it
  feeds ideal_yaw). The exact internal camera input inside func_800E1A58 that
  first diverges is not yet isolated, so 385 is the first causal camera STATE,
  not the final instruction-level root cause.
```

## Error amplification link

```text
PLAYER_DRIFT_CHAIN_CONFIRMED: YES
  camera-control delta == ideal_yaw delta to 0.0001 deg at every key sample:

glidx | d_camctrl | d_ideal_yaw | pos error
385   |  -0.0676  |  -0.0675    |   0.05
387   |  +0.6833  |  +0.6832    |   0.07
389   |  +2.4040  |  +2.4040    |   0.25
392   |  +5.7495  |  +5.7494    |   3.00
400   |  +5.7770  |  +5.7800    |  21.39
```

The stick path (bang identical at every glidx) and the selector are REFUTED;
the camera-control value is the sole divergent writer input and its delta maps
1:1 onto the ideal_yaw drift that becomes the position error.

## Classification

```text
SELECTOR_MATCH:               YES  (ps+0x184 == 0 in both, every sample)
TABLE_SLOT_MATCH:             YES  (D_80127708[0] selected in both)
CAMCTRL_WRITER_MATCH:         YES  (func_800CA628, store 0x800CA65C)
CAMCTRL_STORE_SET_COMPLETE:   YES  (only 0x800CA65C; no alias/block write)
EARLY_CAMERA_WRITER_CONTROL:  PASS (glidx 74..91)
PRIMARY_CLASS:                SAME_WRITER_DIFFERENT_INPUT
                              first divergent input = CONTROL_CAMERA_INPUT
FIRST_BAD_STAGE:              camctrl (+0x1C) at the func_800CA628 write, glidx 385
FIRST_BAD_FUNCTION:           CORRECTED/SUPERSEDED — not func_800E1A58.
                              See [[p08-demo-camera-e1a58-differential]]:
                              E1A58 is identity; first meaningful src2[1] is
                              already at FE14/CA628 during func_8010FC38
FIRST_BAD_GUEST_PC:           writer store 0x800CA65C is SAME in both
ROOT_CAUSE_LEVEL:             R  (func_800E1A58 REFUTED as origin; see
                              [[p08-demo-camera-e1a58-differential]])
ROOT_CAUSE:                   CORRECTED/SUPERSEDED: cam+0x1C is out2[1], not
                              out2[2]; E1A58 does not produce the 385 value
FIX_OWNER_CANDIDATE:          UNKNOWN (FC38 mode-update helpers; not E1A58)
FIX_IMPLEMENTED:              NO
```

No production behavior change is authorized by this note.

## Raw artifacts

Compact traces (not committed; kept under /tmp for reproduction):

```text
/tmp/p08-camera-original-store-trace.csv   original extended store/entry trace
/tmp/p08-camera-original-wooded-hollow.csv original VI dump (+camctrl)
/tmp/p08-recomp-democam.log               recomp demo-cam selector+camera trace
/tmp/gdb-camera.cmd                       recomp GDB attempt (SIGBUS-limited;
                                          in-process capture used instead)
```

## Independent review hook

The previous independent review (GPT-5.6 Luna) required exactly the unknowns
addressed here. Result: `ORIGINAL_INDEX_MATCH_RECOMP` is now
**PROVEN (selector 0 in both)**, the complete store set to `selected+0x1C` is
**PROVEN ({0x800CA65C} only)**, the internal camera state does not meaningfully
diverge before +0x1C (the +0x08 +0.05 offset is a constant sampling-phase
artifact), and the selected camera-control record is a per-player camera-control
object whose +0x1C is the player-control yaw. The remaining discriminator is
the internal input inside `func_800E1A58` that first diverges.
**CORRECTED/SUPERSEDED** by [[p08-demo-camera-e1a58-differential]]:
`func_800E1A58` is identity on this path (all constraint slots type 1);
the first meaningful camctrl is already `src2[1]` at the FE14/CA628 write
during `func_8010FC38`, before CA8B4/E1A58.
