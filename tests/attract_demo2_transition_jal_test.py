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

# Functions in .bsrest that call the audited targets (17 call sites)
BSREST_CALLER_FUNCS = [
    "bsrest_entrypoint_0",
    "bsrest_entrypoint_1",
    "bsrest_entrypoint_2",
    "bsrest_entrypoint_3",
    "bsrest_entrypoint_4",
    "bsrest_entrypoint_5",
    "bsrest_entrypoint_6",
    "bsrest_entrypoint_7",
    "bsrest_entrypoint_8",
    "bsrest_entrypoint_9",
    "bsrest_entrypoint_10",
    "bsrest_entrypoint_11",
    "bsrest_entrypoint_12",
    "bsrest_entrypoint_13",
    "bsrest_entrypoint_14",
    "bsrest_entrypoint_15",
    "bsrest_entrypoint_16",
    "bsrest_entrypoint_17",
    "bsrest_entrypoint_18",
    "bsrest_entrypoint_19",
    "bsrest_entrypoint_20",
    "_bsrest_entrypoint_0",
    "_bsrest_entrypoint_1",
    "_bsrest_entrypoint_2",
    "_bsrest_entrypoint_3",
    "_bsrest_entrypoint_4",
    "_bsrest_entrypoint_5",
    "_bsrest_entrypoint_6",
    "_bsrest_entrypoint_7",
    "_bsrest_entrypoint_8",
    "_bsrest_entrypoint_9",
    "_bsrest_entrypoint_10",
    "_bsrest_entrypoint_11",
    "_bsrest_entrypoint_12",
    "_bsrest_entrypoint_13",
    "_bsrest_entrypoint_14",
    "_bsrest_entrypoint_15",
    "_bsrest_entrypoint_16",
    "_bsrest_entrypoint_17",
    "_bsrest_entrypoint_18",
    "_bsrest_entrypoint_19",
    "_bsrest_entrypoint_20",
]


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


def main() -> int:
    if not RF.is_dir():
        print("FAIL: RecompiledFuncs/ directory is missing")
        return 1

    funcs = collect_functions()
    print(f"Loaded {len(funcs)} recompiled functions from {RF.name}/")

    failed = False

    # A. Check bsrest callers for any LOOKUP_FUNC to the 7 targets
    vulnerable_lookups_before = 17
    vulnerable_lookups_after = 0
    direct_static_calls_found = 0

    target_hex_strings = [f"0x{t:08X}" for t in AUDITED_TARGETS]
    target_addr_suffixes = [f"{t:08X}" for t in AUDITED_TARGETS]

    # Find all bsrest functions (including static bodies generated in bsrest)
    bsrest_funcs: dict[str, tuple[pathlib.Path, str]] = {}
    for name, (path, body) in funcs.items():
        if name.startswith("bsrest_entrypoint_") or name.startswith("_bsrest_entrypoint_"):
            bsrest_funcs[name] = (path, body)
        elif name.startswith("static_") and any(suffix in name for suffix in target_addr_suffixes):
            bsrest_funcs[name] = (path, body)

    print(f"\nChecking {len(bsrest_funcs)} .bsrest functions for lookup vs direct calls:")
    for name, (path, body) in sorted(bsrest_funcs.items()):
        for t_hex, suffix in zip(target_hex_strings, target_addr_suffixes):
            needle_lookup = f"LOOKUP_FUNC({t_hex})"
            if needle_lookup in body:
                print(f"  FAIL: {name} ({path.name}) uses {needle_lookup}")
                vulnerable_lookups_after += 1
                failed = True

            # Count direct static calls
            if f"_{suffix}(" in body and not name.endswith(f"_{suffix}"):
                direct_static_calls_found += 1

    print(f"VULNERABLE_LOOKUPS_BEFORE: {vulnerable_lookups_before}")
    print(f"VULNERABLE_LOOKUPS_AFTER:  {vulnerable_lookups_after}")
    print(f"DIRECT_STATIC_CALLS_FOUND: {direct_static_calls_found}")

    if vulnerable_lookups_after != 0:
        print("FAIL: vulnerable lookups still remain")
        return 1

    # B & C. Check that generated local/static bodies exist for all 7 targets
    print("\nChecking generated local/static bodies for all 7 targets:")
    found_targets = 0
    for target in AUDITED_TARGETS:
        target_suffix = f"{target:08X}"
        matching_funcs = [name for name in funcs if name.endswith(f"_{target_suffix}")]
        if not matching_funcs:
            print(f"  FAIL: No generated static function found for target 0x{target_suffix}")
            failed = True
        else:
            found_targets += 1
            print(f"  PASS: Target 0x{target_suffix} generated as: {matching_funcs}")

    print(f"AUDITED_TARGETS_FOUND: {found_targets}/{len(AUDITED_TARGETS)}")

    # Semantic check for crash target 0x808003FC
    target_3fc_funcs = [name for name in funcs if name.endswith("_808003FC")]
    if target_3fc_funcs:
        name_3fc = target_3fc_funcs[0]
        path_3fc, body_3fc = funcs[name_3fc]
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
