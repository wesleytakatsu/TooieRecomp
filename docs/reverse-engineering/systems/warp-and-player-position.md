---
title: "Warp and player position"
description: "Current map and entrance, the game's go-to-map entry point, player 1's PlayerState, position, facing, transformation, flight and Breegull Blaster states, as used by the Dev Mode warp and cheats."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-10-31
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

For the Dev Mode per-player selector, shared transition banks and independent Dragon Kazooie variants, see
[player transformations](player-transformations.md). The user approved the revised Windows selector; exhaustive
gameplay and map-handoff validation remain pending.

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

## Dev Mode cheat ownership

`CORRECTED/SUPERSEDED`: the `a2e43c7` implementation was J1-only. Static inspection proved three host restrictions:
input filtered `controller == 0`, chord/requests had no owner, and the execution block returned for `self != slot 0`.
Its single Blaster cleanup bool was likewise J1-only. These restrictions explained the user's J2 report; they did not
establish a native engine limitation.

Current integration:

- `on_player_input(player, ...)` keeps independent edge histories and atomic request flags for J1/J2. Unlock and
  enable/direction settings remain shared. `take_request(player, cheat)` consumes only that owner's action and rechecks
  unlock/enable. Physical R3 and the mapped Debug Button work for both; physical D-pad reads do not change N64 bindings.
- The existing per-player hook calls `tooie_dev_cheats::update_player` inside the owner's transformation scope.
  This executor lives in `tooie_dev_cheats_hook.cpp` so the guest routing and cleanup can be exercised directly by the
  host test. It resolves `self` against slots 0/1 in `D_80135490` and uses that actual pointer for state/model calls.
- Blaster cleanup is per slot: wait for that owner's native exit to restore form 1, remove its secondary model, then
  reload its primary model. Other forms cancel stale cleanup. Finishing an exit is allowed after disabling new cheats.
- Player creation and session reset call `reset_gameplay`, clearing pending actions and cleanup. Chord history remains
  input-owned until button release, avoiding a held chord being replayed after reset. The selector retains input priority.
- Gameplay/Warp/map-transition gates and original allowed form/state rules still apply. Global actor focus is unchanged.
  Logs identify `player=1` or `player=2`.

`PROVEN` in the host harness: actual executor routing targets J2 without changing J1, supports simultaneous Fly/Blaster
requests and independent exit cleanup, honors disabled/locked gates, rejects invalid owners and disallowed forms, and
discards loading/blocked/reset actions. Native state/model calls are stand-ins. The user confirmed the corrected J2
shortcuts work on Windows (2026-10-31); exhaustive state/map combinations and Linux validation remain `NOT_RUN`.
Earlier J1 gameplay acceptance remains as recorded above.

## Extra player and split screen

Current model from the experimental split-screen investigation. Runtime observations below are reported by the prior
Windows investigation and user playtests in the [continuation handoff](../../future/two-player-split-screen.md);
they were not repeated for this checkpoint.

| Address / function | Meaning | Status |
| --- | --- | --- |
| `PlayerState* D_80135490[8]` | player slots; index 1 holds the extra player | `PROVEN` (prior runtime logs and controlled second player) |
| `func_800F73C4(count)` | creates the map's players; normally one outside game mode `0x1B`; requesting two creates player 2 | `PROVEN` (prior runtime investigation) |
| `func_800F6140` / `func_800A907C(slot, controller, viewport)` | allocates a free player slot / associates its controller and viewport | `PROVEN` (prior investigation and slot logs) |
| `0x801276D0` | slot table: `+0x0` count, `+0x2` viewport count (s16), `+0x4` controller uses, `+0x8` controller per slot, `+0x18` viewport per slot, `+0x38` camera per slot | `PROVEN` (prior investigation and slot logs) |
| `func_800A8D70` / `func_800A8FDC` | map load sets one viewport outside multiplayer maps / adds one viewport | `PROVEN` (prior investigation; two viewports rendered) |
| `func_80016BDC(channels)` | `osContSetCh`; save selection reduces reads to one port, so two-player creation requests two ports | `PROVEN` (prior input logs and user control of player 2) |
| player state `0x74` | extra player waits without input or physics; position must be set while waiting before requesting idle `0x1` | `PROVEN` (prior state logs and failed/successful spawn experiments) |
| `func_800F65D0(index)` | active-player gate: all players in mode `0x1B`, otherwise only focused index `D_801354DC` | `PROVEN` (prior investigation; admitting index 1 enabled item collection and attacks) |
| `func_800F54E4()` | returns focused player index `D_801354DC`, used by actor queries | `PROVEN` (prior investigation); which individual enemies need explicit target selection remains `UNKNOWN` |

Player-to-actor collision path (`PROVEN` by the prior native stack trace): `func_800991B0` -> `func_8008E6F0` ->
gate `func_8008DAA8` / `func_800F65D0` -> `func_800CDBA8` -> `func_8008E530` -> `func_800EB210` ->
`func_80109F18` -> `func_8010114C` -> `func_8010108C` -> actor callback (e.g. `chhoney`) ->
`func_800FFAB0` (marks actor for deletion).

Replacing global focus `D_801354DC` with the nearest player for each actor is `REFUTED` as a fix: the prior experiment
broke player 2's eggs and the camera without making enemies pursue it, and was reverted. Actor loop:
`func_800FF62C`; `func_80100368(actor)` returns ActorData. Enemy target selection and player 2's door triggers remain
`UNKNOWN`; the collision gate alone does not solve them. Lighting inputs are per player, but the CPU lighting output
could alias shared model vertices (`CORRECTED` from the earlier unknown/shared-light-value hypothesis). See
[player lighting](player-lighting.md) for the proven conflict and the integration fix.

### Glowbo collection and deferred actor cleanup

`PROVEN` by native code and an isolated host fixture: `func_80800880_chglowbo(actor, event, payload)` handles
event `0x3E` by recording the normal Glowbo's collectible flag (`func_800D0BD4(index, 3)`), adding one to inventory
(`func_800D1844(0x48)`) and setting actor state `1` through `func_80101FDC`. The callback does not immediately mark
the actor for deletion, and it does not reject another `0x3E` when the actor is already in state `1`.
`func_808002F4_chglowbo` performs the later state-`1` cleanup, including its optional effect and `func_800FFAB0`.
See `decomp/src/overlays/ch/glowbo.c:213-255` and `:109-209`.

`PROVEN`: the actor loop `func_800FF62C` caches the focused player's position with `func_8010D1E8` and tests
`func_8010038C(actor, position)` before invoking the actor's update callback. The position producer uses
`func_8008FE68` -> `func_800F54E4` -> `func_800F5A00`; it does not select the nearest active split-screen player.
For distance-gated actors, `func_8010038C` uses the radius from the high halfword at `actor+0x7C` and
`func_800EFFB4`'s squared-distance test. The canonical normal Glowbo descriptor has radius `5000` (`0x1388` at
`chglowbo+0x14D8`). Other actor activity flags can bypass or suppress this range test; distance alone is not the
complete scheduling contract. The Glowbo update also has an independent sector gate in Jolly Roger's Lagoon maps.

The experimental `tooie_dev_on_player_active` hook admits J2 to player/actor collisions. It does not extend actor
update scheduling to J2. `PROVEN` in the host fixture: repeated native collection callbacks can credit the same
actor while the real actor loop's range gate suppresses its cleanup; moving the focused-player position into range
allows the native update to request deletion. The fixture executes the native loop, gate/distance arithmetic,
Glowbo callback/update, state setter and inventory arithmetic. Actor enumeration, player-position input, assets,
save-flag/UI/effect services and final actor-list removal are synthetic. It does not execute collision detection,
real controllers, a map or the renderer.

`STRONG_INFERENCE` for the reported Windows bug (2026-10-31): J2 was away from J1, repeatedly credited one Glowbo,
and J1 could collect it correctly. This matches deferred cleanup blocked by focused-player scheduling.
The actual session's actor flags, map, separation and selected gate were not captured. The user confirmed the fix
resolves the reported Windows collection bug (2026-10-31); the precise gate in that session remains inferred.
Immediate deletion by other collectibles is not evidence that their deferred lifecycle supports J2.
The inventory remains shared; splitting it or replacing global focus is not required by this finding.

`PROVEN` from the canonical ROM descriptor and native code: the normal class descriptor returned through
`func_80100368` and `chglowbo_entrypoint_0` has prefix `0x0039021B`; the same class handles Mega Glowbo via bit
`0x20000000` at `actor+0x74`. The regression executes the native descriptor getter with synthetic relocation/data.
Native `func_8010108C` admits events only with bit `0x10000000` at `actor+0x74`; its original early
return is preserved by the integration. `func_800FFAB0` sets the deferred-delete bit (`actor+0x65 & 2`) and queues
native removal; the integration does not directly reclaim actor storage.

The experimental host hook `tooie_dev_on_actor_event` narrows that resident dispatch to collection events for this
class and live two-player story sessions. It calls the original dispatcher, then completes the original state-`1`
effect release and deferred-delete request synchronously. Repeated collected/deletion-marked events do not rerun
the credit callback. This scheduling change avoids both the focused-player range gate and Glowbo's sector gate
without changing either gate, global focus or general actor scheduling. The reported Windows case is user-validated;
Mega Glowbo/JRL-specific acceptance and Linux remain `NOT_RUN`. Neither that acceptance nor the native regression
establishes complete map/effect/rendering parity.
