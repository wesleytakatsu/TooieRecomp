---
title: "P08 — guest preemption causal validation (clean dynamic experiment, class A)"
description: "Experimento dinâmico reversível que dá ao runtime N64ModernRuntime uma oportunidade cooperativa de agendamento enquanto MAIN gira em func_800C2AB8: com yield_self_1ms injetado via strong override, a thread AUDIO roda naturalmente, func_800205D0 limpa table[2].word0, C2AB8 retorna e o attract progride — provando causalmente o gap de preempção."
status: evidence
phase: P08
updated: 2026-08-12
tags: [tooie-recomp, p08, preemption, causal-validation, clean-experiment, scheduler, C2AB8, AUDIO, class-A]
---

# P08 — GUEST PREEMPTION CAUSAL VALIDATION

## Meta

```text
MODEL:        DeepSeek V4 Flash — HIGH
BASE_HEAD:    e42162469aaf815be25b5fb0e771e80c8aa54306
RUNTIME_REPO: /home/wesley/projetos/N64-Recomp/BanjoRecomp/lib/N64ModernRuntime
              (origin https://github.com/N64Recomp/N64ModernRuntime)
RUNTIME_HEAD: ca568b6ad79b9029d14077f0c3ffa757727c5559
RUNTIME_WORKTREE: DIRTY com fixes locais pré-existentes documentados
              (threadqueue Fix B + threads NULLPTR guard + N64Recomp policy)
              — NÃO alterados neste experimento
MODE:         CLEAN DYNAMIC EXPERIMENT: baseline unmodified + 1 run com
              strong override temporário de func_800C2AB8 chamando a primitiva
              de yield existente do runtime (yield_self_1ms). Sem debugger/
              hook/counter durante o caminho. Patch revertido após a prova.
OBJECTIVE:    Confirmar/refutar causalmente: dar ao runtime uma oportunidade
              de scheduler/preempção cooperativa durante o spin de MAIN em
              C2AB8 faz a thread AUDIO rodar, func_800205D0 limpar table[2],
              C2AB8 retornar e o attract progredir.
```

## Runtime API analisadas (§4)

Contratos lidos das fontes reais (não assumidos):

```text
FUNCTION: yield_self_1ms(rdram)   [ultramodern/src/scheduling.cpp:45]
ROLE:     = wait_for_external_message_timed(1) + check_running_queue()
MAY_PROCESS_EXTERNAL_MESSAGES: YES  (dequeues 1 msg externa e do_send ao mq)
MAY_SWITCH_TO_HIGHER_PRIORITY_THREAD: YES (check_running_queue -> swap_to_thread)
BLOCKS_CURRENT_THREAD: YES (até 1 ms aguardando msg externa; se msg pronta, imediato)
SLEEPS_HOST_TIME: YES (bounded <=1 ms; retorna antes se msg disponível)
SAFE_FROM_RUNNING_GUEST_CONTEXT: YES (usada por pause_self/yield_self do guest)

FUNCTION: check_running_queue(rdram)   [scheduling.cpp:18]
ROLE:     se running_queue tem thread de prioridade maior -> swap_to_thread
MAY_PROCESS_EXTERNAL_MESSAGES: NO
MAY_SWITCH_TO_HIGHER_PRIORITY_THREAD: YES
BLOCKS_CURRENT_THREAD: YES (somente se swap ocorre; swap insere self e espera resume)

FUNCTION: wait_for_external_message_timed(rdram, ms)  [mesgqueue.cpp:44]
ROLE:     espera (bounded) msg externa, depois do_send para o mq destino
MAY_PROCESS_EXTERNAL_MESSAGES: YES (1 msg)
BLOCKS_CURRENT_THREAD: YES (bounded <=ms)

FUNCTION: dequeue_external_messages / swap_to_thread / run_next_thread_and_wait
ROLE:     primitivas internas; yield_self_1ms compõe wait+check
```

`yield_self_1ms` = "B — existing runtime yield primitive" e implementa o serviço
semântico mínimo da "A — runtime scheduler service/poll point": processa msgs
externas pendentes, avalia a run queue, permite AUDIO de prioridade maior rodar
e depois retoma MAIN.

## Método de diagnóstico (§5)

```text
DIAGNOSTIC_METHOD: C — strong override temporário de func_800C2AB8
                   (símbolo weak, mecanismo já suportado: boot_hw_stubs.cpp
                   prova strong override de RECOMP_FUNC weak; func_800C2AB8
                   é 'W' no ELF baseline) chamando a primitiva EXISTENTE
                   yield_self_1ms(rdram) uma vez por passada externa do loop
                   enquanto pending != 0.
LAYER: TooieRecomp não-gerado (src/game/p08_preemption_diag.cpp) — nenhum
       código gerado tocado; nenhuma mudança de runtime.
WHY_THIS_METHOD:
  - A (runtime poll point) não pôde ser testado limpo: o único ponto de runtime
    alcançado no spin é get_function (hot path global); um poll lá perturbaria
    todo o jogo.
  - B (primitiva existente) é o serviço correto, mas MAIN nunca a chama; para
    testá-la limpo é necessário injetá-la no spin — exatamente o override.
  - C usa o mecanismo de override já suportado + a primitiva de yield existente
    (B) → menor e mais reversível, prova o gap sem desenhar o scheduler final.
```

## BASELINE (§6) — reproduzido

Run unmodified (build HEAD e421624, símbolo func_800C2AB8 = 'W'):

```text
FREEZE d=179165 (syscall d estável >= 8s)
MAIN host RIP = func_800C2AB8+0x1ba  (spin)
slot7 +0x78 = 0x01026C00  enabled=1 mode=3 substate=2
table[2].word0 = 0x8005C1B0 (live pool object)
head da delta-queue = 0x8005E97C  key(delta) = 0 (devido agora)  p4 = objeto
audio counters FROZEN: rsp_aud=1527 queue_calls=1526 queue_samp=2304048
```

Baseline corresponde exatamente ao estado documentado (class A, evento devido,
AUDIO estacionada). Sem divergência material.

## PATCH TEMPORÁRIO (§7) — diff exato (revertido)

Arquivo novo `src/game/p08_preemption_diag.cpp` (não gerado), strong override
de `func_800C2AB8`:

```cpp
extern "C" void yield_self_1ms(uint8_t* rdram);   // primitiva existente do runtime

extern "C" void func_800C2AB8(uint8_t* rdram, recomp_context* ctx) {
    // corpo idêntico ao funcs_327.c:1789-1940 (loop de 60 slots, mode 3)
    // única inserção DIAGNOSTICA no caminho `if (s3 != 0)` (pending):
    yield_self_1ms(rdram);              // <<< DIAGNOSTIC: coop scheduler opportunity
    ctx->r17 = ctx->r22 | 0;            // reset s1 = s6, goto L_800C2AF4
}
```

CMakeLists.txt: adicionada 1 linha para compilar o arquivo.

GENERATED_CODE_TOUCHED: NO. TEMP_CODE_COMMITTED: NO.

## BUILD (§8)

```text
BUILD_COMMAND: cmake --build build --target TooieRecompiled -j4
EXIT_CODE: 0
RUNTIME_HEAD (usado): ca568b6ad79b9029d14077f0c3ffa757727c5559
TOOIE_HEAD: e42162469aaf815be25b5fb0e771e80c8aa54306
após build: nm => func_800C2AB8 virou 'T' (strong)  [antes 'W']
```

## RESULTADO DO RUN DIAGNÓSTICO (§9)

Run normal SEM debugger/instrumentação durante a execução:

```text
INTRO_COMPLETES:       YES
TYPE4_COMPLETES:       YES (implicitamente, fluxo continuou)
WARP_CONSUMED:         YES (fluxo continuou)
MODE5_REACHED:         YES
C2AB8_RETURNS:         YES  — o freeze de C2AB8 DESAPARECEU
FIRST_ATTRACT_RENDERED: PARCIAL — attract/world init progrediu (overlays 111-728
                        carregados, gsworldDll/basetup iniciaram), depois
                        SIGSEGV downstream em func_800BCF28 (input/bakey path)
GAMEPLAY_DEMO_STARTED: NO (crash downstream, ver seção seguinte)
```

Prova dinâmica (logs diag do mesmo run):

```text
d (syscall counter) subiu continuamente: 24304 -> 35301 -> 66749 -> 97600
  -> 129788 -> 170536 (sem congelamento; baseline congelava em ~179165)
audio counters AVANÇARAM durante todo o run:
  rsp_aud: 124 -> 424 -> 723 -> 1023 -> 1503
  queue_samp: 186576 -> 639584 -> 1091488 -> 1544128 -> 2268720
  (no baseline esses contadores ficavam CONGELADOS)
```

A thread AUDIO RODOU e a síntese de áudio continuou — evidência causal direta.

## PROVA CAUSAL DE ÁUDIO (§10)

Snapshot pós-C2AB8 (do core do SIGSEGV downstream, estado após C2AB8 retornar):

```text
AUDIO_THREAD_RAN:            YES  (contadores de áudio avançaram o run todo)
func_8001ED68 RAN:           YES (implícito: table[2] foi limpo pelo chain 1ED68->1EE34->200B8->205D0;
                              único clearer estático de *obj->0x30)
func_8001EE34 RAN:           YES (implícito, mesmo chain)
func_800200B8 RAN:           YES (implícito, mesmo chain)
func_800205D0 RAN:           YES (implícito; provado pelo valor final)
table[2].word0:              BEFORE 0x8005C1B0 (live)  ->  AFTER 0x00000000 (CLEARED)
func_800C269C(7):            AFTER -> 1 (C2AB8 retornou; só retorna se ready)
slot7 +0x78/+0x79:           0x00 (C16A4 limpou enabled/substate) -> slot7 DISPENSADO
C16A4_7 RAN:                 YES (slot7 cleared)
C2AB8_RETURNED:              YES
```

Nenhuma instrumentação hot-path usada. O estado final foi lido do core do crash
downstream (sem instrumentação prévia). Chain causal fechado:

```text
yield_self_1ms(rdram) em MAIN spin
  -> wait_for_external_message_timed entrega msg AI pendente p/ audio mq
  -> AUDIO (bloqueada em osRecvMesg) acordada -> schedule_running_thread(AUDIO)
  -> check_running_queue: pri AUDIO 50 > MAIN 20 -> swap_to_thread(AUDIO)
  -> AUDIO roda func_800128C0 -> func_800129FC -> func_80021574 -> jalr mgr->handler
  -> func_8001ED68 -> func_8001EE34 -> func_800200B8 -> func_800205D0
  -> *obj->0x30 = 0  (table[2].word0 = 0)
  -> AUDIO bloqueia -> run_next_thread_and_wait -> MAIN resume
  -> func_800C269C(7) == 1 -> func_800C16A4(7) -> pending==0 -> C2AB8 retorna
```

## CLASSIFICAÇÃO (§11)

```text
PRIMARY_CLASS: A — CAUSAL CONFIRMATION
   A oportunidade de agendamento deixou AUDIO rodar naturalmente; o evento
   devido executou; func_800205D0 limpou table[2].word0; C2AB8 saiu; o attract
   progrediu até o world/input init.

Downstream blocker (NOVO, NÃO o gap): após C2AB8 retornar, o attract/world
init progrediu (overlays 111-728 carregados; gsworldDll_entrypoint_2 ->
basetup_entrypoint_2 -> func_800A4B08 -> func_80097B4C -> bakey_heldCount ->
func_800BCF28) e então SIGSEGV determinístico em func_800BCF28
(RIP off 0x656d11; core diag2). Esse é um bloqueador downstream NOVO, fora do
escopo desta tarefa (não investigar agora).

Rejeitados:
  B — não: AUDIO rodou e o evento avançou até limpar table[2]
  C — não: AUDIO rodou (contadores avançaram)
  D — não: table[2] foi limpo (0x0) E C2AB8 retornou
  E — parcial: C2AB8 corrigido diagnosticamente, mas attract ainda bloqueia
       downstream via SIGSEGV em func_800BCF28 (não é o gap de preempção)
  F — não: a mudança de comportamento corresponde exatamente ao chain esperado
       (table[2] limpo, slot7 dispensado, áudio avançando)
  G — não
```

## FIX-LAYER ASSESSMENT (§14)

```text
PREEMPTION_GAP_CAUSALLY_PROVEN: YES
RUNTIME_FIX_REQUIRED: LIKELY
   (o serviço correto é uma oportunidade cooperativa de scheduler durante
   spins de guest; o override local foi PROVA, não o fix preferido)
LOCAL_GAME_YIELD_REQUIRED: NO
   (a menos que evidência contrária; o fix preferido é no runtime,
    não yield por-game)
```

O override local (method C) é prova apenas — não o fix final. O design do
scheduler genérico fica para a próxima tarefa.

## CONCLUSÃO

- `func_800C2AB8` com uma oportunidade cooperativa de scheduler (via primitiva
  EXISTENTE do runtime `yield_self_1ms`) deixa a thread AUDIO rodar, o chain
  `func_8001ED68 → func_8001EE34 → func_800200B8 → func_800205D0` limpar
  `table[2].word0`, `func_800C269C(7)` voltar 1, `func_800C16A4(7)` dispensar o
  slot 7 e `func_800C2AB8` retornar. O attract progride até um SIGSEGV
  downstream NOVO em `func_800BCF28` (input/world init) — bloqueador fora do
  escopo.
- O gap de preempção do N64ModernRuntime é CAUSALMENTE PROVADO.

```text
CODE_CHANGED            = NO (após revert)
GENERATED_CODE_TOUCHED  = NO
RUNTIME_CODE_CHANGED    = NO
TEMP_PATCH_REVERTED     = YES
TOOIE_WORKTREE_CLEAN    = YES
RUNTIME_WORKTREE        = clean relativo ao estado inicial (dirty pré-existente documentado)
```

Artefatos (não versionados): `/tmp/opencode/p08p/baseline*`, `diag*`,
`core2/diag2.core` (supervisores, logs, core do crash downstream).

=== END EVIDENCE ===
