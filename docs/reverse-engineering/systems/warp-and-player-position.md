---
title: "Warp and player position"
description: "Current map and entrance, the game's go-to-map entry point, player 1's PlayerState, position, facing, transformation, flight and Breegull Blaster states, as used by the Dev Mode warp and cheats."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-10-30
tags: [reverse-engineering, map, warp, player]
---

# Warp and player position

What the Dev Mode warp ([window UI spec](../../specs/Tooie/window-ui-and-config.md), `src/game/tooie_dev_warp_hook.cpp`)
reads and calls. Runtime checks: in the attract demos the current map read as 335, then 314, matching the demo
sequence; a teleport from map 335 to a position saved in map 314 loaded map 314 through entrance 8 and placed the
player at the saved coordinates (2026-10-29, Windows, D3D12). With a save file loaded the mode read 3 in Jinjo Village
(map 322), and saving, teleporting within a map and to another map worked (2026-10-30, Windows, user playtest).

## Map state

| Address / function | Meaning | Status |
| --- | --- | --- |
| `u16 0x80132DC2`, read by `func_800EA05C()` | current map id (`MAP_*` of `decomp/include/maps.h`, e.g. `0xC7` Glitter Gulch Mine) | `PROVEN` (actors compare it with map ids; runtime values match the demo maps) |
| `s16 0x80132DC8`, read by `func_800EA090()` | entrance (exit id) the current map was entered through | `STRONG_INFERENCE` (warp pads compare it with their table's entrance while `FLAG3_9FE_WARPING_VIA_WARPPAD` is set; passing it to the go-to entry point loaded the map) |
| `D_80127630 + 0x2` (s16) | game mode: 3 while a save file is played (and on the title intro, with no player), 5 in attract demos; `func_800A819C` returns true for exactly 3 or 5 | `PROVEN` (decompiled getter; runtime: 3 in a loaded save, 5 in demos). **CORRECTED**: an earlier version of this page called 5 "gameplay"; it is the attract demo mode. |
| `D_80127630 + 0x10` (s16, `mapValue`) | map of a pending transition, **not** the current map: it read 0 throughout gameplay | `PROVEN` (runtime) |
| `u8 D_80127630 + 0x12` (`0x80127642`) | pending map change: `func_800A7B18(1)` sets it (called by `func_800A7990`, which the go-to entry point uses); the main loop `func_800A7D84` clears it when it starts the change (once `func_800C9510()` returns 0) and re-enters mode 3 | `STRONG_INFERENCE` (decompiled code and generated `sb 0x7642`; the warp waits for it to clear before it trusts load signs, and cross-map teleports then placed the player) |

## Going to a map

`_gcgoto_entrypoint_1(map, entrance)` (syscall stub `0x80087A58`, overlay `gcgoto`) is what warp pads call. It runs
`func_8080011C_gcgoto(map, entrance, 0, 1)`, which does nothing while `D_80127194 + 0x42C` is non-zero, else maps
the pair through `gcgoto_entrypoint_0`, schedules the transition with `func_800A7990(map, entrance, 1)` and calls
`func_800EA334(0)` and `gcgoto_entrypoint_14(1)`. The old map keeps running while it fades out; the map change starts
when the main loop clears the pending flag at `0x80127642`, and the new map loads over the next frames, with the usual
fade-in and, for most entrances, a short walk from the entrance.
`PROVEN` (decompiled `src/overlays/gc/goto.c`; runtime teleport).

## Player 1

| Address / function | Meaning | Status |
| --- | --- | --- |
| `PlayerState* D_80135490[]` | player states; index 0 is player 1 | `STRONG_INFERENCE` (core2 wrappers index it by player; index 0 was the controlled player in every run) |
| `yaw_update(PlayerState*)`, `0x8009C88C` | runs once per frame for each player while it exists | `STRONG_INFERENCE` (the hook ran every gameplay frame and not on the title screen) |
| `PlayerState + 0xE4` | pointer to a struct whose `+0x0` is the position (`f32[3]`); `+0xC` and `+0x24` are further vectors cleared with it by `func_8009C038` (roles `UNKNOWN`) | `PROVEN` (`func_8009C128(self, out)` copies it out, `func_8009C0F8(self, in)` in; runtime teleport) |
| `PlayerState + 0xF8` | `BaYaw*`: facing in degrees at `+0x0`, ideal facing at `+0x4` | `PROVEN` (`yaw_set` `0x8009C944`, `yaw_setIdeal` `0x8009C914`, `yaw_get` `0x8009C984`; the setters take the angle's bits in `a1`) |

Writing the position through `func_8009C0F8` moves the player; velocity and the camera are not reset, so the camera
catches up over a few frames. Whether objects that track the player (enemies, triggers) need more state is `UNKNOWN`.

## Player state machine: flying

`bs_setState(PlayerState*, state)` (`0x8009E5C8`) requests a transition and `bs_getCurrentState(PlayerState*)`
(`0x8009E6F8`) returns the current state; overlay code calls `bs_setState` through the function lookup, so a hook
registered for its address sees those calls. A temporary log of player 1's requests (2026-10-30, Windows, user taking
off from the flight pad in Mayahem Temple, map 184) read:

| Time | Transition | Meaning |
| --- | --- | --- |
| 71.4 s | `0x1` -> `0x23` | standing on the pad, A pressed: launch |
| 73.1 s | `0x23` -> `0x24` | launch over (about 1.7 s): flying |
| 85.4 s | `0x24` -> `0x1` | landed |

The launch handlers are the first entry of `bskazfly` (`func_80800248_bskazfly`: take-off animation, gravity -1200;
`func_80800320_bskazfly`: vertical velocity 1600 at 39% of the animation, then the flying state once the player falls).
`0x1` is idle; `0x2` and `0x4` were the states while walking in the same log.

| Claim | Status |
| --- | --- |
| `0x23` is the flight launch and `0x24` flying, for Banjo and Kazooie together | `PROVEN` (runtime trace above) |
| Requesting `0x23` from idle or walking (`0x1`, `0x2`, `0x4`) away from any pad takes off and flies normally | `PROVEN` (Dev Mode fly cheat, user playtest on Windows, 2026-10-30) |
| `0x2` and `0x4` are the ground movement states | `STRONG_INFERENCE` (seen only while walking) |
| Whether the pad spends a red feather before the launch state, and whether `0x23` is safe from other states or forms | `UNKNOWN` |

## Transformation and the Breegull Blaster

| Address / function | Meaning | Status |
| --- | --- | --- |
| `PlayerState + 0xA0` -> `+0x29` (u8) | transformation (`TransformationId`, `decomp/include/transformations.h`): `0x1` Banjo and Kazooie, `0x9` (`TRANSFORM_9_FIRSTPERSON`) the Breegull Blaster | `PROVEN` (getter `func_800A3274`; runtime: `0x1` -> `0x9` -> `0x1` around the Blaster) |
| `func_800A3410(PlayerState*, transformation)` | sets the transformation (extra work when Banjo alone, `0xA`, is involved) | `PROVEN` (disassembly; called by the Blaster states, split-up, Humba's transformations) |
| `func_800991B0(PlayerState*)` | per-player update, run every gameplay frame for each player; it calls the yaw dispatcher `func_8009D088` only under a condition, and that dispatcher calls `yaw_update` only for some yaw types | `PROVEN` (disassembly; runtime: the hook ran in the Blaster, `yaw_update` did not) |
| `func_80092778(PlayerState*)` | loads the model of the current transformation (`func_80084F38` -> `func_80092EC8` model, `func_8009312C`) | `PROVEN` (disassembly; called after transformations by Humba, split-up and player setup) |
| `func_8009359C(PlayerState*, model)` | sets the second model (`PlayerState+0x50` -> `+0x90` block); 0 removes it | `STRONG_INFERENCE` (disassembly; with `func_80092778` it restored Banjo and Kazooie after the Blaster) |

A temporary log (2026-10-30, Windows, user entering the Blaster in Targitzan's Temple lobby, map 377, and walking out to
Mayahem Temple, map 184) read: idle `0x1` -> `0xC8` (the transformation becomes `0x9`), then `0xCA` and `0xD1`
alternating; after the map change the new player started in `0xCC`, went `0xCE`, `0xCC`, then `0xC9` (the transformation
becomes `0x1`) and idle `0x1`. The handlers are in overlay `bsfirstp`: `func_80804B10_bsfirstp` (enter: transformation
`0x9`, Blaster setup `func_80807DD4_bsfirstp`, which turns on a second model and pose flags) and
`func_80804FD4_bsfirstp` (exit: once the put-away animation ends, transformation `0x1` and state `0x1`).

| Claim | Status |
| --- | --- |
| `0xC8` enters and `0xC9` leaves the Blaster; `0xCA`, `0xCC`, `0xCE`, `0xD1` are states inside it | `PROVEN` (runtime trace above) |
| Requesting `0xC8` from idle or walking away from any Blaster area turns the Blaster on | `PROVEN` (Dev Mode Blaster cheat, user playtest on Windows, 2026-10-30) |
| Requesting `0xC9` in place restores the transformation but leaves the Blaster's models (a garbled Banjo); removing the second model and calling `func_80092778` fixes it | `PROVEN` (same playtest, before and after the fix) |
| The game leaves the Blaster only through a map change | `STRONG_INFERENCE` (every exit seen; the exit handlers do not undo the model setup) |
| Which of `0xCA`, `0xCC`, `0xCE`, `0xD1` is idle, walking or shooting, and whether the pose flags set by `func_80807DD4_bsfirstp` matter after the reload | `UNKNOWN` |
