---
title: "P08 — attract actor-loop diagnosis: clean freeze in func_80026500"
description: "Diagnóstico post-mortem do freeze natural do mode-5/attract: MAIN presa no walk de lista encadeada em func_80026500; slot de kinematics 7 preso."
status: evidence
phase: P08
updated: 2026-08-12
tags: [tooie-recomp, p08, attract, actor-loop, clean-freeze, func_80026500, list-cycle]
---

# P08 — CLEAN ATTRACT ACTOR LOOP DIAGNOSIS

## HEAD

- Checkpoint documental corrente: `670d994e74f939e1df881e58ecb4689b661bc575`
  (BASE_HEAD). Checkpoint anterior (superseded):
  `06b2e9d4d383372a32593f74af2b269969bd5974`.

> **VALIDAÇÃO INDEPENDENTE (P08):** Gemini 3.6 Flash HIGH repetiu análise
> estática + 2 clean runs e CONFIRMOU independentemente a forense nativa
> (DeepSeek) — **DEEPSEEK_GEMINI_AGREE=YES**. Blocker corrente renomeado para
> **KINEMATICS SLOT-7 READINESS BUSY-WAIT** (SYNCHRONOUS DEPENDENCY CYCLE /
> BUSY-WAIT, NÃO scheduler/thread deadlock).

## Método — clean / post-freeze only

- Binário original; `TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1`
  com `VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json`.
- Sem debugger/hook/contador/printf durante a execução. Freeze confirmado por
  `d` estável `>=6s` (`178999`..`179315`), black screen pós-intro,
  `last_ovl=635:2`.
- Supervisor pai (`ptrace_scope=1`): `PTRACE_ATTACH`/`PTRACE_GETREGS`/`/proc/pid/mem`
  somente depois do freeze. Amostragem de RIP (60×) por thread para identificar
  a thread guest da MAIN; `nm -n` para resolver RIP→símbolo (PIE base das maps).
- 6+ freezes capturados (forense nativa) + 2 clean runs independentes (Gemini).

## Reconstrução estática — func_800C2AB8 (dispatcher de kinematics)

`func_800C2AB8` (chamada por `func_800A7380`/`func_800A7D84`, world update) é um
loop de convergência de 60 slots de kinematics/items, NÃO um dispatcher de
callbacks de actor:

```text
func_800C2AB8():
  func_800C2B80()              ; pré-pass de counters (+0x62)
  base = 0x80128C10            ; = slot index 1 (slots em 0x80128B90 + idx*0x80)
  outer:
    s2 = 1 .. 59               ; iterator (skip index 0)
    s1 = base + s2*0x80
    pending = 0
    for each slot:
      if byte(slot+0x78) == 0: skip               ; enabled flag
      mode = (word(slot+0x78) >> 10) & 7          ; bits 10-12
      if mode != 3: skip
      if func_800C269C(s2) != 0: func_800C16A4(s2) ; dispatch -> clears slot
      else: pending++                             ; pending
    if pending != 0: repeat outer pass            ; <-- BUSY-WAIT se algum slot nunca fica ready
```

- `ACTOR_TABLE_BASE`: `0x80128C10` (base do loop; slots `0x80128B90+idx*0x80`)
- `SLOT_COUNT`: 60 (loop usa índices 1..59; índice 0 reservado)
- `SLOT_STRIDE`: `0x80`
- `ACTIVE_FLAG_OFFSET`: `+0x78` (byte; mode = bits 10-12 do word `+0x78`)
- `SUBSTATE_OFFSET`: `+0x79`
- `LOOP_INCREMENT`: 1 índice / 0x80 por slot
- `LOOP_TERMINATION`: pass com `pending == 0` (nenhum slot pendente)
- **OUTER_BUSY_WAIT_CONFIRMED: YES** (native single-step: MAIN nunca sai do loop)

### Papel de func_800C269C (query de prontidão)

```text
func_800C269C(idx):
  slot = 0x80128B90 + idx*0x80
  if mode(slot) != 3: return 0
  substate = byte(slot+0x79)
  if substate == 0: return 1                      ; pronto
  return (table[substate].word0 == 0) ? 1 : 0     ; table = 0x80128B08 + s*8
```

`func_800C1448(s)` lê `table[s].word0`; `func_800C1568(s)` limpa `table[s].word1`;
`func_800C16A4(idx)` limpa `enabled(+0x78)` e `substate(+0x79)` e chama
`func_800C1568(substate)`.

**GATE DE LOOP INFINITO do dispatcher:** um slot enabled + mode-3 com
`substate != 0` E `table[substate].word0 != 0` (nunca limpo) faz
`func_800C269C` retornar 0 para sempre → o pass externo repete para sempre.

### get_function(0x800C269C)

- `FIXED_LOOKUP: YES`. `func_800C2AB8` chama `func_800C269C`/`func_800C16A4`
  por endereço **constante** no código (`LOOKUP_FUNC(0x800C269C)`). Não é
  callback dinâmico de actor. O lookup é helper core fixo.

## Reconstrução estática — func_80026500 (não é o PC do hang; ver forense nativa abaixo)

`func_80026500` é um **core helper** (não é comprovadamente actor callback) de
**inserção em lista duplamente encadeada com chaves DELTA/RELATIVAS** (re-derivado
do MIPS original no ROM decompressa e do C recompilado; ver
[#native-forensics-p08-func80026500-refuted](#native-forensics-p08-func80026500-refuted)
para o passo a passo do native loop):

```text
func_80026500(a0=listhdr, a1=data_buf, a2=key, a3=arg3):
  prev_mask = osSetIntMask(1)          ; seção crítica (0x8002FEC0)
  insert_node = *a0                    ; [sp+0x2c] = a0[0] (node a (re)inserir)
  if insert_node == 0: restore; return ; no-op
  if insert_node->next == 0 AND arg3 == 0: restore; return  ; no-op
  func_80020E74(insert_node)           ; UNLINK do DLL corrente
  bcopy(arg1, insert_node+0xC, 0x10)   ; copia payload 16B p/ arg1 (0x80031BF0)
  if key == 0x7FFFFFFF: flag=-1
  curr = a0 + 8                        ; [sp+0x24] = &listhdr->head (next-slot)
  walk:
    next = *curr                       ; [sp+0x28]
    if next == 0:                      ; tail insert
        insert_node->key = flag ? 0 : key
        func_80020E40(insert_node, curr); end
    next_key = next->key               ; node key em +8
    if SIGNED(key) < SIGNED(next_key): ; insert before next
        insert_node->key = key
        next->key -= key               ; ajuste delta
        func_80020E40(insert_node, curr); end
    else:                              ; advance
        key -= next_key                ; [sp+0x38] = key - next_key  (MUTAÇÃO DE KEY)
        curr = next
        if *curr != 0: goto walk       ; else [sp+0x24]=0; end
  end: osSetIntMask(prev_mask); return
```

- **KEY_IS_MUTATED_DURING_WALK: YES** — `key = key - next->key` (`subu`, 32-bit),
  gravado de volta em `[sp+0x38]` a cada advance.
- **EXACT_KEY_UPDATE:** `key = (int32)(key - next->key)`.
- **NEXT_LOAD:** `next = *(uint32_t*)(curr+0)`; `curr` inicia em `a0+8` e depois
  `curr = next`.
- **TERMINATION_A:** `next == 0` → tail insert.
- **TERMINATION_B:** `SIGNED(key) < SIGNED(next->key)` → insert-before + delta.
- Comparação é **signed** (`slt`); subtração `subu` (wrap 32-bit = igual p/ signed).
- **NÃO** é "sorted absolute-key list": é **delta/relative keys** (cada node +8 é
  o intervalo em relação ao predecessor).
- `func_8002FEC0` = `osSetIntMask`; `func_80031BF0` = `bcopy` (confirmados na
  syms table). `func_80020E74` = DLL unlink; `func_80020E40` = DLL insert-after.
- **ORIGINAL_RECOMP_MATCH: YES** (MIPS no ROM == C recompilado, instrução a
  instrução; sem divergência).

## Snapshots A/B/C (clean freeze)

| campo | RUN A (6s) | RUN B (8s) | RUN C (10s) |
| --- | --- | --- | --- |
| freeze `d` | 179305 | 179295 | 179300..179315 |
| `last_ovl` | 635:2 | 635:2 | 635:2 |
| thread guest (MAIN) | (versão sem histograma) | `func_80026500` x60 | `func_80026500` x60 |
| RIP off dentro de func_80026500 | — | `0x5bbfa6` | `0x5bbf94`/`0x5bbf9d`/`0x5bbfa6` |
| slot preso | **slot[7]** | **slot[7]** | **slot[7]** |
| `table[2].word0` | `0x8005C120` | `0x8005C168` | `0x8005C168`/`0x8005C0D8` |

> **CORREÇÃO (2026-08-12):** a coluna "thread guest / RIP off dentro de
> `func_80026500`" acima está **REFUTADA / SUPERSEDED** pela forense nativa
> (DeepSeek) + re-análise Gemini: o histograma real do freeze é
> `func_800C2AB8 ~45%`, `func_800C269C ~35%`, `get_function ~20%`,
> `func_80026500 0%`. Os samples `0x5bbfa6`/`0x5bbf94`/`0x5bbf9d` não se
> reproduzem; `func_80026500` NÃO é o current hang. O que se mantém: slot 7
> preso e `table[2].word0≠0`.

- `SAME_PC_ACROSS_RUNS`: **YES** — no mesmo mecanismo
  (`func_800C2AB8`/`func_800C269C`/`get_function`), NÃO no walk de
  `func_80026500`.
- `ITERATOR_SAME_ACROSS_3_RUNS`: YES (slot 7 preso em todos).

## Identificação do slot / actor / callback

- `CURRENT_SLOT_INDEX`: **7**
- `CURRENT_SLOT_ADDR`: **`0x80128F10`** (`0x80128B90 + 7*0x80`; CORREÇÃO:
  o valor anterior `0x80128BD8` estava errado — ver
  [#correcao-slot-7](#correcao-slot-7))
- `CURRENT_SLOT_RAW` (`+0x78` word): `0x01026C00` → byte `+0x78` = 1 (enabled),
  mode (bits 10-12, `(w>>10)&7`) = 3, substate `+0x79` = **2**, byte `+0x62` = 255.
- `table[2]` (`0x80128B18`): `word0=0x8005C120..0x8005C168` (≠ 0), `word1=1`.
  `0x8005C120`/`0x8005C168` caem em GAP do mapeamento recompilado (não é função
  recompilada; entre `lldiv` 0x80037774 e `func_800815C0`) — dado, não callback.
- `ACTOR_PTR_VALID`: UNKNOWN (slot[7] contém floats de transform e flags, sem
  ponteiro de actor evidente).
- Caller (stack host, resolução de símbolo): funções de overlay da inicialização
  do attract — `0x80801484` (nome do binário `func_80801484_bskaztorpedo`;
  metadata também tem `bsstonehead_entrypoint_40` no MESMO endereço — **colisão
  de símbolo**), `0x8080144C` (`func_8080144C_chhumba` / `bsblongleg_entrypoint_31`),
  `subaddietaxi_entrypoint_11`, além de core `func_800FB6E0`, `func_800991B0`.
- `OVERLAY_NAME` / `OVERLAY_INDEX` exatos do caller: UNKNOWN (ambiguidade de
  símbolo em 0x80801484/0x8080144C).

## Binding / lista / iterator

- `CALLBACK_BINDING_MATCH`: UNKNOWN — as funções de overlay estão carregadas e
  executando (o jogo chegou ao attract), mas a colisão de nomes de símbolo em
  `0x80801484` (bskaztorpedo/bsstonehead) e `0x8080144C` (chhumba/bsblongleg)
  impede nomear o actor com segurança só pelo endereço.
- `ITERATOR_VALUE` (dispatcher s2): **7** (recuperado do ctx nativo: `s1=slot 7`,
  `s2=7`, `s4=3`, `s5=0x3C`, `s6=0x80128C10`).
- `ITERATOR_ADVANCES`: N/A — MAIN NÃO está no walk de `func_80026500`; está no
  **loop externo do dispatcher `func_800C2AB8`** (host `+0x130..+0x1bd`), girando
  no slot 7.
- `LIST_CYCLE`: **N/A / REFUTADO como PC do hang** — `func_80026500` nunca é
  executado no freeze (0 hits em 3000+ single-steps nativos; não está no call
  graph do dispatcher). A chain de `func_80026500` não é caminhada no freeze.
- `CALLBACK_NONRETURN_CANDIDATE`: **YES** — o call graph
  `func_800A7D84 -> func_800C2AB8 -> func_800C269C(7) -> func_800C1414/
  func_800C1448` nunca retorna ao tick (dispatcher repete o pass externo).
- `BAKEY_NATURALLY_REACHED`: **NO** (func_800BCF28 não é o PC corrente).

## Classificação

**KINEMATICS SLOT-7 READINESS BUSY-WAIT (SYNCHRONOUS DEPENDENCY CYCLE /
BUSY-WAIT)** — `func_80026500` NÃO é o PC do hang. Forense nativa ptrace
(DeepSeek, 6+ runs, freeze `d≈178999..179315`, `last_ovl=635:2`) + validação
independente (Gemini, 2 clean runs) — **DEEPSEEK_GEMINI_AGREE=YES**:

- MAIN (host thread `[Game] MAIN`, guest OSThread id 6) fica presa no **loop
  de convergência mode-3 de `func_800C2AB8`** (host `0x69ff68..0x69ffad`),
  ciclando `func_800C269C(7) → func_800C1414 / func_800C1448 → get_function`.
  Histograma RIP: `func_800C2AB8 ~45%`, `func_800C269C ~35%`,
  `get_function ~20%`, `func_80026500 0%`.
- **Gate provado:** o slot 7 (`0x80128F10`) é o ÚNICO slot enabled mode-3;
  `raw=0x01026C00` (enabled=1, mode=3, substate=2), `table[2].word0=0x8005C168
  ≠0` → `func_800C269C(7)` retorna **0 (PENDING)** para sempre → pending nunca
  chega a zero → `func_800C2AB8` **busy-waits** indefinidamente.
  `func_800C16A4(7)` (dispatch/clear) nunca dispara.
- A hipótese anterior "A — CALLBACK NON-RETURN em `func_80026500`" está
  **REFUTADA** (os samples `0x5bbf94..0x5bbfa6` não se reproduzem nativamente).

**C (LIST CYCLE) não aplicável**: a chain de `func_80026500` não é executada no
freeze. Não declarar ciclo de `func_80026500` como fechado.

## Nível de prova

**PROVEN ROOT MECHANISM (freeze):**

```text
- func_800C2AB8 não retorna porque slot7 permanece pending:
  substate=2 e table[2].word0 != 0                       PROVEN
- func_800C269C(7) == 0                                  PROVEN
- pending nunca chega a zero                             PROVEN
- func_800C16A4(7) não dispara                           PROVEN
- func_80026500 não é o current hang                     PROVEN
```

**AINDA NÃO PROVADO ROOT CAUSE:**

```text
- por que table[2].word0 ainda contém o objeto 0x8005C168
  na entrada/execução deste func_800C2AB8:
  qual objeto é 0x8005C168;
  quem o criou neste fluxo concreto;
  quando deveria terminar;
  qual condição leva func_800205D0 a limpá-lo;
  por que esse cleanup não ocorreu ANTES de entrar no busy-wait;
  se há erro de ordering/lifecycle;
  se algum callback/binding anterior impediu completion;
  se estado stale sobreviveu à troca intro -> attract.
```

## Table lifecycle (auditoria Gemini)

```text
table[substate].word0 = ponteiro para objeto/tarefa/emissor ativo,
                        criado/atribuído através de func_80020790.
table[substate].word1 = flag de entrada reservada/alocada.

slot substate       = configurado através de func_800C16F8,
                      após entrada obtida por func_800C15BC.

clearer esperado    = func_800205D0, via lifecycle manager func_800200B8.

Execução do clearer é SÍNCRONA na MAIN. Enquanto MAIN permanece dentro do
busy-wait de func_800C2AB8, func_800200B8 / func_800205D0 NÃO recebem
oportunidade normal de executar.
```

Classificar como **SYNCHRONOUS DEPENDENCY CYCLE / BUSY-WAIT** e NÃO como
scheduler deadlock / thread deadlock.

## First proven divergence

> During mode-5/attract initialization, `func_800C2AB8` enters its mode-3
> kinematics convergence loop. Slot 7 (0x80128F10) remains enabled with
> substate 2 while `table[2].word0` remains 0x8005C168. Therefore
> `func_800C269C(7)` continuously returns 0, the pending count never reaches
> zero, and the MAIN guest thread busy-waits indefinitely inside
> `func_800C2AB8`. The immediate freeze mechanism is proven. The unresolved
> root cause is why the table[2] object has not completed/been cleared before
> this convergence loop.

## Unknowns

- **Writer/lifecycle de `table[2].word0`:** qual objeto é `0x8005C168` (região
  de GAP 0x8005Cxxx), quem o criou neste fluxo, quando deveria terminar e por
  que `func_800205D0` não o limpou antes do busy-wait. (Próxima etapa.)
- Overlay index exato do caller (ambiguidade de símbolo 0x80801484/0x8080144C).
- Um run em 7 (C-v10) mostrou a thread "spinning" em
  `recompui::create_empty_document` x60 em vez do dispatcher — variança de
  amostragem ou momento de freeze levemente diferente; não reprodutível como
  padrão.

## Next single action

Investigar o **writer** de `slot 7` e `table[2]`: quais funções (overlay
0x80801484/bskaztorpedo|bsstonehead?) populam o slot e a table, e por que o
substate/table nunca avança para o estado que permitiria `func_800C269C(7)==1`.
Inspeção somente pós-freeze; nenhuma instrumentação.

## Exact func_80026500 list-chain snapshot

> **ESTADO (2026-08-12, REVISADO pela forense nativa):** `func_80026500` NÃO é
> o PC do hang. A forense ptrace pós-freeze (single-step nativo, 3000+ passos em
> múltiplos runs) mostra MAIN presa no **dispatcher `func_800C2AB8`** (gate do
> slot 7), e `func_80026500` nunca é executado. Portanto a chain de
> `func_80026500` NÃO pode ser caminhada no freeze; o ciclo daquela lista é
> **N/A** para o hang. A classificação `LIST_CYCLE` anterior (`UNPROVEN`) fica
> **N/A / REFUTADA** — a lista não participa do freeze.

### Método de extração (forense nativa, post-freeze only)

- Supervisor pai: `run original -> freeze (d estável >=6s) -> PTRACE_ATTACH`
  de todas as threads -> identificar host thread `[Game] MAIN` (comm) ->
  `PTRACE_GETREGS` -> `PTRACE_SINGLESTEP` (host) gravando RIP/regs por passo.
- Resolução RIP->símbolo via `nm -n build/TooieRecompiled` com a ELF base das
  maps (offset 0 da mapping com nome TooieRecompiled).
- `recomp_context` localizado por scan do host stack (candidato `P` com
  `[P+0xe8]` = guest sp sign-extended válido).
- Host stack RA walk para reconstruir a cadeia de chamadas.

### Resultado (consistente em 6+ runs)

```text
HOST_RIP        = func_800C2AB8 +0x130..0x1bd (0x69ff68..0x69ffad)  [dispatcher loop]
                  get_function 0xb314xx; func_800C269C 0xa323xx
CONTEXT         = host stack (rbx do frame do dispatcher); guest sp 0x80045670..0x80045688
HOST_RBP        = 0x800a76f4  (valor guest estável no frame; coincide com vaddr
                  de func_800A76F4 mas é um dado do dispatcher, não o caller)
HOST_RCX        = ponteiro do slot corrente (s1); congelado no slot 7:
                  0xffffffff80128f10 = SLOT 7 (0x80128b90+7*0x80)
```

- Live guest state do dispatcher: `s1=0x80128F10` (slot 7), `s2=7`, `s4=3`,
  `s5=0x3C`, `s6=0x80128C10`; `s3` observado `1` (pending) num run e `0`
  durante o dispatch do próprio slot 7.
- Slot table: somente o slot 7 está enabled mode-3 (`raw=0x01026C00`):
  enabled=1, mode=3, substate=2, `table[2].word0=0x8005C168≠0` → PENDING.
- Cadeia de chamadas host: `_thread_func -> run_thread_function ->
  func_800A7D84 -> func_800C2AB8 -> func_800C269C(7) -> func_800C1414 /
  func_800C1448 -> get_function`.
- `func_80026500`: 0 hits em 3000+ single-steps; não está no call graph do
  dispatcher. **REFUTADO como PC do hang.**

### O que NÃO foi extraído

- A chain de `func_80026500` (head/node/back-edge/CYCLE_LENGTH): **N/A** — a
  função nunca executa no freeze. Não existem live locals do walk para
  recuperar.

### Classificação (nativa)

- `CALLBACK_NONRETURN_CANDIDATE`: **YES** — o call graph do dispatcher
  (`func_800C2AB8 → func_800C269C(7)`) não retorna ao tick (pass externo
  infinito). `func_80026500` não participa.
- `LIST_CYCLE_PROVEN`: **NO / N/A** (func_80026500 não executado no freeze).
- `DISCRIMINATOR_CLASS`: **KINEMATICS SLOT-7 READINESS BUSY-WAIT** —
  a hipótese `func_80026500` é refutada; o mecanismo real é o gate de readiness
  do dispatcher no slot 7 (`func_800C269C(7)==0` eterno: `substate=2`,
  `table[2].word0=0x8005C168≠0`).
- `SLOT7_DIRECTLY_RELATED`: **DIRECT** — `rcx` no dispatch = slot 7
  (`0x80128F10`); é o slot que trava o dispatcher.

### First proven divergence (atualizada)

> During mode-5/attract initialization, `func_800C2AB8` enters its mode-3
> kinematics convergence loop. Slot 7 (0x80128F10) remains enabled with
> substate 2 while `table[2].word0` remains 0x8005C168. Therefore
> `func_800C269C(7)` continuously returns 0, the pending count never reaches
> zero, and the MAIN guest thread busy-waits indefinitely inside
> `func_800C2AB8`. The immediate freeze mechanism is proven. The unresolved
> root cause is why the table[2] object has not completed/been cleared before
> this convergence loop.

### Next single action

Rastrear o ciclo de vida completo de `table[2].word0` / objeto `0x8005C168`:
creation (`func_80020790?`) → ownership → update → completion condition →
`func_800205D0` clear (via `func_800200B8`) — e identificar o ponto mais antigo
onde o clean recomp diverge do lifecycle original. Inspeção somente pós-freeze.

## Correção: slot 7

```text
slot_base = 0x80128B90
stride    = 0x80
index     = 7

0x80128B90 + 7*0x80 = 0x80128F10

KINEMATICS_SLOT7_ADDR = 0x80128F10     (NÃO 0x80128BD8)
```

`rcx = 0x80128F10` observado no freeze é numericamente compatível com o slot 7.
Confirmado nativamente: o dispatcher `func_800C2AB8` processa o slot 7
(`s1=0x80128F10`, `s2=7`) e é exatamente esse slot que mantém o gate de
readiness aberto (`func_800C269C(7)==0`).

## Native forensics P08 — func_80026500 refuted

### Terminologia

- `func_80026500` é **core helper / list helper** (inserção DLL delta-key),
  NÃO comprovadamente "actor callback". Até o caller exato ser reconstruído,
  usar `core helper` / `list helper` / `func_80026500`.
- `LIST_CYCLE_PROVEN = NO` (e na verdade **N/A**: a lista não participa do
  freeze).

### Método (post-freeze only, sem instrumentação antes do freeze)

```sh
# supervisor pai (filho = TooieRecompiled), freeze confirmado por
# 'syscall d=' estável >=6s com last_ovl=635:2; somente então:
# PTRACE_ATTACH de todas as threads -> [Game] MAIN -> PTRACE_GETREGS
# -> PTRACE_SINGLESTEP (host) -> /proc/pid/mem
```

- Binário original; `TOOIE_AUTO_START=1 TOOIE_DIAG=1
  TOOIE_DISABLE_LAUNCHER_MUSIC=1` + `VK_ICD_FILENAMES=.../radeon_icd.json`.
- ELF base = mapping offset 0 da imagem (ex. `0x61e06f0fe000`); símbolos via
  `nm -n build/TooieRecompiled`.
- `recomp_context` por host-stack scan (`[P+0xe8]` == guest sp sign-ext).
- Host stack RA walk para a cadeia de chamadas.

### Resultado consolidado (6+ runs, freeze `d≈178999..179315`)

```text
FREEZE_D          = 178999..179315 ; last_ovl=635:2
MAIN_HOST_THREAD  = [Game] MAIN (guest OSThread id 6)
RIP               = func_800C2AB8+0x130..0x1bd  |  func_800C269C  |  get_function
CALL CHAIN        = _thread_func -> run_thread_function -> func_800A7D84
                    -> func_800C2AB8 -> func_800C269C(7) -> func_800C1414
                    / func_800C1448 -> get_function(0x800C14xx)
GUEST SP          = 0x80045670 / 0x80045688 (sign-ext)
s1 (slot)         = 0x80128F10  (slot 7)
s2 (iterator)     = 7
s4 / s5           = 3 (mode) / 0x3C (60 slots)
s6 (base)         = 0x80128C10
SLOT 7 raw (+0x78)= 0x01026C00  enabled=1 mode=3 substate=2
table[2] raw BE   = 68 C1 05 80 01 00 00 00
table[2].word0    = 0x8005C168  (≠ 0)  -> func_800C269C(7) == 0 (PENDING)
table[2].word1    = 1
PENDING slots     = [7]  (único slot enabled mode-3)
RIP HISTOGRAM     = func_800C2AB8 ~45% | func_800C269C ~35% |
                    get_function ~20% | func_80026500 0%
```

- **Loop observado (single-step nativo, 3000+ passos):** MAIN cicla
  `get_function` (ret) -> `func_800C269C` -> `func_800C1414`/`func_800C1448`
  (lookups) -> `func_800C2AB8` (slot loop `0x69ff68..0x69ffad`) -> repete.
- **`func_80026500`: 0 hits** em 3000+ passos (DeepSeek) e 0% no histograma
  (Gemini); não está no call graph do dispatcher. **O walk de ponteiros de
  `func_80026500` não é o PC do hang.**
- `rbp = 0x800a76f4` estável (valor guest do frame do dispatcher; coincide com
  o vaddr de `func_800A76F4` mas é um dado, não o caller).
- `rcx` = ponteiro do slot corrente (`s1`); congelado no slot 7:
  `0xffffffff80128f10` → **DIRECT** (é o slot que trava o dispatcher).

### Gate do dispatcher (causa imediata provada)

```text
func_800C2AB8 (host 0x69fdf0):
  func_800C2B80()                 ; pre-pass
  outer:
    s2 = 1..59; s3 = 0
    for each slot (s1 = 0x80128C10 + (s2-1)*0x80):
      if byte(slot+0x78) == 0: continue        ; enabled
      if mode(slot+0x78) != 3: continue        ; (w>>10)&7
      if func_800C269C(s2) != 0: func_800C16A4(s2)   ; dispatch+clear
      else: s3++                                ; PENDING
    if s3 != 0: goto outer                      ; <-- NUNCA sai
```

`func_800C269C(idx)`:
```text
  slot = func_800C1414(idx) = 0x80128B90 + idx*0x80
  if mode(slot) != 3: return 0
  substate = byte(slot+0x79)
  if substate == 0: return 1
  return (func_800C1448(substate) == 0) ? 1 : 0    ; table[0x80128B08+s*8].word0
```

Para o slot 7: mode=3, substate=2, `func_800C1448(2)=table[2].word0
=0x8005C168≠0` → retorna **0 para sempre** → `s3` nunca zera → pass externo
infinito. `func_800C16A4(7)` (única forma de limpar o slot) nunca é chamado.

### POR QUE NÃO é func_80026500

- O walk de `func_80026500` só roda se o dispatcher terminar e o caller
  (`func_80020128` etc., core) for alcançado. No freeze, o dispatcher nunca
  retorna.
- O call graph do dispatcher (`func_800C1414`/`func_800C1448`/`func_800C16A4`/
  `func_800C269C`) não referencia `func_80026500` (grep nos recompiled funcs:
  0 hits).
- Samples anteriores (`0x5bbf94..0x5bbfa6`) não se reproduzem nativamente em
  nenhum dos 6+ runs (DeepSeek) nem no histograma independente (Gemini:
  `func_80026500 0%`); a hipótese é refutada.
- **DEEPSEEK_GEMINI_AGREE = YES** (validação independente; ver topo do doc).


## Integridade do checkpoint

```text
CODE_CHANGED             = NO
RUNTIME_CODE_CHANGED     = NO
GENERATOR_CODE_CHANGED   = NO
PATCH_CODE_CHANGED       = NO
CORE_INCLUDED            = NO
BUILD_ARTIFACT_INCLUDED  = NO
NEXT_INVESTIGATION_RUN   = YES (forense nativa post-freeze: 6+ runs de clean
                           freeze + PTRACE_ATTACH/SINGLESTEP; sem instrumentação
                           pré-freeze)
```
