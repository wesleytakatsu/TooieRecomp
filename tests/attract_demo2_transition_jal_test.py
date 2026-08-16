#!/usr/bin/env python3
"""Autonomous check for the P08 attract DEMO2→next-scene crash.

Original bsrest_entrypoint_11 / _3 / _7 jal 0x808003FC to an unnamed
same-section helper (calls func_800A0C20 / func_800A0D14). Generated C
currently emits LOOKUP_FUNC(0x808003FC), which binds to
baflameline_entrypoint_5 (same VRAM, different overlay).

PASS when those bsrest callers use a same-section direct C call.
FAIL documents the current proven misbind. Does not run the game.
"""
from __future__ import annotations

import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
RF = ROOT / "RecompiledFuncs"
NEEDLE = "LOOKUP_FUNC(0x808003FC)"
FUNCS = (
    "bsrest_entrypoint_11",
    "bsrest_entrypoint_3",
    "bsrest_entrypoint_7",
)


def find_func(name: str) -> tuple[pathlib.Path, list[str]] | None:
    for path in sorted(RF.glob("funcs_*.c")):
        text = path.read_text(errors="replace")
        token = f"void {name}("
        if token not in text:
            continue
        start = text.index(token)
        chunk = text[start : start + 2500]
        return path, chunk.splitlines()
    return None


def main() -> int:
    if not RF.is_dir():
        print("FAIL: RecompiledFuncs/ missing")
        return 1
    failed = False
    for name in FUNCS:
        found = find_func(name)
        if found is None:
            print(f"FAIL: {name} not found in RecompiledFuncs")
            failed = True
            continue
        path, lines = found
        uses_lookup = any(NEEDLE in line for line in lines[:80])
        print(f"{name}: file={path.name} lookup808003FC={'YES' if uses_lookup else 'NO'}")
        if uses_lookup:
            failed = True
    if failed:
        print(
            "FAIL: bsrest still LOOKUP_FUNC(0x808003FC); "
            "expected same-section bind to the unnamed .bsrest helper"
        )
        return 1
    print("PASS: bsrest 0x808003FC calls are same-section")
    return 0


if __name__ == "__main__":
    sys.exit(main())
