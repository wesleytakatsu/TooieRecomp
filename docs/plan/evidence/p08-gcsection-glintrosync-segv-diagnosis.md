---
title: "P08 — SIGSEGV natural glintrosync:6 → gcsection:4 (diagnóstico)"
description: "SIGSEGV natural glintrosyncDll:6 → gcsectionDll:4 — func_808000F0 retornava NULL porque ovl_unload não invalidava s_resident_overlays (cache stale; heap reutilizado por overlay 709). FIX aplicado. ROOT CAUSE E, HIGH."
status: evidence
phase: P08
updated: 2026-08-10
tags: [banjo-tooie-recomp, p08, overlay, cache, glintrosync, gcsection, segv]
---

# P08 — SIGSEGV natural glintrosync:6 → gcsection:4 (diagnóstico)

## Objetivo

Diagnosticar a primeira divergência do fluxo NATURAL que causa:

```
glintrosyncDll_entrypoint_6  (ovl 695 ep6)
    -> recomp_syscall_handler (syscall 0x80088268 = ovl 677 ep4)
    -> gcsectionDll_entrypoint_4 (ovl 677 ep4)
    -> func_808000F0_gcsectionDll
    -> SIGSEGV
```

Crash reproduzido com o spawn diagnóstico de marker 419 DESABILITADO
(baseline natural), antes da init natural do actor manager.

## Estado inicial

- HEAD esperado: `a6053a07ca4c81e7f39c829ac3a725856b2b421c`.
- Relocation de data de overlay: RESOLVIDO em etapa anterior.
- Marker 419 (chcollballcontrol, overlay 467) vira spawn diagnóstico OPT-IN:
  default OFF via `TOOIE_DIAG_SPAWN_MARKER419=1` (semântica explícita; `0`/unset
  = OFF, qualquer valor não-numérico = ON). Aplicado nesta etapa.
- O crash em estudo NÃO é o vector-crash do actor (artefato do spawn);
  é o crash natural glintrosync→gcsection.

## Baseline natural

Smoke (marker OFF):

```
timeout 60 env TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 ./build/TooieRecompiled
```

Sequência observada (antes do fix): boot → seção 0x18B (ovl 677:0-4 várias
vezes, com retorno correto) → dezenas de overlays carregados → glintrosync
(ovl 695) ep6 → **gcsection ep4 → SIGSEGV**.

## Fault exato

### OBSERVAÇÃO DIRETA (GDB, baseline natural marker OFF)

```
Thread 44 "[Game] MAIN" received signal SIGSEGV, Segmentation fault.
0x00005555558d57d0 in gcsectionDll_entrypoint_4 ()

=== REGISTERS AT FAULT ===
rax  0x7ffec4400003    (== rdram_host + 3)
rbx  0x7ffe98fe9580    (ctx*)
rcx  0xffffffff80045670
rdx  0x80000004
rsi  0x7ffe98fe9580    (ctx*)
rdi  0x7fff44400000    (== rdram_host)
rbp  0x0
rip  0x5555558d57d0    <gcsectionDll_entrypoint_4+112>
    0x...d7cb <+107>:  mov $0x80000004,%edx
=>  0x...d7d0 <+112>:  movzbl (%rdx,%rax,1),%eax   ; lbu $v0, 0x4($v0)

MIPS ctx->r2 = 0x0    (DIRETO, via recomp_context no momento do fault)
MIPS ctx->r4 = 0x18b  (a0 = section id)

MIPS PC: 0x808001C8  (lbu $v0, 0x4($v0))
FAULT ADDRESS: rdram_host + 0x80000007 (efetivo rdx+rax; 7 bytes além do buffer 8MB)
```

Nota: `rax=0x7ffec4400003` é REGISTRADOR HOST (base rdram + 3), NÃO o retorno
MIPS. O retorno MIPS é `ctx->r2`; a observação direta dá `ctx->r2 == 0`.

### HOST vs GUEST — esclarecido

- HOST REGISTER: `rax=0x7ffec4400003` (= rdram_host + 3) — usado na soma do
  endereço efetivo; NÃO é valor de retorno.
- MIPS REGISTER: `ctx->r2 = 0` (DIRECT GDB OBSERVATION imediatamente após
  `func_808000F0` e antes do `lbu`). Confirma a dedução algébrica
  (`MEM_BU(0x4, reg)` → offset host 0x80000007 ⇒ `(reg+4)^3 = 7` ⇒ `reg=0`).
- GUEST ADDRESS: retorno de `func_808000F0` era guest `0` (NULL).

## glintrosync ep6 original

- Overlay 695 (`glintrosyncDll`), seção 2738, rom 0x0211D810, size 0x5D0.
- MIPS 0x808003E0. Salva `a0` em `sp+0x18`; chama `_gcsectionDll_entrypoint_4`
  (syscall 677 ep4, stub 0x80088268) com o MESMO `a0` repassado.
- Compara o retorno com `0xD`; se `==0xD` seta `MEM_B(0x10, D_801275D0)=1`
  senão 0; depois escolhe um float de `D_808004F8` (tabela própria) conforme
  `a0` e grava em `D_801275D0+0xC`.
- É chamado por `gsworldDll ep2` (ovl 726) em `0x80800450` com
  `a0 = MEM_W(sp+0x1C)` (o id de seção/mapa, ex.: 0x18B).

## gcsection ep4 original

- Overlay 677 (`gcsectionDll`), seção 2666, rom 0x02115200, size 0x770.
- MIPS 0x808001B4:
```
addiu sp, -0x18 ; sw ra, 0x14(sp)
jal func_808000F0        ; v0 = func_808000F0(a0, 0)
  or a1, zero, zero      ; a1 = 0
lw ra, 0x14(sp)
lbu v0, 0x4(v0)          ; return byte at entry+4  ← SIGSEGV quando v0==0
jr ra
```
- Argumento: `a0` = id de seção (0x18B). Retorna o byte `+4` da entrada da
  tabela de seções (0x0D para seções skippable — daí a comparação com 0xD).

## func_808000F0 original

- Seção 2666 (gcsectionDll), MIPS 0x808000F0. Generated C: `funcs_4.c`.
- Semântica: busca linear na tabela `D_80800216_gcsectionDll`
  (entradas de 6 bytes `{u16 section_id; u8[4] value}`), stride 6, término
  quando `section_id == 0`. Se `a0` casa, retorna **ponteiro guest para a
  entrada** (`v1` → `$v0`/ctx->r2). Se não encontra e `a1==0`, retorna **0**.
- Acesso aos dados via `RELOC_HI16/LO16(2666, 0x216)`
  (relocs R_MIPS_HI16/LO16 para target_section=2666 existem).

## Estrutura acessada

- MEMORY CLASS: DATA (tabela `.data` de gcsectionDll, dentro do package
  carregado no heap da overlay).
- EXPECTED ADDRESS (entry id 0x18B): `section_addresses[2666] + 0x240`
  (entry da tabela). Ex.: com text_start 0x80196670 → `0x801968b0`.
- ACTUAL ADDRESS no call que FALHA: `section_addresses[2666] + 0x240` =
  `0x801968b0` — o MESMO endereço dos calls que funcionam, mas o CONTEÚDO é de
  outra overlay (709) → lookup falha.

### Validação runtime vs ROM

```
ROM EXPECTED (entrada id 0x18B @ 0x80800240, rom 0x2115440):
    bytes BE: 01 8B 20 00 0D 00
    parsed (6B, {u16 id; u8[4] val}): id=0x018B, val = 20 00 0D 00
    byte em entry+4 = 0x0D   (ep4 retorna; glintrosync compara com 0xD)

RUNTIME (call WORKING, ANTES do unload):
    section_addresses[2666] = 0x80196670 ; table_base = 0x80196886
    [loop] ids lidos: 0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0x18b
    [FOUND] entry_ptr = 0xffffffff801968b0 -> ctx->r2 = 0xffffffff801968b0
    MATCH: YES

RUNTIME (call FAILING, DEPOIS do unload + reloc de 709):
    section_addresses[2666] = 0x80196670 ; table_base = 0x80196886
    [loop] ids lidos: 0x64,0x24,0xa1a,0x1d,0xa,0xffffff9c,0x64,0xc,0x51...
    [NOT FOUND] ctx->r2 <- 0
    MATCH: NO  →  TERMINATES BY 0??  →  na verdade: TABLE CONTENT WRONG
```

Diagnóstico do trace:
- TABLE ADDRESS: igual nos dois casos (0x80196886) → NÃO é TABLE ADDRESS WRONG.
- TABLE CONTENT: **WRONG no call falho** (overlay 709 sobrescreveu a região).
- COMPARE/STRIDE/TERMINATION: corretos no original (match em WORKING).
- ENTRY FOUND BUT RETURN WRONG: não aplicável — a entrada não foi encontrada.

## Overlay mapping no momento do crash

- CALLER: glintrosyncDll (ovl 695), package 0x801d27f0, text_start 0x801d27f0.
- CALLEE: gcsectionDll (ovl 677), seção 2666.
- Cadeia: gsworld:2 → glintrosync:6 → gcsection:4 (syscall aninhado).
- Eventos (GDB lifecycle):
  1. Primeira carga de 677: `ovl_load idx=677 pkg=0x2115180 +0x80 -> ram=0x80196670`
     (fresh load; `cached.package` fica preenchido pela 1ª carga).
  2. `ovl_unload guest=0xffffffff801965f0 text@0x34=0x80196670` → **o jogo
     descarrega 677** (heap liberado). O host `ovl_unload` chama
     `unload_overlays(text_start, size)` (zera section_addresses[2666] p/
     0x80800000 link-time) mas **NÃO invalida `s_resident_overlays[677]`**.
  3. `ovl_load idx=709 pkg=0x2124040 +0x90 -> ram=0x801967a0` → overlay 709
     alocada em 0x80196710 (package), size 0x400 → **sobrescreve a região
     antiga de 677** (que ia de ~0x801965f0 a ~0x80196d60).
  4. glintrosync:6 → 677:4: dispatch → não está ativa na stack;
     `cached.package != 0` (STALE) → **fast-path cached** → usa
     `cached.mapping.text_start = 0x80196670` → `activate_canonical_mapping`
     → `load_overlays` re-registra `section_addresses[2666] = 0x80196670`
     → gcsection lê a tabela na região que AGORA pertence à overlay 709.
- CANONICAL MAPPING DURING: cached (stale) mapping de 677.
- SECTION ADDRESS: `section_addresses[2666] = 0x80196670` (região reutilizada).

## Expected vs Actual

- EXPECTED (original): após o jogo descarregar 677, o próximo syscall a 677:4
  re-carrega a overlay (load limpo) e a tabela de seções em `section_addresses[2666]`
  é a tabela CORRETA (0x18B encontrado) → ep4 retorna 0x0D.
- ACTUAL (TooieRecomp): o host mantém `s_resident_overlays[677]` apontando para
  o bloco de heap LIBERADO; a região é reutilizada pela overlay 709; o dispatch
  reusa o cached mapping stale e re-registra `section_addresses[2666]` para a
  região da 709 → tabela corrompida → lookup falha → retorna NULL → `lbu 0x4($v0)`
  com $v0=0 → SIGSEGV host.

## Diagnóstico

- FIRST DIVERGENCE: **`ovl_unload` (host) não invalida `s_resident_overlays[677]`
  quando o jogo descarrega a overlay 677**; o cache stale faz o dispatch de 677:4
  reusar um mapping cujo heap já foi realocado para a overlay 709.
- ROOT CAUSE: **E — NESTED OVERLAY MAPPING / RESTORE** (cache de overlay resident
  stale; a DATA/section mapping de gcsection aponta para memória de outra overlay
  porque o host não invalidou o cache no unload).
- CONFIDENCE: **HIGH** (cadeia inteira observada via GDB: cache populado na 1ª
  carga, ovl_unload de 677 sem invalidação, realocação de 709 sobre o bloco,
  dispatch cached reusando endereço stale, tabela corrompida, ctx->r2=0 direto).

### Contraste com hipóteses descartadas

- A (caller args): descartado — a0=0x18B é VALID e presente na ROM; working calls
  com o mesmo a0 retornam válido.
- B (gcsection logic): descartado — código executa igual em working/failing; a
  diferença é o CONTEÚDO da tabela.
- C (overlay data relocation): descartado como causa — relocs de 2666 presentes e
  section_addresses[2666] numericamente correto; o problema é que o conteúdo do
  bloco de heap foi sobrescrito.
- D (BSS lifetime): não é BSS; é DATA dentro do package, liberado/reutilizado.
- F (host/guest pointer): o SIGSEGV é a manifestação host do NULL; não é a
  primeira divergência (func_808000F0 retornou 0 por causa de E).
- G (earlier corruption): a "corrupção" é consequência de E.

## Alteração

- `src/game/syscall_handler.cpp`:
  - Marker-419 spawn → opt-in `TOOIE_DIAG_SPAWN_MARKER419=1`, default OFF
    (comentários atualizados: overlay 419 = chintroticker; marker 419 =
    chcollballcontrol; marker 569 = chintroticker).
  - Nova função exportada `tooie_invalidate_resident_overlay(gpr overlay_ptr)`:
    zera qualquer `s_resident_overlays[i]` cujo `package` aponte para o bloco
    (agora liberado).
- `src/game/boot_hw_stubs.cpp`:
  - `ovl_unload`: após `heap_free`, chama `tooie_invalidate_resident_overlay`
    para descartar o cache do overlay descarregado (não-residente).
- NÃO editado: RecompiledFuncs (generated C) intacto.

## Build

- `ninja -C build TooieRecompiled` → exit 0 (PASS).

## Smoke

- Baseline natural (marker OFF):
  `timeout 60 env TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 ./build/TooieRecompiled`
  - ANTES do fix: SIGSEGV em `gcsectionDll_entrypoint_4` (glintrosync:6→gcsection:4).
  - DEPOIS do fix: **crash sumiu**; glintrosync:6→gcsection:4 retorna corretamente;
    jogo progride muito mais (~33k syscalls, mode=3) até novo bloqueio:
    `terminate called after throwing std::runtime_error: "No threads left to run!"`
    → CRASH AT NEW SITE (diferente do SIGSEGV desta etapa).
- Probe GDB pós-fix: `[FOUND] entry_ptr=0xffffffff80193d00 -> ctx->r2`; ep4 AFTER
  `ctx->r2=0xffffffff80193d00` (válido); `section_addresses[2666]=0x80193ac0`
  (novo reload limpo).

## Regressões

- `LOOKUP_FUNC(0)`: 0 ocorrências em ambos os runs (natural e regression).
- Regression case `TOOIE_DIAG_SPAWN_MARKER419=1`: marker 419 dispara e o run
  termina em SIGSEGV em `vector_push_back` (via func_80108C90 → func_80107C2C →
  func_80106668) — **artefato esperado** (D_80136EE0 NULL enquanto o actor
  manager não está inicializado). NÃO é regressão da correção de relocation.
- `ActorInfo[0x38]`: não re-regressa (sem `Failed to find function at 0x00000000`).

## Próxima única etapa

Diagnosticar o novo bloqueio pós-fix: `std::runtime_error: No threads left to
run!` (esgotamento de threads do runtime/threads do jogo bloqueadas) — segundo
SIGSEGV/divergência de thread no fluxo natural, após o ponto glintrosync:6 →
gcsection:4 agora passar. Não avançar para actor manager init nem marker 569.
