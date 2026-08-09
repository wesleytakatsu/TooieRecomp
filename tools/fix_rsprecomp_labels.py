#!/usr/bin/env python3
"""Fix RSPRecomp label masking when n_aspMain text spans past 0x1FFF.

RSPRecomp uses `addr & 0x1FFF` for branch targets, so a branch to 0x205C
becomes `goto L_005C` while the instruction label (if any) is at L_205C /
never emitted. This post-pass:

1. Walks generated instructions with sequential VRAM from text_address.
2. Inserts missing `L_XXXX:` labels at instruction addresses that are
   referenced by branches (masked or unmasked).
3. Rewrites `goto L_MASKED` to the unmasked VRAM label when unique.

Safe no-op when no missing labels remain.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

RSP_MEM_MASK = 0x1FFF
LABEL_RE = re.compile(r"^L_([0-9A-Fa-f]+):$")
GOTO_RE = re.compile(r"\bgoto L_([0-9A-Fa-f]+);")
# Lines that are the primary comment for an original instruction.
INSTR_COMMENT_RE = re.compile(r"^\s*// ")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "path",
        type=Path,
        nargs="?",
        default=Path(__file__).resolve().parents[1] / "rsp" / "n_aspMain.cpp",
    )
    ap.add_argument("--text-address", type=lambda x: int(x, 0), default=0x1080)
    ap.add_argument("--write", action="store_true", help="rewrite file in place")
    args = ap.parse_args()

    lines = args.path.read_text().splitlines(keepends=True)
    # Strip ends for analysis, keep original endings on write.
    raw = [ln.rstrip("\n") for ln in lines]

    # Map line index → sequential VRAM for each instruction-comment line.
    vram = args.text_address
    line_vram: dict[int, int] = {}
    for i, ln in enumerate(raw):
        if INSTR_COMMENT_RE.match(ln) and not ln.strip().startswith("// nop"):
            # Delay-slot "// nop" still advances PC when it is a real nop instr;
            # RSPRecomp emits "// nop" for actual nops too — count all // lines
            # that look like disassembly (not C++ comments elsewhere).
            # Heuristic: instruction comments are indented with 4 spaces.
            if ln.startswith("    //"):
                line_vram[i] = vram
                vram += 4

    existing_labels = {
        int(m.group(1), 16): i
        for i, ln in enumerate(raw)
        if (m := LABEL_RE.match(ln.strip()))
    }

    goto_targets: set[int] = set()
    for ln in raw:
        for m in GOTO_RE.finditer(ln):
            goto_targets.add(int(m.group(1), 16))

    # Resolve masked targets → unique unmasked VRAM among instruction addresses.
    instr_vrams = sorted(set(line_vram.values()))
    resolve: dict[int, int] = {}
    for t in goto_targets:
        if t in existing_labels or t in instr_vrams:
            resolve[t] = t
            continue
        cands = [v for v in instr_vrams if (v & RSP_MEM_MASK) == (t & RSP_MEM_MASK)]
        if len(cands) == 1:
            resolve[t] = cands[0]
        elif len(cands) > 1:
            # Prefer the candidate in the high page when masked value is low.
            high = [c for c in cands if c >= 0x2000]
            resolve[t] = high[0] if len(high) == 1 else cands[-1]
            print(
                f"warning: ambiguous target {t:#06x} → candidates "
                f"{[hex(c) for c in cands]}, picked {resolve[t]:#06x}",
                file=sys.stderr,
            )
        else:
            print(f"error: cannot resolve goto L_{t:04X}", file=sys.stderr)
            return 1

    # Rewrite gotos
    new_raw = []
    goto_fixes = 0
    for ln in raw:
        def repl(m: re.Match[str]) -> str:
            nonlocal goto_fixes
            old = int(m.group(1), 16)
            new = resolve.get(old, old)
            if new != old:
                goto_fixes += 1
            return f"goto L_{new:04X};"

        new_raw.append(GOTO_RE.sub(repl, ln))

    # Also fix disassembly comments L_XXXX for branches (cosmetic)
    comment_fix = re.compile(r"\bL_([0-9A-Fa-f]+)\b")

    def fix_comment(ln: str) -> str:
        if not ln.startswith("    //"):
            return ln

        def crepl(m: re.Match[str]) -> str:
            old = int(m.group(1), 16)
            new = resolve.get(old, old)
            return f"L_{new:04X}"

        return comment_fix.sub(crepl, ln)

    new_raw = [fix_comment(ln) for ln in new_raw]

    # Insert missing labels before the instruction at that VRAM.
    needed = set(resolve.values()) | {
        resolve[t] for t in goto_targets if t in resolve
    }
    # Recompute existing after rewrite (same positions)
    existing_labels = {
        int(m.group(1), 16): i
        for i, ln in enumerate(new_raw)
        if (m := LABEL_RE.match(ln.strip()))
    }

    # Build reverse map vram → first instruction-comment line
    vram_to_line: dict[int, int] = {}
    for i, v in line_vram.items():
        vram_to_line.setdefault(v, i)

    inserts: dict[int, list[str]] = {}  # line index → labels to insert before
    for addr in sorted(needed):
        if addr in existing_labels:
            continue
        if addr not in vram_to_line:
            print(f"error: no instruction at {addr:#06x} to label", file=sys.stderr)
            return 1
        inserts.setdefault(vram_to_line[addr], []).append(f"L_{addr:04X}:")

    out_lines: list[str] = []
    inserted = 0
    for i, ln in enumerate(new_raw):
        if i in inserts:
            for lab in inserts[i]:
                out_lines.append(lab)
                inserted += 1
        out_lines.append(ln)

    # Validate no missing gotos
    final = "\n".join(out_lines) + "\n"
    labels = set(re.findall(r"^L_([0-9A-Fa-f]+):", final, re.M))
    gotos = set(re.findall(r"goto L_([0-9A-Fa-f]+);", final))
    missing = sorted(gotos - labels, key=lambda x: int(x, 16))
    if missing:
        print(f"error: still missing labels: {missing}", file=sys.stderr)
        return 1

    print(
        f"fix_rsprecomp_labels: goto_rewrites={goto_fixes} "
        f"labels_inserted={inserted} path={args.path}"
    )
    if args.write:
        args.path.write_text(final)
    else:
        sys.stdout.write(final)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
