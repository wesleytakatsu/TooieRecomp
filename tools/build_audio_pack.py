#!/usr/bin/env python3
"""Build the private four-track HD audio prototype; never downloads audio.

The source MP3s remain byte-identical. Playback trims and loops are manifest
metadata measured at 48 kHz. Spiral Mountain layers use native channel groups
observed in the user's gameplay log; both decoders share one timeline.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import zipfile


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = args.source
    if not (source / "download-manifest.json").exists():
        source = source / "Banjo-Tooie HD"
    original = json.loads((source / "download-manifest.json").read_text(encoding="utf-8-sig"))
    hashes = {v["filename"]: v["sha256"] for v in original["files"]}
    files = ["03. Game Select.mp3", "07. Collect Blue Eggs.mp3",
             "197. Spiral Mountain (Sans SFX).mp3", "09. Spiral Mountain (Underwater).mp3"]
    for name in files:
        with (source / name).open("rb") as file:
            if hashlib.file_digest(file, "sha256").hexdigest() != hashes[name]:
                raise ValueError("Source checksum mismatch: " + name)
    def rule(sequence, file, **kwargs):
        return dict(id=sequence, file="audio/" + file, gain=1.0, **kwargs)
    audio = dict(schema_version=2, sequences=[
        rule(0x67, files[0], loop_start_frame=580512, loop_end_frame=4454469),
        rule(0x04, files[1], end_frame=72000),
        rule(0x32, files[2], channel_mask=0xFFFFFFFF, channel_value=0x63FF, native_channel_mask=0x63FF,
             loop_start_frame=576000, loop_end_frame=6336000),
        rule(0x32, files[3], channel_mask=0xFFFFFFFF, channel_value=0x1C00, native_channel_mask=0x1C00,
             loop_start_frame=576000, loop_end_frame=6336000),
    ])
    mod = dict(game_id="bt", id="tooie_hd_audio_prototype", display_name="Banjo-Tooie HD Audio (Prototype)",
               version="0.2.0", minimum_recomp_version="0.1.3", authors=["Grant Kirkhope", "L. Spiro (HD restore)"],
               description="Audio prototype: Game Select, Blue Eggs and synchronized Spiral Mountain surface/underwater layers driven by native channel gains. Requires the audio.json schema 2 prototype build. Unmapped channel arrangements retain original audio.",
               short_description="HD music and Blue Egg jingle prototype")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w", compression=zipfile.ZIP_STORED) as archive:
        archive.writestr("mod.json", json.dumps(mod, indent=2) + "\n")
        archive.writestr("audio.json", json.dumps(audio, indent=2) + "\n")
        for name in files:
            archive.write(source / name, "audio/" + name)
    digest = hashlib.file_digest(args.output.open("rb"), "sha256").hexdigest()
    args.output.with_suffix(args.output.suffix + ".sha256").write_text(digest + "  " + args.output.name + "\n", encoding="utf-8")
    print(f"Built {args.output} ({args.output.stat().st_size} bytes)\nSHA256 {digest}")


if __name__ == "__main__":
    main()
