---
title: "P08 — session handoff: intro para attract demo"
description: "Estado autossuficiente do blocker pós-Jiggy e ponto exato para a próxima investigação."
status: evidence
phase: P08
updated: 2026-08-12
tags: [tooie-recomp, p08, handoff, intro, attract, type-4, clean-freeze]
---

# P08 — session handoff: INTRO → ATTRACT DEMO

> **CORRECTION (2026-08-12):** o clean-freeze type-4 snapshot (8/8 runs) provou
> que a type-4 **completa** e o warp **é consumido** no run natural. A narrativa
> anterior de “type-4 incompleta (`0.9 / 3.5`) com warp gated” está
> **SUPERSEDED**. O blocker corrente é downstream, no caminho de inicialização do
> mode-5/attract (`func_800A76F4 → func_800A7380 → func_800C2AB8`). Fonte:
> [[p08-type4-transition-semantics-audit#clean-freeze-exact-type-4-snapshot]].

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
- O attract request é correto (`map=335`, `entrance=8`, `kind=2`).
- A transition type-4 **completa** e o `warp=3` **é consumido** no clean freeze.
- A transição para o mode 5 é alcançada.
- A gameplay demo ainda não renderiza: black freeze downstream durante a
  inicialização do attract.
- `RecompiledFuncs` é gerado e não deve ser editado manualmente.

Descrição canônica:

> The opening/intro sequence completes visually through the Jiggy transition and
> the attract map request, the type-4 transition completes and warp 3 is
> consumed, but the subsequent gameplay demo/attract scene still does not start
> — a black freeze occurs downstream during mode-5 / attract initialization.

## CURRENT BLOCKER

**mode-5 / attract actor-update initialization path.** A primeira attract/demo
não começa; o freeze natural é downstream da type-4/warp consumption. O clean
post-mortem (6 freezes, 3 runs A/B/C) captura a MAIN guest presa **dentro de
`func_80026500`** (inserção em lista encadeada ordenada): o walk de ponteiros
nunca termina (60/60 amostras no loop `0x5bbf40..0x5bc000`), consistente com
chain circular. O slot de kinematics `7` está preso (enabled, mode 3,
substate 2, `table[2].word0 != 0`). Caller na stack inclui overlay funcs
(`0x80801484` — colisão de símbolo bskaztorpedo/bsstonehead). Detalhe:
[[p08-attract-actor-loop-diagnosis]].

## CURRENT FIRST DIVERGENCE

MAIN é capturada **não no dispatcher `func_800C2AB8`, mas no walk de
`func_80026500`** (chamado pelo init de actor do attract/mode-5):

```text
mode-5 / attract actor init
 -> overlay func 0x80801484 (bskaztorpedo/bsstonehead)
 -> func_8002xxxx (list op) -> func_80026500 (sorted insert)
 -> walk de ponteiros nunca termina  (MAIN 60/60 no loop)
```

O slot de kinematics `7` preso (substate 2, `table[2].word0 != 0`) manteria o
dispatcher `func_800C2AB8` girando se alcançado. O callback/slot/PC exato da
lista ainda não foi nomeado (ctx guest não capturado com confiança).

## CURRENT NEXT SINGLE ACTION

Capturar o `recomp_context` vivo da MAIN (via `rbx` de `PTRACE_GETREGS`, com
validação sign-extended de r29/r31), ler os args do frame de `func_80026500`
no guest stack (arg0 = head da lista, a3 = key, curr/next) e caminhar a chain
para achar o ciclo (self-reference/back-edge) e o dono da lista. Inspeção
somente pós-freeze. Não investigar BaKey ainda.

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
8. **Type-4 como blocker (REFUTADO pelo clean snapshot):** a type-4 completa no
   run natural; `current_state=2`, `render_frame_count=2`, gate do warp ABERTO.
9. **Warp consumption (REFUTADO):** `warp=3` é consumido no clean freeze
   (`+0x12=0`), `func_800A9924(map, entr)` e `func_800A76F4(5)` executam
   (`D_80127760` retém `map=335`/`entrance=8`; mode machine `+0x1C=5`).

Não chamar o blocker atual de scheduler bug, frame-sync starvation, event-3
barrier, cothemedll hang, actor manager, do_dma, **nem type-4/warp** sem nova
prova específica.

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

## CURRENT OBSERVED FLOW (clean snapshot)

```text
intro visually completes through Jiggy
 -> gsattract selects first demo (D_80127760: map=335, entrance=8, kind=2)
 -> type-4 created, runs and COMPLETES (state 6 -> 2, render=2, gate open)
 -> warp=3 consumed (+0x12=0)
 -> func_800A9924 + func_800A76F4(5) executed (mode machine +0x1C=5)
 -> MAIN captured downstream: func_800A76F4 -> func_800A7380 -> func_800C2AB8
 -> black freeze during mode-5/attract init; gameplay demo not reached
```

Em runs perturbados, `func_800C9510` retornou nonzero 18 vezes e zero na 19ª;
isso é coerente com `effective_dt=0.05` e `target=0.9`: 18 updates × 0.05 =
0.9. `func_800A9924=1` e `func_800A76F4(5)=1` confirmam que warp consumption
funciona quando a type-4 termina.

## CLEAN-FREEZE SNAPSHOT

### Reprodução ground truth

```sh
TOOIE_AUTO_START=1 \
TOOIE_DIAG=1 \
TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
./build/TooieRecompiled
```

- Binário original.
- Sem debugger, breakpoints, hooks ou counters durante a execução.
- Freeze confirmado por `d` estável `>=6s` (`178958`..`179320`), black screen
  pós-intro, `last_ovl=635:2`. **8/8 runs idênticos e determinísticos.**
- Supervisor pai (`ptrace_scope=1`): `PTRACE_ATTACH`/dump `/proc/pid/mem`
  somente depois do freeze; decoder Python independente valida os campos.
- GDB/gcore somente depois do freeze; não versionar core de ~6.4 GB.

### MAIN

```text
OSThread = 0x80045788
id       = 6
priority = 20
queue    = 0
```

Backtrace capturado (sessão anterior; consistente com o estado do snapshot):

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

MAIN está viva/current e executando guest code, não bloqueada em MQ. `d`
congelado sem dispatch de overlay é consistente com MAIN girando no actor loop
(`func_800C2AB8`), não com idle. O PC/callback exato do hang ainda não foi
confirmado neste checkpoint.

### Threads/runqueue/host VI

- VIMGR, SCHEDULE, JOY/MOTOR/CIC e outros estavam bloqueados em MQs vazias.
- Running queue continha IDLE; MAIN era current/running, já retirada da fila.
- Self-loop: NO; duplicate: NO; invalid pointer: NO.
- Host VI thread viva, em pacing de aproximadamente 60 Hz.
- Cerca de sete `external_messages` retidas: consequência/correlação, não causa
  primária provada.
- `SCHEDULER_BUG_PROVEN=NO`.

### Transition type-4 (corrigido pelo clean snapshot)

- Count: `1`.
- Objeto: `0x8027AAC0` naquele run; endereço heap-specific (idêntico nos 8 runs).
- `+0x0C target_duration = 0.9` (NÃO é progress/elapsed).
- `+0x18 cfg_param3 = 3.5` (NÃO é duration/target).
- `+0x10 cfg_sfx_id = 2883` (NÃO é tick counter nem `+0x20`/`+0x30`).
- `+0x20 active_sfx_id = 2885`.
- `+0x28 elapsed_time = 0.0` (reset após completion/state transition).
- `+0x30 state_frame_counter = 1`.
- `current_state = 2`, `render_frame_count = 2` → gate do warp **ABERTO**.
- `subactor = NULL` → não bloqueia completion.

CORRECTED: `TRN_CNT=0` veio do endereço errado. O byte efetivo é o físico
correspondente a aproximadamente `0x8012AB57` e vale `1`.

Não chamar `2883` de “ticks reais”: `2883` é o `cfg_sfx_id` em `+0x10`; o
contador real em `+0x30` vale `1`.

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

### Type-4 timing/progress stall

REFUTED pelo clean snapshot: a type-4 completou no run natural
(`current_state=2`, `render_frame_count=2`, gate aberto). `elapsed=0` é o reset
pós-completion. `effective_dt = 0.05` (`0x8012C760 = 0x3D4CCCCE`,
`0x801275DC = 1.0`, `timing_flag=1`) é coerente; 18 updates × 0.05 = 0.9 explica
os runs perturbados. `FRAME_TIME_DIVERGENCE` não provada.

### Warp consumption

REFUTED: `warp=3` é consumido no clean freeze; `D_80127760` retém
`map=335`/`entrance=8`/`kind=2`; mode machine `+0x1C=5` (gravado por
`func_800A76F4(5)`).

## TIMING / OBSERVATION WARNING

GDB, breakpoints e instrumentação em hot paths mudam o timing. Nesses runs, a
type-4 termina, warp é consumido e mode avança. Use os dados perturbados apenas
para contratos locais. Para o estado natural:

```text
run clean -> wait freeze -> attach only after freeze -> capture snapshot/core
```

## OPEN QUESTIONS

- Qual lista/chain circular `func_80026500` está caminhando no freeze (head,
  node, key)?
- O ciclo é corrupção de lista, binding errado ou estado legítimo do jogo?
- Qual overlay/actor exato em `0x80801484` (bskaztorpedo vs bsstonehead — mesma
  vram) chamou o insert?
- Por que o slot de kinematics `7` ficou com substate 2 e `table[2].word0 != 0`
  sem ser processado?
- Por que este ambiente roda a ~20 Hz (`delta=0.05`)?
- Um run em 7 mostrou a thread spinning em `recompui::create_empty_document`
  em vez de `func_80026500` — variança não reprodutível como padrão.

BaKey (`func_800BCF28`) é POSSIBLE DOWNSTREAM BLOCKER visto só depois que runs
perturbados avançam. Não é root cause provada do clean freeze.

## FILES TO READ NEXT

1. [[p08-attract-actor-loop-diagnosis]] — diagnóstico do freeze em
   `func_80026500` (MAIN 60/60 no walk) + slot 7 preso.
2. [[p08-type4-transition-semantics-audit]] — auditoria estática + clean
   snapshot (correção de campos 0.9/3.5/2883).
3. [[p08-intro-to-demo-transition-diagnosis]] — timeline e post-mortem
   (narrativa antiga marcada como SUPERSEDED).
4. [[p08-visual-validation/README]] — interpretação humana dos frames.
5. [[p08-overlay-rmips26-binding-diagnosis]] — último fix antes do blocker.
6. [[p08-do-dma-segv-diagnosis]] — cadeia histórica fechada.
7. [[p08-natural-actor-spawn-divergence]] — IPL3/actors.
8. [[p08-no-threads-left-diagnosis]] — scheduler fechado.
9. [[p08-active-banjo-reuse-audit]] — limites de reuso/camadas.
10. [[p08-title-input-eeprom]] — input diferido.
11. `docs/HANDOFF.md` e `docs/plans/BT/08-rt64-input-and-frontend.md`.

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
