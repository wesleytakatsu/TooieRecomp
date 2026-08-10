---
title: "P08 — diagnóstico: callback nulo em func_80108C90 (ActorInfo[0x38])"
description: "Prova com runtime + ROM que o valor 0 lido como callback de actor no spawn forçado não é um callback opcional legítimo; a origem é o mapeamento de data de overlay."
status: evidence
phase: P08
updated: 2026-08-09
tags: [banjo-tooie-recomp, p08, actors, func_80108C90, gemarkersDll, chintroticker, overlay-data, null-callback]
---

# P08 — diagnóstico: callback nulo em func_80108C90 (ActorInfo[0x38])

## Objetivo

Determinar com evidência se o valor `0` passado a `LOOKUP_FUNC` no spawn
forçado de chintroticker é um **callback legitimamente opcional** no jogo
original, ou consequência de estado / ActorInfo / inicialização incorreta no
TooieRecomp.

## Estado inicial

- Auto-spawn de diagnóstico (workaround, não commitado como fix) chama
  `func_80108C90(419, ...)` no retorno de `gcsectionDll:4` para o mapa
  `0x18B` (`src/game/syscall_handler.cpp`).
- O runtime abortava com `Failed to find function at 0x00000000` (exit 139).
- Documentação anterior (p08-session-handoff) atribuía o abort a
  `func_80107C2C` / `ctx->r2`; o handoff não estava preciso.

## Caminho no C recompilado

Função real onde o abort ocorre: **`func_80108C90`**
(`TooieRecomp/RecompiledFuncs/funcs_126.c:2388`), não `func_80107C2C`.

```c
// 0x80108CA0: jal  _gemarkersDll_entrypoint_0   (syscall 793, marker resolver)
LOOKUP_FUNC(0x80089750)(rdram, ctx);          // r2 = entrypoint do actor
if (r2 != 0) { a3 = r2; ... }
// 0x80108CB8: jalr $a3                        (chama construtor do actor)
LOOKUP_FUNC(ctx->r7)(rdram, ctx);             // r2 = ponteiro para ActorInfo
// ...
// 0x80108CD0: lw  $t9, 0x38($v0)              (lê callback do ActorInfo)
ctx->r25 = MEM_W(0x38, ctx->r2);
// 0x80108CDC: jalr $t9                        ← LOOKUP_FUNC(0) aqui
LOOKUP_FUNC(ctx->r25)(rdram, ctx);            // r25 == 0 → abort
```

- **Origem de `ctx->r25` (= callback nulo):** campo `+0x38` da struct apontada
  pelo retorno do construtor do actor.
- **Argumento/ActorInfo envolvido:** o construtor é `chcollballcontrol
  _entrypoint_3` (resolvido pelo marker), que retorna `D_80804FA0
  _chcollballcontrol` (o "ActorInfo" do actor).
- **Endereço MIPS da chamada indireta:** `0x80108CDC` (`jalr $t9`).
- **Registradores antes do LOOKUP_FUNC:** `$v0` = retorno do construtor
  (0x80804FA0 no runtime), `$t9` = `MEM_W(0x38, $v0)` = 0 no runtime.

`func_80107C2C` **não** contém `LOOKUP_FUNC(ctx->r2)`: foi verificado que todas
as chamadas dela são estáticas. `func_80107C2C` é o construtor genérico de
actor que **deveria** ser alcançado via `ActorInfo[0x38]`; o abort acontece
antes, ao resolver esse callback.

## Comportamento MIPS original

Evidência da ROM (`banjo-tooie-Recomp/asm/data/overlays/ch/collballcontrol
.data.s`):

```asm
/* 20877E0 80804FA0 */ dlabel D_80804FA0_chcollballcontrol   # = ActorInfo
# ... offset +0x38 ...
/* 2087818 80804FD8 80107C2C */ .word func_80107C2C           # campo +0x38
```

- O campo `ActorInfo[0x38]` no ROM contém **`func_80107C2C`** (0x80107C2C),
  **não** zero. Portanto o callback **não** é opcional nesse caminho.
- No original, o fluxo seria: `func_80108C90` → `gemarkersDll` → construtor
  (retorna ActorInfo) → `jalr ActorInfo[0x38]` = `func_80107C2C`.

Respostas à investigação obrigatória:

1. O original executaria um `jalr` com target 0? **Não comprovado como
   intencional** — o campo tem valor fixo não-nulo no ROM.
2. Existe branch/null check antes da chamada? **Não.** `func_80108C90` faz
   `lw $t9, 0x38($v0)` + `jalr $t9` sem guarda.
3. O campo deveria estar preenchido antes desse ponto? **Sim** — o construtor
   retorna um ActorInfo cujo campo `+0x38` está preenchido no ROM.
4. Qual estrutura fornece esse callback? O **ActorInfo** (data do overlay) do
   actor; para chcollballcontrol, `D_80804FA0` (data section do overlay 467).
5. O ActorInfo de chintroticker possui esse campo nulo legitimamente? **Não é o
   actor spawnado**: o marker 419 resolve para `chcollballcontrol_entrypoint_3`
   (ver Comparação), e mesmo para esse actor o ROM tem `func_80107C2C`.
6. O spawn manual via `func_80108C90(419)` pula inicialização necessária?
   **Sim, indiretamente**: usa o **marker id errado** (419 = chcollballcontrol,
   não chintroticker) e depende do data de overlay estar mapeado em endereço
   link-time.

## Comparação

EXPECTED (original, ROM):

- `gemarkersDll_entrypoint_0(419)` → `_chcollballcontrol_entrypoint_3`
  (0x80086EB0).
- Construtor retorna `D_80804FA0_chcollballcontrol`.
- `MEM_W(0x38, ActorInfo)` = `func_80107C2C` (0x80107C2C) — não-nulo.
- `jalr` chama `func_80107C2C` (construtor genérico de actor).

ACTUAL (TooieRecomp runtime):

- `gemarkers(419) = 0x80086EB0` → `ovl=467 ep=3` (chcollballcontrol).
- Construtor recompilado retorna `0x80804FA0` (constante link-time).
- `MEM_W(0x38, 0x80804FA0) = 0` — o data do overlay 467 **não** está mapeado
  em `0x80804FA0` (overlay carregado no heap em `0x80198470`).
- `LOOKUP_FUNC(0)` → `get_function(0)` → `Failed to find function at
  0x00000000` → exit 139.

Instrumentação de diagnóstico (removida após coleta):

```
[tooie-intro] diag gemarkers(419)=0xffffffff80086eb0
[tooie-intro] diag constructor returned 0xffffffff80804fa0
[tooie-intro] diag ActorInfo[0x38]@0xffffffff80804fa0 = 0x0
[tooie-intro] diag ROM[0x2087818]=0x80107c2c (link-time D_80804FD8)
```

## Diagnóstico

NULL CALLBACK SEMANTICS: **INVALID**

CONFIDENCE: **HIGH**

- O valor 0 **não** é um callback opcional legítimo: o ROM contém
  `func_80107C2C` no campo lido.
- O 0 é consequência de o **data section do overlay** (que contém o ActorInfo)
  não estar acessível no endereço link-time `0x80804FA0` no runtime do
  TooieRecomp. O construtor recompilado hardcoda `0x80804FA0`, mas o
  `ovl_load` host carrega o overlay no heap (ex.: `0x80198470`).
- Descoberta adicional: o auto-spawn usa o **marker id 419**, que resolve para
  `chcollballcontrol_entrypoint_3` — **não** chintroticker. O marker id de
  chintroticker é **569** (0x239).

## Alteração realizada

Nenhuma alteração funcional — diagnóstico apenas.

A instrumentação temporária adicionada em `src/game/syscall_handler.cpp` foi
removida; o arquivo voltou ao estado do HEAD.

## Build

Comando: `ninja -C build TooieRecompiled`

Exit code: `0`

## Smoke

Comando:
`timeout 25 env TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 ./build/TooieRecompiled`

Resultado (com instrumentação de diag):

- Crash reproduzido em `func_80108C90` → `get_function(0)` (exit 139).
- `ovl=793 ep=0` (gemarkersDll) → `ovl=467 ep=3` (chcollballcontrol ep3).
- Diag: `gemarkers(419)=0x80086eb0`, construtor → `0x80804fa0`,
  `ActorInfo[0x38]=0`, ROM `D_80804FD8=0x80107c2c`.
- Após remoção da instrumentação, o comportamento volta ao estado anterior.

## Regressões observadas

Nenhuma (sem alteração funcional).

## Próxima ação

Investigar o mapeamento de **data de overlays**: confirmar se o `ovl_load` host
deveria copiar o data section do overlay para a região link-time (base
`0x80800000`) e, se sim, corrigir nessa camada; depois reavaliar o auto-spawn
usando o marker id correto de chintroticker (569, não 419).
