#!/usr/bin/env python3
"""Autonomous check for the P08 attract DEMO2→next-scene crash fix.

Validates the structural resolution of unnamed same-section overlay JAL targets:
A. bsrest 0x808003FC call does NOT emit LOOKUP_FUNC.
B. A generated local/static body for the original 0x808003FC target exists and matches semantics.
C. All seven audited unnamed targets in .bsrest are represented as generated local bodies.
D. All 17 vulnerable call sites are no longer lookup-bound (VULNERABLE_LOOKUPS_AFTER = 0).

Does not run the full game.
"""
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
RF = ROOT / "RecompiledFuncs"

# Seven audited unnamed targets in .bsrest (VRAM range 0x80800000..0x80800940)
AUDITED_TARGETS = [
    0x80800000,
    0x8080003C,
    0x80800080,
    0x808000E4,
    0x8080020C,
    0x8080032C,
    0x808003FC,
]

# Original caller VRAM, target VRAM pairs selected by the 17-site audit.
# The final entry is the unnamed same-section call inside 0x8080020C itself.
EXPECTED_CALL_SITES = (
    ("bsrest_entrypoint_6", 0x80800550, 0x8080032C),
    ("bsrest_entrypoint_2", 0x8080048C, 0x8080032C),
    ("bsrest_entrypoint_11", 0x80800668, 0x808003FC),
    ("bsrest_entrypoint_11", 0x80800670, 0x8080003C),
    ("bsrest_entrypoint_14", 0x80800700, 0x8080003C),
    ("bsrest_entrypoint_1", 0x80800450, 0x8080020C),
    ("bsrest_entrypoint_17", 0x80800810, 0x80800000),
    ("bsrest_entrypoint_17", 0x80800818, 0x80800080),
    ("bsrest_entrypoint_19", 0x808008B4, 0x808000E4),
    ("bsrest_entrypoint_5", 0x80800514, 0x8080020C),
    ("bsrest_entrypoint_9", 0x808005D8, 0x8080020C),
    ("bsrest_entrypoint_9", 0x808005E0, 0x80800080),
    ("bsrest_entrypoint_3", 0x808004C8, 0x808003FC),
    ("bsrest_entrypoint_10", 0x80800608, 0x808000E4),
    ("bsrest_entrypoint_10", 0x80800624, 0x8080032C),
    ("bsrest_entrypoint_7", 0x8080058C, 0x808003FC),
    ("static_344_8080020C", 0x80800308, 0x80800000),
)


def collect_functions() -> dict[str, tuple[pathlib.Path, str]]:
    """Index all generated functions by name -> (file_path, function_body)."""
    func_map: dict[str, tuple[pathlib.Path, str]] = {}
    func_sig_re = re.compile(r"RECOMP_FUNC void ([a-zA-Z0-9_]+)\(uint8_t\* rdram, recomp_context\* ctx\) \{")

    for path in sorted(RF.glob("funcs_*.c")):
        content = path.read_text(errors="replace")
        matches = list(func_sig_re.finditer(content))
        for i, m in enumerate(matches):
            fname = m.group(1)
            start_pos = m.start()
            end_pos = matches[i + 1].start() if i + 1 < len(matches) else len(content)
            body = content[start_pos:end_pos]
            func_map[fname] = (path, body)
    return func_map


def collect_function_defs() -> list[tuple[str, pathlib.Path, str]]:
    """Return every generated function definition without collapsing duplicate names."""
    defs: list[tuple[str, pathlib.Path, str]] = []
    func_sig_re = re.compile(r"RECOMP_FUNC void ([a-zA-Z0-9_]+)\(uint8_t\* rdram, recomp_context\* ctx\) \{")

    for path in sorted(RF.glob("funcs_*.c")):
        content = path.read_text(errors="replace")
        matches = list(func_sig_re.finditer(content))
        for i, match in enumerate(matches):
            end_pos = matches[i + 1].start() if i + 1 < len(matches) else len(content)
            defs.append((match.group(1), path, content[match.start() : end_pos]))
    return defs


def main() -> int:
    if not RF.is_dir():
        print("FAIL: RecompiledFuncs/ directory is missing")
        return 1

    funcs = collect_functions()
    function_defs = collect_function_defs()
    print(f"Loaded {len(funcs)} recompiled functions from {RF.name}/")

    failed = False

    # A. Check every audited original call site and require its exact section-local target.
    vulnerable_lookups_before = 17
    direct_static_calls_found = 0
    wrong_static_calls = 0
    missing_calls = 0
    lookup_calls = 0

    print(f"\nChecking {len(EXPECTED_CALL_SITES)} audited original JAL call sites:")
    for caller_name, caller_vram, target in EXPECTED_CALL_SITES:
        caller_hex = f"0x{caller_vram:08X}"
        target_hex = f"0x{target:08X}"
        target_name = f"static_344_{target:08X}"
        comment = f"// {caller_hex}: jal         {target_hex}"
        candidates = [item for item in function_defs if item[0] == caller_name]
        matches = [item for item in candidates if comment in item[2]]

        if not matches:
            print(f"  MISSING: {caller_name} {caller_hex} -> {target_hex}")
            missing_calls += 1
            failed = True
            continue

        _, path, body = matches[0]
        call_start = body.index(comment)
        call_window = body[call_start : call_start + 1200]
        lookup = f"LOOKUP_FUNC({target_hex})" in call_window
        direct = f"{target_name}(rdram, ctx);" in call_window
        wrong = re.search(r"static_[0-9]+_808[0-9A-Fa-f]+\(rdram, ctx\);", call_window)

        if lookup:
            print(f"  LOOKUP_REMAINS: {caller_name} {caller_hex} -> {target_hex} ({path.name})")
            lookup_calls += 1
            failed = True
        elif direct:
            direct_static_calls_found += 1
        elif wrong:
            print(f"  WRONG_STATIC: {caller_name} {caller_hex} -> {target_hex} ({path.name})")
            wrong_static_calls += 1
            failed = True
        else:
            print(f"  MISSING_CALL: {caller_name} {caller_hex} -> {target_hex} ({path.name})")
            missing_calls += 1
            failed = True

    vulnerable_lookups_after = sum(
        1
        for _, _, body in function_defs
        for target in AUDITED_TARGETS
        if f"LOOKUP_FUNC(0x{target:08X})" in body
    )
    print(f"VULNERABLE_LOOKUPS_BEFORE: {vulnerable_lookups_before}")
    print(f"VULNERABLE_LOOKUPS_AFTER:  {vulnerable_lookups_after}")
    print(f"DIRECT_STATIC_CALLS_FOUND: {direct_static_calls_found}")
    print(f"WRONG_STATIC_CALLS: {wrong_static_calls}")
    print(f"CALLS_MISSING: {missing_calls}")
    print(f"LOOKUP_CALLS_REMAINING: {lookup_calls}")

    if (
        direct_static_calls_found != len(EXPECTED_CALL_SITES)
        or wrong_static_calls != 0
        or missing_calls != 0
        or lookup_calls != 0
        or vulnerable_lookups_after != 0
    ):
        print("FAIL: audited call-site mapping is incomplete or lookup-bound")
        return 1

    # B & C. Require exactly one generated helper in section 344 (.bsrest).
    print("\nChecking generated .bsrest/static bodies for all 7 targets:")
    found_targets = 0
    for target in AUDITED_TARGETS:
        target_name = f"static_344_{target:08X}"
        matching_defs = [item for item in function_defs if item[0] == target_name]
        if len(matching_defs) != 1:
            print(f"  FAIL: Expected exactly one {target_name} definition, found {len(matching_defs)}")
            failed = True
        else:
            found_targets += 1
            print(f"  PASS: 0x{target:08X} -> {target_name} ({matching_defs[0][1].name})")

    print(f"AUDITED_TARGETS_FOUND: {found_targets}/{len(AUDITED_TARGETS)}")

    # Semantic check for crash target 0x808003FC
    target_3fc_defs = [item for item in function_defs if item[0] == "static_344_808003FC"]
    if len(target_3fc_defs) == 1:
        name_3fc, path_3fc, body_3fc = target_3fc_defs[0]
        has_800a0c20 = "0x800A0C20" in body_3fc
        has_800a0d14 = "0x800A0D14" in body_3fc
        has_half_float_arg = "0X3F00" in body_3fc or "0x3F00" in body_3fc or "0x3f00" in body_3fc
        has_stack_frame = "-0X18" in body_3fc or "-0x18" in body_3fc or "-24" in body_3fc

        print(f"\nSemantic verification for {name_3fc} ({path_3fc.name}):")
        print(f"  stack_frame:       {'PASS' if has_stack_frame else 'FAIL'}")
        print(f"  calls_func_800A0C20: {'PASS' if has_800a0c20 else 'FAIL'}")
        print(f"  calls_func_800A0D14: {'PASS' if has_800a0d14 else 'FAIL'}")
        print(f"  arg_0.5f_lui:      {'PASS' if has_half_float_arg else 'FAIL'}")

        if not (has_800a0c20 and has_800a0d14 and has_half_float_arg and has_stack_frame):
            print("FAIL: Semantic check failed for 0x808003FC static body")
            failed = True
    else:
        print("FAIL: 0x808003FC static body missing")
        failed = True

    if failed:
        print("\nOVERALL: FAIL")
        return 1

    print("\nOVERALL: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
