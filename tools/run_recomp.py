#!/usr/bin/env python3
"""Build N64Recomp (unless --skip-build) and regenerate RecompiledFuncs.

Port of tools/run_recomp.sh plus the generator build in tools/build_linux.sh stage_recomp.

    run_recomp.py [--skip-build] [--jobs N]

Writes <repo>/.tooie.recomp.gen.toml with rom_file_path pointed at the decompressed ROM
(TOML literal, POSIX slashes). Never edits tooie.us.rev0.recomp.toml.
Refuses to delete RecompiledFuncs unless the ROM, the symbols file, and the generator binary all check out.
"""
from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import rom_check  # noqa: E402
from tooie_paths import BUILD, DECOMPRESSED_ROM, N64RECOMP, TOOIE  # noqa: E402

_TRACKED = TOOIE / "tooie.us.rev0.recomp.toml"
_GEN = TOOIE / ".tooie.recomp.gen.toml"
_SYMS = TOOIE / "metadata" / "tooie.us.syms.toml"
_OUT = TOOIE / "RecompiledFuncs"
_SRC = TOOIE / "lib" / "N64ModernRuntime" / "N64Recomp"


class _Usage(Exception):
    pass


def main(argv: list[str] | None = None) -> int:
    """CLI entry. `argv` is the argument list without the program name."""
    if argv is None:
        argv = sys.argv[1:]
    try:
        skip, jobs = _parse(argv)
    except _Usage as exc:
        print(f"run_recomp: {exc}", file=sys.stderr)
        print("usage: run_recomp.py [--skip-build] [--jobs N]", file=sys.stderr)
        return 2
    if not skip:
        reason = _build(jobs)
        if reason:
            return _untouched(reason)
    reason = _write_config()
    if reason:
        return _untouched(reason)
    reason = _preflight()
    if reason:
        return _untouched(reason)
    try:
        if _OUT.exists():
            shutil.rmtree(_OUT)
        _OUT.mkdir()
    except OSError as exc:
        print(f"run_recomp: {exc}", file=sys.stderr)
        return 1
    cmd = [os.fspath(N64RECOMP), os.fspath(_GEN)]
    try:
        proc = subprocess.run(cmd, cwd=os.fspath(TOOIE))
    except OSError as exc:
        print(f"run_recomp: {exc}", file=sys.stderr)
        return 1
    if proc.returncode != 0:
        print(f"run_recomp: N64Recomp exited {proc.returncode}", file=sys.stderr)
        return proc.returncode
    files = [p for p in _OUT.iterdir() if p.is_file()]
    total = sum(p.stat().st_size for p in files)
    print(f"generated {len(files)} files ({total / (1024 * 1024):.1f} MB)")
    return 0


def _parse(argv: list[str]) -> tuple[bool, int]:
    skip = False
    jobs: int | None = None
    i = 0
    while i < len(argv):
        arg = argv[i]
        if arg == "--skip-build":
            skip = True
        elif arg == "--jobs":
            if i + 1 >= len(argv):
                raise _Usage("--jobs needs a number")
            i += 1
            jobs = _jobs(argv[i])
        else:
            raise _Usage(f"unknown argument: {arg}")
        i += 1
    if jobs is None:
        jobs = _jobs(os.environ.get("TOOIE_JOBS") or "2")
    return skip, jobs


def _jobs(raw: str) -> int:
    if not raw.isdigit() or int(raw) < 1:
        raise _Usage(f"jobs must be a positive integer, got {raw!r}")
    return int(raw)


def _build(jobs: int) -> str | None:
    bld = BUILD / "n64recomp"
    compilers = (
        ["-DCMAKE_C_COMPILER=clang-cl", "-DCMAKE_CXX_COMPILER=clang-cl"]
        if os.name == "nt"
        else ["-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++"]
    )
    configure = [
        "cmake", "-S", os.fspath(_SRC), "-B", os.fspath(bld), "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release", *compilers,
    ]
    reason = _cmake(configure, "N64Recomp configure failed")
    if reason:
        return reason
    build = ["cmake", "--build", os.fspath(bld), "--target", "N64Recomp", "-j", str(jobs)]
    return _cmake(build, "N64Recomp build failed")


def _cmake(cmd: list[str], label: str) -> str | None:
    try:
        proc = subprocess.run(cmd, cwd=os.fspath(TOOIE))
    except OSError as exc:
        return f"{label}: {exc}"
    if proc.returncode != 0:
        return f"{label} ({proc.returncode})"
    return None


def _write_config() -> str | None:
    if not _TRACKED.is_file():
        return f"missing {_TRACKED.name}"
    try:
        data = _retarget(_TRACKED.read_bytes(), DECOMPRESSED_ROM)
    except ValueError as exc:
        return str(exc)
    try:
        _GEN.write_bytes(data)  # binary: keep the tracked file's LF bytes
    except OSError as exc:
        return str(exc)
    return None


def _retarget(src: bytes, rom: Path) -> bytes:
    # Single-quoted TOML literal: backslashes are not escapes, so the path must already be POSIX.
    posix = rom.as_posix()
    if "'" in posix or "\n" in posix or "\r" in posix:
        raise ValueError(f"decompressed ROM path cannot be a TOML literal: {posix}")
    replacement = f"rom_file_path = '{posix}'".encode("utf-8")
    found = False
    pieces: list[bytes] = []
    for line in src.splitlines(keepends=True):
        body, nl = _split_nl(line)
        if body.startswith(b"rom_file_path = "):
            pieces.append(replacement + nl)
            found = True
        else:
            pieces.append(line)
    if not found:
        raise ValueError(f"{_TRACKED.name} has no rom_file_path line")
    return b"".join(pieces)


def _split_nl(line: bytes) -> tuple[bytes, bytes]:
    for ending in (b"\r\n", b"\n", b"\r"):
        if line.endswith(ending):
            return line[: -len(ending)], ending
    return line, b""


def _preflight() -> str | None:
    try:
        rom_check.check_decompressed(DECOMPRESSED_ROM)
    except rom_check.RomCheckError as exc:
        return str(exc)
    except ValueError as exc:
        return str(exc)
    if not _SYMS.is_file():
        return "missing metadata/tooie.us.syms.toml"
    if not N64RECOMP.is_file():
        return f"missing N64Recomp at {N64RECOMP}"
    return None


def _untouched(reason: str) -> int:
    print(f"run_recomp: {reason} (RecompiledFuncs left untouched)", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
