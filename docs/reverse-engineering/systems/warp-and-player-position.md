---
title: "Warp and player position"
description: "Current map and entrance, the game's go-to-map entry point, player 1's PlayerState, position and facing, as used by the Dev Mode warp."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-10-29
tags: [reverse-engineering, map, warp, player]
---

# Warp and player position

What the Dev Mode warp ([window UI spec](../../specs/Tooie/window-ui-and-config.md), `src/game/tooie_dev_warp_hook.cpp`)
reads and calls. Runtime checks: in the attract demos the current map read as 335, then 314, matching the demo
sequence; a teleport from map 335 to a position saved in map 314 loaded map 314 through entrance 8 and placed the
player at the saved coordinates (2026-10-29, Windows, D3D12).

## Map state

| Address / function | Meaning | Status |
| --- | --- | --- |
| `u16 0x80132DC2`, read by `func_800EA05C()` | current map id (`MAP_*` of `decomp/include/maps.h`, e.g. `0xC7` Glitter Gulch Mine) | `PROVEN` (actors compare it with map ids; runtime values match the demo maps) |
| `s16 0x80132DC8`, read by `func_800EA090()` | entrance (exit id) the current map was entered through | `STRONG_INFERENCE` (warp pads compare it with their table's entrance while `FLAG3_9FE_WARPING_VIA_WARPPAD` is set; passing it to the go-to entry point loaded the map) |
| `D_80127630 + 0x2` (s16) | game mode; 5 in gameplay, 3 on the title intro | `PROVEN` (transition log) |
| `D_80127630 + 0x10` (s16) | **not** the current map: it read 0 throughout demo gameplay; the transition log reads it while a transition is processed | `PROVEN` (runtime) |

## Going to a map

`_gcgoto_entrypoint_1(map, entrance)` (syscall stub `0x80087A58`, overlay `gcgoto`) is what warp pads call. It runs
`func_8080011C_gcgoto(map, entrance, 0, 1)`, which does nothing while `D_80127194 + 0x42C` is non-zero, else maps
the pair through `gcgoto_entrypoint_0`, schedules the transition with `func_800A7990(map, entrance, 1)` and calls
`func_800EA334(0)` and `gcgoto_entrypoint_14(1)`. The new map loads over the next frames, with the usual fade.
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
