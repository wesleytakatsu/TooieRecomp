---
title: "Índice da documentação"
status: current
area: "root"
updated: 2026-08-08
---

# Índice da documentação

## Notas

- [[AGENTS]] — Regras para agentes — Ordem de leitura, execução e evidências do TooieRecomp.
- [[MARKDOWN_VAULT_TOOLS]] — Guia de uso do MCP markdown-vault — Este vault usa as tools do MCP `markdown-vault` (`/home/wesley/projetos/markdown-vault-mcp-standalone`).
- [[README]] — Documentação — Esta pasta descreve o funcionamento do projeto de recompilação estática do Banjo-Tooie para PC (`N64Recomp` + `N64ModernRuntime` + `RT64`).

## roadmap

- [[roadmap/tooie-recomp]] — TooieRecomp Roadmap — Goal: build a static recomp of **Banjo-Tooie** for PC by adapting the infrastructure used by **Banjo: Recompiled**, feeding it the reverse engineering data from the **banjo-tooie-Recomp** decomp.

## plan

- [[plan/index]] — Plano mestre — Banjo-Tooie nativo no Linux — Caminho crítico, marcos, riscos e definição de sucesso.
- [[plan/definition-of-done]] — Definição de pronto — TooieRecomp — Gates funcionais, audiovisuais, técnicos e de distribuição.
- [[plan/baseline-manifest]] — Baseline manifest — TooieRecomp P00 — Ambiente, commits, ROM, decisões e licenças.

## plan/evidence (P08)

- [[plan/evidence/p08-actor-null-callback-diagnosis]] — P08 — Diagnóstico: callback nulo em `func_80108C90` (ActorInfo[0x38]) — o 0 é inválido (ROM tem `func_80107C2C`); causa: relocation de data de overlay descartada pelo gerador de metadados (resolvida por `p08-overlay-data-relocation-diagnosis`).
- [[plan/evidence/p08-intro-actor-spawn-diagnosis]] — P08 — Divergência no spawn de actors da intro (chintroticker/chintrocam/chintrochar não instanciados).
- [[plan/evidence/p08-session-handoff]] — P08 — Handoff da sessão: spawn de chintroticker (419), gemarkersDll e logo da Rare; relocation de data de overlay resolvida (ver `p08-overlay-data-relocation-diagnosis`).
- [[plan/evidence/p08-overlay-data-relocation-diagnosis]] — P08 — Relocation de data de overlay perdida (chcollballcontrol) — ROOT CAUSE A: `readelf -S` sem `-W` truncava nomes de seção; relocs existiam no ELF. Fix no gerador de metadados; `ActorInfo[0x38] == 0x80107C2C`; `LOOKUP_FUNC(0)` FIXED.
## plans/BT

- [[plans/BT/index]] — Etapas de implementação — TooieRecomp — Índice ordenado das fases.
- [[plans/BT/00-baseline-and-decisions]] — BT P00 — Baseline e decisões.
- [[plans/BT/01-linux-toolchain]] — BT P01 — Toolchain Linux.
- [[plans/BT/02-rom-decompression-and-elf]] — BT P02 — ROM, decomposição e ELF.
- [[plans/BT/03-recompiler-metadata]] — BT P03 — Metadados do recompilador (done).
- [[plans/BT/04-static-recompilation]] — BT P04 — Recompilação estática.
- [[plans/BT/05-native-runtime-and-boot]] — BT P05 — Runtime nativo e boot.
- [[plans/BT/06-overlays-and-syscalls]] — BT P06 — Overlays e syscalls.
- [[plans/BT/07-rsp-and-audio]] — BT P07 — RSP e áudio.
- [[plans/BT/08-rt64-input-and-frontend]] — BT P08 — RT64, entrada e frontend.
- [[plans/BT/09-gameplay-parity-and-enhancements]] — BT P09 — Paridade de gameplay e melhorias.
- [[plans/BT/10-linux-packaging-and-release]] — BT P10 — Empacotamento e release Linux.

## specs

- [[specs/BK/build]] — BK — Build — Full pipeline to go from the Banjo-Kazooie US ROM to a native executable, based on `BUILDING.md`.
- [[specs/BK/config]] — BK — Recompiler Config — `banjo.us.rev0.toml` configures `N64Recomp` for the Banjo-Kazooie NTSC 1.0 ROM. All paths are relative to the TOML location.
- [[specs/BK/features]] — BK — Features — Feature set of the port, mostly provided by the `RT64` rendering engine, `N64ModernRuntime` runtime, `RecompFrontend` UI, and the native patch layer.
- [[specs/BK/index]] — Banjo: Recompiled — Specifications for the `BanjoRecomp` folder — the reference port of Banjo-Kazooie built via static recompilation.
- [[specs/BK/overview]] — BK — Static Recompilation Overview — The game's own assets are **not** shipped. The executable grabs them from a user-provided ROM at runtime. The repository and releases contain no game assets.
- [[specs/BK/patches]] — BK — Native Patches — BanjoRecomp changes game behavior by compiling **native C patches** into a MIPS ELF, then recompiling that ELF with `N64Recomp` and registering the result as overlays. This is the mechanism that powers features like widescreen, note saving, and camera changes.
- [[specs/BK/rsp]] — BK — RSP ASP Microcode — Banjo uses the Nintendo *Audio Sample Processor* DSP microcode (`n_aspMain`) on the N64 RSP. The `RSPRecomp` tool converts that RSP binary code into C just like `N64Recomp` does for MIPS CPU code.
- [[specs/BK/runtime]] — BK — Runtime — How `BanjoRecomp` boots, registers recompiled code, and wires overlays, patches and game features through `N64ModernRuntime` / `RecompFrontend`.
- [[specs/BK/ui]] — BK — UI Layer — RecompUI launcher menu, keyframed SVG intro animation, in-game config tabs, theming, fonts.
- [[specs/BK/input]] — BK — Input & Controllers — SDL gamepad support, `recompinput` framework, default control bindings, camera/axis exports.
- [[specs/BK/mods]] — BK — Mods & Extension APIs — How `BanjoRecomp` lets third-party code attach to the recompiled game: per-object extension data, container APIs, and mod packages.
- [[specs/BK/ci]] — BK — CI/CD & Release — Automate building and publishing the port via GitHub Actions workflows.
- [[specs/BK/symbols]] — BK — BanjoRecompSyms — The `BanjoRecompSyms` submodule provides the symbol maps that let `N64Recomp` know which bytes map to which function. It is the BK analog of what `banjo-tooie-Recomp` produces via `symbol_addrs.us.txt` + ELF.
- [[specs/BT/build]] — BT — Build — The `Makefile` produces an ELF that matches the original ROM layout, then a recompressed runnable ROM. The invariant is byte-exact matching.
- [[specs/BT/index]] — Banjo Tooie Decomp Fork — Especificações do fork local `wesleytakatsu/banjo-tooie-Recomp` e de sua relação com o upstream `Mr-Wiseguy/banjo-tooie`.
- [[specs/BT/overlays]] — BT — Overlay System — Banjo-Tooie makes heavy use of **dynamically loaded code overlays** — the largest complicating factor for a static recomp. Overlays are code banks loaded into a fixed 0x80800000 VRAM region at runtime.
- [[specs/BT/overview]] — BT — Decompilation Overview — WIP decompilation of Banjo-Tooie (NTSC-U) by **Mr-Wiseguy** (author of `N64Recomp` / `N64ModernRuntime` / `rarezip`). Uses **Splat** to split the ROM and rebase the game to an ELF.
- [[specs/BT/setup]] — BT — Setup — Place a Banjo-Tooie NTSC-U ROM in the repo root as `baserom.us.z64`.
- [[specs/BT/sources]] — BT — Source Tree — Layout of `banjo-tooie-Recomp/src/` (generated by Splat), organized by memory segment.
- [[specs/BT/splat]] — BT — Splat configuration — `baserom.us.yaml` drives **Splat**, which splits the decompressed ROM into `src/`, `asm/`, and `assets/`.
- [[specs/BT/symbols]] — BT — Symbols and Relocations — The repo ships plain-text symbol and relocation maps used by Splat at build time. These are the raw material for a static-recomp symbol table.
- [[specs/BT/syscalls]] — BT — Overlay Syscall Dispatch — Overlay functions are called through a syscall table (`overlay_syscalls.s`): each entry is `syscall <overlay>` + an immediate with the entrypoint index; core2 intercepts and routes to the overlay.
- [[specs/BT/compression]] — BT — ROM Compression & Recompression — The cartridge ships deflated core1/core2 + overlays; the repo decompresses the baserom and rebuilds the compressed ROM byte-for-byte (CRC / xor scaffold).
- [[specs/BT/tools]] — BT — Tooling — C++ and Python tools under `banjo-tooie-Recomp/tools/`, built by `tools/Makefile` and used by the main Makefile.
- [[specs/BT/matching]] — BT — Matching & Decomp Workflow — How a `#pragma GLOBAL_ASM` stub becomes matched C code, and the tools + CI that verify byte-exact output.
- [[specs/BT/ci]] — BT — CI/CD — The single `ci.yml` workflow that builds the decomp, prints progress, and uploads it to the progress page.
- [[specs/index]] — Specifications Index — Technical specifications for the two reference projects used to build a Banjo-Tooie static recompilation.
