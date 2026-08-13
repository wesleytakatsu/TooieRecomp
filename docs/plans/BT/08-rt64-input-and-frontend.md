---
title: "BT P08 — RT64, entrada e frontend"
description: "Completa renderização, janela, controles e experiência de inicialização no Linux."
status: active
type: implementation-stage
project: TooieRecomp
phase: P08
order: 8
depends_on: P07
updated: 2026-08-13
tags: [banjo-tooie-Recomp, rt64, input, frontend, linux]
---

# BT P08 — RT64, entrada e frontend

## Resultado esperado

O jogo renderiza corretamente pelo RT64, aceita teclado/controle e oferece um
launcher Linux suficiente para selecionar ROM e configurações essenciais.

## Estado corrente

**P08 ACTIVE BLOCKER (P08-T11):** intro/Jiggy/type-4/warp OK; first attract
demo ausente. O freeze de MAIN em `func_800C2AB8` (slot 7, `table[2].word0` live
pool object) foi **causalmente provado** como gap de preempção do runtime e
superado em experimento limpo; agora um **SIGSEGV downstream** em
`func_800BCF28` (attract/world input init) bloqueia o demo.

Reconstrução do Scheduler/Driver e Preempção: A cadeia completa de despacho
indireto de áudio e o gap estrutural de preempção foram provados:
- Hardware N64: Hardware interrupt preempta `OSThread 6` MAIN (Pri 20) imediatamente para `OSThread 4` AUDIO (Pri 50).
- `N64ModernRuntime`: Agendamento puramente cooperativo; o busy-wait puro em `func_800C2AB8` retém o token de execução e impede que mensagens externas acordem a thread AUDIO.
- **Causa validada (experimento limpo 2026-08-12):** strong override temporário
  de `func_800C2AB8` com `yield_self_1ms(rdram)` → AUDIO roda, `func_800205D0`
  limpa `table[2].word0`, `func_800C269C(7)==1`, `func_800C16A4(7)` dispensa o
  slot 7, `func_800C2AB8` retorna, attract progride até SIGSEGV em
  `func_800BCF28`. Patch revertido.

Evidência corrente:

- [[../../plan/evidence/p08-preemption-patch2-validation]] (**CURRENT** — PATCH 2 landed; T01–T10/T12 PASS; flag OFF)
- [[../../plan/evidence/p08-preemption-patch1-independent-review]] (PATCH 1 review B)
- [[../../plan/evidence/p08-preemption-patch1-validation]] (PATCH 1 implementer evidence)
- [[../../plan/evidence/p08-preemption-implementation-plan]] (spec F'; PATCH 2 landed, PATCH 3+ pending)
- [[../../plan/evidence/p08-preemption-design-adversarial-review]] (arquitetura F' aprovada)
- [[../../plan/evidence/p08-permanent-preemption-design]] (DeepSeek Hybrid F, `CORRECTED/SUPERSEDED`)
- [[../../plan/evidence/p08-guest-preemption-causal-validation]]
- [[../../plan/evidence/p08-guest-preemption-runtime-audit]]
- [[../../plan/evidence/p08-slot7-scheduled-event-validation]]
- [[../../plan/evidence/p08-slot7-scheduled-event-driver]]
- [[../../plan/evidence/p08-slot7-object-lifecycle-arbitration]]
- [[../../plan/evidence/p08-session-handoff]]
- [[../../plan/evidence/p08-attract-actor-loop-diagnosis]]
- [[../../plan/evidence/p08-type4-transition-semantics-audit]]

> **VALIDAÇÃO DINÂMICA CLEAN FREEZE (2026-08-12):** 3 clean freezes
> determinísticos confirmam **CLASS A**: o objeto de `table[2]` está agendado na
> delta-queue do SoundManager (head, delta 0, payload.p4 == table[2].word0),
> handler `mgr+0x08 = func_8001ED68` (MATCH), `func_8001EE34` pending; a thread
> `[Game] AUDIO` está estacionada em `LightweightSemaphore::waitWithPartialSpinning`
> e os contadores de áudio ficam congelados no freeze → o dispatch não pode rodar
> enquanto MAIN gira em `func_800C2AB8`. Ver
> [[../../plan/evidence/p08-slot7-scheduled-event-validation]].


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
- [~] `P08-T11` Máquina de estados da abertura: type-4+warp OK; **freeze C2AB8
  = gap de preempção do runtime, provado causalmente**. Hybrid F' PATCH 1
  PATCH 2 landed (runtime service, flag OFF). PATCH 3+ pending. Attract
  ainda bloqueado até safepoints + `func_800BCF28`.


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
| [[../../plan/evidence/p08-preemption-patch2-validation]] | **CURRENT:** PATCH 2 runtime service |
| [[../../plan/evidence/p08-preemption-patch1-independent-review]] | PATCH 1 independent review B |
| [[../../plan/evidence/p08-preemption-patch1-validation]] | PATCH 1 scaffold + T01 PASS |
| [[../../plan/evidence/p08-preemption-implementation-plan]] | spec file/function + T01–T15; PATCH 2 landed |
| [[../../plan/evidence/p08-preemption-design-adversarial-review]] | arquitetura F' aprovada (veredito C) |
| [[../../plan/evidence/p08-permanent-preemption-design]] | DeepSeek Hybrid F; `CORRECTED/SUPERSEDED` |
| [[../../plan/evidence/p08-guest-preemption-causal-validation]] | experimento limpo class A — gap de preempção provado causalmente |
| [[../../plan/evidence/p08-guest-preemption-runtime-audit]] | auditoria estrutural do gap (Gemini) |
| [[../../plan/evidence/p08-slot7-scheduled-event-validation]] | validação dinâmica class A (evento agendado+due, dispatch AUDIO estacionado) |
| [[../../plan/evidence/p08-slot7-scheduled-event-driver]] | reconstrução estática do scheduler/driver |
| [[../../plan/evidence/p08-slot7-object-lifecycle-arbitration]] | arbitragem Gemini×DeepSeek; struct/driver/order |
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

1. **Review independente de PATCH 2**, depois PATCH 3 —
   [[../../plan/evidence/p08-preemption-implementation-plan]],
   [[../../plan/evidence/p08-preemption-patch2-validation]].
   Arquitetura:
   [[../../plan/evidence/p08-preemption-design-adversarial-review]].
2. **Novo SIGSEGV downstream em `func_800BCF28`** — surge após C2AB8 retornar
   (chain gsworldDll/basetup/input/bakey). Fora do escopo até o fix permanente.
3. Origem do objeto `table[2]` (intro/attract/handoff) — permanece UNKNOWN.
4. Então: gsattract ep1 / demo, depois input title/file-select.

## Gate de saída

- [~] Abertura até Jiggy/type-4/warp; demo ainda ausente (SIGSEGV downstream func_800BCF28).
- [~] RT64 intro OK; gameplay demo não comprovada.
- [ ] mode 5 / gsattract ep1 / gameplay demo renderizam.
- [ ] Entrada title/file select e rumble/hotplug.
- [x] Persistência BanjoRecomp; `controls.json` lazy.
- [x] Sem melhorias que alterem timing/paridade de referência.

## Próxima etapa

**NEXT:** review independente de PATCH 2, depois PATCH 3 (generator
safepoints). Review: [[../../plan/evidence/p08-preemption-patch2-validation]].
Não PATCH 4. Depois: SIGSEGV `func_800BCF28`. Após demo: input e
[[09-gameplay-parity-and-enhancements]].
