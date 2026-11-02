---
title: "Player transformations and map handoff"
description: "Per-player forms, shared native transition banks, Dragon Kazooie and the Dev Mode selector."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-11-02
tags: [reverse-engineering, player, transformation, multiplayer]
---

# Player transformations and map handoff

Related: [player position](warp-and-player-position.md), [player lighting](player-lighting.md),
[UI contract](../../specs/Tooie/window-ui-and-config.md), [manual coverage](../../specs/Tooie/testing-and-oracle.md#5-experimental-split-screen-manual-coverage).

## Native mechanisms

`PROVEN` by static inspection of the generated MIPS instructions (not by a new runtime reproduction):

- `PlayerState+0xA0 -> +0x29` is the form; `func_800A3410(self, form)` changes it.
- `func_80092778(self)` reloads primary/LOD model assets for that form. Removing the secondary model uses
  `func_8009359C(self, 0)`. A form change also needs the new behavior state; replacing the model alone is insufficient.
- `_badata_entrypoint_34`, syscall address `0x80084750`, returns the default state from the current form/context.
  `badata_entrypoint_35` instead uses the initial/map-handoff context. These are native state selectors, not constant idle 1.
- `func_80098E64(self)` serializes an active player's map-handoff information before destroying its subsystems.
  `func_8008DAA8(self)` calls active-player gate `func_800F65D0(self->index)`. Admitting J2 to that gate also admits
  it to serialization. `func_800F482C(0)` destroys occupied slots in ascending order, so J2 can overwrite J1's handoff.
- `func_8009E9D8(form)` writes the shared transition form byte at `0x8012704C`; `func_8009E958()` reads it.
  `func_800A3820` uses this byte during player allocation; `_basetup_entrypoint_2` (`0x80084FB0`) uses it again
  while choosing the initial model/state after allocation. Fixing only the first read is insufficient.
- Native transition values occupy banks starting at `0x80127040`, with mode/timer values through `0x80127080`.
  `func_8009E880` resets current transition values; getters/setters through `func_8009EB24` access the banks.
  The integration captures `0x44` bytes (`0x80127040..0x80127083`), including return-form, motion, mode and timer
  values, rather than sharing J2's movement/Blaster/Clockwork handoff with J1.
- Native teardown can intentionally change a form before serialization: Golden Goliath (`0xE`) becomes Mumbo (`0xD`)
  when the native `func_80095738(self, 0x40)` condition holds. The serialized value takes precedence over an earlier snapshot.
- Dragon Kazooie is not a separate `TransformationId`. `func_8009EA2C()` reads shared flag
  `FLAG_3EC_ABILITY_DRAGON_KAZOOIE`; `func_8009EAAC(value)` writes it. Forms 1/B can have a Dragon variant.
- `func_800A8380(viewport)` selects a native screen-layout row at `0x8011A420 + row*8`. The four s16 fields are
  x/y/width/height in half-screen units, consumed by `func_800A8424`. Player-to-viewport mapping is at
  `0x801276E8 + player*4`.

`STRONG_INFERENCE`: the shared handoff explains the reported loss of J1's Humba transformation after leaving her tent
with J2 still Banjo/Kazooie. The reported reproduction has not been traced in a new runtime capture.

## Integration and validation boundary

The Dev Mode integration (`tooie_dev_forms_hook.cpp`) captures each player's native serialized transition after teardown
and restores J1's global handoff after J2. Allocation and initial setup temporarily select the corresponding player's
handoff, then restore the previous globals. No guest pointers or model allocations are retained in these snapshots.
Session initialization at `func_8009E8D4` clears the host snapshots. Ordinary single-player play before Dev Mode unlock,
attract mode and original multiplayer requests do not select these snapshots.

Selector availability is checked again in live gameplay (mode 3, outside native multiplayer and excluded maps), not
only during player creation. This allows unlocking Dev Mode in the current map, independent of the transitional mode
at map load. The menu has its own persisted switch and explicit open buttons in the Dev Mode tab. Physical R3 is
accepted in addition to the configurable Debug Button, without changing the player's saved control bindings.
The host shortcut direction is configurable in the Dev Mode tab (default Down); it is independent of native
transformation/transition data. The opening press is consumed before cheat dispatch for either player.

Menu requests run on the game thread for their owner: native waiting state `0x74` ends the old behavior, the form and
models are changed, the native base animation is refreshed, then the native default state starts. Blaster uses its
native `0xC8` entry. J2's delayed release also uses the native default state instead of assuming Banjo idle `0x1`.
The release waits for J1's position to settle rather than requiring a Banjo-specific idle state; a menu selection
that already starts J2 cancels the delayed repositioning.

Golden Goliath menu entry seeds 120 seconds through native `func_8009EB00(f12)`, temporarily replacing the transition
duration at `0x80127068` only while starting the new behavior. `PROVEN` statically: `func_808001FC_bsstatue` reads this
duration via `func_8009EA7C`; `func_808000C0_bsstatue` passes it to `func_800CE8D8(2, duration, integer_duration)`
and activates HUD timer 2 through `func_800CE88C(2, 1)`. Expiry uses `func_800CEAD0(2)`. A direct selection previously
supplied a zero spell-handoff duration, consistent with the user's report of immediate expiry. The original transition
value is restored after entry, and the live native timer counts down normally. There is no repeated refill and no
change to original spell activation/range code. HUD timer 2 is native/global; this change does not isolate simultaneous
Goliath countdowns. The 120-second seeding contract is covered by the host harness. The user approved the revised
Windows build; full timer expiry remains unverified by a specific gameplay test.
Clockwork is excluded from the selector following the user's crash report. The selector's Clockwork timer-seeding
call was removed; original egg/remote mechanics remain native. `PROVEN` statically: `baduo_entrypoint_14` creates an
additional player with `func_800F6224`, then assigns form `0x11` and establishes owner/remote links. This differs from
replacing the owner in place. `STRONG_INFERENCE`: this lifecycle mismatch contributes to the reported crash;
the exact crash site remains `UNKNOWN`.

## Catalog coverage

The declared enum has 16 native form IDs. The current selector exposes 15 of them (excluding Clockwork), plus the
two Dragon variants, for 17 entries; Banjo/Dragon Kazooie is second, and Kazooie/Dragon Kazooie/Mumbo/Snowball are
consecutive in that order. Additional independent playable forms were not
confirmed by the native model/behavior dispatch inspection:

- IDs 3/5 have zero entries in the `bapreload` primary model table at overlay-relative `0x2A0`; the normal path falls
  back to Banjo (or no model in game type `0xD`). They have no dedicated form branch in the two `badata` selectors.
- ID 4 has model `0x61F`, but `basetup_entrypoint_2` routes it straight to `func_800F44DC(self, 0)`, skipping the normal
  playable spawn/setup; it has no dedicated form behavior
  in `badata`. A directly selectable additional playable character is `UNKNOWN`; it is not exposed.
- `func_80800000_bapreload` has per-player model variants for forms 1/6/B/C in native multiplayer. They reuse those
  form IDs and do not establish an additional independent transformation type. No extra menu entries were added.

Dragon selection is scoped to that player's update, draw, model setup and initialization via the getter hook. Menu
choices do not write the shared save flag. Native writes still execute for progression and update the scoped player's
variant; the other player's variant remains independent.

There is no map filter on the selector's character list. Native Humba range/reversion routines are not patched, and
the integration does not continuously force an old form back after a native change. Compatibility with every native
range boundary, special-form timer and camera remains `UNKNOWN` until gameplay validation.

The host harness checks actual integration wrappers with native lifecycle stand-ins: independent requests, all 17
catalog entries, per-player transition bank selection, native reversion precedence, Dragon isolation, viewport
publication and session reset. It does not execute native movement/animations or render the overlay. Linux runtime
validation is `NOT_RUN`. The user confirmed general selection works and approved the revised Windows build;
their screenshot proved the former help-label overlap. Header, rows and footer now have explicit widths, heights and
non-wrapping text. This approval does not establish exhaustive form, map-range or timer-expiry compatibility.

## Dragon appearance after an in-place selection

Reported on Windows after `a2e43c7`: with two story players, selecting Banjo/Dragon Kazooie for J2 leaves its normal
appearance while J1 is Banjo/Kazooie. The report establishes the visual failure, not a shared-state overwrite.

`PROVEN` statically:

- The menu writes `saved[index].dragon` before calling `func_80092778(self)`. The getter hook selects this per-player
  optional value inside update/model/draw scopes. Both combined variants retain native form ID 1.
- `func_80092778` asks `bapreload_entrypoint_0` for the model pair, then calls `func_80092EC8` and `func_8009312C`.
  The ordinary story path chooses its primary ID from the form table; Dragon is tested there only for solo form B.
- At `0x80092EE4`, `func_80092EC8` returns when the requested primary ID equals `PlayerState+0x50 -> +0xC`.
  This skips `func_80093FD4(self, model)` at `0x80092F44`. For primary ID `0x607`, that function selects the
  Dragon-sensitive auxiliary model through `func_80093F30`, then installs it with `func_800941C8`.
- This auxiliary model selection reads the Dragon getter, but the loaded and pending auxiliary IDs are per-player
  fields at `PlayerState+0x50 -> +0x10C/+0x110`. `func_80094070`, called during drawing, can also request a changed
  auxiliary ID. Therefore the early return alone does not prove that the appearance stays wrong indefinitely.
- Before this fix, the host test replaced `func_80092778` with an owner-recording stand-in. Its Dragon isolation
  assertion covered getter state, not the model cache, auxiliary selection or visual appearance.

`STRONG_INFERENCE`: the selector's refresh is incomplete when changing the Dragon variant without changing the
combined form/model ID. A cached auxiliary model or its later refresh path is the leading discriminator. Exact runtime
failure location remains `UNKNOWN`; a global flag overwrite or model-memory alias has not been established.

`PROVEN` by the user's Windows test (2026-10-31): J2 Bee -> Banjo/Dragon Kazooie displays Dragon, whereas the reported
direct selection from normal Banjo/Kazooie does not. Changing the base form makes the requested appearance work.
This strengthens the refresh/cache explanation and shows that J2 can display the variant. It does not by itself
identify which native refresh step fails or prove every simultaneous normal/Dragon combination.

The selector now compares primary IDs before/after native reload using `func_80092B80(self)`. When a combined selection
retains primary `0x607`, it calls native `func_80093FD4(self, 0x607)` inside the selecting player's scope. This executes
the auxiliary selection skipped by the primary setter; native `func_800941C8` remains responsible for installation,
including the existing private-lighting-vertices hook. No primary state is force-cleared. The save flag, Humba range
and original transformation flows remain unchanged. Refresh occurs only on menu confirmation.

`PROVEN` in the host regression: linking `RecompiledFuncs` and executing actual `func_80092778`, `func_80092EC8`,
`func_80093FD4` and `func_80093F30` reproduces the old failure on direct normal -> Dragon selection. The test fails at
the expected auxiliary-ID assertion without the fix, and passes with it. Model pairs/LOD tables are synthetic;
asset allocation/installation and unrelated lifecycle routines remain stand-ins. Direct normal -> Dragon -> normal
works for both player slots in this harness, preserving the other slot and the shared save flag. Both Dragon at once,
returning J1 to normal while J2 remains Dragon, and absence of per-frame reinstallation are also covered.

The user approved the corrected Windows build on 2026-10-31. This closes the reported visual regression and prior
cache-coverage gap, without establishing exhaustive form/map or later-LOD compatibility. Linux validation remains
`NOT_RUN`.
