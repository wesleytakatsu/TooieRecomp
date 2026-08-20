---
title: "BT P09 — Paridade de gameplay e melhorias"
description: "Fecha campanha, saves e timing antes de habilitar widescreen, FPS alto e extensões."
status: pending
type: implementation-stage
project: TooieRecomp
phase: P09
order: 9
depends_on: P08
updated: 2026-08-20
tags: [banjo-tooie-Recomp, parity, gameplay, framerate, widescreen]
---

# BT P09 — Paridade de gameplay e melhorias

## Resultado esperado

Comportamento funcional equivalente ao original em timing padrão, seguido por
melhorias opcionais que não alteram gameplay.

## Estado corrente (P09 Pendente — evidência pré-abertura, 2026-08-20)

- **P08 Formal**: `ACTIVE / MANUAL_VALIDATION_PENDING`; a observação positiva dos controles durante gameplay não prova o checklist físico completo, e não há confirmação física de display.
- **P09 Formal**: `PENDING / NOT_FORMALLY_OPEN`.
- **Evidência provisória**: [[../../plan/evidence/p09-user-observed-parity-residuals]] cobre apenas Demo 0 (Wooded Hollow) contra o oracle hardware-faithful `ANGRYLION_CONTROL` (Mupen64Plus Pure Interpreter + Angrylion RDP Plus + CXD4 LLE).
- **Demo 0**: pad 765/765, trajetória limitada, paridade de behavior-state, Blue Egg no evento glidx 322 e resultado de Z-buffer/classifier são preservados. Atributos diretos de animação e render-state completo permanecem não medidos/parciais.
- **Relatos globais**: Fire Egg permanece `USER_REPORTED_UNLOCATED`; drift visual permanece `USER_REPORTED / OPEN`. Demo 1, Demo 2 e Demo 3 estão `UNTESTED`.
- **Backlog de Resíduos de Fidelidade**: `P09-RES-03` glitches gráficos (`USER_REPORTED / UNCATALOGUED`), `P09-RES-04` tic/tic de áudio (`USER_REPORTED / UNDIAGNOSED`) e `P09-RES-05` culling de borda widescreen (`USER_REPORTED`).

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

1. Completar a validação física formal de P08.
2. Depois de abrir P09 formalmente, alinhar Demo 1, Demo 2 e Demo 3 contra `ANGRYLION_CONTROL`, identificar eventos de ovos/projéteis, medir animação/render-state direto e localizar o drift visual.
3. Só então selecionar o próximo defeito entre áudio e gráficos.
