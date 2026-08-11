---
title: "P08 — corrigir binding de R_MIPS_26 intra-overlay no GENERATOR"
description: "Fix estrutural no N64Recomp: jal same-section em seções relocatable agora resolve para chamada direta a C, mesmo com use_lookup_for_all_function_calls=true. Elimina a colisão de base vram 0x80800000 (chintrochar→gsworldDll). Scheduler preservado via restrição a relocatable. Validado 3/3 smokes + 140s run."
status: done
phase: P08
updated: 2026-08-11
tags: [banjo-tooie-recomp, p08, generator, jal, overlay, func_map, rmips26, same-section]
---

# P08 — corrigir binding de R_MIPS_26 intra-overlay no GENERATOR

**HEAD TooieRecomp:** `f865c038455da1e5fafb5257ece974e1e15c9399`
**N64ModernRuntime:** `ca568b6ad79b9029d14077f0c3ffa757727c5559`
**N64Recomp submodule:** `2b6f05688de2abc7d86da5b4a89b84c2c6acbabe`

## 1. Causa raiz (DeepSeek dinâmico + Gemini estático)

`func_80800484_chintrochar` tem um `jal 0x80800000` (R_MIPS_26 @ 0x808004E8)
intra-overlay para `func_80800000_chintrochar` (mesma seção `.chintrochar`).

O generator com a política Tooie P04 (`use_lookup_for_all_function_calls =
true`) emitia `LOOKUP_FUNC(0x80800000)`. No runtime, o `func_map` plano
vram→func tem colisão na base canônica `0x80800000` compartilhada por centenas
de DLL overlays (`.gsworldDll`, `.chintrochar`, `.gsmaker`, `.fxdialog`, ...).

Quando o `.gsworldDll` foi a última ativação, `func_map[0x80800000]` apontava
para `gsworldDll_entrypoint_0`, e a chamada intra-overlay do chintrochar
resolveu para a função errada.

**Cadeia:** actor ptr `0x801DF300` → `gsworldDll ep0` espúrio → gsmaker ep2
(`+0x955`) → gsmaker ep1 (`int16` truncado) → idx `-0x3AB` → asset table OOB
→ devAddr `0xF13B24` / size `0xFF3FF03E` → `osPiStartDma` → `do_dma` SIGSEGV.

Detalhes completos em `p08-do-dma-segv-diagnosis.md`.

## 2. Escala estrutural (Gemini)

- ~12.224 intra-overlay calls usando LOOKUP (est. Gemini).
- 605 overlays afetados; 888 target VRAMs com colisão; 227 overlays usando base `0x80800000`.
- A correção é **estrutural**, não special-case de chintrochar.

## 3. Contrato escolhido (same-section only)

Em `resolve_jal()` (N64Recomp `src/recompilation.cpp`), antes do
early-return `use_lookup_for_all_function_calls → Ambiguous`:

1. Se existe target inequívoco e `target_func.section_index == cur_section_index`
   **E** a seção corrente é **relocatable** → `Match` (chamada direta a C).
2. Caso contrário, se `use_lookup_for_all_function_calls` → `Ambiguous` (fallback preservado).
3. Restante semântica existente preservada.

**Restrição a relocatable** (adicional ao contrato mínimo): seções
não-relocatable (`.boot`, `.core1`, ...) podem ter **host runtime hooks**
instalados sobre funções geradas em runtime (ex.: `__osGetSR_2` @ `0x8001DCA0`,
que devolve `0x1000` para manter o scheduler fora do DP-flush loop). Uma
chamada direta a C nesse caso desviaria do hook que o `LOOKUP_FUNC` alcançaria.

Sem essa restrição, o scheduler travou: `func_80014A54` passou a chamar
`__osGetSR_2` direto (gerado, retorna 0) em vez do hook (retorna `0x1000`),
entrando no loop fatal de DP-flush de `func_800149BC`.

## 4. A/B da regressão (scheduler)

| build | resultado |
|---|---|
| PRE-fix (só main.cpp policy) | boot passou pelo scheduler, opening ovl 677, crash `do_dma` (bug antigo) |
| POST-fix sem restrição | **stall** no scheduler: SCHEDULE thread girando em `get_function`/`osDpSetStatus` (DP-flush loop), MAIN bloqueado em `osSendMesg` (handshake event 3) |
| POST-fix final (relocatable-only) | boot OK, scheduler ack, opening, sem crash |

## 5. Generated ANTES / DEPOIS

`func_80800484_chintrochar` callsite @ 0x808004E8:

```
ANTES:  LOOKUP_FUNC(0x80800000)(rdram, ctx);
DEPOIS: func_80800000_chintrochar(rdram, ctx);
```

## 6. Código alterado (N64Recomp submodule, patch versionado)

`patches/runtime/n64recomp-same-section-jal.patch` contém:

1. `src/main.cpp` (política P04 existente, agora versionada):
   `context.use_lookup_for_all_function_calls = true;`
2. `src/recompilation.cpp` — refinement do `resolve_jal`:
   pre-scan same-section relocatable → `Match` direto; depois
   `use_lookup_for_all_function_calls → Ambiguous`; depois lógica original.

SHA256 do patch: `0afeb0e57bfdd18be7a125bebe4f7bd48273622c5f5f626ae3279fd1ded0efa8`

## 7. Apply script

`tools/apply_runtime_patches.sh` agora aplica dois patches idempotentes via
helper `apply_one_patch` (valida base commit, SHA256, dry-run, reverse-check):

1. `n64modernruntime-scheduler.patch` (runtime root, base `ca568b6...`)
2. `n64recomp-same-section-jal.patch` (N64Recomp submodule, base `2b6f056...`)

## 8. Validação

### Builds
- Generator: `cmake --build N64ModernRuntime/N64Recomp/build --target N64Recomp` → PASS.
- Regeneração: `tools/run_recomp.sh` → 420 arquivos (~89 MB), exit 0.
- Tooie: `cmake --build build --target TooieRecompiled` → PASS.

### Audit estático (gerado)
- `INTRA_OVERLAY_LOOKUP_BEFORE (est.)`: 22743 (todas as same-section sob lookup-for-all).
- `RELOC_SAME_SECTION_DIRECT`: **12153** (≈ Gemini 12224).
- `RELOC_SAME_SECTION_LOOKUP_RESIDUAL`: **0**.
- Não-relocatable same-section (`.core1`/`.boot`) permanecem `LOOKUP_FUNC` (preservado de propósito).
- `jalr` continua dinâmico: `LOOKUP_FUNC(ctx->r2)(rdram, ctx);` ✓
- Externals/ambiguous continuam `LOOKUP_FUNC(vram)` ✓
- Amostra overlays (chintrochar/gsmaker/anseq/bsbanpackwhack/bsbansnooze/bsbeefly): direto ✓
- bsbbarge/bsbbuster: sem same-section calls (legítimo) ✓

### Smokes 3/3 (TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1, marker419 OFF)

| counter | run1 | run2 | run3 |
|---|---|---|---|
| IPL3 flag (`ipl3bad`) | 0 | 0 | 0 |
| BDCDC | 1 | 1 | 1 |
| cube5 | 1 | 1 | 1 |
| collect8 | 1012/1274/1265 | | |
| C90 | 11 | 11 | 11 |
| C2C | 10 | 10 | 10 |
| ALLOC | 10 | 10 | 10 |
| chint484 | 0/5/0* | | |
| chintbase | 0/0/0* | | |
| gsworld ep0 (legítimo, de gsworld ep2) | 1 | 1 | 1 |
| invalid idx -0x3AB | 0 | 0 | 0 |
| bad devaddr 0xF13B24 | 0 | 0 | 0 |
| bad size 0xFF3FF03E | 0 | 0 | 0 |
| do_dma SIGSEGV | 0 | 0 | 0 |

\* chintrochar cai fora da janela de 6 reports em alguns runs; verificado
separadamente com gdb: `func_80800000_chintrochar` é atingida a partir de
`func_80800484_chintrochar` ← `func_80800540_chintrochar` (backtrace confirmado).

### Spurious gsworld
`gsworldDll_entrypoint_0` é atingida 2× sempre a partir de
`gsworldDll_entrypoint_2` (dispatch interno legítimo do próprio overlay),
nunca de `func_80800484_chintrochar`. Requisito atendido.

### Run longo
- 70 reports (~140s), 179k syscalls, `last_ovl=635:2`, sem SIGSEGV/ABORT/HANG.
- Regressões históricas: no-threads-left=0, scheduler self-loop=0,
  `LOOKUP_FUNC(0)`=0, gcsection/glintrosync segv=0, IPL3 flag=0.

## 9. Novo primeiro blocker

**Nenhum capturado** — o jogo segue rodando a abertura/intro (ovl 635/695,
gsmaker ovl=2) sem falha nas janelas testadas.

## 10. Não feito / limites

- NÃO ampliado para cross-section non-relocatable calls (fora do bug mínimo).
- NÃO mexido em runtime `func_map` nesta etapa.
- NÃO mexido em DMA / asset table / IPL3.
- `RecompiledFuncs/` é gitignored — não commitado.
- Checkout externo BanjoRecomp continua com dirty files (patches versionados agora refletem).

## Logs

- `/tmp/opencode/p08_final_smoke{1,2,3}.log` — 3 smokes com contadores.
- `/tmp/opencode/p08_long.log`, `/tmp/opencode/p08_long2.log` — runs longos.
- `/tmp/opencode/gsworld.log`, `gsworld2.log` — spurious gsworld backtrace.
- `/tmp/opencode/chint.log` — chintrochar target backtrace.
