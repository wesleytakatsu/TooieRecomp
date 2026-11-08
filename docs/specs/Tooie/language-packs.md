---
title: "TooieRecomp Spec — In-Game Language Bundles and Packs"
description: "Built-in pt-BR/FR/DE/ES/IT bundles, procedural IA8 fonts, graphic deltas and USA-ROM rewriting."
status: current
area: "specs/Tooie"
updated: 2026-11-08
tags: [specs, tooierecomp, localization, language, tlp, tlpb, text, font]
---

# In-Game Language Bundles and Packs

## 1. Selection and installation

General > Game Language offers English, Português (Brasil), Français, Deutsch, Español and Italiano (bozza).
The selected value persists as `language` in `general.json`; it applies when the game starts.
Changing it during gameplay requires restarting the game. English uses the original USA assets.

The other five languages load files next to the executable:

| Language | Config value | Bundle |
| --- | --- | --- |
| Brazilian Portuguese | `PortugueseBrazil` | `assets/languages/pt-BR.tlpb` |
| French | `French` | `assets/languages/fr.tlpb` |
| German | `German` | `assets/languages/de.tlpb` |
| Spanish | `Spanish` | `assets/languages/es.tlpb` |
| Italian draft | `Italian` | `assets/languages/it.tlpb` |

The player needs only the verified USA ROM. No European-ROM selection or extraction dialog is offered by Game Language.
CMake's `TooieUIAssets` synchronizes the entire assets directory, including these bundles, on app builds.

The pt-BR file is the existing translation bundle, unchanged. FR/DE/ES files contain official localized text extracted
from the user's verified European ROM. They are proprietary localization data, not covered by the host-code licence;
they contain no ROM image or font artwork. See [[asset-and-licensing-manifest]] for provenance and the distribution boundary.
Italian comes from the external Rulesless v0.9 USA patch plus `translations/it.json`, a machine-assisted supplemental
draft with project terminology and manual corrections. Editorial and in-game review remain pending. The verified
European text bank contains only EN/FR/DE/ES; Italian is not a hidden fifth section. Attribution is recorded in
`assets/languages/NOTICE-Italian.txt`.

## 2. Loading and compatibility

At ROM load, `src/game/language_pack.cpp`:
1. Resets the glyph mapping to USA behavior.
2. Selects the configured bundle and validates its language tag.
3. Builds replacement text/graphic assets and an IA8 font in memory from the bundle plus the USA ROM.
4. Relocates the affected asset tail and updates its cart-table offsets in memory.
5. Installs the rewritten ROM buffer and European character mapping.

A present bundle takes precedence over an older imported `languages/<tag>.tlp` in the user's app folder.
If a FR/DE/ES bundle is absent, the loader can still use that legacy pack. It converts any IA4 font-1 payload to IA8
in memory, without rewriting the user's file. A corrupt present bundle is an error, not an excuse to silently load a stale pack.

If files are missing or invalid when selecting a language, the previous usable setting is restored and a repair prompt
asks for the supplied assets/languages files. If validation fails at boot, the unchanged USA image starts in English.
The native C++ EUR importer remains available to developer tests and tools; the settings UI no longer invokes it.

`TOOIE_TEST_LANGUAGE_PACK=<path>` accepts either TLPK or TLPB by magic, bypasses configured language selection and
suppresses boot-error UI. This diagnostic is not a player installation requirement.

## 3. Fonts

Font 0 (`0x0C21`) is shared between USA and unmodified EUR. The pt-BR bundle applies its existing 37 patch runs
(209 bytes) after size/CRC32 verification. FR/DE/ES/IT bundles have zero font-0 patches.

Font 1 (`0x0C22`) is synthesized from the player's 51-frame USA IA8 font. Eight transparent rows are prepended;
23 frames are added using procedural accent masks, rotated punctuation and a repositioned apostrophe for the comma.
All resulting fonts contain 74 frames in IA8 (`0x100`), which the USA text renderers support.

| Profile | Additional frames 51–73 |
| --- | --- |
| pt-BR | Ã Á À É È Í Ì Ç Õ Ó Ò Ü Ú Ù ¿ ¡ ª ° Ê Ï Ô Â comma |
| European (FR/DE/ES/IT) | Ä Á À É È Í Ì Ñ Ö Ó Ò Ü Ú Ù ¿ ¡ ª ° Ï Ê Û Ô comma |

These four languages share the procedural profile, not copies of proprietary font artwork. Italian maps native
font-0 code `0x5F` (À) to generated frame 53 (À), rather than the European table's frame 52 (Á).
The legacy IA4 decoder expands I3/A1 to I4/A4, preserves glyph dimensions/order and rebuilds pixel offsets on serialization.
Changing the sprite flag alone is insufficient.

`func_800D35D0` retains original USA mapping for English. Active bundles/packs supply a 73-byte charset and 30-byte
accent map. Font 1 maps comma to frame 73, escape codes `0x5B..0x78` through the accent map and other characters
through the charset. Do not uppercase these escape codes or reuse pt-BR frame identities for European languages.
See [[../../reverse-engineering/systems/text-and-fonts]].

## 4. Containers

### TLPK v1 (.tlp)

Little-endian header: magic `TLPK`, u32 version 1, NUL-padded char[16] language tag,
u32 USA image size (35098224), u32 entry count, u32 charset length, charset bytes,
then 30 accent-map bytes when the charset is nonempty.

Entries are ordered by asset ID: u32 ID, u32 payload size, payload.
Payloads retain cartridge encoding: big-endian u16 ceil(raw_size/16), raw deflate, then 0xAA padding to an 8-byte boundary.
Parsing/serialization and in-memory relocation live in `include/tooie_language_pack.h`.

### TLPB v1 (.tlpb)

Little-endian header, in actual serialization order:

1. Magic `TLPB`, u32 version 1, NUL-padded char[16] language tag.
2. u32 charset length (73), charset bytes, 30 accent-map bytes.
3. u32 USA font-0 size, u32 font-0 CRC32, u32 patch-run count.
4. For each patch: u32 offset, u32 size, patch bytes.
5. u32 text-entry count.
6. Ordered entries: u32 asset ID, u8 kind, u32 data length, data bytes.

Kind 0 contains a selected text section, without the USA three-byte container header.
Kind 1 contains keep/replace flags and replacement text for the USA section's text records; original control records and
unchanged records are read from the player's ROM. The bundle language tag selects the procedural font profile.
The existing pt-BR v1 bundle stays compatible. Unsupported tags, unordered IDs and trailing/truncated data are rejected.

### TLPB v2

Same header and font section as v1, with version 2. Before the text-entry count, v2 inserts a u32 graphic-entry count.
Each entry has six u32 fields: asset ID, USA decoded size, USA CRC32, target decoded size, target CRC32, patch-run count.
Each run has u32 offset, u32 length and replacement bytes. IDs must increase; runs must be ordered, disjoint and in bounds.
Text IDs and font IDs `0xC20..0xC23` are forbidden here. Decoded sizes are bounded by `65535 * 16`.
The builder validates the USA asset, resizes it to the declared target size, applies runs and verifies the target CRC32.
The resulting pack is sorted before relocation. The Italian bundle stores localized graphic differences this way,
without embedding a ROM or complete original graphics.
Italian preserves the complete USA models `0x632` (Rareware) and `0x8AC` (Dolby). Title model `0x8A8` uses the Italian
art from Rulesless and a distinct identity for every tile: one least-significant red bit is toggled in an alpha-zero
texel per tile. Alpha and all texels with nonzero alpha stay unchanged. This gives all 28 Italian title textures their
own RT64 hashes, including those whose visible art is shared with USA. Source translation credits remain in
`NOTICE-Italian.txt`; they do not replace Rareware or Dolby.

## 5. Developer extraction and coverage

The developer supplies the verified EUR ROM (33554432 bytes, SHA-1
`93bf2fac1387320ad07251cb4b64fd36bac1d7a6`) and decompressed USA image (35098224 bytes,
SHA-1 `af479771ff5e91ce3732f3d84146614ab71d8d79`):

```sh
python tools/build_language_packs.py --pal-rom "<European ROM>" --usa-image "<USA image>" \
  --out-dir "<private reference packs directory>" --bundle-dir assets/languages --report "<report.json>"
```

The generator rebuilds every bundle text entry and compares it to its reference TLPK payload before reporting PASS.
It does not modify the ROMs. Optional `--ptbr-bps <patch>` rebuilds pt-BR at `--ptbr-bundle <path>`;
without that argument the existing pt-BR file is untouched.

Conversion preserves USA group/control structure:
- Rule 1: identical record structure; select the translated section.
- Rule 2: align single-group menu records by matching EUR English to USA text.
- Rule 3: keep USA record types/parameters where compatible text records differ.
- Preserve English when these rules cannot establish compatibility.

| Language | Rule 1 | Rule 2 | Rule 3 | Text entries | Total runtime entries |
| --- | ---: | ---: | ---: | ---: | ---: |
| FR | 2227 | 5 | 1 | 2233 | 2234 |
| DE | 2225 | 4 | 1 | 2230 | 2231 |
| ES | 2229 | 4 | 1 | 2234 | 2235 |
| pt-BR | 2230 | 3 | 1 | 2234 | 2236 |

FR `0x15CB` has placeholder data. DE `0x0FB5/0x14F7/0x14F8/0x1809` have divergent controls.
USA-only `0x1A89` and unmatched credit records remain English in the FR/DE/ES conversion. That conversion does not
invent translations or change event controls to force those assets through. Its scope excludes localized textures/audio;
Italian graphic deltas and supplemental text are described below.

The title-screen pt-BR legal line remains fixed by `PTBR_TEXT_FIXES` in the generator to fit the USA layout.

### Italian extraction and completion

```sh
python tools/build_italian_bundle.py --usa-rom "<USA retail ROM>" --usa-image "<USA image>" \
  --bps "<Rulesless v0.9 BPS>" --translations translations/it.json \
  --reference "<private it-reference.tlp>" --report "<private report.json>"
```

The source patch is pinned to SHA-256 `1789ade8728408b9c382c5964c436411b874513409e2ed2a29b7079fc4f728f5`.
Only complete deflate streams inside the original asset slots are accepted. Valid streams with inaccurate size headers
are re-encoded; truncated streams and incompatible layouts are rejected. Graphic raw sizes must match the USA asset.
The tool never imports patched code or tables. Unusable graphics retain USA data; text in rejected assets is translated
from the USA records by the supplemental catalog instead.
Rareware/Dolby exclusions are listed separately as `usa_branding_preserved`; `italian_hd_title` records title identity texels.

### Italian HD title artwork

The optional private `Tooie-Italian-Title-HD.rtz` contains 28 PNG tiles at 256×256, forming a 2048×1024 Italian title.
The complete artwork is prepared as one transparent image, then split using the model's actual display-list vertex
loads. Texture index order is not spatial order, and two corner tiles use triangles rather than quads.
All 28 Italian hashes must be distinct and absent from the supplied reference HD database; the tool first reproduces
all 28 USA title hashes against that database using the vendored RT64 v5 CPU hasher. This prevents the Italian artwork
from replacing the title in other configured languages, even when the mod remains enabled.

```sh
cmake --build build-win --target texture_hasher
python tools/build_italian_title_hd.py --usa-image "<USA image>" --artwork "<private title-source.png>" \
  --texture-hasher build-win/rt64/src/tools/texture_hasher/texture_hasher.exe \
  --reference-pack "<existing RT64 HD pack>" --output "<private Italian Title HD.rtz>" \
  --report "<private report.json>" --preview "<private title-preview.png>"
```

The normal bundle builder must run first. The HD builder uses Pillow for technical resizing, UV-aligned slicing and
PNG encoding; it does not generate artwork or contact a translation/image service. The source artwork was adapted
with the built-in `image_gen` tool. The resulting pack has `enabled_by_default=true`, so the existing mod scanner enables
it on first discovery; later user toggles remain authoritative. With the HD mod disabled, the native Italian title remains.
The full PNG and RTZ stay private, outside git, with their hashes, source prompt and provenance. Normal builds require
no external HD pack or artwork generator. They synchronize the updated language bundle as before.

The catalog uses SHA-256 of the original record text (before its first NUL), independent of translated byte offsets.
It covers every nonempty record still identical to USA after compatible source imports. Entries explicitly translate
or preserve names, credits, numeric content, functional codes and intentionally fictional speech. Original title-screen
legal notices remain intact.
Native record types, parameters and control records are unchanged. Button glyphs and `~` substitutions are checked
as multisets; supplemental title newlines/`#` markers must match the original. All encoded records must fit the native
255-byte limit. Missing or unused catalog entries fail generation.
No translation service is used by the builder or game; generation with the checked-in catalog is offline.
Omitting `--translations` produces the compatible partial source and a private catalog of the remainder in the report.

## 6. Validation

Windows host regressions cover:
- TLPK parsing, asset relocation and glyph mapping.
- TLPB parsing for all five supported tags, unsupported/truncated input and text overlays.
- V2 graphic-delta bounds, source/target CRC32 checks, asset ordering and rewritten-image extraction parity.
- Italian native accents, button/substitution preservation and record-size rejection.
- European/pt-BR frame identities, punctuation and all 16 IA4 intensity/alpha combinations.
- Legacy font normalization, idempotence and malformed payload rejection.
- Real FR/DE/ES bundle text parity against developer reference packs and 74 generated IA8 glyphs.

The full Windows required suite passed 34/34 on 2026-11-08; app build passed.
Runtime observations and remaining platform/manual checks are recorded in [[../../HANDOFF]].
Linux/PortMaster runtime validation of this change remains NOT_RUN.
