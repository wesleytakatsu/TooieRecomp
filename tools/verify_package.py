#!/usr/bin/env python3
"""Verify a TooieRecompiled package (zip, tar.gz, or an extracted directory).

Port of tools/verify_package.sh. Also checks that every packaged file is listed
in MANIFEST.sha256 (the manifest itself is the only allowed extra).

  python tools/verify_package.py <archive.zip | archive.tar.gz | package dir>
"""
from __future__ import annotations

import hashlib
import os
import re
import shutil
import sys
import tarfile
import tempfile
import zipfile

_HASH_LINE = re.compile(r"^([0-9a-fA-F]{64}) [ *](.*)$")
_ROM_EXT = {".z64", ".n64", ".v64", ".elf"}


def die(message: str) -> None:
    print(message)
    raise SystemExit(1)


def read_text(path: str) -> str:
    with open(path, "rb") as handle:
        data = handle.read()
    if data.startswith(b"\xff\xfe") or data.startswith(b"\xfe\xff"):
        return data.decode("utf-16")
    return data.decode("utf-8-sig")


def sha256_file(path: str) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        while True:
            chunk = handle.read(1024 * 1024)
            if not chunk:
                break
            digest.update(chunk)
    return digest.hexdigest()


def parse_hash_line(line: str) -> tuple:
    text = line.strip().lstrip("\ufeff")
    match = _HASH_LINE.match(text)
    if not match:
        die("VERIFY FAIL: bad checksum line: " + text)
    return match.group(1).lower(), match.group(2).strip()


def norm_rel(path: str) -> str:
    text = path.strip().replace("\\", "/")
    while text.startswith("./"):
        text = text[2:]
    if text.endswith("/"):
        text = text[:-1]
    return text


def is_unsafe(rel: str) -> bool:
    if not rel or rel.startswith("/") or re.match(r"^[A-Za-z]:", rel):
        return True
    return any(part in ("", "..") for part in rel.split("/"))


def same_file(left: str, right: str) -> bool:
    return os.path.normcase(os.path.realpath(left)) == os.path.normcase(os.path.realpath(right))


def is_rom_like(rel: str) -> bool:
    base = rel.replace("\\", "/").split("/")[-1].lower()
    if os.path.splitext(base)[1] in _ROM_EXT:
        return True
    return base.startswith("baserom") or base.startswith("decompressed")


def check_sidecar(archive: str) -> None:
    side = archive + ".sha256"
    if not os.path.isfile(side):
        return
    directory = os.path.dirname(os.path.abspath(archive))
    named = False
    for raw in read_text(side).splitlines():
        if not raw.strip():
            continue
        digest, rel_raw = parse_hash_line(raw)
        rel = norm_rel(rel_raw)
        if is_unsafe(rel):
            die("VERIFY FAIL: unsafe path in sidecar: " + rel_raw)
        target = os.path.realpath(os.path.join(directory, *rel.split("/")))
        if not os.path.isfile(target):
            die("VERIFY FAIL: sidecar names missing file " + rel)
        if sha256_file(target) != digest:
            die("VERIFY FAIL: sidecar checksum mismatch for " + rel)
        if same_file(target, archive):
            named = True
    if not named:
        die("VERIFY FAIL: sidecar does not name " + os.path.basename(archive))


def _write_member(src, target: str) -> None:
    parent = os.path.dirname(target)
    if parent:
        os.makedirs(parent, exist_ok=True)
    with open(target, "wb") as out:
        shutil.copyfileobj(src, out)


def extract_zip(path: str, dest: str) -> None:
    with zipfile.ZipFile(path) as archive:
        for info in archive.infolist():
            rel = norm_rel(info.filename)
            if not rel:
                continue
            if is_unsafe(rel):
                die("VERIFY FAIL: unsafe path in archive: " + info.filename)
            if info.is_dir() or info.filename.replace("\\", "/").endswith("/"):
                continue
            target = os.path.join(dest, *rel.split("/"))
            with archive.open(info, "r") as src:
                _write_member(src, target)


def extract_tar(path: str, dest: str) -> None:
    with tarfile.open(path, "r:gz") as archive:
        for member in archive.getmembers():
            name = member.name.replace("\\", "/")
            if name.startswith("./"):
                name = name[2:]
            rel = norm_rel(name)
            if not rel:
                continue
            if is_unsafe(rel):
                die("VERIFY FAIL: unsafe path in archive: " + member.name)
            if member.issym() or member.islnk():
                die("VERIFY FAIL: link in archive: " + member.name)
            if member.isdir() or not member.isfile():
                continue
            src = archive.extractfile(member)
            if src is None:
                die("VERIFY FAIL: unreadable archive member: " + member.name)
            target = os.path.join(dest, *rel.split("/"))
            with src:
                _write_member(src, target)


def single_top(dest: str) -> str:
    names = sorted(n for n in os.listdir(dest) if n not in (".", ".."))
    if len(names) != 1:
        found = ", ".join(names) if names else "(empty)"
        die("VERIFY FAIL: expected a single top-level package directory, found: " + found)
    top = os.path.join(dest, names[0])
    if not os.path.isdir(top):
        die("VERIFY FAIL: expected a single top-level package directory")
    return top


def iter_files(root: str) -> list:
    found = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames.sort()
        for name in sorted(filenames):
            full = os.path.join(dirpath, name)
            if os.path.isdir(full):
                continue
            found.append(norm_rel(os.path.relpath(full, root)))
    return found


def check_tree(root: str) -> int:
    root = os.path.abspath(root)
    manifest_path = os.path.join(root, "MANIFEST.sha256")
    if not os.path.isfile(manifest_path):
        die("VERIFY FAIL: MANIFEST.sha256 not found")
    seen = {}
    entries = []
    for raw in read_text(manifest_path).splitlines():
        if not raw.strip():
            continue
        digest, rel_raw = parse_hash_line(raw)
        rel = norm_rel(rel_raw)
        if is_unsafe(rel):
            die("VERIFY FAIL: unsafe manifest path: " + rel_raw)
        if rel in seen:
            die("VERIFY FAIL: duplicate manifest path: " + rel)
        seen[rel] = digest
        entries.append(rel)
    errors = []
    for rel in entries:
        full = os.path.join(root, *rel.split("/"))
        if not os.path.isfile(full):
            errors.append("VERIFY FAIL: missing: " + rel)
            continue
        if sha256_file(full) != seen[rel]:
            errors.append("VERIFY FAIL: hash mismatch: " + rel)
    present = iter_files(root)
    for rel in present:
        if rel == "MANIFEST.sha256":
            continue
        if rel not in seen:
            errors.append("VERIFY FAIL: not listed in MANIFEST.sha256: " + rel)
    if errors:
        for error in errors:
            print(error)
        return 1
    print("MANIFEST OK (%d files)" % len(entries))
    roms = [rel for rel in present if is_rom_like(rel)]
    if roms:
        for rel in roms:
            print("VERIFY FAIL: ROM/ELF-like file inside package: " + rel)
        return 1
    print("no ROM/ELF-like files")
    return 0


def main(argv: list) -> int:
    # argv is the argument list without the program name (sys.argv[1:]).
    if len(argv) != 1:
        sys.stderr.write(
            "usage: tools/verify_package.py <archive.zip|archive.tar.gz|package dir>\n"
        )
        return 1
    path = argv[0]
    if os.path.isdir(path):
        return check_tree(path)
    if not os.path.isfile(path):
        die("VERIFY FAIL: not a file or directory: " + path)
    lower = path.lower()
    if lower.endswith(".zip"):
        kind = "zip"
    elif lower.endswith(".tar.gz") or lower.endswith(".tgz"):
        kind = "tar"
    else:
        die("VERIFY FAIL: expected .zip, .tar.gz, or a package directory")
    check_sidecar(path)
    tmp = tempfile.mkdtemp(prefix="tooie-verify-")
    try:
        if kind == "zip":
            extract_zip(path, tmp)
        else:
            extract_tar(path, tmp)
        return check_tree(single_top(tmp))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
