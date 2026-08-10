---
title: "Banjo Tooie Decomp Fork — Index"
description: "Especificações do fork local wesleytakatsu/banjo-tooie-Recomp e de sua relação com o upstream Mr-Wiseguy/banjo-tooie."
status: current
area: "specs/BT"
updated: 2026-08-09
source_path: banjo-tooie-Recomp
source_repo: wesleytakatsu/banjo-tooie-Recomp
upstream_repo: Mr-Wiseguy/banjo-tooie
tags:
  - banjo-tooie-Recomp
  - specs
---

# Banjo Tooie Decomp Fork

Specifications for the local `banjo-tooie-Recomp/` folder: Wesley Takatsu's
fork of Mr-Wiseguy's WIP Banjo-Tooie NTSC-U decompilation using Splat.

At the verified baseline, `origin` is
`https://github.com/wesleytakatsu/banjo-tooie-Recomp.git`, `upstream` is
`https://github.com/Mr-Wiseguy/banjo-tooie.git`, and `master` points to
`25033be6ec9e35975483f064d086391d2381ca4c`. The local branch is 0 ahead and
0 behind `upstream/master`. See [Setup](setup.md) before running the build.

## Notes

- [Overview](overview.md) — decomp project, progress, goals.
- [Setup](setup.md) — baserom requirements, checkout, dependencies.
- [Splat](splat.md) — `baserom.us.yaml` segment layout.
- [Build](build.md) — `Makefile` pipeline to ELF + recompressed ROM.
- [Symbols](symbols.md) — symbol and relocation maps for the recompiler.
- [Overlays](overlays.md) — dynamic overlay architecture (851 overlays).
- [Syscalls](syscalls.md) — overlay dispatch via the syscall table (decomp + TooieRecomp P06).
- [RSP e áudio](rsp-and-audio.md) — n_aspMain / OSTask / pipeline SDL (TooieRecomp P07).
- [Input e controles](input.md) — contrato compartilhado com BanjoRecomp, perfis, persistência e rumble.
- [Compression](compression.md) — cartridge/overlay deflate + CRC scaffold.
- [Tools](tools.md) — C++ tooling for decompression / compression / overlays.
- [Sources](sources.md) — `src/` tree layout and decompilation style.
- [Matching](matching.md) — per-function decomp/matching workflow.
- [CI](ci.md) — GitHub Actions build + progress upload.
- [Prontidão para recompilação](recomp-readiness.md) — insumos existentes, contratos e lacunas para o port nativo.

## Why it matters for TooieRecomp

This repo produces exactly the raw material a `N64Recomp` recompiler needs:

- a built ELF (`build/us/banjotooie_decompressed.elf`)
- relocations (`reloc_addrs.us.txt`, `ovl_reloc_addrs.us.txt`)
- symbols (`symbol_addrs.us.txt`, `ovl_symbol_addrs.us.txt`,
  `syscall_symbol_addrs.us.txt`)
- overlay table metadata (`overlays.us.toml` + overlay headers)

See [TooieRecomp roadmap](../../roadmap/tooie-recomp.md) (histórico) and
implementation stage [[../../plans/BT/06-overlays-and-syscalls]] for the
current native syscall dispatch.
