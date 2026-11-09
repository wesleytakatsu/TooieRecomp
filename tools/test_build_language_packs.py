#!/usr/bin/env python3
"""Synthetic tests for build_language_packs.py.

    python tools/test_build_language_packs.py
"""
from __future__ import annotations

import struct
import unittest
import zlib
from unittest.mock import patch

import build_language_packs as packs
import build_italian_bundle as italian
import build_japanese_bundle as japanese


def varint(value: int) -> bytes:
    out = bytearray()
    while True:
        byte = value & 127
        value >>= 7
        if value == 0:
            out.append(byte | 128)
            return bytes(out)
        out.append(byte)
        value -= 1


def make_patch(source: bytes) -> bytes:
    target = b"abXYdeXYde"
    actions = varint((2 - 1) * 4) + varint((2 - 1) * 4 + 1) + b"XY"
    actions += varint((2 - 1) * 4 + 2) + varint(3 * 2)
    actions += varint((4 - 1) * 4 + 3) + varint(2 * 2)
    patch = b"BPS1" + varint(len(source)) + varint(len(target)) + varint(0) + actions
    patch += struct.pack("<II", zlib.crc32(source), zlib.crc32(target))
    return patch + struct.pack("<I", zlib.crc32(patch))


class LanguagePackTests(unittest.TestCase):
    def test_section_round_trip(self) -> None:
        section = [[(3, 7, b"hi"), (2, None, b"control"), (128, None, b"text")], []]
        raw = packs.serialize_section(section)
        self.assertEqual(packs.parse_section(raw), (section, len(raw)))
        with self.assertRaises(ValueError):
            packs.parse_section(raw[:-2])
        with self.assertRaises(ValueError):
            packs.serialize_section([[(128, None, b"x" * 256)]])

    def test_decisions(self) -> None:
        usa = [[(3, 1, b"hello"), (2, None, b"c")]]
        language = [[(3, 1, b"bonjour"), (2, None, b"c")]]
        self.assertEqual(packs.shape(usa), packs.shape(language))
        self.assertEqual(packs.choose_text(usa, usa, language), (1, b"\x01\x03\x00" + packs.serialize_section(language), 0))
        us = [[(128, None, b"a"), (129, None, b"b")]]
        en = [[(128, None, b"a"), (129, None, b"different")]]
        lang = [[(128, None, b"translated")]]
        rule, raw, count = packs.choose_text(us, en, lang)
        self.assertEqual((rule, count), (2, 1))
        self.assertEqual(packs.parse_section(raw[3:])[0], [[(128, None, b"translated"), (129, None, b"b")]])
        self.assertEqual(packs.choose_text(usa, usa, [[(2, None, b"other")]]), (0, None, 0))
        dialog = [[(5, None, b"\0")], [(128, None, b"hi"), (9, None, b"d\0"), (128, None, b"bye")]]
        english = [[(5, None, b"\0")], [(128, None, b"hi"), (9, None, b"d\0"), (129, None, b"bye")]]
        french = [[(5, None, b"\0")], [(128, None, b"salut"), (9, None, b"d\0"), (129, None, b"au revoir")]]
        rule, raw, count = packs.choose_text(dialog, english, french)
        self.assertEqual((rule, count), (3, 0))
        self.assertEqual(packs.parse_section(raw[3:])[0],
                         [[(5, None, b"\0")], [(128, None, b"salut"), (9, None, b"d\0"), (128, None, b"au revoir")]])
        moved = [[(5, None, b"\0")], [(128, None, b"salut"), (128, None, b"au revoir"), (9, None, b"d\0")]]
        self.assertEqual(packs.choose_text(dialog, english, moved), (0, None, 0))

    def test_payload_and_pack(self) -> None:
        payload = packs.encode_payload(b"synthetic data" * 10)
        self.assertEqual(len(payload) % 8, 0)
        self.assertEqual(packs.inflate(payload), b"synthetic data" * 10)
        packed = packs.pack_bytes("pt-BR", b"abc", bytes(range(30)), {7: payload, 2: payload})
        magic, version, tag, size, count, length = struct.unpack_from("<4sI16sIII", packed)
        self.assertEqual((magic, version, tag.rstrip(b"\0"), size, count, length), (b"TLPK", 1, b"pt-BR", packs.USA_SIZE, 2, 3))
        self.assertEqual(packed[36:39], b"abc")
        self.assertEqual(packed[39:69], bytes(range(30)))
        pos = 69
        for expected in (2, 7):
            index, length = struct.unpack_from("<II", packed, pos)
            pos += 8
            self.assertEqual((index, length), (expected, len(payload)))
            self.assertEqual(packed[pos:pos + length], payload)
            pos += length
        self.assertEqual(pos, len(packed))

    def test_bps(self) -> None:
        patch = make_patch(b"abcdef")
        self.assertEqual(packs.apply_bps(b"abcdef", patch), b"abXYdeXYde")
        with self.assertRaisesRegex(ValueError, "source CRC"):
            packs.apply_bps(b"abcdeg", patch)
        with self.assertRaisesRegex(ValueError, "patch CRC"):
            packs.apply_bps(b"abcdef", patch[:-1] + bytes([patch[-1] ^ 1]))

    def test_bundle_round_trip(self) -> None:
        base = b"0123456789"
        target = b"0A23B56789"
        patches = packs.font_patches(base, target)
        entries = {12: (1, b"\0\1\3ola"), 3: (0, b"\0")}
        encoded = packs.bundle_bytes("pt-BR", b"abc", bytes(range(30)), base, patches, entries)
        bundle = packs.read_bundle(encoded)
        self.assertEqual((bundle["tag"], bundle["charset"], bundle["accents"]), ("pt-BR", b"abc", bytes(range(30))))
        self.assertEqual(bundle["entries"], entries)
        self.assertEqual(packs.apply_font_patches(base, bundle["font_size"], bundle["font_crc"], bundle["patches"]), target)
        with self.assertRaises(ValueError):
            packs.read_bundle(encoded + b"x")
        with self.assertRaises(ValueError):
            packs.read_bundle(encoded[:-1])

    def test_overlay(self) -> None:
        usa = [[(3, 7, b"keep"), (2, None, b"control"), (128, None, b"change")]]
        translated = [[(3, 7, b"keep"), (2, None, b"control"), (128, None, b"mudar")]]
        raw = b"\x01\x03\x00" + packs.serialize_section(usa)
        overlay = packs.text_overlay(usa, translated)
        self.assertEqual(overlay, b"\0\1\5mudar")
        self.assertEqual(packs.apply_text_entry(raw, 1, overlay), b"\x01\x03\x00" + packs.serialize_section(translated))
        replacement = packs.serialize_section([[(128, None, b"novo")]])
        bundle = {"entries": {7: (1, overlay), 8: (0, replacement)}}
        with patch.object(packs, "asset", return_value=raw):
            self.assertEqual(packs.rebuild_text_assets(bundle, b"synthetic"),
                             {7: b"\x01\x03\x00" + packs.serialize_section(translated),
                              8: b"\x01\x03\x00" + replacement})
        for invalid in (overlay[:-6], overlay + b"\0"):
            with self.assertRaises(ValueError):
                packs.apply_text_entry(raw, 1, invalid)

    def test_font_patches(self) -> None:
        base = b"0123456789abcdef"
        target = b"0A23B56789Cdefgh"
        patches = packs.font_patches(base, target)
        self.assertEqual(patches, [(1, b"A23B"), (10, b"Cdefgh")])
        self.assertEqual(packs.apply_font_patches(base, len(base), zlib.crc32(base), patches), target)
        with self.assertRaises(ValueError):
            packs.font_patches(base, target[:-1])
        with self.assertRaises(ValueError):
            packs.apply_font_patches(base, len(base) + 1, zlib.crc32(base), patches)
        with self.assertRaises(ValueError):
            packs.apply_font_patches(base, len(base), zlib.crc32(base) ^ 1, patches)

    def test_graphic_delta_validation(self) -> None:
        encoded = packs.bundle_bytes("it", italian.ITALIAN_CHARSET, italian.ITALIAN_ACCENTS,
                                     b"font", [], {}, {7: (b"base", b"bAse!")})
        bundle = packs.read_bundle(encoded)
        with patch.object(packs, "asset", return_value=b"base"):
            self.assertEqual(packs.rebuild_graphic_assets(bundle, b"synthetic"), {7: b"bAse!"})
            bundle["assets"][7]["target_crc"] ^= 1
            with self.assertRaisesRegex(ValueError, "target CRC"):
                packs.rebuild_graphic_assets(bundle, b"synthetic")
        with patch.object(packs, "asset", return_value=b"wrong base"):
            with self.assertRaisesRegex(ValueError, "base size or CRC"):
                packs.rebuild_graphic_assets(bundle, b"synthetic")
        for invalid in (encoded[:-1], encoded + b"extra"):
            with self.assertRaises(ValueError):
                packs.read_bundle(invalid)
        with self.assertRaisesRegex(ValueError, "graphic asset ID"):
            packs.bundle_bytes("it", b"abc", bytes(30), b"font", [], {}, {0xC22: (b"a", b"b")})

    def test_italian_native_encoding(self) -> None:
        self.assertEqual(italian.encode_text("À È É Ì Ò Ù {86} ~"), b"\x5F \x63 \x62 \x6C \x6F \x71 \x86 ~\0")
        self.assertEqual(italian.tokens(b"press \x86 twice: ~ ~\0"), italian.tokens(italian.encode_text("~ ~ PREMI {86}")))
        self.assertNotEqual(italian.tokens(b"\x86 ~ ~"), italian.tokens(italian.encode_text("~ PREMI {86}")))
        for invalid in ("È" * 255, "lowercase", "{01}", "EMOJI 😀"):
            with self.assertRaises(ValueError):
                italian.encode_text(invalid)

    def test_italian_hd_identity_preserves_visible_texels(self) -> None:
        count, setup = 28, 0x50
        base = setup + 8 + count * 8
        model = bytearray(base + count * 4096)
        model[:4] = b"\0\0\0\x0B"
        struct.pack_into(">H", model, 8, setup)
        struct.pack_into(">H", model, setup + 4, count)
        for index in range(count):
            struct.pack_into(">IHBB", model, setup + 8 + index * 8, index * 4096, 8, 32, 32)
            start = base + index * 4096
            model[start:start + 4096] = bytes([40, 80, 120, 255]) * 1024
            model[start:start + 8] = bytes([10, 20, 30, 0]) * 2
        result, markers = italian.italian_title_hd_identity(bytes(model))
        changed = [index for index, (a, b) in enumerate(zip(model, result)) if a != b]
        self.assertEqual(changed, markers)
        self.assertEqual(len(changed), 28)
        for offset in changed:
            self.assertEqual((model[offset + 3], result[offset + 3]), (0, 0))
            self.assertEqual(model[offset] ^ result[offset], 1)
        opaque = bytearray(model)
        opaque[base:base + 8] = bytes([40, 80, 120, 255]) * 2
        with self.assertRaisesRegex(ValueError, "no transparent"):
            italian.italian_title_hd_identity(bytes(opaque))
        with self.assertRaises(ValueError):
            italian.italian_title_hd_identity(bytes(model[:-1]))


class JapaneseBundleTests(unittest.TestCase):
    def test_native_punctuation_and_long_vowel_are_not_shifted(self) -> None:
        codes = japanese.character_codes()
        # Native frame 55 is apostrophe; the following symbols shift by one.
        for glyph, character in {55: "'", 56: "*", 57: "+", 58: ",", 60: ".", 61: "/",
                                 62: ":", 63: "=", 64: "?", 65: "@"}.items():
            self.assertEqual(codes[glyph], ord(character))
        self.assertNotEqual(codes[59], ord("."))
        self.assertNotEqual(codes[59], codes[58])
        self.assertEqual(japanese.encode_ui("ー。", codes), bytes([codes[59], codes[66], 0]))
        self.assertEqual(japanese.convert_text(b"\xADj\x3B\x42\0", codes),
                         japanese.encode_ui("ー。", codes))

    def test_kana_never_uses_inline_button_codes(self) -> None:
        codes = japanese.character_codes()
        self.assertEqual(len(codes), 230)
        self.assertEqual(len(set(codes.values())), 229)  # Two native space frames.
        self.assertFalse(set(codes.values()) & japanese.USA_BUTTON_CODES)
        self.assertFalse(set(codes.values()) & {0, 10, 0x78, 0x7E, 0xFD, 0xFE, 0xFF})
        for glyph in range(1, 230):
            native = glyph + 26 if glyph >= 149 else glyph
            expected = 10 if glyph == 10 else codes[glyph]
            self.assertEqual(japanese.convert_text(b"\xADj" + bytes([native, 0]), codes),
                             bytes([expected, 0]))

    def test_native_newline_survives_japanese_font_mode(self) -> None:
        codes = japanese.character_codes()
        self.assertEqual(japanese.convert_text(b"\xADj\x3B\n\x42\0", codes),
                         japanese.encode_ui("ー\n。", codes))
        self.assertEqual(japanese.convert_text(b"ASCII\n\xADj\x3B\n\0", codes),
                         b"ASCII\n" + japanese.encode_ui("ー\n", codes))

    def test_authored_native_kana_and_standalone_marks(self) -> None:
        codes = japanese.character_codes()
        self.assertEqual(japanese.KANA[78 - 69], "ヲ")
        self.assertEqual(japanese.encode_ui("ヲ〜゛゜″", codes),
                         bytes([codes[78], codes[13], codes[67], codes[68], codes[9], 0]))
        self.assertNotIn(b"~", japanese.encode_ui("〜", codes))
        with self.assertRaisesRegex(ValueError, "unsupported authored"):
            japanese.encode_ui("ヮ", codes)
        with self.assertRaisesRegex(ValueError, "native record limit"):
            japanese.encode_ui("ア" * 255, codes)

    def test_typed_question_and_answers_keep_every_fragment(self) -> None:
        codes = japanese.character_codes()
        usa = [[(128, None, b"QUESTION\0"), (129, None, b"ANSWER ONE\0"),
                (130, None, b"ANSWER TWO\0")]]
        # Deliberately interleave types: order within each field must survive.
        japan = [[(128, None, b"\xADj\x1A\0"), (130, None, b"SECOND\0"),
                  (128, None, b"\xADj\x1B\0"), (129, None, b"FIRST\0"),
                  (128, None, b"\xADj\x1C\0")]]
        merges = []
        result, _ = japanese.select_section(0x1910, usa, japan, codes, merges)
        self.assertEqual(result, [[(128, None, b"A B C\0"), (129, None, b"FIRST\0"),
                                   (130, None, b"SECOND\0")]])
        self.assertEqual(merges[0]["source_records"], 3)
        self.assertEqual(merges[0]["separator"], " ")

    def test_typed_newlines_and_repeated_usa_slots(self) -> None:
        codes = japanese.character_codes()
        usa = [[(128, None, b"TITLE\nSUBTITLE\0"), (129, None, b"ONE\0"),
                (129, None, b"TWO\0")]]
        japan = [[(129, None, b"FIRST\0"), (128, None, b"TOP\0"),
                  (128, None, b"BOTTOM\0"), (129, None, b"SECOND\0")]]
        result, _ = japanese.select_section(0x186D, usa, japan, codes)
        self.assertEqual(result, [[(128, None, b"TOP\nBOTTOM\0"),
                                   (129, None, b"FIRST\0"), (129, None, b"SECOND\0")]])
        with self.assertRaisesRegex(ValueError, "USA 2, Japan 1"):
            japanese.select_section(0x186D, usa, [japan[0][:-1]], codes)

    def test_typed_space_boundaries_use_native_glyph_identity(self) -> None:
        codes = japanese.character_codes()
        # Deliberately relocate glyph codes: decisions must follow glyph identity.
        codes = {glyph: 255 - code for glyph, code in codes.items()}
        cases = [
            (["フンバ・", "うんば"], "フンバ・うんば", [""]),
            (["ティラノ", "ザウルス"], "ティラノザウルス", [""]),
            (["スタ", "ー", "ッ", "ヴ"], "スターッヴ", ["", "", ""]),
            (["つぎは「マヤヤン", "しんでん", "」だ", "よ"], "つぎは「マヤヤンしんでん」だ よ", ["", "", " "]),
            (["「ひとつ」「ふたつ", "みっつ」", "おわり"], "「ひとつ」「ふたつみっつ」 おわり", ["", " "]),
            (["かいて", "あった"], "かいて あった", [" "]),
            (["アと", "イ"], "アと イ", [" "]),
        ]
        for fragments, expected, boundaries in cases:
            with self.subTest(fragments=fragments):
                rows = [japanese.encode_ui(fragment, codes) for fragment in fragments]
                text, actual = japanese.join_typed_rows(rows, codes, b" ")
                self.assertEqual(text, japanese.encode_ui(expected, codes))
                self.assertEqual(actual, boundaries)

    def test_menu_labels_keep_space_between_katakana_terms(self) -> None:
        codes = japanese.character_codes()
        rows = [japanese.encode_ui(part, codes) for part in ("トリトリマッチ", "プレイできません")]
        self.assertEqual(japanese.join_typed_rows(rows, codes, b" ", prose=False),
                         (japanese.encode_ui("トリトリマッチ プレイできません", codes), [" "]))
        quoted = [japanese.encode_ui(part, codes) for part in ("「マヤヤン", "しんでん」")]
        self.assertEqual(japanese.join_typed_rows(quoted, codes, b" ", prose=False)[1], [""])

    def test_typed_punctuation_and_closed_quotes_keep_wrap_spaces(self) -> None:
        codes = japanese.character_codes()
        for ending in ["は", "、", "。", "!", "?", "」", "…"]:
            with self.subTest(ending=ending):
                left = "「こと」" if ending == "」" else "こと" + ending
                rows = [japanese.encode_ui(part, codes) for part in (left, "つぎ")]
                self.assertEqual(japanese.join_typed_rows(rows, codes, b" "),
                                 (japanese.encode_ui(left + " つぎ", codes), [" "]))

    def test_native_typed_fragments_apply_join_rules_and_report_boundaries(self) -> None:
        codes = japanese.character_codes()
        glyphs = {code: glyph for glyph, code in codes.items()}
        fragments = ["「フンバ・ ", " ウン", "バ", "の", "こと」", "かいて", "あった"]
        rows = []
        for fragment in fragments:
            native = []
            for code in japanese.encode_ui(fragment, codes)[:-1]:
                glyph = glyphs[code]
                native.append(glyph + 26 if glyph >= 149 else glyph)
            rows.append((128, None, b"\xADj" + bytes(native) + b"\0"))
        merges = []
        result, _ = japanese.select_section(0x1938, [[(128, None, b"USA\0")]], [rows], codes, merges)
        self.assertEqual(result[0][0][2], japanese.encode_ui("「フンバ・ウンバのこと」 かいて あった", codes))
        self.assertEqual(merges[0]["separators"], ["", "", "", "", " ", " "])

    def test_typed_trimming_preserves_internal_separators_and_newlines(self) -> None:
        codes = japanese.character_codes()
        for fragments, expected in [(["A  B   ", "   C  D  ", " E "], "A  B C  D E"),
                                    (["ティラノ  ", "  ザウルス "], "ティラノザウルス"),
                                    (["「マヤヤン ", " しん でん」 "], "「マヤヤンしん でん」")]:
            with self.subTest(fragments=fragments):
                rows = [japanese.encode_ui(fragment, codes) for fragment in fragments]
                self.assertEqual(japanese.join_typed_rows(rows, codes, b" ")[0],
                                 japanese.encode_ui(expected, codes))
        rows = [japanese.encode_ui(part, codes) for part in ("ティラノ  ", " ザウルス ")]
        self.assertEqual(japanese.join_typed_rows(rows, codes, b"\n"),
                         (japanese.encode_ui("ティラノ  \n ザウルス ", codes), ["\n"]))
        # One-to-one fields must remain byte-for-byte unchanged.
        japan = [[(128, None, b"\xADj\x1A\xAC\xAC\x1B\xAC\0")]]
        result, _ = japanese.select_section(0x1910, [[(128, None, b"USA\0")]], japan, codes)
        self.assertEqual(result[0][0][2], b"A  B \0")

    def test_missing_typed_fields_are_reported_including_repeated_slots(self) -> None:
        usa = [[(128, None, b"ONE\0"), (129, None, b"TWO\0"), (129, None, b"THREE\0")]]
        kept = []
        result, _ = japanese.select_section(0x1910, usa, [[(128, None, b"JP\0")]],
                                             japanese.character_codes(), kept_usa_fields=kept)
        self.assertEqual(result[0][1:], usa[0][1:])
        self.assertEqual([(r["asset"], r["group"], r["type"], r["source_type"]) for r in kept],
                         [(0x1910, 0, 129, 129), (0x1910, 0, 129, 129)])

    def test_remapped_source_type_cannot_be_consumed_twice(self) -> None:
        for kinds in ([0xA3, 0xAB], [0xAB, 0xA3], [0xA3, 0xA3]):
            with self.subTest(kinds=kinds):
                usa = [[(kind, None, b"USA\0") for kind in kinds]]
                japan = [[(0xAB, None, b"JP\0") for _ in range(kinds.count(0xA3))]]
                with self.assertRaisesRegex(AssertionError, "remapped source type 0xab consumed twice"):
                    japanese.select_section(0x18E1, usa, japan, japanese.character_codes())

    def test_typed_merge_reports_asset_type_and_overflow_size(self) -> None:
        codes = japanese.character_codes()
        usa = [[(128, None, b"QUESTION\0")]]
        japan = [[(128, None, b"A" * 126 + b"\0"), (128, None, b"B" * 127 + b"\0")]]
        result, _ = japanese.select_section(0x1910, usa, japan, codes)
        self.assertEqual(len(result[0][0][2]), 255)
        japan[0][1] = (128, None, b"B" * 128 + b"\0")
        with self.assertRaisesRegex(ValueError, "0x1910 group 0 type 0x80: merged text is 256 bytes"):
            japanese.select_section(0x1910, usa, japan, codes)

    def test_settings_prompts_and_widescreen_states_map_by_meaning(self) -> None:
        codes = japanese.character_codes()
        kinds = [0x83, 0x84, 0x85, 0x86, 0x87, 0x8F, 0x90, 0x9A, 0x99]
        usa = [[(kind, None, b"USA\0") for kind in kinds]]
        japan = [[(0x83, None, b" \0"), (0x84, None, b"WIDE PROMPT\0"),
                  (0x85, None, b"SOUND PROMPT\0"), (0x86, None, b" \0"),
                  (0x87, None, b"TV WIDE\0"), (0x88, None, b"TV NORMAL\0"),
                  (0x8F, None, b"ON\0"), (0x90, None, b"OFF\0"), (0x99, None, b" \0")]]
        kept = []
        result, _ = japanese.select_section(0x18E2, usa, japan, codes, kept_usa_fields=kept)
        self.assertEqual([r[2] for r in result[0][:8]],
                         [b"WIDE PROMPT\0", b"SOUND PROMPT\0", b" \0", b"TV WIDE\0",
                          b"TV NORMAL\0", b"OFF\0", b"ON\0", b"USA\0"])
        self.assertEqual(result[0][8][2], b" \0")
        self.assertEqual([r["type"] for r in kept], [0x9A])

    def test_controller_warning_and_regional_extras_are_preserved(self) -> None:
        codes = japanese.character_codes()
        usa = [[(0xA3, None, b"ONE CONTROLLER\0")]]
        japan = [[(0xA3, None, b"TWO REQUIRED\0"), (0xAB, None, b"ONE CONTROLLER\0"),
                  (0xAC, None, b"CONTROLLER ERROR\0")]]
        result, _ = japanese.select_section(0x18E1, usa, japan, codes)
        self.assertEqual(result[0][0][2], b"ONE CONTROLLER\0")
        self.assertCountEqual([r[2] for r in result[0]], [r[2] for r in japan[0]])

    def test_staff_credits_and_cast_roll_always_keep_usa_text(self) -> None:
        codes = japanese.character_codes()
        usa = [[(128, None, b"USA CREDITS\0")]]
        japan = [[(128, None, b"REGIONALLY SHIFTED\0")]]
        for index in japanese.CREDITS_IDS:
            self.assertEqual(japanese.select_section(index, usa, japan, codes), (usa, False))

    def test_japanese_buttons_and_separator_use_usa_codes(self) -> None:
        codes = japanese.character_codes()
        self.assertEqual(japanese.convert_text(b"\xADj\x9D\xAC\x9C\0", codes), b"\x87 \x86\0")
        self.assertEqual(japanese.convert_text(b"\xADj" + bytes(range(0x95, 0x9E)) + b"\0", codes),
                         bytes(range(0x7F, 0x88)) + b"\0")
        for code in range(0x9E, 0xAC):
            with self.assertRaisesRegex(ValueError, "inline control"):
                japanese.convert_text(b"\xADj" + bytes([code, 0]), codes)

    def test_native_control_and_glyph_codes_are_distinct(self) -> None:
        codes = {13: 0x90, 126: 0x91, 227: 0x92, 228: 0x93}
        # Wave dash and kana must not become USA substitution/format commands.
        self.assertEqual(japanese.convert_text(b"\xADj\x0D\x7E\xAE\xFD\xFE\0", codes),
                         b"\x90\x91~\x92\x93\0")
        self.assertEqual(japanese.convert_text(b"ASCII\0", codes), b"ASCII\0")
        with self.assertRaisesRegex(ValueError, "font command"):
            japanese.convert_text(b"\xADj\xADk\0", codes)

    def test_regional_waits_preserve_usa_event_parameters(self) -> None:
        a, b = (128, None, b"a\0"), (128, None, b"b\0")
        wait, event = (11, None, b"\0"), (9, None, b"\x66\0")
        source = [a, wait, event, b, (5, None, b"\0")]
        region = [a, wait, wait, event, b, (5, None, b"\0")]
        aligned = japanese.align_controls(source, region)
        self.assertEqual([r for r in aligned if not packs.is_text(r)],
                         [r for r in source if not packs.is_text(r)])
        self.assertEqual([r for r in aligned if packs.is_text(r)], [a, b])

    def test_legal_indices_do_not_import_japanese_debug_rows(self) -> None:
        source = [[(128, None, b"DOLBY\0"), (131, None, b"SHOW DEMO  \0"),
                   (132, None, b"x 2000 NINTENDO/RARE\0")]]
        region = [[(128, None, b"TEXT 26.OCT\0"), (131, None, b"TEST2\0")]]
        selected, _ = japanese.select_section(0x186C, source, region, {i: 0x80 for i in range(230)})
        self.assertEqual(selected[0][0], source[0][0])
        self.assertEqual(selected[0][2], source[0][2])
        self.assertEqual(selected[0][1][2], b"\x80" * 5 + b"\0")

    def test_extended_font_profile_requires_version_three(self) -> None:
        base = b"base"
        data = packs.bundle_bytes("ja", b"\xFF" * 1024, bytes(30), base, [], {},
                                  {0xC21: (base, b"new font")})
        self.assertEqual(struct.unpack_from("<I", data, 4)[0], 3)
        self.assertEqual(packs.read_bundle(data)["tag"], "ja")
        wrong_version = data[:4] + struct.pack("<I", 2) + data[8:]
        with self.assertRaises(ValueError):
            packs.read_bundle(wrong_version)
        with self.assertRaises(ValueError):
            packs.bundle_bytes("it", b"A" * 73, bytes(30), base, [], {}, {0xC21: (base, b"bad")})


if __name__ == "__main__":
    unittest.main()
