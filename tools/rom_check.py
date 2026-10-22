#!/usr/bin/env python3
"""Read-only check of the private Banjo-Tooie ROM. Never writes, copies, or prints ROM bytes.

    rom_check.py [rom]
    rom_check.py --decompressed <file>

Exit 0 match, 1 missing/wrong, 2 usage error. Ports tools/rom_check.sh.
"""
from __future__ import annotations

import hashlib
import os
import sys
from pathlib import Path

from toolchain_lock import load
from tooie_paths import TOOIE, rom_path

_CHUNK = 1 << 20  # 1 MiB


class RomCheckError(Exception):
    """Missing or wrong ROM. str(exc) is the reason, without the rom_check prefix."""


class _Usage(Exception):
    pass


def check_raw(path: str | Path) -> None:
    """Size + MD5 + SHA1 of the raw Banjo-Tooie (USA) rev 0 .z64. Prints the OK line."""
    display = os.fspath(path)
    lock = load()
    basename = _need(lock, "ROM_BASENAME")
    src = _file(display, f'put your ROM at "{TOOIE / basename}" or set TOOIE_ROM')
    size = src.stat().st_size
    want_size = _need(lock, "ROM_SIZE")
    if str(size) != want_size:
        raise RomCheckError(
            f"size {size} != {want_size} (need Banjo-Tooie USA rev 0, big-endian .z64): {display}"
        )
    got = _digests(src, ("md5", "sha1"))
    want_md5 = _need(lock, "ROM_MD5")
    if got["md5"] != want_md5:
        raise RomCheckError(f"MD5 {got['md5']} != {want_md5}: {display}")
    want_sha = _need(lock, "ROM_SHA1")
    if got["sha1"] != want_sha:
        raise RomCheckError(f"SHA1 {got['sha1']} != {want_sha}: {display}")
    print(f"rom_check: OK {size} bytes md5 {got['md5']} sha1 {got['sha1']}")


def check_decompressed(path: str | Path) -> None:
    """Size + SHA1 of decompressed.us.z64. The file argument is required. Prints the OK line."""
    display = os.fspath(path)
    lock = load()
    src = _file(display, "create it with the extract stage: tools/build_linux.sh extract or Build-Tooie.ps1 -Stage Extract")
    size = src.stat().st_size
    want_size = _need(lock, "DECOMPRESSED_ROM_SIZE")
    if str(size) != want_size:
        raise RomCheckError(f"size {size} != {want_size}: {display}")
    got = _digests(src, ("sha1",))["sha1"]
    want_sha = _need(lock, "DECOMPRESSED_ROM_SHA1")
    if got != want_sha:
        raise RomCheckError(f"SHA1 {got} != {want_sha}: {display}")
    print(f"rom_check: OK decompressed {size} bytes sha1 {got}")


def main(argv: list[str] | None = None) -> int:
    """CLI entry. `argv` is the argument list without the program name."""
    if argv is None:
        argv = sys.argv[1:]
    try:
        mode, target = _parse(argv)
        if mode == "dec":
            check_decompressed(target)
        else:
            check_raw(target)
    except _Usage as exc:
        print(f"rom_check: {exc}", file=sys.stderr)
        return 2
    except RomCheckError as exc:
        print(f"rom_check: FAIL: {exc}", file=sys.stderr)
        return 1
    except ValueError as exc:
        print(f"rom_check: {exc}", file=sys.stderr)
        return 2
    return 0


def _parse(argv: list[str]) -> tuple[str, str]:
    # Same shape as rom_check.sh: only a leading --decompressed switches mode.
    if argv and argv[0] == "--decompressed":
        if len(argv) < 2 or argv[1] == "":
            raise _Usage("--decompressed needs a file")
        return "dec", argv[1]
    # ${1:-default}: an empty positional is unset, so use the default ROM path.
    if argv and argv[0]:
        return "raw", argv[0]
    return "raw", os.fspath(rom_path())


def _need(lock: dict[str, str], key: str) -> str:
    try:
        return lock[key]
    except KeyError:
        raise ValueError(f"toolchain.lock: missing {key}") from None


def _file(display: str, hint: str) -> Path:
    path = Path(display)
    if not path.is_file():
        raise RomCheckError(f"not a file: {display} ({hint})")
    return path


def _digests(path: Path, names: tuple[str, ...]) -> dict[str, str]:
    hashers = {name: hashlib.new(name) for name in names}
    with path.open("rb") as fh:
        while True:
            chunk = fh.read(_CHUNK)
            if not chunk:
                break
            for hasher in hashers.values():
                hasher.update(chunk)
    return {name: hasher.hexdigest() for name, hasher in hashers.items()}


if __name__ == "__main__":
    raise SystemExit(main())
