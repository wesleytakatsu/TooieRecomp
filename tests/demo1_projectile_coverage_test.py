#!/usr/bin/env python3
"""Wrapper-coverage gate for TOOIE_DIAG_DEMO1_PROJECTILE / P08_DEMO1_PROJECTILE.

Every wrapped function must (a) have the expected vram in metadata, (b) be declared in funcs.h,
(c) be reached by generated code ONLY through LOOKUP_FUNC(<vram>) and never by a direct named call.
Python port of demo1_projectile_coverage_test.sh. usage: python tests/demo1_projectile_coverage_test.py
"""
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
RF = ROOT / "RecompiledFuncs"
SRC = ROOT / "src"

CHECKS_BEFORE_FIRST_WRITER = (
    ("_baeggsetup_entrypoint_5", "0x80084A60"),
    ("_cheggfire_entrypoint_2", "0x800871F0"),
    ("_suegg_entrypoint_9", "0x8008A3C0"),
    ("_sueggmove_entrypoint_9", "0x8008A480"),
    ("func_800CD820", "0x800CD820"),
    ("func_800BE444", "0x800BE444"),
    ("func_800E9E88", "0x800E9E88"),
    ("func_800E9EB4", "0x800E9EB4"),
    ("func_80106790", "0x80106790"),
    ("_subaddiecustomhits_entrypoint_5", "0x8008A018"),
    ("func_800EEB40", "0x800EEB40"),
    ("func_800EC3C4", "0x800EC3C4"),
    ("func_800CD6B4", "0x800CD6B4"),
    ("func_800CB854", "0x800CB854"),
    ("func_800FF62C", "0x800FF62C"),
    ("func_80100368", "0x80100368"),
)

CHECKS_AFTER_FIRST_WRITER = (
    ("func_800DC0C0", "0x800DC0C0"),
    ("func_800DC090", "0x800DC090"),
    ("func_800E3E8C", "0x800E3E8C"),
    ("func_800CACEC", "0x800CACEC"),
    ("func_8011447C", "0x8011447C"),
)


def read_text(path: pathlib.Path) -> str:
    with open(path, encoding="utf-8", errors="replace", newline="") as f:
        return f.read()


def lines_of(path: pathlib.Path) -> list[str]:
    return read_text(path).split("\n")


def src_lines() -> list[str]:
    out: list[str] = []
    for path in sorted(SRC.rglob("*")):
        if path.is_file():
            out.extend(lines_of(path))
    return out


def count_lines(lines: list[str], needle: str) -> int:
    return sum(1 for line in lines if needle in line)


def check(name: str, vram: str, funcs_lines: list[str], generated: list[str]) -> bool:
    meta = read_text(ROOT / "metadata" / "tooie.us.syms.toml")
    if f'name = "{name}", vram = {vram},' not in meta:
        print(f"FAIL {name} vram != {vram}")
        return False
    if not any(line.startswith(f"void {name}(") for line in funcs_lines):
        print(f"FAIL {name} not declared")
        return False
    direct_re = re.compile(r"^\s+" + re.escape(name) + r"\(rdram, ctx\);")
    direct = sum(1 for line in generated if direct_re.search(line))
    lookups = count_lines(generated, f"LOOKUP_FUNC({vram})")
    print(f"{name} {vram} lookup_sites={lookups} direct_calls={direct}")
    if not (direct == 0 and lookups >= 1):
        print(f"FAIL {name} coverage")
        return False
    return True


def func_body(lines: list[str], name: str) -> list[str]:
    start = next((i for i, line in enumerate(lines) if line.startswith(f"RECOMP_FUNC void {name}(")), None)
    if start is None:
        return []
    body: list[str] = []
    for line in lines[start:]:
        body.append(line)
        if re.fullmatch(r";?\}", line):
            break
    return body


def main() -> int:
    fail = False
    funcs_lines = lines_of(RF / "funcs.h")
    generated: list[str] = []
    for path in sorted(RF.glob("funcs_*.c")):
        generated.extend(lines_of(path))

    for name, vram in CHECKS_BEFORE_FIRST_WRITER:
        if not check(name, vram, funcs_lines, generated):
            fail = True

    body = func_body(lines_of(RF / "funcs_338.c"), "func_800FF62C")
    first_call = next((i + 1 for i, line in enumerate(body) if "LOOKUP_FUNC(0x80100368)" in line), None)
    r31_before = 0
    if first_call is not None:
        r31_before = sum(1 for line in body[: first_call - 1] if "ctx->r31 = " in line)
    r31_total = count_lines(body, "ctx->r31 = ")
    sites = count_lines(body, "LOOKUP_FUNC(0x80100368)")
    print(
        f"func_800FF62C r31_assign_before_first_getter_call={r31_before} "
        f"r31_assign_total={r31_total} getter_sites={sites}"
    )
    if not (r31_before == 0 and r31_total == 1 and sites == 2):
        print("FAIL func_800FF62C r31/site proof")
        fail = True

    loop_callers = count_lines(generated, "LOOKUP_FUNC(0x800FF62C)")
    src = src_lines()
    b_loop = sum(1 for line in src if re.search(r"add_loaded_function\(.*0x800FF62C", line))
    b_get = sum(1 for line in src if re.search(r"add_loaded_function\(.*0x80100368", line))
    print(f"loop_lookup_sites={loop_callers} host_bindings_800FF62C={b_loop} host_bindings_80100368={b_get}")
    if not (loop_callers == 1 and b_loop == 1 and b_get == 1):
        print("FAIL first-writer binding coverage")
        fail = True

    h_lines = lines_of(ROOT / "src" / "game" / "boot_hw_stubs.cpp")
    o_lines = lines_of(ROOT / "tools" / "p08_attract_egg_hook.h")
    acc = count_lines(h_lines, "upd_accept(s_d1t_loop_scope, sp, record_arg, expected,")
    bad_acc = sum(1 for line in h_lines if re.search(r"upd_accept\(.*s_d1t_marker", line))
    post_rec = count_lines(h_lines, 'tooie_d1t_upd_dump(rdram, "post", s_d1t_upd_post, rec, s_d1t_marker')
    bad_post = count_lines(h_lines, 'tooie_d1t_upd_dump(rdram, "post", s_d1t_upd_post, s_d1t_marker')
    orc = count_lines(o_lines, "record_arg != expected")
    orc_post = count_lines(o_lines, 'p08_d1t_upd_dump(r4300, "post", s_p08_d1t_upd_post, rec, s_p08_d1t_marker')
    print(
        f"abi_guard host_accept={acc} host_accept_marker={bad_acc} host_post_record={post_rec} "
        f"host_post_marker={bad_post} oracle_cmp={orc} oracle_post_record={orc_post}"
    )
    if not (acc == 1 and bad_acc == 0 and post_rec == 1 and bad_post == 0 and orc == 1 and orc_post == 1):
        print("FAIL first-writer ABI source guard")
        fail = True

    for name, vram in CHECKS_AFTER_FIRST_WRITER:
        if not check(name, vram, funcs_lines, generated):
            fail = True

    for vram in ("0x800DC0C0", "0x800DC090"):
        n = sum(1 for line in src if re.search(r"add_loaded_function\(.*" + vram, line))
        print(f"host_bindings_{vram}={n}")
        if n != 1:
            print(f"FAIL binding {vram}")
            fail = True

    if fail:
        return 1
    print("PASS demo1 projectile wrapper coverage")
    return 0


if __name__ == "__main__":
    sys.exit(main())
