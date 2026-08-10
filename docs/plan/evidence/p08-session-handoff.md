---
title: "P08 — handoff da sessão de diagnóstico e spawn dos actors da intro"
description: "Estado atual do spawn diagnóstico de chcollballcontrol (marker 419), chintrochar (440) e renderização da logo da Rare na abertura. REVISADO: marker 419 = chcollballcontrol; marker 569 = chintroticker; hook agora opt-in."
status: evidence
phase: P08
updated: 2026-08-10
tags: [banjo-tooie-recomp, p08, intro, chintroticker, chintrochar, gemarkers, rare-logo]
---

# P08 — handoff da sessão (Intro Actors & Rare Logo)

## 1. Estado Atual

- **Cadeia de boot e setup:** O motor inicializa perfeitamente o mapa `MAP_18B_CS_STARTUP_INTRO` (`0x18B`) com o asset `0xAE0` em `mode=3`.
- **Mapeamento de Actors:**
  - `chintroticker` (overlay 419) é o diretor de cena da intro.
  - `chintrochar` (overlay 440) instancia a malha 3D da logo dourada da Rareware, Nintendo logo e caracteres.
  - `chintrocam` (overlay 441) comanda a spline de câmera.
  - `gemarkersDll` (overlay 793) resolve os ponteiros de entrypoint dos actors.
- **Implementação Realizada:**
  - Em `TooieRecomp/src/game/syscall_handler.cpp`, adicionamos o hook de inicialização automática de `chintroticker` (419) no retorno de `gcsectionDll:4` para o mapa `0x18B`.
  - Corrigimos o gerenciamento de overlays em `syscall_handler.cpp` para reter o pacote do actor em `s_resident_overlays`.

## 2. Callback nulo no spawn (diagnóstico provado)

**Hipótese anterior (revisada):** acreditava-se que o abort ocorria em
`func_80107C2C` (`RecompiledFuncs/funcs_292.c`) com `LOOKUP_FUNC(ctx->r2)` e
`ctx->r2 == 0`. **Isso foi provado incorreto.**

**Achado real (gdb + instrumentação + ROM):** o abort está em
**`func_80108C90`** (`RecompiledFuncs/funcs_126.c:2388`), na segunda chamada
indireta `LOOKUP_FUNC(ctx->r25)` em `0x80108CDC`, onde `ctx->r25 =
MEM_W(0x38, retorno_do_construtor)`. O construtor (resolvido por
`gemarkersDll`) retorna o endereço link-time do ActorInfo
(`D_80804FA0_chcollballcontrol`), mas o data section do overlay 467 não está
mapeado em `0x80804FA0` no runtime (overlay carregado no heap), então o campo
`+0x38` é lido como **0**. O ROM em `D_80804FD8` contém **`func_80107C2C`**
(não-nulo) — o callback não é opcional.

Diagnóstico completo: [[p08-actor-null-callback-diagnosis]].

**Descoberta adicional:** o auto-spawn usa marker id **419**, que o
`gemarkersDll` resolve para `chcollballcontrol_entrypoint_3` — **não**
chintroticker. O marker id correto de chintroticker é **569** (0x239).

**Decisão (CASO B):** nenhum fallback global para `LOOKUP_FUNC(0)` foi
implementado; diagnóstico apenas. **Resolvido por
[[p08-overlay-data-relocation-diagnosis]]** (causa: relocation de data de
overlay descartada pelo gerador de metadados; corrigida na camada
generator/metadata). O auto-spawn agora alcança `func_80107C2C` com
`ActorInfo[0x38] == 0x80107C2C`; crash residual em `vector_push_back`
(`func_80106668`) é problema posterior. Reavaliar o auto-spawn com o marker id
correto de chintroticker (569) permanece para etapa posterior.

---

## 3. REVISÃO POSTERIOR (2026-08-10) — correção de fatos desatualizados

O texto histórico acima usava "chintroticker (419)" de forma ambígua. Fatos
corrigidos (a evidência histórica acima NÃO foi reescrita):

- **overlay index 419 = chintroticker**;
- **marker id 419 = chcollballcontrol** (overlay 467);
- **marker id 569 = chintroticker**;
- o hook em `syscall_handler.cpp` que chamava `func_80108C90(419, ...)`
  (marker **419** = chcollballcontrol) era **diagnóstico/regressão de
  relocation** e agora é **opt-in**, default **OFF**, via
  `TOOIE_DIAG_SPAWN_MARKER419=1`;
- o SIGSEGV em `vector_push_back` (`func_80106668`) foi **diagnosticado como
  artefato desse hook** (ver [[p08-actor-init-segv-diagnosis]]): o hook
  disparava o spawn antes de o actor manager (`chbaddieDll ep1 →
  func_800FF390 → func_801065E0`) inicializar `D_80136EE0`;
- a **prioridade atual** é o crash natural do fluxo (com marker 419 OFF):
  `glintrosync:6 → gcsection:4 → SIGSEGV`, diagnosticado e **corrigido** em
  [[p08-gcsection-glintrosync-segv-diagnosis]] (causa: `ovl_unload` não
  invalidava `s_resident_overlays`, deixando mapping stale cuja região de heap
  foi reutilizada por outra overlay; fix estreito em `ovl_unload`).

Referências: [[p08-actor-init-segv-diagnosis]],
[[p08-gcsection-glintrosync-segv-diagnosis]].
Novo bloqueio pós-fix (`No threads left to run!`) diagnosticado em
[[p08-no-threads-left-diagnosis]] (ROOT CAUSE E, HIGH).
Auditoria de reuso BK (nenhum behavior BK ativo): [[p08-active-banjo-reuse-audit]].
