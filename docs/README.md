---
title: "Documentação"
status: current
area: "root"
updated: 2026-08-09
---

# Documentação

Esta pasta descreve o funcionamento do projeto de recompilação estática do
Banjo-Tooie para PC (`N64Recomp` + `N64ModernRuntime` + `RT64`).

## Índices

- [VAULT (índice hierárquico)](VAULT.md)
- [Regras para agentes](AGENTS.md)
- [Markdown Vault MCP (guia de uso)](MARKDOWN_VAULT_TOOLS.md) — workspace = TooieRecomp; vault = `docs/`
- [Specifications (hub)](specs/index.md)
- [Reverse-engineering knowledge](reverse-engineering/README.md) — current model of the game (not forensic evidence)

## Especificações por projeto

- [Banjo: Recompiled — specs](specs/BK/index.md) — referência de port estático
  (configs, patches, RSP, build, runtime, UI, input, features, símbolos,
  mods, CI).
- [Banjo Tooie Decomp — specs](specs/BT/index.md) — fork local de Wesley Takatsu do decomp do Mr-Wiseguy
  (setup, splat, build, símbolos/relocs, overlays, syscalls, compressão,
  tools, sources, matching, CI).
- [ANGRYLION_CONTROL oracle](specs/BT/angrylion-control-oracle.md) — Mupen + Angrylion RDP Plus + CXD4 LLE
  para medição P08; o executável TooieRecomp continua no RT64.

## Roadmap

- [TooieRecomp Roadmap](roadmap/tooie-recomp.md) — plano para recompilar
- [Plano mestre](plan/index.md) — caminho crítico, marcos, riscos e definição de sucesso
- [Baseline P00](plan/baseline-manifest.md) — manifesto de ambiente, commits, ROM e decisões
- [Etapas de implementação BT](plans/BT/index.md) — tarefas e gates executáveis por fase
  Banjo-Tooie reaproveitando os dois projetos acima.

## Estado

- Projeto em fase de pesquisa/engenharia reversa.
- `instruções.md` (raiz) contém a análise de viabilidade do `TooieRecomp`.
- Proveniência e linhagem: `BanjoRecomp/` serviu como projeto de referência inicial e `banjo-tooie-Recomp/` (fork local; `origin` de Wesley Takatsu e `upstream` do decomp do Mr-Wiseguy) como fonte de decompilação; sob a arquitetura standalone da P10, as fontes e runtimes necessários estão vendorizados diretamente em `lib/`, `decomp/` e `assets/`. O checkout antigo `banjo-tooie/` não é usado.

## Regra de manutenção

- Documentar comportamento existente e verificável.
- Uma única heading `#` por nota.
- Notas acima de 30 KB viram hub + partes.
- Toda nota tem frontmatter mínimo.
- Ao alterar, atualizar o índice ([VAULT.md](VAULT.md)).
