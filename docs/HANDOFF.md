---
title: "Handoff — TooieRecomp"
description: "Estado atual do TooieRecomp por área: build Linux/Windows, runtime, render (widescreen e interpolação), desempenho, pendências e próxima ação."
status: current
type: handoff
project: TooieRecomp
updated: 2026-10-23
tags: [tooie-recomp, handoff, status]
---
# Handoff — TooieRecomp

Estado corrente para um agente ou chat novo. Histórico de investigações e evidências antigas: histórico do git
(as pastas `docs/plan/`, `docs/plans/`, `docs/roadmap/`, `docs/specs/BK/` e `docs/specs/BT/` foram removidas em
2026-10-23; recupere com `git log -- <caminho>` se precisar). Contratos estáveis: [[specs/index]]. Conhecimento do jogo:
[[reverse-engineering/README]].

## Estado por área

| Área | Estado | Referência |
| --- | --- | --- |
| Build standalone (vendorizado, kit-free) | Linux e Windows 11 x64 nativo funcionando; `VENDOR VERIFY OK` (29 patches) | [[specs/Tooie/build-and-generator]], [[specs/Tooie/packaging-and-dependencies]] |
| ROM / extração | ROM canônica `Banjo-Tooie (USA).z64` (privada, git-ignored) → imagem descomprimida exigida pelo launcher | [[specs/Tooie/asset-and-licensing-manifest]] |
| Runtime e overlays | dispatch de syscalls de overlay, `ovl_load`/`ovl_unload` host-aware; dedupe de `loaded_sections` no librecomp | [[specs/Tooie/runtime-and-overlays]] |
| Áudio | `n_aspMain` + fila de saída corrigida; BGM controlável | [[specs/Tooie/audio-system]] |
| Render 4:3 / widescreen | 16:9 por planos de culling nativos (produção) | [[specs/Tooie/graphics-and-presentation]], [[reverse-engineering/systems/view-volume]] |
| Interpolação (60/120 FPS) | pareamento ordenado + guardas P09 (produção) | [[specs/Tooie/graphics-and-presentation]], [[reverse-engineering/systems/transform-interpolation]] |
| Entrada / UI / config | SDL2, menu ESC (Graphics: Interpolation), FPS overlay | [[specs/Tooie/input-and-controllers]], [[specs/Tooie/window-ui-and-config]] |
| Testes | `Build-Tooie.ps1 -Stage Test -AllTests`: required 9/9, extra 17/17, 5 skipped (Windows, 2026-10-23) | [[specs/Tooie/testing-and-oracle]] |

## Correções recentes (todas em `origin/main`)

- `55ecdd0` — Widescreen: o aspecto do volume de visão (`+0xE8`) é alargado só enquanto `800CA1B0`/`800CA218`/`800CA2A8`
  constroem os vetores laterais; projeção `800CB124` fica 4:3 para o RT64 expandir. Demos do attract
  (`0x80127632 == 5`) mantêm os planos originais (planos largos dessincronizam demos gravados). `TOOIE_TEST_WS_NATIVE_PLANES=0`
  desliga. Usuário: "Parece tudo correto agora".
- `6685b48` + `f760b6d` — Interpolação: pareamento ordenado por hash, alinhamento com lacunas, corte de câmera, coerência de
  segmentos e de juntas, guarda de espinho; guarda de fechamento antiga default OFF. Attract: triângulos esticados 6640 → 0,
  juntas abertas → 0. Usuário: "Ficou muito bom".
- `ad0359b` — Desempenho: `load_overlay` não duplica mais `loaded_sections` (travas de troca de fase 110,8 s → 2,5 s em 7 min de
  attract, pior 91 s → 1,8 s); logs sempre ligados (Demo 5 / mapa 296 `ud5`, log por overlay) agora atrás de
  `TOOIE_DIAG_UD5` / `TOOIE_DIAG_OVERLAY_LOG`. FPS mediano 56 → 72.
- `e7bc8c1` — Removidos 23 wrappers `TOOIE_UD4_WRAP` de investigação encerrada.
- Spike guard elementwise — Interpolação: Mumbo e Kazooie tremiam nas demos 3/6/12 porque o spike guard desenhava o
  objeto em CURRENT ao detectar pequenos estiramentos da interpolação rígida. O fallback elementwise resolveu sem
  esticar triângulos (WORLDs em CURRENT -96%, 190.633 → 8.374). Usuário confirmou movimento correto (2026-10-23).

## Pendências conhecidas

- Validação Linux (feita pelo usuário no PC Linux; nunca WSL) das correções de widescreen, interpolação e desempenho: `NOT_RUN`.
- Paredes do intro em 16:9 (`T_INTRO`, cenas SCENE_A/SCENE_B) e ultrawide (> 16:9): resíduo separado, não tratado.
- Pular o matching de frames do RT64 com Interpolation OFF: tentado e `REFUTED` (o processo encerrou sem log na troca de cena
  do attract); causa `UNKNOWN`.
- Ainda há wrappers de diagnóstico `ud4`/`ud5` não-macro registrados (custo baixo; logs já desligados por padrão).
- Captura burst `TOOIE_TEST_INTERP_BURST_AT_WORKLOAD` encerrou o processo no Windows/D3D12 (1×, `UNKNOWN`).

## Notas operacionais

- Windows: `tools\windows\Build-Tooie.ps1 -Stage Test` **não** atualiza `build-win\TooieRecompiled.exe`; recompile `build-win`
  com ninja antes de medir ou entregar build para teste.
- Medições úteis (default OFF): `TOOIE_DIAG_P09_ROD_CLASSIFY=<arquivo>` (triângulos esticados, juntas e linhas `OBJ`),
  `TOOIE_DIAG_WS_CULL_CENSUS=<arquivo>` (culling widescreen), `TOOIE_DIAG_PRESENT_LOG=1` (taxa de frames, travas).
  Attract automático: `TOOIE_AUTO_START=1`; interpolação forçada: `TOOIE_TEST_INTERPOLATION_FPS=60`.
- Mudanças em código vendorizado (`lib/`, `decomp/`) viram `vendor/patches-local/*.patch` + linhas em
  `vendor/patches-applied/ORDER.tsv`, `vendor/UPSTREAMS.tsv` e `vendor/MANIFEST.sha256`; conferir com
  `LC_ALL=C tools/vendor_verify.sh --reverse-check` (no Windows, não rodar `tools/vendor_manifest.sh`).

`NEXT_SINGLE_ACTION`: validação Linux pelo usuário das correções de widescreen, interpolação e desempenho.
