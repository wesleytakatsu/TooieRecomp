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

## 2. Próximo Ponto a Ajustar

Durante a execução da rotina de alocação de actor `func_80107C2C` (`TooieRecomp/RecompiledFuncs/funcs_292.c`), ocorre uma chamada `LOOKUP_FUNC(ctx->r2)` com `ctx->r2 == 0x00000000` (callback nulo opcional do actor ou proteção de ponteiro de função).

Tratando essa proteção de ponteiro nulo em `LOOKUP_FUNC` ou nos stubs em `TooieRecomp/src/game/boot_hw_stubs.cpp`, `chintroticker` completará a inicialização e instanciará `chintrochar` (440), permitindo a exibição visual da logo da Rareware.
