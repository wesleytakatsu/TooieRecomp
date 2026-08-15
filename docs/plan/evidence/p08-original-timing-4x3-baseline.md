---
title: "P08-T03 — original timing / 4:3 baseline"
description: "Static NTSC timing contract, current baseline configuration audit, gated timing instrumentation, and validation blockers before high-FPS work."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, timing, vi, logic, animation, present, 4x3, baseline]
---

# P08-T03 — ORIGINAL TIMING / 4:3 BASELINE

## Classification

```text
BASE_HEAD:                         c3ad2e8d4c77e6c33c260415012f8bc1c5831315
P08_T10_DOC_STATUS:                VALIDATED / COMPLETE
P08_T03_STATUS:                    BLOCKED
BLOCKER_CLASS:                     E — insufficient live baseline evidence
TRANSFORM_TAGGING_IMPLEMENTED:    NO
SHARED_RT64_OR_RUNTIME_CHANGED:   NO
RECOMPILEDFUNCS_EDITED:            NO
```

P08-T03 is not marked validated. The source-level NTSC contract and the
static 4:3 render dimensions are established, but the current user config is
not an original baseline and the live 10–20 second measurements could not be
completed because this execution environment has no usable X11 display.

## Original NTSC timing contract

Primary artifacts used:

- `banjo-tooie-Recomp/src/core1/1E2B840.c` — scheduler event constants,
  scheduler thread, VI registration, and control timer.
- `banjo-tooie-Recomp/src/core1/1E2C940.c` — VI handler registration and
  `wait_one_frame`.
- `banjo-tooie-Recomp/src/core2/baanim.c` — `baanim_update`.
- `banjo-tooie-Recomp/include/gfx.h` and `src/core2/1EBCFC0.c` — original
  framebuffer and 4:3 orthographic dimensions.

The scheduler registers `SCHEDULER_EVENT_VI` and dispatches it from the
scheduler thread. NTSC uses `osViSetMode(&D_8003F488)`. The controller timer
is `((osClockRate / 60) * 2) / 3`, with the source comment identifying this
as two thirds of one NTSC VI (about 11.1 ms). This proves the intended VI
contract without assuming a game FPS from memory.

```text
EXPECTED_VI_HZ:                    60.000 Hz (NTSC)
EXPECTED_LOGIC_HZ:                 UNKNOWN — guest frame divisor not proven
EXPECTED_RENDERED_GAME_FRAMES_HZ: UNKNOWN — display-list submission count not measured
EXPECTED_ANIMATION_UPDATE_HZ:     UNKNOWN — baanim_update is a guest update call,
                                   but its live call rate was not measured
```

`recomp_get_target_framerate` returns `60 / frame_divisor` to the runtime.
That API contract is not sufficient to name the active Tooie divisor, so it
is recorded as `UNKNOWN` rather than converting it into an assumed 20/30/60
Hz result.

## Current host observations

The existing diagnostic reported syscall/RSP/audio counters only. This
checkpoint adds a small opt-in local diagnostic:

```text
TOOIE_DIAG_TIMING=1
TOOIE_DIAG_INTERVAL_MS=1000
```

It reports `host_update_gfx` and `guest_input_updates` once per interval.
These labels are deliberately boundary names:

```text
HOST_UPDATE_GFX_HZ:               NOT MEASURED — live window unavailable
HOST_PRESENT_HZ:                  UNKNOWN — update_gfx is not proven RT64 present
RT64_RENDER_HZ:                   UNKNOWN — no shared RT64 instrumentation added
GUEST_VI_HZ:                      NOT MEASURED — no live guest window
GAME_LOGIC_HZ:                    UNKNOWN — guest_input_updates is a candidate,
                                   not accepted as the logic boundary
ANIMATION_HZ:                     UNKNOWN
```

Therefore a prior observation of hundreds to roughly 1000 host callbacks is
not classified as a game-timing defect. `IS_SIMULATION_TIED_TO_HOST_PRESENT`
and `IS_ANIMATION_TIED_TO_HOST_PRESENT` remain `UNKNOWN`; the new counters
are diagnostic-only and do not alter scheduling.

## Configuration audit

The live configuration read before validation was:

```text
~/.config/TooieRecompiled/graphics.json
ar_option:                  Expand
cutscene_aspect_ratio_mode: Clamp16x9
hr_option:                  Clamp16x9
hpfb_option:                Off
res_option:                 Auto
rr_option:                  Display
rr_manual_value:            60
```

This is not the requested clean baseline. In particular, `ar_option=Expand`
and `cutscene_aspect_ratio_mode=Clamp16x9` prevent calling the final content
presentation original 4:3. No high-FPS or transform-interpolation switch was
observed in this config; the transform-tagging suite is also not enabled by
the current Tooie source. A future clean run must explicitly set the aspect
and cutscene modes to `Original`, keep high-precision framebuffer and other
enhancements off, and record the exact runtime resolution/pacing/VSync values
after the frontend has applied them.

```text
ORIGINAL_GAME_TIMING:      NOT YET VALIDATED
ORIGINAL_ASPECT:           OFF in live config; required value Original
HIGH_FPS_ENHANCEMENT:      OFF / no switch observed
TRANSFORM_INTERPOLATION:   OFF / unavailable in baseline
WIDESCREEN:                ON in live config (`ar_option=Expand`)
RT64_ENHANCEMENTS:         hpfb Off; remaining runtime values require live
                            config confirmation
INTERNAL_RESOLUTION:       Auto (not accepted as original until measured)
FRAME_PACING/VSync:        Display/60 in live config; not live-validated
```

## Aspect-ratio chain

The guest source defines `SCREEN_WIDTH=304` and `SCREEN_HEIGHT=228`.
Therefore:

```text
GUEST_VIEWPORT / FRAMEBUFFER:     304 x 228
GUEST_RENDER_ASPECT:              304 / 228 = 4:3 exactly
GUEST_SCISSOR:                    not measured in a live frame
PROJECTION_ASPECT:                original branch is 4:3; widescreen branch exists
HOST_DRAWABLE:                    not measured (no usable X11 window)
FINAL_GAME_CONTENT_ASPECT:        NOT MEASURED
4_3_BASELINE:                     BLOCKED
```

The guest dimensions are primary-source evidence, not proof that the current
host window preserves them. Black bars would be acceptable, but the host
drawable/content chain still requires a live capture.

## Three required states

```text
A. title/file-select:             NOT RE-RUN — current environment has no X11
B. Wooded Hollow attract mode-5:  NOT RE-RUN — current environment has no X11
C. Spiral Mountain gameplay:      NOT RE-RUN — current environment has no X11
```

The independent P08-T10 evidence remains authoritative for the documented
synthetic sequence: intro -> file select -> slot navigation -> confirmation
-> Spiral Mountain. It is now `VALIDATED / COMPLETE`; it is not rewritten as
timing evidence. The prior Wooded Hollow visual evidence remains `PASS` for
the upright green Jinjo, upright Banjo, elevated camera, and plausible path.

## Movement and transform-tagging readiness

`recomp_update_inputs` is now counted only when `TOOIE_DIAG_TIMING=1`, and
`update_gfx` is counted under the explicit `host_update_gfx` name. No claim is
made that either is a VI or simulation boundary until a clean runtime sample
correlates it with guest state.

```text
MOVEMENT_INTEGRATION_BOUNDARY:             UNKNOWN
IS_SIMULATION_TIED_TO_HOST_PRESENT:       UNKNOWN
IS_ANIMATION_TIED_TO_HOST_PRESENT:        UNKNOWN
IS_CAMERA_UPDATE_TIED_TO_HOST_PRESENT:    UNKNOWN
IS_RT64_PRESENT_FREE_RUNNING:              UNKNOWN
TRANSFORM_TAGGING_REQUIRED_FOR_BASELINE:   NO (no evidence of baseline dependency)
TRANSFORM_TAGGING_REQUIRED_FOR_HIGH_FPS:   YES (next enhancement scope)
```

No `gEXEnable`, new matrix group IDs, widescreen implementation, shared RT64
change, N64ModernRuntime change, N64Recomp change, or generated-code edit was
made here.

## Validation commands

```text
ninja -C build TooieRecompiled                         PASS, exit 0
./build/matrix_submission_test                         PASS, exit 0
./build/input_direction_test                           PASS, exit 0
./build/bcf28_helpers_test                              PASS, exit 0
./build/core1_ra_wrappers_test                          PASS, exit 0
```

The requested live run was attempted with `TOOIE_AUTO_START=1`,
`TOOIE_DIAG_TIMING=1`, `TOOIE_DIAG_FRONTEND=1`, and the Radeon Vulkan ICD.
With the normal audio path it was terminated after the timeout without a
usable timing window; with `SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=x11`, SDL
reported `x11 not available`. This is recorded as an environment limitation,
not as a game timing result.

## Next single action

Run the three states for 10–20 seconds in a session with a usable X11/Wayland
window, first applying an explicit `Original` aspect/cutscene config. Capture
the `TOOIE_DIAG_TIMING=1` lines and add guest VI, display-list, animation, and
state-boundary counters before deciding whether P08-T03 can become
`VALIDATED`.

## Documentation check

`markdown_vault_audit` (MCP) scanned 110 vault files: `errors=0`, with
pre-existing warnings/lint findings in historical notes. The standalone
`markdown_vault_audit` executable is not installed in this environment; the
repository-local audit result above is the applicable check.
