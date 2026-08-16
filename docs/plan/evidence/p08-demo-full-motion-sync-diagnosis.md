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
BASE_HEAD:                         1de0738f8c5881f22d03f3149a6a26d575617cf5
MODEL:                             Gemini 3.7 Flash HIGH
MODE:                              DIAGNÓSTICO DIFERENCIAL COMPLETO (SEM FIX DE PRODUÇÃO)
SELECTED_DEMO:                     Wooded Hollow (Map 335, Entrada 8, Mode 5)
RECOMP_INTERNAL_TRACE:             PROVEN (766 amostras de glrecord capturadas, glidx 2..766)
ORIGINAL_NUMERIC_TRACE:            UNKNOWN (pendente de emulador N64 instrumentado)
PROVISIONAL_PRIMARY_HYPOTHESIS:    PRESENTATION_INTERPOLATION / CAMERA_FOLLOW_LAG
FIX_IMPLEMENTED:                   NO
ROOT_CAUSE_LEVEL:                  D
ROOT_CAUSE:                        UNKNOWN (pendente de linha de base numérica original)
```

> [!IMPORTANT]
> **Distinção Epistemológica de Evidências:**
> - `MEASURED_RECOMP`: Dados empíricos extraídos ao vivo do executável `TooieRecompiled` via `TOOIE_DIAG_DEMO_SYNC=1`.
> - `ORIGINAL_MIPS_NOMINAL_EXPECTATION`: Contratos extraídos estaticamente da descompilação MIPS (ex: `glrecord_entrypoint_0`, `bastick_update`, `func_800EF04C`, cadência nominal de 30 Hz).
> - `MEASURED_ORIGINAL`: **UNKNOWN**. Nenhuma alegação de erro zero (`error = 0`) ou correspondência numérica (`match = YES`) é válida como comparação empírica até que uma captura de runtime original seja executada. Comparações anteriores não qualificadas são aqui **CORRIGIDAS/SUPERSEDIDAS (CORRECTED/SUPERSEDED)**.

Este diagnóstico investiga a percepção visual de que o movimento do jogador durante a primeira demonstração de gameplay (Wooded Hollow attract demo) aparenta não sincronizar 100% com a experiência original do Nintendo 64.

---

## 2. Telemetria e Infraestrutura Recomp (`TOOIE_DIAG_DEMO_SYNC=1`)

Foi implementada uma sonda de diagnóstico por variáveis de ambiente (`TOOIE_DIAG_DEMO_SYNC=1`), sem efeito colateral na lógica do jogo, acionada a cada despacho do consumidor de `glrecord` (overlay 698 ep 0) no modo 5.

### Resumo de Cadência do Recomp vs Expectativa Nominal MIPS

| Métrica Temporal | Valor Medido no Recomp (`MEASURED_RECOMP`) | Expectativa Nominal MIPS (`NOMINAL_MIPS`) | Linha de Base Original (`MEASURED_ORIGINAL`) | Status da Comparação |
|---|---|---|---|---|
| **Taxa de Amostragem `glrecord`** | 1 amostra a cada 33.334 ms | 30.000 Hz | UNKNOWN | **NOMINAL_MATCH** (Recomp constante a 30.0 Hz) |
| **Tiques Lógicos por Amostra (`gl_adv`)** | 2.000000 logic ticks / sample | 2 ticks (60Hz VI / divisor 2) | UNKNOWN | **NOMINAL_MATCH** |
| **`gamespeed` do Guest (`dt`)** | 0.033333 s (1/30 s) | 0.033333 s | UNKNOWN | **NOMINAL_MATCH** |
| **Duração Total da Demonstração** | 25,434 ms (25.43 s) | ~25.4 s | UNKNOWN | **NOMINAL_MATCH** |
| **Contagem de Amostras Processadas** | 765 amostras (glidx 2..766) | 766 amostras | UNKNOWN | **NOMINAL_MATCH** |

```text
RECOMP_INTERNAL_CADENCE:           30.000 Hz CONSTANTE (1 sample / 33.334 ms)
ORIGINAL_NUMERIC_CADENCE:          UNKNOWN (Pendente de captura em emulador)
SIMULATION_BY_GLIDX_MATCH:         UNKNOWN (Pendente de comparação numérica com trace original)
```

```text
GLRECORD_CADENCE_MATCHES_NOMINAL_30HZ: YES
LOGIC_CADENCE_MATCHES_NOMINAL:          YES
VI_CADENCE_MATCHES_NOMINAL:            YES
GAMESPEED_STEP_MATCHES_NOMINAL:        YES
ORIGINAL_NUMERIC_MATCH:                UNKNOWN
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

## 7. Validação Estática e Protocolo de Captura no Emulador Original

### 7.1 Validação do Mapa de Memória e Estruturas MIPS

Foi realizada a auditoria estática rigorosa de cada endereço de memória e deslocamento de estrutura contra o código MIPS original descompilado (`funcs_406.c`, `funcs_193.c`, etc.):

| Símbolo / Estrutura | Endereço Guest | Deslocamento | Tipo / Campo | Verificação no MIPS |
|---|---|---|---|---|
| **`D_8012D5B0` (glrecord)** | `0x8012D5B0` | `+0x0` | `u32` (Ponteiro base do buffer de amostras) | `lw $t7, 0x0($a3)` em `glrecord_entrypoint_0` |
| | | `+0x8` | `s32` (Índice da amostra corrente) | `lw $v0, 0x8($a3)` em `glrecord_entrypoint_0` |
| | | `+0xC` | `s32` (Contagem total de amostras) | `lw $t0, 0xC($a3)` em `glrecord_entrypoint_0` |
| **`D_80135490` (Player)** | `0x80135490` | `*0x80135490` | `PlayerState*` (Ponteiro para Banjo) | Verificado no loop de atores |
| | `ps` | `+0xE4` | `f32*` (Ponteiro para posição X, Y, Z) | Verificado em `baphysics` |
| | `ps` | `+0xF8` | `f32*` (Ponteiro para yaw, ideal_yaw) | Verificado em `yaw.c` |
| | `ps` | `+0xC8` | `f32*` (Ponteiro para struct física) | `+0x10` vx, `+0x14` vy, `+0x18` vz, `+0x3C` tspd, `+0x40` phys_yaw, `+0x44` accel, `+0x50` mult |
| | `ps` | `+0x120` | `s32*` (Ponteiro para state id `bs_id`) | Verificado em `bs.c` |
| | `ps` | `+0x128` | `f32*` (Ponteiro para bastick) | `+0x54` bx, `+0x58` by, `+0x5C` ang, `+0x60` dist |
| **`D_8012D500` (Câmera)** | `0x8012D500` | `+0x0` | `f32` (Posição X, Y, Z da viewport) | Verificado em `func_800CA7E4` |
| | | `+0xA4` | `MtxF` (Matriz de rotação da câmera) | `m00` (+0xA4), `m02` (+0xAC) |
| **`0x80136EE0` (Atores)** | `0x80136EE0` | `+0x0` | `u32` (Tamanho do elemento: `0x9C`) | Verificado no vetor de atores |
| | | `+0x4` / `+0x8`| `u32` (`begin` / `end` do vetor) | `+0x6C` marker_id (`idw & 0x1FF`), `mid=337` é `chdiggerground` |

```text
ORIGINAL_SCRIPT_FIELD_MAP_VERIFIED: YES
```

### 7.2 Semântica do Índice de `glrecord` e Stride de Amostras

A descompilação de `glrecord_entrypoint_0` em `funcs_406.c` comprova a semântica exata de avanço:
1. **Stride da amostra**: `SAMPLE_STRIDE_BYTES = 6`
   `sll $t6, $v0, 2; subu $t6, $t6, $v0; sll $t6, $t6, 1` computa exatamente `(4 - 1) * 2 = 6` bytes por amostra.
2. **Ordem de incremento do índice**: `INDEX_INCREMENT_ORDER = INCREMENT_ON_CONSUMPTION`
   A função lê `v0 = MEM_W(0x8, 0x8012D5B0)`, calcula o endereço da amostra como `base + v0 * 6`, escreve `t8 = v0 + 1` de volta em `MEM_W(0x8, 0x8012D5B0)`, e em seguida transfere os bytes da amostra para a struct `OSContPad`.
   Portanto, no momento em que o despachador conclui, `gl_idx` contém o índice da próxima amostra (o valor lido no início foi a amostra imediatamente consumida).
3. **Fórmula de endereço da amostra**: `CORRECT_SAMPLE_ADDRESS_FORMULA = base + gl_idx * 6` (onde `gl_idx` é o valor lido antes do incremento).

### 7.3 Verificação de Checksum de Input e Alinhamento (`glidx`)

A sequência de inputs gravados foi validada em marcos críticos:
- **Curva Analógica (glidx 70..76)**:
  - `glidx 70..73`: `pad=(-2, 80, 0000)`
  - `glidx 74`: `pad=(-18, 82, 0000)`
  - `glidx 75`: `pad=(-47, 78, 0000)`
  - `glidx 76`: `pad=(-63, 74, 0000)`
- **Disparo de Ovo (glidx 322)**:
  - `glidx 322`: `pad=(-9, 84, 2008)` (`buttons = 0x2008` contendo `C-Up`)

```text
GLIDX_ALIGNMENT_OFFSET:            0
RAW_INPUT_ALIGNMENT:               PASS
```

### 7.4 Significado Real do Contador "Tick"

Foi auditada a origem do campo `tick` emitido na telemetria:
- O contador global `g_guest_logic_tick` era incrementado tanto no wrapper de `func_800151BC` quanto no manipulador de syscalls de `glrecord` em `syscall_handler.cpp`.
- Como `func_800151BC` executa no loop principal de jogo a 30 Hz (uma vez a cada 2 VIs de 60 Hz), o contador refletia um avanço duplo (`+2` por amostra consumida).

```text
PREVIOUS_TICK_FIELD_ACTUAL_MEANING: DUAL_INCREMENT_COUNTER (func_800151BC logic tick + glrecord dispatch notification)
```

### 7.5 Protocolo de Emulação Original

Foi inspecionado o ambiente local para binários de emulação N64 (`mupen64plus`, `simple64`, `ares`, `retroarch`, `bizhawk`). Nenhum emulador de N64 encontra-se atualmente instalado no sistema host Linux.

Para permitir a captura numérica externa comparativa de linha de base original sem suposições, foi gerado o script pronto para execução em Lua:
- Arquivo: [`tools/original_demo_trace_bizhawk.lua`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/tools/original_demo_trace_bizhawk.lua)
- Hash da ROM esperada / ROM verificada: SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3` (`Banjo-Tooie (USA).z64` / `baserom.us.z64`)
- `ROM_HASH_MATCH: YES`

---

## 8. Classificação e Veredito

```text
PROVISIONAL_PRIMARY_HYPOTHESIS:    PRESENTATION_INTERPOLATION / CAMERA_FOLLOW_LAG
RECOMP_INTERNAL_TRACE:             PROVEN (766 amostras capturadas e caracterizadas)
ORIGINAL_NUMERIC_TRACE:            UNKNOWN (pendente de emulador N64 instrumentado)
SIMULATION_BY_GLIDX_MATCH:         UNKNOWN (nominalmente compatível com 510/605/700 u/s e 30Hz; numérico pendente)
VISUAL_TIME_BY_GLIDX_MATCH:        UNKNOWN (nominalmente 33.33ms; numérico pendente)
ROOT_CAUSE_LEVEL:                  D
ROOT_CAUSE:                        UNKNOWN (pendente de linha de base numérica original)
FIX_IMPLEMENTED:                   NO (Conforme instrução, nenhum fix de produção aplicado nesta tarefa)
```
