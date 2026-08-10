---
title: "P08 — Fix B threadqueue: validação de contrato e segundo bug do scheduler"
description: "Validação do fix B em threadqueue.cpp (scan real de links, sem field-equality guard). Contrato de thread_queue_remove provado: HEAD PASS, NON-HEAD/TAIL FAIL (infinite loop). SECOND SCHEDULER BUG CONFIRMED: remove só checa o HEAD. NO_THREADS_LEFT FIXED (0/2 smoke). Novo blocker: SIGSEGV em thread_queue_insert com queue==NULLPTR via osSetThreadPri sobre thread RUNNING (JOY)."
status: evidence
phase: P08
updated: 2026-08-10
tags: [banjo-tooie-recomp, p08, threads, scheduler, threadqueue, fix-b, remove-bug, smoke]
---

# P08 — Fix B threadqueue: validação

## Decisão

Fix B (Grok review aprovado): remover o guard `if (toadd->queue == queue_)`
em `thread_queue_insert`; manter o scan real de links anti-duplicata que parte
do HEAD da fila (antes do loop de prioridade). NÃO limpar `t->queue` em
`thread_queue_remove` (A descartado: `osSetThreadPri` depende do campo para
reinserir).

## N64MODERNRUNTIME BASE

- base: `ca568b6ad79b9029d14077f0c3ffa757727c5559`
- arquivo: `../BanjoRecomp/lib/N64ModernRuntime/ultramodern/src/threadqueue.cpp`
- upstream remove (`a2f201d`, PR #87) só consertou unlink do head; o walk
  sempre usa `queue_to_ptr(queue_)` — ver SECOND SCHEDULER BUG.

## LOCAL PATCH BEFORE (estado dirty pré-B)

Em `thread_queue_insert`: guard de igualdade de campo + scan de links.

```cpp
    debug_printf("[Thread Queue] Inserting thread %d into queue 0x%08X\n", toadd->id, (uintptr_t)queue_);
    // A thread can be woken by two host-side event sources before the first
    // wakeup has run. Inserting it into the same queue twice makes next point
    // back to itself and permanently wedges the cooperative scheduler.
    if (toadd->queue == queue_) {
        debug_printf("  Thread %d is already in queue 0x%08X\n", toadd->id, (uintptr_t)queue_);
        return;
    }

    // The queue field is cleared when a thread starts running, so it is not
    // sufficient on its own to detect a stale link left in the run queue.
    // Scan the links as well before mutating them.
    PTR(OSThread)* scan = cur;
    for (uint32_t hops = 0; *scan != NULLPTR; hops++) {
        if (*scan == toadd_) {
            debug_printf("  Thread %d is already linked in queue 0x%08X\n", toadd->id, (uintptr_t)queue_);
            return;
        }
        if (hops == 255) {
            debug_printf("  Refusing to insert into malformed queue 0x%08X\n", (uintptr_t)queue_);
            return;
        }
        scan = &TO_PTR(OSThread, *scan)->next;
    }
```

## LOCAL PATCH AFTER (fix B)

```cpp
    debug_printf("[Thread Queue] Inserting thread %d into queue 0x%08X\n", toadd->id, (uintptr_t)queue_);

    // A thread can be woken by two host-side event sources before the first
    // wakeup has run. Inserting it into the same queue twice makes next point
    // back to itself and permanently wedges the cooperative scheduler.
    // Membership is decided by the REAL links in the queue, not by the queue
    // field: the field is a reinsert handle and stays stale after remove, so
    // checking it would wrongly reject a legitimate osSetThreadPri reinsert.
    // Scan the links as well before mutating them.
    PTR(OSThread)* scan = cur;
    for (uint32_t hops = 0; *scan != NULLPTR; hops++) {
        if (*scan == toadd_) {
            debug_printf("  Thread %d is already linked in queue 0x%08X\n", toadd->id, (uintptr_t)queue_);
            return;
        }
        if (hops == 255) {
            debug_printf("  Refusing to insert into malformed queue 0x%08X\n", (uintptr_t)queue_);
            return;
        }
        scan = &TO_PTR(OSThread, *scan)->next;
    }
```

Diff final (base->after): apenas o field-guard removido + comentário atualizado.
Confirmação: `thread_queue_remove`, `thread_queue_pop`, `osSetThreadPri`,
`scheduling.cpp`, `mesgqueue.cpp` INALTERADOS.

## Teste de contrato (harness nativo)

`/tmp/opencode/tq_contract.cpp` + stub `tqstub/`; inclui o `threadqueue.cpp`
real com cast fiel de `cur` (o tipo `PTR(PTR(OSThread))` vs `OSThread*` só
compila como int32 no modo recomp; cast preserva a semântica do loop).

| Caso | Inicial | Ação | Esperado | Real | Resultado |
|---|---|---|---|---|---|
| HEAD REMOVE | A->B->C | remove(A) | B->C | head=B next=C | **PASS** |
| NON-HEAD REMOVE | A->B->C | remove(B) | A->C | timeout 124 (loop) | **FAIL** |
| TAIL REMOVE | A->B->C | remove(C) | A->B | timeout 124 (loop) | **FAIL** |
| ABSENT REMOVE | A->B->C | remove(D) | false | timeout 124 (loop) | **FAIL** |
| CASE A dup | RQ JOY->TIDY->IDLE | insert(JOY) | rejeitado, 1x, sem loop | 1x, selfloop=0 | **PASS** |
| CASE B stale field | X.queue=RQ, não linkado | insert(X) | permitido 1x | count=1 | **PASS** |

## SECOND SCHEDULER BUG (CONFIRMED)

`thread_queue_remove`:

```cpp
    PTR(PTR(OSThread)) cur = queue_;
    while (cur != NULLPTR) {
        PTR(OSThread)* cur_ptr = queue_to_ptr(PASS_RDRAM queue_);
        if (*cur_ptr == t_) { *cur_ptr = TO_PTR(OSThread, *cur_ptr)->next; return true; }
        cur = TO_PTR(OSThread, *cur_ptr)->next;
    }
    return false;
```

`cur` avança, mas `queue_to_ptr` SEMPRE recebe `queue_` (o head). Resultado:

- HEAD: remove e retorna true.
- NON-HEAD/TAIL/ABSENT em fila com head->next != NULL: `cur` fica preso no
  mesmo valor (head->next) → **infinite loop**.
- NON-HEAD com fila de head único (`head->next == NULL`): o loop termina e
  retorna false (é o que salvou o run real dos 5 casos).

Presente no UPSTREAM (não é patch local). Fora do escopo desta etapa — NÃO
corrigido.

## Smoke (após fix B)

Rebuild completo `cmake --build build --parallel` → PASS.
Execução: `timeout 60 env TOOIE_AUTO_START=1 TOOIE_DIAG=1
TOOIE_DISABLE_LAUNCHER_MUSIC=1 ./build/TooieRecompiled`.

- NO_THREADS_LEFT: **0 ocorrências** (2 runs; exit 139 = SIGSEGV)
- RUNNING_QUEUE EMPTY: não observado (sem throw do scheduler)
- SELF_LOOP: não observado
- DUPLICATE THREAD: não observado
- PRIORITY ORDERING: scan mantém inserção por prioridade (head->priority >
  toadd)
- GCSECTION REGRESSION: NÃO (last_ovl=677:3 no diag; gcsection:4 passou)
- LOOKUP_FUNC(0): 0 ocorrências

## Novo blocker: SIGSEGV em thread_queue_insert (queue==NULLPTR)

Trace GDB (`gdb_b_trace.gdb`), 21 chamadas a `thread_queue_remove`:

- REMOVE CALLS TOTAL: 21
- REMOVE queue==0 (running/stale): 1
- REMOVE target era HEAD: 15
- REMOVE target NON-HEAD (fila válida): 5

Crash (`gdb_b_crash.txt`):

```
#0 thread_queue_insert
#1 osSetThreadPri
#2 func_8001608C
#3 func_800123F4
#4 func_800124EC
RDI(rdram)=0x7ffec4400000 RSI(queue)=0x0 RDX(toadd)=0x80079ba8 (JOY)
TOADD id=7 pri=0 queue=0x00000000 next=0x800775f0
```

`osSetThreadPri` (threads.cpp:314-315) faz `remove(t->queue)` + `insert(t->queue)`
para alvo `t != this_thread() && state != STOPPED`. Quando JOY está RUNNING
(`pop()` zerou `t->queue`, `next` stale), o guard não exclui RUNNING →
`remove(NULLPTR)` retorna false imediato → `insert(NULLPTR)` faz
`queue_to_ptr(0)` = `TO_PTR(PTR(OSThread), 0)` → offset enorme fora do rdram →
SIGSEGV. O field-guard antigo (`toadd->queue == queue_`, 0==0) mascarava este
caso; o fix B o expôs.

Não alterado nesta etapa (osSetThreadPri fora do escopo do fix B).

## Repro: como tornar o fix reproduzível

Estado atual: patch NÃO commitado no repo externo (autorização pendente).

Opções propostas (sem refactor arquitetural):

1. **PATCHFILE VERSIONADO NO TOOIE** (recomendado): `patches/runtime/`
   com `threadqueue.patch` (base->after) + script de aplicação no `build`;
   o CMake aplica antes de `add_subdirectory(N64MR)`. Reproduzível por CI e
   por nova clonagem.
2. **FORK/PIN**: forkar `N64ModernRuntime` e pinar o submodule em SHA próprio
   contendo o patch. Mais limpo a longo prazo, porém mexe no monorepo externo.
3. **Aguardar upstream**: propor fix B upstream (PR), pinar quando aceito.

Artefatos de captura: `/tmp/opencode/threadqueue.base.cpp`,
`threadqueue.before.cpp`, `threadqueue.after.cpp`, `tq_contract.cpp`,
`gdb_b_crash.txt`, `gdb_b_trace.txt`, `smoke_b_1.log`, `smoke_b_2.log`.

## Referências

- [[p08-no-threads-left-diagnosis]] (diagnóstico original + seção Fix validation)
- [[p08-active-banjo-reuse-audit]] (N64ModernRuntime DIRTY: threadqueue.cpp)
- `threads.cpp` (osSetThreadPri 304-320; osDestroyThread 282-302)
- `threadqueue.cpp` (insert 14-52; pop 59-66; remove 68-82)
- `scheduling.cpp` (schedule_running_thread 3-7; swap_to_thread 9-16)
