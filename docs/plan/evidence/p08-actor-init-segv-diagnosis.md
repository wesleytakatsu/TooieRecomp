---
title: "P08 — diagnóstico do SIGSEGV na inicialização de actor"
description: "Prova com GDB que o SIGSEGV em vector_push_back (via func_80106668/func_80107C2C) é artefato do spawn diagnóstico: D_80136EE0 (vetor global de Actors) está NULL porque o initializer (chbaddieDll ep1 → func_800FF390 → func_801065E0) nunca executou. Natural flow diverge antes. Não é bug de vector/runtime."
status: evidence
phase: P08
updated: 2026-08-10
tags: [banjo-tooie-recomp, p08, actors, vector_push_back, func_80106668, func_80107C2C, D_80136EE0, chbaddieDll, actor-manager]
---

> ## SUPERSEDED / HISTORICAL CONTEXT
>
> As observações de "actor manager init = 0 hits" deste documento foram
> obtidas no estado antigo do projeto (`a6053a0` / janela correspondente).
> No HEAD `c43e89b`, após fixes posteriores de overlay-cache (`5ac74c0`) e
> scheduler (`f9d1c24`), o fluxo natural foi
> revalidado:
>
> ```
> chbaddieDll_entrypoint_1
>   -> func_800FF390
>   -> func_801065E0
>   -> vector_new
>   -> D_80136EE0 válido (0x801df080)
> ```
>
> Portanto "actor manager nunca inicializa" **NÃO é blocker atual**. Este
> documento permanece válido como registro histórico do crash causado pelo
> spawn diagnóstico naquele estado. Ver
> `p08-actor-manager-init-divergence.md`.

# P08 — diagnóstico do SIGSEGV na inicialização de actor

## Objetivo

Diagnosticar a PRIMEIRA DIVERGÊNCIA que causa o novo SIGSEGV no caminho:

```
func_80108C90 (spawn por marker)
  -> func_80107C2C (construtor genérico de actor)
    -> func_80106668 (alocador de slot de Actor)
      -> vector_push_back
        -> SIGSEGV
```

Pergunta central: o SIGSEGV ocorre por um defeito real/genérico na
infraestrutura de actors/vector do TooieRecomp, ou porque o spawn diagnóstico
de marker 419 está sendo executado com timing/estado que não corresponde a um
spawn legítimo do jogo original?

## Estado inicial

- Spawn diagnóstico mantido: `func_80108C90(419, scratch, 0, 0)` no retorno de
  `gcsectionDll:4` em `MAP_18B` (`src/game/syscall_handler.cpp`). Marker 419 =
  chcollballcontrol (overlay 467). Sem mudança para 569.
- Falha observada: SIGSEGV em `vector_push_back` (`func_80106668` em
  `func_80107C2C`), exit 139.
- Antes: relocation de overlay RESOLVIDA (`ActorInfo[0x38] == 0x80107C2C`);
  `LOOKUP_FUNC(0)` não ocorre mais.

## Primeiro teste discriminador

### AUTO-SPAWN ON (gdb, contagem de hits)

Breakpoints em `vector_push_back`, `func_80106668`, `func_80107C2C`,
`func_80108C90`, `func_801065E0` (init do vetor), `func_800FF390` (actor
manager init), `chbaddieDll_entrypoint_1` (ovl 181 ep1). Env:
`TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1`.

Comando:
```
gdb -batch -x /tmp/opencode/gdb_count.gdb --args ./build/TooieRecompiled
```

Resultado:

| função | hits antes do hook |
| --- | --- |
| `vector_push_back` | **1** (o próprio spawn artificial) |
| `func_80106668` | 0 (natural) |
| `func_80107C2C` | 0 (natural) |
| `func_80108C90` | só a chamada do hook |
| `func_801065E0` (vector_new/D_80136EE0) | **0** |
| `func_800FF390` (actor manager init) | **0** |
| `chbaddieDll_entrypoint_1` (ovl 181 ep1) | **0** |

- **O primeiro e único uso do caminho de alocação de actor é o spawn artificial.**
- **O initializer do vetor de actors nunca executou.**
- Classificação do teste discriminador: **B** — o primeiro uso desse caminho é o
  spawn artificial; não há chamadas naturais anteriores.

### AUTO-SPAWN OFF (gate temporário `TOOIE_NO_AUTOSPAWN`)

Gate adicionado temporariamente em `syscall_handler.cpp`
(`&& !std::getenv("TOOIE_NO_AUTOSPAWN")`), buildado, executado, e **revertido**
antes do handoff.

Comando:
```
gdb -batch -x /tmp/opencode/gdb_count_off.gdb --args ./build/TooieRecompiled
```

Resultado (45 s):

| observação | valor |
| --- | --- |
| SIGSEGV em vector_push_back | **NÃO ocorre** |
| `vector_push_back` natural | 15 hits — em OUTROS vetores (`0x8012AB00`, `0x8004565C`, `0x80127EF8`), **nunca** em `D_80136EE0` |
| `func_80106668` (slot de actor) | **0** hits |
| `func_801065E0` (init D_80136EE0) | **0** hits |
| `func_800FF390` (actor manager init) | **0** hits |
| `chbaddieDll_entrypoint_1` | **0** hits |
| progresso | muito maior: boot, attract (gsattract 731:4), seção 0x18B (677:0-4), glintrosync, dezenas de overlays carregados |
| **nova falha** | SIGSEGV em **`gcsectionDll_entrypoint_4`** (de `glintrosync:6`), leitura de seção com ponteiro inválido |

Conclusões do controle:

1. **O SIGSEGV em vector_push_back desaparece** com o spawn OFF → causalidade
   provada: o crash é do spawn artificial, não do fluxo natural.
2. **O vetor de actors (D_80136EE0) nunca é inicializado nem no fluxo natural**
   dentro da janela observada.
3. O fluxo natural, porém, **diverge antes** — SIGSEGV diferente em
   `gcsectionDll:4` (chamado de `glintrosync:6`), lendo o ponteiro de seção
   retornado por `func_808000F0` com valor inválido (`0x7ffec4400003` em rax —
   não é endereço MIPS válido). **Este é um segundo ponto de divergência,
   anterior à init do actor manager, não diagnosticado nesta etapa.**

## Fault exato

Captura gdb (AUTO-SPAWN ON), thread `[Game] MAIN` (Thread 44):

```
== vector_push_back entry: vecptr(r4)=0xffffffff80136ee0 D_80136EE0=0x0
Thread 44 "[Game] MAIN" received signal SIGSEGV.
rip = 0x555555f6169e <vector_push_back+110>
rax = 0x0            (vec = *(&D_80136EE0) = NULL)
rcx = 0xffffffff80136ee0   (vecptr arg = &D_80136EE0)
rdx = 0x7ffec4400000 (rdram)
r12 = 0x80000008     (offset constante)
instrução: movslq (%r12,%rdx,1),%rcx   => lê em rdram + 0x80000008 (N64 addr 0x8)
backtrace: vector_push_back <- func_80106668 <- func_80107C2C <- func_80108C90
           <- recomp_syscall_handler (hook) <- gclevel_entrypoint_2 <- gclevel_entrypoint_0
           <- recomp_syscall_handler <- func_800A7BB8 <- func_8001209C
```

Campos:

```
FAULTING FUNCTION:  vector_push_back (MIPS 0x800B2974)
FAULTING INSTRUCTION: movslq (%r12,%rdx,1),%rcx   (MIPS lw $a3, 0x8($a2) @ 0x800B2988)
FAULTING ACCESS:     READ em rdram+0x80000008 (N64 addr 0x8 = campo vec->end de um Vector NULL)
VECTOR OBJECT:       D_80136EE0 @ 0x80136EE0 (o Vector* global; valor = 0)
VECTOR DATA:         0 (NULL — nunca alocado)
VECTOR SIZE:         n/a
VECTOR CAPACITY:     n/a
VALUE/ELEMENT:       n/a (falha no primeiro deref de *vec_ptr)
```

- `vector_push_back(Vector** vec_ptr)`: `vec = *vec_ptr` → `vec->end` (offset
  +8) → leitura em `0 + 8` → host `rdram + 0x80000008` → fora do buffer → SIGSEGV.
- **VECTOR/RUNTIME SEMANTICS: DESCARTADO.** A implementação está correta
  (decomp `src/core2/vector.c:39`); ela simplesmente recebeu um `Vector*` NULL
  porque o vetor nunca foi criado.

## func_80106668 no original

`func_80106668` (`RecompiledFuncs/funcs_145.c:3825`, MIPS `0x80106668`):

```c
*(a0) = 0;                       // saída = índice
elem = vector_push_back(&D_80136EE0);   // 1ª chamada — aloca slot
idx  = vector_index_of(D_80136EE0, elem);
*(a0) = idx;
bzero(elem, 0x9C);               // zera o Actor (0x9C = sizeof Actor)
return elem;
```

- **O que adiciona ao vetor:** um slot de `Actor` (0x9C bytes) na lista global.
- **Qual vetor/container:** `D_80136EE0` = `Vector*` global em BSS core2
  (`core2bss2.s`, `.space 0x10`). Elemento 0x9C, pré-alocado 0x1E (30) via
  `vector_new(0x9C, 0x1E)`.
- **Onde fica:** 0x80136EE0 (rdram), BSS.
- **Subsistema:** actor manager (lista global de Actors usada pelo construtor
  genérico `func_80107C2C`).
- **Quem cria/inicializa:** `func_801065E0` = `vector_new(0x9C, 0x1E)` +
  `D_80136EE0 = v0`. ÚNICO caminho.
- **Quem destroi/reseta:** `func_80106608` = `vector_free` + `D_80136EE0 = 0`
  (via `func_800FF564`/`chbaddieDll ep2`).

## Contrato de func_80108C90

Callsites reais do original:

| callsite | a0/r4 | a1/r5 | a2/r6 | a3/r7 |
| --- | --- | --- | --- | --- |
| `collectDll.c:141` | marker `0x21F` | ponteiro pos `s32[3]` | `0U` | `NULL` |
| `collectDll.c:201` | marker | ponteiro pos | `arg0->FlagOrRotation` | `arg0` (prop) |
| `func_80108454` | marker | pos | flags | `0` (zero) |
| `func_80108474` | marker | pos (calc) | flags | `0` |
| `func_801085CC` | marker | pos (conv s16→s32) | flags | `0` |

**Contrato:** `a0` = marker id; `a1` = ponteiro para posição (s32[3]); `a2` =
flags/contexto (frequentemente 0); `a3` = owner/prop (frequentemente NULL/0).

**Hook atual:**

| arg | valor | classificação |
| --- | --- | --- |
| r4 (marker) | `419` (chcollballcontrol) | **VALID** (id dentro da faixa; marcador deliberado de regressão) |
| r5 (pos) | `scratch` (0x8007D800, {0,0,0}) | **VALID** (ponteiro para s32[3]; origem (0,0,0) plausível) |
| r6 (flags) | `0` | **VALID** (igual a vários callsites) |
| r7 (owner) | `0` | **VALID** (igual a NULL em vários callsites) |

**Conclusão:** argumentos do hook são **VALID** segundo o contrato real. O
crash **NÃO** é classe B (invalid spawn arguments): `func_80107C2C` falha antes
de usar os argumentos, na primeira chamada (`func_80106668` → push_back no
vetor NULL).

## Inicialização esperada do container

EXPECTED (original):

- `D_80136EE0` (Vector* global) deveria ser criado por
  `func_801065E0` (`vector_new(0x9C, 0x1E)`) → `D_80136EE0 != 0`, com
  `element_size=0x9C`, `begin=end=data`, `mem_end=data+0x1E*0x9C`.
- Único caminho: `chbaddieDll_entrypoint_1` (ovl 181 ep1) →
  `func_800FF390` (actor manager init) → `func_801065E0`.

ACTUAL (TooieRecomp, no momento do crash / janela observada):

- `D_80136EE0 == 0` (confirmado via `x/wx rdram+0x136EE0` = 0).
- `func_801065E0`/`func_800FF390`/`chbaddieDll ep1`: **0 hits** em ON e OFF.
- **Initializer NUNCA executou** em nenhuma execução observada.

Estado da rotina de inicialização:
- A. executou em outro endereço? **Não** (0 hits).
- B. executou sem alterar memória? **Não** (nunca chamada).
- C. executou relocada? **Não**.
- D. corrompida depois? **Não aplicável** (nunca criada).
- **E. não executou / não provado quando deveria** → ver Diagnóstico.

## Timing do hook

- O hook dispara no **retorno de `gcsectionDll:4`** (ovl 677 ep4) para
  `MAP_18B` — fase de carga de seção/mapa, durante `gclevel_entrypoint_0/2`
  (backtrace do crash).
- No fluxo natural (OFF), mesmo após 45 s e bem mais progresso (glintrosync),
  o actor manager **não** foi inicializado — e o fluxo diverge/crash em
  `gcsectionDll:4` (de `glintrosync:6`) antes de chegar à init de actors.
- **Não provado** que, no fluxo original, a init do actor manager já deveria ter
  ocorrido antes do ponto do hook. O spawn artificial roda num ponto em que o
  subsystem de actors ainda não existe no TooieRecomp (e o fluxo natural
  também não o alcança dentro da janela).

## Expected vs Actual

```
EXPECTED:  D_80136EE0 != 0 (Vector de Actors criado por chbaddieDll ep1 ->
           func_800FF390 -> func_801065E0) antes de qualquer actor spawn;
           vector_push_back(&D_80136EE0) aloca slot com sucesso.

ACTUAL:    D_80136EE0 == 0; initializer nunca executou; o spawn artificial é o
           primeiro uso do caminho; vector_push_back recebe Vector* NULL e
           lê vec->end em 0x8 -> SIGSEGV.
```

## Diagnóstico

```
ROOT CAUSE:  A (diagnostic spawn timing) primário — o hook dispara o spawn
             antes de o actor manager estar inicializado.
             C (actor subsystem initialization) é candidata, mas NÃO provada
             com HIGH (ver abaixo).
CONFIDENCE:  MEDIUM

DIAGNOSTIC SPAWN ARTIFACT:  YES
```

Racional:

1. **O SIGSEGV em vector_push_back é artefato do spawn diagnóstico.** Com
   AUTO-SPAWN OFF o crash some; o vetor `D_80136EE0` fica NULL; a infraestrutura
   de vector está correta (recebeu NULL, não tem bug). Não é VECTOR/RUNTIME
   SEMANTICS (D), nem MEMORY/RELOCATION (E), nem STATE CORRUPTION EARLIER (F).
2. **Args do hook são VALID** per o contrato real de `func_80108C90` → não é
   INVALID SPAWN ARGUMENTS (B).
3. **A vs C:** provado que o initializer **nunca executou**. Porém NÃO foi
   provado que ele **deveria ter executado antes deste ponto** segundo o fluxo
   original — o fluxo natural do TooieRecomp diverge/crash antes
   (`gcsectionDll:4` via glintrosync), e não há evidência suficiente do ponto
   legítimo da init no intro. Portanto, conforme regra da etapa: "initializer
   nunca executou" ≠ "initializer deveria ter executado antes deste ponto".
   A (spawn dispara antes do momento legítimo) é a leitura mais consistente com
   o comportamento observado, mas não fecha C com HIGH.
4. Fica **C candidata / A**, CONFIDENCE **MEDIUM**. Próxima etapa: localizar a
   divergência que impede `chbaddieDll_entrypoint_1`/actor-manager init (e o
   segundo SIGSEGV em `gcsectionDll:4` de glintrosync).

## Alteração

Nenhuma alteração funcional — diagnóstico apenas.

- Gate temporário `TOOIE_NO_AUTOSPAWN` adicionado em
  `src/game/syscall_handler.cpp` para o teste de controle e **revertido**.
- `git diff src/game/syscall_handler.cpp` limpo (sem mudanças).

## Build

Comando: `ninja -C build TooieRecompiled`

Exit code: `0`

## Smoke

AUTO-SPAWN ON (baseline):
`timeout 60 env TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 ./build/TooieRecompiled`

- Exit `139` (SIGSEGV esperado em vector_push_back).
- `[tooie-intro] spawning marker 419 ...`; `Failed to find function at 0x00000000` **0×**.

AUTO-SPAWN OFF (controle):
- Exit: SIGSEGV em site **diferente** (`gcsectionDll:4` via glintrosync), exit 139.
- SIGSEGV em vector_push_back **não ocorre**.

## Regressões

- `LOOKUP_FUNC(0)`: **YES — sem regressão** (0×; ActorInfo[0x38]==0x80107C2C
  continua resolvido).
- Overlay relocation (851 seções, RELOC_HI16/LO16): intacta.
- Nenhuma mudança de código mantida.

## Próxima única etapa

Localizar a divergência que impede `chbaddieDll_entrypoint_1` (ovl 181 ep1) /
actor-manager init (`func_800FF390`/`func_801065E0`) de executar no fluxo do
TooieRecomp — e/ou diagnosticar o segundo SIGSEGV observado em
`gcsectionDll_entrypoint_4` (chamado de `glintrosync:6`), que ocorre antes da
init de actors no fluxo natural. NÃO forçar a init manualmente; NÃO mudar
marker 419 para 569; NÃO avançar para chintroticker/Rare logo.
