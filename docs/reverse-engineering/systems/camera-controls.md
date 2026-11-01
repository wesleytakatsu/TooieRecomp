---
title: "Camera controls and orbit filters"
description: "Native digital camera requests, player ownership, orbit filters and pre-collision integration."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-11-01
tags: [reverse-engineering, camera, input, multiplayer]
---

# Camera controls and orbit filters

## Digital requests (`PROVEN`, static MIPS)

`func_800A4878(PlayerState*)` resolves the native camera node through `func_800A4CA8`, checks camera/player
availability, and calls `bainput_should_rotate_camera_left` (`0x80097B70`) or
`bainput_should_rotate_camera_right` (`0x80097B94`). `func_80110EFC(node, action)` checks the current
camera mode's manual-control flag before dispatching `func_80110E00`. Actions 0/1 request -45/+45 degrees
through `func_801111D0`; action 2 is recentering. Repeated digital requests are not a continuous analog input.

The focus predicate `func_800F65D0` normally compares its index with `0x801354DC`; the analog integration does
not change that focus or broaden the original digital path. Its independent eligibility resolves the node's owner.

## Ownership and camera state (`PROVEN`, static MIPS)

| Address/offset | Contract |
| --- | --- |
| `0x80135490 + 4*player` | `PlayerState*` table |
| `PlayerState + 0x184` | Native camera index; `func_800A4C88` resolves it via `func_800A93E4` |
| `0x80127728 + 4*camera` | Native camera object pointer; eight slots traversed by `func_800A95C4` |
| Camera object `+0x4` | ba-camera node, returned by `func_80110014` |
| Camera object `+0x16` | Camera class, queried by `func_8010FAE4`; `func_800A4D40` tests class 2 |
| Node `+0x64` | Owning camera object |
| Node `+0x68` | Player slot/index; read by `func_80110840` |
| Node `+0x6C` | Camera controller mode/table index |
| Node `+0x70` / `+0x72` | Update enabled / locked update state checked by `func_80110588` |
| Node `+0x74` / `+0x80` | Position / rotation copied by `func_80110848` |
| Node `+0x60` | Yaw restriction/transition substate |
| Node `+0x3C` | Pitch restriction/transition substate |
| `0x801247A0 + 16*mode` | Camera controller table: init/update/end pointers at +0/+4/+8, manual flag +0xC, recenter policy +0xE |

`bakey_getControllerIndex` (`0x800918F8`) resolves `PlayerState + 0x40 -> +0x0` and reads the native
input-buffer controller byte at `+0x6` through `func_800BCCF4`. Player slot, camera index and controller
port must not be assumed interchangeable.

Native room identities come from `decomp/include/maps.h`. Some minigames share larger story maps, so a room
list alone is not a universal active-minigame detector. Camera-controller/ownership/input guards are independent
checks. Exhaustive coverage of all timed challenges remains `UNKNOWN`.

## Orbit processing (`PROVEN`, static MIPS; synthetic native-controller regression)

`func_80110588(node)` updates camera subsystems, then calls the current table's update callback. String,
pan and pivot convert the camera-to-target vector to spherical angles, filter pitch through `func_80115750`
and yaw through `func_80112C3C`, reconstruct the candidate position, and pass it to their native collision/
position-resolution path. These filters precede collision and final rotation calculation.

Resident update thunks: string `0x80089160`, pan `0x800891B8`, pivot `0x80089488`. They dispatch the
relocatable camera overlays; hooks in the resident scalar filters do not depend on those overlays' load addresses.

- `func_80112C3C(node, floatBits)` applies yaw restrictions selected by the node's `+0x60` substate.
- `func_80115750(node, floatBits)` applies pitch/terrain restrictions from `+0x3C`.
- `func_801111D0` computes a relative yaw target and schedules a native transition through `func_80112D90`.
  It is the original stepped-control path, not the host analog integrator.

The analog integration adds proportional angular deltas to the filters' input only inside the matching node's
update scope. Original filter functions still execute. Synthetic regression executes the real string controller,
checks opposite independent J1/J2 motion and native pitch restriction before a synthetic clear-world collision sink.
The fixture substitutes vector math/target services and collision geometry; it does not prove in-game wall handling,
camera feel, camera-controller coverage or all transformation transitions. The user confirmed the feature working
on Windows (2026-11-01), limited to the cases tested (`USER_OBSERVED`). Linux: `NOT_RUN`.

Host policy and validation boundaries: [input contract](../../specs/Tooie/input-and-controllers.md#331-analog-camera).
