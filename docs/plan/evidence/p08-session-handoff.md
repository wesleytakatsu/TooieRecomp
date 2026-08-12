---
title: "P08 — session handoff: intro para attract demo"
description: "Estado autossuficiente do blocker pós-Jiggy e ponto exato para a próxima investigação."
status: evidence
phase: P08
updated: 2026-08-11
tags: [tooie-recomp, p08, handoff, intro, attract, type-4, clean-freeze]
---

# P08 — session handoff: INTRO → ATTRACT DEMO

## CURRENT HEAD

- Branch: `main`.
- Checkpoint base/parent obrigatório: `cf3116696e290069372a85764929c69284135d0e`.
- O checkpoint documental é o commit que contém este arquivo; confirme o hash
  publicado com `git rev-parse HEAD` em vez de copiar um hash autorreferente.

## CURRENT PHASE

P08 — RT64, input e frontend. A abertura automática é a prioridade ativa;
title/file-select input continua deliberadamente diferido.

## CURRENT PRODUCT STATE

- TooieRecomp usa `N64Recomp`, `N64ModernRuntime`, `RT64` e `RecompFrontend`.
- Música inicial e renderização da intro estão validadas.
- A abertura completa visualmente céu/cena 3D, Dolby, logo BANJO-TOOIE,
  progressão de câmera e o fechamento pela Jiggy transition.
- Depois da Jiggy, a primeira gameplay demo/attract deveria aparecer, mas a tela
  permanece preta.
- `RecompiledFuncs` é gerado e não deve ser editado manualmente.

Descrição canônica:

> The opening/intro sequence completes visually through the Jiggy transition,
> but the subsequent gameplay demo/attract scene does not start.

## CURRENT BLOCKER

A first attract/demo não começa. O fluxo solicitou `map=335 / 0x14F /
MAP_14F_IOH_WOODED_HOLLOW`, `entrance=8`, `warp=3`, mas uma transition type-4
permanece ativa e incompleta no freeze natural.

## CURRENT FIRST DIVERGENCE

MAIN continua executando o game tick, mas a type-4 não alcança
completion/progresso suficiente no run clean. No snapshot, progress/elapsed era
aproximadamente `0.9`, contra duration/target aproximadamente `3.5`.

## CURRENT NEXT SINGLE ACTION

Investigar por que a type-4 ativa no clean freeze permanece incompleta enquanto
MAIN executa o tick. Prioridade: validar a semântica dos campos, o delta/timing
que alimenta `gctransitionDll ep9`, gates de state/subactor e a discrepância do
raw field `≈2883`. Preferir comparação estática/original/emulador antes de criar
instrumentação timing-sensitive. Não investigar BaKey ainda.

## RESOLVED — DO NOT REOPEN

1. **Overlay data relocation / `LOOKUP_FUNC(0)`:** `readelf` sem `-W` truncava
   nomes relocatable; generator/metadata corrigidos; `ActorInfo[0x38]` válido.
2. **Actor manager:** o nulo veio de spawn artificial. No fluxo natural,
   `D80136EE0≈0x801DF080` após `chbaddieDll ep1 -> func800FF390 ->
   func801065E0 -> vector_new`.
3. **`func800D6E54`:** resource/reclaim diferido, não population pass.
4. **Resident overlay/gcsection:** stale `s_resident_overlays` após heap reuse
   corrigido por invalidação no unload.
5. **Scheduler antigo:** queue insert/remove/`osSetThreadPri` corrigidos no
   N64ModernRuntime; harness 13/13 PASS; sem `No threads left`, self-loop ou
   corrupção nas validações posteriores.
6. **IPL3 integrity gate:** sign extension + seed one-shot dos resíduos
   `0x802FB1F4=0xAD090010` e `0x802FE1C0=0xAD170014`; natural actor creation
   restaurada em 3/3 runs.
7. **R_MIPS_26 / flat func_map / do_dma:** same-section JAL em relocatable agora
   é direct C call. O path `chintrochar -> gsworld ep0 -> idx -0x3AB -> bad DMA`
   está fechado; 3/3 smokes sem regressões.

Não chamar o blocker atual de scheduler bug, frame-sync starvation, event-3
barrier, cothemedll hang, actor manager ou do_dma sem nova prova específica.

## CURRENT EXPECTED FLOW

```text
intro mode 3
 -> gctransitionDll / subaddiefade
 -> gsattract overlay 731 ep5
 -> func_800A9BA0
 -> func_800A79D4(map=335, entrance=8)
 -> func_800A7A90 -> retain/create transition type-4
 -> func_800A7B18(3) -> warp=3 pending
 -> func_800A7D84 tick
 -> func_800C9510()==0
 -> func_800A9924
 -> func_800A76F4(5)
 -> next tick mode 5
 -> func_800A9A14 -> gsattract ep1
 -> gameplay demo
```

## CURRENT OBSERVED FLOW

```text
intro visually completes through Jiggy
 -> gsattract selects first demo
 -> map=335, entrance=8, warp=3 requested
 -> transition count=1, type=4 active
 -> clean run freezes black
 -> MAIN remains inside game tick
 -> type-4 incomplete (~0.9 / ~3.5)
 -> warp remains gated
 -> mode 5 / gameplay demo not reached
```

Em runs perturbados, `func_800C9510` retornou nonzero 18 vezes e zero na 19ª;
`func_800A9924=1` e `func_800A76F4(5)=1`. Isso prova que warp consumption
funciona quando a transition termina, não que o clean freeze progride.

## CLEAN-FREEZE SNAPSHOT

### Reprodução ground truth

```sh
TOOIE_AUTO_START=1 \
TOOIE_DIAG=1 \
TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
./build/TooieRecompiled
```

- Binário original.
- Sem debugger, breakpoints, hooks ou counters durante a execução.
- Black screen pós-intro; syscall `d≈179275` estável por `>=5s`;
  `last_ovl=635:2`.
- Estado típico na fronteira: mode 3, map 335, entrance 8, warp 3,
  transition count 1.
- GDB attach/gcore somente depois do freeze.
- Core: aproximadamente 6.4 GB, 48 LWPs; não versionar.

### MAIN

```text
OSThread = 0x80045788
id       = 6
priority = 20
queue    = 0
```

Backtrace capturado:

```text
func_800124EC
 -> func_800123F4
 -> func_800123BC
 -> func_800A7D84
 -> func_800A76F4
 -> func_800A7380
 -> func_800C2AB8
 -> get_function(0x800C269C)
```

MAIN está viva/current e executando guest code no tick, não bloqueada em MQ.

### Threads/runqueue/host VI

- VIMGR, SCHEDULE, JOY/MOTOR/CIC e outros estavam bloqueados em MQs vazias.
- Running queue continha IDLE; MAIN era current/running, já retirada da fila.
- Self-loop: NO; duplicate: NO; invalid pointer: NO.
- Host VI thread viva, em pacing de aproximadamente 60 Hz.
- Cerca de sete `external_messages` retidas: consequência/correlação, não causa
  primária provada.
- `SCHEDULER_BUG_PROVEN=NO`.

### Transition type-4

- Count: `1`.
- Objeto naquele run: aproximadamente `0x8027AAC0`; endereço heap-specific.
- Progress/elapsed: `≈0.9`.
- Duration/target: `≈3.5`.
- Raw field: `≈2883`.
- `FIELD_SEMANTICS_NOT_FULLY_VALIDATED=YES`.

CORRECTED: `TRN_CNT=0` veio do endereço errado. O byte efetivo é o físico
correspondente a aproximadamente `0x8012AB57` e vale `1`.

Não chamar `2883` de “ticks reais”: isso conflita com os ~18 updates do run
perturbado e ainda precisa de validação semântica.

## EXONERATED HYPOTHESES

### Frame-sync / VI

Queue `0x80078FD0`, capacity 10, `osViSetEvent(..., msg=0, retrace=1)`.

```text
RECV_ENTER=57 RECV_RETURN=57 OUTSTANDING=0
SEND_ATTEMPT=57 SEND_SUCCESS=57
PRODUCER_EVENT=361 HOST_VI=361
```

REFUTED: MAIN congela por falta de frame-sync delivery.

### Event-3 / graphics flush

Request queue `0x800783D0` capacity 20; reply queue `0x80078438` capacity 10;
deferred completion por msg5 / `func_8001443C`.

```text
EVENT3_CALL=3 EVENT3_HANDLER=3
ACK_SEND=3/3 success
ACK_RECV_ENTER=3 ACK_RECV_RETURN=3 OUTSTANDING=0
MSG5_DISPATCH=57
```

REFUTED: `func_80013D10(event=3)` é blocker.

### Type-4 circular gate

REFUTED: mesmo com `func_800C9510()!=0`, `func_800A7D84` continua o tick e
executa o update da transition via `func_800C92A8`.

### cothemedll

`cothemedll_entrypoint_2` retorna sincronicamente e não possui MQ/DMA wait.
`LAST_OVL_IS_BLOCKING_PC=NO`.

## TIMING / OBSERVATION WARNING

GDB, breakpoints e instrumentação em hot paths mudam o timing. Nesses runs, a
type-4 termina, warp é consumido e mode avança. Use os dados perturbados apenas
para contratos locais. Para o estado natural:

```text
run clean -> wait freeze -> attach only after freeze -> capture snapshot/core
```

## OPEN QUESTIONS

- Por que progress/elapsed fica em aproximadamente 0.9?
- Qual delta/timing source alimenta `gctransitionDll ep9`?
- A state machine interna ou um subactor visual bloqueia completion?
- Alguma timing-source flag está errada?
- `func_800C2AB8 / get_function(0x800C269C)` é causal ou apenas o PC amostrado?
- Algum callback esperado é despachado incorretamente?
- Qual é a semântica real do raw field `≈2883`?

BaKey (`func_800BCF28`) é POSSIBLE DOWNSTREAM BLOCKER visto só depois que runs
perturbados avançam. Não é root cause provada do clean freeze.

## FILES TO READ NEXT

1. [[p08-intro-to-demo-transition-diagnosis]] — evidência principal e timeline.
2. [[p08-visual-validation/README]] — interpretação humana dos frames.
3. [[p08-overlay-rmips26-binding-diagnosis]] — último fix antes do blocker.
4. [[p08-do-dma-segv-diagnosis]] — cadeia histórica fechada.
5. [[p08-natural-actor-spawn-divergence]] — IPL3/actors.
6. [[p08-no-threads-left-diagnosis]] — scheduler fechado.
7. [[p08-active-banjo-reuse-audit]] — limites de reuso/camadas.
8. [[p08-title-input-eeprom]] — input diferido.
9. `docs/HANDOFF.md` e `docs/plans/BT/08-rt64-input-and-frontend.md`.

## CHECKPOINT SCOPE

```text
CODE_CHANGED             = NO
RUNTIME_CODE_CHANGED     = NO
GENERATOR_CODE_CHANGED   = NO
PATCH_CODE_CHANGED       = NO
CORE_INCLUDED            = NO
BUILD_ARTIFACT_INCLUDED  = NO
NEXT_INVESTIGATION_RUN   = NO
```
