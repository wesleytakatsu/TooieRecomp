---
title: "P08 — title screen / file-selection input validation"
description: "Autonomous validation of Banjo-Tooie title screen and file-selection flow under synthetic input. Frontend input through CONT_A confirm is PASS. The later overlay names and 'direct Spiral Mountain gameplay' claim are CORRECTED/SUPERSEDED: confirm warps to MAP_A1_CS_INTRO (161) in mode=3. The later synthetic mode=1 FAIL is REFUTED — the sequencer does not inject the START presses that skip the opening story cutscenes."
status: evidence
phase: P08
updated: 2026-08-16
tags: [tooie-recomp, p08, input, frontend, title, file-select, validation]
---

# P08 — TITLE SCREEN / FILE-SELECTION INPUT VALIDATION

```text
CANONICAL_HEAD:                            a3d84dce564a7ff108a899b0777a1042e0415e85
MODE:                                      VALIDATION + NARROW DIAGNOSIS
FRONTEND_INPUT_DIVERGENCE:                 NONE (ZERO DIVERGENCE FOUND)
TITLE_VISIBLE:                             YES (mode=3 -> mode=2 transition)
START_REACHES_NEXT_STATE:                  YES (chgameselect loaded at t=22.02s)
FILE_SELECT_VISIBLE:                       YES (overlay 205 active, mode=2)
SLOT_NAVIGATION:                           PASS (STICK_LEFT and STICK_RIGHT consumed)
CONFIRM_RESULT:                            PASS (CONT_A confirmed selection)
GAME_START_TRANSITION:                     PASS as frontend confirm (see §6)
VALIDATION_STATUS:                         SUCCESS for frontend input (P08-T10)
P08_T10_FRONTEND_INPUT:                    PASS
DIRECT_SPIRAL_MOUNTAIN_TRANSITION_CLAIM:   CORRECTED/SUPERSEDED
CURRENT_T10_REGRESSION:                    REFUTED
```

## 1. Summary of Results

An autonomous synthetic input sequencer was integrated into the host input boundary (`tooie_diag::apply_synthetic_input`) and executed under full runtime conditions (`TOOIE_DIAG_FRONTEND=1`).

The automated test executed the full end-to-end frontend sequence:
1. **Startup Intro**: Rareware, Nintendo, and Intro cutscenes run at `mode=3, map=0`.
2. **Title Skip / Attract**: `CONT_START` (`0x1000`) injected to advance intro to File Selection.
3. **File Selection Menu Load**: Overlay 205 (`chgameselect`, section 804) loaded at `ram=0x80800000`, setting `mode=2`.
4. **Slot Navigation**: `STICK_LEFT` (`x = -1.0f`) and `STICK_RIGHT` (`x = +1.0f`) delivered through `recompinput` and `func_8001608C` to `chgameselect_entrypoint_1` (tick).
5. **Confirmation**: `CONT_A` (`0x8000`) injected at $t=26.03\text{s}$ to confirm file selection.
6. **Post-confirm map load**: `chgameselect` executed its destructor (`chgameselect_entrypoint_7`) and unloaded cleanly. The destination is **not** live Spiral Mountain gameplay. See §6.

**Conclusion**: There is **no missing or broken frontend input behavior** through file-select confirm. The later "direct Spiral Mountain / mode=1" wording is **CORRECTED/SUPERSEDED**.

---

## 2. Frontend State Machine & Functions Contract

```text
TITLE_STATE_FUNCTION:                      func_80800014_chattract (overlay 208) / func_808005AC_chintroticker (overlay 419)
FILE_SELECT_STATE_FUNCTION:                func_80805498_chgameselect / func_808027DC_chgameselect / func_80800AEC_chgameselect (overlay 205)
INPUT_CONSUMER_FUNCTIONS:                  bakey_pressed (0x800DA298), bakey_held (0x800DA3B8), bakey_getStick (0x800DA540), func_8001608C (pad reader)
STATE_TRANSITION_CHAIN:                    mode=3 (Intro) -> mode=2 (chgameselect) -> slot navigation -> CONT_A confirm -> chgameselect exit -> mode=3 / MAP_A1_CS_INTRO (161) opening story. Live mode=1 requires later START skips that the synthetic sequencer does not inject.
```

### State Probe Variables
- **Current Mode**: `D_80127630 + 0x2` (`mode_v = 3` for intro, `mode_v = 2` for file select, `mode_v = 1` for gameplay).
- **Current Map**: `D_80127630 + 0x10` (`map_v`).
- **Warp Index**: `D_80127630 + 0x12` (`warp_v`).
- **Active Overlay**: Dispatched via `recomp_syscall_handler` (`overlay_index == 205` for `chgameselect`, `208` for `chattract`, `419` for `chintroticker`).
- **Controller Pad 0 State**: `D_80079B40[0]` (`buttons`, `stick_x`, `stick_y`).
- **Remapped Pad State**: `D_80079AF8[0]` (`remap_x`, `remap_y`, `stick_mag`, `stick_angle`).

---

## 3. Input Button Contract

| Button | Host Mapping | N64 Pad Bitmask | Guest ButtonId Enum / Value |
|---|---|---|---|
| **START** | Keyboard `RETURN`, Gamepad `START` | `0x1000` (`CONT_START`) | `BUTTON_START` (`0x00`) via `bakey_pressed(0, 0)` |
| **A** | Keyboard `SPACE`, Gamepad `SOUTH` (`A`) | `0x8000` (`CONT_A`) | `BUTTON_A` (`0x08`) via `bakey_pressed(0, 8)` |
| **B** | Keyboard `LSHIFT`, Gamepad `WEST` (`X`/`Square`) | `0x4000` (`CONT_B`) | `BUTTON_B` (`0x09`) via `bakey_pressed(0, 9)` |
| **D-PAD UP** | Keyboard `UP`, Gamepad `DPAD_UP` | `0x0800` (`CONT_DPAD_UP`) | `BUTTON_D_UP` (`0x04`) via `bakey_pressed(0, 4)` |
| **D-PAD DOWN** | Keyboard `DOWN`, Gamepad `DPAD_DOWN` | `0x0400` (`CONT_DPAD_DOWN`) | `BUTTON_D_DOWN` (`0x05`) via `bakey_pressed(0, 5)` |
| **D-PAD LEFT** | Keyboard `LEFT`, Gamepad `DPAD_LEFT` | `0x0200` (`CONT_DPAD_LEFT`) | `BUTTON_D_LEFT` (`0x06`) via `bakey_pressed(0, 6)` |
| **D-PAD RIGHT** | Keyboard `RIGHT`, Gamepad `DPAD_RIGHT` | `0x0100` (`CONT_DPAD_RIGHT`) | `BUTTON_D_RIGHT` (`0x07`) via `bakey_pressed(0, 7)` |
| **ANALOG STICK**| Gamepad Left Stick / Directional Keys | Range $[-82, 82]$ | `D_80079B40[0].stick_x / y` $\to$ `D_80079AF8[0]` |

---

## 4. Execution Trace Log

```text
[tooie-fe] t=0.00s phase=0 (INTRO_WAIT) ovl=727 injecting btn=0000 stick=(0.0,0.0)
[tooie-fe] t=1.52s phase=1 (INTRO_SKIP_START) ovl=674 injecting btn=1000 stick=(0.0,0.0)
...
[tooie-fe] t=21.02s phase=1 (INTRO_SKIP_START) ovl=808 injecting btn=1000 stick=(0.0,0.0)
[tooie] ovl_load: idx=205 pkg=0x20358a0 +0x90 -> ram=0x80800000 text=0x7500 map=0x8550
[tooie-fe] t=22.02s phase=3 (GAMESELECT_INIT) ovl=205 injecting btn=0000 stick=(0.0,0.0)
[tooie-fe] t=22.02s state_change ovl=chgameselect (205) ep=0 mode=2 map=0 warp=0
[tooie-fe] t=22.02s state_change ovl=chgameselect (205) ep=2 mode=2 map=0 warp=0
[tooie-fe] t=22.02s state_change ovl=chgameselect (205) ep=1 mode=2 map=0 warp=0
[tooie-fe] t=23.03s phase=4 (GAMESELECT_STICK_LEFT) ovl=674 injecting btn=0000 stick=(-1.0,0.0)
[tooie-fe] t=23.53s phase=5 (GAMESELECT_SETTLE_LEFT) ovl=674 injecting btn=0000 stick=(0.0,0.0)
[tooie-fe] t=24.53s phase=6 (GAMESELECT_STICK_RIGHT) ovl=725 injecting btn=0000 stick=(1.0,0.0)
[tooie-fe] t=25.03s phase=7 (GAMESELECT_SETTLE_RIGHT) ovl=725 injecting btn=0000 stick=(0.0,0.0)
[tooie-fe] t=26.03s phase=8 (GAMESELECT_CONFIRM_A) ovl=725 injecting btn=8000 stick=(0.0,0.0)
[tooie-fe] t=27.03s phase=9 (GAMESELECT_WAIT_START) ovl=725 injecting btn=0000 stick=(0.0,0.0)
[tooie-fe] t=28.49s state_change ovl=chgameselect (205) ep=7 mode=2 map=0 warp=0
[tooie] ovl_load: idx=128  historically mislabeled chspiralmountain; actual baeggcursor
[tooie] ovl_load: idx=111  historically mislabeled chbanjohouse; actual baattach
[tooie] ovl_load: idx=148  historically mislabeled chjinjo; actual bashoes
```

Historical overlay names in this trace are **CORRECTED/SUPERSEDED** (see §6).

---

## 5. Verification Checklist

- [x] Base HEAD clean (`a3d84dce564a7ff108a899b0777a1042e0415e85`).
- [x] Regression tests (`matrix_submission_test`, `input_direction_test`, `bcf28_helpers_test`, `core1_ra_wrappers_test`) all PASS.
- [x] Synthetic input sequencer implemented at real input boundary.
- [x] Title / File-Select guest states and functions identified and documented.
- [x] Button bitfield contract documented.
- [x] Autonomous run completed: title screen $\to$ file select $\to$ slot navigation $\to$ confirm. The post-confirm destination is MAP_A1_CS_INTRO (161) / mode=3, not live Spiral Mountain.
- [x] No regressions introduced to runtime scheduler, matrix submission, or BCF28 binding.

---

## 6. Overlay identity and post-confirm destination (CORRECTED)

Gemini 3.7 Flash differential (current HEAD vs historical `c3ad2e8`): both
builds execute the **same** frontend transition after `CONT_A`.

```text
AFTER_CONT_A:
  chgameselect:
    warpVal  = 1
    mapValue = 161 / MAP_A1_CS_INTRO
  func_800A7D84:
    func_800A76F4(3)
    mode     = 3

OVERLAY_IDENTITY (CORRECTED):
  128 = baeggcursor     (old: chspiralmountain)
  111 = baattach        (old: chbanjohouse)
  148 = bashoes         (old: chjinjo)

P08_T10_FRONTEND_INPUT:                  PASS
DIRECT_SPIRAL_MOUNTAIN_TRANSITION_CLAIM: CORRECTED/SUPERSEDED
CURRENT_T10_REGRESSION:                  REFUTED
```

The later synthetic observation that confirm "fails" to reach `mode=1` is
**not** a current-HEAD frontend regression. The sequencer never injects the
later `CONT_START` presses that skip the opening story cutscenes. `c3ad2e8`
and current execute the same confirm path.
