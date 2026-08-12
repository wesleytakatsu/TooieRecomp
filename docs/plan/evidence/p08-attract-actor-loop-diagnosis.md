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

`06b2e9d4d383372a32593f74af2b269969bd5974` (documentation correction checkpoint).

## Método — clean / post-freeze only

- Binário original; `TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1`
  com `VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json`.
- Sem debugger/hook/contador/printf durante a execução. Freeze confirmado por
  `d` estável `>=6s` (`179295`..`179315`), black screen pós-intro,
  `last_ovl=635:2`.
- Supervisor pai (`ptrace_scope=1`): `PTRACE_ATTACH`/`PTRACE_GETREGS`/`/proc/pid/mem`
  somente depois do freeze. Amostragem de RIP (60×) por thread para identificar
  a thread guest da MAIN; `nm -n` para resolver RIP→símbolo (PIE base das maps).
- 3 runs independentes (A=6s, B=8s, C=10s após o freeze) + runs extras:
  mesmo resultado em 6/7 freezes capturados.

## Reconstrução estática — func_800C2AB8 (dispatcher de kinematics)

`func_800C2AB8` (chamada por `func_800A7380`, world update) é um loop de
dispersão de slots de kinematics, NÃO um dispatcher de callbacks de actor:

```text
func_800C2AB8():
  func_800C2B80()              ; pré-pass de counters (+0x62)
  base = 0x80128C10            ; = slot index 1 (slots em 0x80128B90 + idx*0x80)
  outer:
    s2 = 1 .. 59               ; iterator (skip index 0)
    s1 = base + s2*0x80
    for each slot:
      if byte(slot+0x78) == 0: skip               ; enabled flag
      mode = (word(slot+0x78) >> 13) & 7
      if mode != 3: skip
      if func_800C269C(s2) != 0: func_800C16A4(s2) ; dispatch -> clears slot
      else: s3++                                  ; pending
    if s3 != 0: repeat outer pass                 ; <-- LOOP INFINITO se algum slot nunca fica ready
```

- `ACTOR_TABLE_BASE`: `0x80128C10` (base do loop; slots `0x80128B90+idx*0x80`)
- `SLOT_COUNT`: 60 (loop usa índices 1..59; índice 0 reservado)
- `SLOT_STRIDE`: `0x80`
- `ACTIVE_FLAG_OFFSET`: `+0x78` (byte; word `+0x78` bits 13-15 = mode)
- `SUBSTATE_OFFSET`: `+0x79`
- `LOOP_INCREMENT`: 1 índice / 0x80 por slot
- `LOOP_TERMINATION`: pass com `s3 == 0` (nenhum slot pendente)

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

## Reconstrução estática — func_80026500 (o hang real)

`func_80026500` é um helper core de **inserção em lista encadeada ordenada**:

```text
func_80026500(a0=struct com head em +0x08, a1, a2, a3=key):
  node = alloc (func_8002FEC0)
  curr = *(a0 + 8)                  ; head da chain
  walk:
    next = *curr                    ; (curr em +0x24, next em +0x28)
    if next == 0: insert no tail; done
    if a3 < *(next + 8):            ; key < node.key (key em +0x08 do node)
        insert antes; done
    curr = next                     ; avança a chain
    goto walk
```

- Terminação: tail (`next == 0`) ou `key < node.key`.
- **LOOP INFINITO se a chain é circular** (nenhum `next == 0`) E a key nunca é
  menor que as node keys — walk gira para sempre.
- Callers diretos (todos core): `func_80020128, func_80020C08, func_80026330,
  func_80025ED0, func_80020A60, func_8001EB10, func_80024DD8, func_8001EE34`.

## Snapshots A/B/C (clean freeze)

| campo | RUN A (6s) | RUN B (8s) | RUN C (10s) |
| --- | --- | --- | --- |
| freeze `d` | 179305 | 179295 | 179300..179315 |
| `last_ovl` | 635:2 | 635:2 | 635:2 |
| thread guest (MAIN) | (versão sem histograma) | `func_80026500` x60 | `func_80026500` x60 |
| RIP off dentro de func_80026500 | — | `0x5bbfa6` | `0x5bbf94`/`0x5bbf9d`/`0x5bbfa6` |
| slot preso | **slot[7]** | **slot[7]** | **slot[7]** |
| `table[2].word0` | `0x8005C120` | `0x8005C168` | `0x8005C168`/`0x8005C0D8` |

- `SAME_PC_ACROSS_RUNS`: **YES** (função `func_80026500`, mesmo walk loop
  `0x5bbf40..0x5bc000`; RIP varia pouco dentro do loop = MAIN ciclando o walk).
- `ITERATOR_SAME_ACROSS_3_RUNS`: YES (slot 7 preso em todos).

## Identificação do slot / actor / callback

- `CURRENT_SLOT_INDEX`: **7**
- `CURRENT_SLOT_ADDR`: `0x80128BD8` (`0x80128B90 + 7*0x80`)
- `CURRENT_SLOT_RAW` (`+0x78` word): `0x01026C00` → byte `+0x78` = 1 (enabled),
  mode (bits 13-15) = 3, substate `+0x79` = **2**, byte `+0x62` = 255.
- `table[2]` (`0x80128B18`): `word0=0x8005C120..0x8005C168` (≠ 0), `word1=1`.
  `0x8005C120`/`0x8005C168` caem em GAP do mapeamento recompilado (não é função
  recompilada; entre `lldiv` 0x80037774 e `func_800815C0`) — dado, não callback.
- `ACTOR_PTR_VALID`: UNKNOWN (ctx guest não capturado de forma confiável; o
  slot[7] contém floats de transform e flags, sem ponteiro de actor evidente).
- Caller (stack host, resolução de símbolo): funções de overlay da inicialização
  do attract — `0x80801484` (nome do binário `func_80801484_bskaztorpedo`;
  metadata também tem `bsstonehead_entrypoint_40` no MESMO endereço — **colisão
  de símbolo**), `0x8080144C` (`func_8080144C_chhumba` / `bsblongleg_entrypoint_31`),
  `subaddietaxi_entrypoint_11`, além de core `func_800FB6E0`, `func_800991B0`.
- `OVERLAY_NAME` / `OVERLAY_INDEX` exatos do caller: UNKNOWN (ambiguidade de
  símbolo em 0x80801484/0x8080144C).

## Binding / lista / iterator

- `CALLBACK_BINDING_MATCH`: UNKNOWN — os funções de overlay estão carregadas e
  executando (o jogo chegou ao attract), mas a colisão de nomes de símbolo em
  `0x80801484` (bskaztorpedo/bsstonehead) e `0x8080144C` (chhumba/bsblongleg)
  impede nomear o actor com segurança só pelo endereço.
- `ITERATOR_VALUE` (dispatcher s2): não capturado (ctx guest); o estado estável
  do slot[7] implica que o dispatcher, se alcançado, giraria nele.
- `ITERATOR_ADVANCES`: N/A para o hang — MAIN NÃO está no dispatcher; está no
  walk de `func_80026500`.
- `LIST_CYCLE`: **YES (provável)** — MAIN 60/60 dentro do walk de ponteiros de
  `func_80026500` (loop `jne 0x5bbf40`), consistente com chain circular.
- `CALLBACK_NONRETURN_CANDIDATE`: **YES** — `func_80026500` não retorna.
- `BAKEY_NATURALLY_REACHED`: **NO** (func_800BCF28 não é o PC corrente).

## Classificação

**A — CALLBACK NON-RETURN** (primária): MAIN é capturada sempre dentro de
`func_80026500` (60/60 amostras, 6 freezes) e a função nunca retorna.
Mecanismo concreto = **C (ACTOR LIST CYCLE/CORRUPTION)**: o walk de ponteiros de
`func_80026500` nunca encontra tail (chain circular) nem key menor — gira para
sempre. O slot de kinematics 7 preso é condição relacionada (dispatcher
`func_800C2AB8` giraria nele se alcançado).

Nota: `func_800C2AB8` NÃO é o PC do hang; o backtrace documentado anterior
(`func_800C2AB8`) era de outra sessão/momento. A fronteira corrente reproduzível
é `func_80026500`.

## First proven divergence

> mode-5/attract init reaches the actor spawn / list-registration path. An
> overlay actor function at `0x80801484` (symbol collision
> bskaztorpedo/bsstonehead) initiates a sorted-list insert via
> `func_80026500`. In the clean freeze, `func_80026500`'s pointer-chain walk
> never terminates (MAIN captured 60/60 samples inside the walk loop
> `0x5bbf40..0x5bc000`); simultaneously the kinematics dispatch slot[7] stays
> enabled with substate 2 and `table[2].word0 != 0`, which would keep the
> `func_800C2AB8` dispatch pass from settling.

## Unknowns

- Endereço da head da lista / node corrente / key (ctx guest não capturado com
  confiança; o ctx vive em `rbx` do frame de `func_80026500`, mas a validação
  por padrão no scan de stack deu falsos positivos).
- Por que a chain está circular: corrupção, binding errado ou estado do jogo.
- Overlay index exato do caller (ambiguidade de símbolo 0x80801484/0x8080144C).
- Um run em 7 (C-v10) mostrou a thread "spinning" em
  `recompui::create_empty_document` x60 em vez de `func_80026500` — variança de
  amostragem ou momento de freeze levemente diferente; não reprodutível como
  padrão.

## Next single action

Capturar o `recomp_context` vivo da MAIN via `rbx` (`PTRACE_GETREGS`) com
validação sign-extended (r29 sp guest + r31 ra guest ≠ 0), ler os args do frame
de `func_80026500` no guest stack (arg0 = struct com head em +0x08, a3 = key,
curr/next em +0x24/+0x28), e caminhar a chain a partir de `*(arg0+8)` para
achar o ciclo (self-reference / back-edge) e o dono da lista. Inspeção somente
pós-freeze; nenhuma instrumentação.

## Integridade do checkpoint

```text
CODE_CHANGED             = NO
RUNTIME_CODE_CHANGED     = NO
GENERATOR_CODE_CHANGED   = NO
PATCH_CODE_CHANGED       = NO
CORE_INCLUDED            = NO
BUILD_ARTIFACT_INCLUDED  = NO
NEXT_INVESTIGATION_RUN   = YES (se necessário para o ctx)
```
