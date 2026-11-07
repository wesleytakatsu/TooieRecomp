---
title: "TooieRecomp Spec — Modding, Extensions and Asset Customization"
description: "Specification for the recomp extension API, extended object data structures, texture pack container formats (.rtz), and mod callback hooks."
status: current
area: "specs/Tooie"
updated: 2026-10-31
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
| **REQ-MOD-03** | Texture Pack Support | Loads `.rtz` and structurally identified `.zip` texture archives containing replacement RT64 textures. | `VALIDATED_WINDOWS` (§4) |
| **REQ-MOD-04** | Runtime Mod Toggles | Allows enabling, disabling, and reordering texture packs at runtime. | ZIP enable/disable `VALIDATED_WINDOWS`; reordering `NOT_RUN` |
| **REQ-MOD-05** | Deprecated Mod Rejection| Rejects incompatible legacy mods (`bk_recomp_mod_fov_slider`). | `IMPLEMENTED` |
| **REQ-MOD-06** | Native Audio ZIP | `audio.json` maps CSP sequences to MP3 playback, with runtime toggles and queue-level proof logging. | `HOST_TESTED`; three mappings and toggles observed in user Windows test; synchronized layers host-tested with positive user-reported Windows gameplay; installer UI and Linux validation pending |

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
- **Layout**: `rt64.json` (`configuration.hashVersion` = 5, the current RT64 TMEM hash) plus `.png`/`.dds` files; each
  `textures[]` entry maps `hashes.rt64` (16 hex digits) to a `path`. `mod.json` is optional for `.rtz`: without it the
  mod id is the file stem. With it, `game_id` must be `bt`.
- **Install**: drop the file in `%LOCALAPPDATA%\TooieRecompiled\mods` (Linux: `~/.config/TooieRecompiled/mods`). A new
  pack is enabled by default and recorded in `mods.json`.
- **Hashes**: RT64 hashes the loaded TMEM contents, so packs made for other emulators (GLideN64/Rice `hires_texture`)
  do not match. Hashes come from RT64's texture dump (developer mode → texture replacements → "Start dumping textures";
  dump base names are `<hash>.v5`).
- **Registration (`src/main/main.cpp`)**:
  - Registers `rtz` (texture content) and lowercase `zip` (texture and audio content) with the runtime mod manager.
  - Registers deprecated mod rejection for `bk_recomp_mod_fov_slider` via `recomp::mods::register_deprecated_mod`.
  - Legacy initialization calls `register_bk_overlays()` and `register_bk_patches()` are retained from the recompiler architecture.

### 3.4 Plain ZIP Texture Packs

- A `.zip` file can be copied intact to the mods directory or installed through the Mods interface on the initial
  launcher screen, before starting the game. Installation and refresh buttons are disabled once the game is running;
  eligible already-installed mods can still be toggled. Installation copies
  the entire archive without extracting, converting, renaming or repacking its contents.
- The shared `find_zip_content_root` probe searches case-sensitively for registered markers (`rt64.json`, `audio.json`). A marker at the archive root takes
  precedence; otherwise exactly one internal directory must contain a marker. Markers in the same directory share one
  root; markers in different internal directories are ambiguous. Multiple internal roots are rejected with an
  explicit error. Roots beginning with `/`, containing `:` or containing `.`/`..` path components are rejected.
- The selected root is stored in `ZipModFileHandle`. Manifest, content-marker and thumbnail reads use that same root;
  the renderer receives the physical ZIP filename plus the root through RT64's existing `zipBasePath` interface.
- A ZIP without either marker is ignored by the startup scanner. The installer retains its existing collection-of-mods
  fallback for such ZIPs. `.rtz`, `.nrm`, loose directories and embedded mods retain their existing root behavior.
- `mod.json` is optional. Without it, the default id is the ZIP filename stem. A present but invalid manifest reports
  the existing manifest error. The ZIP content whitelist excludes code content.
- Detection is structural; RT64 validates the texture database when loading it. This does not convert a Rice-only pack
  or discover mappings for a collection of numbered images. Uppercase `.ZIP` extensions and backslash-separated entry
  names are outside this format's supported input contract.

---

## 4. Invariants & Verification Status

### Audio pack format (prototype)

An audio ZIP contains `audio.json`, MP3 files and optionally `mod.json` at one content root. `mod.json` uses `game_id=bt`
and the existing mod identity/version fields. It is installed through the existing Mods installer before game startup and can be enabled,
disabled and reordered. The first matching pack in Mods order wins. Within a pack the first matching rule wins; put
specific channel variants before a generic rule for the same sequence.

`audio.json` has `schema_version: 1` or `2` and a nonempty `sequences` array. Each rule has:

| Field | Contract |
| --- | --- |
| `id` | Native integer CSP sequence ID, 0..32767; validated before narrowing; not an album track number. |
| `file` | Relative, case-sensitive `.mp3` path; no absolute path, backslash, empty, `.` or `..` component. |
| `gain` | Optional finite linear gain, 0..2; default 1. |
| `start_frame`, `end_frame` | Optional decoded PCM interval, start inclusive/end exclusive; defaults to whole file. |
| `loop_start_frame`, `loop_end_frame` | Optional loop within the playback interval; both must form a valid nonempty interval. Without a loop, play once. |
| `channel_mask`, `channel_value` | Optional paired uint32 fields; either both must be present or neither. Matches `(native_channels & channel_mask) == channel_value`. Mask must be nonzero; value must fit it. |
| `native_channel_mask` | Schema 2 only: nonempty uint16 channel group. Requires exact activation selector (`channel_mask: 4294967295`). All rules of that sequence form a synchronized family; at most four disjoint groups with identical start/end/loop bounds. Mixing generic and native-layer rules for one sequence is rejected. |

MP3s must be stereo at 48000 Hz. Missing files, unsupported formats, invalid bounds, duplicate selectors and excessive
compressed-memory use reject the pack without muting original audio. A file failing to enable is reported via a message
box; its saved enabled flag is not automatically rewritten. The pack can be corrected and re-enabled.

When any native-layer rule matches the requested channel mask, every layer of its family runs on the shared clock.
Current native channel gains determine weights, including outgoing layers during transitions. For unknown activation
masks the family does not replace native audio. Duplicate selectors and overlapping groups reject the pack.

`tools/build_audio_pack.py <extracted-album> <output.zip>` builds the private version `0.2.0` four-track prototype using
original MP3 bytes and verifies their source-manifest SHA-256 values. The builder records measured loops and the
trimmed Blue Eggs endpoint as metadata. Spiral Mountain uses the observed surface (`0x63FF`) and underwater
(`0x1C00`) groups. Their semantic association remains `STRONG_INFERENCE` pending annotated gameplay QA; it is not
a general channel mapping for other songs. Source audio and the resulting ZIP are private inputs, never repository
or game-release assets. Audio playback and proof logging details: [[audio-system]].

While the modding and extension infrastructure compiles cleanly into `TooieRecompiled`, runtime modding in Banjo-Tooie has not undergone dedicated verification. The system status is classified as `INHERITED_UNTESTED`, except:

- **REQ-MOD-03 `VALIDATED_WINDOWS` (2026-10-27, `d430ddc`)**: the 168 textures RT64 dumped during the first 40 s of
  attract (Rare logo → title screen) were packed as magenta checkerboards into a test `.rtz` with a `mod.json`. Installed
  in the mods folder, the pack was auto-enabled at startup and every captured frame (8/15/25/40 s) showed the
  replacements; without it the same frames are the original art. Runtime toggling from the Mods menu (REQ-MOD-04) and
  Linux were not tested.

- **Plain ZIP `VALIDATED_WINDOWS` (2026-10-30, user validation)**: BT Reloaded 0.1.1 was tested through manual copy into
  the mods directory, installation through the interface and enable/disable of its texture replacements. The original
  ZIP remains unchanged. Host test `tooie_texture_zip_mod_test` covers root selection, relative reads, the real mod-folder
  scanner, the default manifest and texture-only content detection (C1-C13). Linux and reordering remain `NOT_RUN`.
  BT Reloaded is a separate project by GhostlyDark (https://github.com/GhostlyDark/BT-Reloaded/releases, published as
  `.rtz` and GLideN64 ZIPs); the READMEs link to it and the releases do not include it.

- **REQ-MOD-06 `USER_REPORTED_WINDOWS` (2026-10-31)**: the user reported that audio prototype `0.2.0` appears to
  work correctly after the synchronized surface/underwater changes. Prior logs established MP3 contribution and
  runtime toggling. This is perceptual gameplay feedback; a measured acoustic comparison, audio installation through
  the UI and Linux remain unverified. The host harness covers decoding, shared phase, native gains and reversals.

Code mods (`.nrm`) load through the same manager, but Tooie publishes no mod SDK yet (no `RECOMP_EXPORT` patches, no
headers or mod-tool symbol file). General data-file replacement remains unsupported. External audio is supplied through
the native `audio.json` content type above. In-game text languages use [language packs](language-packs.md).
