#!/usr/bin/env python3
"""Fail if RecompiledFuncs is older than ELF/ROM/metadata inputs (P04-T07)."""
from __future__ import annotations

import sys
from pathlib import Path

from tooie_paths import DECOMPRESSED_ROM, TOOIE as ROOT

INPUTS = [
    DECOMPRESSED_ROM,
    ROOT / "metadata/tooie.us.syms.toml",
    ROOT / "tooie.us.rev0.recomp.toml",
    ROOT / "metadata/recomp_stubs.txt",
    ROOT / "metadata/relocatable_sections.recomp.txt",
]
OUT_DIR = ROOT / "RecompiledFuncs"
MARKERS = [OUT_DIR / "funcs.h", OUT_DIR / "lookup.cpp"]


def main() -> None:
    missing = [p for p in INPUTS if not p.is_file()]
    if missing:
        print("FAIL: missing inputs:", *missing, sep="\n  ")
        raise SystemExit(1)
    if not OUT_DIR.is_dir():
        print("FAIL: RecompiledFuncs missing — run tools/run_recomp.sh")
        raise SystemExit(1)
    for m in MARKERS:
        if not m.is_file():
            print(f"FAIL: missing generated marker {m}")
            raise SystemExit(1)

    out_mtime = min(m.stat().st_mtime for m in MARKERS)
    stale = [p for p in INPUTS if p.stat().st_mtime > out_mtime + 1.0]
    if stale:
        print("FAIL: generated output is stale vs:")
        for p in stale:
            print(f"  {p}")
        print("Run: TooieRecomp/tools/run_recomp.sh")
        raise SystemExit(1)
    print("OK: RecompiledFuncs is newer than inputs")


if __name__ == "__main__":
    main()
