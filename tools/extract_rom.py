#!/usr/bin/env python3
"""Prepare the decompressed ROM image the runtime launcher needs, from the private retail ROM. Port of tools/extract_rom.sh.

    extract_rom.py [rom]     rom default: tooie_paths.rom_path() ($TOOIE_ROM or "<repo>/Banjo-Tooie (USA).z64")

Builds decomp/tools/rom_decompressor (vendored miniz instead of zlib) with CMake+Ninja into <BUILD>/extract/tool; cmake, ninja and the compiler
must already be on PATH (Windows: enter the VS developer shell first). Output: <BUILD>/extract/decompressed.us.z64 (git-ignored, ROM-derived).
The ROM is validated BEFORE any output is touched; the result is verified in a temp file and renamed, so a failure never leaves a partial output.
"""
from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import rom_check  # noqa: E402
from tooie_paths import BUILD, TOOIE, rom_path  # noqa: E402


def _run(cmd: list[str], log: Path, cwd: Path | None = None) -> bool:
    with log.open("wb") as fh:
        fh.write(("$ " + " ".join(cmd) + "\n").encode())
        fh.flush()
        try:
            return subprocess.run(cmd, cwd=cwd, stdout=fh, stderr=subprocess.STDOUT).returncode == 0
        except OSError as exc:
            fh.write(f"{exc}\n".encode())
            return False


def main(argv: list[str]) -> int:
    if len(argv) > 1:
        print("usage: extract_rom.py [rom]", file=sys.stderr)
        return 2
    rom = Path(os.path.abspath(argv[0] if argv else rom_path()))
    try:
        rom_check.check_raw(rom)
    except rom_check.RomCheckError as exc:
        print(f"rom_check: FAIL: {exc}", file=sys.stderr)
        return 1
    except ValueError as exc:
        print(f"rom_check: {exc}", file=sys.stderr)
        return 2

    for tool in ("cmake", "ninja"):
        if shutil.which(tool) is None:
            print(f"extract_rom: missing {tool} on PATH (Windows: run from the VS developer shell)")
            return 1

    out = BUILD / "extract"
    work = out / "work"
    tool_dir = out / "tool"
    tmp = out / ".decompressed.us.z64.tmp"
    final = out / "decompressed.us.z64"
    build_log = out / "rom_decompressor.build.log"
    run_log = out / "rom_decompressor.run.log"
    out.mkdir(parents=True, exist_ok=True)
    shutil.rmtree(work, ignore_errors=True)
    tmp.unlink(missing_ok=True)

    cfg = ["cmake", "-S", str(TOOIE / "tools" / "windows" / "extract"), "-B", str(tool_dir), "-G", "Ninja",
           "-DCMAKE_BUILD_TYPE=Release"]
    if os.name == "nt":
        cfg += ["-DCMAKE_C_COMPILER=clang-cl", "-DCMAKE_CXX_COMPILER=clang-cl"]
    try:
        if not _run(cfg, build_log):
            print(f"extract_rom: rom_decompressor configure failed, see {build_log}")
            return 1
        with build_log.open("ab") as fh:  # append the build step to the same log
            fh.write(b"\n")
        build_step = out / ".build.step.log"
        ok = _run(["cmake", "--build", str(tool_dir), "--target", "rom_decompressor"], build_step)
        with build_log.open("ab") as fh:
            fh.write(build_step.read_bytes())
        build_step.unlink(missing_ok=True)
        if not ok:
            print(f"extract_rom: rom_decompressor build failed, see {build_log}")
            return 1

        exe = tool_dir / ("rom_decompressor.exe" if os.name == "nt" else "rom_decompressor")
        # rom_decompressor ALSO writes assets/overlays/*/relocs.bin relative to its CWD: run it inside work so nothing lands in the repo.
        work.mkdir(parents=True)
        if not _run([str(exe), str(rom), str(tmp)], run_log, cwd=work):
            print(f"extract_rom: rom_decompressor failed, see {run_log}")
            return 1
        try:
            rom_check.check_decompressed(tmp)
        except rom_check.RomCheckError as exc:
            print(f"rom_check: FAIL: {exc}", file=sys.stderr)
            return 1
        os.replace(tmp, final)
    finally:
        tmp.unlink(missing_ok=True)
        shutil.rmtree(work, ignore_errors=True)  # also drops the relocs.bin side outputs

    print(f"extract_rom: wrote {final}")
    launcher = "start TooieRecompiled.exe" if os.name == "nt" else "./TooieRecompiled (or run-tooie.sh)"
    print(f"extract_rom: launcher: {launcher}. The decompressed image is the generator input; "
          "for playing, either the retail ROM (.z64/.n64) or this decompressed.us.z64 "
          "can be selected in the launcher (a retail cart is decompressed internally).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
