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
| `func_800F54E4()` | returns focused player index `D_801354DC`, used by actor queries | `PROVEN`; the enemy-query paths below demonstrate focused-player priority and shared caches, not a universal per-enemy target field |

Player-to-actor collision path (`PROVEN` by the prior native stack trace): `func_800991B0` -> `func_8008E6F0` ->
gate `func_8008DAA8` / `func_800F65D0` -> `func_800CDBA8` -> `func_8008E530` -> `func_800EB210` ->
`func_80109F18` -> `func_8010114C` -> `func_8010108C` -> actor callback (e.g. `chhoney`) ->
`func_800FFAB0` (marks actor for deletion).

Replacing global focus `D_801354DC` with the nearest player for each actor is `REFUTED` as a fix: the prior experiment
broke player 2's eggs and the camera without making enemies pursue it, and was reverted. Actor loop:
`func_800FF62C`; `func_80100368(actor)` returns ActorData. Enemy query mechanisms are described below; player 2's door
triggers remain `UNKNOWN`. The collision gate alone does not solve either limitation. Lighting inputs are per player, but the CPU lighting output
could alias shared model vertices (`CORRECTED` from the earlier unknown/shared-light-value hypothesis). See
[player lighting](player-lighting.md) for the proven conflict and the integration fix.

### Enemy perception and focused-player priority

`PROVEN` from native code and an isolated Windows host fixture: adding player 2 and admitting its player-to-actor
collisions does not give story enemies a second perception/target context. There are at least two query paths:

- **Cached position:** `func_8010D1E8` populates `0x80136F50` with the focused player's position through
  `func_8008FE68` -> `func_800F54E4` -> `func_800F5A00`. It also stores query state at offsets `+0xC`, `+0xE`,
  `+0x10`, `+0x14`. `func_8010D254(out)` copies the position; `func_8010C9FC(origin, radius, top, bottom)` tests
  that single cached position, gated by the cached halfword at `+0xE`, using native `func_800F00A4`.
  `func_8010C668` likewise uses the cached position for its vertical/region test. Enemy examples:
  `func_8080082C_chseaweedbaddy` and `func_808007F0_chflowerbaddie` call these tests before entering their active
  behavior. Their update paths copy this same cached position into actor-relative working vectors.
- **Explicit index:** `func_8010D290(excluded_forms)` first reads `func_800F54E4`. It returns that index immediately
  if `func_800F6438(index)` succeeds and `func_800F64A4(index, excluded_forms)` is zero. Only otherwise does it
  scan the other slots, in index order, for an available non-excluded form. Focus `-1` returns `-1` immediately.
  It takes no actor/origin/position argument and performs no nearest-player comparison. This mask is an
  **exclusion** mask in this caller, despite the generic transformation enum's name.
  `func_80800610_chnewshovel` uses mask `0x10000`, then checks the selected player's additional state and region
  through `func_800F6478` and `func_8010C550`. Its update `func_8080004C_chnewshovel` also uses `func_8010D290`
  followed by `func_800F5A00` to obtain the selected player's position.

`PROVEN`: `suchase_entrypoint_0` / syscall `0x80089EF8` has another shared cache, native overlay offset `+0x1D0`:
one tick value and one actor marker. On a new `func_8001211C()` value it copies the focused-position cache and calls
`func_808000A8_suchase`, which chooses the closest eligible actor marker from the spatial enumeration. Other calls
with the same tick reuse that marker, regardless of a subsequently changed player position. The Seaweed/Flower
predicates compare this marker with their own. This is selection of a pursuing **actor**, not selection of a player.
The complete rules deciding which actors qualify remain outside this investigation.

`PROVEN`: `suchase_entrypoint_1(actor)` / syscall `0x80089F00` calls entrypoint `0` directly, then returns true
when there is no selected marker or when the marker equals `actor->marker`. Intercepting only syscall entrypoint
`0` does not isolate the call made inside entrypoint `1`. Both outer query entrypoints must own the cache scope.

The fixture executes the native selector, slot lookup, transformation-mask predicate, focus getter, position-cache
producer/copy, range arithmetic, and both `suchase` routines with synthetic overlay data. Player-validity/form/position
leaf services, the four cached-state queries, spatial enumeration, marker-to-actor mapping and tick input are stand-ins.
It proves: J1 is returned without querying an eligible J2; a J1 position at 6000 excludes a J2 position at 100 from a
1000-unit perception test; focus changes alone leave the cached position stale; refreshing it for J2 makes the test
succeed; excluded/unavailable J1 can fall back to J2; no available player returns `-1`; and a second perception context
within one tick reuses the first `suchase` result until the tick changes. This is bounded native-function proof, not
execution of an enemy's full update/attack, a real map, damage delivery or gameplay.

`STRONG_INFERENCE` for the initial report: retaining J1 as focus explains J2 being ignored by these paths even though
J2 can hit enemies and collect items. The native fixture alone does not identify the session's enemies; the later
read-only sample below identifies two affected classes. Neither finding proves that every enemy or boss shares these
paths, or that J2 cannot receive incidental contact damage.
The actor loop's focused-player update gate is an additional potential blocker when J2 is far from J1 (see Glowbo
below); it is distinct from choosing an attack target.

`CORRECTED/SUPERSEDED`: "enemy AI always calls `func_800F54E4` directly" is too broad. Some paths use a previously
filled cache; some explicitly prefer focus and can fall back to another slot; some arbitrate a single pursuing actor
per tick. Replacing global focus or only changing `func_8010D290` is not a complete two-player AI contract. An
implementation must first scope a supported enemy family, keep perception/target/attack ownership coherent, address
its shared chase cache and activation scheduling, and preserve camera/projectile focus.

#### Experimental per-enemy integration

`PROVEN` in the host regression: the integration wraps update callbacks
for these canonical descriptors. Entries were read from the verified decompressed ROM; the callback is at descriptor
offset `+0xC`, and the radius shown is the descriptor's native activation radius.

| Overlay | ROM section | Descriptor offset / prefix | Update offset | Radius |
| --- | --- | --- | --- | --- |
| `ch2dbaddy` | `0x020058F0` | `+0xA9C` / `0x02F70464` | `+0xA0` | 3000 |
| `chcrystalbaddie` | `0x0200F1F0` | `+0x664` / `0x03040426` | `+0x54` | 2000 |
| `chflowerbaddie` | `0x0200F9B0` | `+0xC14` / `0x01A9046B` | `+0x5C` | 2000 |
| `chnewshovel` | `0x02025560` | `+0xFB4` / `0x000A011A` | `+0x4C` | 2000 |
| `chseaweedbaddy` | `0x01FBCDC0` | `+0xEB4` / `0x02EF035C` | `+0x18` | 3000 |
| `chhillbilly` | `0x01F9D080` | `+0xC60` / `0x01A10150` | `+0x64` | 3000 |
| `chdiggerground` | `0x01F9DE20` | `+0x408` / `0x01A20131` | `+0xC` | 3000 |
| `chdiggerfly` | `0x01F9E8F0` | `+0xF8` / `0x01A40133` | syscall `0x80089ED8` -> `sucoasterfly+0xE8` | 2500 |

`PROVEN` by read-only sampling of the user's existing Windows process: Jinjo Village (map `322`, mode `3`) contains
`chdiggerground` and `chdiggerfly` actor records in the native `0x9C`-stride vector at `0x80136EE0`. Both player slots
were live, with focus `0`; the nearby flying actor was within J2's vicinity while J1 was far away. Neither class was
included in the first five-family build. In that build, the user reported contact damage worked but J2 was not pursued.
This proves the original coverage gap for the observed classes, not failure of the whole scoped-target design or of the other
five families. `chhillbilly` is included from the native descriptor/control path; its association with the reported
mine cowboy remains `STRONG_INFERENCE`, without a live mine sample.

The seven overlay-owned update hooks are bound at physical load and canonical remapping, not at a shared transient
VRAM without overlay identity. Fly's update callback is a resident syscall, so its scope surrounds the shared native
`sucoasterfly_entrypoint_3` only after the original dispatcher activates that overlay, and only for its class prefix.
It validates the actor's descriptor prefix. Only live experimental two-player story mode `3` is eligible; native
multiplayer and attract mode keep native execution. Native availability predicates (`func_800F6438`, `func_800F6478`)
are required; Shovel also excludes mask `0x10000`. Per-actor target selection and retention are enhancement policy,
not recovered original two-player behavior. See the [UI contract](../../specs/Tooie/window-ui-and-config.md).

`D_801354DC` is never rewritten. A thread-local index overrides `func_800F54E4` only within the audited update;
`func_8010D1E8` fills all six perception words for that context and the previous words are restored after it. Nested
scopes restore their parent. Native `suchase` dispatch for both query entrypoints uses its original overlay lifecycle, with separate tick/marker
words per player and restoration of its shared storage. Supported activation gates retain native flag/initialization
logic and use the nearer original/J2 point. Map player creation resets the integration's target/chase state.

The regression runs the real native Shovel update and selection path through acquisition and pursuit into its attack
state's target-position assignment. The regression also executes `func_8080000C_chdiggerground` and native
`sucoaster_entrypoint_2` through `func_80802760_sucoaster` and the player's position getter, and executes the shared
Fly update with native `func_80801AB0_sucoasterfly`, range/region checks and chase predicate. Pose/state-application,
parts of the behavior/movement pipeline and external services are stand-ins. The other five update bodies are
probes; their bindings, query scope and nested restoration are tested, not their full AI. Original
query/range/chase arithmetic runs natively with synthetic player/world/overlay data. The user accepted the revised fix
for the reported Windows gameplay case (2026-10-31). Exhaustive attack collision/damage, camera/projectile parity and
family/map coverage remain `UNKNOWN`. This implementation does not establish a safe universal enemy/boss hook and
does not reopen the refuted global-focus experiment.

`CORRECTED`: the first fixture exercised chase entrypoint `0` only. Adding an entrypoint-`1` regression failed on
shared-cache restoration before its scope was added, then passed. The user's first playtest did not approve the
five-family build; the revised coverage/cache fix was subsequently accepted for the reported case. This does not
identify the mine cowboy's class or prove universal enemy support.

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
