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
| **REQ-MOD-06** | Native Audio ZIP | `audio.json` maps CSP sequences (32 channels, area masks and tempos) to MP3 playback, with runtime toggles and queue-level proof logging. | `HOST_TESTED` + `RUNTIME_TESTED` on Windows: the full HD map plays the expected MP3 for every area preset and sampled songs/jingles in game (renderer with packs); real area transitions, campaign, installer UI and Linux pending |

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

### Audio pack format

An audio ZIP contains `audio.json`, MP3 files and optionally `mod.json` at one content root. `mod.json` uses `game_id=bt`
and the existing mod identity/version fields. It is installed through the existing Mods installer before game startup and can be enabled,
disabled and reordered. The first matching pack in Mods order wins. Within a pack the first matching rule wins; put
specific channel variants before a generic rule for the same sequence.

`audio.json` has `schema_version: 1` or `2` and a nonempty `sequences` array of at most 1024 rules. Each rule has:

| Field | Contract |
| --- | --- |
| `id` | Native integer CSP sequence ID, 0..32767; validated before narrowing; not an album track number. |
| `file` | Relative, case-sensitive `.mp3` path; no absolute path, backslash, empty, `.` or `..` component. |
| `gain` | Optional finite linear gain, 0..2; default 1. |
| `start_frame`, `end_frame` | Optional decoded PCM interval, start inclusive/end exclusive; defaults to whole file. |
| `loop_start_frame`, `loop_end_frame` | Optional loop within the playback interval; both must form a valid nonempty interval. Without a loop, play once. |
| `channel_mask`, `channel_value` | Optional paired uint32 fields; either both must be present or neither. Matches `(native_channels & channel_mask) == channel_value`. Mask must be nonzero; value must fit it. |
| `native_channel_mask` | Schema 2 only: nonempty uint32 channel group (the game's sequences have 32 channels). Requires exact activation selector (`channel_mask: 4294967295`). All rules of that sequence form a synchronized family of at most sixteen layers with the same start; each layer may have its own loop (Rare's player loops every channel on its own). Groups may overlap (an area that adds channels to another). Mixing generic and native-layer rules for one sequence is rejected. |
| `bpm` | Optional tempo the MP3 was recorded at (an area's tempo, 0 < bpm ≤ 1000). With it the rule replaces only while the game's requested tempo is within 0.5 BPM, and the layer follows the native beat position instead of elapsed samples. The same selector may appear once per tempo. |

MP3s must be stereo at 48000 Hz. Rules may share an MP3 when they share its playback contract. Missing files,
unsupported formats, invalid bounds, duplicate selectors and excessive compressed-memory use reject the pack without
muting original audio. A file failing to enable is reported via a message box; its saved enabled flag is not
automatically rewritten. The pack can be corrected and re-enabled.

Packs up to 96 MB of MP3s are read and checked when enabled and stay in memory. Larger packs (a full soundtrack is
about 1 GB) load on demand: the manifest and file sizes are checked when enabled, every MP3 is decoded once in a
background thread to check it and build its seek table, small files (jingles, up to 1 MB) stay in memory, and the
MP3s of a sequence are loaded when the game starts it. Loaded MP3s beyond `TOOIE_AUDIO_MOD_CACHE_MB` (default 384)
are released least recently used first, never while their sequence plays. Until a rule's MP3s are ready the original
music plays; a rejected MP3 leaves its rules on the original music and is logged. Layers of an on-demand pack must
state `end_frame`, so the shared clock is known before decoding.

When any native-layer rule matches the requested channel mask, every layer of its family runs on the shared clock.
Timeline: a layer without `bpm` plays at the elapsed sample count since the sequence started. A layer with `bpm`
plays at the native beat position (beats advance at the sequence's current tempo, read from the player) converted to
its own tempo, so entering an area at another tempo starts the new layer at the same musical point. While the native
tempo moves between areas a layer at another tempo drifts; past 25 ms it seeks back to the beat position. Layers at
different tempos must start at the same beat; their loops are converted per file. An MP3 without a loop that ends
while its sequence keeps playing hands that slot back to the original music (`ENDED` in the proof log).
Layer weights are the non-negative least-squares combination of the layer channel groups closest to the current native
channel gains (`255` = 1, otherwise `q / 256`): disjoint groups get their mean gain, an overlapping area resolves to its
own layer in steady state, and the native crossfade blends the layers involved. Silent layers are not decoded; they
seek back to the shared clock when the game brings them back. For unknown activation masks the family does not
replace native audio. Duplicate selectors reject the pack.

`tools/build_audio_pack.py <album> <output.zip> [--mapping <map.json>]` builds a pack from a map: it verifies each
album MP3 against the map's SHA-256 and copies the bytes unchanged (`ZIP_STORED`). Source audio and the resulting ZIP
are private inputs, never repository or game-release assets. Audio playback and proof logging details:
[[audio-system]].

Maintainer renderer: `TOOIE_TEST_AUDIO_RENDER=<plan.tsv>` (output folder `TOOIE_TEST_AUDIO_RENDER_OUT`, default the
plan's folder) waits until Game Select plays, then for each plan line `<name>	<sequence hex>	<mask hex or ->	<seconds>`
plays that sequence in the Game Select slot at full volume with that channel mask, mutes the other slots, records the
native mix as 48 kHz stereo float WAV and sends silence to the device; `render.log` records the channel mask and the
32 channel gains each second. While it runs no audio pack replaces anything. Empty sequence IDs (for example `0x13`)
crash the original player and must not be listed. A fifth column sets the area tempo (BPM), applied the way the game
applies a preset (the renderer re-requests it until it holds over the sequence's own start tempo). With
`TOOIE_TEST_AUDIO_RENDER_MOD=1` enabled packs keep replacing music and the recording is the final mix, to check a pack
against the native recordings. It is off unless the variable is set.

The Banjo-Tooie HD soundtrack map is `tools/audio/banjo-tooie-hd.json` (no audio: album file names and SHA-256,
sequence IDs, channel masks, tempos and loop points); `tools/build_audio_pack.py <album> <out.zip>` builds the pack
from the user's own album copy, and `tools/audio/analysis/` holds the scripts that produced the map.

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
