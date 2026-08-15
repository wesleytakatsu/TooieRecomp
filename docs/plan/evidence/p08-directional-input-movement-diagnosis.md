---
title: "P08 — directional input / movement diagnosis"
description: "Narrow diagnosis of the user report that Banjo only moves toward the right side of the screen. Host-to-N64 pad conversion is proven bidirectional on e97; a playable user-controlled session was not reproduced autonomously."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, input, stick, movement, diagnosis]
---

# P08 — DIRECTIONAL INPUT / MOVEMENT DIAGNOSIS

```text
BASE_HEAD:                                 e97bd09041f74abd8dad56003664bec8975c150b
MODEL:                                     Grok 4.6 HIGH
MODE:                                      NARROW DIAGNOSIS ONLY
RIGHT_ONLY_MOVEMENT_REPRODUCED_ON_E97:     UNKNOWN
REPORT_SUPERSEDED_BY_MATRIX_FIX:           UNKNOWN (possible; not proven)
PHYSICAL_USER_SESSION:                     NOT REPRODUCED (no autonomous pad)
HOST_TO_PAD_BIDIRECTIONAL:                 YES (harness + runtime LEFT)
D_8007DB54_SCALE_SEEDED:                   YES (0xAD090010 → 0.0125)
FIX_IMPLEMENTED:                           NO
MATRIX_FIX_STILL_GOOD:                     YES
ROOT_CAUSE_LEVEL:                          E
```

## 1. User observation vs current HEAD

The user reported, after the game became startable:

- a playable session could be started;
- the character seemed to move only toward the **right side of the screen**;
- other directions did not produce the expected motion;
- some intro/demo movement also looked wrong.

That report may overlap the now-fixed render-submission matrix bug
([[p08-render-submission-matrix-fix]]). This note first asks whether
right-only motion still exists on `e97bd09`.

```text
PHYSICAL 4-DIRECTION PLAYABLE RETEST:   NOT PERFORMED
  reason: no host physical controller/keyboard events in this environment
SYNTHETIC HOST VECTOR INJECTION:        YES (TOOIE_DIAG_INPUT_VECTOR=left)
ATTRACT DEMO ON E97:                    YES (mode 5, map 335 then later maps)
```

Do **not** treat the user report as proven on current HEAD.

## 2. Input path used by Tooie

```text
SHARED_FRONTEND_PATH: YES
```

Tooie links BanjoRecomp's `RecompFrontend` and `N64ModernRuntime` and
registers the same callbacks:

- `recompinput::poll_inputs`
- `recompinput::profiles::get_n64_input`

`get_n64_input` builds normalized axes:

```text
cur_x = analog(X_AXIS_POS) - analog(X_AXIS_NEG)
cur_y = analog(Y_AXIS_POS) - analog(Y_AXIS_NEG)
```

Defaults (unchanged Tooie-side except C-button extras):

| input | keyboard | controller |
| --- | --- | --- |
| X_NEG | A | LEFTX negative half |
| X_POS | D | LEFTX positive half |
| Y_POS | W | LEFTY negative half (SDL Y inverted) |
| Y_NEG | S | LEFTY positive half |

SDL axis halves are clamped to `[0, 1]` then subtracted, so signedness
is reconstructed in the frontend. Deadzone is applied to the controller
stick only, then keyboard is added, then clamp `[-1, 1]`.

N64 conversion is `convert_to_n64_range` in
`BanjoRecomp/lib/N64ModernRuntime/ultramodern/src/input.cpp`.

Guest pad write (`osContGetReadData_recomp`):

```text
MEM_H(6*i + 0) = button
MEM_B(6*i + 2) = stick_x   // MEM_B is int8_t*
MEM_B(6*i + 3) = stick_y
MEM_B(6*i + 4) = err_no
```

N64 contract (`ultramodern/ultra64.h`):

```text
typedef struct {
    u16 button;
    s8  stick_x;   /* -80 <= stick_x <= 80 */
    s8  stick_y;   /* -80 <= stick_y <= 80 */
    u8  err_no;
} OSContPad;
```

Expected full-deflection N64 values from the real converter:

| direction | host (x,y) | pad (sx,sy) |
| --- | --- | --- |
| center | (0, 0) | (0, 0) |
| right | (+1, 0) | (+82, 0) |
| left | (-1, 0) | (-82, 0) |
| up | (0, +1) | (0, +82) |
| down | (0, -1) | (0, -82) |
| up-right | (+1, +1) | (+69, +69) |
| up-left | (-1, +1) | (-69, +69) |
| down-right | (+1, -1) | (+69, -69) |
| down-left | (-1, -1) | (-69, -69) |

Tooie guest decode (`func_8001608C`) uses **signed** `lb` at pad+2/+3,
then `func_80015D60(stick, deadzone=7, max=0x3B/0x3D)`. That is the
Banjo-Kazooie `func_8024E420` remap. Scale word `D_8007DB54` must be
`0xAD090010` so that `0xE955CCDD - word` is `0x3C4CCCCD` (0.0125).

`func_80016068` copies the remapped floats. `bakey_getStick` /
`func_800BCF90` / `bastick_update` consume them. Attract/demo may
replace the pad stream (`pfsManager` / `gsattract`) independently of
live host input.

## 3. Synthetic harness (real converter)

Command:

```sh
cmake --build build --target input_direction_test
./build/input_direction_test
```

```text
RESULT: PASS (exit 0)
HOST→PAD: all 9 vectors keep the expected sign pair
SIGNED_BYTE: -80 survives as s8 -80 (u8 view is 176)
SCALE IDENTITY: 0xAD090010 → 0.0125 PASS
UNSEEDED_BSS: 0xE955CCDD → -1.61543e+25 (not observed at runtime)
```

The test compiles `ultramodern/src/input.cpp` and calls
`convert_to_n64_range` directly. The algorithm is not reimplemented.

## 4. Runtime on e97 (`TOOIE_DIAG_INPUT=1`, vector=left)

```sh
TOOIE_AUTO_START=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
TOOIE_DIAG_INPUT=1 TOOIE_DIAG_INPUT_VECTOR=left TOOIE_DIAG_MOVEMENT=1 \
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
timeout -k 2s 90s ./build/TooieRecompiled
```

Exit 137 (timeout). No `LOOKUP_FUNC(0)`, no crash string.

Dominant live-conversion samples:

```text
host=(-1.000,0.000)
pad=(-82,0)
buttons=0000
scale=ad090010
derived=0.012500
remap=(-1.000,0.000)     # 500 samples after func_80015D60
```

```text
HOST_INPUT_CORRECT:              YES
N64_PAD_CORRECT:                 YES
TOOIE_RAW_STICK_CORRECT:         YES (lb path; -82 survives)
TOOIE_REMAP_CORRECT:             YES when live pad is remapped
D_8007DB54:                      0xAD090010 (seeded; not BSS zero)
```

Player `bastick` during mode-5 Wooded Hollow is **not** locked to +X:

```text
107× stick=( 0.000, 1.000)   attract/demo forward
  6× stick=( 1.000, 0.000)
  3× stick=(-1.000, 1.000)
plus mixed signed X with +Y
```

Injected LEFT does **not** become player bastick `(-1, 0)` while
`gsattract` ep1 is live. Attract/demo owns that stick stream. This
matches the earlier finding that attract movement is scripted
([[p08-visible-demo-movement-diagnosis]]).

Banjo world path still changes both X and Z (start `1861, -166`, then
a grounded walk, later scene drop). Not a +X-only integrator.

```text
WORLD_DIRECTION_COLLAPSED:                         NO (attract)
WORLD_DIRECTION_CORRECT_BUT_SCREEN_PROJECTION_WRONG: NO new proof
USER_CONTROLLED_VS_ATTRACT_SHARED_ROOT:            REFUTED
  (attract stick != live host pad)
```

## 5. Discriminated bug classes

| class | verdict |
| --- | --- |
| A signed/unsigned stick byte | **REFUTED** (pad=-82; MEM_B is s8; guest `lb`) |
| B one axis not propagated | **REFUTED** at host/pad (Y=0 is the injected LEFT) |
| C X/Y swapped | **REFUTED** at convert_to_n64_range |
| D negative values clipped | **REFUTED** |
| E deadzone implementation wrong | **REFUTED** for full-deflection LEFT |
| F angle/quadrant bug | **NOT REACHED** for user control |
| G camera-relative yaw bug | **NOT REACHED** for user control |
| H movement vector/integrator | **REFUTED** as attract first error (path still curves) |
| I render-only illusion | **POSSIBLE** historical overlap with the matrix bug |
| J other | attract/demo scripted stick; no playable file session |

Unseeded `D_8007DB54==0` would explode/invert **both** axes, not collapse
to right-only. Runtime shows the word **is** seeded. Do not treat that
as the current bug.

## 6. Matrix fix regression

```text
matrix_submission_test:     PASS (MAX_MATRIX_ERROR 0)
bcf28_helpers_test:         PASS
core1_ra_wrappers_test:     PASS
mode-5 Wooded Hollow:       reached on this run
```

Do not reopen the render-submission split.

## 7. Root-cause level

```text
ROOT_CAUSE_LEVEL: E
ROOT_CAUSE:
  The reported right-only user-control bug was not reproduced on e97.
  Host→N64 pad conversion is proven correct for all 8 directions
  (harness) and for injected LEFT at runtime. Guest scale word is
  seeded. Attract/demo stick is a separate scripted stream.
  A playable file-select session was not obtained autonomously, so a
  surviving user-control directional bug is neither proven nor fully
  refuted.

FIRST_BAD_INPUT_STAGE:      none proven on e97
FIRST_EXPECTED_VALUE:       LEFT → pad.stick_x < 0
FIRST_ACTUAL_VALUE:         pad.stick_x = -82

REFUTED_CANDIDATES:
  signed/unsigned stick byte at host write / guest lb
  convert_to_n64_range quadrant collapse
  D_8007DB54 unseeded scale as the current runtime state
  attract XZ integrator always +X
  render-submission matrix split (still good)
  PATCH4 / Hybrid F' / BCF28 / core1 RA wrappers
```

## 8. Fix design

Not applicable. No directional conversion writer is proven wrong.

```text
FIX_OWNER:           n/a
FIX_IMPLEMENTED:     NO
```

If a later playable session still shows right-only motion while
`TOOIE_DIAG_INPUT` prints `pad=(-82,0)` for LEFT and `pad=(+82,0)` for
RIGHT, search **after** the shared N64 pad boundary (Tooie attract
override / camera-relative yaw / file-select context). Do not patch
`convert_to_n64_range` or `osContGetReadData_recomp` on current evidence.

## 9. Future acceptance test

Deterministic, after a real playable spawn (not attract-only):

1. `TOOIE_DIAG_INPUT=1` and optional `TOOIE_DIAG_INPUT_VECTOR`.
2. LEFT → remapped X < 0 and world motion left-relative to camera.
3. RIGHT → remapped X > 0 and world motion right-relative.
4. UP → remapped Y > 0 and forward-relative motion.
5. DOWN → remapped Y < 0 and backward-relative motion.
6. Diagonals keep both signs.
7. Still required: `input_direction_test`, `matrix_submission_test`,
   `core1_ra_wrappers_test`, `bcf28_helpers_test` PASS; Wooded Hollow
   attract still renders upright.

## 10. Instrumentation retained

```text
TOOIE_DIAG_INPUT=1
TOOIE_DIAG_INPUT_VECTOR=center|right|left|up|down|up-right|up-left|down-right|down-left
```

No behavioral change when unset. Tooie source only. `RecompiledFuncs`
untouched. N64ModernRuntime / N64Recomp untouched.

Files:

- `include/tooie_diag.h`, `src/game/tooie_diag.cpp`
- `src/main/main.cpp` (get_n64_input wrap)
- `src/game/boot_hw_stubs.cpp` (`tooie_osContGetReadData`)
- `src/game/syscall_handler.cpp` (combined line)
- `tests/input_direction_test.cpp`, `CMakeLists.txt`

Raw log: `/tmp/tooie-p08-input/left-stderr.txt` (not committed).
