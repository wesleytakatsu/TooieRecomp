---
title: "P08 — handoff da sessão de diagnóstico e spawn dos actors da intro"
description: "Estado atual do spawn de chintroticker (419), chintrochar (440) e renderização da logo da Rare na abertura."
status: evidence
phase: P08
updated: 2026-08-09
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
implementado; diagnóstico apenas. Próximo passo: investigar o mapeamento de
data de overlays no `ovl_load` host (base link-time `0x80800000`) e reavaliar
o auto-spawn com o marker id correto (569).
