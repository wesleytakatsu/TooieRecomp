#!/usr/bin/env python3
"""Build a private Japanese language bundle from verified ROMs; never import regional code."""
from __future__ import annotations

import argparse
import difflib
import hashlib
import json
from pathlib import Path
import struct

import build_language_packs as packs

JAPAN_SHA1 = "5a5172383037d171f121790959962703be1f373c"
JAPAN_TABLE, JAPAN_BASE = 0x5148, 0x12AE4
FONT1_MAP_OFFSET = 0x3934
CORE2_DATA = 0x1E2DBA2
TITLE_ID = 0x8A8
USA_BUTTON_CODES = frozenset(range(0x7F, 0x88))
JAPAN_BUTTON_CODES = frozenset(range(0x95, 0x9E))
# USA staff credits and cast roll. Japan inserts four staff pages at 0x1B84.
CREDITS_IDS = range(0x1B68, 0x1B91)


def character_codes() -> dict[int, int]:
    codes = {i + 16: ord("0") + i for i in range(10)}
    codes.update({i + 26: ord("A") + i for i in range(26)})
    codes.update({0: 32, 15: 32, 2: ord("$"), 3: ord("("), 4: ord(")"), 6: ord("%"),
                  10: ord("<"), 11: ord(">"), 12: ord("&"), 52: ord("!"),
                  53: ord('"'), 54: ord("#"), 55: ord("'"), 56: ord("*"), 57: ord("+"), 58: ord(","),
                  60: ord("."), 61: ord("/"), 62: ord(":"), 63: ord("="), 64: ord("?"), 65: ord("@")})
    # gczoombox recognizes 7F..87 before glyph lookup. Keep kana outside that range.
    # Nonzero low bytes are glyphs too; retain 0A for authored menu newlines.
    available = iter(c for c in [*range(0x10, 0xFD), *range(1, 10)]
                     if not 0x20 <= c <= 0x5A and c not in USA_BUTTON_CODES
                     and c not in (ord("~"), ord("x")))
    for glyph in range(230):
        if glyph not in codes:
            codes[glyph] = next(available)
    return codes


KANA = ("ァィゥェォッャュョヲン" +
        "アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワ" +
        "ガギグゲゴザジズゼゾダヂヅデドバビブベボパピプペポ" +
        "ぁぃぅぇぉっゃゅょをん" +
        "あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわ" +
        "がぎぐげござじずぜぞだぢづでどばびぶべぼぱぴぷぺぽヴ")


def encode_ui(text: str, codes: dict[int, int]) -> bytes:
    glyphs = {character: index + 69 for index, character in enumerate(KANA)}
    # Frame 9 is the paired prime mark in the native font-2 atlas.
    glyphs.update({"、": 1, "・": 5, "「": 7, "」": 8, "″": 9, "〜": 13, "…": 14,
                   "ー": 59, "。": 66, "゛": 67, "゜": 68})
    output = bytearray()
    for character in text:
        if character in glyphs:
            output.append(codes[glyphs[character]])
        elif character in "？！":
            output.append(ord("?" if character == "？" else "!"))
        elif 32 <= ord(character) <= 90 or character in "~\n":
            output.append(ord(character))
        else:
            raise ValueError(f"unsupported authored Japanese character {character!r}")
    if len(output) + 1 > 255:
        raise ValueError("authored Japanese text exceeds native record limit")
    return bytes(output) + b"\0"


def japanese_rom(data: bytes) -> bytes:
    if len(data) != 33554432:
        raise ValueError("Japanese ROM must contain 33554432 bytes")
    if data[:4] == bytes.fromhex("37804012"):
        out = bytearray(data)
        out[0::2], out[1::2] = data[1::2], data[0::2]
        data = bytes(out)
    elif data[:4] == bytes.fromhex("40123780"):
        out = bytearray(data)
        for i in range(4):
            out[i::4] = data[3 - i::4]
        data = bytes(out)
    if data[:4] != bytes.fromhex("80371240") or hashlib.sha1(data).hexdigest() != JAPAN_SHA1:
        raise ValueError("unsupported Japanese ROM SHA-1")
    return data


def read_sprite(raw: bytes) -> list[tuple[int, int, int, bytes]]:
    if len(raw) < 12:
        raise ValueError("truncated font sprite")
    count, fmt = struct.unpack_from(">HH", raw)
    base = struct.unpack_from(">I", raw, 8)[0]
    if fmt not in (0x20, 0x80, 0x100) or base != (12 + count * 8 + 7) // 8 * 8:
        raise ValueError("unsupported Japanese font format")
    result = []
    for i in range(count):
        width, height, advance, other, offset = struct.unpack_from(">4BI", raw, 12 + i * 8)
        size = width * height if fmt == 0x100 else width * height // 2
        if not width or width % 8 or advance > width or other != height or base + offset + size > len(raw):
            raise ValueError("invalid font frame")
        pixels = raw[base + offset:base + offset + size]
        if fmt != 0x100:
            unpacked = []
            for byte in pixels:
                for value in (byte >> 4, byte & 15):
                    if fmt == 0x20:
                        unpacked.append(value * 17)  # I4: intensity and alpha have the same value.
                    else:
                        intensity = value >> 1
                        unpacked.append(((intensity << 1 | intensity >> 2) << 4) | (15 if value & 1 else 0))
            pixels = bytes(unpacked)
        result.append((width, height, advance, pixels))
    return result


def sprite_bytes(frames: list[tuple[int, int, int, bytes]]) -> bytes:
    base = (12 + len(frames) * 8 + 7) // 8 * 8
    out = bytearray(base)
    struct.pack_into(">4HI", out, 0, len(frames), 0x100, max(f[0] for f in frames), max(f[1] for f in frames), base)
    for i, (width, height, advance, pixels) in enumerate(frames):
        if len(pixels) != width * height:
            raise ValueError("invalid normalized frame")
        struct.pack_into(">4BI", out, 12 + i * 8, width, height, advance, height, len(out) - base)
        out.extend(pixels)
    return bytes(out)


def fallback_title_glyph(frame: tuple[int, int, int, bytes]) -> tuple[int, int, int, bytes]:
    """Supply an outlined 24-pixel glyph when the original title font omits that kana."""
    width, height, advance, source = frame
    target_width = ((width * 3 // 2 + 7) // 8) * 8
    target_height = height * 3 // 2
    pixels = bytearray(target_width * target_height)
    filled = []
    for y in range(target_height):
        for x in range(width * 3 // 2):
            value = source[(y * height // target_height) * width + x * 2 // 3]
            if value & 15:
                filled.append((x, y, (value & 0xF0) | 15))
    for x, y, _ in filled:
        for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            if 0 <= x + dx < target_width and 0 <= y + dy < target_height:
                pixels[(y + dy) * target_width + x + dx] = 15
    for x, y, value in filled:
        pixels[y * target_width + x] = value
    return target_width, target_height, min(target_width, advance * 3 // 2), bytes(pixels)


def font_profile(japan: bytes, usa: bytes) -> tuple[bytes, dict[int, int], dict[int, bytes], dict]:
    native = read_sprite(packs.asset(japan, JAPAN_TABLE, JAPAN_BASE, 0xC20))
    titles = read_sprite(packs.asset(japan, JAPAN_TABLE, JAPAN_BASE, 0xC22))
    if len(native) != 230 or len(titles) != 178:
        raise ValueError("unexpected Japanese glyph inventory")
    native_map = packs.inflate(japan[CORE2_DATA:])[FONT1_MAP_OFFSET:FONT1_MAP_OFFSET + 256]
    if len(native_map) != 256 or list(native_map[16:26]) != list(range(7, 17)) or max(native_map) != 177:
        raise ValueError("invalid Japanese title glyph map")
    fonts = [read_sprite(packs.asset(usa, packs.USA_TABLE, packs.USA_BASE, i)) for i in (0xC21, 0xC22)]
    maps = [[-1] * 256 for _ in range(2)]
    for code in range(0x21, 0x7B):
        if code - 0x21 < len(fonts[0]):
            maps[0][code] = code - 0x21
    charset = packs.CHARSET_PREFIX + b"'"
    for index, code in enumerate(charset):
        maps[1][code] = index
    # The USA title-screen copyright uses font-0 code 'x'; retain it for native formatted/legal strings.
    maps[1][ord("x")] = maps[1][ord("X")]
    codes = character_codes()
    extra = [glyph for glyph, code in codes.items() if code != 32 and not 0x21 <= code <= 0x5A]
    fallback = []
    for glyph, code in codes.items():
        if code in (0, 32, ord("~"), ord("#")):
            continue
        # Preserve standard Latin glyphs for strings formatted by the USA executable.
        for font in (0, 1):
            if glyph not in extra and maps[font][code] >= 0:
                continue
            if font == 0:
                frame = native[glyph]
            else:
                original_code = glyph + 26 if glyph >= 149 else glyph
                title_index = native_map[original_code]
                if title_index or glyph == 1:
                    frame = titles[title_index]
                else:
                    frame = fallback_title_glyph(native[glyph])
                    fallback.append(glyph)
            maps[font][code] = len(fonts[font])
            fonts[font].append(frame)
    for table in maps:
        for c in (0, 32, 0xFD, 0xFE, 0xFF):
            table[c] = -1
    raw_maps = struct.pack("<512h", *(maps[0] + maps[1]))
    assets = {0xC21 + index: sprite_bytes(frames) for index, frames in enumerate(fonts)}
    return raw_maps, codes, assets, {"dialog_frames": len(fonts[0]), "title_frames": len(fonts[1]),
                                    "native_dialog_frames": 230, "native_title_frames": 178,
                                    "fallback_title_glyphs": fallback, "code_to_native_glyph": {str(v): k for k, v in codes.items()}}


def convert_text(data: bytes, codes: dict[int, int]) -> bytes:
    # Japanese text uses AD 'j' to select its encoded font. The host profile supplies those glyphs directly.
    output = bytearray()
    japanese = False
    position = 0
    while position < len(data):
        if data[position:position + 2] == b"\xADj":
            japanese = True
            position += 2
            continue
        code = data[position]
        position += 1
        if code in (0, 0x0A) or not japanese:
            output.append(code)
            continue
        if code == 0xAD:
            raise ValueError("unsupported Japanese font command")
        if code == 0xAE:
            output.append(ord("~"))  # Native Japanese numeric/text substitution marker.
            continue
        if code in JAPAN_BUTTON_CODES:
            output.append(code - 0x16)
            continue
        if code == 0xAC:
            output.append(0x20)  # Japanese word separator, handled before glyph lookup.
            continue
        if 0x9E <= code < 0xAC:
            raise ValueError("unsupported Japanese inline control")
        glyph = code - 26 if code >= 0xAF else code
        if glyph not in codes:
            raise ValueError("Japanese glyph exceeds native font")
        output.append(codes[glyph])
    if len(output) > 255:
        raise ValueError("converted text exceeds native record limit")
    return bytes(output)


def align_controls(usa: list, japan: list) -> list:
    """Keep Japanese text rows while retaining the USA event/control stream verbatim."""
    original_controls = [record for record in usa if not packs.is_text(record)]
    regional_controls = [record for record in japan if not packs.is_text(record)]
    matcher = difflib.SequenceMatcher(a=regional_controls, b=original_controls, autojunk=False)
    before = {i: [] for i in range(len(regional_controls) + 1)}
    replacements = {}
    for kind, a, b, c, d in matcher.get_opcodes():
        if kind == "equal":
            for x, y in zip(range(a, b), range(c, d)):
                replacements[x] = [original_controls[y]]
        elif kind == "insert":
            before[a].extend(original_controls[c:d])
        else:
            replacements[a] = original_controls[c:d]
            for x in range(a + 1, b):
                replacements[x] = []
    result = []
    index = 0
    for record in japan:
        if packs.is_text(record):
            result.append(record)
        else:
            result.extend(before[index])
            result.extend(replacements.get(index, []))
            index += 1
    result.extend(before[index])
    assert [r for r in result if not packs.is_text(r)] == original_controls
    return result


def join_typed_rows(rows: list[bytes], codes: dict[int, int], separator: bytes,
                    prose: bool = True) -> tuple[bytes, list[str]]:
    """Join line fragments using native glyph identities at space boundaries.

    Katakana words split across lines are rejoined only in prose fields (quiz questions,
    prompts). Menu labels keep a space between separate katakana terms."""
    fragments = [row.removesuffix(b"\0") for row in rows]
    if separator == b"\n":
        return separator.join(fragments) + b"\0", ["\n"] * (len(rows) - 1)
    characters = {codes[69 + index]: character for index, character in enumerate(KANA)}
    characters.update({codes[5]: "・", codes[7]: "「", codes[8]: "」", codes[59]: "ー"})
    katakana = set(KANA[:KANA.index("ぁ")]) | {"ー", "ヴ"}
    text = fragments[0].rstrip(b" ")
    decoded = "".join(characters.get(code, "\ufffd") for code in text)
    separators = []
    for fragment in fragments[1:]:
        fragment = fragment.strip(b" ")
        right = "".join(characters.get(code, "\ufffd") for code in fragment)
        no_space = (decoded.endswith("・") or
                    bool(prose and decoded and right and decoded[-1] in katakana and right[0] in katakana) or
                    decoded.count("「") > decoded.count("」"))
        boundary = "" if no_space else " "
        separators.append(boundary)
        text += boundary.encode("ascii") + fragment
        decoded += boundary + right
    return text + b"\0", separators


def select_typed_group(index: int, group_index: int, usa: list, japan: list,
                       codes: dict[int, int], merges: list | None = None,
                       kept_usa_fields: list | None = None) -> list:
    """Resolve USA field IDs without discarding Japan's repeated text records."""
    sources = {}
    if index == 0x18E1:
        # USA A3 is the one-controller warning; Japan A3 requires TWO controllers.
        sources = {0xA3: 0xAB}
    elif index == 0x18E2:
        # Japan inserts a blank field at 83, shifts the prompts, and reverses ON/OFF.
        # USA settings enumerate exactly three options (funcs_197.c:2009-2048);
        # funcs_141.c:2183-2200 selects fields 0A + option * 5 + value. There is
        # no language option 3. Japan supplies blank 99; missing 9A..9D keep USA.
        sources = {0x83: 0x84, 0x84: 0x85, 0x85: 0x83, 0x86: 0x87,
                   0x87: 0x88, 0x8F: 0x90, 0x90: 0x8F}

    by_type = {}
    for kind, param, text in japan:
        if not packs.is_text((kind, param, text)):
            raise ValueError(f"{index:#06x} group {group_index}: unexpected typed-field control")
        by_type.setdefault(kind, []).append(convert_text(text, codes))

    counts = {}
    for kind, _, _ in usa:
        counts[kind] = counts.get(kind, 0) + 1

    selected = []
    positions = {}
    consumed = set()
    for kind, param, original in usa:
        source_kind = sources.get(kind, kind)
        rows = by_type.get(source_kind)
        if rows is None:
            selected.append((kind, param, original))
            if kept_usa_fields is not None:
                kept_usa_fields.append({"asset": index, "group": group_index, "type": kind,
                                        "source_type": source_kind,
                                        "reason": "Japan has no field of this type; USA text retained"})
            continue
        if source_kind in sources.values():
            assert source_kind not in consumed, (f"{index:#06x} group {group_index}: "
                                                 f"remapped source type {source_kind:#04x} consumed twice")
        consumed.add(source_kind)
        position = positions.get(kind, 0)
        positions[kind] = position + 1
        if len(rows) == counts[kind]:
            text = rows[position]
        elif counts[kind] == 1:
            # USA gldbstring_entrypoint_1 returns only the first matching record
            # (RecompiledFuncs/funcs_204.c:2223-2238). Zoombox's word wrapper only
            # breaks at spaces (funcs_372.c:5438-5481,5550-5608), not at 0A.
            # Retain explicit newlines only for fields whose USA consumer already
            # uses them, e.g. chjiggygamenew's 186D splitter (funcs_295.c:1035-1123).
            separator = b"\n" if b"\n" in original else b" "
            # Field 0x80 carries the prompt/question prose; higher fields are menu labels.
            text, separators = join_typed_rows(rows, codes, separator, prose=kind == 0x80)
            if merges is not None:
                merges.append({"asset": index, "group": group_index, "type": kind,
                               "source_records": len(rows), "separator": separator.decode("ascii"),
                               "separators": separators,
                               "bytes": len(text)})
        else:
            raise ValueError(f"{index:#06x} group {group_index} type {kind:#04x}: "
                             f"ambiguous typed fields: USA {counts[kind]}, Japan {len(rows)}")
        if len(text) > 255:
            raise ValueError(f"{index:#06x} group {group_index} type {kind:#04x}: "
                             f"merged text is {len(text)} bytes; native record limit is 255")
        selected.append((kind, param, text))

    # Keep Japan-only fields, including its additional controller-error messages.
    # For remapped IDs, the USA lookup finds the corrected field first; preserved
    # regional rows follow it and cannot change that first-match lookup.
    for kind, param, text in japan:
        if kind not in consumed:
            selected.append((kind, param, convert_text(text, codes)))
    return selected


def select_section(index: int, usa: list, japan: list, codes: dict[int, int],
                   merges: list | None = None, kept_usa_fields: list | None = None) -> tuple[list, bool]:
    if index in CREDITS_IDS:
        return usa, False
    if len(usa) != len(japan):
        raise ValueError("regional text group count differs")
    adjusted = False
    result = []
    if index == 0x186C:
        # Japan's extra legal/demo rows include debug placeholders. Keep the USA legal notices and indices.
        return [[(kind, param, encode_ui("デモをみる", codes) if text.rstrip(b" \0") == b"SHOW DEMO" else text)
                 for kind, param, text in group] for group in usa], False
    for group_index, (ug, jg) in enumerate(zip(usa, japan)):
        if all(r[0] >= 128 for r in ug):
            result.append(select_typed_group(index, group_index, ug, jg, codes, merges, kept_usa_fields))
            continue
        selected = align_controls(ug, jg)
        adjusted |= [r for r in ug if not packs.is_text(r)] != [r for r in jg if not packs.is_text(r)]
        unchanged = {record[2] for record in ug if packs.is_text(record)}
        converted = []
        for kind, param, text in selected:
            if packs.is_text((kind, param, text)):
                plain = text.replace(b"\xADj", b"")
                text = plain if plain in unchanged else convert_text(text, codes)
            converted.append((kind, param, text))
        result.append(converted)
    if index in (0x1007, 0x143E):
        # These Japanese scripts omit dynamic values which the USA game still supplies.
        lines = (["さいごのもちものを、", "~ダブルンで かわんかね？"] if index == 0x1007 else
                 ["100コぜんぶを ~びょうで", "ヒットさせたな！ こんどは", "もっと はやく やってみるんだ！"])
        group = result[0]
        text_positions = [n for n, r in enumerate(group) if packs.is_text(r)]
        keep = text_positions[0]
        kind, param, _ = group[keep]
        additions = [(kind, param, encode_ui(line, codes)) for line in lines]
        result[0] = group[:keep + 1] + additions + [r for r in group[keep + 1:] if not packs.is_text(r)]
    elif index == 0x154A:
        # USA requests exactly one Jiggy here; the Japanese executable uses a variable instead.
        result = [[(kind, param, text.replace(b"~", b"1") if packs.is_text((kind, param, text)) else text)
                   for kind, param, text in group] for group in result]
    return result, adjusted


def build(japan: bytes, usa: bytes) -> tuple[bytes, bytes, dict]:
    japan = japanese_rom(japan)
    if len(usa) != packs.USA_SIZE or hashlib.sha1(usa).hexdigest() != packs.USA_SHA1:
        raise ValueError("invalid decompressed USA image")
    maps, codes, fonts, font_report = font_profile(japan, usa)
    entries, graphics, expected = {}, {}, {}
    substitutions = 0
    report = {"source_sha1": JAPAN_SHA1, "font_profile": font_report, "controls_adapted": [],
              "usa_gameplay_text_adaptations": [0x1007, 0x143E, 0x154A, 0x186C, 0x18E1, 0x18E2, 0x1A89],
              "usa_only_translated": [],
              "unavailable_in_japan": [], "graphic_assets": [], "regional_data_not_imported": [],
              "credits_usa_preserved": [], "typed_text_merges": [], "typed_fields_kept_usa": [],
              "unreachable_usa_language_fields": {"asset": 0x18E2, "types": list(range(0x99, 0x9E)),
                                                  "reason": "USA settings enumerate only screen alignment, widescreen and sound",
                                                  "handling": "0x99 uses Japan's blank field; 0x9A..0x9D retain USA text because Japan has no matching fields"},
              "typed_field_remaps": {"0x18E1": "USA A3 uses Japan AB (one controller)",
                                    "0x18E2": "prompts shifted; wide/normal and OFF/ON mapped by meaning"}}
    for index in packs.TEXT_IDS:
        first, last = struct.unpack_from(">II", usa, packs.USA_TABLE + index * 4)
        if first & 255 != 0x13 or first >> 8 == last >> 8:
            continue
        original = packs.asset(usa, packs.USA_TABLE, packs.USA_BASE, index)
        if not original.startswith(b"\1\3\0"):
            continue
        a, b = struct.unpack_from(">II", japan, JAPAN_TABLE + index * 4)
        if a >> 8 == b >> 8:
            report["unavailable_in_japan"].append(index)
            if index != 0x1A89:
                raise ValueError(f"Japanese text unavailable for {index:#x}")
            # USA-only multiplayer quiz selection; preserve its field IDs and line/format markers.
            lines = ["だれが つぶされる？", "つぶされるのは どっち？",
                     "バンジョーのまけ\nそれとも\nグランティがつぶれる？", "タワー DE クイズ/N#マルチプレイヤー"]
            original_section = packs.parse_section(original[3:])[0]
            if len(original_section) != 1 or len(original_section[0]) != len(lines):
                raise ValueError("unexpected USA-only quiz menu")
            section = [[(kind, param, encode_ui(line, codes))
                        for (kind, param, _), line in zip(original_section[0], lines)]]
            adjusted = False
            report["usa_only_translated"].append(index)
        else:
            translated = packs.asset(japan, JAPAN_TABLE, JAPAN_BASE, index)
            if not translated.startswith(b"\1\3\0"):
                raise ValueError(f"invalid Japanese text asset {index:#x}")
            section, adjusted = select_section(index, packs.parse_section(original[3:])[0],
                                                packs.parse_section(translated[3:])[0], codes,
                                                report["typed_text_merges"], report["typed_fields_kept_usa"])
            if index in CREDITS_IDS:
                report["credits_usa_preserved"].append(index)
        original_section = packs.parse_section(original[3:])[0]
        for old, new in zip(original_section, section):
            if [r for r in old if not packs.is_text(r)] != [r for r in new if not packs.is_text(r)]:
                raise ValueError(f"USA event control stream changed in {index:#x}")
            if sum(r[2].count(b"~") for r in old if packs.is_text(r)) != sum(r[2].count(b"~") for r in new if packs.is_text(r)):
                raise ValueError(f"USA substitution count changed in {index:#x}")
            substitutions += sum(r[2].count(b"~") for r in old if packs.is_text(r))
        if adjusted:
            report["controls_adapted"].append(index)
        body = packs.serialize_section(section)
        entries[index] = (0, body)
        expected[index] = b"\1\3\0" + body
    for index in range(packs.ASSET_COUNT):
        first, last = struct.unpack_from(">II", usa, packs.USA_TABLE + index * 4)
        a, b = struct.unpack_from(">II", japan, JAPAN_TABLE + index * 4)
        if first & 255 != 0x10 or a & 255 != 0x10 or first >> 8 == last >> 8 or a >> 8 == b >> 8:
            continue
        original = packs.asset(usa, packs.USA_TABLE, packs.USA_BASE, index)
        translated = packs.asset(japan, JAPAN_TABLE, JAPAN_BASE, index)
        if original == translated:
            continue
        if index != TITLE_ID:
            # Only the fonts and the title logo are distributed; other regional graphics keep USA art.
            report["regional_data_not_imported"].append(index)
            continue
        graphics[index] = (original, translated)
        expected[index] = translated
        report["graphic_assets"].append(index)
    for index, raw in fonts.items():
        graphics[index] = (packs.asset(usa, packs.USA_TABLE, packs.USA_BASE, index), raw)
        expected[index] = raw
    base = packs.asset(usa, packs.USA_TABLE, packs.USA_BASE, 0xC21)
    bundle = packs.bundle_bytes("ja", maps, bytes(30), base, [], entries, graphics)
    parsed = packs.read_bundle(bundle)
    rebuilt = packs.rebuild_text_assets(parsed, usa) | packs.rebuild_graphic_assets(parsed, usa)
    if rebuilt != expected:
        raise ValueError("Japanese bundle failed reconstruction parity")
    reference = packs.pack_bytes("ja", maps, bytes(30), {i: packs.encode_payload(raw) for i, raw in expected.items()})
    report.update({"text_count": len(entries), "graphic_count": len(graphics) - 2, "font_count": 2,
                   "typed_fields_merged": len(report["typed_text_merges"]),
                   "typed_assets_merged": len({m["asset"] for m in report["typed_text_merges"]}),
                   "typed_fragments_recovered": sum(m["source_records"] - 1 for m in report["typed_text_merges"]),
                   "typed_join_counts": {name: sum(m["separators"].count(value) for m in report["typed_text_merges"])
                                         for name, value in (("space", " "), ("no_space", ""), ("newline", "\n"))},
                   "largest_merged_record_bytes": max((m["bytes"] for m in report["typed_text_merges"]), default=0),
                   "bundle_bytes": len(bundle), "bundle_sha256": hashlib.sha256(bundle).hexdigest(),
                   "verification": "PASS", "title_texture_count": 31,
                   "usa_substitutions_preserved": substitutions})
    return bundle, reference, report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--japan-rom", type=Path, required=True)
    parser.add_argument("--usa-image", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=packs.app_dir() / "languages" / "ja.tlpb")
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    bundle, reference, report = build(args.japan_rom.read_bytes(), args.usa_image.read_bytes())
    packs.atomic_write(args.output, bundle)
    packs.atomic_write(args.reference, reference)
    packs.atomic_write(args.report, (json.dumps(report, indent=2) + "\n").encode("utf-8"))
    print(json.dumps({key: value for key, value in report.items()
                      if key not in ("font_profile", "graphic_assets", "regional_data_not_imported", "typed_text_merges")}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
