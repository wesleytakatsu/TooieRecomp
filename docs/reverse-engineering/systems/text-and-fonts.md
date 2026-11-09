---
title: "Text and Font System"
description: "Reverse-engineering model of Banjo-Tooie asset table offsets, multi-language text container formats, dialog parsing grammars, font descriptors, and character-to-glyph mapping."
status: current
type: reverse-engineering
project: TooieRecomp
updated: 2026-11-08
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
| **Font 1** | Asset `0x0C22` | Titles, pause menu, banners | USA: 51 frames, IA8 (`0x100`), height 23. EUR: 74 frames, IA4 (`0x80`), height 31, including accented capitals and comma. The prior IA8 classification of EUR is `CORRECTED/SUPERSEDED`; format and per-frame pixel lengths were verified from the canonical ROM. |
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

### 5.3 USA text rendering and European font compatibility (`PROVEN`)

The canonical EUR `0x0C22` sprite contains 74 valid frames and 26,288 bytes of IA4 pixels.
Its additional frames 51–73 are Ä Á À É È Í Ì Ñ Ö Ó Ò Ü Ú Ù ¿ ¡ ª ° Ï Ê Û Ô and comma.
All 2,263 EUR text assets contain four sections (9,052 sections total), EN/FR/DE/ES; no fifth language section
was found in this bank. A complete hidden Italian translation is not supported by these primary data.

USA text renderers `func_800B62CC` and `func_800B81CC` dispatch texture formats without a `0x80` branch.
The first recognizes `0x800/0x200/0x100/0x40/0x20/0x4`; the second recognizes
`0x800/0x200/0x100/0x40/0x20`. Importing the EUR IA4 sprite without adaptation therefore skips its texture-load
path. This is distinct from a missing asset or a bad character-to-frame map.

TooieRecomp generates European-profile IA8 from the USA font for built-in FR/DE/ES bundles. Legacy imported
IA4 fonts are expanded to IA8 (I3/A1 to I4/A4) in memory with rebuilt offsets. Generated C is not edited.

The external Rulesless v0.9 Italian patch targets USA retail, not EUR (`PROVEN` from BPS source CRC32 and asset layout).
Its compatible text and graphic deltas are imported by the host bundle builder; they do not prove a native Italian bank.
Supplemental Italian uses USA font-0 accent codes: `0x5F` À, `0x62` É, `0x63` È, `0x6C` Ì, `0x6F` Ò, `0x71` Ù
(`PROVEN` from the decoded font atlas). Its font-1 map directs `0x5F` to the European-profile À frame, and is an explicit
project adaptation rather than a claim about the original European table.

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
- When an active language pack or bundle provides a font-1 charset and accent map, the routine dynamically switches to the European mapping logic.

## 8. Japanese release (`PROVEN` unless qualified)

- Japan normalized SHA-1: `5a5172383037d171f121790959962703be1f373c`, 33554432 bytes. The supplied `.n64` file is
  16-bit byte-swapped (`37 80 40 12`), despite its extension. Table/base are `0x5148/0x12AE4`, 13926 assets.
- Native font 0 (`0xC21`) is byte-identical to USA: 88 IA8 frames. Japan adds font 2 (`0xC20`): 230 I4 (`0x20`)
  frames, nominal 16×16; this slot is empty in the verified USA image. Japan font 1 (`0xC22`) has 178 IA4 (`0x80`)
  frames at height 24. The scripts use hiragana/katakana, Latin symbols and a proprietary one-byte mapping.
- Native font-2 punctuation frames 55..66 are apostrophe, `*`, `+`, comma, `ー`, period, slash, colon, equals,
  question mark, at sign and `。`, respectively (`PROVEN` by the decoded atlas and corresponding source records).
  Omitting frame 55 in the host punctuation list shifted later symbols: `ゲーム` appeared as `ゲ.ム`,
  `キックボール` lost its long-vowel mark, and a question could end in an A-shaped symbol (`CORRECTED`).
  The authored UI encoder also uses frame 59 for `ー` and 66 for `。`.
- Boot instructions identify compressed core1 at `0x1DD2DE0` and core2 at `0x1DEB330`. Sequential raw-deflate
  decoding locates core2 data at `0x1E2DBA2`. Its 256-byte font-1 lookup is at decompressed offset `0x3934`;
  entries are unsigned bytes, reaching frame 177. It maps native numeric codes `0x10..0x19` to frames `7..16`.
- The native Japan glyph mapper does not fold font-1 lowercase as USA does. Font 2 excludes `AD`, maps codes
  `>= AF` to `c - 0x1A`, and otherwise returns `c`. Font 0 retains the USA mapping.
- Text records commonly begin `AD 6A`, selecting the Japanese font path. Numeric/text substitutions use `AE`,
  while dialog glyph `0x0D` is a literal wave dash. Treating the latter as USA `~` corrupts substitution counts.
  Shared scripts show 227 USA substitutions; three release-dependent messages require explicit adaptation.
- All 2251 shared text assets have matching group counts, but only 490 have matching per-group record counts.
  2222 have identical non-text control streams; the other 29 mostly differ in wait records. This is a regional
  script-layout difference, not evidence that the Japanese executable is binary-compatible with USA.
- Japan `0x186C` retains legacy English Dolby lines and adds unused `TEXT 26.OCT`/`TEST*` records. Its copyright
  row is indexed differently. Blindly importing those rows into USA displays debug strings. The host preserves
  USA legal-row identities and the font-0 `x` copyright glyph.
- USA-only `0x1A89` is a nonempty four-field multiplayer quiz menu (`ONE GETS TONNED!`, choice text and title),
  not an empty placeholder. Japan has no corresponding payload. The host supplies a Japanese translation while
  preserving its field IDs, newlines and `/N#` marker sequence.
- Title model `0x8A8` has 31 RGBA32 32×32 textures and a 2000×1000-unit canvas translated vertically by 13 units.
  Several tiles are submitted as two separate three-vertex loads. All 31 RT64 v5 hashes resolve in the user's
  BT Reloaded database to Japanese title artwork/shared trademark artwork. Rareware `0x632` and Dolby `0x8AC`
  are byte-identical to USA. These facts do not imply parity of other regional code or assets.

The host adapts the Japanese text into composite IA8 fonts and explicit per-font maps. This is a project encoding,
not a claim that the original Japanese ROM uses the host byte codes. Regional animations and incompatible model
geometry are not imported. Source hashes, selected asset IDs and original differences are recorded by the offline builder.

### Inline buttons and Japanese separators

- `PROVEN`: USA `func_80800E24_gczoombox` classifies bytes `0x7F <= c < 0x88` as inline buttons before normal
  glyph lookup. A custom font map cannot make those bytes safe for kana. Initial host allocation reused them;
  the user's file-select screenshot exposed Start/C icons replacing kana (`CORRECTED`).
- `PROVEN` from corresponding file-select/copy/delete/option fields: Japan `0x9D` is A and `0x9C` is B;
  the nine button codes `0x95..0x9D` correspond to USA `0x7F..0x87`. They require conversion independently of
  the glyph table. Treating `0x9D/0x9C` as glyph indices instead displays kana in place of A/B.
- `STRONG_INFERENCE`: Japan `0xAC` is a word separator, based on its recurring placement between complete words
  and before button prompts in corresponding regional records. The host renders it as a space, rather than the
  unrelated font-2 frame 172 (`す`). This is an adaptation, not proof of all native Japanese wrapping semantics.
- The corrected project namespace reserves all nine inline-button bytes plus NUL, newline, substitution and
  formatting commands. All 230 glyphs have distinct codes except the two native blank frames; extra nonzero
  low bytes supply the remaining capacity. This does not change the game's native button-rendering path.

### Repeated typed records (2026-11-09)

- `PROVEN` static code: Japan has a routine that collects every typed text record (`>= 0x80`) of a type into a
  list, while the USA lookup returns the first match (`RecompiledFuncs/funcs_204.c:2223`). Japan therefore splits
  one menu prompt, quiz question or notice into several same-type records where USA uses one.
- `PROVEN` static code: the USA zoombox path wraps at spaces and does not interpret 0x0A
  (`RecompiledFuncs/funcs_372.c:5438`); 0x186D has an explicit line splitter (`funcs_295.c:1035`).
- Japan byte 0x0A inside `AD 6A` text is a line break, not glyph 10 (`<`).
- Native glyph 78 is `ヲ`; 13 `〜`, 67 `゛`, 68 `゜`, 9 `″` (checked against the font-2 frames).
- Japan inserts localization staff credits, shifting the credits IDs (`0x1B68..0x1B90`) against USA.
- Host adaptation: same-type fragments are joined in order with a space (0x0A where the record type already
  supports it); credits keep USA text. Line breaks/clipping on screen: `PENDING_USER`.
