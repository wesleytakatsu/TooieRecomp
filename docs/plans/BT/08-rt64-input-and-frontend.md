---
title: "BT P08 — RT64, entrada e frontend"
description: "Completa renderização, janela, controles e experiência de inicialização no Linux."
status: active
type: implementation-stage
project: TooieRecomp
phase: P08
order: 8
depends_on: P07
updated: 2026-08-12
tags: [banjo-tooie-Recomp, rt64, input, frontend, linux]
---

# BT P08 — RT64, entrada e frontend

## Resultado esperado

O jogo renderiza corretamente pelo RT64, aceita teclado/controle e oferece um
launcher Linux suficiente para selecionar ROM e configurações essenciais.

## Estado corrente

**P08 ACTIVE BLOCKER (P08-T11):** intro/Jiggy/type-4/warp OK; first attract
demo ausente. MAIN busy-wait em `func_800C2AB8` no slot 7 porque
`table[2].word0` aponta para um pool object vivo nunca limpo por
`func_800205D0`.

Reconstrução do Scheduler/Driver: Cadeia completa de despacho indireto provada
(`func_8001EB10 → func_800128C0 → func_80021574 → func_8001ED68 → func_8001EE34 → func_800200B8 → func_800205D0`).
O clearer roda no contexto da thread de áudio / síntese, a qual é inibida de
progredir pelo spin síncrono da thread MAIN no host.

Evidência corrente:

- [[../../plan/evidence/p08-slot7-scheduled-event-driver]] (**CURRENT**)
- [[../../plan/evidence/p08-slot7-object-lifecycle-arbitration]]
- [[../../plan/evidence/p08-session-handoff]]
- [[../../plan/evidence/p08-attract-actor-loop-diagnosis]]
- [[../../plan/evidence/p08-type4-transition-semantics-audit]]

## Tarefas

- [x] `P08-T01` Integrar RT64 e validar Vulkan nesta máquina.
- [~] `P08-T02` Catalogar display lists/efeitos Tooie; scheduler DP/VI OK.
- [ ] `P08-T03` Reproduzir 4:3 e timing originais sem melhorias ativas.
- [x] `P08-T04` Integrar path SDL/RecompFrontend do BanjoRecomp.
- [x] `P08-T05` Defaults/persistência BanjoRecomp; `controls.json` lazy.
- [x] `P08-T06` Esconder launcher no auto-start e título Tooie.
- [ ] `P08-T07` Janela, fullscreen, foco, multi-monitor e escalas.
- [ ] `P08-T08` Fallback/mensagem para driver/API incompatível.
- [~] `P08-T09` Evidência visual: intro/Jiggy OK; first demo ausente.
- [ ] `P08-T10` Input title/file select (diferido).
- [~] `P08-T11` Máquina de estados da abertura: type-4+warp OK; **blocker =
  slot7 / table[2] audio scheduler preemption vs C2AB8 busy-wait**.


## Implementado / validado

### Frontend, render e áudio

- RT64 + Vulkan, auto-start, janela “Banjo-Tooie: Recompiled”.
- Música inicial validada.
- Input host equivalente ao BanjoRecomp; title/file-select não validado.

### Blockers históricos fechados

- data relocation overlays / `LOOKUP_FUNC(0)`;
- actor manager / D6E54 false lead;
- resident overlay / gcsection;
- scheduler queue / `No threads left` (sem nova prova);
- IPL3 integrity gate;
- R_MIPS_26 / do_dma;
- type-4 incomplete e warp não consumido (clean freeze);
- `func_80026500` como PC do hang;
- `func_800A907C → func_8001EE34` como driver do objeto (refutado).

## Evidências P08

| evidência | estado/finalidade |
| --- | --- |
| [[../../plan/evidence/p08-slot7-object-lifecycle-arbitration]] | **CURRENT:** arbitragem Gemini×DeepSeek; struct/driver/order |
| [[../../plan/evidence/p08-session-handoff]] | **CURRENT:** handoff para chat zerado |
| [[../../plan/evidence/p08-attract-actor-loop-diagnosis]] | freeze mechanism slot7 / C2AB8 |
| [[../../plan/evidence/p08-type4-transition-semantics-audit]] | type-4 complete, warp consumido |
| [[../../plan/evidence/p08-intro-to-demo-transition-diagnosis]] | timeline histórica |
| [[../../plan/evidence/p08-visual-validation/README]] | frames humanos |
| demais p08-* | resolved/deferred (ver handoff) |

## Fluxo esperado

```text
intro -> Jiggy -> gsattract -> map 335/entrance 8 -> type-4 complete
 -> warp 3 -> A9924 -> A76F4(5) -> mode 5 -> A9A14 -> gsattract ep1
 -> gameplay demo
```

## Estado observado (clean)

```text
... warp consumido, A76F4(5) ...
 -> A7380 -> C2AB8 busy-wait (slot7 / table[2] live object)
 -> black freeze; demo não começa
```

## Residual prioritário

1. **Discriminador lifecycle:** origem do objeto `table[2]` + se 1EE34/205D0
   deveriam rodar antes ou durante C2AB8 (A vs C).
2. Só depois: fechar root cause e **um** fix candidate (sem implementar ainda
   nesta fase se a causa não fechar).
3. Então: gsattract ep1 / demo, depois input title/file-select.

## Gate de saída

- [~] Abertura até Jiggy/type-4/warp; demo ainda ausente (slot7).
- [~] RT64 intro OK; gameplay demo não comprovada.
- [ ] mode 5 / gsattract ep1 / gameplay demo renderizam.
- [ ] Entrada title/file select e rumble/hotplug.
- [x] Persistência BanjoRecomp; `controls.json` lazy.
- [x] Sem melhorias que alterem timing/paridade de referência.

## Próxima etapa

**NEXT:** discriminador dinâmico mínimo do objeto `table[2]` (dump pós-freeze;
opcional contadores 20790/1EE34/205D0). Não implementar fix; não BaKey; não
reabrir scheduler. Após demo atrair: retomar input e
[[09-gameplay-parity-and-enhancements]].
