---
title: "P08 — adversarial architecture review of generic guest-preemption design"
description: "Revisão adversarial (Grok 4.6 HIGH) do Hybrid F proposto por DeepSeek V4 Pro. O esqueleto safepoint+hint+service+swap sobrevive; o modelo de interrupção, o protocolo do flag, a cobertura backedge-only e a API do gerador exigem revisão maior. Sem implementação."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, adversarial-review, n64modernruntime, n64recomp, libultra, safepoint, design]
---

# P08 — PREEMPTION DESIGN ADVERSARIAL REVIEW

## Meta

```text
MODE:            ADVERSARIAL ARCHITECTURE REVIEW (docs only; no runtime/game/generator code)
MODEL:           Grok 4.6 HIGH
ROLE:            Principal runtime architect / try-to-break reviewer
BASE_HEAD:       63d41a871b1708f4ff945a5df26ce17e3139529c
TOOIE_STATUS:    clean
RUNTIME_REPO:    /home/wesley/projetos/N64-Recomp/BanjoRecomp/lib/N64ModernRuntime
RUNTIME_HEAD:    ca568b6ad79b9029d14077f0c3ffa757727c5559
RUNTIME_STATUS:  dirty pré-existente NÃO resetado (threadqueue Fix B, threads
                 NULLPTR guard, N64Recomp JAL policy)
N64RECOMP_HEAD:  2b6f05688de2abc7d86da5b4a89b84c2c6acbabe-dirty
DESIGN_UNDER_REVIEW: docs/plan/evidence/p08-permanent-preemption-design.md
QUESTION:        Does Hybrid F emulate observable N64 scheduling well enough
                 for arbitrary recomp projects without new global bugs?
```

Classificação usada abaixo: `PROVEN` / `REFUTED` / `CORRECTED/SUPERSEDED` /
`INFERENCE` / `DISPUTED` / `UNKNOWN`.

O desenho DeepSeek **não é apagado**. Ele passa a
`CORRECTED/SUPERSEDED` por esta nota.

---

## 0. Continuity / worktree

```text
TOOIE_HEAD:     63d41a871b1708f4ff945a5df26ce17e3139529c
TOOIE_BRANCH:   main
TOOIE_DIFF:     empty
RUNTIME_HEAD:   ca568b6ad79b9029d14077f0c3ffa757727c5559
RUNTIME_DIFF:   ultramodern/src/threadqueue.cpp
                ultramodern/src/threads.cpp
                N64Recomp (pointer dirty; src/main.cpp + src/recompilation.cpp)
```

Fontes primárias lidas, não só a evidência DeepSeek:

- `ultramodern/src/{scheduling,mesgqueue,threads,events,timer}.cpp`
- `librecomp/src/{recomp,ultra_translation}.cpp`
- `N64Recomp/{include/recompiler/generator.h,src/cgenerator.cpp,src/recompilation.cpp,LiveRecomp/live_generator.cpp,include/recomp.h}`
- `banjo-tooie-Recomp/lib/ultralib/src/os/{setintmask.s,getintmask.s,interrupt.s,setglobalintmask.c}`
- `banjo-tooie-Recomp/lib/ultralib/include/PR/os_exception.h`
- `TooieRecomp/RecompiledFuncs/funcs_327.c` (`func_800C2AB8` gerado)
- BanjoRecomp `patches/scheduler_patches.c` (não é preempção guest)

Fatos herdados (§1 do prompt) **não reabertos**. `PROVEN`.

---

## 29. Verdict

```text
VERDICT: C — MAJOR REVISION REQUIRED
```

Hybrid F (safepoint gerado + hint atômico + service não-bloqueante +
`check_running_queue` / `swap_to_thread` existentes) é o esqueleto certo.
**Não é aprovável as-is.**

Buracos que quebram o desenho genérico:

1. `osSetIntMask` **não** é um boolean + nesting counter. `__osDisableInt`
   **não** é push/pop. `OS_IM_NONE` deixa `IE=1` e zera IM/RCP.
2. Backedge-only via `emit_goto` **não** cobre jump tables
   (`CGenerator::emit_case` emite `goto` cru; live usa jump indireto).
3. A justificação "C2AB8 não faz jal" é **falsa** — o C gerado chama
   `LOOKUP_FUNC(0x800C269C)` / `0x800C16A4`.
4. Clear-before-drain + `dequeue_external_messages` as-is **perde wakeup**
   quando `requeue_if_blocked` reenfileira sem setar o flag.
5. Service proposto não recebe `recomp_context*` e não consegue ler IE/IM.
6. `acquire` em todo backedge não é o mínimo correto.
7. Implementar máscara "de verdade" sem o contrato de retorno de
   `osSetIntMask` introduz regressão global.

```text
ROOT_CAUSE_PROVEN:           YES   (herdado; não reaberto)
READY_FOR_IMPLEMENTATION:    YES   (do revised design abaixo)
REMAINING_DISCRIMINATOR:     n/a
NEW_DOWNSTREAM_BLOCKER:      func_800BCF28
```

---

## 4. Interrupt model — red team

### Primary source

`setintmask.s` (libultra):

- `mfc0 SR`; devolve máscara **combinada** em `v0`:
  bits IM de `SR` (`OS_IM_CPU`) **OR** bits RCP lidos de
  `MI_INTR_MASK_REG<<16` **OR** bits "lógicos" escondidos por
  `__OSGlobalIntMask`.
- Escreve `MI_INTR_MASK_REG` via LUT `__osRcpImTable` (set/clr por classe
  SP/SI/AI/VI/PI/DP), AND com a metade RCP de `__OSGlobalIntMask`.
- Escreve **somente** `SR.IM` (bits 8–15). **Não toca `SR.IE`.**
- `jr ra` sem `mtc0` de IE.

`interrupt.s`:

```text
__osDisableInt (VERSION_J):
  save __OSGlobalIntMask IM snapshot
  mfc0 SR; SR &= ~IE; mtc0 SR
  v0 = old SR.IE
  if global IM changed: rewrite running thread SR IM, keep IE=0
  return v0                  // NÃO é push de nesting

__osRestoreInt:
  mfc0 SR; SR |= a0; mtc0 SR
  return                     // NÃO é pop; OR do bit salvo
```

`os_exception.h`:

```text
OS_IM_NONE = 0x00000001   // IE=1, todos os IM/RCP = 0
OS_IM_ALL  = 0x003fff01
OS_IM_AI   = 0x00040401   // IE + IM_AI + ...
```

Libultra áudio (`event.c`, `csplayer.c`, `synaddplayer.c`, …) faz
`mask = osSetIntMask(OS_IM_NONE); … osSetIntMask(mask);` em seções
críticas **curtas**. Isso **não** desliga IE.

### Estado atual do runtime — `PROVEN`

- `osSetIntMask_recomp` / `__osDisableInt_recomp` / `__osRestoreInt_recomp`
  são no-ops (`ultra_translation.cpp:121-129`). `v0` **não é escrito**.
- `cop0_status_write` trata só `FR`; qualquer outro bit (incl. IE/IM)
  → `assert` + `exit`.
- `ctx->status_reg` existe mas IE/IM **não são mantidos** pelos stubs.

### Resposta

```text
INTERRUPT_STATE_REQUIRED:
  Per-thread (vive no OSThread/ctx corrente):
    - SR.IE
    - SR.EXL, SR.ERL (tratar como 0 se o runtime não entra em exceção)
    - SR.IM[7:0]
  Global:
    - software MI mask (6 classes RCP)
    - __OSGlobalIntMask (default all-ones)
  NÃO: nesting depth counter
  NÃO: um único bool como estado canônico

IS_SIMPLE_INTERRUPTS_ENABLED_BOOL_SUFFICIENT: NO

WHY:
  1. osSetIntMask(OS_IM_NONE) deixa IE=1. Um bool "IE" autorizaria
     preempção DENTRO das CS de áudio libultra. Isso é um bug global novo.
  2. __osDisableInt devolve o IE anterior e __osRestoreInt faz OR; nesting
     counter diverge se o guest restaurar um valor intermediário.
  3. IM é por-thread (SR salvo em OSContext); MI e __OSGlobalIntMask são
     globais. Colapsar isso perde restore correto do valor de retorno.
  4. O predicado de PREEMPÇÃO pode *derivar* um bool, mas o estado
     armazenado tem de ser a máscara.

MINIMUM_STATE_FOR_PREEMPTION_DECISION:
  preemptible =
       (IE == 1)
    && (EXL == 0) && (ERL == 0)
    && ( (IM & global_cpu_mask) != 0
      || (mi_mask & global_rcp_mask) != 0 )
  Equivale a: IE && ((effective_os_int_mask & ~OS_IM_NONE) != 0).
```

`PROVEN` (fontes libultra). A redução DeepSeek a
`(status_reg & 1) && global_mask_on && depth==0` está `REFUTED`.

**Não implementar MI hardware de verdade.** Não há RCP. Basta estado
software que preserve o contrato `osSetIntMask`/`osGetIntMask` e o
predicado acima. Implementar máscara sem devolver o valor anterior em
`ctx->r2` é pior que o no-op atual.

---

## 5. osSetIntMask semantics

```text
OSSETINTMASK_MODEL:
  in:  a0 = OSIntMask combinado (IE implícito nos defines + IM CPU + RCP<<16)
  out: v0 = máscara anterior combinada (CPU IM | RCP | bits tapados pelo global)
  side:
    - atualiza software MI mask (classes RCP individuais)
    - atualiza SR.IM da thread corrente (AND __OSGlobalIntMask)
    - NÃO altera SR.IE
    - NÃO é per-thread para MI; É per-thread para IM
  preserve across guest swap: IM/IE via ctx da thread; MI global fica

DISABLE_RESTORE_MODEL:
  __osDisableInt:
    v0 = IE antigo; IE = 0; IM inalterado
  __osRestoreInt(a0):
    IE |= a0   (a0 deve ser 0 ou SR_IE)
    se a transição tornar preemptible && running_queue head.pri > self:
      check_running_queue()
  DISTINCT de osSetIntMask. Não é stack. Não é depth.
```

`PROVEN`.

---

## 6. Safepoint coverage — attack backedge-only

DeepSeek: "código MIPS finito + execução infinita ⇒ arco retroativo".
Isso é verdade **intra-procedimento no CFG estático**. Falso como
cobertura do **gerador atual**.

### C2AB8 não é um leaf spin — `PROVEN`

`RecompiledFuncs/funcs_327.c`:

```c
// inner, 60 slots
if (ctx->r18 != ctx->r21) { ctx->r17 = ADD32(...); goto L_800C2AFC; }
// outer, while pending
if (ctx->r19 != 0)        { ctx->r17 = ctx->r22 | 0; goto L_800C2AF4; }
// cada slot mode-3:
LOOKUP_FUNC(0x800C269C)(rdram, ctx);
LOOKUP_FUNC(0x800C16A4)(rdram, ctx);   // se ready
```

- Function-entry **cobriria Tooie**. A frase "C2AB8 não faz jal" está
  `REFUTED`.
- `get_function` poll (opção D) **também cobriria Tooie**. Continua
  insuficiente como solução genérica (spin leaf sem jal).

### Contraexemplos de backedge-only via `emit_goto`

| caso | por que fura Hybrid F |
| --- | --- |
| Jump table / switch loop | `CGenerator::emit_case` emite `case N: goto L; break;` **sem** `emit_goto`. Live: `emit_switch` faz `sljit_emit_ijump`; `emit_case` é no-op. |
| JR não-jtbl | gerador trata como tail-call + `return` (recursão C, não goto). Não é backedge. Estoura pilha. Limitação pré-existente. |
| Recursão mútua A→B→A | sem arco retroativo intra-função; pilha C cresce. Prático: crash, não hang. |
| `b` para si | já vira `emit_pause_self()` — coberto por outro caminho. |
| Bloco reto enorme | finito; não é execução infinita. |
| Live vs static | detecção "label já emitido" existe em `LiveGenerator::emit_goto`; **não** existe em `CGenerator::emit_goto` (é só `goto`). A lógica tem de viver em `recompilation.cpp`. |

```text
BACKEDGE_ONLY_COVERS_ALL_INFINITE_GUEST_EXECUTION: NO
```

### Comparação

| esquema | veredito |
| --- | --- |
| A backedge estático só | insuficiente (jtbl + live ijump) |
| B function entry + backedge | cobre C2AB8 duas vezes; ainda fura jtbl |
| C toda aresta de CF | correto, caro |
| D orçamento por BB | correto, mais intrusivo |
| E budget periódico | last-resort; introduz quantum |
| **F' híbrido revisto** | **backedge estático + safepoint antes de `emit_switch` + opcional function entry** |

`INFERENCE` (cobertura); o furo do `emit_case` é `PROVEN`.

---

## 7. Safepoint vs delay slot

O gerador **já** materializa o delay slot **antes** do `goto`:

```cpp
// recompilation.cpp print_goto_with_delay_slot / print_branch
process_delay_slot(...);
generator.emit_goto(target);
```

C2AB8 gerado confirma: o `addiu` do delay está **dentro** do `if` e
**antes** do `goto`. Branch-likely (`bnel`/`beql`) só executa o slot se
taken — o C gerado já faz isso.

MIPS não aceita interrupção entre branch e delay slot; o par completa.
EPC com BD=1 re-executa o par. Portanto o único ponto auto-consistente
no C gerado é **depois do delay slot, imediatamente antes do goto**
(ou no destino, depois da transferência).

```text
SAFEPOINT_POSITION_RELATIVE_TO_DELAY_SLOT:
  AFTER delay slot, BEFORE generated goto (taken path).
  Equivalent: at destination label after transfer.
  NEVER between branch decision and delay slot.
  NEVER before the branch instruction.
```

Preemptar entre branch e delay slot alteraria o estado visível do slot
(memória/registros escritos pela outra thread). No N64 isso não ocorre.
`PROVEN` pela forma do gerador + semântica MIPS.

---

## 8. recomp_context consistency

`recomp_context` (`recomp.h`): GPRs, FPRs, `hi`/`lo` **no struct**,
`status_reg`, `mips3_float_mode`.

`emit_function_start` emite **cópia local**:

```c
uint64_t hi = 0, lo = 0, result = 0;
int c1cs = 0;
```

Comentário do gerador: *"shouldn't need to be preserved across function
boundaries"*. GPRs/FPRs são `ctx->rN` / `ctx->fN` por instrução. HI/LO
vivos são as **locals C**, não `ctx->hi/lo`.

O swap atual **estaciona o `std::thread` nativo**
(`resume_thread_and_wait` → `LightweightSemaphore::wait`). Pilha C,
registros nativos e locals sobrevivem. Por isso, **neste modelo de
token**, não é preciso flush extra de HI/LO para `ctx` no safepoint.

```text
CONTEXT_SAFE_AT_BACKEDGE: CONDITIONAL

YES if and only if:
  - safepoint is after delay slot
  - swap continues to park the native thread (not a stackless ctx-switch)
  - service is a real C call (forces spill of any cached ctx->rN)
  - service receives ctx if it must read SR

MUST document the invariant: a future stackless scheduler would have to
materialize hi/lo/c1cs into recomp_context before parking.
```

Custo extra de flush: **zero** no modelo atual. A estimativa DeepSeek
não explode. `PROVEN` (modelo de thread) / `INFERENCE` (spill do ABI).

---

## 9. Atomic flag memory ordering

A fila `moodycamel::BlockingConcurrentQueue` **já sincroniza**
enqueue → `try_dequeue`. O flag/epoch é só um **hint** para não chamar
`try_dequeue` em todo backedge.

```text
FLAG_STORE_ORDER:  release   (depois do enqueue; ou o próprio fetch_add)
FLAG_LOAD_ORDER:   relaxed   (hint). NÃO acquire em todo backedge.
WHY:
  - ver a mensagem: o try_dequeue no slow path já faz o acquire
  - acquire em todo backedge é no-op de barreira no x86 e caro no ARM
  - relaxed atomic load não pode ser cacheado pelo compilador
  - liveness: cache-coherence torna o hint visível; o próximo
    ponto cooperativo (osSend/Recv) é fallback
```

`acquire` everywhere: `REFUTED` como necessidade. x86 esconde o custo;
o runtime é multi-arch.

---

## 10. Lost-wakeup protocol

Clear-before-drain **não** basta com o `dequeue_external_messages` atual.

```cpp
// mesgqueue.cpp — PROVEN
while (external_messages.try_dequeue(to_send)) {
    if (!do_send(...) && to_send.requeue_if_blocked)
        requeued_messages.push_back(to_send);
}
for (auto& m : requeued_messages)
    external_messages.enqueue(m);   // NÃO seta flag
```

Produtores com `requeue_if_blocked=true`: SP, DP, SI, timer, PI, flash.
VI/AI usam `false` (drop se cheio).

Corrida:

```text
T1: flag = false
T1: drain; do_send falha (mq cheia); requeue sem flag
T1: sai do service
T2: ninguém mais enfileira
guest gira para sempre; mensagem requeued invisível
```

No caminho cooperativo atual isso é mascarado porque o guest **já está**
dentro de `osSendMesg`/`osRecvMesg` e volta a drenar. No safepoint de
spin, é lost wakeup genérico. `PROVEN`.

```text
PREEMPTION_FLAG_PROTOCOL:

  // producer (host):
  external_messages.enqueue(msg);
  work_epoch.fetch_add(1, memory_order_release);

  // consumer fast path (generated):
  if (work_epoch.load(relaxed) == tls_last_seen) return;

  // service (game thread only):
  for (;;) {
      uint64_t seen = work_epoch.load(acquire);
      dequeue_external_messages(rdram);          // may requeue
      if (cpu_preemptible(ctx) && higher_pri_runnable())
          swap_to_thread(...);                   // parks; resumes here
      tls_last_seen = seen;
      if (work_epoch.load(relaxed) == seen)
          break;                                 // no publish during service
  }

  // requeue path MUST also publish:
  dequeue_external_messages:
      if (any_requeued) work_epoch.fetch_add(1, release);

  // IE/mask enable transition:
  __osRestoreInt / osSetIntMask / cop0_status_write
      if (became_preemptible) check_running_queue(rdram);

PROVEN_NO_LOST_WAKEUP: YES  (revised protocol)
                       NO   (DeepSeek clear-before-drain as specified)
```

Bool+`exchange(false, acq_rel)` + recheck + publish-on-requeue também
fecha. Epoch perde menos corridas de "já era true, skip store".

---

## 11. External message service policy

Drain-all é **exatamente** o que `osSendMesg`/`osRecvMesg` já fazem.
Inconsistência entre safepoint e ponto cooperativo seria um bug novo.

Riscos reais de drain-all (não inventados):

- Colapsa VI+AI+SP+timer num único instante (no N64 cada um é uma
  exceção). Pode acordar várias threads e só então `check_running_queue`
  uma vez.
- Não reordena *dentro* de um produtor (FIFO per-producer).
- Pode adiantar entregas relativamente ao hardware.
- Não é starvação do current se o predicado de swap for `pri >`.

```text
RECOMMENDED: B — drain ALL pending, then ONE scheduling decision
  (same as existing cooperative points).

REJECT as v1 default:
  A one-message (subótimo; adia wakeup)
  E priority-aware dequeue (a fila host não tem pri)
  F timestamp/epoch select (overkill)

KEEP as regression valve:
  D drain-until-preemption-decision
    (para se drain-all colapsar timing num jogo específico)
```

`INFERENCE`. Compatível com o código atual: `PROVEN`.

---

## 12. Message ordering

`BlockingConcurrentQueue` é MPMC. Ordem é **FIFO por produtor**, não
ordem total entre VI, AI, SP, DP, SI, PI, timer, flash. Publicação
concorrente já existe hoje; só aparece em `osSend`/`osRecv`.

N64 tem ordem de exceção / IPL, não um enqueue multi-core.

```text
MULTIPRODUCER_ORDERING_RISK:
  YES — safepoints tornam visível, com mais frequência e em PCs
  diferentes, o nondeterminismo que o caminho cooperativo já tem.
  Não é um bug novo de ordem; é exposição maior.
  N64 original também é wall-clock / interrupt driven.
```

`PROVEN` (tipo da fila) / `INFERENCE` (impacto).

---

## 13. Who sets the hint?

```text
FLAG_MEANING: "async external work published"
              NÃO "preempt now"
              NÃO "higher-priority thread is runnable"
```

Todo `enqueue_external_message` publica. O safepoint decide entrega vs
swap. Produtor **não** deve analisar máscara, pri, ou "esse evento
acorda alguém" — ele não tem o token nem o mq guest.

Otimização "só setar se o evento pode acordar" está `REFUTED` para v1
(precisa do estado guest sob race).

---

## 14. Interrupts-disabled case

DeepSeek: drenar sempre, só gatear o swap. Isso é **opção B**.

No N64, com IE=0 ou IM=0 o handler **não roda** e a mensagem **não é
postada** na mq guest. Fiel seria A (deixar na fila host).

Mas os pontos cooperativos atuais **já fazem B** (drenam sem olhar
máscara, porque a máscara é no-op). Mudar safepoint para A e deixar
`osRecvMesg` em B é inconsistente.

```text
V1:
  B — deliver into guest mq even if not preemptible
  do NOT swap unless cpu_preemptible(ctx)
  do NOT need a second flag
  on disable→enable: check_running_queue immediately
       (a thread acordada já está na running_queue)

FUTURE FIDELITY (not v1):
  A — leave host-queue items undelivered while !preemptible
```

`INFERENCE` (escolha v1) justificada por paridade com o runtime atual.

---

## 15. Priority predicate

DeepSeek: `IE && !empty(running) && peek.pri > self.pri`.

Insuficiente.

```text
PREEMPTION_PREDICATE:
  is_game_thread
  && self != NULL && self->state == RUNNING
  && cpu_preemptible(ctx)          // §4, NOT IE-only
  && !thread_queue_empty(running_queue)
  && peek(running_queue)->priority > self->priority

SAME PRIORITY:     no preemption (libultra already)
LOWER PRIORITY:    stays queued
MULTIPLE HIGHER:   peek is highest (queue is pri-sorted) — OK
osSetThreadPri:    already reinserts + check_running_queue
  (dirty NULLPTR guard INTERACTS — see §27)
THREAD DESTROYED:  existing wait_for_resumed / null context
PRIORITY CHANGE DURING DRAIN: check AFTER drain, using post-drain queue
CURRENT NO LONGER RUNNING: do not swap from a non-running self
```

`check_running_queue` já implementa o comparador `>`. Falta só o gate
`cpu_preemptible`. `PROVEN` + `CORRECTED`.

---

## 16. Preemption vs event delivery

São dois passos, uma função no caminho quente:

```text
service_preemption_if_requested(rdram, ctx):
    1. service_external_events()     // always, if epoch moved
    2. maybe_preempt(ctx)            // predicate §15
```

Não dois símbolos públicos no gerador. Entrega sem wakeup de pri maior
ainda precisa acontecer (a mq guest tem de ver o evento; a thread
current pode ser quem espera no próximo `osRecvMesg`).

`INFERENCE`.

---

## 17. Native thread safety

`swap_to_thread` no backedge:

| preocupação | veredito |
| --- | --- |
| locks do runtime | generated code não segura mutex; `message_mutex` é só no host |
| `thread_local thread_self` / `is_game_thread` | por native thread; correto |
| dono do RDRAM | token único; só o holder chega ao safepoint |
| pilha C++ | park do `std::thread`; locals de `service` sobrevivem |
| sinais | não usados |
| reentrância | o outro guest é **outro** native thread; ao retomar, `service` volta |
| `do_send(..., block=false)` | dequeue nunca chama `run_next_thread_and_wait` |

```text
SAFE at arbitrary backedge IFF called only from a game thread that
holds the execution token (true of all generated code).
```

`PROVEN` pelo modelo atual. Inseguro se alguém chamar o service de
thread host (VI/AI). A API deve `assert(is_game_thread())`.

---

## 18. Game/runtime reentrancy

Safepoint no meio de uma operação guest de várias funções, com
interrupções habilitadas: no N64 isso **também** é preemptível.
Isso é preempção legal, não unsafety do host.

Unsafe para o host = locks/invariantes C++ (§17). Legal guest =
qualquer ponto com `cpu_preemptible`. Não confundir.

`INFERENCE`.

---

## 19. Performance

C2AB8 quente (freeze):

- backedge interno: até 60 / passada
- backedge externo: 1 / passada
- passadas: o mais rápido que o host conseguir

Ordem de grandeza: 10⁵–10⁷ loads atômicos/s no spin. Depois do fix o
spin dura ~1 período de AI/VI.

Gameplay normal: a maior parte do tempo já está em `osRecvMesg` (VI).
Loops quentes (mtx, anim) geram dezenas de milhares de backedges/frame,
não milhões.

```text
FAST_PATH_RECOMMENDATION:
  TLS last_seen_epoch vs atomic<uint64_t> work_epoch relaxed load
  + branch. No acquire. No clock. No mutex.
  Producer fetch_add(release) after enqueue (cacheline bounce at 60 Hz
  VI/AI is acceptable; bounce during a spin is short-lived).

EXPECTED_PERFORMANCE_RISK: LOW   (revised relaxed/epoch)
                           MEDIUM (DeepSeek acquire-everywhere, ARM)
```

`INFERENCE`.

---

## 20. Boolean vs epoch

```text
SELECT: EPOCH (atomic<uint64_t> work_epoch + TLS last_seen)

vs bool:
  + múltiplos publishes colapsam sem clear race
  + produtor nunca precisa "set if not already set"
  + recheck `epoch != seen` fecha lost wakeup
  + overflow 64-bit irrelevante
  = mesmo cacheline bounce
  − um load+compare a mais no fast path (barato)

Bool+exchange+recheck é aceitável se alguém insistir em menos estado.
Epoch é o default.
```

`INFERENCE`.

---

## 21. Generator API

**Não** espalhar a string
`if (preemption_requested.load(acquire)) service...` em C e SLJIT.

```text
generator.h  Generator:
  virtual void emit_preemption_safepoint() const = 0;

CGenerator::emit_preemption_safepoint:
  "if (ultramodern_preemption_hint()) "
  "    ultramodern_service_preemption(rdram, ctx);\n"

LiveGenerator::emit_preemption_safepoint:
  same call via sljit (load hint, branch, call)

REQUEST SITE (architecture-independent) — recompilation.cpp:
  1. print_goto_with_delay_slot / print_branch:
       after process_delay_slot, if target_vram <= instr_vram:
         emit_preemption_safepoint();
       emit_goto(...)
  2. JR jump-table path, immediately before emit_switch:
       emit_preemption_safepoint();
  3. optional: emit_function_start (belt; not required for C2AB8)

DO NOT hook only CGenerator::emit_goto
  (live and emit_case would diverge).
DO NOT put "already emitted" logic only in LiveGenerator::emit_goto.
```

`emit_pause_self` já cede; não precisa de safepoint extra no `b` para
si.

`INFERENCE` (API) / `PROVEN` (onde o gerador emite goto/switch).

---

## 22. Live recomp

DeepSeek lista `live_generator.cpp` no patch plan. Bom. Insuficiente:

- Live `emit_goto` já sabe se o label existe (backedge detector
  natural) mas o pedido tem de vir de `recompilation.cpp` para os dois.
- Live jump tables **não** passam por `emit_goto` nem por `emit_case`
  útil. Sem safepoint antes de `emit_switch`, live fura o que static
  cobriria só com `emit_case`.

```text
STATIC_LIVE_PARITY_RISK:
  HIGH if safepoint is implemented only in CGenerator::emit_goto
       or only via "label already emitted" in one backend.
  LOW  if recompilation.cpp emits the virtual safepoint on
       backward intra-func transfer AND before emit_switch.
```

`PROVEN` (dois backends divergem hoje na emissão de switch).

---

## 23. Configurability

Projetos existentes (BanjoRecomp incluso) dependem do comportamento
cooperativo. Ligar preempção **é** a semântica N64, mas muda
interleavings e pode revelar races hóspedes que o coop escondia.

BanjoRecomp `scheduler_patches.c` é freeze-bit do RDP, não preempção
guest. Nenhum outro recomp local mostra o hang C2AB8. Isso **não**
prova ausência de spins em Zelda/MM/etc.

```text
MIGRATION:
  build/runtime flag ultramodern::guest_preemption_safepoints
    default ON (correct N64)
    OFF = current cooperative-only (bisect / regressão)
  Generator always emits the call; runtime no-ops if flag off
    (avoids two generated corpuses).
  Do not default OFF and leave Tooie broken.
```

`INFERENCE`.

---

## 24. Determinism

Eventos host já são wall-clock (thread VI, timer). Safepoints não
introduzem uma fonte nova de tempo; só **observam mais cedo**.

Ordem por produtor permanece. Ordem entre produtores já não é
determinística.

```text
DETERMINISM_IMPACT:
  Interleavings guest mudam (mais cedo / mais frequente).
  Isso é a correção, não uma regressão de "determinismo N64".
  Não há como preservar o escalonamento coop e o N64 ao mesmo tempo.
```

`INFERENCE`.

---

## 25. Other games

Localmente só Tooie + BK. BK não tem o hang C2AB8 documentado. O patch
de scheduler do BK é outro bug (DPC freeze).

O padrão "MAIN baixa pri em loop puro / quase-puro + AUDIO alta pri
bloqueada em `osRecvMesg` + evento host due" é **genérico** em jogos
libultra. O fix revisto ajuda; o Hybrid F as-is arrisca CS de áudio
(`osSetIntMask(OS_IM_NONE)`).

`INFERENCE`.

---

## 26. Test plan (red team)

Além dos 7 itens DeepSeek, **obrigatório**:

1. LOW pri spin leaf **sem jal** + HIGH pri async wake
2. LOW pri spin **com jal** (C2AB8-shaped) + HIGH pri wake
3. IE=0 via `__osDisableInt` → evento chega → sem swap → restore → swap
4. `osSetIntMask(OS_IM_NONE)` → evento chega → **sem swap** (IE ainda 1!)
   → restore mask → swap
5. `osSetIntMask` devolve máscara anterior e restore reabre a mesma
6. same-pri wake: sem preempção
7. lower-pri wake: sem preempção
8. dois waiters de pri alta: o de maior pri roda primeiro
9. publish durante `exchange`/`epoch` clear
10. publish durante `swap_to_thread`
11. rajada VI+AI
12. mq cheia + `requeue_if_blocked` **sem** novo publish host
    (o caso que quebra clear-before-drain)
13. thread exit / destroy no alvo do swap
14. `osSetThreadPri` no current e num queued durante drain
15. shutdown / pause / resume
16. static e live: mesmo predicado num loop jtbl e num bne backward
17. flag/epoch OFF: comportamento idêntico ao HEAD atual
18. Fix B: double-wake do mesmo OSThread durante o novo caminho
19. `__osDisableInt` aninhado save/restore (não counter):
    `s1=disable; s2=disable; restore(s2); restore(s1)`
20. MTC0 Status alterando IE/IM não pode mais `exit()`
21. Tooie clean: intro → C2AB8 retorna → attract avança
    (SIGSEGV `func_800BCF28` **esperado** até o blocker seguinte)

---

## 27. Pre-existing runtime dirty state

| dirty | relação com preempção |
| --- | --- |
| threadqueue Fix B (scan de links, recusa insert duplicado, remove por pred) | **INTERACTS / praticamente REQUIRED.** Safepoints aumentam wake-twice (VI+timer no mesmo OSThread). Sem Fix B, `next` pode ciclar. |
| `osSetThreadPri` `queue != NULLPTR` | **INTERACTS.** Pri mutation no caminho novo. Não é requisito do safepoint. |
| N64Recomp `use_lookup_for_all_function_calls` + same-section bind | **INDEPENDENT** semanticamente. **INTERACTS** no ficheiro: o hook vive em `recompilation.cpp`, o mesmo do dirty JAL. Não misturar os diffs. |

```text
DEPENDENCY: INTERACTS
```

Não desenhar contra o dirty sem o documentar. Não resetar. Não commitar
o dirty neste trabalho.

---

## 28. Downstream

```text
NEW_DOWNSTREAM_BLOCKER = func_800BCF28
```

Não investigado. O fix permanente de scheduling vem primeiro.

---

## 30. Revised architecture

```text
EVENT_NOTIFICATION:
  every enqueue_external_message:
      enqueue then work_epoch.fetch_add(1, release)
  requeue_if_blocked path also publishes

SAFEPOINT_SCHEME:
  F' = intra-function backward transfer
     + safepoint before jump-table emit_switch
     + optional function entry
     + runtime flag to no-op

SAFEPOINT_PLACEMENT:
  after delay slot, before generated goto, when target_vram <= instr_vram
  and immediately before emit_switch
  never between branch and delay slot

FAST_PATH:
  if (work_epoch.load(relaxed) == tls_last_seen) ; // fall through
  else ultramodern_service_preemption(rdram, ctx);

INTERRUPT_STATE:
  software: per-thread IE + IM (in ctx->status_reg)
            global MI mask + __OSGlobalIntMask
  cpu_preemptible = IE && !EXL && !ERL && (IM or MI unmasked)
  osSetIntMask implements full return+mask contract, not a bool
  __osDisableInt/__osRestoreInt are IE save/restore, not a counter
  cop0_status_write accepts IE/IM/EXL/ERL; keeps FR special-case

EVENT_SERVICE_POLICY:
  drain all pending FIFO (existing dequeue_external_messages)
  then one check_running_queue
  deliver even if !preemptible (match current coop points)
  do not swap if !cpu_preemptible
  enable-transition calls check_running_queue

PREEMPTION_PREDICATE:
  see §15

THREAD_SWAP:
  unchanged swap_to_thread / resume_thread_and_wait
  assert is_game_thread()
  Fix B remains the queue invariant

N64RECOMP_CHANGES:
  generator.h: emit_preemption_safepoint()
  cgenerator.cpp + live_generator.cpp: implement
  recompilation.cpp: request at backward goto + before emit_switch
  do not touch the dirty JAL policy hunk except to avoid merge conflict

RUNTIME_CHANGES:
  ultramodern.hpp: epoch, hint, service, cpu_preemptible
  mesgqueue.cpp: publish on enqueue + on requeue; service wrapper
  ultra_translation.cpp: real osSetIntMask / Disable / Restore
  recomp.cpp: cop0_status_write IE/IM
  scheduling.cpp: gate check_running_queue with cpu_preemptible
                  OR keep check pure and gate only in service
                  (prefer: keep check pure; gate in service + RestoreInt)

TOOIERECOMP_CHANGES:
  none (rebuild runtime + regenerate RecompiledFuncs after N64Recomp)
```

---

## 31. Implementation readiness

```text
READY_FOR_IMPLEMENTATION: YES
```

Não há discriminador experimental em aberto para o *desenho*. O
experimento causal já mostrou que uma oportunidade de scheduler resolve
C2AB8. O que faltava era um desenho genérico que não inventasse um
modelo de interrupção falso.

### Patch plan (docs only here; next chat implements)

1. Runtime: epoch + publish-on-enqueue + publish-on-requeue + service
   with recheck. **Sem** gerador ainda: harness chama service à mão.
2. Runtime: `cpu_preemptible` + `__osDisableInt`/`__osRestoreInt` corretos.
3. Runtime: `osSetIntMask`/`osGetIntMask` com valor de retorno; testes
   `OS_IM_NONE`.
4. `cop0_status_write` deixa de matar o processo em IE/IM.
5. N64Recomp: `emit_preemption_safepoint` + hooks em `recompilation.cpp`
   (static **e** live). Diff separado do dirty JAL.
6. Regenerar `RecompiledFuncs` Tooie; flag ON.
7. Tooie clean run: C2AB8 retorna; parar no SIGSEGV `func_800BCF28`.

### Test order

Harness 1–20 (§26) **antes** do run Tooie. Tooie é o teste 21, não o
primeiro.

---

## 32. Documentation continuity

| nota | ação |
| --- | --- |
| esta | criada |
| `p08-permanent-preemption-design.md` | marcado `CORRECTED/SUPERSEDED` (histórico preservado) |
| `p08-session-handoff.md` | NEXT = implementar o revised design |
| `docs/HANDOFF.md` | apontar esta revisão |
| `plans/BT/08-rt64-input-and-frontend.md` | residual atualizado |
| specs | só contrato já PROVEN; nada de design como comportamento feito |

```text
PROVEN:     gap causal; token único; chokepoint enqueue; C2AB8 tem jal;
            delay slot antes do goto; emit_case fura emit_goto;
            osSetIntMask combina CPU+RCP e não toca IE;
            Disable/Restore são IE save/restore;
            requeue não publica flag;
            OS_IM_NONE = 0x1
REFUTED:    C2AB8 sem jal; nesting counter; IE-only bool;
            acquire obrigatório; clear-before-drain as-is sem requeue publish;
            backedge-only via emit_goto cobre todo spin
CORRECTED/SUPERSEDED: Hybrid F DeepSeek → Hybrid F' desta nota
INFERENCE:  epoch vs bool; drain-all v1; relaxed load; function-entry opcional
DISPUTED:   (none remaining vs DeepSeek after this note)
UNKNOWN:    causa de func_800BCF28; origem do objeto table[2]
```

---

## 33. Specs

Revisadas: `recomp-readiness.md`, `rsp-and-audio.md`, `build.md`,
`matching.md`.

- `build.md` / `matching.md`: sem relação. Sem mudança.
- `recomp-readiness.md`: contrato 1–4 do N64 **já PROVEN** permanece.
  Atualizar só a frase "design em projeto" → "design revisto
  adversarialmente; não implementado".
- `rsp-and-audio.md`: ponteiro para esta revisão. Sem contrato novo.

Nada de Hybrid F/F' como comportamento entregue.

---

## 34–35. Vault / commit

```text
markdown_vault_audit:  (run at end of session)
git diff --check:      (run at end of session)
CODE_CHANGED:          NO
COMMIT:                docs(p08): adversarially review preemption design
```

=== END ADVERSARIAL REVIEW ===
