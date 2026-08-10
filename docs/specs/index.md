---
title: "Specifications Index"
status: current
area: "specs"
updated: 2026-08-08
tags:
  - docs
  - index
---

# Specifications Index

Technical specifications for the two reference projects used to build a
Banjo-Tooie static recompilation.

## BK — Banjo: Recompiled

Reference implementation of a static recomp port built with `N64Recomp` +
`RT64` + `N64ModernRuntime` + `RecompFrontend`. Used as the baseline to copy,
adapt, and extend for Tooie.

- [BK index](BK/index.md) — hub of all BK spec notes
- [Overview](BK/overview.md) — what the project is and produces
- [Config](BK/config.md) — `banjo.us.rev0.toml` recompiler config
- [RSP](BK/rsp.md) — `n_aspMain.us.rev0.toml` ASP microcode
- [Patches](BK/patches.md) — native patch ELF and `patches.toml`
- [Build](BK/build.md) — full build pipeline from ROM to executable
- [Runtime](BK/runtime.md) — launcher, game, overlay and patch registration
- [UI](BK/ui.md) — launcher menu, animated intro, config tabs, theming
- [Input](BK/input.md) — controller DB, bindings, input exports
- [Symbols](BK/symbols.md) — `BanjoRecompSyms` symbol maps
- [Features](BK/features.md) — framerate, widescreen, mods, audio
- [Mods](BK/mods.md) — modding & extension APIs
- [CI](BK/ci.md) — release automation
- [Contrato de reutilização](BK/reuse-contract.md) — fronteira entre infraestrutura reaproveitável e dados específicos de BK

## BT

- [Banjo Tooie Decomp](BT/index.md) — overview of all BT notes
- [Overview](BT/overview.md) — WIP decompilation by Mr-Wiseguy
- [Setup](BT/setup.md) — baserom, dependencies, submodules
- [Splat](BT/splat.md) — `baserom.us.yaml` segment config
- [Build](BT/build.md) — Makefile pipeline to ELF and recompressed ROM
- [Symbols](BT/symbols.md) — symbol and relocation maps
- [Overlays](BT/overlays.md) — dynamic overlay system
- [Syscalls](BT/syscalls.md) — overlay dispatch via syscall table
- [Compression](BT/compression.md) — cartridge/overlay deflate + CRC
- [Tools](BT/tools.md) — C++/Python tooling
- [Sources](BT/sources.md) — `src/` tree layout
- [Matching](BT/matching.md) — decomp/matching workflow
- [CI](BT/ci.md) — CI build + progress
- [Prontidão para recompilação](BT/recomp-readiness.md) — artefatos disponíveis, contratos e lacunas

## Cross-project

- [TooieRecomp Roadmap](../roadmap/tooie-recomp.md) — plan to reuse BK and BT
- [Plano mestre TooieRecomp](../plan/index.md) — execução ordenada para Linux nativo
- [Etapas de implementação BT](../plans/BT/index.md) — tarefas, gates e evidências por fase
  work to build a Tooie static recomp
