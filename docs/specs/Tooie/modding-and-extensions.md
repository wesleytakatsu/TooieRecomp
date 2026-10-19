---
title: "TooieRecomp Spec — Modding, Extensions and Asset Customization"
description: "Specification for the recomp extension API, extended object data structures, texture pack container formats (.rtz), and mod callback hooks."
status: current
area: "specs/Tooie"
updated: 2026-10-16
tags: [specs, tooierecomp, modding, extensions, rtz, textures, api]
---

# Modding, Extensions and Asset Customization Specification

## 1. Subsystem Architecture

TooieRecomp provides extensible modding APIs inherited from the static recompilation architecture (`RecompModTool`, `specs/BK/mods.md`), enabling community developers to hook game functions, attach arbitrary data to game actors, and load custom high-resolution textures.

```text
+---------------------------------------------------------------------------------+
|                       Modding & Extension Architecture                          |
|  - Data Extension API (src/game/recomp_data_api.cpp)                            |
|  - Object Extension API (src/game/recomp_extension_api.cpp)                      |
|  - Mod Content Containers (.rtz, rt64.json via src/main/main.cpp)               |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                         Runtime Integration Hooks                               |
|  - recomp::mods::register_mod_content_type (Runtime texture pack toggles)       |
|  - recomputil::init_extended_object_data (Object subtype custom payloads)       |
|  - dod::slot_map32<ExtensionData> (Lock-free object handle allocation)          |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Implementation Status |
| :--- | :--- | :--- | :--- |
| **REQ-MOD-01** | Extended Object Data | Mods can attach custom memory blocks to spawned game actors. | `INHERITED_INFRASTRUCTURE` / `UNTESTED_FOR_TOOIE` |
| **REQ-MOD-02** | Type Contexts & Slots | `dod::slot_map32` manages per-object extension lifecycle cleanly. | `INHERITED_INFRASTRUCTURE` / `UNTESTED_FOR_TOOIE` |
| **REQ-MOD-03** | Texture Pack Support | Loads `.rtz` texture archives containing replacement RT64 textures. | `INHERITED_INFRASTRUCTURE` / `UNTESTED_FOR_TOOIE` |
| **REQ-MOD-04** | Runtime Mod Toggles | Allows enabling, disabling, and reordering texture packs at runtime. | `INHERITED_INFRASTRUCTURE` / `UNTESTED_FOR_TOOIE` |
| **REQ-MOD-05** | Backward Compatibility | Reuses BK modding contracts without breaking Tooie memory invariants. | `INHERITED_INFRASTRUCTURE` / `UNTESTED_FOR_TOOIE` |

---

## 3. Interface & Memory Contracts

### 3.1 Object Extension API (`src/game/recomp_extension_api.cpp`)
```cpp
struct ExtensionInfo {
    // Subtype ID, or 0xFFFFFFFF for generic object data.
    uint32_t subtype_index;
    // Byte offset from start of extension data.
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
- `recomputil::init_extended_object_data(size_t num_types)`: Initializes extension contexts for each actor type on engine boot.
- `recomputil::clear_all_object_data(size_t type_index)`: Resets custom actor payloads on area transition or scene reload.
- `recomputil::register_data_api_exports()`: Publishes extension management symbols to the guest symbol table.

### 3.3 Texture Pack Container Format (`.rtz`)
- **Container**: ZIP archive with `.rtz` extension.
- **Manifest**: Contains `rt64.json` defining texture hashes, mipmap levels, and materials (albedo, normal, roughness, metallic).
- **Registration (`src/main/main.cpp:1135-1147`)**:
  ```cpp
  recomp::mods::ModContentType texture_pack_content_type{
      .content_filename = "rt64.json",
      .allow_runtime_toggle = true,
      .on_enabled = enable_texture_pack,
      .on_disabled = disable_texture_pack,
      .on_reordered = reorder_texture_pack,
  };
  auto id = recomp::mods::register_mod_content_type(texture_pack_content_type);
  recomp::mods::register_mod_container_type("rtz", std::vector{ id }, false);
  ```

---

## 4. Invariants & Relation to Banjo-Kazooie Contract

- **Contract Reuse**: The extension system reuses the design established in [[../BK/mods|BK Modding Specification]].
- **Memory Safety Invariant**: Extended object memory allocations are tracked independently of guest RDRAM and are deallocated on actor despawn to prevent memory leaks.
- **Validation Boundary & Reality Check**: While the modding and extension infrastructure (`recomp_data_api.cpp`, `recomp_extension_api.cpp`, and `.rtz` registration in `src/main/main.cpp:1135-1147`) compiles cleanly into `TooieRecompiled`, this code represents inherited infrastructure from the BanjoRecomp / RecompModTool baseline. Active modding capabilities, `.rtz` replacement packs in Banjo-Tooie, and actor extension data on actual Tooie entities have NOT undergone runtime QA or verification. The system is classified strictly as `INHERITED_INFRASTRUCTURE / UNTESTED_FOR_TOOIE` until validated in a dedicated post-parity enhancement phase.
