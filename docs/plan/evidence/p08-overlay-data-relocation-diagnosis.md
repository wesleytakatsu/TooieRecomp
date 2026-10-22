---
title: "P08 — diagnóstico: relocation de data de overlay perdida (chcollballcontrol)"
description: "Prova que o construtor recompilado devolvia 0x80804FA0 (link-time) porque o pipeline de metadados truncava nomes de seção; relocs existiam no ELF. Fix na camada generator/metadata; ActorInfo[0x38] == 0x80107C2C."
status: evidence
phase: P08
updated: 2026-08-10
tags: [banjo-tooie-recomp, p08, actors, overlay-data, relocations, generate-metadata, chcollballcontrol, func_80108C90]
---

# P08 — diagnóstico: relocation de data de overlay perdida (chcollballcontrol)

## Objetivo

Descobrir por que o construtor recompilado de `chcollballcontrol` retorna o
endereço link-time `0x80804FA0` em vez de uma referência válida ao ActorInfo do
overlay 467 carregado dinamicamente.

## Resultado

**ROOT CAUSE: A — relocation metadata ausente/incorreta.**

As relocations **NÃO foram perdidas nem corrompidas dentro do ELF**. Elas
continuam presentes no ELF, mas **não foram CONSUMIDAS pelo N64Recomp** porque
a seção deixou de ser reconhecida como relocatable: o gerador de metadados
invocava `readelf -S` sem `-W`, e o readelf truncava nomes de seção longos para
`Nome[...]`. O nome truncado nunca casava com o nome real no ELF, a seção não
entrava na lista relocatable consumida pelo N64Recomp e o `.rel` da seção era
descartado.

## Contexto

- [[p08-actor-null-callback-diagnosis]] — prova que o `ActorInfo[0x38] == 0`
  era inválido (callback nulo); esta etapa resolve a origem desse 0.
- [[p08-session-handoff]] — handoff pré-fix: a decisão era investigar o
  mapeamento de data de overlays no `ovl_load` host.
- Etapa [[../../plans/BT/03-recompiler-metadata|P03 — metadados do recompilador]]:
  camada onde o bug foi corrigido (gerador de metadados).

## Cadeia exata da causa raiz

```
ELF possui relocations corretas (.rel.chcollballcontrol, 779 entradas)
  ->
readelf -S (sem -W) trunca nomes de seção longos para "Nome[...]"
  ->
.chcollballcontrol vira ".chcollballc[...]" na listagem parseada
  ->
seção real não entra corretamente na lista relocatable consumida pelo N64Recomp
  ->   (nome truncado != nome real; lista .recomp.txt nem contém a entrada)
N64Recomp não associa .rel.chcollballcontrol à seção 1840
  ->   (elf.cpp só lê o .rel de seções na lista relocatable)
codegen trata os immediates como R_MIPS_NONE
  ->   (recompilation.cpp: section.relocs vazio -> reloc_type = R_MIPS_NONE)
C gerado contém 0x80804FA0 literal
  ->   (lui 0x8080; addiu 0x4FA0 viram constantes link-time)
runtime acessa endereço link-time em vez do DATA relococado no heap
  ->   (overlay 467 carregado em 0x80198470+; 0x80804FA0 é memória de jogo não mapeada)
ActorInfo[0x38] = 0
  ->
LOOKUP_FUNC(0)
  ->
crash (Failed to find function at 0x00000000, exit 139)
```

**Ponto central:** as relocations continuam no ELF; o elo quebrado é a
**associação seção→reloc** no N64Recomp, que depende do nome da seção estar na
lista relocatable com o nome **exato** do ELF.

## Endereços

| item | valor |
| --- | --- |
| Link-time (ActorInfo `D_80804FA0_chcollballcontrol`) | `0x80804FA0` |
| Runtime (ActorInfo, heap overlay 467) | `0x8019D410` *(runtime-example)* |
| ROM do dado | `0x20877E0` (decompressed.us.z64) |
| Campo `ActorInfo[0x38]` (callback) | `0x80107C2C` (`func_80107C2C`) |

## Instruções MIPS originais

`banjo-tooie-Recomp/asm/nonmatchings/overlays/ch/collballcontrol/chcollballcontrol_entrypoint_3.s`:

```asm
/* 2085A74 80803234 3C028080 */  lui        $v0, %hi(D_80804FA0_chcollballcontrol)
/* 2085A78 80803238 03E00008 */  jr         $ra
/* 2085A7C 8080323C 24424FA0 */   addiu     $v0, $v0, %lo(D_80804FA0_chcollballcontrol)
```

## Relocation records (ELF)

`readelf -r build/us/banjotooie_decompressed.elf` — seção
`.rel.chcollballcontrol` (779 entradas: 552 R_MIPS_26, 79 R_MIPS_32,
74 R_MIPS_HI16, 74 R_MIPS_LO16) contém os pares relevantes:

```
80803234  R_MIPS_HI16  80804fa0
8080323C  R_MIPS_LO16  80804fa0
808031D8  R_MIPS_HI16  80804c98   (D_80804C98, mesma seção)
```

## Evidence before / after

### ANTES (metadata + codegen antigos)

**readelf sem `-W`** — nome observado para a seção do overlay:
```
.chcollballc[...]
```

**metadata (HEAD do git):**
| arquivo | count | observação |
| --- | --- | --- |
| `relocatable_sections.txt` | 830 | 750 nomes completos + 80 nomes truncados `[...]` |
| `relocatable_sections.recomp.txt` | 750 | subconjunto com os 750 nomes legíveis; **`.chcollballcontrol` ausente** |

Origem dos números (auditoria dos conjuntos — seção seguinte):

- **830** = o que o parser antigo lia de `readelf -S` sem `-W`: 750 nomes
  legíveis + 80 strings truncadas distintas.
- **750** = o subconjunto de nomes que o readelf exibia completos. O
  `relocatable_sections.recomp.txt` foi derivado **descartando** as 80 strings
  truncadas — um subconjunto histórico/manual **sem justificativa funcional**,
  não uma allowlist curada. Nenhuma seção foi excluída deliberadamente.
- **80 truncadas** representam **101 seções reais**: nomes com ≥ 18 caracteres
  (101 seções) são truncados a 12 caracteres + `[...]`, e 39 prefixos de 12
  caracteres são compartilhados por 2+ seções → 101 seções viram só 80 strings
  distintas. **21 seções** ficam ocultas atrás de prefixos compartilhados.

**C gerado** de `chcollballcontrol_entrypoint_3`
(`RecompiledFuncs/funcs_383.c`, seção 1840):
```c
// 0x80803234: lui $v0, 0x8080
ctx->r2 = S32(0X8080 << 16);
// 0x8080323C: addiu $v0, $v0, 0x4FA0
ctx->r2 = ADD32(ctx->r2, 0X4FA0);
```

**runtime:**
```
ActorInfo = 0x80804FA0 (constante link-time)
ActorInfo[0x38] = 0
LOOKUP_FUNC(0) -> Failed to find function at 0x00000000 -> exit 139
```

### DEPOIS (generator corrigido + regeneração)

**readelf `-SW`** — `.chcollballcontrol` completo (18 caracteres).

**metadata (worktree):**
| arquivo | count |
| --- | --- |
| `relocatable_sections.txt` | **851** |
| `relocatable_sections.recomp.txt` | **851** (idêntico por construção) |

- **851** = conjunto esperado: todas as seções executáveis (flag `X`) com
  endereço VRAM `0x80800000` e nome começando com `.` — confirmado por
  `readelf -W` (policy set do generator). `851 = 750 (≤17 chars) + 101 (≥18 chars)`.
- **Por que -W dá 851:** com `-W` não há truncamento; os 101 nomes longos
  aparecem completos. `851` casa **exatamente** o policy set de executable
  overlays em `0x80800000` (verificado pelo teste invariante).

**C gerado** (mesma função, regenerada):
```c
// 0x80803234: lui $v0, 0x8080
ctx->r2 = S32(RELOC_HI16(1840, 0X4FA0) << 16);
// 0x8080323C: addiu $v0, $v0, 0x4FA0
ctx->r2 = ADD32(ctx->r2, (int16_t)RELOC_LO16(1840, 0X4FA0));
```

**runtime:**
```
ActorInfo = 0x8019D410 (heap, via RELOC)
ActorInfo[0x38] = 0x80107C2C
LOOKUP_FUNC(0) -> não ocorre mais nesse ponto (func_80107C2C executa)
```

## Auditoria dos conjuntos de seções

| métrica | valor |
| --- | --- |
| OLD `relocatable_sections.txt` count | **830** |
| OLD `relocatable_sections.recomp.txt` count | **750** |
| NEW canonical expected count (policy set do ELF) | **851** |
| NEW `relocatable_sections.txt` count | **851** |
| NEW `relocatable_sections.recomp.txt` count | **851** |
| Added due to truncated names (strings `[...]` → nomes completos) | 80 strings → 101 nomes |
| Added because old readelf parser did not parse the line | **0** (nenhuma linha era pulada; todas eram lidas, com nome truncado) |
| Added por nomes ocultos atrás de prefixos compartilhados | 21 (101 seções longas − 80 strings truncadas) |
| Removed | **0** |
| Unexpected differences | **nenhuma** (851 == policy set; testado) |

A antiga lista de 750 **não era uma allowlist deliberada**: era o subconjunto de
nomes que o readelf exibia completos, e as seções longas foram simplesmente
perdidas. Não houve exclusão intencional a restaurar.

## As duas listas (relocatable_sections.txt vs .recomp.txt)

| pergunta | resposta |
| --- | --- |
| Fonte autoritativa | ELF `banjotooie_decompressed.elf` → policy set computado por `write_relocatable_sections` |
| Quem gera | `tools/generate_metadata.py` (ambas, da **mesma coleção em memória**) |
| Config que consome `.txt` | `tooie.us.rev0.dump.toml` (dump de contexto N64Recomp) |
| Config que consome `.recomp.txt` | `tooie.us.rev0.recomp.toml` (recompilação real) |
| Por que ambas existem | histórico: P04 criou `.recomp.txt` como twin consumido pela recomp; mantidas **idênticas por construção** |

A partir desta etapa o generator grava as duas do mesmo `set` em memória
(`write_relocatable_sections`), então elas **não podem divergir** por edição
paralela. O teste invariante também asserta `recomp.txt == txt`.

## Causa raiz (camadas)

1. **metadata/generator** — `tools/generate_metadata.py::run_readelf_sections`
   chamava `mips-linux-gnu-readelf -S` **sem `-W`**. Sem `-W`, o readelf trunca
   nomes de seção com ≥ 18 caracteres para 12 caracteres + `[...]` (1285 nomes
   com `[...]` no ELF inteiro; com `-W`, zero).
2. **metadata** — `write_relocatable_sections` gravava os nomes truncados em
   `metadata/relocatable_sections.txt`; o twin `.recomp.txt` (derivado) ficava
   com 750 nomes e sem `.chcollballcontrol`.
3. **ELF → N64Recomp** — `elf.cpp:296/313` só marca uma seção como relocatable
   (e só lê seu `.rel`) se o **nome da seção** estiver na lista.
   `.chcollballc[...]` ≠ `.chcollballcontrol` → seção não relocatable →
   `.rel.chcollballcontrol` descartado → `recomp_overlays.inl` com
   `.relocs = nullptr`.
4. **codegen** — `recompilation.cpp:165` (`section.relocs.size() > 0`) →
   `reloc_type = R_MIPS_NONE` → codegen emite o immediate literal (constante
   link-time).
5. **runtime** — construtor retorna `0x80804FA0`; data do overlay está no heap
   (`0x80198470+`); `ActorInfo[0x38]` lido em link-time = 0 → `LOOKUP_FUNC(0)`.

Distinção de camadas: **ELF correto** (relocs presentes) · **metadata**
rompida (nomes truncados/ausentes) · **codegen** correto mas sem a reloc
(não a recebeu) · **runtime** correto (`section_addresses[section]` =
base heap do texto).

## Math da relocation no runtime (prova de que NÃO é preciso alias/cópia do DATA)

Valores reais observados na execução de smoke (endereços de heap são
**runtime-example** — variam por alocação):

| símbolo | valor | origem |
| --- | --- | --- |
| overlay/package runtime base (`overlay_ptr`) | `0x80198130` | derivado: `text_start − text_off` |
| `text_off` | `0x340` | header do pacote (ne=6, nr=363, unk32=0, name_len=18) → `0x38 + 6*4 + 18` + pads + `363*2` + pad16 |
| `text_start` (base do texto no heap) | `0x80198470` | log `ovl_load: idx=467 pkg=0x2082500 +0x340 -> ram=0x80198470` |
| link-time ActorInfo | `0x80804FA0` | símbolo ELF `D_80804FA0_chcollballcontrol` |
| offset relativo (dentro da seção 1840) | `0x4FA0` | `0x80804FA0 − 0x80800000` |
| offset do ActorInfo dentro do pacote | `0x52E0` | `text_off + 0x4FA0 = 0x340 + 0x4FA0` (= `0x20877E0 − 0x2082500`) |
| section index (N64Recomp == ELF) | `1840` | `recomp_overlays.inl` `.index = 1840` |
| `section_addresses[1840]` | `0x80198470` *(runtime-example)* | `load_overlay` seta = `ram` passado por `load_overlays` (= `text_start`) |
| computed runtime ActorInfo | `0x8019D410` | `section_addresses[1840] + 0x4FA0 = 0x80198470 + 0x4FA0` |
| `ActorInfo + 0x38` | `0x8019D448` | `0x8019D410 + 0x38` |
| `MEM_W(0x38, 0x8019D410)` | `0x80107C2C` | word copiado do ROM `0x2087818` (= `D_80804FD8`) |

Conclusão: `RELOC_HI16/LO16(section, offset)` resolve
`section_addresses[section] + offset`; como `section_addresses[1840]` aponta
para o texto do overlay **no heap**, `+0x4FA0` cai exatamente sobre o DATA
também no heap (`overlay_ptr + 0x52E0`). **Não é necessária cópia/alias de
DATA em `0x80800000`** — o N64Recomp reloca a **referência** (resposta E).

## Proof: o codegen mudou

| | antes | depois |
| --- | --- | --- |
| arquivo | `RecompiledFuncs/funcs_383.c` | `RecompiledFuncs/funcs_383.c` |
| função | `chcollballcontrol_entrypoint_3` | `chcollballcontrol_entrypoint_3` |
| endereços MIPS | `0x80803234` / `0x8080323C` | `0x80803234` / `0x8080323C` |
| emite | `S32(0X8080 << 16)` + `ADD32(ctx->r2, 0X4FA0)` | `RELOC_HI16(1840, 0X4FA0)` + `RELOC_LO16(1840, 0X4FA0)` |
| section index | — (constante) | `1840` |
| relocation target | — | `D_80804FA0_chcollballcontrol` (offset `0x4FA0` da seção) |

Seção 1840 em `RecompiledFuncs/recomp_overlays.inl`:
`.relocs = nullptr, .num_relocs = 0` → `.relocs = section_1840_chcollballcontrol_relocs`.

`RecompiledFuncs/` continua **generated evidence** — nunca editado
manualmente; apenas regenerado por `tools/run_recomp.sh`.

## Guarda de regressão (invariantes)

`tools/test_metadata_invariants.py` agora valida **semanticamente contra o
ELF**, não só existência/sort/unique:

- a lista está ordenada, sem duplicatas, não vazia;
- **nenhum nome contém `[...]`** (truncamento proibido);
- **`actual == expected`**: o conteúdo é **exatamente** o policy set do ELF
  (flag `X` ∧ addr `0x80800000` ∧ nome começa com `.`), via `readelf -W`;
- `relocatable_sections.recomp.txt == relocatable_sections.txt` (twin em sync).

Comando: `python3 tools/test_metadata_invariants.py` — **EXIT 0, PASS**.
Snapshot `tests/snapshots/metadata_snapshot.json` atualizado
(`reloc_section_count = 851`).

## Alteração

Camada causadora: **generator/metadata**.

- `tools/generate_metadata.py`:
  - `run_readelf_sections`: adicionado `-W` ao readelf.
  - `write_relocatable_sections`: grava o `relocatable_sections.txt` e o twin
    `relocatable_sections.recomp.txt` da mesma coleção em memória.
- `tools/test_metadata_invariants.py`: policy-set equality + proibição de `[...]`
  + twin em sync.
- `metadata/relocatable_sections.txt` / `.recomp.txt` regenerados (851).
- `RecompiledFuncs/` regenerado.
- `src/game/generated/overlay_dispatch.gen.cpp` regenerado — **BYTE-IDENTICAL /
  UNCHANGED** (git diff vazio; o script só lê rom/ram/size/entrypoints).
- `src/game/syscall_handler.cpp`: comentário/log do auto-spawn corrigido
  (marker id 419 = chcollballcontrol, **não** chintroticker); **419 mantido**
  como caso de regressão. Sem mudança funcional.
- Nenhum arquivo gerado foi editado manualmente.

## Reproducibilidade (comandos, ordem, exit codes)

| passo | comando | exit | resultado |
| --- | --- | --- | --- |
| metadata | `python3 tools/generate_metadata.py` | 0 | "Wrote metadata … relocatable sections: 851" |
| metadata (re-run) | `python3 tools/generate_metadata.py --skip-dump` | 0 | idem (dump refeito; dump.toml não existe) |
| invariantes | `python3 tools/test_metadata_invariants.py --write-snapshot` | 0 | PASS (snapshot 851) |
| invariantes | `python3 tools/test_metadata_invariants.py` | 0 | PASS (policy-set equality) |
| recomp | `./tools/run_recomp.sh` | 0 | 420 arquivos, ~90M |
| dispatch | `python3 tools/generate_overlay_dispatch.py` | 0 | **BYTE-IDENTICAL / UNCHANGED** |
| freshness | `python3 tools/check_generated_fresh.py` | 0 | "RecompiledFuncs is newer than inputs" |
| syscall tests | `python3 tools/test_syscall_decode.py` | 0 | all passed |
| build | `ninja -C build TooieRecompiled` | 0 | 424 targets, link OK |
| smoke | `timeout 40 env TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 ./build/TooieRecompiled` | 139 | `Failed to find function at 0x00000000` **0×**; progride até ovl=467 ep=3; nova falha em `vector_push_back` |

## Smoke

Comando (mesmo de [[p08-actor-null-callback-diagnosis]]):
`timeout 40 env TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 ./build/TooieRecompiled`

Resultado:

- `Failed to find function at 0x00000000`: **0 ocorrências** (antes: crash
  exit 139 imediato).
- Fluxo atinge `ovl=467 ep=3` (construtor chcollballcontrol) e retorna.
- Log novo: `[tooie-intro] spawning marker 419 (chcollballcontrol, NOT
  chintroticker) on MAP_18B...`.
- gdb (breakpoint em `func_80107C2C`): `ctx->r2` (ActorInfo) =
  `0xffffffff8019d410`, `ctx->r25` (callback) = `0xffffffff80107c2c`.
- **`func_80108C90` ultrapassa o LOOKUP_FUNC específico** e chama
  `func_80107C2C`.

## Novo ponto de parada (SIGSEGV — sem diagnóstico nesta etapa)

Após corrigir a relocation, a execução revela uma **nova falha** em
`vector_push_back` chamada por `func_80106668` durante `func_80107C2C`. A causa
desse novo SIGSEGV **ainda não foi diagnosticada**.

Stack observado:
```
func_80108C90
  -> func_80107C2C
    -> func_80106668
      -> vector_push_back
        -> SIGSEGV
```

**Não** tratar como "bug no vector_push_back". Hipóteses **NÃO PROVADAS** (lista
inicial para a próxima etapa, sem investigar agora): container/global não
inicializado; estado incorreto do subsystem de actor; timing inadequado causado
pelo spawn diagnóstico; ponteiro/estrutura inválida; defeito real no
bridge/runtime.

## Status da etapa

- **FIX CONFIRMED:** overlay relocation / ActorInfo (`ActorInfo[0x38] == 0x80107C2C`).
- **NOT YET DIAGNOSED:** novo SIGSEGV de inicialização de actor (`vector_push_back`).
- **NOT ATTEMPTED:** marker 569; chintroticker natural/forçado; chintrochar;
  chintrocam; logo Rare.
- **P08 continua ACTIVE.** O gate geral da P08 **não** foi marcado done.

## Próxima única etapa

Investigar o novo SIGSEGV em `vector_push_back` (de `func_80106668`, chamado
por `func_80107C2C`) no caminho do construtor genérico de actor. NÃO avançar
para marker 569, chintroticker nem logo Rare.

## Handoff

```
ROOT CAUSE:                     A
RELOCATION PRESENT IN ELF:      YES   (80803234 R_MIPS_HI16 / 8080323C R_MIPS_LO16 -> 0x80804fa0)
RELOCATION PRESENT IN GENERATED CODE: YES  (RELOC_HI16/LO16(1840, 0x4FA0) apos regenerar)
EXPECTED ACTORINFO:             0x80804FA0 link-time
RUNTIME ACTORINFO:              0x8019D410
ACTORINFO[0x38]:                0x80107C2C
LOOKUP_FUNC(0):                 FIXED
```
