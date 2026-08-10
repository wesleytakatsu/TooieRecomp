---
title: "BT — Matching & Decomp Workflow"
status: current
area: "specs/BT"
updated: 2026-08-08
tags:
  - banjo-tooie-Recomp
  - matching
  - workflow
---

# BT — Matching & Decomp Workflow

How a `#pragma GLOBAL_ASM` stub becomes matched C code in `banjo-tooie-Recomp`, and
the tools + CI that verify byte-exact output.

## The loop

1. Splat leaves an unmatched function as assembly in
   `asm/nonmatchings/<seg>/<func>.s` and a stub at the matching address.
2. Decompiler reverse-engineers the C (via `m2c` / `decomp.me`) by pasting
   the same context file `tools/m2ctx.py` produces, writing the C into
   `src/`.
3. `make` recompiles with IDO; `make check` diffs built vs baserom.
4. `tools/progress.py` reports the new match % per folder.
5. `tools/restructure.py` moves the renamed file into the right
   `src/overlays/<prefix>/` and rewrites `baserom.us.yaml` relabeled
   subsegments.
6. CI re-verifies on PR (see [CI](ci.md)).

## `make check`

```make
check: $(ROM)
	diff $(DECOMPRESSED_BASEROM) $(UNCOMPRESSED_ROM) && printf "OK\n"
```

Byte-exact against `decompressed.us.z64` is the acceptance bar.

## Diff tooling

- **`tools/diff.py`** — the workhorse. `./diff -wm3 <symbol>` opens an
  interactive N64 MIPS diff (`-m3` uses `mips3`?). Args mirror the zeldaret
  tool of the same name.
- **`tools/first_diff.py`** — `-c 5 -v us`: prints the first instruction
  differences between the built ROM and `decompressed.us.z64`, decoding
  each with `rabbitizer` and resolving `jal` targets from the map file.
- **`diff_settings.py`** (repo root) — the `diff.py` hook: mapfile,
  myimg (`build us…/z64`), baseimg (`decompressed.us.z64`), `-j8`, the
  folder(s) to search.

## `tools/m2ctx` / permuter

- `tools/m2ctx.py` — produces `ctx.c` for the decompiler: preprocesses the
  target `.c` with the project flags and emits a context file.
- `tools/permuter_settings.toml` — configures `permuter` to test IDO `-O2`
  variants against the target.

## Naming & restructure

`tools/restructure.py` (remember `DRY_RUN = True` until you mean to run):

- `PREFIX_MAP` maps flat overlay `.c` files into hierarchy
  (`baegg -> overlays/ba/egg`, `chgrunty -> ch/grunty`, …).
- Rewrites the config `dir` field for the affected segments.
- No-prefix files fall back to `overlays/<2 first letters>/`.
- `list_overlays.py` lists active (non-empty) overlays for the build.

## Notes

`symbol_addrs_us.txt` + `ovl_*` maps keep the address->name translation at
every step; these are conserved while Splat re-splits.

See [Tools](tools.md), [Symbols](symbols.md), [Build](build.md).