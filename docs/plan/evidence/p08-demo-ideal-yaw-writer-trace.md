---
description: "Original Pure-Interpreter store trace vs Recomp writer capture for ideal_yaw in the post-egg window. Exact writer = yaw_setIdeal store 0x8009C930; input already differs at glidx 385; first divergent input = player-control camera angle."
phase: P08
tags: [tooie-recomp, p08, measured-original, measured-recomp, ideal-yaw, store-trace, writer, camera]
---

# P08 — Ideal-Yaw Writer Store Trace

## Scope

Narrow differential only. No production fix was implemented.

```text
BASE_HEAD:                    c82986ad8a558ae43e5285699e4132a96864fbfa
ALIGNMENT:                    MEASURED_ORIGINAL[N] vs MEASURED_RECOMP[N+1]
WINDOW:                       glidx 368..406 (writer); post-egg 382..392 detail
MODE:                         mode=5, first Wooded Hollow clip, map=0
FIX_IMPLEMENTED:              NO
```

## Environment

The original Mupen64Plus environment was rediscovered at `/tmp/p08-m64p`
(core `6dca4c1`, Rice `038882d`). The core was rebuilt locally (user space,
no sudo) from the same official 2.6.0 source with a narrow store tracer
added to `pure_interp.c` (SW/SWC1/SD/SDC1 store interception for the live
yaw-struct fields). The Rice video plugin was rebuilt with a `cam_ctrl`
extension in the VI dump. Both are diagnostic-only and live outside the
TooieRecomp tree.

```text
MUPEN_SOURCE_FOUND:           YES  (/tmp/p08-m64p, official 2.6.0 source)
MUPEN_REBUILT:                YES  (core + rice, local, no sudo)
MUPEN_CORE_COMMIT:            6dca4c15370ac3e2171ce7b31426695f8f39b460
RICE_COMMIT:                  038882dd2e7cb660fd7512c0745032f17edd0bfa
CPU_MODE:                     Pure Interpreter (emumode 0, NO_ASM=1)
ROM_SHA1:                     af1a89e12b638b8d82cc4c085c8e01d4cba03fb3
ROM_MATCH:                    YES
```

## Live addresses

Guest memory is resolved live at trace time:

```text
ps_addr        = r32(D_80135490)
yaw_struct_ptr = r32(ps_addr + 0xF8)
IDEAL_YAW      = yaw_struct_ptr + 0x4
YAW            = yaw_struct_ptr + 0x0
PHYS_YAW       = r32(ps_addr + 0xC8) + 0x40
```

The main player struct (`D_80135490[0]`) changes during the clip
(`0x801d5d20` → `0x801d59d0` → `0x801d4970`), so the live ideal_yaw address
follows (`0x801d6aec` → `0x801d65dc` → `0x801d557c` across glidx
375/380/392). Recomp guest addresses are semantically identical.

## Original store trace (pure interpreter)

For every `SWC1/SW/SD/SDC1` store that hit the live `ideal_yaw`/`yaw`/`phys_yaw`
address during mode 5, glidx 368..406, the interpreter recorded:

```text
consumed_glidx, post_glidx, mode, map, store_pc, ra, target, old, new, srcbits, field
```

**Every** original ideal_yaw store in the first clip has:

```text
ORIGINAL_IDEAL_WRITER_PCS:  {0x8009C930}
                           (swc1 $f0, 0x4($t7) inside yaw_setIdeal 0x8009C914)
CALLER_RA:                  0x8009D0F4  (func_8009D088 dispatch case 1,
                           jal at 0x8009D0EC)
```

Stored value == writer input for the whole window (a1 input trace matches the
stored `new` exactly), so `yaw_setIdeal` stores its input unchanged here
(wrap is identity in this range).

## Recomp writer capture

The same semantic writer is confirmed in Recomp by a GDB breakpoint on
`yaw_setIdeal` (mode-5 armed), reading `ctx->r5` (a1 = writer input),
`ctx->r4` (a0 = ps), and the player-control camera angle. The writer PC is
statically identical (`funcs_299.c`, store at guest 0x8009C930).

```text
RECOMP_IDEAL_WRITER_PCS:      {0x8009C930}
WRITER_PC_MATCH:              YES
```

## Writer input already differs

Per-sample writer input (a1 to `yaw_setIdeal`), ORIGINAL writer-phase vs
RECOMP writer-phase:

```text
glidx | ORIG_in   | REC_in    | d_input
382   | 111.6114  | 111.6111  | -0.0002
383   | 107.0768  | 107.0766  | -0.0002
384   | 102.5875  | 102.5875  | -0.0000
385   |  99.3756  |  99.3081  | -0.0675   ← first
386   |  94.9919  |  94.9976  | +0.0056
387   |  92.1362  |  92.8194  | +0.6832
388   |  87.7321  |  89.4093  | +1.6772
389   |  87.4119  |  89.8159  | +2.4040
390   |  96.3813  |  99.7116  | +3.3303
391   | 100.7116  | 106.0839  | +5.3723
392   | 107.8779  | 113.6273  | +5.7494
```

`WRITER_INPUT_ALREADY_DIFFERS = YES` at glidx 385.

## Upstream formula decomposition

The writer input is computed by `bastick_getAngleRelativeToBanjo`
(`func_8009EEB8`) → `func_800CAF00`:

```text
a1 = wrap( camctrl + bang + 90 )
camctrl = *( D_80127708[*(ps+0x184)*4] + 0x1C )   (player-control camera angle)
bang     = *( *(ps+0x128) + 0x5C )                (bastick angle)
```

Per-sample writer-phase components (ORIGINAL vs RECOMP):

```text
glidx | ORIG bang   | REC bang    | ORIG camctrl | REC camctrl | d_camctrl
382   | 315.7252    | 315.7252    | 65.8862      | 65.8860     | -0.0002
383   | 315.7252    | 315.7252    | 61.3517      | 61.3514     | -0.0003
384   | 315.7252    | 315.7252    | 56.8624      | 56.8623     | -0.0001
385   | 315.7252    | 315.7252    | 53.6505      | 53.5829     | -0.0676  ← first
386   | 315.7252    | 315.7252    | 49.2668      | 49.2724     | +0.0056
387   | 315.7252    | 315.7252    | 46.4110      | 47.0943     | +0.6833
388   | 315.7252    | 315.7252    | 42.0070      | 43.6842     | +1.6772
389   | 318.4112    | 318.4112    | 39.0007      | 41.4047     | +2.4040
390   | 330.6422    | 330.6422    | 35.7391      | 39.0694     | +3.3303
391   | 340.7099    | 340.7099    | 30.0017      | 35.3740     | +5.3723
392   | 352.1709    | 352.1709    | 25.7069      | 31.4564     | +5.7495
```

The stick path is **not** the first divergent input:

```text
RAW_PAD_MATCH:            YES   (pad identical at every glidx)
REMAP_MATCH:              YES
STICK_MAG_MATCH:          YES
STICK_ANGLE_MATCH:        YES   (bang identical to 4 decimals at every glidx)
CAMERA_YAW_INPUT_MATCH:   NO    (player-control camctrl first differs at 385)
```

## Player-control camera source

The camera value consumed by the ideal-yaw computation is **not** the display
camera previously traced (`D_8012D500` matrix). It is the player-control camera
angle:

```text
PLAYER_CONTROL_CAMERA_SOURCE_ADDRESS: D_80127708[*(ps+0x184)*4] + 0x1C
PLAYER_CONTROL_CAMERA_SOURCE_FUNCTION: read via func_800A4C48 / func_800A93F8
                                       (0x800A9408) → func_800CAF00
SAME_AS_PREVIOUS_DISPLAY_CAMERA_FIELD: NO
```

The display camera (`D_8012D500`, `cam_yaw`) departs at glidx 386 — one step
after the player-control camera and after the ideal_yaw departure:

```text
glidx | ORIG disp_cam | RECOMP[N+1] | delta
384   | 307.80        | 307.90      | +0.10
385   | 312.30        | 312.30      | +0.00   (still matches)
386   | 315.10        | 314.50      | -0.60   ← display camera first departs
```

So the player-control camera divergence is **UPSTREAM OF PLAYER**; the earlier
`CAMERA_DIVERGENCE_RELATIVE_TO_PLAYER=AFTER_PLAYER` label is
**CORRECTED/SUPERSEDED** for the player-control source (the old trace watched
the display field).

## Delta classification

```text
INITIAL_0P07_DELTA_CLASS: PROVEN_UPSTREAM_DIVERGENT_VALUE (writer input already
                          differs at 385; it is not accumulated downstream FPU
                          noise in the writer — the writer input value itself
                          differs; an earlier camera selector/internal-state
                          divergence is not excluded)
```

The 0.07° is the first **observed upstream value** in the player-control camera
angle feeding the writer. Its origin (camera-control computation, selector, or
internal state) is the remaining discriminator; it is **not** in the stick path
and **not** in `yaw_setIdeal` arithmetic.

## Control run

The same store tracer / alignment over the earlier known-good turning region
(glidx 74..91) shows exact matches:

```text
EARLY_TURN_STORE_TRACE_CONTROL: PASS  (ideal_yaw delta 0.0000 at every glidx)
```

## Classification

```text
PRIMARY_CLASS:            SAME_WRITER_DIFFERENT_INPUT
                          first divergent input = CONTROL_CAMERA_INPUT
FIRST_BAD_STAGE:          writer input (a1) at yaw_setIdeal, glidx 385
FIRST_BAD_FUNCTION:       (upstream) player-control camera angle source feeding
                          func_800CAF00; writer itself is identical in both
FIRST_BAD_GUEST_PC:       writer store 0x8009C930 is SAME in both;
                          the camera-angle writer instruction is UNKNOWN
ROOT_CAUSE_LEVEL:         B  (exact writer + input provenance proven; first
                          divergent input is the player-control camera angle;
                          stick path refuted)
FIX_OWNER_CANDIDATE:      UNKNOWN (camera-control computation, not the yaw
                          writer, not the stick path)
FIX_IMPLEMENTED:          NO
```

No production behavior change is authorized by this note.

## Raw artifacts

Compact traces (not committed; kept under /tmp for reproduction):

```text
/tmp/p08-evidence-original-ideal-yaw-store-trace.csv   original store trace
/tmp/p08-evidence-recomp-ideal-yaw-writer.log          recomp writer capture
/tmp/p08-evidence-original-wooded-hollow.csv           original VI dump (+camctrl)
```

## Independent acceptance review — GPT-5.6 Luna (2026-08-17)

The static review accepts the DeepSeek result **WITH CORRECTIONS**. No
production behavior was changed.

```text
IDEAL_WRITER_PC_VERIFIED:       YES
IDEAL_WRITER_FUNCTION_VERIFIED: YES — 0x8009C914..0x8009C940
STORE_TARGET_IS_IDEAL_YAW:      YES — 0x8009C930 swc1 $f0, 0x4($t7)
CALLER_VERIFIED:                YES — func_8009D088 jump-table case 1,
                                      jal 0x8009D0EC, RA 0x8009D0F4
WRITER_PC_MATCH:                YES — original and Recomp 0x8009C930
WRITER_SET_COMPLETE:            UNKNOWN
LIVE_POINTER_TRACKING_CORRECT:  YES
OBJECT_ALIAS_RISK:              LOW — live pointer path is direct, but aliases
                                      and untraced store widths are not excluded
WRITER_EVENT_ALIGNMENT:         ORIGINAL consumed N/new vs RECOMP post N+1;
                                matches at N=382..384 and diverges at N=385
WRITER_ALIGNMENT_VERDICT:       PASS
```

The original MIPS/decomp and current generated function agree: `a1` is loaded
into `$f12`, normalized by `func_800136E4`, and stored through
`yaw_struct = *(ps+0xF8)` at `yaw_struct+0x4`. In case 1, the caller obtains
`a1` from `bastick_getAngleRelativeToBanjo` and passes the same `ps` in `a0`.
The observed inputs are in the identity range of the normalizer.

The complete input chain is proven statically:

```text
func_8009D088
  -> bastick_getAngleRelativeToBanjo (0x8009EEB8)
  -> func_800A4C48 (0x800A4C50: index = *(ps+0x184))
  -> func_800A93F8 (0x800A9408: D_80127708[index])
  -> func_800CAF00 (0x800CAF0C: +0x1C; 0x800CAF18..20: +90.0f)
  -> func_800136E4 (wrap)
  -> yaw_setIdeal (0x8009C930)
```

`D_80127708` is an eight-entry, four-byte-stride pointer table (`.space 0x20`);
`+0x1C` is proven to be the float field consumed as the control-angle term,
but the full record type and its writer are not established. `ps+0x184` is an
unnamed `s32` in `PlayerState` whose role here is the table selector. The
committed writer artifacts do **not** record that selector or the selected
pointer, so `ORIGINAL_INDEX_MATCH_RECOMP` is `UNKNOWN`; this limits the claim
to a proven upstream divergent value, not a proven first causal state.

The original tracer resolves the live yaw object on each store and observed
only `0x8009C930` for ideal-yaw stores in the window. It intercepts `SW/SWC1/
SD/SDC1`, however, and does not prove absence of `SB/SH`, DMA/memcpy, an
arbitrary alias, a secondary entry, or a native overwrite. Therefore
`WRITER_SET_COMPLETE=UNKNOWN` is a coverage limitation, not evidence against
the observed writer.

`D_8012D500` is a distinct display-camera object. The control-camera-to-player
dependency is proven; no direct control-camera-to-display-camera dependency is
proven, so `CAMERA_DEPENDENCY_DIRECTION=UNKNOWN` (the one-sample display lag is
temporal evidence only). Internal camera state may also diverge before `+0x1C`
first differs.

```text
STICK_PATH_REFUTED_AS_FIRST_CAUSE: YES
CAMCTRL_INTERNAL_STATE_MAY_DIVERGE_EARLIER: YES
GLIDX385_CLASS:                PROVEN_UPSTREAM_DIVERGENT_VALUE
ROOT_CAUSE_LEVEL_REVIEW:       B
REVIEW_VERDICT:                B — accepted with the corrections above
EXACT_NEXT_DISCRIMINATOR:      aligned trace of (ps, ps+0x184,
                                D_80127708[index], selected+0x1C) plus all
                                store widths/PCs to selected+0x1C; if the
                                selector matches, walk to the first internal
                                camera-control writer/state input
FIX_IMPLEMENTED:               NO
```

## Selector, store-set, and camera-state resolution (2026-08-17)

The review's exact next discriminator was executed
([[p08-demo-camera-control-divergence]]). Result:

```text
ORIGINAL_INDEX_MATCH_RECOMP:   YES  — selector ps+0x184 == 0 in both runtimes
                                   at every sample (367..405 original,
                                   1..480 recomp)
CAMCTRL_STORE_SET_COMPLETE:    YES  — all store widths + overlap detection;
                                   only 0x800CA65C (func_800CA628) touches
                                   selected+0x1C
CAMCTRL_WRITER_MATCH:          YES  — func_800CA628 (0x800CA628, store
                                   0x800CA65C) identical in both runtimes;
                                   caller func_8010FC38 / func_8010FE14
FIRST_INTERNAL_DIVERGENT_FIELD: camctrl +0x1C at glidx 385 (-0.0676 deg);
                                   eye +0x00 diverges in the same write
GLIDX385_CLASS_REFINED:        FIRST_CAUSAL_CAMERA_STATE (first
                                   player-consumed camera value; the internal
                                   input inside func_800E1A58 that first
                                   diverges remains)
ROOT_CAUSE_LEVEL_REVIEW:       B  (mechanism proven; one internal causal input
                                   to func_800E1A58 remains)
```
