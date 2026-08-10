---
title: "P08 — No threads left to run! (diagnóstico)"
description: "std::runtime_error No threads left to run! = run_next_thread com a running queue vazia (NÃO é esgotamento de threads). ROOT CAUSE E, HIGH: patch local de threadqueue.cpp rejeita re-inserção após osSetThreadPri porque thread_queue_remove não limpa t->queue; fila drena até vazia e a thread SCHEDULE bloqueia em osRecvMesg. Diagnosis-only; fix estreito externo pendente de autorização."
status: evidence
phase: P08
updated: 2026-08-10
tags: [banjo-tooie-recomp, p08, threads, scheduler, threadqueue, osSetThreadPri, running-queue, runtime]
---

# P08 — Diagnóstico: "No threads left to run!"

## Objetivo

Diagnosticar a PRIMEIRA DIVERGÊNCIA que leva a
`std::runtime_error: "No threads left to run!"` no fluxo natural pós-fix de
`glintrosync:6 -> gcsection:4`, com marker-419 OFF.

NÃO tratar como "thread exhaustion": o N64ModernRuntime lança a exceção quando
`run_next_thread()` tenta escolher a próxima thread e a running queue está vazia.
A pergunta desta etapa: por que a running queue está vazia exatamente nesse
momento, e qual thread deveria estar runnable?

## Semântica real do erro

ERROR SITE:
`ultramodern/src/threads.cpp:163-166` — `run_next_thread(RDRAM_ARG1)`:

```cpp
void run_next_thread(RDRAM_ARG1) {
    if (ultramodern::thread_queue_empty(PASS_RDRAM ultramodern::running_queue)) {
        throw std::runtime_error("No threads left to run!\n");
    }
    ...
}
```

CONDITION:
`running_queue` (sentinel `(PTR(PTR(OSThread)))(PTR_SIZE)-1` = `0xffffffff`,
implementação `running_queue_impl` em `threadqueue.cpp:5`) está VAZIA no momento
da chamada. Throw disparado em `threads.cpp:165`.

CALLSITE (capturado em GDB, `ntl_throw_cap.txt`):
`func_80014A88` (thread SCHEDULE) → `osRecvMesg_recomp` → `osRecvMesg` →
`do_recv` (`mesgqueue.cpp:123-127`, osRecvMesg BLOCK em `D_800783D0`) →
`run_next_thread_and_wait` (`threads.cpp:173-177`) → `run_next_thread` → throw.

TRIGGER:
**BLOCK** — a thread SCHEDULE chamou `osRecvMesg(&D_800783D0, OS_MESG_BLOCK)`
com a fila de mensagens vazia, inseriu-se em `blocked_on_recv`, e ao tentar
ceder a CPU para a próxima thread encontrou a running queue VAZIA.
NÃO é THREAD RETURN / STOP / DESTROY.

## Current N64 thread

ID:
5

NAME:
`[Game] SCHEDULE` (nome host via `get_game_thread_name`)

GUEST PTR:
`0x80078C98` (thread table `D_8007D820[5]`)

ENTRYPOINT:
`func_80014A88` (scheduler thread loop: `while(TRUE) { osRecvMesg(&D_800783D0,
BLOCK); ... }` — `1E2B840.c:451-472`)

STATE:
QUEUED (1) no momento do abort (todas as threads mostram state=1 no dump)

PRIORITY:
60 (`SCHEDULER_THREAD_PRIORITY`)

QUEUE:
`0x00000000` no dump do abort — SCHEDULER estava RUNNING (pop da running queue
tinha zerado seu `queue`), não residente em fila; foi o thread que lançou o throw.

SHOULD THIS THREAD RETURN:
**NO** — `func_80014A88` é `while(TRUE)` (loop permanente). Retorno aqui é
incorreto; a thread deveria permanecer bloqueada esperando evento do scheduler.

## Running queue

BEFORE (no dump do abort/throw, `ntl_abort.txt:2242-2244`):
```
running_queue_impl = 0x00000000
RQ len=0 cycle=NO
```

AT FAILURE:
`running_queue_impl = 0` (EMPTY). NENHUMA thread runnable.

INVARIANTS (violadas):
- O patch local assumiu "queue field segue a fila real" mas `thread_queue_remove`
  NÃO limpa `t->queue` → o campo fica STALE (invariante 6 quebrada).
- Threads IDLE/TIDY/JOY carregam `queue=0xffffffff` + `next` links, mas o head
  real (`running_queue_impl`) é 0 → fila corrompida/lost-head.

## Thread inventory

Tabela capturada no abort (`ntl_abort.txt:2232-2241`), rdram=`0x7ffec4400000`:

| idx | id | name (host) | guest | pri | next | queue | state | context |
|----|----|------|-------|-----|------|-------|-------|---------|
| 0 | 0 | VIMGR | 0x80079140 | 80 | 0x0 | 0x80078fd0 | 1 | 0xbc005c00800792f0 |
| 1 | 1 | IDLE | 0x800775f0 | 0 | 0x0 | 0xffffffff | 1 | 0xbc005140800777a0 |
| 2 | 2 | TIDY | 0x80076a20 | 35 | 0x800775f0 | 0xffffffff | 1 | 0xbc07250080076bd0 |
| 3 | 3 | CIC6105 | 0x8007d8b0 | 30 | 0x0 | 0x8007d898 | 1 | 0xbc0053308007da60 |
| 4 | 4 | AUDIO | 0x800459f8 | 50 | 0x0 | 0x80045bb8 | 1 | 0xbc06f09080045ba8 |
| 5 | 5 | SCHEDULER | 0x80078c98 | 60 | 0x0 | 0x0 | 1 | 0xbc06e82080078e48 |
| 6 | 6 | MAIN | 0x80045788 | 127 | 0x80079ba8 | 0x80079030 | 1 | 0x78000b7080045938 |
| 7 | 7 | JOY | 0x80079ba8 | 40 | 0x800775f0 | 0xffffffff | 1 | 0xbc06eaf080079d58 |
| 8 | 8 | MOTOR | 0x8007af10 | 25 | 0x800775f0 | 0x8007aef0 | 1 | 0xbc06edc08007b0c0 |

Observações:
- `queue == 0xffffffff` = sentinel running queue (threads acreditam estar na
  running queue).
- IDLE (1), TIDY (2), JOY (7) têm `queue=0xffffffff` e `next` links apontando
  para IDLE, mas `running_queue_impl=0` — **head perdido / bookkeeping stale**.
- As demais (VIMGR/CIC6105/AUDIO/MAIN/MOTOR) têm `queue=<endereço de mesg
  queue>` → bloqueadas em `blocked_on_recv` de suas filas de mensagens.

## Blocked threads

| THREAD | STATE | QUEUE (blocked_on_recv) | BLOCKED BY | EXPECTED WAKEUP |
|--------|-------|------------------------|------------|-----------------|
| VIMGR | 1 | 0x80078fd0 | osRecvMesg BLOCK (D_80078FD0) | VI host (retrace) |
| CIC6105 | 1 | 0x8007d898 | osRecvMesg BLOCK (D_8007D898) | CIC/SI event |
| AUDIO | 1 | 0x80045bb8 | osRecvMesg BLOCK (D_80045BB8) | AI/audio event |
| MAIN | 1 | 0x80079030 | osRecvMesg BLOCK (D_80079030) | scheduler/retrace via func_800151BC |
| MOTOR | 1 | 0x8007aef0 | osRecvMesg BLOCK (D_8007AEF0) | motor/controller |
| SCHEDULE | RUNNING→BLOCK | 0x0→(D_800783D0) | osRecvMesg BLOCK (D_800783D0) | VI/SP/DP/audio event |

Pergunta da etapa: há thread legítima bloqueada esperando evento que o host
deveria ter enviado e não enviou? → NÃO. Todas as threads bloqueadas estão em
filas de mensagens com producers válidos (VI/SP/DP/AI/SI). O problema NÃO é
wakeup ausente; é a **running queue ter esvaziado** por corrupção.

## Expected wakeups

| THREAD | BLOCKED QUEUE | WAKEUP SOURCE | PRODUCER |
|--------|---------------|---------------|----------|
| VIMGR | D_80078FD0 | OS_EVENT_VI | events.cpp vi_thread_func → enqueue_external_message |
| CIC6105 | D_8007D898 | OS_EVENT_SI / CIC | events.cpp si handling |
| AUDIO | D_80045BB8 | OS_EVENT_AI | audio host thread |
| MAIN | D_80079030 | func_80015190 (osSendMesg) | scheduler/retrace |
| MOTOR | D_8007AEF0 | controller/motor | host input path |
| SCHEDULE | D_800783D0 | OS_EVENT_VI/SP/DP/FAULT/PRENMI + tasks | events.cpp + rsp + gfx |

Todos os producers existem e estão corretos. NÃO é missing wakeup.

## Local N64ModernRuntime diff

BASE COMMIT:
`ca568b6ad79b9029d14077f0c3ffa757727c5559` (HEAD detached de
`../BanjoRecomp/lib/N64ModernRuntime`)

FILES DIRTY:
- `ultramodern/src/threadqueue.cpp` (**M**, +26/−1) — patch anti-duplicata local
- submodule `N64Recomp` (**M**, `src/main.cpp` +4: policy
  `use_lookup_for_all_function_calls = true`)

THREADQUEUE DIFF (única mudança funcional; base upstream = `origin/main`
`589bbf0` sem o patch):

```cpp
+    // A thread can be woken by two host-side event sources before the first
+    // wakeup has run. Inserting it into the same queue twice makes next point
+    // back to itself and permanently wedges the cooperative scheduler.
+    if (toadd->queue == queue_) {
+        debug_printf("  Thread %d is already in queue 0x%08X\n", toadd->id, (uintptr_t)queue_);
+        return;
+    }
+
+    // The queue field is cleared when a thread starts running, so it is not
+    // sufficient on its own to detect a stale link left in the run queue.
+    // Scan the links as well before mutating them.
+    PTR(OSThread)* scan = cur;
+    for (uint32_t hops = 0; *scan != NULLPTR; hops++) {
+        if (*scan == toadd_) {
+            debug_printf("  Thread %d is already linked in queue 0x%08X\n", toadd->id, (uintptr_t)queue_);
+            return;
+        }
+        if (hops == 255) {
+            debug_printf("  Refusing to insert into malformed queue 0x%08X\n", (uintptr_t)queue_);
+            return;
+        }
+        scan = &TO_PTR(OSThread, *scan)->next;
+    }
```

O patch foi adicionado para impedir self-loop da JOY (inserção duplicada por
duas fontes de evento host antes do primeiro wakeup rodar).

## Working vs failing transition

WORKING
-------
- Fila: `running_queue` com ≥1 thread (ex.: [JOY → TIDY → IDLE]).
- Troca: thread atual bloqueia em `do_recv` → insere-se em `blocked_on_recv` →
  `run_next_thread` → pop do head da running queue → sinaliza context do próximo
  → `wait_for_resumed` no próprio context.
- `running_queue` após: head avançou, fila ainda contém threads (fallback IDLE
  sempre presente).

FAILING
-------
- SCHEDULE (pri 60) bloqueia em `osRecvMesg(&D_800783D0, BLOCK)` (fila de
  mensagens vazia).
- `do_recv` insere SCHEDULE em `blocked_on_recv` de D_800783D0.
- `run_next_thread_and_wait` → `run_next_thread` → `thread_queue_empty(running_queue)`
  == TRUE → **throw**.
- `running_queue_impl = 0`; IDLE/TIDY/JOY carregam `queue=0xffffffff` stale.

FIRST DIFFERENCE
----------------
A running queue PERDEU suas threads residentes. As threads IDLE/TIDY/JOY ainda
carregam `queue=0xffffffff` + `next` links, mas o head real foi zerado. A causa
da perda está no fluxo `osSetThreadPri` + `thread_queue_remove` (que não limpa
`t->queue`) + **patch local que rejeita a re-inserção**.

## Expected vs Actual

EXPECTED:
Após `osSetThreadPri` numa thread que está na running queue, a thread é
removida e RE-INSERIDA na posição de prioridade correta. `thread_queue_remove`
deveria limpar `t->queue` (como `thread_queue_pop` faz), de modo que o
`thread_queue_insert` subsequente aceite a re-inserção.

ACTUAL:
`thread_queue_remove` (upstream, inalterado) NÃO limpa `t->queue`. O patch local
em `thread_queue_insert` então rejeita a re-inserção (`toadd->queue == queue_`
→ `return`). Cada chamada a `osSetThreadPri` numa thread residente na running
queue REMOVE a thread e NÃO a devolve → a fila drena thread a thread até
esvaziar.

FIRST DIVERGENCE:
`thread_queue_remove` não limpa `queue` (pre-existente/upstream) + patch local
rejeita re-inserção → running queue esvazia silenciosamente → throw em
`run_next_thread`. A primeira divergência é a **rejeição de re-inserção** (patch
local) agindo sobre o campo stale deixado por `thread_queue_remove`.

## Diagnóstico

ROOT CAUSE:
**E — LOCAL threadqueue.cpp PATCH REGRESSION**

CONFIDENCE:
**HIGH**

Justificativa:
- Captura direta em GDB: throw em `run_next_thread+85` (threads.cpp:165),
  callsite `do_recv`/`osRecvMesg` BLOCK de SCHEDULE (backtrace completo),
  `running_queue_impl=0`.
- Dump do abort mostra IDLE/TIDY/JOY com `queue=0xffffffff` stale + links
  `next`, head zerado → fila drenada por remoção sem limpeza + re-inserção
  rejeitada.
- GDB trace de `osSetThreadPri`: 7690 chamadas; TIDY (`0x80076a20`) pri 35↔10 e
  JOY (`0x80079ba8`) pri 0↔40 — exatamente o fluxo que remove+reinsere na
  running queue.
- Análise semântica do patch: "DUPLICATE INSERT REJECTED" mascara transição
  válida porque `thread_queue_remove` não limpa `queue` (invariante 6 quebrada;
  invariante 9 violada: patch deixa de re-enfileirar thread que DEVERIA voltar).
- Não há missing wakeup; todas as blocked threads têm producers válidos.

## Alteração

**NONE — diagnosis only.** Nenhum código alterado nesta microetapa.

Dependência externa (para reprodução futura):
- `../BanjoRecomp/lib/N64ModernRuntime/ultramodern/src/threadqueue.cpp`
- base `ca568b6`, upstream `origin/main` `589bbf0`.
- Patch exato provável (estreito): em `thread_queue_remove`, limpar
  `TO_PTR(OSThread, t_)->queue = NULLPTR;` após desvincular (simétrico ao
  `thread_queue_pop`), OU ajustar o patch local para re-inserir quando a thread
  NÃO está de fato linkada na fila (scan real, não apenas campo `queue`).
- TooieRecomp deveria tornar a dependência reproduzível (ex.: pinar o runtime
  com o patch aplicado como arquivo versionado/patchfile, não dirty worktree
  silencioso). NÃO commitado externamente sem autorização.

## Build

`ninja -C build TooieRecompiled` — NOT RUN nesta microetapa (sem alteração de
código Tooie; binário é o mesmo `5ac74c0`).

## Smoke

Diagnóstico via GDB sob `TOOIE_AUTO_START=1 TOOIE_DIAG=1
TOOIE_DISABLE_LAUNCHER_MUSIC=1`:
- Throw reproduzido em GDB (SIGABRT em `[Game] SCHEDULE`) em run curto
  (~49k-82k syscalls sob gdb; natural em 3 runs foi a 114k/203k/292k sem throw
  — timing-dependente, correto: é corrupção, não determinístico por contagem).
- Freeze observado (all game threads em `__futex_abstimed_wait`, `running_queue`
  vazia) — mesma condição raiz manifestando como hang em vez de throw.
- gcsection/glintrosync NÃO regride: `last_ovl=677:3`, `stack=(idle)` no ponto
  do throw (após gcsection:4 passar).
- LOOKUP_FUNC(0): 0 ocorrências.
- Contadores: `sched_ack=4`, `rsp_aud=2132+`, `queue_calls` crescente até o
  throw — RSP/VI/audio ativos e saudáveis.

## Regression

GCSECTION FIX:
OK (sem regressão; fluxo natural passa gcsection:4 antes do novo blocker)

LOOKUP_FUNC(0):
NO REGRESSION (0 ocorrências)

## Próxima única etapa

**UMA ação:** decidir/implementar o fix estreito da rejeição de re-inserção
(limpar `t->queue` em `thread_queue_remove`, ou corrigir o guard do patch local
para usar scan real de links) — sob autorização, pois o arquivo é dependência
externa N64ModernRuntime não commitada. Rebuild runtime + smoke natural.
Não executar agora.

## Referências

Docs relacionados: [[p08-gcsection-glintrosync-segv-diagnosis]],
[[p08-actor-init-segv-diagnosis]], [[p08-runtime-frontend-smoke]].

- `ntl_throw_cap.txt`, `ntl_abort.txt`, `ntl_setpri.txt`, `ntl_setpri2.txt`,
  `ntl_freeze_cap.txt` (GDB captures, /tmp/opencode).
- `threads.cpp` (run_next_thread 163-166; run_next_thread_and_wait 173-177;
  osSetThreadPri 304-320; osDestroyThread 282-302).
- `threadqueue.cpp` (local patch 19-41; thread_queue_remove 68-82).
- `scheduling.cpp` (schedule_running_thread 3-7; swap_to_thread 9-16;
  check_running_queue 18-30).
- `mesgqueue.cpp` (do_send 75-112; do_recv 114-144).
- `1E2B840.c:451-472` (SCHEDULER thread), `1E2D890.c` (JOY), `tidy.c` (TIDY).
