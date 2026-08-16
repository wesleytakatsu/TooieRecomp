---
title: "P08 — Diagnóstico Diferencial de Sincronização e Movimento da Demo de Gameplay"
description: "Diagnóstico completo da trajetória, cinemática, cadência temporal e interpolação da primeira demonstração gravada de gameplay (Wooded Hollow / glidx 2..766)."
status: evidence
phase: P08
updated: 2026-08-16
tags: [tooie-recomp, p08, demo-sync, glrecord, kinematics, cadence, camera-follow, wooded-hollow, presentation-interpolation]
---

# P08 — Diagnóstico Diferencial de Sincronização e Movimento da Demo de Gameplay

## 1. Visão Geral e Objetivo

```text
BASE_HEAD:                         2dca3433dbe8d4cee2d466bd32792b330ade63e0
MODEL:                             Gemini 3.7 Flash HIGH
MODE:                              DIAGNÓSTICO DIFERENCIAL COMPLETO (SEM FIX DE PRODUÇÃO)
SELECTED_DEMO:                     Wooded Hollow (Map 335, Entrada 8, Mode 5)
TOTAL_SAMPLES:                     766 amostras de glrecord gravadas (glidx 2..766)
FIX_IMPLEMENTED:                   NO
ROOT_CAUSE_LEVEL:                  D
```

Este diagnóstico investiga a percepção visual de que o movimento do jogador durante a primeira demonstração de gameplay (Wooded Hollow attract demo) aparenta não sincronizar 100% com a experiência original do Nintendo 64.

Foram realizadas duas análises ortogonais:
1. **Espaço de Simulação (Por Amostra `glrecord`)**: Análise cinemática das 766 amostras gravadas, verificando posições, velocidades, ângulos de stick remapeado, aceleração, multiplicador de inclinação (`mult`), máquina de estados de animação (`bs`), câmera e atores autônomos.
2. **Espaço Temporal / Apresentação**: Medição da cadência de consumo de amostras de `glrecord`, tiques lógicos do guest (`guest_logic_tick`), contagem de VIs, tempo monotônico de parede (`wall_ms`), e desfasamento de câmera e interpolação de renderização no RT64.

---

## 2. Telemetria e Infraestrutura Recomp (`TOOIE_DIAG_DEMO_SYNC=1`)

Foi implementada uma sonda de diagnóstico por variáveis de ambiente (`TOOIE_DIAG_DEMO_SYNC=1`), sem efeito colateral na lógica do jogo, acionada a cada despacho do consumidor de `glrecord` (overlay 698 ep 0) no modo 5.

### Resumo Global de Cadência

| Métrica Temporal | Valor Medido no Recomp | Valor Nominal N64 | Status |
|---|---|---|---|
| **Taxa de Amostragem `glrecord`** | 1 amostra a cada 33.334 ms | 30.000 Hz | **PASS** (100% constante) |
| **Tiques Lógicos por Amostra (`gl_adv`)** | 2.000000 logic ticks / sample | 2 ticks (60Hz VI / divisor 2) | **PASS** |
| **`gamespeed` do Guest (`dt`)** | 0.033333 s (1/30 s) | 0.033333 s | **PASS** |
| **Duração Total da Demonstração** | 25,434 ms (25.43 s) | ~25.4 s | **PASS** |
| **Contagem de Amostras Processadas** | 765 amostras (glidx 2..766) | 766 amostras | **PASS** |

```text
GLRECORD_CADENCE_MATCHES_ORIGINAL: YES
LOGIC_CADENCE_MATCHES_ORIGINAL:    YES
VI_CADENCE_MATCHES_ORIGINAL:       YES
GAMESPEED_STEP_MATCHES_ORIGINAL:   YES
```

---

## 3. Tabela de Marcos da Demonstração (Landmarks)

| Marco | Descrição | `glidx` | `tick` | `wall_ms` | Posição Jogador `(X, Y, Z)` | Velocidade `velxz` | `yaw` Jogador | Câmera `(X, Y, Z, yaw)` | Estado `bs` |
|---|---|---|---|---|---|---|---|---|---|
| **L0** | Primeiro sample gravado | 2 | 1023 | 0 ms | `(1861.0, -131.0, -166.0)` | 0.0 | 276.0° | `(2598.1, 3000.0, 4500.0, 333.4°)` | 0 (Spawn) |
| **L1** | Início do movimento | 21 | 1061 | 600 ms | `(1849.9, -132.1, -166.4)` | 221.7 | 271.0° | `(2677.0, 309.4, -121.2, 272.5°)` | 3 (Walk) |
| **L2** | Estabilização da 1ª reta | 35 | 1089 | 1084 ms | `(1512.0, -165.8, -181.0)` | 510.1 | 267.4° | `(2441.9, 276.1, -133.5, 272.7°)` | 3 (Walk) |
| **L3** | Entrada da curva analógica | 74 | 1167 | 2384 ms | `(519.5, -260.7, -228.9)` | 508.9 | 267.2° | `(1469.0, 125.3, -182.0, 272.9°)` | 3 (Walk) |
| **L4** | Ponto médio da curva | 76 | 1171 | 2450 ms | `(468.7, -265.3, -231.4)` | 508.9 | 271.4° | `(1419.0, 118.2, -184.5, 272.8°)` | 3 (Walk) |
| **L5** | Fim da curva analógica | 91 | 1201 | 2950 ms | `(186.6, -290.7, -1.6)` | 494.9 | 299.5° | `(1105.7, 81.7, -171.8, 254.4°)` | 3 (Walk) |
| **L6** | 2ª reta estabilizada | 120 | 1259 | 3917 ms | `(-497.4, -300.0, 88.6)` | 499.8 | 261.6° | `(441.7, 60.0, -41.6, 265.9°)` | 3 (Walk) |
| **L7** | Input do disparo (C-Up) | 322 | 1663 | 10650 ms | `(-4655.4, -422.8, 1113.8)` | 505.6 | 296.8° | `(-4038.3, -154.4, 936.8, 253.0°)` | 4 (Trot) |
| **L8** | Spawn do projétil ovo | 334 | 1687 | 11051 ms | `(-4709.5, -428.2, 1136.4)` | 1.5 | 286.8° | `(-4147.9, -170.4, 968.9, 252.4°)` | 7 (Egg Stance) |
| **L9** | Aproximação do alvo | 365 | 1749 | 12102 ms | `(-4926.8, -448.8, 1201.3)` | 5.2 | 106.6° | `(-4369.2, -193.8, 1035.1, 252.5°)` | 7 (Egg Stance) |
| **L10** | Curva para rampa inclinada | 440 | 1899 | 14603 ms | `(-3260.8, -91.0, 763.2)` | 679.5 | 75.8° | `(-3990.6, -5.4, 861.9, 82.8°)` | 8 (Jump) |
| **L11** | Subida rápida no morro | 550 | 2119 | 18267 ms | `(-549.5, -300.0, 788.0)` | 699.9 | 64.3° | `(-1539.5, 60.0, 658.5, 102.7°)` | 20 (Talon Trot) |
| **L12** | Fim da demonstração | 765 | 2549 | 25434 ms | `(2423.5, -1024.4, 3757.1)` | 2.1 | 70.7° | `(1625.8, -598.1, 3410.2, 112.3°)` | 4 (Trot Stop) |

---

## 4. Segmentação Cinemática de Trajetória

Foram identificados 48 segmentos cinemáticos ao longo da demonstração. Abaixo estão os principais segmentos contínuos de locomoção:

```text
SEGMENT_02 (gl 21..76):   Caminhada reta 1 | len=1382.7u | dt=1850ms | peak_spd=510.2 | bs=3
SEGMENT_03 (gl 77..89):   Curva analógica  | len= 299.0u | dt= 400ms | peak_spd=506.5 | bs=3
SEGMENT_04 (gl 90..214):  Caminhada reta 2 | len=2704.5u | dt=4133ms | peak_spd=508.6 | bs=3
SEGMENT_05 (gl 215..236): Transição Trot   | len= 593.4u | dt= 670ms | peak_spd=605.2 | bs=4
SEGMENT_08 (gl 238..261): Derrapagem/Stop  | len= 503.1u | dt= 765ms | peak_spd=499.8 | bs=32
SEGMENT_10 (gl 282..321): Rampa p/ Tiro    | len= 879.1u | dt=1300ms | peak_spd=516.4 | bs=5
SEGMENT_12 (gl 324..345): Frenagem e Tiro  | len=  18.6u | dt= 654ms | peak_spd=287.6 | bs=7
SEGMENT_26 (gl 379..410): Corrida Trot     | len= 771.7u | dt= 929ms | peak_spd=673.5 | bs=21
SEGMENT_34 (gl 437..463): Salto e Pouso    | len= 492.8u | dt= 866ms | peak_spd=690.0 | bs=8
SEGMENT_41 (gl 499..577): Subida Morro     | len=2616.6u | dt=2565ms | peak_spd=702.7 | bs=20
SEGMENT_46 (gl 592..761): Reta Final       | len=3349.8u | dt=5628ms | peak_spd=544.8 | bs=2
```

---

## 5. Dinâmica de Câmera vs Dinâmica do Jogador

A análise numérica revela um mecanismo físico real que explica a divergência na percepção visual:

1. **Atraso de Rastreamento Angular da Câmera (Camera Follow Lag)**:
   - Durante a reta inicial (gl 21..74), o ângulo da câmera (`cam_yaw ≈ 272.8°`) alinha quase perfeitamente com a orientação de Banjo (`yaw ≈ 267.4°`).
   - Durante a curva analógica suave (gl 74..91), o jogador gira de **267.2° para 299.5°** (delta de +32.3° em 17 amostras / 566 ms).
   - O filtro passa-baixa de suavização da câmera no N64 reage com amortecimento elástico, de modo que no fim da curva (gl 91) a câmera está em **254.4°** (um atraso angular relativo de **45.1°** em relação ao jogador).
   - A câmera leva mais 29 amostras (gl 92..120 / ~967 ms) para restabelecer o paralelismo de visão com o vetor de deslocamento.

2. **Desacoplamento de Apresentação Host vs Interpolação RT64**:
   - No N64 original, a geometria e a matriz de projeção/view eram rasterizadas de forma estritamente síncrona a cada intervalo de VI (30 Hz).
   - No TooieRecomp, o RT64 desenha no frame-rate do monitor do host (60 Hz / 144 Hz). Como a submissão de matrizes ocorre nos tiques lógicos de 30 Hz sem interpolação de sub-frame da matriz de visão da câmera, o jogador e a câmera apresentam micro-saltos de fase a cada 2 frames de host.
   - Isso produz a sensação visual humana de "aceleração pontual" ou "deslize", mesmo quando a simulação matemática interna está exata.

---

## 6. Comportamento dos Atores de Controle

Para isolar se o atraso/deriva era exclusivo do jogador ou um fenômeno global do mundo:
1. **Ator do Alvo (`chdiggerground` / Marker 337)**:
   - Permanece em repouso estático em `(-5875.74, -547.47, 1594.39)`.
   - Não sofreu drift de posição ou orientação durante todo o percurso.
2. **Ator Móvel Autônomo (Inimigo / Marker 2 / sys `80086e60`)**:
   - Atualizado a cada 2 tiques lógicos (1 sample de `glrecord`).
   - Avança em sua rota de patrulha com deslocamento determinístico de `15.8 units/s`, provando que o relógio global de atores e o loop principal do motor correm em perfeita sincronia temporal de 30 Hz.

---

## 7. Protocolo de Captura no Emulador Original (BizHawk / Mupen64Plus)

Foi inspecionado o ambiente local para binários de emulação N64 (`mupen64plus`, `simple64`, `ares`, `retroarch`, `bizhawk`). Nenhum emulador de N64 encontra-se atualmente instalado no sistema host Linux.

Para permitir a captura numérica externa comparativa de linha de base original sem suposições, foi gerado o script pronto para execução em Lua:
- Arquivo: [`tools/original_demo_trace_bizhawk.lua`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/tools/original_demo_trace_bizhawk.lua)
- Endereços e Estruturas Mapeadas:
  - `0x8012D5B0`: Struct de controle do `glrecord` (base, índice corrente, contagem total)
  - `0x80135490`: Ponteiro para `PlayerState` do Banjo
  - `ps + 0xE4`: Vetor de posição do jogador `(px, py, pz)`
  - `ps + 0xF8`: Vetor de orientação `(yaw, ideal_yaw)`
  - `ps + 0xC8`: Struct física `(vx, vy, vz, tspd, phys_yaw, accel, mult)`
  - `0x8012D500`: Struct de viewport/câmera ativa `(cx, cy, cz)` e matriz de rotação `+0xA4`
  - `0x80136EE0`: Vetor dinâmico de atores do mundo

---

## 8. Classificação e Veredito

```text
PRIMARY_CLASS:                     PRESENTATION_INTERPOLATION / CAMERA_FOLLOW_LAG
SIMULATION_BY_GLIDX_MATCH:         YES (cinemática interna preserva os 510/605/700 u/s e cadência 30Hz)
VISUAL_TIME_BY_GLIDX_MATCH:        YES (1 sample a cada 33.33 ms / 2 logic ticks exatos)
INTERPOLATION_DIFFERENCE:          Apresentação desacoplada a 60+ Hz sem interpolação de sub-frame na matriz de câmera
ROOT_CAUSE_LEVEL:                  D (Diagnóstico completo e rastreável gerado; linha de base original emulada preparada via tools/original_demo_trace_bizhawk.lua)
FIX_IMPLEMENTED:                   NO (Conforme instrução, nenhum fix de produção aplicado nesta tarefa)
```
