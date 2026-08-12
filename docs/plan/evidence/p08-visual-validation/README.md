---
title: "P08 — validação visual da intro e handoff para attract demo"
description: "Inventário das capturas e interpretação humana validada da abertura até a tela preta pós-Jiggy."
status: evidence
phase: P08
updated: 2026-08-11
tags: [tooie-recomp, p08, visual, intro, jiggy, attract]
---

# P08 — visual validation: intro → attract handoff

BASE HEAD: `cf3116696e290069372a85764929c69284135d0e`
Env: `TOOIE_AUTO_START=1`, `TOOIE_DIAG=1`,
`TOOIE_DISABLE_LAUNCHER_MUSIC=1`.

## Interpretação humana validada

| captura aproximada | conteúdo |
| --- | --- |
| `0005.png` | black boot/início |
| `0010.png` | opening renderizando, céu/brilho |
| `0020.png` | cena 3D + Dolby Surround |
| `0030.png` | logo BANJO-TOOIE |
| `0045.png` | logo/cenário continuam; câmera e intro progridem |
| `0060.png` | tela preta depois do handoff da intro/Jiggy |
| `0090.png` | preto persistente |
| `0120.png` | preto persistente |
| `0150.png` | preto persistente |
| `0180.png` | preto persistente |

A sequência natural da intro continua depois dos frames de logo e a Jiggy
transition fecha a cena corretamente. Depois da Jiggy, a primeira gameplay
demo/attract deveria começar, mas não aparece.

Descrição correta:

> The opening/intro sequence completes visually through the Jiggy transition,
> but the subsequent gameplay demo/attract scene does not start.

Não usar “opening stalls after logo”.

## Limite temporal da evidência

`0060.png` isoladamente não prova que a Jiggy terminou exatamente naquele
frame. A conclusão de completion vem da validação humana da sequência completa,
correlacionada com o fluxo observado. Os tempos são aproximados.

## Métricas registradas na sessão de captura

| arquivo | cores | mean | hash 16x16 gray |
| --- | ---: | ---: | --- |
| `0005.png` | 1 | 0.0 | uniforme |
| `0010.png` | 31.061 | 142.6 | `c31ab668...` |
| `0020.png` | 54.244 | 98.9 | `c45ece95...` |
| `0030.png` | 212.595 | 69.6 | `6a3c5aef...` |
| `0045.png` | 262.956 | 64.2 | `7716f27b...` |
| `0060.png` | 1 | 0.0 | uniforme |
| `0090.png` | 1 | 0.0 | uniforme |
| `0120.png` | 1 | 0.0 | uniforme |
| `0150.png` | 1 | 0.0 | uniforme |
| `0180.png` | 1 | 0.0 | uniforme |

- `0010→0020`, `0020→0030`, `0030→0045` e `0045→0060` tiveram mudanças
  grandes de pixels.
- `0060→0090→0120→0150→0180` eram idênticos e totalmente pretos.

## Correlação técnica atual

- Request da first attract demo: map 335, entrance 8, warp 3.
- Transition count corrigido: `1`, não `0`; type-4 ativa.
- Clean post-mortem: MAIN executando game tick, sem bloqueio em frame-sync ou
  scheduler reply queue.
- Type-4 incompleta: progress/elapsed `≈0.9`, duration/target `≈3.5`.
- `last_ovl=635:2` não é o blocking PC da MAIN.

Ver [[../p08-intro-to-demo-transition-diagnosis]] para o diagnóstico completo.

## Arquivos revisados neste checkpoint

As dez PNGs originais estão presentes, foram revisadas contra a interpretação
acima e totalizam aproximadamente 3.32 MB. Elas são ignoradas pela regra
genérica `docs/**/*.png`, portanto entram explicitamente neste checkpoint como
evidência P08 revisada; nenhuma captura externa ou fora do escopo foi incluída.
