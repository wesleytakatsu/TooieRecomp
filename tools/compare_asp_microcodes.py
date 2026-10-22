#!/usr/bin/env python3
"""P07-T02: compare Banjo-Tooie n_aspMain blob with Banjo-Kazooie config sizes.

Usage (from TooieRecomp or any cwd):
  python3 tools/compare_asp_microcodes.py \\
      [--tooie-rom PATH] [--bk-rom PATH]

Writes a short summary to stdout. Does not require BK ROM; when absent,
prints Tooie-only inventory and size delta vs known BK RSPRecomp config.
"""
from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path

from tooie_paths import DECOMP

# From banjo-tooie-Recomp/baserom.us.yaml (decompressed layout)
TOOIE_TEXT_OFF = 0x1E4F3B0
TOOIE_TEXT_END = 0x1E50B80  # rspboot
TOOIE_DATA_OFF = 0x1E59B40
TOOIE_DATA_END = 0x1E5A280

# From BanjoRecomp/n_aspMain.us.rev0.toml
BK_TEXT_SIZE = 0xC60
BK_TEXT_ADDRESS = 0x04001080
BK_JUMP_TABLE = [
    0x10F0, 0x13A0, 0x11A0, 0x1A68, 0x11CC, 0x17F0, 0x120C, 0x1280,
    0x134C, 0x124C, 0x1C88, 0x12D8, 0x12B0, 0x1388,
]


def sha16(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()[:16]


def halfword_table(data: bytes, start: int = 0x0E, count: int = 17) -> list[int]:
    out = []
    for i in range(count):
        off = start + i * 2
        if off + 2 > len(data):
            break
        out.append(struct.unpack_from(">H", data, off)[0])
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    root = Path(__file__).resolve().parents[1]
    ap.add_argument(
        "--tooie-rom",
        type=Path,
        default=DECOMP / "decompressed.us.z64",
    )
    ap.add_argument(
        "--bk-rom",
        type=Path,
        default=None,
        help="Optional Banjo-Kazooie decompressed ROM for byte compare",
    )
    ap.add_argument("--bk-text-offset", type=lambda x: int(x, 0), default=0xF4DDC0)
    args = ap.parse_args()

    if not args.tooie_rom.is_file():
        print(f"error: Tooie ROM not found: {args.tooie_rom}", file=sys.stderr)
        return 1

    rom = args.tooie_rom.read_bytes()
    text = rom[TOOIE_TEXT_OFF:TOOIE_TEXT_END]
    data = rom[TOOIE_DATA_OFF:TOOIE_DATA_END]
    jt = halfword_table(data)

    print("=== Tooie n_aspMain (decompressed.us.z64) ===")
    print(f"  text ROM  {TOOIE_TEXT_OFF:#x}..{TOOIE_TEXT_END:#x}  size={len(text):#x}")
    print(f"  data ROM  {TOOIE_DATA_OFF:#x}..{TOOIE_DATA_END:#x}  size={len(data):#x}")
    print(f"  text sha256[:16]  {sha16(text)}")
    print(f"  data sha256[:16]  {sha16(data)}")
    print(f"  text_address (RSP)  {BK_TEXT_ADDRESS:#x}  (same convention as BK)")
    print(f"  dispatch halfwords @ data+0x0E ({len(jt)}):")
    print("   ", ", ".join(f"{x:#06x}" for x in jt))

    print("\n=== Banjo-Kazooie (RSPRecomp config) ===")
    print(f"  text_size  {BK_TEXT_SIZE:#x}")
    print(f"  jump table entries  {len(BK_JUMP_TABLE)}")
    print("   ", ", ".join(f"{x:#06x}" for x in BK_JUMP_TABLE))

    ratio = len(text) / BK_TEXT_SIZE
    print("\n=== Delta ===")
    print(f"  Tooie text is {ratio:.3f}× BK text size ({len(text) - BK_TEXT_SIZE:+#x} bytes)")
    print(f"  Command handlers: Tooie ~{len(jt)} vs BK {len(BK_JUMP_TABLE)}")
    shared = set(jt) & set(BK_JUMP_TABLE)
    print(f"  Shared jump targets: {len(shared)} → {sorted(hex(x) for x in shared)}")
    only_tooie = set(jt) - set(BK_JUMP_TABLE)
    only_bk = set(BK_JUMP_TABLE) - set(jt)
    print(f"  Tooie-only targets: {sorted(hex(x) for x in only_tooie)}")
    print(f"  BK-only targets:    {sorted(hex(x) for x in only_bk)}")
    print(
        "  Conclusion: microcodes are related (n_audio family) but NOT identical;"
        " Tooie needs its own RSPRecomp output (do not reuse BK n_aspMain.cpp)."
    )

    if args.bk_rom and args.bk_rom.is_file():
        bk = args.bk_rom.read_bytes()
        bk_text = bk[args.bk_text_offset : args.bk_text_offset + BK_TEXT_SIZE]
        print("\n=== Byte compare (optional BK ROM) ===")
        print(f"  BK text sha256[:16]  {sha16(bk_text)}")
        n = min(len(text), len(bk_text))
        same = sum(1 for i in range(n) if text[i] == bk_text[i])
        print(f"  first {n:#x} bytes identical: {same}/{n} ({100 * same / n:.1f}%)")
    else:
        print("\n(BK ROM not provided — size/jump-table compare only)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
