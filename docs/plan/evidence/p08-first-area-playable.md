---
title: "P08 — tomada 3D travada após a transição"
description: "Smoke dirigido: boot → jiggy → terreno/água da abertura; câmera e sequência não progridem."
status: evidence
phase: P08
updated: 2026-08-09
tags: [tooie, p08, rt64, opening, camera, pre-m3]
---

# P08 — cena 3D após a transição

> Correção da revisão: as capturas comprovam terreno/água renderizados após o
> wipe, mas não mostram personagem, HUD, logos ou demos. O usuário identificou
> que a sequência original deveria continuar automaticamente; a câmera atual
> permanece quase ao nível do chão. O nome deste arquivo e de uma captura antiga
> é mantido apenas para não quebrar links.

## Comando

```sh
cd TooieRecomp/build
TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DIAG_INTERVAL_MS=3000 \
  TOOIE_DISABLE_LAUNCHER_MUSIC=1 ./TooieRecompiled
```

Janela: `Banjo-Tooie: Recompiled` (1600×900). Capturas via `import -window`.

## Resultado visual

| t | ficheiro | conteúdo |
| --- | --- | --- |
| ~10 s | [[p08-jiggy-transition.png]] | wipe da transição jiggy (âmbar) |
| ~20–50 s | [[p08-first-area-20s.png]], [[p08-first-area-50s.png]] | mundo 3D abrindo sob o wipe |
| ~70–90 s | [[p08-first-area-70s.png]], [[p08-first-area-90s.png]], [[p08-spiral-mountain.png]] | terreno, água e grama da mesma tomada; “spiral-mountain” é apenas nome antigo do arquivo, não identificação confirmada |

## Runtime (diag)

- `TOOIE_AUTO_START: starting game` + hooks LOOKUP registados.
- Scheduler ack (event 3) ×4; depois `last_ovl=677:3` (`gcsectionDll`) com stack `(idle)`.
- Syscalls crescem ~19k → ~98k em ~90 s; `inflight=0`.
- Áudio: `pulseaudio/(default)`, `rsp_aud` e `queue_*` sobem; `peak`/`nonzero` > 0.
- TIDY: coalescing de request/reply (esperado no host).
- Overlays observados no path de boot: `gldbDll` (708), `gsworldDll` (726), `gsmaker` (2); estado estável em `gcsectionDll` (677).

## Áudio

- Utilizador confirmou (2026-08-09): **áudio a funcionar** na sessão de escuta com
  `TOOIE_AUTO_START=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1`.
- Nota: “tic tic” reportado numa sessão anterior; com preview do launcher desligado
  e um único processo, o utilizador validou OK. Sons do cenário também podem ser
  conteúdo do jogo, não necessariamente glitch do host.

## Gate visual

| critério | estado |
| --- | --- |
| RT64 frames pós-launcher hide | done |
| Transição jiggy completa | done (captura + utilizador) |
| Cena 3D após a transição | done |
| Logos, câmera descendente e demos | falha atual: tomada de terreno/água não progride |
| Personagem, HUD e controle da área | não comprovado; não alcançado pelo fluxo automático |
| Áudio estável no sink default | done (utilizador) |
| Logo Rare / menu título isolados | residual após corrigir a máquina de estados da abertura |
| Input físico teclado/gamepad/rumble/hotplug | residual |
| 4:3 / fullscreen / fallback Vulkan | residual P08 polish |

## Conclusão

O runtime ultrapassa a transição e apresenta geometria 3D, mas **M3 ainda não
está fechado** porque a abertura original não executa logos/câmera/demos. M4
(gameplay/área controlável) também permanece aberto. Estado técnico atualizado:
[[p08-session-handoff]].
