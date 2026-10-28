---
title: "Regras para agentes — TooieRecomp"
description: "Navegação da documentação, regras de implementação e de documentação para agentes no TooieRecomp."
status: current
type: agent-rules
project: TooieRecomp
updated: 2026-10-23
tags: [agents, workflow, tooierecomp, rules]
---

# Regras para agentes — TooieRecomp

## Workspace

Abra o CLI/IDE em `TooieRecomp/` (este repositório). Configs de agentes e MCP (`.mcp.json`, `.codex/`, `opencode.json`
etc.) são pessoais e ficam fora do git (`.gitignore`). O projeto é **standalone**: RT64, N64ModernRuntime/N64Recomp, RecompFrontend, SDL2 (Windows),
assets de interface, decomp e wheels estão vendorizados (`lib/`, `decomp/`, `assets/`, `tools/wheelhouse/`; proveniência em
`vendor/`). Sem checkouts irmãos, submódulos ou downloads. Fora do git: a ROM canônica do usuário (`Banjo-Tooie (USA).z64`,
33554432 bytes, MD5 `40e98faa24ac3ebe1d25cb5e5ddf49e4`) e a imagem descomprimida gerada a partir dela (35098224 bytes,
SHA-1 `af479771ff5e91ce3732f3d84146614ab71d8d79`).

## Ordem de leitura

1. `AGENTS.md` (raiz).
2. Esta nota.
3. [[HANDOFF]] — estado atual, pendências e próxima ação.
4. Somente as specs ([[specs/index]]) e páginas de RE ([[reverse-engineering/README]]) da área da tarefa.
5. Escolha um objetivo delimitado antes de editar.

## Regras de execução

- Corrija geradores, metadados ou a camada dona; nunca edite código gerado (`RecompiledFuncs/**`).
- Código vendorizado é editável, mas toda mudança vira patch em `vendor/patches-local/` (`vendor/UPDATING.md`).
- Preserve comportamento, timing e 4:3 originais antes de adicionar melhorias; melhorias têm chave de desligar.
- Diagnósticos ficam default OFF (variável `TOOIE_DIAG_*`/`TOOIE_TEST_*`), nunca logando por chamada/frame sem flag.
- Não copie, distribua ou versione ROM e assets proprietários.
- Meça antes de corrigir (diagnóstico default-OFF, A/B com a chave da correção) e valide com teste e build.

## Documentação

- `docs/specs/` = contratos estáveis do sistema como ele é agora (sem números de medição nem diário de sessão).
- `docs/reverse-engineering/` = melhor modelo atual do jogo (funções, structs, sistemas), com estado de verdade explícito.
- [[HANDOFF]] = estado corrente curto. Não acumula histórico.
- Detalhes de investigação vão na mensagem do commit; não há mais pasta de evidências.
- Classifique conclusões: `PROVEN`, `STRONG_INFERENCE`, `HYPOTHESIS`, `UNKNOWN`, `REFUTED`, `CORRECTED`. Nunca promova
  hipótese em silêncio nem para spec.
- Ao mudar comportamento: atualize a spec da área, o RE quando o conhecimento do jogo mudar e o [[HANDOFF]].
- Antes de encerrar: `git diff --check` e status do worktree.
