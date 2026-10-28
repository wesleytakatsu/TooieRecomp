---
title: "TooieRecomp Spec — In-Game Language Localization, Bundles, and Packs"
description: "Technical specification for in-game language selection, the built-in pt-BR language bundle (.tlpb) and procedural font synthesis, the .tlp language pack container, in-game C++ European ROM language import, runtime in-memory ROM image rewriting, and font glyph mapping."
status: current
area: "specs/Tooie"
updated: 2026-10-27
tags: [specs, tooierecomp, localization, language, tlp, tlpb, text, font]
---

# In-Game Language Localization, Bundles, and Packs Specification

## 1. Subsystem Architecture

TooieRecomp implements in-game language localization for dialogs and menu texts through a dual architecture:
1. **Built-in Brazilian Portuguese (`pt-BR`):** Ships directly with the project and release packages as `assets/languages/pt-BR.tlpb` (~309 KB). It contains only the translator's work (CLEHPTON's fan translation, redistributed with permission obtained by the project owner) and metadata. At startup, the game builds the `pt-BR` pack in memory from the bundle and the player's USA ROM, synthesizing accented font frames procedurally at runtime. No European ROM or external pack file is required.
2. **In-Game European Language Import (`fr`, `de`, `es`):** When the player selects Français, Deutsch, or Español in Settings and the corresponding pack (`<app folder>/languages/<tag>.tlp`) is missing, an in-engine localized modal prompt asks the player to select their verified European ROM (`Banjo-Tooie (Europe) (En,Fr,De,Es).z64`). An embedded C++ importer extracts the language pack directly, validates CRC32 integrity, and atomically writes the `.tlp` container to the application folder. The European ROM is used only once for extraction; the game always executes from the USA ROM. No Python installation is needed by players.
3. **Developer Reference Tooling:** `tools/build_language_packs.py` serves as a developer tool to build the `assets/languages/pt-BR.tlpb` bundle (from European ROM + CLEHPTON v4 BPS) and generate reference `.tlp` packs for automated unit test parity.

```text
======================= pt-BR Built-In Pipeline =======================
  [assets/languages/pt-BR.tlpb] + [Player's USA ROM (0x0C22 Font 1)]
                             │
                             ▼  boot (Game Language == "PortugueseBrazil")
  [src/game/language_pack.cpp]:
    ├─ Builds pt-BR pack in memory from bundle (2230 kind-0 + 4 kind-1 overlays)
    ├─ Patches Font 0 (37 byte runs applied over USA 0x0C21 after CRC32 check)
    └─ Synthesizes Font 1 (include/tooie_font_sprite.h: 23 accented frames)
                             │
                             ▼
  [In-Memory ROM Tail Relocation & Cart Asset Table Rewrite (~49.7 MB)]

==================== FR / DE / ES In-Game Import =====================
  User selects Français / Deutsch / Español (pack missing in languages/)
                             │
                             ▼
  Localized UI Prompt -> User selects European ROM via system file dialog
                             │
                             ▼
  [include/tooie_language_import.h] (C++ native extraction):
    ├─ Validates European ROM size + CRC32
    ├─ Extracts text assets (Rules 1-3) & European Font 1 (0x0C22)
    └─ Writes <app folder>/languages/<tag>.tlp atomically
                             │
                             ▼  subsequent boot
  [src/game/language_pack.cpp]:
    └─ Loads <tag>.tlp from disk into in-memory ROM buffer
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-LANG-01** | Language Selection UI | Settings menu exposes "Game Language" under General (English, Português (Brasil), Français, Deutsch, Español). | `VALIDATED_WINDOWS` (§6) |
| **REQ-LANG-02** | Config Persistence | Selected language persists under `language` key in `general.json`; applies on boot or restart. | `VALIDATED_WINDOWS` (§6) |
| **REQ-LANG-03** | Language Pack Format | `.tlp` binary container adheres to version 1 specification with little-endian encoding and asset table payloads. | `VALIDATED_WINDOWS` (§3) |
| **REQ-LANG-04** | Built-in Language Bundle | `assets/languages/pt-BR.tlpb` embeds translation-only data, overlays, font 0 patch runs, and charset mapping; zero Rare game data. | `VALIDATED_WINDOWS` (§3) |
| **REQ-LANG-05** | Procedural Font 1 Synthesis | `include/tooie_font_sprite.h` generates 23 accented frames procedurally from player's USA font 1 for pt-BR at runtime. | `VALIDATED_WINDOWS` (§5) |
| **REQ-LANG-06** | In-Game C++ ROM Importer | `include/tooie_language_import.h` and UI prompt extract FR/DE/ES packs from verified European ROM without Python. | `VALIDATED_WINDOWS` (§4) |
| **REQ-LANG-07** | In-Memory ROM Rewriting | Loader relocates assets starting from first replaced ID to image tail and updates cart table without modifying disk files. | `VALIDATED_WINDOWS` (§5) |
| **REQ-LANG-08** | Font & Accent Mapping | `func_800D35D0` host implementation routes to European font-1 mapping and accent decoding when pack/bundle charset is present. | `VALIDATED_WINDOWS` (§5) |
| **REQ-LANG-09** | Graceful Fallback & Validation | Missing pack logs `[tooie-lang]`, displays non-fatal message box, and boots cleanly in English; invalid ROMs rejected. | `VALIDATED_WINDOWS` (§5, §6) |
| **REQ-LANG-10** | Diagnostic Test Override | `TOOIE_TEST_LANGUAGE_PACK` environment variable specifies explicit `.tlp` path, bypassing the UI message box. | `VALIDATED_WINDOWS` (§5) |

---

## 3. Container Specifications

All multi-byte numeric fields are encoded in **little-endian** byte order, except asset payloads which retain the game's native big-endian format.

### 3.1 `.tlp` Format (v1)

Used for standalone language packs stored in `<app folder>/languages/<tag>.tlp`.

#### Header Layout

| Offset | Type | Field | Description |
| :--- | :--- | :--- | :--- |
| `0x00` | `char[4]` | `magic` | Fixed ASCII magic identifier `"TLPK"`. |
| `0x04` | `uint32_t` | `version` | Format version; must equal `1`. |
| `0x08` | `char[16]` | `language_tag` | BCP 47 style tag (`[A-Za-z0-9-]`, 1–15 characters, NUL-padded). |
| `0x18` | `uint32_t` | `base_image_size` | Base USA decompressed image size; must equal `35098224`. |
| `0x1C` | `uint32_t` | `entry_count` | Number of replacement asset entries (`1..13926`). |
| `0x20` | `uint32_t` | `charset_length` | Font-1 character set length (`0` or `1..255`). |
| `0x24` | `uint8_t[N]` | `font1_charset` | Character set byte array (`N = charset_length`). |
| `0x24 + N` | `uint8_t[30]`| `accent_map` | Accent mapping table (present **only** if `charset_length != 0`). |

#### Asset Entry Layout

Entries follow immediately after the header structures and are sorted in **strictly ascending** `asset_id` order:

| Field | Type | Description |
| :--- | :--- | :--- |
| `asset_id` | `uint32_t` | Target asset index in the game cart asset table (`< 13926`). |
| `payload_size` | `uint32_t` | Size of payload in bytes; must be `>= 8` and a multiple of 8. |
| `payload` | `uint8_t[]` | Raw asset bytes matching ROM table storage format (see below). |

#### Payload Structure

The payload matches the format stored in the cart table payload segment:
1. `uint16_t` (big-endian): `ceil(uncompressed_size / 16)`.
2. Raw deflate stream (`wbits = -15`).
3. `0xAA` byte padding to pad the total entry size to an 8-byte boundary.

### 3.2 `.tlpb` Language Bundle Format (v1)

Used for the built-in translation bundle (`assets/languages/pt-BR.tlpb`, ~309 KB) shipped in git and release packages. It contains no proprietary Nintendo or Rare dialog scripts or font artwork.

#### Bundle Header Layout

| Field | Type | Description |
| :--- | :--- | :--- |
| `magic` | `char[4]` | Fixed ASCII magic identifier `"TLPB"`. |
| `version` | `uint32_t` | Format version; must equal `1`. |
| `language_tag` | `char[16]` | BCP 47 style tag (`"pt-BR"`, NUL-padded). |
| `base_image_size` | `uint32_t` | Base USA decompressed image size; must equal `35098224`. |
| `section_count` | `uint32_t` | Number of translated text sections (kind 0). |
| `overlay_count` | `uint32_t` | Number of overlay sections (kind 1). |
| `font0_patch_runs` | `uint32_t` | Number of byte run patches for font 0 (`37`). |
| `charset_length` | `uint32_t` | Font-1 character set length (`73`). |
| `font1_charset` | `uint8_t[73]` | European font-1 character set table. |
| `accent_map` | `uint8_t[30]` | European font-1 accent mapping table. |

#### Bundle Section Types
1. **Kind 0 (Full Replacement Section):**
   - 2,230 translated text sections completely authored by the translator.
   - Replaces the entire USA text section payload.
2. **Kind 1 (Overlay Section):**
   - 4 menu sections where only specific records are translated.
   - Encodes a bitmask / flag array of keep/replace decisions per USA record: English records are referenced directly from the player's USA ROM in memory, not duplicated in the bundle.
3. **Font 0 Patch Runs:**
   - 37 byte runs (209 bytes total) representing the translator's modifications to font 0 (asset `0x0C21`).
   - Applied over the player's USA font 0 after verifying target asset size and CRC32.

---

## 4. Language Import and Developer Tooling

### 4.1 In-Game C++ European ROM Importer

When a European language (Français, Deutsch, Español) is selected in Settings and the corresponding `.tlp` file is not present:
1. `src/game/language_import_ui.cpp` first reads and validates the stored USA image (`<app folder>/bt.n64.us.1.0.z64`, size + CRC32). If it is missing or invalid, a localized prompt asks the player to load the USA ROM first (launcher "Load ROM"), the previous language is restored, and nothing else happens. Otherwise it opens a localized modal prompt explaining that the European ROM (`Banjo-Tooie (Europe) (En,Fr,De,Es).z64`, SHA-1 `93bf2fac1387320ad07251cb4b64fd36bac1d7a6`) is required once to extract the language pack.
2. Clicking "Choose European ROM" opens the native system file dialog.
3. `include/tooie_language_import.h` verifies the selected ROM:
   - Size check: 33,554,432 bytes.
   - CRC32 check: verifies European ROM identity. Invalid ROMs (e.g. Australian release) are rejected.
4. The importer decompresses text assets and European font 1 (`0x0C22`), parses language sections, applies conversion rules (identical to developer generator output), and constructs the `.tlp` container in memory.
5. The container is written atomically to `<app folder>/languages/<tag>.tlp` via temporary file replacement.
6. An in-game info prompt reports success or failure in the target language (`[tooie-lang] imported <tag> in N ms` or the failure reason is logged). If cancelled or failed, the previous language setting is restored. Showing a prompt from inside another prompt's action relies on the local recompui patch `vendor/patches-local/recompfrontend-prompt-chain.patch` (a prompt generation counter: the closing prompt is hidden only if its action did not open a new one).
7. Players require no external tools or Python runtime.

### 4.2 Developer Tool (`tools/build_language_packs.py`)

`tools/build_language_packs.py` is maintained strictly as a developer utility:
- Builds the `assets/languages/pt-BR.tlpb` bundle from the European ROM and CLEHPTON's v4 BPS patch.
- Applies project fixes to the pt-BR text from `PTBR_TEXT_FIXES` (keyed by asset ID + record type). Current fix:
  title-screen legal line (`0x186C`, record `0x85`) shortened to "A LOGO RAREWARE É MARCA DA RARE." because the
  46-character translation was cut at both edges of the USA title layout (about 38 characters fit); verified in game.
- Generates reference `.tlp` packs for `fr`, `de`, and `es` to verify C++ importer parity in automated tests (`tooie_language_import_test` verifies bit-for-bit parity across all inflated entries: fr 2,234, de 2,231, es 2,235 entries).

#### CLI Usage

```sh
python tools/build_language_packs.py \
    --pal-rom "<path_to_european_rom>" \
    [--ptbr-bps "<path_to_v4_patch.bps>"] \
    [--usa-image "<path_to_bt.n64.us.1.0.z64>"] \
    [--out-dir "<destination_folder>"] \
    [--build-bundle] \
    [--report "<path_to_report.json>"]
```

#### Asset Conversion & Coverage Rules

1. **Rule 1 (Identical Structure):** The language section has matching groups, record types, params, and control-record data to the USA asset; the translated section is used.
   - Coverage: `fr`: 2,227; `de`: 2,225; `es`: 2,229; `pt-BR`: 2,230 assets.
2. **Rule 2 (Single-Group Menu Alignment):** Single-group menu assets translated record-by-record when the European English record matches the USA record.
   - Coverage: `fr`: 5; `de`: 4; `es`: 4; `pt-BR`: 3 assets.
3. **Rule 3 (Speaker-Type Match):** Same layout and control records, but speaker param differs; USA param is retained with translated text.
   - Coverage: 1 asset across each language.
4. **Fallback to English:**
   - Staff credits (`0x1B6F`–`0x1B90`): retained in English (proper names).
   - USA-only asset `0x1A89`: retained in English.
   - French asset `0x15CB`: retained in English (contains placeholder data in European ROM).
   - German dialogs `0x0FB5`, `0x14F7`, `0x14F8`, `0x1809`: retained in English due to divergent control record structures.

---

## 5. Runtime Architecture & ROM Rewriting

### 5.1 Storage Locations & Configuration

- **Built-in Bundle:** Tracked in git at `assets/languages/pt-BR.tlpb` and packaged in releases under `assets/languages/`.
- **Imported Packs:**
  - Windows: `%LOCALAPPDATA%\TooieRecompiled\languages\<tag>.tlp`
  - Linux: `~/.config/TooieRecompiled/languages/<tag>.tlp` (or `$APP_FOLDER_PATH/languages/<tag>.tlp`)
- **Tags:** `pt-BR`, `fr`, `de`, `es`.

Configuration key in `general.json`:
```json
{
  "language": "PortugueseBrazil"
}
```

### 5.2 Procedural Font 1 Synthesis (`include/tooie_font_sprite.h`)

For Brazilian Portuguese, font 1 is generated entirely in memory at boot:
1. The player's USA font 1 (`0x0C22`, 51 IA8 frames, height 23) is read from the base decompressed ROM image.
2. Base frames are padded to height 31 by prepending 8 transparent pixel rows, matching the European layout.
3. 23 accented glyph frames are synthesized procedurally in the translator's frame order (51–73):
   `Ã`, `Á`, `À`, `É`, `È`, `Í`, `Ì`, `Ç`, `Õ`, `Ó`, `Ò`, `Ü`, `Ú`, `Ù`, `¿`, `¡`, `ª`, `°`, `Ê`, `Ï`, `Ô`, `Â`, and the comma.
4. Accents are rendered from compact procedural masks using the font's fill and outline colors; `¿` and `¡` are rotated copies of `?` and `!`; the comma is formed by shifting the apostrophe glyph downward.
5. The developer target `tooie_font_preview` outputs the complete synthesized font sheet as a PPM image for visual inspection.

### 5.3 Boot-Time In-Memory ROM Rewriting

When a translated language is configured (or overridden via `TOOIE_TEST_LANGUAGE_PACK`):
1. `librecomp` invokes `on_rom_loaded_callback` after loading `bt.n64.us.1.0.z64` into memory.
2. For `pt-BR`: `src/game/language_pack.cpp` reads `assets/languages/pt-BR.tlpb`, patches font 0, synthesizes font 1, and builds the replacement asset table in memory.
3. For `fr`, `de`, `es`: `src/game/language_pack.cpp` reads and validates `<tag>.tlp` from disk.
4. Asset relocation:
   - Lowest modified asset ID is determined (e.g. `0x0C21`).
   - Original asset payloads from this starting ID to the end of the asset table are relocated to the end of the buffer, expanding in-memory ROM size from 35,098,224 bytes to ~49.7 MB.
   - Replacement payloads overwrite target entries; unmodified entries in the relocated tail are preserved.
   - Asset table entries (`0x5188 + id * 4`) are rewritten with updated offsets, preserving original flags (`(offset / 4) << 8 | flags`).
   - The expanded buffer is committed via `recomp::set_rom_contents()`.
5. Normal DMA routines (`func_800D58FC`, `func_800D5B34`) load translated text and font assets transparently.

### 5.4 Font Mapping Replacement (`src/game/text_font_map.cpp`)

Host implementation of `func_800D35D0(font, c)`:
- When English is active, executes original USA behavior (case folding for fonts 1 and 3; 128-entry lookup table at `0x8012B370` for font 1).
- When a language pack or bundle is active:
  - Font 1 maps `,` to glyph index `0x49`.
  - Font 1 maps accent escape codes `0x5B`..`0x78` to `0x33 + accent_map[c - 0x5B]`.
  - Font 1 maps remaining characters through a 256-entry table generated from the 73-character charset.
  - Font 2 handles special codes (`-1` for `0xAD`, `c - 0x1A` for `c >= 0xAF`).
  - Fonts 0 and 3 retain standard indices.

### 5.5 Error Handling & Fallback

- If a pack fails validation or cannot be loaded, `[tooie-lang] <reason>` is logged and the game boots cleanly in English.
- If import is cancelled or encounters an invalid ROM, the previous language setting is preserved.
- Setting `TOOIE_TEST_LANGUAGE_PACK` specifies an explicit `.tlp` path and suppresses UI popups.

---

## 6. Verification Status & Known Limitations

### 6.1 Automated Unit Tests (Windows x64, 2026-10-27)

- `tooie_language_pack_test`: PASS. Validates header parsing, endianness, entry extraction, and asset table relocation on synthetic images.
- `tooie_text_font_map_test`: PASS. Validates USA vs European glyph mapping tables, comma override, and accent map translations.
- `tooie_language_import_test`: PASS. Validates native C++ importer output against developer reference packs (matching headers and inflated entries).
- `tooie_language_bundle_test`: PASS. Validates TLPB bundle parsing, kind 0 sections, kind 1 overlays, font 0 patch runs, and CRC32 verification.
- `tooie_font_sprite_test`: PASS. Validates runtime font 1 frame expansion, procedural accent composition, and dimensions.
- `tools/test_build_language_packs.py`: PASS. Validates developer pack generator, BPS patching, and conversion coverage.
- `tooie_font_preview`: PASS. Verified synthesized font 1 visual PPM output.

### 6.2 Interactive In-Game Validation (Windows x64, 2026-10-27)

- **Brazilian Portuguese Built-in (`pt-BR`):** Tested with no `languages/` folder. Applied cleanly from bundle (2,236 assets). Pause-menu confirmation renders "NÃO" with synthesized Ã. File selection and intro dialogs render accented capital letters correctly.
- **French In-Game Import (`fr`):** Selected Français in launcher Settings with no pack present. Modal prompt appeared in French. Clicking Cancel restored English. Clicking Confirm and selecting the European ROM extracted and wrote `fr.tlp` atomically; success reported; game booted in French.
- **German In-Game Import (`de`):** Tested with Australian ROM dump: rejected with an error in German; language setting restored to previous value. Repeated with verified European ROM: extracted successfully. (German in-game screens were validated earlier with the same pack data.)
- **Review fixes (after the code review):** import from the in-game ESC menu with the game in fullscreen (borderless): prompt, system file dialog in front of the game, in-game result prompt "Français importé"; import time 752 ms. With the stored USA image hidden, selecting Deutsch shows "Amerikanisches ROM erforderlich" and restores the previous language. pt-BR start cost after the refactor: 263 ms (`applied ... in N ms` log). Measured with a harness: bundle build + rewrite ~0.32 s, importer ~0.49 s; deflate level 9 → 1 changed these by ~1–3%.
- **Save Integrity:** one test chose "SAVE AND QUIT" in the pt-BR pause menu, which writes the save before asking; the original save was restored from a backup and verified by SHA-1. No other test wrote saves.

### 6.3 Known Limitations & Open Items

1. **Linux Validation:** Linux host runtime validation is pending execution by user on a native Linux installation.
2. **Fullscreen file dialog:** validated with the game's (borderless) fullscreen on Windows; the dialog has no explicit parent window (recompui's NFD call passes none), so other window managers were not checked.