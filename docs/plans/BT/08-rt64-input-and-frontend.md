---
title: "BT P08 — RT64, entrada e frontend"
description: "Completa renderização, janela, controles e experiência de inicialização no Linux."
status: active
type: implementation-stage
project: TooieRecomp
phase: P08
order: 8
depends_on: P07
updated: 2026-08-11
tags: [banjo-tooie-Recomp, rt64, input, frontend, linux]
---

# BT P08 — RT64, entrada e frontend

## Resultado esperado

O jogo renderiza corretamente pelo RT64, aceita teclado/controle e oferece um
launcher Linux suficiente para selecionar ROM e configurações essenciais.

## Estado corrente — checkpoint INTRO → ATTRACT DEMO

P08 ACTIVE BLOCKER: a intro completa visualmente céu/cena 3D, Dolby, logo
BANJO-TOOIE e Jiggy transition; depois dela, a first gameplay demo/attract não
começa e a tela permanece preta.

O fluxo solicita `map=335`, `entrance=8`, `warp=3`. No clean freeze, uma
transition type-4 permanece ativa (count 1), progress/elapsed `≈0.9` contra
duration/target `≈3.5`, enquanto a MAIN guest continua executando o game tick.
Frame-sync e event-3 foram exonerados; scheduler bug não foi provado e
`cothemedll:2` não é o blocking PC.

Evidência principal: [[../../plan/evidence/p08-intro-to-demo-transition-diagnosis]].

Handoff para novo chat: [[../../plan/evidence/p08-session-handoff]].

## Tarefas

- [x] `P08-T01` Integrar RT64 e validar Vulkan nesta máquina (Haswell Vulkan
  incompleto no MESA, mas RT64 apresenta frames).
- [~] `P08-T02` Catalogar display lists/efeitos que exigem patch específico de
  Tooie; scheduler DP/VI corrigido, catálogo amplo pendente.
- [ ] `P08-T03` Reproduzir 4:3 e timing originais sem melhorias ativas.
- [x] `P08-T04` Integrar path SDL/RecompFrontend do BanjoRecomp.
- [x] `P08-T05` Adotar defaults/persistência do BanjoRecomp; `controls.json` é
  lazy após alteração de binding.
- [x] `P08-T06` Esconder launcher no auto-start e usar título Tooie.
- [ ] `P08-T07` Testar janela, fullscreen, foco, multi-monitor e escalas.
- [ ] `P08-T08` Implementar fallback/mensagem para driver/API incompatível.
- [~] `P08-T09` Evidência visual: intro/logo/Jiggy completas; primeira gameplay
  demo ausente e black screen persistente.
- [ ] `P08-T10` Input no title/file select; diferido até a abertura automática
  progredir.
- [~] `P08-T11` Restaurar a máquina de estados da abertura: intro restaurada;
  blocker atual é completion da type-4 no handoff para first attract demo.

## Implementado / validado

### Frontend, render e áudio

- RT64 + Vulkan, `create_render_context` e RecompFrontend integrados.
- `TOOIE_AUTO_START` chama start e esconde os contexts do launcher.
- Janela “Banjo-Tooie: Recompiled”.
- Música inicial validada no sink default do SO.
- Path host de input/rumble equivalente ao BanjoRecomp; title/file-select ainda
  não validado.

### Blockers históricos fechados

- data relocation de overlays / `LOOKUP_FUNC(0)`;
- actor manager init e false lead `func800D6E54`;
- resident overlay stale / gcsection-glintrosync;
- `No threads left to run!` / queue traversal do runtime;
- IPL3 integrity gate e natural actor creation;
- R_MIPS_26 same-section collision / `do_dma` SIGSEGV.

Não reabrir esses tópicos sem nova evidência. O resumo e os links fonte estão em
[[../../plan/evidence/p08-intro-to-demo-transition-diagnosis#Resolved blockers / do not reopen]].

## Evidências P08

| evidência | estado/finalidade |
| --- | --- |
| [[../../plan/evidence/p08-intro-to-demo-transition-diagnosis]] | **CURRENT:** timeline visual, warp/type-4, discriminators e clean-freeze post-mortem |
| [[../../plan/evidence/p08-session-handoff]] | **CURRENT:** handoff autossuficiente para novo chat |
| [[../../plan/evidence/p08-visual-validation/README]] | leitura humana dos frames: intro/Jiggy completas, demo ausente |
| [[../../plan/evidence/p08-overlay-rmips26-binding-diagnosis]] | **RESOLVED:** same-section JAL / flat func_map |
| [[../../plan/evidence/p08-do-dma-segv-diagnosis]] | histórico da cadeia OOB/bad DMA fechada |
| [[../../plan/evidence/p08-natural-actor-spawn-divergence]] | **RESOLVED:** IPL3 gate e actor creation natural |
| [[../../plan/evidence/p08-no-threads-left-diagnosis]] | **RESOLVED:** scheduler fix + harness 13/13 |
| [[../../plan/evidence/p08-overlay-data-relocation-diagnosis]] | **RESOLVED:** metadata/readelf `-W`, callback correto |
| [[../../plan/evidence/p08-gcsection-glintrosync-segv-diagnosis]] | **RESOLVED:** resident overlay invalidation |
| [[../../plan/evidence/p08-active-banjo-reuse-audit]] | limites de reuso e camada correta |
| [[../../plan/evidence/p08-controller-host-path]] | path host de controles |
| [[../../plan/evidence/p08-title-input-eeprom]] | **DEFERRED:** tentativa EEPROM falhou validação humana |
| [[../../plan/evidence/p08-runtime-frontend-smoke]] | frontend/preview/runtime smoke histórico |
| [[../../plan/evidence/p08-first-area-playable]] | captura histórica anterior, superseded pelo checkpoint visual atual |

## Fluxo esperado corrente

```text
intro mode 3
 -> Jiggy transition
 -> gsattract ep5
 -> func_800A9BA0
 -> map 335 / entrance 8
 -> func_800A7A90 -> type-4
 -> func_800A7B18(3) -> warp pending
 -> func_800A7D84 tick
 -> func_800C9510()==0
 -> func_800A9924 -> func_800A76F4(5)
 -> mode 5 -> func_800A9A14 -> gsattract ep1
 -> gameplay demo
```

## Estado observado corrente

```text
intro/Jiggy completa visualmente
 -> first demo request correto
 -> type-4 count 1 permanece incompleta
 -> MAIN continua dentro do game tick
 -> warp 3 permanece gated
 -> mode 5/demo não começa
```

CORRECTED: o transition count é `1`, lido no físico correspondente a
aproximadamente `0x8012AB57`. A leitura histórica `TRN_CNT=0` usou offset
errado.

## Ground truth e timing warning

Reprodução natural:

```sh
TOOIE_AUTO_START=1 \
TOOIE_DIAG=1 \
TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
./build/TooieRecompiled
```

O freeze natural requer binário original sem debugger/instrumentação durante o
run. GDB, breakpoints e counters mudaram timing e permitiram completion da
type-4. Para post-mortem: run clean, confirmar freeze, anexar somente depois e
capturar snapshot/core. Nunca versionar o core de aproximadamente 6.4 GB.

## Residual prioritário

1. Investigar por que a type-4 ativa permanece incompleta no clean freeze
   enquanto MAIN executa o tick.
2. Validar semântica de progress `≈0.9`, duration `≈3.5` e raw field `≈2883`.
3. Identificar timing source/delta de `gctransitionDll ep9` e gates de
   state/subactor completion.
4. Comparar com original/emulador antes de instrumentação timing-sensitive.
5. Depois, retomar input title/file-select, rumble/hotplug e polish
   4:3/fullscreen.

BaKey (`func_800BCF28`) é possible downstream blocker de runs perturbados, não
root cause do clean freeze.

## Gate de saída

- [~] Abertura progride por logos e Jiggy; first attract demo ainda não começa.
- [~] RT64 renderiza intro 3D; gameplay demo/personagem/HUD não comprovados.
- [ ] Transition type-4 completa naturalmente e warp 3 inicia mode 5/gsattract
  ep1.
- [ ] Entrada no title/file select e rumble/hotplug.
- [x] Persistência segue BanjoRecomp; `controls.json` lazy.
- [x] Sem melhorias que alterem a referência de timing/paridade.

## Próxima etapa

NEXT SINGLE ACTION: investigar a semântica interna de progress/completion da
type-4 no clean state, sem implementar fix nesta fase documental. Só depois de
resolver o handoff INTRO → ATTRACT retomar input e avançar para
[[09-gameplay-parity-and-enhancements]].
