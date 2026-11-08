#!/usr/bin/env python3
"""Build a compatible Italian bundle from Rulesless v0.9 and private USA ROM inputs.

Only compatible asset data is imported. An optional offline catalog completes
text from USA records. The original patch and a patched ROM are never bundled.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zlib
from collections import Counter
from pathlib import Path

import build_language_packs as packs

PATCH_SHA256 = "1789ade8728408b9c382c5964c436411b874513409e2ed2a29b7079fc4f728f5"
USA_ROM_SHA1 = "af1a89e12b638b8d82cc4c085c8e01d4cba03fb3"
SOURCE_URL = "https://letraduzionidirulesless.wordpress.com/banjo-tooie/"
# Keep original branding and its embedded texture hashes for USA-based HD packs.
USA_BRANDING_ASSETS = {0x632: "Rareware logo", 0x8A8: "Banjo-Tooie title", 0x8AC: "Dolby logo"}
ACCENT_CODES = dict(zip("ÄÖÜßÀÂÇÉÈÊËÎÏÔÛÜÙÌÍÓÒÚÙÑÁ¡¿ª°©", range(0x5B, 0x79)))
ITALIAN_CHARSET = packs.CHARSET_PREFIX + "'ÄÁÀÉÈÍÌÑÖÓÒÜÚÙ¿¡ª°ÏÊÛÔ".encode("latin-1")
ITALIAN_ACCENTS = bytes([0, 8, 11, 0, 2, 0, 0, 3, 4, 18, 0, 0, 0, 22, 21,
                        11, 13, 6, 5, 9, 10, 12, 13, 7, 1, 15, 14, 16, 17, 0])


def tokens(data: bytes) -> Counter:
    """Runtime substitutions and button glyphs must survive translation."""
    return Counter(value for value in data.split(b"\0", 1)[0] if value == 0x7E or value >= 0x7F)


def encode_text(text: str) -> bytes:
    out = bytearray()
    pos = 0
    while pos < len(text):
        if text.startswith("{", pos) and pos + 4 <= len(text) and text[pos + 3] == "}":
            value = int(text[pos + 1:pos + 3], 16)
            if value < 0x7F:
                raise ValueError("invalid button token")
            out.append(value)
            pos += 4
            continue
        char = text[pos]
        if char in ACCENT_CODES:
            out.append(ACCENT_CODES[char])
        elif char == "\n" or " " <= char <= "Z" or char == "~":
            out.append(ord(char))
        else:
            raise ValueError(f"unsupported Italian glyph {char!r}")
        pos += 1
    if len(out) > 254:
        raise ValueError("Italian text exceeds the native 255-byte record limit")
    return bytes(out) + b"\0"


def supplement(usa: bytes, texts: dict, translations: dict | None) -> tuple[dict, list]:
    """Apply a complete, source-keyed catalog without changing native commands."""
    pending = []
    changed = {}
    used = set()
    entries = translations.get("entries", {}) if translations is not None else {}
    if translations is not None and (translations.get("schema") != 1 or translations.get("usa_sha1") != packs.USA_SHA1
                                     or translations.get("patch_sha256") != PATCH_SHA256):
        raise ValueError("unsupported Italian translation catalog")
    for index in packs.TEXT_IDS:
        if struct.unpack_from(">I", usa, packs.USA_TABLE + index * 4)[0] & 0xFF != 0x13:
            continue
        original = packs.asset(usa, packs.USA_TABLE, packs.USA_BASE, index)
        if not original.startswith(b"\1\3\0"):
            continue
        source = packs.parse_section(original[3:])[0]
        selected = packs.parse_section(packs.apply_text_entry(original, *texts[index])[3:])[0] if index in texts else source
        result = []
        for group_index, (source_group, target_group) in enumerate(zip(source, selected)):
            group = []
            for record_index, (source_record, target_record) in enumerate(zip(source_group, target_group)):
                if not packs.is_text(source_record):
                    group.append(source_record)
                    continue
                raw = source_record[2].split(b"\0", 1)[0]
                # Retain the original legal notices; translations do not replace attribution.
                legal = index == 0x186C
                if tokens(source_record[2]) != tokens(target_record[2]) or legal:
                    target_record = source_record
                if raw.strip() and target_record[2] == source_record[2] and not legal:
                    key = hashlib.sha256(raw).hexdigest()
                    pending.append({"id": index, "group": group_index, "record": record_index,
                                    "key": key, "source_hex": raw.hex()})
                    if translations is not None:
                        if key not in entries:
                            raise ValueError(f"missing Italian catalog entry {index:#x}/{group_index}/{record_index}: {key}")
                        entry = entries[key]
                        used.add(key)
                        if entry.get("kind") == "preserve":
                            if entry.get("reason") not in ("name", "credit", "code", "numeric", "fictional"):
                                raise ValueError("invalid catalog preservation reason")
                        elif entry.get("kind") == "translate":
                            data = encode_text(entry["text"])
                            if tokens(data) != tokens(raw):
                                raise ValueError(f"Italian substitutions/buttons differ at {index:#x}/{group_index}/{record_index}")
                            if any(data.count(bytes([value])) != raw.count(bytes([value])) for value in (10, 35)):
                                raise ValueError(f"Italian title formatting differs at {index:#x}/{group_index}/{record_index}")
                            target_record = (source_record[0], source_record[1], data)
                        else:
                            raise ValueError("invalid Italian catalog entry kind")
                group.append(target_record)
            result.append(group)
        rebuilt = b"\1\3\0" + packs.serialize_section(result)
        if rebuilt != original:
            changed[index] = rebuilt
    if translations is not None and used != entries.keys():
        raise ValueError("unused Italian catalog entries")
    return changed, pending


def build(source: bytes, usa: bytes, patch: bytes, translations: dict | None = None) -> tuple[bytes, bytes, dict]:
    if len(source) != 33554432 or hashlib.sha1(source).hexdigest() != USA_ROM_SHA1:
        raise ValueError("invalid USA retail ROM")
    if len(usa) != packs.USA_SIZE or hashlib.sha1(usa).hexdigest() != packs.USA_SHA1:
        raise ValueError("invalid decompressed USA image")
    if hashlib.sha256(patch).hexdigest() != PATCH_SHA256:
        raise ValueError("unsupported Italian patch; expected Rulesless v0.9")
    target = packs.apply_bps(source, patch)
    sentinel = struct.unpack_from(">I", source, packs.USA_TABLE + packs.ASSET_COUNT * 4)[0]
    tail = packs.USA_BASE + (sentinel >> 8) * 4
    if target[:packs.USA_BASE] != source[:packs.USA_BASE] or target[tail:] != source[tail:]:
        raise ValueError("Italian patch modifies code or asset-table layout")
    texts = {}
    graphics = {}
    expected = {}
    report = {"source": SOURCE_URL, "version": "0.9", "patch_sha256": PATCH_SHA256,
              "text_assets": [], "graphic_assets": [], "rejected": [], "size_headers_repaired": [],
              "usa_branding_preserved": []}
    for index in range(packs.ASSET_COUNT):
        first, last = struct.unpack_from(">II", source, packs.USA_TABLE + index * 4)
        start, end = packs.USA_BASE + (first >> 8) * 4, packs.USA_BASE + (last >> 8) * 4
        if source[start:end] == target[start:end]:
            continue
        if index in USA_BRANDING_ASSETS:
            report["usa_branding_preserved"].append({"id": index, "role": USA_BRANDING_ASSETS[index]})
            continue
        try:
            original = packs.asset(usa, packs.USA_TABLE, packs.USA_BASE, index)
            payload = target[start:end]
            if len(payload) < 2:
                raise ValueError("truncated asset")
            decoder = zlib.decompressobj(-15)
            translated = decoder.decompress(payload[2:], 65535 * 16 + 1)
            if not decoder.eof or len(translated) > 65535 * 16:
                raise ValueError("truncated or oversized compressed asset")
            if original == translated:
                continue
            if (len(translated) + 15) // 16 != int.from_bytes(payload[:2], "big"):
                report["size_headers_repaired"].append(index)
            if index in packs.TEXT_IDS:
                if not original.startswith(b"\1\3\0") or not translated.startswith(b"\1\3\0"):
                    raise ValueError("invalid USA text header")
                original_section = packs.parse_section(original[3:])[0]
                translated_section = packs.parse_section(translated[3:])[0]
                _, selected, _ = packs.choose_text(original_section, original_section, translated_section)
                if selected is None:
                    raise ValueError("incompatible text controls or layout")
                if selected == original:
                    continue
                overlay = packs.text_overlay(original_section, packs.parse_section(selected[3:])[0])
                texts[index] = (1, overlay)
                expected[index] = selected
                report["text_assets"].append(index)
            else:
                if 0xC20 <= index <= 0xC23 or len(original) != len(translated):
                    raise ValueError("unsupported font or graphic layout change")
                graphics[index] = (original, translated)
                expected[index] = translated
                report["graphic_assets"].append(index)
        except (ValueError, zlib.error) as exception:
            report["rejected"].append({"id": index, "reason": str(exception)})
    if not texts:
        raise ValueError("Italian patch contains no compatible translated text")
    report["source_text_count"] = len(texts)
    completed, pending = supplement(usa, texts, translations)
    texts = {index: (1, packs.text_overlay(packs.parse_section(packs.asset(usa, packs.USA_TABLE, packs.USA_BASE, index)[3:])[0],
                                         packs.parse_section(raw[3:])[0])) for index, raw in completed.items()}
    expected = {index: raw for index, raw in expected.items() if index not in packs.TEXT_IDS} | completed
    font0 = packs.asset(usa, packs.USA_TABLE, packs.USA_BASE, 0xC21)
    # Same generated European font profile, with the native À code mapped to À.
    charset, accents = ITALIAN_CHARSET, ITALIAN_ACCENTS
    data = packs.bundle_bytes("it", charset, accents, font0, [], texts, graphics)
    bundle = packs.read_bundle(data)
    rebuilt = packs.rebuild_text_assets(bundle, usa) | packs.rebuild_graphic_assets(bundle, usa)
    if rebuilt != expected:
        raise ValueError("Italian bundle reconstruction differs from selected source assets")
    # Reference excludes generated font 1; the host verifier checks that separately.
    reference_entries = {i: packs.encode_payload(raw) for i, raw in expected.items()}
    reference_entries[0xC22] = packs.encode_payload(packs.asset(usa, packs.USA_TABLE, packs.USA_BASE, 0xC22))
    reference = packs.pack_bytes("it", charset, accents, reference_entries)
    report.update({"text_count": len(texts), "graphic_count": len(graphics), "rejected_count": len(report["rejected"]),
                   "bundle_sha256": hashlib.sha256(data).hexdigest(), "verification": "PASS",
                   "supplemental_records": len(pending), "catalog": pending,
                   "supplemental_unique": len({row["key"] for row in pending})})
    if translations is not None:
        report["catalog_records_by_kind"] = dict(Counter(translations["entries"][row["key"]]["kind"] for row in pending))
        report["catalog_status"] = translations.get("status", "unspecified")
    return data, reference, report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--usa-rom", type=Path, required=True)
    parser.add_argument("--usa-image", type=Path, required=True)
    parser.add_argument("--bps", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=Path("assets/languages/it.tlpb"))
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--translations", type=Path)
    args = parser.parse_args()
    translations = json.loads(args.translations.read_text(encoding="utf-8")) if args.translations else None
    data, reference, report = build(args.usa_rom.read_bytes(), args.usa_image.read_bytes(), args.bps.read_bytes(), translations)
    if args.translations:
        report["catalog_sha256"] = hashlib.sha256(args.translations.read_bytes()).hexdigest()
    packs.atomic_write(args.output, data)
    packs.atomic_write(args.reference, reference)
    packs.atomic_write(args.report, (json.dumps(report, indent=2) + "\n").encode())
    print(f"Italian: {report['text_count']} text assets, {report['graphic_count']} graphic assets; "
          f"{report['rejected_count']} incompatible source assets rejected; {len(data)} bytes; verification PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
