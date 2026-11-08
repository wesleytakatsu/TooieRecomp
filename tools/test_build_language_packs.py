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


if __name__ == "__main__":
    unittest.main()
