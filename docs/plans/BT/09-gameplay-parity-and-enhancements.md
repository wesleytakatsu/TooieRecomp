---
title: "BT P09 — Paridade de gameplay e melhorias"
description: "Fecha campanha, saves e timing antes de habilitar widescreen, FPS alto e extensões."
status: active
type: implementation-stage
project: TooieRecomp
phase: P09
order: 9
depends_on: P08
updated: 2026-08-20
tags: [banjo-tooie-Recomp, parity, gameplay, framerate, widescreen, active]
---

# BT P09 — Paridade de gameplay e melhorias

## Resultado esperado

Comportamento funcional equivalente ao original em timing padrão, seguido por
melhorias opcionais que não alteram gameplay.

## Estado corrente (P09 Ativo — 2026-08-20)

- **P08 Formalmente Concluído**: RT64, frontend, janelas, mensagens de erro e validação física de controle e display concluídas.
- **P09 Aberto**: Linha de base de paridade de gameplay iniciada ([[../../plan/evidence/p09-user-observed-parity-residuals]]).
- **Attract Demo 0 (Wooded Hollow)**: Paridade de 100% de estados e física contra o oracle de hardware `ANGRYLION_CONTROL` (Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE). Relatos de drift visual e disparo de Fire Egg em vez de Blue Egg foram investigados e esclarecidos (ambos disparam Blue Egg no hardware real).
- **Backlog de Resíduos de Fidelidade**: Clicks periódicos de áudio (`P09-RES-04`), glitches gráficos pontuais (`P09-RES-03`), e culling de borda em widescreen (`P09-RES-05`, associado a `P09-T08`).

## Tarefas de paridade

- [ ] `P09-T01` Criar matriz por boot, áreas, chefes, minigames, transformações, cutscenes e final.
- [ ] `P09-T02` Cobrir criação, carregamento, sobrescrita e migração de saves.
- [ ] `P09-T03` Comparar checkpoints de estado entre execução de referência e nativa.
- [ ] `P09-T04` Corrigir timers, RNG, física, animação e sincronização no timing original.
- [ ] `P09-T05` Executar campanha completa e registrar toda divergência reproduzível.
- [ ] `P09-T06` Rodar sanitizers e soak tests em trocas repetidas de área.

## Tarefas de melhoria

- [ ] `P09-T07` Desacoplar update de gameplay da apresentação para FPS arbitrário.
- [ ] `P09-T08` Adicionar widescreen/ultrawide com correções de câmera, HUD e cutscenes.
- [ ] `P09-T09` Adicionar opções de câmera/entrada sem modificar o preset fiel.
- [ ] `P09-T10` Definir API de mods somente após estabilizar layout e eventos do runtime.
- [ ] `P09-T11` Reexecutar a matriz completa para cada melhoria habilitada e desabilitada.

## Gate de saída

- Campanha completa e saves aprovados no preset fiel.
- FPS alto não muda velocidade, física, animações, timers, áudio ou cutscenes.
- Melhorias podem ser desligadas para restaurar comportamento de referência.
- Critérios funcionais e audiovisuais de [[../../plan/definition-of-done]] atendidos.

## Próxima etapa

[[10-linux-packaging-and-release]]
