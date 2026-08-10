---
title: "Banjo: Recompiled — Index"
status: current
area: "specs/BK"
updated: 2026-08-08
tags:
  - banjo-recomp
  - specs
---

# Banjo: Recompiled

Specifications for the `BanjoRecomp` folder — the reference port of
Banjo-Kazooie built via static recompilation.

## Notes

- [Overview](overview.md) — what the project is, libs used, plug-and-play.
- [Config](config.md) — `banjo.us.rev0.toml` recompiler options.
- [Symbols](symbols.md) — `BanjoRecompSyms` (`syms` + `datasyms`).
- [Patches](patches.md) — native patch layer (`patches/` + `patches.toml`).
- [RSP](rsp.md) — ASP microcode recompilation (`n_aspMain`).
- [Build](build.md) — full end-to-end build pipeline.
- [Runtime](runtime.md) — launcher, `src/main`, `src/game`, overlay/patch
  registration.
- [UI](ui.md) — RecompUI launcher, animated intro, config tabs, theming.
- [Input](input.md) — controller DB, `recompinput`, bindings, input exports.
- [Features](features.md) — framerate, widescreen, mods, audio, save data.
- [Mods](mods.md) — modding & extension APIs (`recomp_data_api`,
  `recomp_extension_api`, mod containers).
- [CI](../BK/ci.md) — release automation and GitHub Actions.
- [Contrato de reutilização](reuse-contract.md) — o que pode ser reaproveitado no TooieRecomp e o que precisa ser substituído.

## The whole spec chain

Specs index: [Index](../index.md)
Roadmap link: [TooieRecomp](../../roadmap/tooie-recomp.md)
