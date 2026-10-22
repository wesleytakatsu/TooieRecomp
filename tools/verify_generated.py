#!/usr/bin/env python3
"""Gate: a generated directory must match metadata/recomp-output.sha256. Port of tools/verify_generated.sh.

    verify_generated.py [dir]            verify (default: <repo>/RecompiledFuncs)
    verify_generated.py --update [dir]   MAINTAINER: rewrite the manifest (review the diff)

Manifest lines are `<sha256>  <name>` (two spaces), sorted by filename bytes (LC_ALL=C), LF, no BOM.
"""
from __future__ import annotations

import hashlib
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from tooie_paths import TOOIE  # noqa: E402

_MANIFEST = TOOIE / "metadata" / "recomp-output.sha256"
_MANIFEST_DISPLAY = "metadata/recomp-output.sha256"
_LINE = re.compile(r"^([0-9a-fA-F]{64}) (\*| )(.+)$")
_SHOW = 20
_CHUNK = 1 << 20


class _Usage(Exception):
    pass


def main(argv: list[str] | None = None) -> int:
    """CLI entry. `argv` is the argument list without the program name."""
    if argv is None:
        argv = sys.argv[1:]
    try:
        update, directory = _parse(argv)
    except _Usage as exc:
        print(f"verify_generated: {exc}", file=sys.stderr)
        print("usage: verify_generated.py [--update] [dir]", file=sys.stderr)
        return 2
    if not directory.is_dir():
        print(f"verify_generated: FAIL: not a directory: {directory}", file=sys.stderr)
        return 1
    try:
        names = _regular_names(directory)
    except OSError as exc:
        print(f"verify_generated: FAIL: {exc}", file=sys.stderr)
        return 1
    if update:
        return _update(directory, names)
    return _verify(directory, names)


def _parse(argv: list[str]) -> tuple[bool, Path]:
    update = False
    args = list(argv)
    if args and args[0] == "--update":
        update = True
        del args[0]
    if len(args) > 1:
        raise _Usage("too many arguments")
    directory = Path(args[0]) if args else TOOIE / "RecompiledFuncs"
    return update, directory


def _regular_names(directory: Path) -> list[str]:
    return [entry.name for entry in directory.iterdir() if entry.is_file()]


def _sort_key(name: str) -> bytes:
    # LC_ALL=C is byte order. UTF-8 preserves codepoint order; surrogateescape covers undecodable names.
    return name.encode("utf-8", "surrogateescape")


def _sha256(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as fh:
        while True:
            chunk = fh.read(_CHUNK)
            if not chunk:
                break
            hasher.update(chunk)
    return hasher.hexdigest()


def _update(directory: Path, names: list[str]) -> int:
    try:
        lines = [f"{_sha256(directory / name)}  {name}\n" for name in sorted(names, key=_sort_key)]
        _MANIFEST.write_bytes("".join(lines).encode("utf-8"))
    except OSError as exc:
        print(f"verify_generated: FAIL: {exc}", file=sys.stderr)
        return 1
    print(f"verify_generated: wrote {len(lines)} entries to {_MANIFEST}")
    return 0


def _verify(directory: Path, names: list[str]) -> int:
    if not _MANIFEST.is_file():
        print(f"verify_generated: FAIL: missing {_MANIFEST_DISPLAY}", file=sys.stderr)
        return 1
    try:
        text = _MANIFEST.read_bytes().decode("utf-8")
    except (OSError, UnicodeError) as exc:
        print(f"verify_generated: FAIL: {exc}", file=sys.stderr)
        return 1
    wanted: dict[str, str] = {}
    problems: list[tuple[str, str]] = []
    entries = 0
    for lineno, line in enumerate(text.splitlines(), 1):
        match = _LINE.match(line)
        if not match:
            print(f"verify_generated: FAIL: bad manifest line {lineno}", file=sys.stderr)
            return 1
        entries += 1
        digest, name = match.group(1).lower(), match.group(3)
        if name in wanted:
            problems.append((name, f"duplicate {name}"))
        wanted[name] = digest
    present = set(names)
    known = set(wanted)
    problems.extend((name, f"missing {name}") for name in sorted(known - present, key=_sort_key))
    problems.extend((name, f"extra {name}") for name in sorted(present - known, key=_sort_key))
    try:
        for name in sorted(present & known, key=_sort_key):
            if _sha256(directory / name) != wanted[name]:
                problems.append((name, f"mismatch {name}"))
    except OSError as exc:
        print(f"verify_generated: FAIL: {exc}", file=sys.stderr)
        return 1
    if entries != len(present) or problems:
        problems.sort(key=lambda item: _sort_key(item[0]))
        shown = [label for _, label in problems[:_SHOW]]
        if not shown:
            shown = [f"count {len(present)} != manifest {entries}"]
        detail = ", ".join(shown)
        extra = len(problems) - _SHOW
        if extra > 0:
            detail += f" (+{extra} more)"
        print(f"verify_generated: FAIL: {detail}", file=sys.stderr)
        return 1
    print(f"verify_generated: OK {entries} files match {_MANIFEST_DISPLAY}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
