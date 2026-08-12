---
title: "P08 — diagnóstico consolidado da transição intro para attract demo"
description: "Checkpoint da validação visual, warp/type-4 e post-mortem do freeze natural depois da Jiggy transition."
status: evidence
phase: P08
updated: 2026-08-11
tags: [tooie-recomp, p08, intro, attract, transition, clean-freeze, post-mortem]
---

# P08 — INTRO → ATTRACT DEMO transition diagnosis

BASE HEAD: `cf3116696e290069372a85764929c69284135d0e`
MODO DESTE CHECKPOINT: documentação e consolidação; nenhum diagnóstico novo,
fix, instrumentação ou alteração de código.

## Estado canônico

> The opening/intro sequence completes visually through the Jiggy transition,
> but the subsequent gameplay demo/attract scene does not start.

O fluxo solicita corretamente a primeira demo (`map=335`, `entrance=8`,
`warp=3`) e mantém uma transition type-4 ativa. No freeze natural, sem debugger
ou instrumentação durante a execução, o post-mortem prova que a MAIN guest ainda
executa o game tick; ela não está bloqueada em frame-sync nem na reply queue do
scheduler. A type-4 permanece incompleta (`progress/elapsed ≈ 0.9` contra
`duration/target ≈ 3.5`), portanto o warp continua gated e a gameplay demo não
começa.

FIRST PROVEN CURRENT DIVERGENCE: a MAIN continua executando o tick, mas a
transition type-4 não alcança completion/progresso suficiente no run limpo.

## Contexto do projeto e camada de correção

TooieRecomp é a recompilação estática de Banjo-Tooie formada por `N64Recomp`,
`N64ModernRuntime`, `RT64` e `RecompFrontend`. `RecompiledFuncs` é saída gerada e
nunca deve ser editada manualmente. Correções reproduzíveis pertencem à camada
que possui a causa: generator, metadata, runtime ou integração source/host.

A prioridade ativa é P08. Este checkpoint fecha somente a documentação da
investigação INTRO → ATTRACT DEMO.

## Timeline consolidada

### 1. CONFIRMED — validação visual humana

As capturas foram feitas aproximadamente em `0005.png`, `0010.png`,
`0020.png`, `0030.png`, `0045.png`, `0060.png`, `0090.png`, `0120.png`,
`0150.png` e `0180.png`. A leitura humana é:

| frame aproximado | leitura validada |
| --- | --- |
| `0005` | preto inicial/boot |
| `0010` | abertura renderizando, céu/brilho |
| `0020` | cena 3D com Dolby Surround |
| `0030` | logo BANJO-TOOIE |
| `0045` | logo/cenário continuam e a câmera avança |
| depois | a sequência natural continua e a Jiggy transition fecha a cena |
| `0060+` | preto persistente; a primeira gameplay demo não aparece |

Uma captura isolada em `0060` não localiza com precisão o instante em que a
Jiggy terminou. A conclusão vem da validação humana do fluxo completo, apoiada
pela sequência observada. Não descrever o produto como “opening stalls after
logo”. Ver [[p08-visual-validation/README]].

### 2. CONFIRMED — fluxo original reconstruído

A reconstrução estática, correlacionada com o request observado, é:

```text
intro mode 3
  -> gctransitionDll / subaddiefade (Jiggy transition)
  -> gsattract overlay 731, ep5 (demo selector)
  -> func_800A9BA0
  -> func_800A79D4(map=335, entrance=8)
  -> func_800A7A90(...)
  -> create/retain transition type-4
  -> func_800A7B18(3)
  -> warp=3 pending
  -> future func_800A7D84 tick
  -> func_800C9510()==0
  -> consume warp
  -> func_800A9924(...)
  -> func_800A76F4(5)
  -> next tick mode 5
  -> func_800A9A14
  -> gsattract ep1
  -> gameplay demo
```

O request é `map=335 / 0x14F / MAP_14F_IOH_WOODED_HOLLOW`, `entrance=8`,
`warp=3`. Os símbolos e argumentos são sustentados pela auditoria estática e
pelos discriminators registrados abaixo; o binding de overlay que permitiu
chegar aqui está em [[p08-overlay-rmips26-binding-diagnosis]].

### 3. CONFIRMED — warp request

O request chega a `func_800A9BA0`, grava o mapa/entrada e agenda warp 3. A
gameplay demo ainda não começa porque o warp só pode ser consumido depois que o
gate da transition type-4 retorna zero.

### 4. HYPOTHESIS — missing tick / guest idle

A primeira leitura do freeze inferiu que a mode machine não recebia novos ticks
e que a guest teria ido para idle. Essa interpretação foi útil para desenhar os
discriminators, mas não sobrevive ao post-mortem limpo.

REFUTED: o post-mortem captura a MAIN viva dentro do game tick. `last_ovl=635:2`
e o rótulo diagnóstico anterior `stack=(idle)` não eram o PC atual da MAIN.

### 5. CORRECTED — transition count

Um diagnóstico anterior publicou `TRN_CNT=0` porque leu o offset/endereço
errado. Isso está incorreto.

No binário MIPS big-endian, o byte efetivo do transition count está no endereço
físico correspondente a aproximadamente `0x8012AB57`. No clean freeze:

```text
TRANSITION_COUNT = 1
ACTIVE_TYPE       = 4
```

Não repetir `TRN_CNT=0` como fato atual. O endereço do array é aproximadamente
`0x8012AB40`; o objeto apontado por ele é heap allocation específica de cada
run.

### 6. CONFIRMED — lifecycle da type-4

Auditoria estática consolidada:

```text
creation:
  func_800A7A90 -> func_800C9610 -> func_800C9150(4)
  -> gctransitionDll init

lookup/gate:
  func_800C9510 -> func_800C91C8(4)

update:
  func_800A7D84 -> func_800C92A8 -> gctransitionDll update

removal:
  terminal transition state -> func_800C9214 -> remove/free
  -> transition count--
```

REFUTED: não existe circular gate “a transition bloqueia o warp, então não pode
atualizar”. Mesmo com `func_800C9510()!=0`, `func_800A7D84` continua o restante
do tick e chega ao transition update.

### 7. CONFIRMED — warp consumption quando a type-4 termina

Em runs instrumentados/timing-perturbed:

```text
POST_WARP_123F4 = 19
POST_WARP_123BC = 19
POST_WARP_A7D84 = 19
POST_WARP_C9510 = 19

C9510 returns: 18 x nonzero, depois 1 x zero
func_800A9924 = 1
func_800A76F4(5) = 1
```

CONFIRMED: a lógica de warp funciona quando a type-4 termina.

REFUTED: o main loop não roda.

REFUTED: `D_80043380` está zero; ele permaneceu `1` nos ticks observados.

REFUTED: o dispatch lógico de warp está quebrado.

Esses runs provam contratos locais, mas não reproduzem fielmente o freeze
natural; ver a advertência de timing abaixo.

### 8. REFUTED — cothemedll como blocker

`last_ovl` frequentemente mostra `635:2 / cothemedll_entrypoint_2`. A auditoria
estática mostrou que esse entrypoint:

- não chama `osRecvMesg`;
- não possui DMA wait ou message queue wait bloqueante;
- retorna sincronicamente;
- é callback legítimo de theme/audio.

O clean post-mortem confirmou `LAST_OVL_IS_BLOCKING_PC=NO`. `635:2` é apenas o
último dispatch de overlay registrado, não o PC onde a MAIN está presa.

### 9. REFUTED — frame-sync / VI starvation

Contrato:

- queue `0x80078FD0`;
- initializer `func_80014FE8 -> osCreateMesgQueue(..., capacity=10)`;
- `osViSetEvent(queue=0x80078FD0, msg=0, retrace=1)`;
- producer host `vi_thread_func` / external-message machinery;
- consumer guest: frame-sync `osRecvMesg`.

Resultados repetíveis:

```text
RECV_ENTER=57       RECV_RETURN=57       OUTSTANDING=0
SEND_ATTEMPT=57     SEND_SUCCESS=57
PRODUCER_EVENT=361  HOST_VI=361
```

CONFIRMED: producer/consumer estava saudável nesses runs.

REFUTED: “a MAIN congela porque VI/frame-sync deixa de entregar mensagens”.

### 10. REFUTED — event-3 / graphics flush barrier

Contrato estático:

```text
func_80013D10(event=3)
  -> send request para 0x800783D0 (capacity 20)
  -> scheduler processa msg3
  -> ACK imediato ou deferido
  -> reply queue 0x80078438 (capacity 10)
  -> blocking recv da MAIN
  -> retorna após ACK
```

Completion deferida usa scheduler msg5 / `func_8001443C`.

```text
EVENT3_CALL=3        EVENT3_HANDLER=3
ACK_SEND=3/3 success
ACK_RECV_ENTER=3     ACK_RECV_RETURN=3    OUTSTANDING=0
MSG5_DISPATCH=57
```

O diagnóstico limpo também registrou todos os scheduler ACKs concluídos antes
do freeze. REFUTED: `func_80013D10(event=3)` é o blocker.

### 11. CONFIRMED — observação altera timing

GDB, breakpoints, counters/hooks e até instrumentação leve em hot paths
alteraram o timing o suficiente para a type-4 terminar, o warp ser consumido e
o mode avançar. Portanto:

- runs perturbados são evidência válida de contratos locais;
- não são reprodução fiel do freeze natural;
- a reprodução ground truth usa binário original, sem debugger e sem hooks;
- debugger/core só devem ser anexados depois de o freeze estar confirmado.

### 12. CONFIRMED — clean-freeze post-mortem

Esta é a evidência central do checkpoint.

#### Reprodução

```sh
TOOIE_AUTO_START=1 \
TOOIE_DIAG=1 \
TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
./build/TooieRecompiled
```

Binário original, sem debugger e sem instrumentação durante a execução. Freeze
confirmado antes do attach: black screen pós-intro, syscall `d≈179275` sem
mudança por pelo menos cinco segundos e `last_ovl=635:2`. Estado típico da
fronteira: `mode=3`, `map=335`, `entrance=8`, `warp=3`, transition count `1`.

Somente depois: GDB attach e `gcore`. O core tinha aproximadamente 6.4 GB e 48
LWPs. Ele é artefato temporário, não versionável. Com `ptrace_scope=1`, o attach
exigiu que shell/GDB fosse o pai apropriado; esse é apenas um detalhe de
reprodução.

#### Achado central: MAIN viva no tick

OSThread guest MAIN:

```text
address  = 0x80045788
id       = 6
priority = 20
queue    = 0
```

`queue=0` significa que a MAIN não estava registrada como bloqueada em message
queue no snapshot. Backtrace capturado, sem reinterpretação:

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

Conclusão segura: MAIN está viva e executando guest code no game tick, mas não
faz progresso normal suficiente para completar a transition e alcançar os
próximos syscalls.

#### Outras guest threads e runqueue

VIMGR, SCHEDULE, JOY/MOTOR/CIC e outros subsistemas relevantes estavam
bloqueados em message queues vazias. A running queue observada continha apenas
IDLE, enquanto MAIN era a guest thread current/running já retirada da fila.

```text
RUNQUEUE_SELF_LOOP = NO
DUPLICATE_THREAD   = NO
INVALID_POINTER    = NO
SCHEDULER_BUG_PROVEN = NO
```

Não concluir que MAIN “sumiu da running queue”: current thread e queued runnable
threads têm semânticas diferentes.

#### Host VI

A host VI thread estava viva em pacing/sleep de aproximadamente 60 Hz. O
snapshot continha cerca de sete `external_messages` retidas/não drenadas. Isso
é correlação/consequência de a guest não alcançar o ponto normal de drenagem,
não causa primária provada. Frame-sync já foi exonerado independentemente.

#### Raw state da transition type-4

O array apontava para uma type-4 ativa, naquele run aproximadamente em
`0x8027AAC0`. Esse endereço é heap-specific e não é constante global.

```text
transition count      = 1
type                  = 4
progress/elapsed      ≈ 0.9
duration/target       ≈ 3.5
RAW_FIELD_VALUE       ≈ 2883
FIELD_SEMANTICS_NOT_FULLY_VALIDATED = YES
```

CONFIRMED: a type-4 ainda não atingiu a condição temporal simples de completion.
Não documentar “2883 ticks reais da transition”: a interpretação estática
anterior do campo como tick counter conflita com a expectativa de cerca de 18
updates observada no run perturbado. A discrepância é evidência aberta, não algo
a esconder ou fechar por inferência.

### 13. CURRENT — blocker, limites e próximo passo

CURRENT BLOCKER: depois da intro/Jiggy visualmente correta, a primeira attract
demo não inicia porque a type-4 permanece incompleta no clean freeze enquanto a
MAIN executa o game tick; warp 3 continua gated.

Ainda não está provado:

- por que o progress permanece em aproximadamente `0.9`;
- se o delta-time usado pela transition está errado;
- se a state machine interna está presa;
- se um subactor visual impede completion;
- se uma timing-source flag está errada;
- se `func_800C2AB8 / get_function(0x800C269C)` é causalmente relevante;
- se algum callback esperado é despachado incorretamente;
- se o raw field `2883` foi semanticamente interpretado errado.

Esses itens pertencem à próxima investigação e não são conclusões deste
checkpoint.

## Resolved blockers / do not reopen

### A. Overlay data relocation / `LOOKUP_FUNC(0)`

`readelf -S` sem `-W` truncava nomes longos de seções relocatable no generator
de metadata, descartando relocations de data de overlays. O fix corrigiu o
generator/listas e `ActorInfo[0x38]` passou a conter o callback correto. Ver
[[p08-overlay-data-relocation-diagnosis]].

### B. Actor manager

O `D_80136EE0` nulo veio de spawn artificial. No fluxo natural, o pipeline
`func800A76F4/800A7BB8 -> func800A72A4 -> func800EA0CC -> gsworldDll ep2 ->
func800A5C28 -> chbaddieDll ep1 -> func800FF390 -> func801065E0 -> vector_new ->
D80136EE0` foi comprovado, com valor aproximado `0x801DF080`. Actor manager não
é o blocker. Ver [[p08-natural-actor-spawn-divergence]].

### C. `func800D6E54` false lead

É processamento diferido de resource/reclaim, não population pass de actors. O
early return observado era legítimo.

### D. Resident overlay cache / gcsection

`ovl_unload` liberava heap sem invalidar `s_resident_overlays`; outro overlay
reutilizava o heap e deixava stale mapping. A invalidação pós-`heap_free`
resolveu o crash gcsection/glintrosync. Ver
[[p08-gcsection-glintrosync-segv-diagnosis]].

### E. Scheduler antigo

O antigo `No threads left to run!` foi corrigido no N64ModernRuntime com scan
real de duplicata em `thread_queue_insert`, traversal correto em
`thread_queue_remove` e guard apropriado de reinserção em `osSetThreadPri`.
Harness 13/13 PASS e runs posteriores sem mensagem, self-loop ou corrupção.
Não chamar o blocker atual de scheduler bug sem nova prova. Ver
[[p08-no-threads-left-diagnosis]].

### F. IPL3 integrity gate

`func8001DE64` esperava `0x802FB1F4=0xAD090010` e
`0x802FE1C0=0xAD170014`. O static recomp não reconstruía esse estado e o stub
tinha erro de sign extension/endereço. O fix corrigiu a tradução e fez seed
one-shot preservando o probe. Em 3/3 runs: flag=0, BDCDC=1, cube/collect,
`c90=11`, `c2c=10`, actor alloc=10. Natural actor creation restaurada. Ver
[[p08-natural-actor-spawn-divergence]].

### G. R_MIPS_26 / flat func_map / do_dma

Com `use_lookup_for_all_function_calls=true`, JALs intra-overlay conhecidos
viravam `LOOKUP_FUNC(vram)` apesar da colisão de canonical VRAM `0x80800000`.
`func_80800484_chintrochar -> jal 0x80800000` resolvia indevidamente para
`gsworldDll_entrypoint_0`, levando a `gsmaker`, idx `-0x3AB`, asset OOB,
`devAddr=0xF13B24`, `size=0xFF3FF03E` e SIGSEGV em `do_dma`.

O generator agora emite direct C call para known same-section JAL em section
RELOCATABLE; demais casos preservam lookup, inclusive non-relocatable para host
hooks. Patch: `patches/runtime/n64recomp-same-section-jal.patch`. Validação 3/3:
old do_dma=0, idx -0x3AB=0, bad dev/size=0, `LOOKUP_FUNC(0)=0`, regressões de
scheduler=0. Ver [[p08-overlay-rmips26-binding-diagnosis]] e
[[p08-do-dma-segv-diagnosis]].

## BaKey — downstream only

Em runs perturbados, a transition termina, o warp é consumido e o mode avança;
depois surgiu crash em `func_800BCF28 / BaKey`. O path perturbado pode não ter a
mesma init ordering do original limpo.

Classificação atual: POSSIBLE DOWNSTREAM BLOCKER. Não é a root cause do clean
freeze e não deve ser diagnosticado neste checkpoint.

## Próximo single action — documentado, não executado

Investigar por que a transition type-4 ativa no freeze natural permanece
incompleta enquanto MAIN executa o game tick:

1. determinar o estado interno e a semântica exata dos campos da type-4;
2. validar `progress/elapsed≈0.9` contra `duration/target≈3.5`;
3. resolver a discrepância do raw field `≈2883`;
4. identificar a timing source/delta que alimenta `gctransitionDll ep9`;
5. verificar gates de state/subactor completion;
6. comparar com original/emulador no mesmo handoff, se possível;
7. só então desenhar discriminator de baixíssima perturbação.

Não executar essa investigação como parte deste checkpoint.

## Referências

- [[p08-session-handoff]] — handoff autossuficiente para novo chat.
- [[p08-visual-validation/README]] — inventário e leitura dos frames.
- [[p08-overlay-rmips26-binding-diagnosis]] — último blocker resolvido antes deste.
- [[p08-do-dma-segv-diagnosis]] — cadeia histórica de corrupção/DMA.
- [[p08-natural-actor-spawn-divergence]] — actor pipeline e IPL3 gate.
- [[p08-no-threads-left-diagnosis]] — scheduler fix e harness.
- [[p08-active-banjo-reuse-audit]] — fronteiras de reuso BK.
- [[p08-title-input-eeprom]] — input diferido; tentativa EEPROM não validada.

## Integridade do checkpoint

```text
SOURCE_CODE_CHANGED    = NO
RUNTIME_CODE_CHANGED   = NO
GENERATOR_CODE_CHANGED = NO
PATCH_CODE_CHANGED     = NO
CORE_INCLUDED          = NO
BUILD_ARTIFACT_INCLUDED= NO
```
