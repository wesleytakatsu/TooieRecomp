---
title: "BK — RSP ASP Microcode (n_aspMain)"
status: current
area: "specs/BK"
updated: 2026-08-08
tags:
  - banjo-recomp
  - rsp
  - audio
  - n64recomp
---

# BK — RSP ASP Microcode

Banjo uses the Nintendo *Audio Sample Processor* DSP microcode (`n_aspMain`)
on the N64 RSP. The `RSPRecomp` tool converts that RSP binary code into C just
like `N64Recomp` does for MIPS CPU code.

## Config: `n_aspMain.us.rev0.toml`

```toml
text_offset = 0xF4DDC0
text_size = 0xC60
text_address = 0x04001080
rom_file_path = "banjo.us.v10.decompressed.z64"
output_file_path = "rsp/n_aspMain.cpp"
output_function_name = "n_aspMain"
extra_indirect_branch_targets = [
    0x10F0, 0x13A0, 0x11A0, 0x1A68, 0x11CC, 0x17F0, 0x120C, 0x1280,
    0x134C, 0x124C, 0x1C88, 0x12D8, 0x12B0, 0x1388
]
```

The original file labels the array above with a comment (`# dispatch table`).

Key points:

- `text_offset`/`text_size` point inside the decompressed ROM at the ASP
  microcode.
- `text_address` — VRAM address where the RSP code is loaded.
- `output_file_path` — `rsp/n_aspMain.cpp` (kept under version control).
- `extra_indirect_branch_targets` — the RSP dispatch table; jump table
  targets the recompiler could not infer.

## Why it matters

N64 audio is not just playback: it runs DSP microcode for mixing, filtering,
and WES/Seq effects. Banjo audio keeps the same microcode. `RSPRecomp` makes
`n_aspMain` run natively.

## Build linkage

Run both recompilers when building:

```sh
./N64Recomp banjo.us.rev0.toml
./RSPRecomp n_aspMain.us.rev0.toml
```

See [Build](build.md).

## Tooie relevance

Banjo-Tooie likely uses the same or a similar `n_aspMain`/`n_aspMain2` family
of microcodes. The BT decomp repo's `tools/` does not recompile RSP, so this
part of the pipeline has to be replicated for Tooie.

See [TooieRecomp roadmap](../../roadmap/tooie-recomp.md).