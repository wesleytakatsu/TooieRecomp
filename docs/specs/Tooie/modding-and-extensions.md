---
title: "TooieRecomp Spec — Modding, Extensions and Asset Customization"
description: "Specification for the recomp extension API, extended object data structures, texture pack container formats (.rtz), and mod callback hooks."
status: current
area: "specs/Tooie"
updated: 2026-10-23
tags: [specs, tooierecomp, modding, extensions, rtz, textures, api]
---

# Modding, Extensions and Asset Customization Specification

## 1. Subsystem Architecture

TooieRecomp incorporates modding and extension APIs inherited from the static recompilation core architecture, providing data structures for attaching arbitrary data to game entities, custom texture pack loading, and mod configuration hooks.

```text
+---------------------------------------------------------------------------------+
|                       Modding & Extension Architecture                          |
|  - Data Container API (src/game/recomp_data_api.cpp: hashmaps, slotmaps)        |
|  - Object Extension API (src/game/recomp_extension_api.cpp)                      |
|  - Mod Content Containers (.rtz, rt64.json via src/main/main.cpp)               |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                         Runtime Integration Hooks                               |
|  - recomp::mods::register_mod_content_type (Runtime texture pack toggles)       |
|  - recomputil::init_extended_object_data (Object subtype custom payloads)       |
|  - dod::slot_map32<ExtensionData> (Handle allocation)                           |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-MOD-01** | Extended Object Data | Modding API allows attaching custom memory blocks to spawned entities. | `INHERITED_UNTESTED` |
| **REQ-MOD-02** | Type Contexts & Slots | `dod::slot_map32` manages per-object extension lifecycle. | `INHERITED_UNTESTED` |
| **REQ-MOD-03** | Texture Pack Support | Loads `.rtz` texture archives containing replacement RT64 textures. | `INHERITED_UNTESTED` |
| **REQ-MOD-04** | Runtime Mod Toggles | Allows enabling, disabling, and reordering texture packs at runtime. | `INHERITED_UNTESTED` |
| **REQ-MOD-05** | Deprecated Mod Rejection| Rejects incompatible legacy mods (`bk_recomp_mod_fov_slider`). | `IMPLEMENTED` |

---

## 3. Interface & Memory Contracts

### 3.1 Object Extension API (`src/game/recomp_extension_api.cpp`)
```cpp
struct ExtensionInfo {
    uint32_t subtype_index;
    uint32_t data_offset;
};

struct ExtensionData {
    uint32_t object_spawn_index;
    PTR(void) data_addr;
};

using extension_data_map_t = dod::slot_map32<ExtensionData>;

struct ExtensionContext {
    std::vector<uint32_t> data_sizes{};
    uint32_t generic_data_size = 0;
    std::vector<ExtensionInfo> extensions{};
    extension_data_map_t extension_data{};
    uint32_t spawn_count = 0;
    size_t alloc_count = 0;
    size_t free_count = 0;
};
```

### 3.2 Mod Lifecycle Hooks (`include/recomp_data.h`)
- `recomputil::init_extended_object_data(size_t num_types)`:
  - Invoked during startup (`src/main/main.cpp`) with `num_types = 2` (inherited labeled contexts for Props and ActorMarkers). Semantic utilization in Tooie remains unvalidated.
- `recomputil::clear_all_object_data(size_t type_index)`: Clears custom object payloads on map transitions.
- `recomputil::register_data_api_exports()`: Publishes extension symbols (`recomputil_*`) to guest dispatch tables.
- **Data Container Library (`src/game/recomp_data_api.cpp`)**:
  - Exports helper functions for mods, providing host-side implementations of hash maps, hash sets, and slot maps accessible to guest code.

### 3.3 Texture Pack Container Format (`.rtz`)
- **Container**: ZIP archive formatted with `.rtz` extension.
- **Manifest**: Contains `rt64.json` defining texture hashes, mipmap specifications, and PBR material definitions.
- **Registration (`src/main/main.cpp`)**:
  - Registers `rtz` container format with runtime mod manager.
  - Registers deprecated mod rejection for `bk_recomp_mod_fov_slider` via `recomp::mods::register_deprecated_mod`.
  - Legacy initialization calls `register_bk_overlays()` and `register_bk_patches()` are retained from the recompiler architecture.

---

## 4. Invariants & Verification Status

While the modding and extension infrastructure compiles cleanly into `TooieRecompiled`, runtime modding in Banjo-Tooie has not undergone dedicated verification. The system status is classified as `INHERITED_UNTESTED`.