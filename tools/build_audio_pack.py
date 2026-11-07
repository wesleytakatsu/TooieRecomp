#!/usr/bin/env python3
"""Build a Banjo-Tooie music pack ZIP from an album you own; never downloads or ships audio.

    build_audio_pack.py <album-folder> <output.zip> [--mapping tools/audio/banjo-tooie-hd.json] [--thumbnail image.png]

The mapping lists each album file (with its SHA-256), the sequence and channel arrangement it replaces and its
measured playback bounds. Files are copied byte for byte (ZIP_STORED); the album's own MP3s must match the hashes.
The resulting ZIP contains copyrighted music: keep it private. --thumbnail adds the image the Mods menu shows
(`thumb.png`, scaled to 512x512 when Pillow is installed, otherwise a PNG copied as is).
"""
from __future__ import annotations
import argparse
import hashlib
import io
import json
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parent


def sha256(path: Path) -> str:
    with path.open("rb") as file:
        return hashlib.file_digest(file, "sha256").hexdigest()


def thumbnail_png(path: Path) -> bytes:
    try:
        from PIL import Image
    except ImportError:
        data = path.read_bytes()
        if not data.startswith(b"\x89PNG"):
            raise SystemExit("thumbnail must be a PNG (or install Pillow to convert it)")
        return data
    with Image.open(path) as image:
        image = image.convert("RGBA")
        side = min(image.size)
        left, top = (image.width - side) // 2, (image.height - side) // 2
        image = image.crop((left, top, left + side, top + side)).resize((512, 512), Image.LANCZOS)
        out = io.BytesIO()
        image.save(out, "PNG", optimize=True)
        return out.getvalue()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("album", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--mapping", type=Path, default=ROOT / "audio" / "banjo-tooie-hd.json")
    parser.add_argument("--thumbnail", type=Path, help="image shown in the Mods menu (stored as thumb.png)")
    args = parser.parse_args()
    mapping = json.loads(args.mapping.read_text(encoding="utf-8"))
    album = args.album
    if not any(album.glob("*.mp3")) and (album / mapping.get("album_folder", "")).is_dir():
        album = album / mapping["album_folder"]
    sequences = []
    files = []
    for track in mapping["tracks"]:
        source = album / track["file"]
        if not source.is_file():
            raise SystemExit(f"missing album file: {track['file']}")
        if sha256(source) != track["sha256"]:
            raise SystemExit(f"album file differs from the mapped one: {track['file']}")
        packed = "audio/" + track["file"]
        files.append((source, packed))
        bounds = {k: track[k] for k in ("start_frame", "end_frame", "loop_start_frame", "loop_end_frame", "gain", "bpm") if k in track}
        for rule in track["rules"]:
            entry = {"id": rule["id"], "file": packed, **bounds}
            for key in ("channel_mask", "channel_value", "native_channel_mask"):
                if key in rule:
                    entry[key] = rule[key]
            sequences.append(entry)
    audio = {"schema_version": 2, "sequences": sequences}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w", compression=zipfile.ZIP_STORED) as archive:
        archive.writestr("mod.json", json.dumps(mapping["mod"], indent=2) + "\n")
        archive.writestr("audio.json", json.dumps(audio, indent=2) + "\n")
        if args.thumbnail:
            archive.writestr("thumb.png", thumbnail_png(args.thumbnail))
        for source, packed in files:
            archive.write(source, packed)
    digest = sha256(args.output)
    args.output.with_suffix(args.output.suffix + ".sha256").write_text(f"{digest}  {args.output.name}\n", encoding="utf-8")
    print(f"Built {args.output} ({args.output.stat().st_size} bytes, {len(files)} MP3s, {len(sequences)} rules)\nSHA256 {digest}")


if __name__ == "__main__":
    main()
