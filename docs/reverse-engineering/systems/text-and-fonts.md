---
title: "Text and Font System"
description: "Reverse-engineering model of Banjo-Tooie asset table offsets, multi-language text container formats, dialog parsing grammars, font descriptors, and character-to-glyph mapping."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-10-27
tags: [reverse-engineering, text, fonts, dialog, assets]
---

# Text and Font System

This page documents the data structures, asset formats, and routines governing text storage, dialog parsing, font sprites, and glyph mapping in *Banjo-Tooie*.

For the implementation spec governing language pack generation and loading in TooieRecomp, see [[specs/Tooie/language-packs]].

---

## 1. Asset Table Structure (`PROVEN`)

Assets in *Banjo-Tooie* are indexed by an entry table in the ROM cartridge:

- **Entry Format:** Big-endian 32-bit word encoded as `(offset / 4) << 8 | flags`.
- **Size:** 13,926 assets plus a terminating sentinel entry.
- **USA Decompressed Image (`bt.n64.us.1.0.z64`):**
  - Table start: offset `0x5188`.
  - Payload base: offset `0x12B24` (`= 0x5188 + 13927 * 4`).
- **European ROM (`Banjo-Tooie (Europe) (En,Fr,De,Es).z64`):**
  - Table start: offset `0x5148`.
  - Payload base: offset `0x12AE4` (`= 0x5148 + 13927 * 4`).
- **Equivalence:** Both builds define identical 13,926 asset IDs; table flags match across 13,887 assets (`PROVEN`).
- **Storage Layout:** Each compressed payload starts with big-endian `uint16_t` representing `ceil(uncompressed_size / 16)`, followed by raw deflate data (`wbits = -15`), padded with `0xAA` bytes to an 8-byte boundary.

---

## 2. Text IDs and Asset Resolution (`PROVEN`)

- **1:1 Mapping:** Dialog and text IDs correspond directly to asset IDs without an index offset:
  `gcdialogexec` -> `gldialog_entrypoint_4` -> `func_800D674C(asset_id)`.
- **Text Asset Range:** Asset IDs `0x0CDA` through `0x1C6B` are text assets.
- **Table Flag:** Text assets carry asset table flag `0x13`.

---

## 3. Multi-Language Asset Container Layout (`PROVEN`)

Text asset payloads differ in container header between regional releases:

- **USA Format (`01 03 00`):**
  - `uint8_t language_count`: `0x01` (single language).
  - `uint16_t section_offset`: `0x0003` (little-endian, pointing immediately to the English section).
  - Single text section payload.
- **European Format (`04 ...`):**
  - `uint8_t language_count`: `0x04` (four languages: English, French, German, Spanish).
  - Four `uint16_t` little-endian section offsets pointing to the respective language sections within the asset.
- **Section Parity:** Across 2,251 shared text asset IDs between USA and European ROMs, 2,203 contain a byte-identical English section (`PROVEN`).

---

## 4. Text Section Grammar and Parsing (`PROVEN`)

A text section contains sequential groups from offset start to the end of the section:

```text
Section:
  [Group 0]
  [Group 1]
  ...
  [Group N]

Group:
  u8 record_count
  [Record 0]
  ...
  [Record M]

Record:
  u8 type
  u8 param   (present ONLY if type == 0x03, specifies speaker ID)
  u8 length
  u8 data[length]
```

- **Text Records:**
  - `type == 0x03`: Dialog text record with a speaker parameter byte (`u8 param`), followed by length and text character data.
  - `type >= 0x80`: Menu and inline text records (no speaker parameter byte).
- **Control Records:**
  - `type` in `{ 0x02, 0x04, 0x05, 0x07, 0x08, 0x09, 0x0B }`. Carry formatting, pauses, choices, or audio/camera triggers.
- **Validation:** All 9,052 European language sections across the four PAL languages parse consistently under this grammar without remainder or overrun (`PROVEN`).

---

## 5. Font Descriptors and Sprites (`PROVEN`)

Font descriptors are configured at RAM address `0x8011B560` (8 bytes per descriptor):

| Font Index | Target Asset | Primary Usage | Regional Differences |
| :--- | :--- | :--- | :--- |
| **Font 0** | Asset `0x0C21` | Dialog boxes, file selection menu | 88 frames (IA8 sprite); byte-identical between USA and European ROMs (`PROVEN`). |
| **Font 1** | Asset `0x0C22` | Titles, pause menu, banners | Differs: USA contains 51 frames (IA8 sprite, height 23); European contains 74 frames (IA8 sprite, height 31, with 8 transparent rows on top, adding accented capital letters in frames 51–73) (`PROVEN`). |
| **Font 2** | Asset `0x0C20` | Secondary UI indicators | Identical between USA and European ROMs (`PROVEN`). |
| **Font 3** | Asset `0x0C23` | Counters, numerical digits | Identical between USA and European ROMs (`PROVEN`). |

### 5.1 pt-BR Translation Font Modifications & Divergent Code Semantics (`PROVEN`)

In the Brazilian Portuguese translation data:
- **Font 0 Modifications:** Asset `0x0C21` undergoes 37 localized byte runs (totalling 209 bytes) modifying glyph frames (such as Ã and Õ) over the baseline USA font 0 (`PROVEN`).
- **Font 1 Frame Order:** The pt-BR European Font 1 frames 51–73 correspond sequentially to: Ã Á À É È Í Ì Ç Õ Ó Ò Ü Ú Ù ¿ ¡ ª ° Ê Ï Ô Â and the comma (`PROVEN`).
- **Divergent Character Code Semantics:** The pt-BR text scripts employ distinct character code meanings between Font 0 and Font 1 (`PROVEN`):
  - In Font 1, character code `i` maps to Â (observed in place names) (`PROVEN`).
  - In Font 1, character code `h` displays a comma (`PROVEN`).
  - In Font 1, character codes `*` and `+` display nothing, whereas in Font 0 they render Ã and Õ (`PROVEN`).
- **Visual Parity:** Reproducing the exact pt-BR Font 1 frame identities ensures that displayed dialogs and banners match the translation's original European build (`PROVEN`).

### 5.2 Procedural Font 1 Synthesis in TooieRecomp (`PROVEN`)

To preserve clean-room licensing without distributing proprietary font artwork, TooieRecomp synthesizes pt-BR Font 1 at runtime (`include/tooie_font_sprite.h`):
1. **Base Frame Padding:** The 51 USA Font 1 frames (`0x0C22`, height 23) are padded to height 31 by inserting 8 transparent rows on top, matching the European baseline height.
2. **Procedural Accented Glyph Synthesis:** 23 additional frames (frames 51–73) are generated procedurally in the pt-BR frame order:
   - Accented letters (Ã, Á, À, É, È, Í, Ì, Ç, Õ, Ó, Ò, Ü, Ú, Ù, Ê, Ï, Ô, Â, ª, °) are constructed using custom accent masks rendered with the font's fill and outline color parameters.
   - ¿ and ¡ are generated by rotating the USA ? and ! frames.
   - The comma is synthesized by repositioning the USA apostrophe glyph downward.

---

## 6. Character to Glyph Mapping: `func_800D35D0` (`PROVEN`)

The function `func_800D35D0(font, c)` translates character codes to glyph indices for rendering.

### 6.1 USA Implementation
- **Case Folding:** Lowercase characters are folded to uppercase for fonts other than Font 0 and Font 2.
- **Font 1 Mapping:** Uses a 128-entry lookup table at RAM address `0x8012B370`. This table is populated at startup by `func_800D34A0` using a 51-character string located at `0x8011B580`.

### 6.2 European Implementation
- **No Case Folding:** Lowercase characters are preserved across all fonts.
- **Font 1 Mapping:**
  - Character `,` maps directly to glyph index `0x49`.
  - Accent escape character codes in the range `0x5B` through `0x78` map to `0x33 + accent_map[c - 0x5B]`.
  - All other character codes map through a 256-entry lookup table generated from a 73-character charset (the 51 base characters plus 22 Latin-1 accented uppercase characters).
- **Font 2 Mapping:**
  - Returns `-1` for character code `0xAD`.
  - Returns `c - 0x1A` for character codes `c >= 0xAF`.
- **Fonts 0 and 3:** Identical to USA behavior.

### 6.3 ROM Location of European Tables (`PROVEN`)
The European 73-character font-1 charset and 30-byte accent mapping table reside in the European ROM `core2` data segment:
- ROM offset: `0x1E8AD81` (stored as `uint16_t ceil(size / 16)` followed by raw deflate stream).
- Decompressed data offsets:
  - Charset (73 bytes): offset `0x3928`.
  - Accent map (30 bytes): offset `0x3974`.

---

## 7. TooieRecomp Host Implementation

TooieRecomp provides a host implementation of `func_800D35D0` in `src/game/text_font_map.cpp`:
- By default, it preserves faithful USA mapping behavior for standard English gameplay.
- When an active language pack (imported `.tlp` or built-in pt-BR bundle) provides a font-1 charset and accent map, the routine dynamically switches to the European mapping logic.
