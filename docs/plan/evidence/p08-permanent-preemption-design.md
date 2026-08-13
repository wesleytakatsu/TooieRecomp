---
title: "P08 — permanent guest-preemption scheduler design (N64ModernRuntime + N64Recomp)"
description: "Design de arquitetura do fix genérico de preempção de threads guest no N64ModernRuntime: safepoint gerado em backward-branch + flag atômica de preempção + primitiva de serviço não-bloqueante no runtime. Sem implementação; sem código commitado."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, n64modernruntime, ultramodern, libultra, scheduler, safepoint, design, n64recomp]
---

# P08 — PERMANENT GUEST-PREEMPTION DESIGN

## Meta

```text
MODE:          DESIGN / ARCHITECTURE ONLY (nenhum código de runtime/game/gerador tocado)
BASE_HEAD:     afa729fb480970bb51180bf73b33509d628bbadc
MODEL:         DeepSeek V4 Pro 0813 — MAX
RUNTIME_REPO:  /home/wesley/projetos/N64-Recomp/BanjoRecomp/lib/N64ModernRuntime
RUNTIME_HEAD:  ca568b6ad79b9029d14077f0c3ffa757727c5559  (esperado)
RUNTIME_STATUS: dirty pré-existente (NÃO alterado): threadqueue.cpp (Fix B),
                threads.cpp (guard NULLPTR), submodule N64Recomp pointer dirty
```

## 0. Continuidade

Fatos provados herdados (não re-abrir):

```text
PROVEN:  MAIN = OSThread 6 pri 20; AUDIO = OSThread 4 pri 50 (higher numeric wins).
PROVEN:  Interrupções habilitadas durante func_800C2AB8 (Status.IE = 1).
PROVEN:  CPU original é preemptível dentro de guest code puro.
PROVEN:  N64ModernRuntime mapeia OSThreads guest para std::thread nativos,
         mas só UM execution token ativo (UltraThreadContext.running,
         moodycamel::LightweightSemaphore).
PROVEN:  Eventos externos são enfileirados em external_messages (host side),
         mas scheduling/progresso exige serviço cooperativo do runtime.
PROVEN:  func_800C2AB8 não tem ponto natural de runtime/preempção
         (loop MIPS puro, sem syscall).
PROVEN:  Clean freeze: slot7 pending, table[2] live, evento de áudio REGISTERED
         e DUE (delta 0), AUDIO parkada, MAIN girando.
PROVEN (causal): strong override temporário de func_800C2AB8 com
         yield_self_1ms(rdram) → AUDIO roda → 1ED68→1EE34→200B8→205D0 limpa
         table[2] → C269C(7)==1 → C16A4(7) → C2AB8 retorna → attract progride
         até SIGSEGV downstream NOVO em func_800BCF28. Patch REVERTIDO.
PROVEN:  PREEMPTION_GAP_CAUSALLY_PROVEN = YES.
```

O yield local foi evidência, não solução permanente.

---

## 4. Arquitetura de execução atual (mapeada nas fontes reais)

Caminhos lidos do checkout (não assumidos):

| componente | local | papel |
| --- | --- | --- |
| `UltraThreadContext` | `ultramodern.hpp:24` | `std::thread host_thread` + `LightweightSemaphore running/initialized` |
| token de execução | `threads.cpp:149-156` | `wait_for_resumed()` espera no `context->running` |
| `running_queue` | `threadqueue.cpp:5` | lista simples encadeada ordenada por prioridade (sentinel `-1`); `queue_to_ptr` resolve o sentinel |
| `external_messages` | `mesgqueue.cpp:15` | `moodycamel::BlockingConcurrentQueue<QueuedMessage>` |
| `enqueue_external_message` | `mesgqueue.cpp:17` | produtor host (VI/SP/DP/SI/timer) |
| `dequeue_external_messages` | `mesgqueue.cpp:23` | drena TODAS as msgs via `try_dequeue` e `do_send` |
| `wait_for_external_message[_timed]` | `mesgqueue.cpp:36/44` | espera UMA msg (bounded) |
| `do_send` | `mesgqueue.cpp:75` | insere na mq; se blocked_on_recv, `schedule_running_thread` |
| `do_recv` | `mesgqueue.cpp:114` | remove da mq; se blocked_on_send, `schedule_running_thread` |
| `check_running_queue` | `scheduling.cpp:18` | se head da running_queue tem pri > atual → `swap_to_thread` |
| `swap_to_thread` | `scheduling.cpp:9` | insere self na running_queue + `resume_thread_and_wait(to)` |
| `run_next_thread_and_wait` | `threads.cpp:173` | pop da running_queue + sinaliza + espera self |
| `yield_self_1ms` | `scheduling.cpp:45` | `wait_for_external_message_timed(1)` + `check_running_queue` |
| `pause_self`/`yield_self` | `scheduling.cpp:32/40` | espera msg externa + check_running_queue |
| `osSendMesg`/`osJamMesg`/`osRecvMesg` | `mesgqueue.cpp:146/168/190` | `dequeue_external_messages` + `do_send/do_recv` + `check_running_queue` |
| `osStartThread`/`osCreateThread`/`osSetThreadPri` | `threads.cpp:230/247/304` | lifecycle; `osSetThreadPri` re-insere na fila + check_running_queue |
| `this_thread()` | `threads.cpp:336` | `thread_local PTR(OSThread) thread_self` |
| produtores de evento host | `events.cpp` | VI thread (`vi_thread_func`) → VI/AI; SP (`sp_complete`) → SP; gfx (`dp_complete`) → DP; SI (`send_si_message`) → SI |
| produtor timer host | `timer.cpp:133` | `enqueue_external_message` quando OSTimer expira |
| `cop0_status_read/write` | `librecomp/src/recomp.cpp:421/456` | status_reg em `recomp_context.status_reg`; **só trata bit FR**; qualquer outro bit mudado → `assert(false)` |
| `osSetIntMask/__osDisableInt/__osRestoreInt_recomp` | `ultra_translation.cpp:121/125/129` | **NO-OPs** (corpo vazio) |
| generator (AOT) | `N64Recomp/src/cgenerator.cpp` + `recompilation.cpp` | emite `goto`/`label`/`if()`/`switch`; `emit_function_start` |
| generator (live) | `N64Recomp/LiveRecomp/live_generator.cpp` | implementa a mesma interface `Generator` |
| interface do generator | `N64Recomp/include/recompiler/generator.h` | virtual `emit_goto/emit_label/emit_function_start/...` |

Diagrama de execução:

```text
  [host VI/AI/SP/DP/SI/timer thread]
        |  enqueue_external_message(mq, msg)      (mesgqueue.cpp:17)
        v
  external_messages (BlockingConcurrentQueue)   <-- NÃO drenado sem ponto cooperativo
        |
        |  (só em osRecvMesg/osSendMesg/yield/pause)
        v
  dequeue_external_messages -> do_send -> schedule_running_thread -> running_queue
        |                                                          |
        v                                                          v
  check_running_queue ---------------------------------------> swap_to_thread
        |                                                          |
        v                                                          v
  [guest MAIN (OSThread 6) mantém token]                    [guest AUDIO (OSThread 4) parkada
   loop puro func_800C2AB8: SEM safepoint                      em running.wait() até ser sinalizada]
```

## 5. Componente do gap faltante

```text
PREEMPTION_GAP_COMPONENT: D — os três, unificados por UMA causa raiz estrutural.

A (msg não entregue até ponto cooperativo):        TRUE — do_send nunca roda; a
    msg fica em external_messages (visível ao runtime, invisível ao guest).
B (msg entregue mas running_queue não checada):    TRUE (decorrente) — a mq guest
    nunca recebe a msg, logo blocked_on_recv nunca é schedule_running_thread.
C (run queue atualizada mas token não transfere):  TRUE (decorrente) — mesmo que
    a fila fosse atualizada, não há ponto que chame check_running_queue/swap.
RAIZ: ausência de QUALQUER safepoint/ponto de serviço no guest code puro. O
      token só transfere em chamadas de runtime; func_800C2AB8 não tem nenhuma.

CLASSIFICAÇÃO: PROVEN (audit Gemini §6 + validação dinâmica + experimento causal).
```

## 6. Semântica de máscara de interrupção

```text
CAN_GENERIC_SAFEPOINT_OBSERVE_INTERRUPTIBILITY: NO (hoje).

Estado atual das fontes:
- cop0_status_read retorna ctx->status_reg (raw CP0 Status)  -> bit IE presente no dado
- cop0_status_write trata APENAS o bit FR; QUALQUER outro bit (inclusive IE, bit 0)
  muda -> "Unhandled status register bits changed" -> assert(false)/exit.
  => escrever IE via MTC0 hoje CRASHA.
- osSetIntMask_recomp / __osDisableInt_recomp / __osRestoreInt_recomp = NO-OPs.
  => máscara global de interrupção NÃO rastreada em lugar nenhum.
- Não há estado de "critical section" nem contagem de nesting.

O QUE FALTA (infraestrutura ausente):
1. Honrar o bit IE (0x1) em cop0_status_write (remover assert; não é bit de estado
   de FP), mantendo o tratamento de FR.
2. Implementar osSetIntMask/__osDisableInt/__osRestoreInt para rastrear
   "interrupts enabled" global + profundidade de nesting (__osDisableInt push,
   __osRestoreInt pop), expondo uma query barata interrupts_enabled().

NOTA DE FIDELIDADE: o bit raw já vive em recomp_context.status_reg, que é
acessível pelo safepoint gerado (mesmo ctx). A infraestrutura é um acréscimo
pequeno, não uma re-arquitetura. CLASSIFICAÇÃO: INFERENCE (factível; não implementado).
```

## 7. Semântica de prioridade — predicado de decisão

```text
SHOULD_PREEMPT(current) =
      interrupts_enabled()
  AND !thread_queue_empty(running_queue)
  AND peek(running_queue).priority > current.priority

Esse predicado JÁ é computado por check_running_queue (scheduling.cpp:18-30),
exceto pelo termo interrupts_enabled(). O fix precisa adicionar o gate de
interrupção; os outros dois termos reutilizam a primitiva existente.

CLASSIFICAÇÃO: PROVEN (o comparador de prioridade > já é a regra libultra e já
está implementado; o termo de interrupção é o único ausente).
```

## 8. Espaço de design

### A. Safepoints gerados

Inserir check leve de preempção em pontos do guest recompilado.

- function entry: barato; cobre entrada, NÃO cobre loop infinito interno.
- backward branch / loop backedge: cobre QUALQUER ciclo de controle → garante
  progresso de qualquer caminho infinito de execução.
- basic-block budget: cobre tudo; mais intrusivo, precisa de contador.
- instruction budget: maior custo; inviável semanticamente (guest não é
  interpretado por instrução).

### B. Flag de época / runtime timer

Host event seta um `std::atomic<bool> preemption_requested`. Safepoint checa
barato. Não usa clock; é o caminho negativo ideal (atomic load + branch).

### C. Preempção forçada de thread nativa

Suspender assincronamente o std::thread que detém o token e transferir.
Rejeitado: não há como salvar/restaurar o estado guest no meio de uma instrução
recompilada; o estado vive no `recomp_context*` apontado na pilha nativa, e só
é consistente em fronteiras de função/ramo. Suspensão forçada portável de
std::thread não é suportada (não há SuspendThread em POSIX; signals são
inseguras no contexto). CLASSIFICAÇÃO: REFUTED (inviável/portável).

### D. Hot-helper global (poll em get_function)

`get_function` é chamado em jal/LOOKUP_FUNC. Não cobre loops puros como
`func_800C2AB8` (que não faz jal). Insuficiente como mecanismo único.
Pode ser suplemento barato, mas não resolve o caso geral. CLASSIFICAÇÃO:
REFUTED como solução única.

### E. Yield específico do jogo

Strong override de func_800C2AB8 com yield. Foi o diagnóstico. Rejeitado como
fix permanente: knowledge específico de Tooie no runtime genérico. CLASSIFICAÇÃO:
REFUTED como fix permanente (PROVEN como diagnóstico).

### F. Híbrido (RECOMENDADO)

```text
host event -> enqueue_external_message -> preemption_requested = true (release)
guest safepoint (backward branch):
    if (preemption_requested.load(acquire))   // fast path: ~1 atomic load + branch
        service_preemption_if_requested(rdram)
runtime service:
    preemption_requested.store(false, release)   // clear antes de drenar
    dequeue_external_messages(rdram)             // drena tudo (try_dequeue)
    if (interrupts_enabled()) check_running_queue(rdram)  // swap se pri maior
```

Reaproveita `dequeue_external_messages` e `check_running_queue` existentes.
Adiciona (a) flag atômica global, (b) service não-bloqueante, (c) gate de
interrupção, (d) hook de geração. Não dorme, não chama clock, não mutex no
caminho negativo. CLASSIFICAÇÃO: INFERENCE (melhor candidato; ver seção 20).

## 9. Requisitos atendidos

| requisito | como é preservado |
| --- | --- |
| semântica de prioridade libultra | `peek.priority > current.priority` (inalterada) |
| interrupt masking | novo gate `interrupts_enabled()` antes do swap |
| determinismo | safepoint determinístico (backward branch), sem timer/quantum |
| sem execução concorrente de 2 guest sobre RDRAM | token único inalterado; swap é o mecanismo existente |
| semântica de message queue | `dequeue_external_messages`/`do_send`/`do_recv` inalterados; FIFO preservado |
| fixes de scheduler existentes | `check_running_queue`/`swap_to_thread`/Fix B reutilizados, não substituídos |
| performance | negative path = 1 atomic load + branch |
| portabilidade | std::atomic + std::thread existentes; nada de signals/SuspendThread |
| sem knowledge de endereço Tooie no runtime | nenhum endereço/função específica referenciada |
| sem edição de código gerado manual | hook no GENERATOR, não no RecompiledFuncs |

## 10. Segurança / concorrência

| risco | análise |
| --- | --- |
| RACE no flag | clear-antes-de-drenar + release/acquire: produtor enfileira ANTES de setar flag; se o dreno perder a msg, o set do flag aconteceu depois do clear → próximo safepoint re-serviça. Sem lost-wakeup. |
| TOKEN OWNERSHIP | inalterado: só o holder do token roda guest e alcança o safepoint; swap é o mesmo caminho de osSendMesg. |
| MEMORY ORDERING | `enqueue_external_message`: enqueue (plain) + `flag.store(true, release)`. safepoint: `flag.load(acquire)`. x86-64 (target): praticamente gratuito. |
| ABA / stale thread state | Fix B pré-existente (threadqueue.cpp) já impede insert duplicado e queue malformada; `thread_queue_pop` limpa `->queue` e `->next`. Não reabrir. |
| interrupção chegando durante o swap | flag setada durante swap fica true; o thread que assume o token a observa no próximo safepoint. Inofensivo. |
| preempção aninhada | thread de pri maior que assume o token pode ser ele mesmo preemptado por outro de pri ainda maior — semântica libultra correta. |
| inversão de prioridade | swap só se `peek.priority > current.priority`; thread de pri menor/igual NUNCA preempta. |
| estado de interrupção desligada | swap gated por `interrupts_enabled()`. Ver seção 12 para re-serviço ao religar. |
| destruição de thread | `osDestroyThread`/`wait_for_resumed` já tratam contexto nulo/substituído; inalterado. |
| mutação de message queue | `do_send`/`do_recv` só rodam de game thread com token (ou de dreno no safepoint, também game thread). Produtores só tocam `external_messages` (thread-safe). |
| lifetime de semáforo host | `UltraThreadContext` liberado por `thread_cleaner_thread`; inalterado. |
| shutdown/pause/menu | `pause_self` já espera msg + check_running_queue; o flag só é observado por thread que detém token (parada no menu = não roda guest). Debugger: safepoint só roda em contexto guest, que fica suspenso durante o debug. |

CLASSIFICAÇÃO: INFERENCE (análise de risco; sujeita a revisão adversarial).

## 11. Performance

| granulação | custo negativo estimado | avaliação |
| --- | --- | --- |
| check em toda chamada de função | 1 atomic load + branch | ok, mas insuficiente p/ loops |
| check em todo basic block | 1 atomic load + branch, mais frequente | desnecessário |
| check em loop backedge | 1 atomic load + branch, raro | IDEAL |
| instruction budget | contador por bloco + branch | overhead extra, desnecessário |
| atomic load (acquire) | ~sub-ns, sem serialização em x86 | ideal |
| host clock query | syscall/vdso, mais caro | EVITADO (flag-based) |
| mutex/semaphore call | só no caminho POSITIVO (raro) | aceitável |

Caminho negativo ideal atingido: `atomic/branch only`. CLASSIFICAÇÃO: INFERENCE.

## 12. Fast path da requisição de preempção

Compatível com o codebase. `enqueue_external_message` é o chokepoint ÚNICO de
todos os produtores host (VI/AI em vi_thread_func, SP em sp_complete, DP em
dp_complete, SI em send_si_message, timer em timer_thread). Basta:

```cpp
void ultramodern::enqueue_external_message(...) {
    external_messages.enqueue({mq, msg, jam, requeue_if_blocked});
    preemption_requested.store(true, std::memory_order_release);  // NOVO
}
```

e o service (ver seção 15). Não adotar cegamente; é a hipótese preferida,
sujeita à revisão adversarial (NEXT_SINGLE_ACTION). CLASSIFICAÇÃO: INFERENCE.

## 13. Localização do safepoint

```text
MINIMUM_SAFEPOINT_SCHEME_THAT_SOLVES_GENERAL_CASE:
    safepoint em BACKWARD BRANCH (loop backedge) + flag atômica.

Justificativa: func_800C2AB8 é um loop MIPS retroativo sem chamadas. Qualquer
caminho de execução infinito em código MIPS finito percorre um arco retroativo
do grafo de controle (estado finito + código finito => ciclo). Logo, safepoint
em backward branch garante liveness para TODOS os spins de guest, não só C2AB8.

function-entry-only é INSUFICIENTE (não intercepta o ciclo interno).
basic-block/instruction budget são supersets desnecessários.
function entry + backedge = cobertura máxima com custo mínimo; o backedge é o
mínimo CORRETO.

O generator já emite `goto label` para branch (recompilation.cpp:268-276,
cgenerator.cpp:431-434); o safepoint é um emit extra quando o target já foi
emitido (backward). CLASSIFICAÇÃO: INFERENCE (mínimo provável; validar no fix).
```

## 14. Responsabilidade N64Recomp vs Runtime

```text
FIX_COMPONENTS:
  N64Recomp:      YES — o hook de geração (emitir check em backward branch) vive
                  no generator (CGenerator AOT + LiveGenerator). Sem ele, nenhum
                  safepoint existe em guest code puro.
  N64ModernRuntime: YES — flag atômica + service_preemption_if_requested +
                  gate de interrupção + tracking de IE/máscara global.
  TooieRecomp:    NO — nenhum knowledge de endereço/função; apenas recompila e
                  usa o runtime atualizado. Sem strong override permanente.

CLASSIFICAÇÃO: PROVEN (evidência: o spin é guest puro; o serviço é runtime).
```

## 15. Reuso de infraestrutura existente

```text
service_preemption_if_requested(rdram):
    preemption_requested.store(false, release)
    dequeue_external_messages(rdram)            // já existe (mesgqueue.cpp:23), try_dequeue, sem sleep
    if (interrupts_enabled()) check_running_queue(rdram)  // já existe (scheduling.cpp:18)

Reuso:
- dequeue_external_messages: drena sem dormir (try_dequeue em loop) -> NÃO é o
  yield_self_1ms (que dorme até 1ms). O ponto de 1ms por safepoint é
  explicitamente REJEITADO (seção 15 do prompt).
- check_running_queue: swap por prioridade; só bloqueia quando swap ocorre (desejado).
- NOVO: preemption_requested (atomic bool), interrupts_enabled(), o service wrapper.

Os internos atuais PERMITEM isso: dequeue é não-bloqueante, check_running_queue
só faz swap sob condição, e o token continua único. CLASSIFICAÇÃO: PROVEN (lido
das fontes).
```

## 16. Política de serviço da fila de mensagens externas

```text
EXTERNAL_MESSAGE_SERVICE_POLICY:
  drenar TODAS as mensagens pendentes no momento do serviço (bounded pelo tamanho
  da fila), em ordem FIFO, via dequeue_external_messages (já drena tudo).

Ordem/estagnação:
- BlockingConcurrentQueue é FIFO; do_send preserva ordem por mq destino.
- requeue_if_blocked re-enfileira no fim (comportamento já existente; não muda ordem
  observável em relação ao baseline).
- Drenar tudo não estagna: o conjunto é limitado pelo que há no instante; novas
  chegadas são atendidas no próximo safepoint (o flag é re-setado).
- NÃO drenar "uma só msg" (como wait_for_external_message) tornaria o serviço
  subótimo para rajadas e poderia adiar wakups legítimos.

CLASSIFICAÇÃO: INFERENCE.
```

## 17. Caso especial de áudio host?

```text
GENERIC_INTERRUPT_CLASSES_SUPPORTED:
  AI, VI, SP, DP, SI, PI, timer — TODOS que produzem mensagem externa via
  enqueue_external_message (o chokepoint único). O fix é genérico: NENHUM
  hardcode de AUDIO. Áudio é apenas o primeiro caso observado (AI é o evento que
  acorda a thread AUDIO em Tooie). CLASSIFICAÇÃO: PROVEN (chokepoint único).
```

## 18. Estratégia de teste (desenhada antes da implementação)

```text
UNIT/HARNESS (threadqueue + scheduler harness existentes):
 1. guest "MAIN-like" de pri baixa gira em loop puro; evento host assíncrono
    acorda guest de pri alta; pri alta preempta; pri alta bloqueia; pri baixa
    retoma.
 2. Verificação de ordem: eventos N chegam em rajada -> todos entregues em FIFO.

INTERRUPT MASK:
 3. evento chega com interrupts disabled -> NENHUM swap imediato; ao religar
    (__osRestoreInt/osSetIntMask) -> serviço pendente -> swap ocorre.

PRIORITY:
 4. wakeup de pri menor/igual -> NÃO preempta (spin continua; fila aguarda).

STRESS:
 5. interrupções repetidas, chegadas aninhadas, produção rápida de msgs
    (flag setado/limpo em rajada -> sem lost-wakeup).

REGRESSION:
 6. harness de threadqueue (Fix B) e de scheduler inalterados.
 7. Tooie clean run: intro, áudio, C2AB8, attract (o critério original).
```

## 19. Bloqueador downstream

```text
NEW_DOWNSTREAM_BLOCKER: func_800BCF28
Aparece somente depois que o diagnóstico causal permite C2AB8 retornar.
NÃO investigar a fundo até o fix permanente estar pronto e o timing downstream
ser canônico o suficiente para root-cause. CLASSIFICAÇÃO: PROVEN (observado),
causa raiz UNKNOWN.
```

## 20. Escolha de design

### PREFERRED — Hybrid F (backward-branch safepoint + flag atômica + service não-bloqueante)

- COMPONENTS: N64Recomp (hook generator) + N64ModernRuntime (flag + service + IE/mask).
- MECHANISM: host event seta flag; backedge safepoint checa barato; service drena + check + swap gated por IE.
- SEMANTIC_FIDELITY: ALTA (preempção por prioridade preservada; máscara respeitada; sem timer/quantum que altere timing).
- PERFORMANCE: negative path = 1 atomic load + branch.
- CONCURRENCY_RISK: BAIXO (token único; padrão clear-antes-de-drenar).
- IMPLEMENTATION_COMPLEXITY: MÉDIA (2 repositórios; hook pequeno; service ~15 linhas; tracking IE/mask ~30 linhas).
- TESTABILITY: ALTA (harness existentes + casos da seção 18).
- EXPECTED_GLOBAL_IMPACT: BAIXO (só altera o que já seria servido em pontos cooperativos; agora em backedge).

### SECONDARY — Safepoint em backedge chamando check direto (sem flag dedicada)

- Variante: safepoint chama `service_preemption(rdram)` incondicional, que faz
  `try_dequeue` barato + check. Sem flag atômica. Mais simples, porém o negative
  path vira uma chamada + try_dequeue (um pouco mais caro que atomic+branch) e o
  runtime é chamado em TODO backedge. Aceitável, mas menos elegante que o flag.

### REJECTED

- C (force native preemption) — inviável/portável (estado guest no meio de instrução).
- D (poll em get_function) — não cobre loops puros.
- E (yield específico Tooie) — knowledge de jogo no runtime.
- yield_self_1ms por safepoint — dorme 1ms a cada backedge (inaceitável).

```text
RECOMMENDED_ARCHITECTURE:
    Hybrid F. Safepoint em backward branch (gerador) + flag atômica
    preemption_requested (runtime) + service_preemption_if_requested
    (runtime, não-bloqueante, com gate de interrupção). Interruptibilidade via
    IE de status_reg + máscara global rastreada em osSetIntMask/__osDisableInt/
    __osRestoreInt. CLASSIFICAÇÃO: INFERENCE (a validar em revisão adversarial).
```

## 21. Plano de patch (SEM código)

```text
N64ModernRuntime (librecomp/ultramodern):
- ultramodern/include/ultramodern/ultramodern.hpp
  - declarar extern std::atomic<bool> preemption_requested;
  - declarar void service_preemption_if_requested(RDRAM_ARG1);
  - declarar bool interrupts_enabled();
- ultramodern/src/mesgqueue.cpp
  - enqueue_external_message: setar preemption_requested.store(true, release)
    após o enqueue.
  - (novo) service_preemption_if_requested: clear flag -> dequeue_external_messages
    -> se interrupts_enabled() check_running_queue.
- ultramodern/src/threads.cpp (ou scheduling.cpp)
  - state global de máscara de interrupção + nesting (thread_local, pois IE é
    per-thread no guest; máscara global é global).
  - interrupts_enabled(): (status_reg & 0x1) && global_mask_on && depth==0.
- librecomp/src/ultra_translation.cpp
  - osSetIntMask_recomp/__osDisableInt_recomp/__osRestoreInt_recomp: deixar de ser
    no-op; rastrear máscara/depth; ao transicionar disabled->enabled, setar o flag
    e/ou check_running_queue para atender pendências (caso de teste 3).
- librecomp/src/recomp.cpp
  - cop0_status_write: honrar bit IE (remover assert para bits não-FR); continuar
    tratando FR.

N64Recomp (gerador):
- include/recompiler/generator.h
  - adicionar virtual void emit_preemption_check() const = 0;
- src/cgenerator.cpp + LiveRecomp/live_generator.cpp
  - implementar emit_preemption_check(): emitir
    "if (preemption_requested.load(std::memory_order_acquire))
        service_preemption_if_requested(rdram);"
    (ou chamada a uma helper inline).
- src/recompilation.cpp
  - no print_goto_with_delay_slot (e/ou na emissão de branch cujo target já foi
    emitido, i.e., backward): chamar generator.emit_preemption_check() antes do
    goto. Opcional: emitir também em emit_function_start.

TooieRecomp:
- nenhuma mudança (integração via rebuild do runtime + RecompiledFuncs regenerado).
```

## 22. Continuidade de documentação

- Criado: `docs/plan/evidence/p08-permanent-preemption-design.md` (este arquivo).
- Atualizados: `p08-session-handoff.md`, `docs/HANDOFF.md`, `plans/BT/08-rt64-input-and-frontend.md`.
- Specs: revisadas; NENHUMA mudança (nada de contrato implementado; apenas o
  requisito de preempção já PROVEN é documentado).

Classificações sumárias:

```text
PROVEN:    gap causal; priority rule; token único; chokepoint de eventos; componente D.
REFUTED:   force native preemption (C); hot-helper único (D); yield específico (E);
           1ms sleep por safepoint.
INFERENCE: Hybrid F preferido; backedge como safepoint mínimo; clear-antes-de-drenar;
           gate de IE; ordem FIFO; custo de performance.
UNKNOWN:   causa raiz de func_800BCF28; origem do objeto de table[2].
```

## 23. Specs

Revisadas: `recomp-readiness.md` (contrato de threads/preempção já descreve o gap
PROVEN — ok), `rsp-and-audio.md` (scheduler de áudio e causal já registrados — ok),
`build.md` e `matching.md` (sem relação com preempção — sem mudança).

Nenhuma spec recebeu contrato de implementação do fix (ainda é DESIGN).
A spec `recomp-readiness.md` já documenta corretamente o requisito original de
preempção (itens 1-4 do "Contrato de Threads e Preempção") como comportamento
N64 original PROVEN — mantido como está.

## 24. Validação

```text
markdown_vault_audit: executado (ver saída da sessão).
git diff --check:       executado (sem erros de whitespace).
CODE_CHANGED:           NO.
```

## 25. Commit

```text
docs(p08): design generic guest preemption fix
```

=== END DESIGN ===
