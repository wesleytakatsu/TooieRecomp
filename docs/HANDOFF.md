---
title: "Handoff — TooieRecomp"
description: "Estado global corrente do port, com prioridade P08 no handoff intro para attract demo."
status: current
type: handoff
project: TooieRecomp
updated: 2026-08-11
tags: [tooie-recomp, handoff, p08, intro, attract, type-4]
---

# Handoff — TooieRecomp

## Prioridade atual

**P08 ACTIVE BLOCKER:** a primeira attract/gameplay demo não começa depois da
intro e Jiggy transition visualmente corretas.

Estado clean-freeze corrente:

- request `map=335 / entrance=8 / warp=3` confirmado;
- transition type-4 ativa, count `1`;
- MAIN guest viva e executando dentro do game tick;
- type-4 incompleta: progress/elapsed `≈0.9`, duration/target `≈3.5`;
- frame-sync/VI exonerado;
- event-3 graphics barrier exonerado;
- scheduler bug não provado;
- `cothemedll:2` não é blocker.

Descrição canônica:

> The opening/intro sequence completes visually through the Jiggy transition,
> but the subsequent gameplay demo/attract scene does not start.

Handoff técnico autossuficiente:
[[plan/evidence/p08-session-handoff]]. Evidência consolidada:
[[plan/evidence/p08-intro-to-demo-transition-diagnosis]].

## NEXT

Investigar a semântica interna de progress/completion da transition type-4 no
estado clean. Começar por `≈0.9 / ≈3.5`, pela discrepância do raw field `≈2883`
e pelo delta/timing source de `gctransitionDll ep9`. Preferir análise
estática/comparação com original ou emulador antes de instrumentação que altere
timing.

Não diagnosticar BaKey ainda; ele é apenas possible downstream blocker visto em
runs perturbados.

## Estado do produto validado

1. O launcher/RecompFrontend, auto-start e RT64/Vulkan apresentam a janela do
   jogo; input do launcher funciona.
2. A música inicial toca corretamente no sink default do SO.
3. A intro renderiza céu/brilho, cena 3D, Dolby Surround e logo BANJO-TOOIE;
   cenário/câmera continuam avançando.
4. A Jiggy transition fecha a intro corretamente.
5. Depois da Jiggy, a tela fica preta e a primeira gameplay demo não aparece.
6. Title/file-select input continua diferido; a tentativa EEPROM anterior
   falhou validação humana.

A captura isolada `0060` não prova o instante exato do fim da Jiggy; a conclusão
vem da validação humana do fluxo mais os estados observados. Ver
[[plan/evidence/p08-visual-validation/README]].

## Arquitetura e regras

TooieRecomp recompila estaticamente Banjo-Tooie com:

- `N64Recomp` / `RSPRecomp`;
- `N64ModernRuntime`;
- `RT64`;
- `RecompFrontend`.

`RecompiledFuncs` é código gerado e não deve ser editado. Fixes devem estar na
camada proprietária da causa — generator, metadata, runtime ou integração
source/host — e precisam ser versionados/reproduzíveis.

Repo Git: `TooieRecomp/`, branch `main`. A raiz do workspace
`/home/wesley/projetos/N64-Recomp` não é um repositório Git.

## Resolved blockers / do not reopen

### Overlay data relocation / `LOOKUP_FUNC(0)`

Generator de metadata chamava `readelf` sem `-W`, truncava nomes longos de
seções relocatable e descartava data relocations. Corrigido; `ActorInfo[0x38]`
agora contém callback correto. Ver
[[plan/evidence/p08-overlay-data-relocation-diagnosis]].

### Actor manager e D6E54

O manager inicializa naturalmente por `chbaddieDll ep1 -> func800FF390 ->
func801065E0 -> vector_new`; `D80136EE0≈0x801DF080`. O nulo veio de spawn
artificial. `func800D6E54` é resource/reclaim diferido, não actor population.
Ver [[plan/evidence/p08-natural-actor-spawn-divergence]].

### Resident overlay cache / gcsection

`ovl_unload` deixava mapping stale em `s_resident_overlays` depois de liberar
heap. A invalidação pós-unload resolveu gcsection/glintrosync. Ver
[[plan/evidence/p08-gcsection-glintrosync-segv-diagnosis]].

### Scheduler antigo

N64ModernRuntime recebeu scan real de duplicata em insert, traversal correto em
remove e guard de reinserção em `osSetThreadPri`. Harness 13/13 PASS e múltiplos
runs sem `No threads left`, self-loop ou corrupção. Ver
[[plan/evidence/p08-no-threads-left-diagnosis]].

### IPL3 integrity gate

Corrigidos sign extension/endereço e seed one-shot dos resíduos esperados
`0xAD090010`/`0xAD170014`; natural actor creation restaurada em 3/3 runs. Ver
[[plan/evidence/p08-natural-actor-spawn-divergence]].

### R_MIPS_26 / flat func_map / do_dma

Same-section JAL conhecido em section relocatable agora vira direct C call no
generator. Fechado o path `chintrochar -> gsworld ep0 espúrio -> idx -0x3AB ->
bad DMA -> do_dma SIGSEGV`. Patch versionado:
`patches/runtime/n64recomp-same-section-jal.patch`. Ver
[[plan/evidence/p08-overlay-rmips26-binding-diagnosis]] e
[[plan/evidence/p08-do-dma-segv-diagnosis]].

## Current expected flow

```text
intro mode 3
 -> Jiggy transition
 -> gsattract ep5
 -> map 335, entrance 8
 -> create/retain type-4
 -> warp 3 pending
 -> func_800A7D84 tick
 -> type-4 completes / func_800C9510 returns 0
 -> func_800A9924 -> func_800A76F4(5)
 -> mode 5 -> func_800A9A14 -> gsattract ep1
 -> gameplay demo
```

## Current clean-freeze evidence

Ground truth usa binário original, sem debugger ou instrumentação durante a
execução:

```sh
TOOIE_AUTO_START=1 \
TOOIE_DIAG=1 \
TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
./build/TooieRecompiled
```

Freeze típico: black screen pós-intro, syscall `d≈179275` estável por `>=5s`,
`last_ovl=635:2`, mode 3, map 335, entrance 8, warp 3, transition count 1.
Somente então anexar GDB/capturar snapshot. Não versionar o core de ~6.4 GB.

MAIN snapshot:

```text
OSThread 0x80045788, id 6, priority 20, queue 0

func_800124EC
 -> func_800123F4
 -> func_800123BC
 -> func_800A7D84
 -> func_800A76F4
 -> func_800A7380
 -> func_800C2AB8
 -> get_function(0x800C269C)
```

MAIN não estava bloqueada em MQ. A running queue continha IDLE porque MAIN era
current/running. Self-loop, duplicata e invalid pointer: NO. Scheduler bug:
NOT PROVEN.

Type-4 naquele run: objeto heap específico aproximadamente `0x8027AAC0`, count
1, progress `≈0.9`, duration `≈3.5`, raw field `≈2883`. Não chamar 2883 de tick
counter real: `FIELD_SEMANTICS_NOT_FULLY_VALIDATED`.

## Exonerated current hypotheses

- **Frame-sync:** recv 57/57, send 57/57, host VI/producer 361/361,
  outstanding 0.
- **Event-3:** call/handler 3/3, ACK send/recv 3/3, outstanding 0, msg5
  dispatch 57.
- **cothemedll:** callback síncrono sem MQ/DMA wait;
  `LAST_OVL_IS_BLOCKING_PC=NO`.
- **Circular type-4 gate:** update continua no restante de `func_800A7D84`
  mesmo quando warp permanece gated.
- **Warp logic:** em run perturbado, C9510 fica nonzero 18 vezes e zera na 19ª;
  `func_800A9924` e `func_800A76F4(5)` executam uma vez.

## Timing warning

GDB, breakpoints e instrumentação em hot paths alteraram timing e permitiram que
a type-4 terminasse. Dados perturbados servem para provar contratos locais, não
para representar o freeze natural.

```text
run clean -> confirm freeze -> attach after freeze -> capture snapshot/core
```

## Não provado / não concluir

- motivo de progress ficar em `≈0.9`;
- delta-time/timing source incorreto;
- state machine interna ou subactor preso;
- timing-source flag errada;
- relevância causal de `func_800C2AB8 / get_function(0x800C269C)`;
- callback incorreto;
- semântica do raw field `≈2883`;
- BaKey como root cause do clean freeze.

## Files to read

1. [[plan/evidence/p08-session-handoff]]
2. [[plan/evidence/p08-intro-to-demo-transition-diagnosis]]
3. [[plan/evidence/p08-visual-validation/README]]
4. [[plans/BT/08-rt64-input-and-frontend]]
5. Evidências resolvidas ligadas na seção “do not reopen”.

## Scope do checkpoint documental

```text
SOURCE_CODE_CHANGED    = NO
RUNTIME_CODE_CHANGED   = NO
GENERATOR_CODE_CHANGED = NO
PATCH_CODE_CHANGED     = NO
CORE_INCLUDED          = NO
BUILD_ARTIFACT_INCLUDED= NO
```
