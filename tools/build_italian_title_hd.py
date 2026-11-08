#!/usr/bin/env python3
"""Build a private RT64 Italian title pack using a complete HD artwork and the player's ROM.

The vendored RT64 texture_hasher computes v5 hashes offline. All USA hashes must
match the supplied reference texture database before any localized mapping is used.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
import struct
import subprocess
import tempfile
import zipfile
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

import build_language_packs as packs

TITLE = 0x8A8
MOD_ID = "tooie_italian_title_hd"


def title_textures(model: bytes) -> list[bytes]:
    setup = struct.unpack_from(">H", model, 8)[0]
    count = struct.unpack_from(">H", model, setup + 4)[0]
    if count != 28 or model[:4] != b"\0\0\0\x0B":
        raise ValueError("unexpected title model")
    base = setup + 8 + count * 8
    images = []
    for index in range(count):
        offset, fmt, width, height = struct.unpack_from(">IHBB", model, setup + 8 + index * 8)
        start, end = base + offset, base + offset + width * height * 4
        if (fmt, width, height) != (8, 32, 32) or end > len(model):
            raise ValueError("unexpected title texture layout")
        images.append(model[start:end])
    return images


def title_layout(model: bytes) -> list[dict]:
    """Use display-list vertex loads; texture order is not spatial order."""
    setup = struct.unpack_from(">H", model, 8)[0]
    offsets = {struct.unpack_from(">I", model, setup + 8 + index * 8)[0]: index for index in range(28)}
    dl = struct.unpack_from(">I", model, 12)[0]
    count = struct.unpack_from(">I", model, dl)[0]
    vertex_base = struct.unpack_from(">I", model, 16)[0] + 24
    texture = None
    placements = {}
    for first, second in struct.iter_unpack(">II", model[dl + 8:dl + 8 + count * 8]):
        if first >> 24 == 0xFD:
            texture = offsets[second & 0xFFFFFF]
        elif first >> 24 == 1:
            vertices = [struct.unpack_from(">hhhHhh4B", model, vertex_base + (second & 0xFFFFFF) + index * 16)
                        for index in range((first >> 12) & 255)]
            if texture is None or len(vertices) not in (3, 4) or texture in placements:
                raise ValueError("unexpected title display-list vertex load")
            placements[texture] = vertices
    if len(placements) != 28:
        raise ValueError("title layout does not cover 28 textures")
    left = min(v[0] for vertices in placements.values() for v in vertices)
    top = max(v[1] for vertices in placements.values() for v in vertices)
    right = max(v[0] for vertices in placements.values() for v in vertices)
    bottom = min(v[1] for vertices in placements.values() for v in vertices)
    if (right - left, top - bottom) != (2000, 1000):
        raise ValueError("unexpected title canvas")
    result = []
    for index in range(28):
        vertices = placements[index]
        x, y = min(v[0] for v in vertices), max(v[1] for v in vertices)
        if (x - left) % 250 or (top - y) % 250:
            raise ValueError("title tile not aligned to its native grid")
        result.append({"column": (x - left) // 250, "row": (top - y) // 250,
                       "polygon": [[(v[0] - left) / 2000, (top - v[1]) / 1000] for v in vertices]})
    return result


def rgba32_tmem(data: bytes) -> bytes:
    """Same split RG/BA banks and odd-row XOR as RT64 RDP::loadWord/loadToTMEMCommon."""
    if len(data) != 32 * 32 * 4:
        raise ValueError("expected one RGBA32 title tile")
    tmem = bytearray(4096)
    for y in range(32):
        for x in range(32):
            source = (y * 32 + x) * 4
            address = (y * 32 + x) * 2
            for channel in (0, 1):
                offset = (address + channel) ^ ((y & 1) * 4)
                tmem[offset] = data[source + channel]
                tmem[offset | 2048] = data[source + 2 + channel]
    return bytes(tmem)


def texture_hashes(hasher: Path, images: list[bytes]) -> list[str]:
    # This isolated synthetic dump is upgraded by the unmodified, vendored RT64 CPU tool.
    with tempfile.TemporaryDirectory(prefix="tooie-title-hashes-") as directory:
        root = Path(directory)
        entries = []
        tile = {"fmt": 0, "siz": 3, "line": 8, "tmem": 0, "palette": 0, "cms": 2, "cmt": 2,
                "masks": 5, "maskt": 5, "shifts": 0, "shiftt": 0, "uls": 0, "ult": 0, "lrs": 124, "lrt": 124}
        for index, image in enumerate(images):
            key = f"{index + 1:016x}"
            entries.append({"hashes": {"rt64": key}, "path": key})
            (root / (key + ".v4.tmem")).write_bytes(rgba32_tmem(image))
            (root / (key + ".v4.tile.json")).write_text(json.dumps({"tile": tile, "tlut": "None", "width": 32, "height": 32}))
        config = {"hashVersion": 4, "configurationVersion": 3, "autoPath": "rt64", "defaultOperation": "preload", "defaultShift": "none"}
        (root / "rt64.json").write_text(json.dumps({"configuration": config, "textures": entries}))
        result = subprocess.run([str(hasher.resolve()), str(root), "--upgrade"], capture_output=True, text=True)
        if result.returncode:
            raise ValueError("RT64 texture_hasher failed: " + result.stderr)
        database = json.loads((root / "rt64.json").read_text())
        if database["configuration"]["hashVersion"] != 5:
            raise ValueError("unexpected RT64 hash version")
        lookup = {entry["path"]: entry["hashes"]["rt64"] for entry in database["textures"]}
        return [lookup[f"{index + 1:016x}"] for index in range(len(images))]


def reference_hashes(path: Path) -> set[str]:
    with zipfile.ZipFile(path) as archive:
        names = [name for name in archive.namelist() if name == "rt64.json" or name.endswith("/rt64.json")]
        if len(names) != 1:
            raise ValueError("reference pack must have exactly one RT64 database")
        data = json.loads(archive.read(names[0]))
    if data["configuration"]["hashVersion"] != 5:
        raise ValueError("reference pack must use RT64 hash version 5")
    return {entry["hashes"]["rt64"].lower() for entry in data["textures"]}


def png_bytes(image: Image.Image) -> bytes:
    stream = io.BytesIO()
    image.save(stream, format="PNG")
    return stream.getvalue()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--usa-image", type=Path, required=True)
    parser.add_argument("--bundle", type=Path, default=Path("assets/languages/it.tlpb"))
    parser.add_argument("--artwork", type=Path, required=True)
    parser.add_argument("--texture-hasher", type=Path, required=True)
    parser.add_argument("--reference-pack", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--preview", type=Path, required=True)
    args = parser.parse_args()
    usa = packs.checked_image(args.usa_image, packs.USA_SIZE, packs.USA_SHA1)
    bundle = packs.read_bundle(args.bundle.read_bytes())
    if bundle["tag"] != "it" or TITLE not in bundle["assets"]:
        raise ValueError("Italian HD title model missing from bundle")
    original = packs.asset(usa, packs.USA_TABLE, packs.USA_BASE, TITLE)
    localized = packs.rebuild_graphic_assets({"assets": {TITLE: bundle["assets"][TITLE]}}, usa)[TITLE]
    old_images, new_images = title_textures(original), title_textures(localized)
    hashes = texture_hashes(args.texture_hasher, old_images + new_images)
    original_hashes, localized_hashes = hashes[:28], hashes[28:]
    known = reference_hashes(args.reference_pack)
    if not set(original_hashes).issubset(known):
        raise ValueError("USA title hashes do not match the reference pack")
    if len(set(localized_hashes)) != 28 or set(localized_hashes) & known:
        raise ValueError("Italian HD hashes overlap another texture or each other")
    layout = title_layout(original)
    if title_layout(localized) != layout:
        raise ValueError("Italian title geometry differs from USA")
    artwork = Image.open(args.artwork).convert("RGBA")
    if artwork.width != artwork.height * 2 or artwork.width < 1024:
        raise ValueError("HD artwork must be at least 1024px wide and exactly 2:1")
    if artwork.getextrema()[3][0] != 0 or artwork.getextrema()[3][1] == 0:
        raise ValueError("title artwork must have visible content and transparency")
    artwork = artwork.resize((2048, 1024), Image.Resampling.LANCZOS)
    coverage = Image.new("L", artwork.size)
    draw = ImageDraw.Draw(coverage)
    for placement in layout:
        draw.polygon([(round(x * artwork.width), round(y * artwork.height)) for x, y in placement["polygon"]], fill=255)
    preview = artwork.copy()
    preview.putalpha(ImageChops.multiply(artwork.getchannel("A"), coverage))
    significant = artwork.getchannel("A").point(lambda value: 255 if value > 32 else 0)
    lost = ImageChops.subtract(significant, coverage).histogram()[255]
    total = significant.histogram()[255]
    if lost > total * 0.005:
        raise ValueError(f"HD artwork exceeds the native model silhouette: {lost}/{total} pixels")
    files = {}
    replacements = []
    for index, placement in enumerate(layout):
        x, y = placement["column"] * 256, placement["row"] * 256
        tile = artwork.crop((x, y, x + 256, y + 256)).transpose(Image.Transpose.FLIP_TOP_BOTTOM)
        name = f"title/{index:02d}.png"
        files[name] = png_bytes(tile)
        replacements.append({"hashes": {"rt64": localized_hashes[index]}, "path": name[:-4]})
    config = {"hashVersion": 5, "configurationVersion": 3, "autoPath": "rt64", "defaultOperation": "preload", "defaultShift": "none"}
    files["rt64.json"] = (json.dumps({"configuration": config, "textures": replacements}, indent=2) + "\n").encode()
    mod = {"id": MOD_ID, "game_id": "bt", "display_name": "Italian Title HD", "version": "1.0.0",
           "minimum_recomp_version": "0.1.6", "enabled_by_default": True,
           "authors": ["Rulesless (original Italian localization)", "TooieRecomp (HD title adaptation)"],
           "short_description": "High-resolution Italian title",
           "description": "HD title artwork for the Italian game language. Rareware and Dolby logos remain original."}
    files["mod.json"] = (json.dumps(mod, indent=2) + "\n").encode()
    files["NOTICE.txt"] = ("Original Italian localization: Rulesless v0.9.\n"
                           "https://letraduzionidirulesless.wordpress.com/banjo-tooie/\n"
                           "HD title adapted using the built-in image_gen tool from the user's local game/HD artwork.\n"
                           "Private generated derivative asset; no redistribution clearance is implied.\n"
                           "Original game rights remain with Nintendo/Rare.\n").encode()
    output = io.BytesIO()
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, (2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)
    data = output.getvalue()
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        if archive.testzip() is not None or set(archive.namelist()) != set(files):
            raise ValueError("HD pack ZIP verification failed")
        if any(archive.read(name) != expected for name, expected in files.items()):
            raise ValueError("HD pack entry verification failed")
    report = {"mod_id": MOD_ID, "tiles": 28, "tile_size": 256, "artwork_size": [2048, 1024],
              "usa_hash_matches": 28, "cross_language_hash_collisions": 0,
              "source_artwork_sha256": hashlib.sha256(args.artwork.read_bytes()).hexdigest(),
              "bundle_sha256": hashlib.sha256(args.bundle.read_bytes()).hexdigest(),
              "pack_sha256": hashlib.sha256(data).hexdigest(), "clipped_significant_pixels": lost,
              "verification": "PASS", "entries": replacements}
    packs.atomic_write(args.output, data)
    packs.atomic_write(args.preview, png_bytes(preview))
    packs.atomic_write(args.report, (json.dumps(report, indent=2) + "\n").encode())
    print(f"Italian HD title: 28 tiles at 256x256; 28/28 baseline hashes match; 0 locale collisions; {len(data)} bytes; PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
