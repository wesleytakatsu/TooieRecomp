#!/usr/bin/env python3
"""Build local language packs from private ROMs.

    build_language_packs.py --pal-rom PATH [--ptbr-bps PATH] [--usa-image PATH]
                            [--ptbr-bundle PATH] [--out-dir DIR] [--report PATH]
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import struct
import sys
import tempfile
import zlib
from pathlib import Path

ASSET_COUNT = 13926  # Number of entries before the sentinel.
PAL_SIZE = 33554432  # Expected European ROM size.
PAL_SHA1 = "93bf2fac1387320ad07251cb4b64fd36bac1d7a6"  # European ROM digest.
USA_SIZE = 35098224  # Expected decompressed USA image size.
USA_SHA1 = "af479771ff5e91ce3732f3d84146614ab71d8d79"  # USA image digest.
CONTROL_TYPES = {2, 4, 5, 7, 8, 9, 11}  # Records whose data must match.
USA_TABLE = 0x5188  # USA asset table (big-endian entries).
USA_BASE = 0x12B24  # USA asset payload base.
PAL_TABLE = 0x5148  # European asset table.
PAL_BASE = 0x12AE4  # European asset payload base.
TEXT_IDS = range(0x0CDA, 0x1C6C)  # Text asset IDs in both builds.
FONT_ASSETS = {"fr": [0xC22], "de": [0xC22], "es": [0xC22]}  # Fonts copied into each pack.
PAL_CORE2_DATA = 0x1E8AD81  # European core2 data segment (u16 size/16 + raw deflate).
CHARSET_OFFSET = 0x3928  # Font 1 character list inside that segment.
ACCENT_MAP_OFFSET = 0x3974  # Font 1 accent map (codes 0x5B..0x78) inside that segment.
CHARSET_PREFIX = b'0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZ@%?()<>".;-!/'  # Font 1 base characters.
# Project fixes to the pt-BR translation, keyed by (asset ID, record type). The title-screen legal line must fit the
# USA layout (about 38 characters); `b` is the font-0 code for É.
PTBR_TEXT_FIXES = {
    (0x186C, 0x85): b"A LOGO RAREWARE b MARCA DA RARE.\x00",
}
Record = tuple[int, int | None, bytes]
Section = list[list[Record]]


def apply_ptbr_fixes(index: int, section: Section) -> Section:
    fixed: Section = []
    for group in section:
        records: list[Record] = []
        for kind, param, data in group:
            records.append((kind, param, PTBR_TEXT_FIXES.get((index, kind), data)))
        fixed.append(records)
    return fixed


def checked_image(path: Path, size: int, digest: str) -> bytes:
    data = path.read_bytes()
    if len(data) != size or hashlib.sha1(data).hexdigest() != digest:
        raise ValueError(f"invalid image size or SHA-1: {path}")
    return data


def inflate(payload: bytes) -> bytes:
    if len(payload) < 2:
        raise ValueError("truncated compressed payload")
    decoder = zlib.decompressobj(-15)
    raw = decoder.decompress(payload[2:]) + decoder.flush()
    if not decoder.eof or (len(raw) + 15) // 16 != int.from_bytes(payload[:2], "big"):
        raise ValueError("invalid compressed payload or size")
    return raw


def encode_payload(raw: bytes) -> bytes:
    units = (len(raw) + 15) // 16
    if units > 65535:
        raise ValueError("asset too large")
    compressor = zlib.compressobj(9, zlib.DEFLATED, -15)
    payload = struct.pack(">H", units) + compressor.compress(raw) + compressor.flush()
    return payload + b"\xAA" * (-len(payload) % 8)


def asset(image: bytes, table: int, base: int, index: int, partial: bool = False) -> bytes:
    if not 0 <= index < ASSET_COUNT:
        raise ValueError("asset ID out of range")
    a, b = struct.unpack_from(">II", image, table + 4 * index)
    start = base + (a >> 8) * 4
    end = base + (b >> 8) * 4
    if not base <= start <= end <= len(image):
        raise ValueError(f"invalid asset bounds for {index:#x}")
    payload = image[start:end]
    if partial:  # Patched text streams may end after the translated section.
        if len(payload) < 2:
            raise ValueError("truncated compressed payload")
        decoder = zlib.decompressobj(-15)
        return decoder.decompress(payload[2:]) + decoder.flush()
    return inflate(payload)


def parse_section(data: bytes, groups: int | None = None) -> tuple[Section, int]:
    pos = 0
    result: Section = []
    while pos < len(data) and (groups is None or len(result) < groups):
        count = data[pos]
        pos += 1
        group: list[Record] = []
        for _ in range(count):
            if pos >= len(data):
                raise ValueError("truncated record type")
            kind = data[pos]
            pos += 1
            if kind not in CONTROL_TYPES and kind != 3 and kind < 128:
                raise ValueError(f"unknown record type {kind:#x}")
            param = None
            if kind == 3:
                if pos >= len(data):
                    raise ValueError("truncated record parameter")
                param = data[pos]
                pos += 1
            if pos >= len(data):
                raise ValueError("truncated record length")
            length = data[pos]
            pos += 1
            if pos + length > len(data):
                raise ValueError("truncated record data")
            group.append((kind, param, data[pos:pos + length]))
            pos += length
        result.append(group)
    if groups is not None and len(result) != groups:
        raise ValueError("truncated groups")
    return result, pos


def serialize_section(section: Section) -> bytes:
    out = bytearray()
    for group in section:
        if len(group) > 255:
            raise ValueError("too many records in group")
        out.append(len(group))
        for kind, param, data in group:
            if len(data) > 255:
                raise ValueError("record data exceeds 255 bytes")
            if kind not in CONTROL_TYPES and kind != 3 and kind < 128:
                raise ValueError("unknown record type")
            if (kind == 3) != (param is not None):
                raise ValueError("invalid record parameter")
            out.append(kind)
            if param is not None:
                out.append(param)
            out.append(len(data))
            out.extend(data)
    return bytes(out)


def shape(section: Section) -> list[list[tuple[int, int | None, bytes | None]]]:
    return [[(kind, param, data if kind in CONTROL_TYPES else None)
             for kind, param, data in group] for group in section]


def choose_text(usa: Section, english: Section, language: Section) -> tuple[int, bytes | None, int]:
    if shape(usa) == shape(language):
        return 1, b"\x01\x03\x00" + serialize_section(language), 0
    if len(usa) == 1 and len(language) == 1 and all(r[0] >= 128 for r in usa[0]):
        translated: list[Record] = []
        count = 0
        for kind, param, data in usa[0]:
            matches = [r for group in english for r in group if r[0] == kind and r[2] == data]
            targets = [r for r in language[0] if r[0] == kind]
            if matches and targets:
                data = targets[0][2]
                count += 1
            translated.append((kind, param, data))
        if count:
            return 2, b"\x01\x03\x00" + serialize_section([translated]), count
    positional = positional_text(usa, english, language)
    if positional is not None:
        return 3, b"\x01\x03\x00" + serialize_section(positional), 0
    return 0, None, 0


def is_text(record: Record) -> bool:
    return record[0] == 3 or record[0] >= 128


def positional_text(usa: Section, english: Section, language: Section) -> Section | None:
    """Same layout where only text record types differ (e.g. another speaker): keep USA types, take language text."""
    if [len(g) for g in usa] != [len(g) for g in english] or [len(g) for g in usa] != [len(g) for g in language]:
        return None
    result: Section = []
    for usa_group, english_group, language_group in zip(usa, english, language):
        group: list[Record] = []
        for usa_record, english_record, language_record in zip(usa_group, english_group, language_group):
            if not is_text(usa_record):
                if usa_record != english_record or usa_record != language_record:
                    return None
                group.append(usa_record)
            elif is_text(english_record) and is_text(language_record) and english_record[2] == usa_record[2]:
                if usa_record[1] != english_record[1] or language_record[1] != english_record[1]:
                    return None
                group.append((usa_record[0], usa_record[1], language_record[2]))
            else:
                return None
        result.append(group)
    return result


def european_sections(raw: bytes) -> list[Section]:
    if len(raw) < 9 or raw[0] != 4:
        raise ValueError("invalid European text header")
    offsets = list(struct.unpack_from("<4H", raw, 1))
    if offsets[0] != 9 or offsets != sorted(offsets) or offsets[-1] > len(raw):
        raise ValueError("invalid European section offsets")
    sections = []
    for start, end in zip(offsets, offsets[1:] + [len(raw)]):
        sections.append(parse_section(raw[start:end])[0])
    return sections


def read_varint(patch: bytes, pos: int, end: int) -> tuple[int, int]:
    value = 0
    shift = 1
    while pos < end:
        byte = patch[pos]
        pos += 1
        value += (byte & 127) * shift
        if byte & 128:
            return value, pos
        shift <<= 7
        value += shift
    raise ValueError("truncated BPS varint")


def apply_bps(source: bytes, patch: bytes) -> bytes:
    if len(patch) < 16 or patch[:4] != b"BPS1":
        raise ValueError("invalid BPS header")
    end = len(patch) - 12
    source_crc, target_crc, patch_crc = struct.unpack_from("<III", patch, end)
    if zlib.crc32(patch[:-4]) != patch_crc:
        raise ValueError("BPS patch CRC mismatch")
    if zlib.crc32(source) != source_crc:
        raise ValueError("BPS source CRC mismatch")
    source_size, pos = read_varint(patch, 4, end)
    target_size, pos = read_varint(patch, pos, end)
    metadata_size, pos = read_varint(patch, pos, end)
    if source_size != len(source) or pos + metadata_size > end:
        raise ValueError("invalid BPS source size or metadata")
    pos += metadata_size
    target = bytearray()
    source_relative = 0
    target_relative = 0
    while pos < end and len(target) < target_size:
        action, pos = read_varint(patch, pos, end)
        mode = action & 3
        length = (action >> 2) + 1
        if len(target) + length > target_size:
            raise ValueError("BPS action exceeds target size")
        if mode == 0:
            start = len(target)
            if start + length > len(source):
                raise ValueError("BPS SourceRead out of bounds")
            target.extend(source[start:start + length])
        elif mode == 1:
            if pos + length > end:
                raise ValueError("truncated BPS TargetRead")
            target.extend(patch[pos:pos + length])
            pos += length
        else:
            offset, pos = read_varint(patch, pos, end)
            delta = -(offset >> 1) if offset & 1 else offset >> 1
            if mode == 2:
                source_relative += delta
                if source_relative < 0 or source_relative + length > len(source):
                    raise ValueError("BPS SourceCopy out of bounds")
                target.extend(source[source_relative:source_relative + length])
                source_relative += length
            else:
                target_relative += delta
                for _ in range(length):
                    if target_relative < 0 or target_relative >= len(target):
                        raise ValueError("BPS TargetCopy out of bounds")
                    target.append(target[target_relative])
                    target_relative += 1
    if pos != end or len(target) != target_size or zlib.crc32(target) != target_crc:
        raise ValueError("invalid BPS target size, actions, or CRC")
    return bytes(target)


def pack_bytes(tag: str, charset: bytes, accents: bytes, entries: dict[int, bytes]) -> bytes:
    if not tag or len(tag) > 15 or not all(c.isascii() and (c.isalnum() or c == "-") for c in tag):
        raise ValueError("invalid language tag")
    if not 1 <= len(entries) <= ASSET_COUNT or len(charset) > 255:
        raise ValueError("invalid pack entry count or charset")
    if len(accents) != (30 if charset else 0):
        raise ValueError("invalid accent map length")
    out = bytearray(struct.pack("<4sI16sIII", b"TLPK", 1, tag.encode("ascii"), USA_SIZE, len(entries), len(charset)))
    out.extend(charset)
    out.extend(accents)
    for index, payload in sorted(entries.items()):
        if not 0 <= index < ASSET_COUNT or len(payload) < 8 or len(payload) % 8:
            raise ValueError("invalid pack entry")
        out.extend(struct.pack("<II", index, len(payload)))
        out.extend(payload)
    return bytes(out)


def font_patches(base: bytes, translated: bytes) -> list[tuple[int, bytes]]:
    if len(base) != len(translated):
        raise ValueError("font 0 size mismatch")
    changed = [i for i, (a, b) in enumerate(zip(base, translated)) if a != b]
    runs: list[tuple[int, bytes]] = []
    if changed:
        start = previous = changed[0]
        for index in changed[1:]:
            if index - previous > 4:
                runs.append((start, translated[start:previous + 1]))
                start = index
            previous = index
        runs.append((start, translated[start:previous + 1]))
    return runs


def apply_font_patches(base: bytes, size: int, crc: int, patches: list[tuple[int, bytes]]) -> bytes:
    if len(base) != size or zlib.crc32(base) != crc:
        raise ValueError("font 0 base size or CRC mismatch")
    result = bytearray(base)
    end = 0
    for offset, data in patches:
        if not data or offset < end or offset + len(data) > size:
            raise ValueError("invalid font 0 patch")
        result[offset:offset + len(data)] = data
        end = offset + len(data)
    return bytes(result)


def text_overlay(usa: Section, translated: Section) -> bytes:
    if [len(group) for group in usa] != [len(group) for group in translated]:
        raise ValueError("overlay layout mismatch")
    out = bytearray()
    for original_group, translated_group in zip(usa, translated):
        for original, target in zip(original_group, translated_group):
            if original[:2] != target[:2] or (not is_text(original) and original != target):
                raise ValueError("overlay record mismatch")
            if is_text(original):
                data = target[2]
                if data == original[2]:
                    out.append(0)
                else:
                    out.extend((1, len(data)))
                    out.extend(data)
    return bytes(out)


def apply_text_entry(raw_usa: bytes, kind: int, data: bytes) -> bytes:
    if not raw_usa.startswith(b"\x01\x03\x00"):
        raise ValueError("invalid USA text header")
    if kind == 0:
        parse_section(data)
        return b"\x01\x03\x00" + data
    if kind != 1:
        raise ValueError("invalid text entry kind")
    section = parse_section(raw_usa[3:])[0]
    pos = 0
    result: Section = []
    for group in section:
        new_group: list[Record] = []
        for record in group:
            if is_text(record):
                if pos >= len(data):
                    raise ValueError("overlay record count mismatch")
                flag = data[pos]
                pos += 1
                if flag == 1:
                    if pos >= len(data) or pos + 1 + data[pos] > len(data):
                        raise ValueError("truncated overlay record")
                    length = data[pos]
                    pos += 1
                    record = (record[0], record[1], data[pos:pos + length])
                    pos += length
                elif flag != 0:
                    raise ValueError("invalid overlay flag")
            new_group.append(record)
        result.append(new_group)
    if pos != len(data):
        raise ValueError("overlay record count mismatch")
    return b"\x01\x03\x00" + serialize_section(result)


def bundle_bytes(tag: str, charset: bytes, accents: bytes, base: bytes,
                 patches: list[tuple[int, bytes]], entries: dict[int, tuple[int, bytes]]) -> bytes:
    if not tag or len(tag) > 15 or not all(c.isascii() and (c.isalnum() or c == "-") for c in tag):
        raise ValueError("invalid language tag")
    if not 1 <= len(charset) <= 255 or len(accents) != 30:
        raise ValueError("invalid font map")
    apply_font_patches(base, len(base), zlib.crc32(base), patches)
    out = bytearray(struct.pack("<4sI16sI", b"TLPB", 1, tag.encode("ascii"), len(charset)))
    out.extend(charset)
    out.extend(accents)
    out.extend(struct.pack("<III", len(base), zlib.crc32(base), len(patches)))
    for offset, data in patches:
        out.extend(struct.pack("<II", offset, len(data)))
        out.extend(data)
    out.extend(struct.pack("<I", len(entries)))
    for index, (kind, data) in sorted(entries.items()):
        if not 0 <= index < ASSET_COUNT or kind not in (0, 1):
            raise ValueError("invalid bundle entry")
        out.extend(struct.pack("<IBI", index, kind, len(data)))
        out.extend(data)
    return bytes(out)


def read_bundle(data: bytes) -> dict:
    pos = 0
    def take(length: int) -> bytes:
        nonlocal pos
        if length < 0 or pos + length > len(data):
            raise ValueError("truncated bundle")
        result = data[pos:pos + length]
        pos += length
        return result
    def number() -> int:
        return struct.unpack("<I", take(4))[0]
    magic, version, raw_tag, charset_length = struct.unpack("<4sI16sI", take(28))
    tag_bytes = raw_tag.split(b"\0", 1)[0]
    if magic != b"TLPB" or version != 1 or not tag_bytes or raw_tag != tag_bytes.ljust(16, b"\0"):
        raise ValueError("invalid bundle header")
    try:
        tag = tag_bytes.decode("ascii")
    except UnicodeDecodeError as exc:
        raise ValueError("invalid language tag") from exc
    if not all(c.isalnum() or c == "-" for c in tag) or not 1 <= charset_length <= 255:
        raise ValueError("invalid bundle tag or charset")
    charset = take(charset_length)
    accents = take(30)
    size, crc, count = number(), number(), number()
    patches = []
    end = 0
    for _ in range(count):
        offset, length = number(), number()
        if not length or offset < end or offset + length > size:
            raise ValueError("invalid font 0 patch")
        patches.append((offset, take(length)))
        end = offset + length
    entries = {}
    previous = -1
    for _ in range(number()):
        index = number()
        kind = take(1)[0]
        length = number()
        if index <= previous or index >= ASSET_COUNT or kind not in (0, 1):
            raise ValueError("invalid bundle entry")
        entries[index] = (kind, take(length))
        previous = index
    if pos != len(data):
        raise ValueError("trailing bundle data")
    return {"tag": tag, "charset": charset, "accents": accents, "font_size": size,
            "font_crc": crc, "patches": patches, "entries": entries}


def rebuild_text_assets(bundle: dict, usa: bytes) -> dict[int, bytes]:
    return {index: apply_text_entry(asset(usa, USA_TABLE, USA_BASE, index), kind, data)
            for index, (kind, data) in bundle["entries"].items()}


def atomic_write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    name = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix="." + path.name + ".", delete=False) as file:
            name = file.name
            file.write(data)
        os.replace(name, path)
    finally:
        if name is not None and os.path.exists(name):
            os.unlink(name)


def app_dir() -> Path:
    if os.name == "nt":
        local = os.environ.get("LOCALAPPDATA")
        if local is None:
            raise ValueError("LOCALAPPDATA is not set; pass --usa-image and --out-dir")
        return Path(local) / "TooieRecompiled"
    folder = os.environ.get("APP_FOLDER_PATH")
    return Path(folder) if folder is not None else Path.home() / ".config" / "TooieRecompiled"


def build(pal: bytes, usa: bytes, patched: bytes | None, out_dir: Path, bundle_path: Path) -> dict:
    core = inflate(pal[PAL_CORE2_DATA:])
    end = core.find(b"\0", CHARSET_OFFSET)
    charset = core[CHARSET_OFFSET:end] if end >= 0 else b""
    accents = core[ACCENT_MAP_OFFSET:ACCENT_MAP_OFFSET + 30]
    if len(charset) != 73 or not charset.startswith(CHARSET_PREFIX) or len(accents) != 30:
        raise ValueError("invalid European font map")
    tags = ["fr", "de", "es"] + (["pt-BR"] if patched is not None else [])
    entries: dict[str, dict[int, bytes]] = {tag: {} for tag in tags}
    bundle_entries: dict[int, tuple[int, bytes]] = {}
    expected_text: dict[int, bytes] = {}
    report = {tag: {"rule1": 0, "rule2": 0, "rule3": 0, "records_translated": 0, "english_kept": []} for tag in tags}
    for index in TEXT_IDS:
        a = struct.unpack_from(">I", usa, USA_TABLE + 4 * index)[0]
        if a & 0xFF != 0x13:
            continue
        raw_usa = asset(usa, USA_TABLE, USA_BASE, index)
        if not raw_usa.startswith(b"\x01\x03\x00"):
            continue
        if patched is not None:
            expected_text[index] = raw_usa
        pal_start, pal_end = struct.unpack_from(">II", pal, PAL_TABLE + 4 * index)
        if pal_start >> 8 == pal_end >> 8:
            for tag in tags:
                report[tag]["english_kept"].append(f"0x{index:04X}")
            continue
        try:
            us = parse_section(raw_usa[3:])[0]
            sections = european_sections(asset(pal, PAL_TABLE, PAL_BASE, index))
            languages = {"fr": sections[1], "de": sections[2], "es": sections[3]}
            if patched is not None:
                raw_pt = asset(patched, PAL_TABLE, PAL_BASE, index, partial=True)
                if len(raw_pt) < 9 or raw_pt[0] != 4 or struct.unpack_from("<H", raw_pt, 1)[0] != 9:
                    raise ValueError("invalid patched text header")
                languages["pt-BR"] = apply_ptbr_fixes(index, parse_section(raw_pt[9:], len(sections[0]))[0])
            for tag in tags:
                rule, result, count = choose_text(us, sections[0], languages[tag])
                if result is None:
                    report[tag]["english_kept"].append(f"0x{index:04X}")
                else:
                    if tag == "pt-BR":
                        expected_text[index] = result
                        kind = 0 if rule == 1 else 1
                        data = result[3:] if kind == 0 else text_overlay(us, parse_section(result[3:])[0])
                        bundle_entries[index] = (kind, data)
                    else:
                        entries[tag][index] = encode_payload(result)
                    report[tag][f"rule{rule}"] += 1
                    report[tag]["records_translated"] += count
        except (ValueError, zlib.error, struct.error) as exc:
            raise ValueError(f"text asset {index:#x}: {exc}") from exc
    for tag in ("fr", "de", "es"):
        for index in FONT_ASSETS[tag]:
            entries[tag][index] = encode_payload(asset(pal, PAL_TABLE, PAL_BASE, index))
        output = out_dir / f"{tag}.tlp"
        data = pack_bytes(tag, charset, accents, entries[tag])
        atomic_write(output, data)
        report[tag]["output"] = str(output)
        report[tag]["size"] = len(data)
    if patched is not None:
        font_base = asset(usa, USA_TABLE, USA_BASE, 0xC21)
        font_target = asset(patched, PAL_TABLE, PAL_BASE, 0xC21)
        patches = font_patches(font_base, font_target)
        data = bundle_bytes("pt-BR", charset, accents, font_base, patches, bundle_entries)
        atomic_write(bundle_path, data)
        bundle = read_bundle(bundle_path.read_bytes())
        rebuilt = rebuild_text_assets(bundle, usa)
        for index, expected in expected_text.items():
            actual = rebuilt.get(index)
            if actual is None:
                actual = asset(usa, USA_TABLE, USA_BASE, index)
            if actual != expected:
                raise ValueError(f"bundle verification failed for text asset {index:#x}")
        if apply_font_patches(font_base, bundle["font_size"], bundle["font_crc"], bundle["patches"]) != font_target:
            raise ValueError("bundle verification failed for font 0")
        report["pt-BR"].update({"output": str(bundle_path), "size": len(data),
                                "kind0": sum(kind == 0 for kind, _ in bundle_entries.values()),
                                "kind1": sum(kind == 1 for kind, _ in bundle_entries.values()),
                                "font_runs": len(patches), "font_bytes": sum(len(run) for _, run in patches),
                                "verification": "PASS"})
    return report


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pal-rom", type=Path, required=True)
    parser.add_argument("--ptbr-bps", type=Path)
    parser.add_argument("--ptbr-bundle", type=Path, default=Path(__file__).resolve().parent.parent / "assets/languages/pt-BR.tlpb")
    parser.add_argument("--usa-image", type=Path)
    parser.add_argument("--out-dir", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args(argv)
    try:
        # Defaults are resolved only when needed, so explicit paths work without LOCALAPPDATA.
        if args.usa_image is None:
            args.usa_image = app_dir() / "bt.n64.us.1.0.z64"
        if args.out_dir is None:
            args.out_dir = app_dir() / "languages"
        pal = checked_image(args.pal_rom, PAL_SIZE, PAL_SHA1)
        usa = checked_image(args.usa_image, USA_SIZE, USA_SHA1)
        patched = apply_bps(pal, args.ptbr_bps.read_bytes()) if args.ptbr_bps else None
        report = build(pal, usa, patched, args.out_dir, args.ptbr_bundle)
        for tag, item in report.items():
            print(f"{tag}: rule 1: {item['rule1']}; rule 2: {item['rule2']} assets ({item['records_translated']} records); rule 3: {item['rule3']}; English kept: {len(item['english_kept'])} " + ", ".join(item["english_kept"]))
            print(f"  {item['output']} ({item['size']} bytes)")
            if tag == "pt-BR":
                print(f"  kind 0: {item['kind0']}; kind 1: {item['kind1']}; font 0: {item['font_runs']} runs, {item['font_bytes']} bytes; verification: {item['verification']}")
        if args.report:
            atomic_write(args.report, (json.dumps(report, indent=2) + "\n").encode("utf-8"))
    except (OSError, ValueError, zlib.error, struct.error) as exc:
        print(f"build_language_packs: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
