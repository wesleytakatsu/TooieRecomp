---
title: "BK — Mods & Extension APIs"
status: current
area: "specs/BK"
updated: 2026-08-08
tags:
  - banjo-recomp
  - mods
  - api
---

# BK — Mods & Extension APIs

How `BanjoRecomp` lets third-party code attach to the recompiled game:
per-object extension data, container APIs, and modI/O. This is the layer
between the native [Patches](patches.md) runtime and full community mods.

## Two `recomputil` namespaces

`src/game/` holds two API families, both exported back into the recompiled
game so patches and mods can call them:

| File | Purpose |
| ---- | ------- |
| `recomp_extension_api.cpp` | per-object extension memory attached to recompiled actors |
| `recomp_data_api.cpp` | generic containers (hashmaps, hashmaps, slotmaps, hashsets) usable by mods |

Both register themselves at startup from `src/main/main.cpp` via
`recomputil::register_data_api_exports()`. They use the shared
`ExtensionInfo` / `ExtensionContext` machinery and `dod::slot_map32`,
guarded by a single `extension_mutex`.

## Object extensions (Props, ActorMarkers)

`main.cpp:741`:

```cpp
recomputil::init_extended_object_data(2); // types: Props and ActorMarkers
```

An `ExtensionContext` exists per object type. Each has:

- `data_sizes[]` — total size of subtype-specific extension data per subtype
- `generic_data_size` — size of generic (subtype-agnostic) extension data
- `extensions[]` — registered `{subtype_index, data_offset}` pairs
- `extension_data` — slotmap of `{object_spawn_index, data_addr}`
- debug `alloc_count` / `free_count`

External API (all `extern "C"`):

- `recomp_register_object_extension(type, subtype, size)` — declare a
  subtype-specific extension block for an object type.
- `recomp_register_object_extension_generic(type, size)` — generic data
  shared across subtypes (offset 0, put before the subtype-specific blobs).
- `recomp_clear_all_object_data(type)` — reset on overlay/array-thread unload.
- `recomp_create_object_data(type)` — allocate a slot, returns a slot key.
- `recomp_destroy_object_data(type, handle)` / `recomp_get_object_data(...)`.
- `recomp_get_object_spawn_index(type, handle)` — the game-stable spawn
  index; returns `0xFFFFFFFF` for an invalid handle.

The games-side writer of the extension data assigns the exact RAM offset by
summing the generic size plus the subtype offset. `round_up_16` keeps each
extension 16-byte aligned.

## Container APIs (`recomp_data_api`)

Exports follow `REGISTER_FUNC(name)` → `register_base_export(name, name)`,
so a patch can simply call, e.g., `recomputil_create_u32_value_hashmap()`.

| Category | Functions |
| -------- | --------- |
| u32 value map | `create/destroy/contains/insert/get/erase/size` |
| u32 memory map | `create/destroy/contains/create/get/erase/size` |
| hash set | `create/destroy/contains/insert/erase/size` |
| u32 slotmap | `create/destroy/contains/create/get/set/erase/size` |
| memory slotmap | `create/destroy/contains/create/get/erase/size` |

Implemented with thread-safe `LockedMap` / `LockedSet` / `LockedSlotmap`
wrappers plus `addrans_map` style helpers. Handles are `uint32_t` slot keys;
an invalid key aborts with a fatal message box.

On error these print a "Fatal error in mod - <func> : <msg>" box and call
`ultramodern::error_handling::quick_exit`.

## Mod container / content types

`librecomp/mods` (N64ModernRuntime) manages installed mods. `main.cpp`
registers supported containers:

- a **texture pack** content type: `content_filename = "rt64.json"`,
  `allow_runtime_toggle = true`, plus `on_enabled` / `on_disabled` /
  `on_reordered` callbacks.
- container format **`.rtz`** (`register_mod_container_type("rtz",
  {texture_pack_content_type_id}, false)`).
- a deprecated-mod entry for the old FOV slider mod
  (`register_deprecated_mod("bk_recomp_mod_fov_slider",
  BrokenVersion, v1.1.0)`).

## How a mod reaches the game

- Mods are zip/`.rtz` packages; user drags them onto the window or installs
  via the mod menu ([Features](features.md#mod-support)).
- The game's archive (`librecomp`/RT64) loads content by content type; the
  patched game code then reads/writes object extension data through the
  `recomp_*` exports defined above.
- Official features (note/jinjo saving, camera/axis inversion, widescreen
  transform tagging) are themselves implemented as patches inside `patches/`
  — see [Patches](patches.md).

## Related

- [Patches](patches.md) — the native patch ELF that these APIs serve
- [Features](features.md) — user-facing modding story
- [Runtime](runtime.md) — `register_base_exports` / `register_base_events`