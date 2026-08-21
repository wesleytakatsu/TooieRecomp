---
title: "Regras para agentes — TooieRecomp"
description: "Regras de navegação, implementação e evidência para agentes que trabalham no plano TooieRecomp."
status: current
type: agent-rules
project: TooieRecomp
tags: [agents, workflow, tooierecomp, rules]
---

# Regras para agentes — TooieRecomp

## Workspace

Abra o CLI/IDE em `TooieRecomp/` (este repositório git). Não use
`N64-Recomp/` como raiz do agente: as configs de MCP/vault agora vivem
aqui (`docs/MARKDOWN_VAULT_TOOLS.md`).

Irmãos no disco, fora deste git, que o **build** ainda usa:

- `../BanjoRecomp` — RT64, N64ModernRuntime, assets (`CMakeLists.txt` `BK_ROOT`)
- `../Banjo-Tooie (USA).z64` — cartucho do oráculo Angrylion (`TOOIE_CART_Z64`)
- `../banjo-tooie-Recomp` — decomp; não é o executável Tooie

Não copie `.venv` do pai. Não trate `BanjoRecomp` como o jogo.

## Ordem de leitura

1. Leia [[plan/index|o plano mestre]].
2. Localize a fase ativa em [[plans/BT/index]].
3. Leia somente as specs BK/BT relacionadas pela nota da fase.
4. Selecione um ID de tarefa aberto antes de alterar código.

## Regras de execução

- Não pule o gate de saída da fase anterior.
- Não trate o BanjoRecomp como emulador nem troque apenas sua ROM.
- Use BK como contrato de runtime e BT como fonte dos dados do jogo.
- Corrija geradores e metadados; nunca edite código recompilado gerado.
- Faça o build falhar para símbolo, relocation, syscall ou tarefa RSP desconhecida.
- Preserve comportamento, timing e 4:3 originais antes de adicionar melhorias.
- Não copie, distribua ou versione ROM e assets proprietários.
- Não altere um hash esperado sem evidência e revisão da fonte de verdade.

## Evidência obrigatória

Para fechar uma tarefa, registre na nota da fase:

- comando reproduzível;
- exit code e resumo do resultado;
- commit dos repositórios envolvidos;
- hashes dos artefatos binários relevantes;
- teste ou reprodução que prova o critério de aceite.

Depois, marque o checkbox, atualize o `status` da fase quando aplicável e rode
`markdown_vault_audit`.

## Fontes de verdade

- Critérios finais: [[plan/definition-of-done]]
- Reutilização BK: [[specs/BK/reuse-contract]]
- Prontidão BT: [[specs/BT/recomp-readiness]]
- ROM de entrada: SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`
- Oráculo de medição `ANGRYLION_CONTROL`: [[specs/BT/angrylion-control-oracle]] (`tools/oracle/angrylion-control/run.sh`). Angrylion não substitui o RT64.

## Contrato de continuidade entre agentes

Cada tarefa técnica deve terminar revisando:

- evidence específica;
- `docs/plan/evidence/p08-session-handoff.md` quando o estado operacional mudou;
- `docs/HANDOFF.md` quando o estado global mudou;
- phase plan ativo;
- specs relacionadas;
- indexes/vault quando necessário.

Specs = contratos estáveis.
Evidence = investigação e hipóteses.
Handoff = estado corrente para novo chat.

Toda conclusão deve ser classificada:
- `PROVEN`
- `REFUTED`
- `CORRECTED/SUPERSEDED`
- `INFERENCE`
- `DISPUTED`
- `UNKNOWN`

Não promover hipótese para spec.

Quando modelos discordarem:
registrar a divergência explicitamente.

Antes de encerrar:
- `markdown_vault_audit`
- `git diff --check`
- status do worktree
