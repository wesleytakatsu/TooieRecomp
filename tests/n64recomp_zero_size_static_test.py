#!/usr/bin/env python3
"""Regression: a zero-sized function symbol at a static function's address must not produce an
empty generated body (bsrest 0x80800000 helper: tspd=0 + velocity clear on state 4->116).

Python port of n64recomp_zero_size_static_test.sh. usage:
    python tests/n64recomp_zero_size_static_test.py <N64Recomp binary> <decompressed rom>
Runs the generator twice in symbols mode on COPIES of metadata:
  A = metadata as is, B = metadata + zero-sized func_80800000_bsrest.
Generator exit codes are checked (no failure masking). Requires the bsrest static helper body to
be present and output A == output B (the zero-size symbol must not change anything).
"""
from __future__ import annotations

import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]

SYMBOLS_ANCHOR = 'functions = [\n    { name = "bsrest_entrypoint_2"'
ZERO_SIZE_INSERT = (
    'functions = [\n    { name = "func_80800000_bsrest", vram = 0x80800000, size = 0x0 },\n'
    '    { name = "bsrest_entrypoint_2"'
)


def render_config(template: str, rom: str) -> str:
    text = re.sub(r"rom_file_path = .*", lambda _m: f"rom_file_path = '{rom}'", template)
    text = re.sub(r"output_func_path = .*", lambda _m: "output_func_path = 'out'", text)
    text = re.sub(r"^elf_path = .*", lambda _m: "symbols_file_path = 'meta/tooie.us.syms.toml'", text, flags=re.M)
    return text.replace("metadata/", "meta/")


def gen(workdir: pathlib.Path, name: str, rom: str, n64r: str, zero_size: bool) -> bool:
    d = workdir / name
    (d / "out").mkdir(parents=True)
    shutil.copytree(ROOT / "metadata", d / "meta")
    if zero_size:
        symbols = d / "meta" / "tooie.us.syms.toml"
        text = symbols.read_text(encoding="utf-8")
        if SYMBOLS_ANCHOR not in text:
            print(f"FAIL zero-size anchor not found in {symbols.name}")
            return False
        symbols.write_text(text.replace(SYMBOLS_ANCHOR, ZERO_SIZE_INSERT, 1), encoding="utf-8")
    template = (ROOT / "tooie.us.rev0.recomp.toml").read_text(encoding="utf-8")
    (d / "t.toml").write_text(render_config(template, rom), encoding="utf-8", newline="\n")

    log = d / "log.txt"
    try:
        with open(log, "wb") as log_file:
            rc = subprocess.run([n64r, "t.toml"], cwd=d, stdout=log_file, stderr=subprocess.STDOUT).returncode
    except OSError as exc:
        print(f"FAIL generator '{name}' could not run: {exc}")
        return False
    if rc != 0:
        print(f"FAIL generator '{name}' exit {rc}:")
        kept = [line for line in log.read_text(encoding="utf-8", errors="replace").splitlines() if not line.startswith("[Info]")]
        for line in kept[-5:]:
            print(line)
        return False
    print(f"generator '{name}' exit 0")
    return True


def fbody(outdir: pathlib.Path, fname: str) -> str:
    header = f"RECOMP_FUNC void {fname}("
    body: list[str] = []
    started = False
    for path in sorted(outdir.glob("funcs_*.c")):
        with open(path, encoding="utf-8", errors="replace", newline="") as f:
            for line in f.read().split("\n"):
                if not started:
                    if not line.startswith(header):
                        continue
                    started = True
                body.append(line)
                if re.fullmatch(r";?\}", line):
                    return "\n".join(body)
    return "\n".join(body)


def tree_files(root: pathlib.Path) -> dict[str, pathlib.Path]:
    return {p.relative_to(root).as_posix(): p for p in root.rglob("*") if p.is_file()}


def diff_report(a: pathlib.Path, b: pathlib.Path) -> list[str]:
    fa, fb = tree_files(a), tree_files(b)
    report = [f"Only in {a}: {rel}" for rel in sorted(fa.keys() - fb.keys())]
    report += [f"Only in {b}: {rel}" for rel in sorted(fb.keys() - fa.keys())]
    for rel in sorted(fa.keys() & fb.keys()):
        if fa[rel].read_bytes() != fb[rel].read_bytes():
            report.append(f"Files {fa[rel]} and {fb[rel]} differ")
    return report


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print("usage: n64recomp_zero_size_static_test.py <N64Recomp binary> <decompressed rom>")
        return 2
    n64r = os.path.abspath(argv[1])
    rom = os.path.abspath(argv[2]).replace("\\", "/")
    if "'" in rom:
        print("FAIL ROM path contains a single quote, cannot be a TOML literal string")
        return 1

    workdir = pathlib.Path(tempfile.mkdtemp(prefix="n64recomp_zero_size_"))
    try:
        if not gen(workdir, "A", rom, n64r, zero_size=False):
            return 1
        if not gen(workdir, "B", rom, n64r, zero_size=True):
            return 1

        b_out = workdir / "B" / "out"
        m = re.search(r"static_([0-9]*)_8080020C", fbody(b_out, "bsrest_entrypoint_9"))
        n = m.group(1) if m else ""
        if not n:
            print("FAIL cannot find bsrest static section (bsrest_entrypoint_9 -> static_N_8080020C)")
            return 1

        helper = fbody(b_out, f"static_{n}_80800000")
        caller = fbody(b_out, f"static_{n}_8080020C")
        if f"static_{n}_80800000(rdram, ctx)" not in caller:
            print(f"FAIL static_{n}_8080020C does not call static_{n}_80800000")
            return 1
        if "LOOKUP_FUNC(0x8009B9B0)" in helper and "LOOKUP_FUNC(0x8009BA9C)" in helper:
            print(f"bsrest helper static_{n}_80800000 keeps tspd/velocity stop body")
        else:
            print(f"FAIL bsrest helper static_{n}_80800000 is empty/incomplete")
            return 1

        report = diff_report(workdir / "A" / "out", b_out)
        if report:
            print("FAIL zero-size symbol changed generated output:")
            for line in report[:5]:
                print(line)
            return 1
        print(f"PASS zero-size static (bsrest section {n}): body kept, output identical with/without symbol")
        return 0
    finally:
        shutil.rmtree(workdir)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
