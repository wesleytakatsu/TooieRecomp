#!/usr/bin/env python3
"""Unit tests for Tooie overlay syscall stub decoding (P06).

Validates against decompressed ROM + known symbols from syscalls.table.toml.
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOOIE = ROOT / "TooieRecomp"
BT = ROOT / "banjo-tooie-Recomp"
ROM = BT / "decompressed.us.z64"

CORE2_RAM = 0x800815C0
CORE2_ROM = 0x01E5AEB0


def decode_stub(word0: int, word1: int) -> tuple[int, int] | None:
    op0 = (word0 >> 26) & 0x3F
    funct = word0 & 0x3F
    if op0 != 0 or funct != 0x0C:
        return None
    code = (word0 >> 6) & 0xFFFFF
    op1 = (word1 >> 26) & 0x3F
    rs = (word1 >> 21) & 0x1F
    rt = (word1 >> 16) & 0x1F
    imm = word1 & 0xFFFF
    if op1 not in (8, 0xE) or rs != 0 or rt != 8 or (imm & 3) != 0:
        return None
    return code, imm >> 2


def read_words(rom: bytes, vram: int) -> tuple[int, int]:
    off = CORE2_ROM + (vram - CORE2_RAM)
    w0, w1 = struct.unpack_from(">II", rom, off)
    return w0, w1


def main() -> int:
    if not ROM.is_file():
        print(f"SKIP: missing {ROM}")
        return 0

    rom = ROM.read_bytes()
    cases = [
        # (name, vram, expected_overlay, expected_entry, expect_xori)
        ("_gsmaker_entrypoint_0", 0x80082548, 2, 0, False),
        ("_gsmaker_entrypoint_1", 0x80082550, 2, 1, False),
        ("_bainvisible_entrypoint_3", 0x800854C0, 164, 3, True),
        # Session example VRAM (ROM says syscall 371)
        ("example_0x80086788", 0x80086788, 371, 0, False),
    ]

    failed = 0
    for name, vram, exp_ovl, exp_ep, exp_xori in cases:
        w0, w1 = read_words(rom, vram)
        got = decode_stub(w0, w1)
        if got is None:
            print(f"FAIL {name}: decode failed words={w0:08X} {w1:08X}")
            failed += 1
            continue
        ovl, ep = got
        op1 = (w1 >> 26) & 0x3F
        is_xori = op1 == 0xE
        ok = ovl == exp_ovl and ep == exp_ep and is_xori == exp_xori
        status = "ok" if ok else "FAIL"
        print(
            f"{status} {name}: ovl={ovl} ep={ep} xori={is_xori} "
            f"(expect {exp_ovl}/{exp_ep}/xori={exp_xori}) words={w0:08X} {w1:08X}"
        )
        if not ok:
            failed += 1

    # Dispatch table sanity if generated
    gen = TOOIE / "src" / "game" / "generated" / "overlay_dispatch.gen.cpp"
    if gen.is_file():
        text = gen.read_text(encoding="utf-8")
        for needle in ("ovl_2_entrypoints", "gsmaker_entrypoint_0", "bainvisible_entrypoint_3"):
            if needle not in text:
                print(f"FAIL dispatch table missing {needle}")
                failed += 1
            else:
                print(f"ok dispatch has {needle}")

    if failed:
        print(f"{failed} failure(s)")
        return 1
    print("all syscall decode tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
