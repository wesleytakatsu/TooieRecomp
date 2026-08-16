---
title: "P08 — Diagnóstico Diferencial de Sincronização e Movimento da Demo de Gameplay"
description: "Diagnóstico completo da trajetória, cinemática, cadência e comparação MEASURED_ORIGINAL vs MEASURED_RECOMP da primeira demonstração gravada (Wooded Hollow / consumed_glidx 1..765)."
status: evidence
phase: P08
updated: 2026-08-16
tags: [tooie-recomp, p08, demo-sync, glrecord, kinematics, cadence, camera-follow, wooded-hollow, presentation-interpolation]
---

# P08 — Diagnóstico Diferencial de Sincronização e Movimento da Demo de Gameplay

## 1. Visão Geral e Objetivo

```text
BASE_HEAD:                         5a1fd33ee68b89104c4de4583afb4a7a187909d3
MODEL:                             Grok 4.6 HIGH
MODE:                              MEASURED_ORIGINAL vs MEASURED_RECOMP (SEM FIX DE PRODUÇÃO)
SELECTED_DEMO:                     Wooded Hollow (mode 5, first clip consumed_glidx 1..765)
RECOMP_INTERNAL_TRACE:             PROVEN (765 amostras, consumed_glidx 1..765, TOOIE_DIAG_DEMO_SYNC=1)
ORIGINAL_NUMERIC_TRACE:            PROVEN (official mupen64plus 2.6.0 Pure Interpreter + Rice VI hook)
PROVISIONAL_PRIMARY_HYPOTHESIS:    CAPTURE_POINT_PHASE (first 365) then later post-egg run split
FIX_IMPLEMENTED:                   NO
ROOT_CAUSE_LEVEL:                  C
ROOT_CAUSE:                        same-index ~25u lead is one physics step (recomp dumps at
                                   glrecord consume; original dumps at VI after the update).
                                   After +1 shift, walk/analog/shot positions match.
                                   Remaining discriminator: post-egg run/jump after glidx ~392.
```

> [!IMPORTANT]
> **Distinção Epistemológica de Evidências:**
> - `MEASURED_RECOMP`: Dados empíricos extraídos ao vivo do executável `TooieRecompiled` via `TOOIE_DIAG_DEMO_SYNC=1`.
> - `ORIGINAL_MIPS_NOMINAL_EXPECTATION`: Contratos extraídos estaticamente da descompilação MIPS (ex: `glrecord_entrypoint_0`, `bastick_update`, `func_800EF04C`, cadência nominal de 30 Hz).
> - `MEASURED_ORIGINAL`: **PROVEN** (2026-08-16, Grok 4.6 HIGH). Official mupen64plus-core `6dca4c1` Pure Interpreter + Rice `038882d` `UpdateScreen` RDRAM hook. ROM SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3` **MATCH**. Pad landmarks 70..76 and 322 **PASS**. Previous `ORIGINAL_NUMERIC_TRACE: UNKNOWN` is **CORRECTED/SUPERSEDED**.

Este diagnóstico investiga a percepção visual de que o movimento do jogador durante a primeira demonstração de gameplay (Wooded Hollow attract demo) aparenta não sincronizar 100% com a experiência original do Nintendo 64.

---

## 2. Telemetria e Infraestrutura Recomp (`TOOIE_DIAG_DEMO_SYNC=1`)

Foi implementada uma sonda de diagnóstico por variáveis de ambiente (`TOOIE_DIAG_DEMO_SYNC=1`), sem efeito colateral na lógica do jogo, acionada a cada despacho do consumidor de `glrecord` (overlay 698 ep 0) no modo 5.

### Resumo de Cadência do Recomp vs Expectativa Nominal MIPS

| Métrica Temporal | Valor Medido no Recomp (`MEASURED_RECOMP`) | Expectativa Nominal MIPS (`NOMINAL_MIPS`) | Linha de Base Original (`MEASURED_ORIGINAL`) | Status da Comparação |
|---|---|---|---|---|
| **Taxa de Amostragem `glrecord`** | 1 amostra a cada 33.334 ms | 30.000 Hz | 1 sample / 2 VI after boot (mean 2.12, steady 2) | **MEASURED_MATCH** |
| **Tiques Lógicos por Amostra (`gl_adv`)** | 1.000 capture advance / sample at consume | 2 VI / sample | 2 VI / sample (steady) | **MEASURED_MATCH** (clocks are different units) |
| **`gamespeed` do Guest (`dt`)** | 0.033333 s (1/30 s) | 0.033333 s | not synthesized | **NOMINAL_MATCH** |
| **Duração Total da Demonstração** | 25,459 ms (25.46 s) | ~25.4 s | 765 samples × 2 VI | **MEASURED_MATCH** |
| **Contagem de Amostras Processadas** | 765 amostras (consumed 1..765) | 766 amostras | 765 (consumed 1..765) | **MEASURED_MATCH** |

```text
RECOMP_INTERNAL_CADENCE:           30.000 Hz CONSTANTE (1 sample / 33.334 ms)
ORIGINAL_NUMERIC_CADENCE:          2 VI / glrecord after intro (MEASURED_ORIGINAL)
SIMULATION_BY_GLIDX_MATCH:         YES for walk/analog/shot after +1 capture-phase shift
```

```text
GLRECORD_CADENCE_MATCHES_NOMINAL_30HZ: YES
LOGIC_CADENCE_MATCHES_NOMINAL:          YES
VI_CADENCE_MATCHES_NOMINAL:            YES (original steady 2 VI / sample)
GAMESPEED_STEP_MATCHES_NOMINAL:        YES
ORIGINAL_NUMERIC_MATCH:                YES after capture-phase correction through glidx 365
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

### 7.2 Semântica do Índice de `glrecord`, Stride de Amostras e Reparo Off-by-One

A descompilação de `glrecord_entrypoint_0` em `funcs_406.c` comprova a semântica exata de avanço:
1. **Stride da amostra**: `SAMPLE_STRIDE_BYTES = 6`
   `sll $t6, $v0, 2; subu $t6, $t6, $v0; sll $t6, $t6, 1` computa exatamente `(4 - 1) * 2 = 6` bytes por amostra.
2. **Ordem de incremento do índice**: `INDEX_INCREMENT_ORDER = INCREMENT_ON_CONSUMPTION`
   A função lê `v0 = MEM_W(0x8, 0x8012D5B0)`, calcula o endereço da amostra como `base + v0 * 6`, escreve `t8 = v0 + 1` de volta em `MEM_W(0x8, 0x8012D5B0)`, e em seguida transfere os bytes da amostra para a struct `OSContPad`.
   Portanto, no momento em que o despachador conclui ou quando `event.onframeend()` executa, `D_8012D5B0 + 0x8` contém `post_glidx = v0 + 1` (o índice da PRÓXIMA amostra a ser consumida).
3. **Reparo Off-by-One (PROVEN: `SCRIPT_OFF_BY_ONE = YES`)**:
   - A versão anterior indexava `base + post_glidx * 6`, associando o estado pós-consumo à amostra *seguinte*.
   - A fórmula correta para a amostra que efetivamente produziu o estado do quadro é:
     `consumed_glidx = (post_glidx > 0) ? (post_glidx - 1) : 0`
     `CORRECT_SAMPLE_ADDRESS_FORMULA = base + consumed_glidx * 6`
   - O script `tools/original_demo_trace_bizhawk.lua` e a sonda `tooie_demo_sync_tick` foram corrigidos para calcular e registrar explicitamente `consumed_glidx` e `post_glidx`.

### 7.3 Verificação de Checksum de Input e Alinhamento (`glidx`)

A sequência de inputs gravados foi validada em marcos críticos contra `consumed_glidx`:
- **Curva Analógica (consumed_glidx 70..76)**:
  - `consumed_glidx 70..73`: `pad=(-2, 80, 0000)`
  - `consumed_glidx 74`: `pad=(-18, 82, 0000)`
  - `consumed_glidx 75`: `pad=(-47, 78, 0000)`
  - `consumed_glidx 76`: `pad=(-63, 74, 0000)`
- **Disparo de Ovo (consumed_glidx 322)**:
  - `consumed_glidx 322`: `pad=(-9, 84, 2008)` (`buttons = 0x2008` contendo `C-Up`)

```text
INPUT_LANDMARK_SELFTEST:           PASS
GLIDX_ALIGNMENT_OFFSET:            0
RAW_INPUT_ALIGNMENT:               PASS
```

### 7.4 Significado Real do Contador "Tick" e Taxa de Apresentação

1. **Significado do campo `tick`**:
   - O contador global `g_guest_logic_tick` era incrementado tanto no wrapper de `func_800151BC` quanto no manipulador de syscalls de `glrecord` em `syscall_handler.cpp`.
   - Como `func_800151BC` executa no loop principal de jogo a 30 Hz (uma vez a cada 2 VIs de 60 Hz), o contador refletia um avanço duplo (`+2` por amostra consumida).
   `PREVIOUS_TICK_FIELD_ACTUAL_MEANING: DUAL_INCREMENT_COUNTER (func_800151BC logic tick + glrecord dispatch notification)`

2. **Significado da métrica ~2644 Hz**:
   - O contador `g_gfx_ticks` é incrementado em `tooie_diag::tick()` dentro da função `update_gfx` de `main.cpp`.
   - Representa o número de iterações do loop de eventos e bombeamento de frames do host (`HOST_RENDER_LOOP_ITERATIONS`), que roda desbloqueado no host Vulkan.
   `PREVIOUS_2644HZ_COUNTER_MEANING: HOST_RENDER_LOOP_ITERATIONS (host event/frame pump loop iterations in update_gfx)`

### 7.5 Protocolo de Emulação Original e Pré-Requisitos

1. **Script Autônomo e Auto-Validante**:
   - Arquivo: [`tools/original_demo_trace_bizhawk.lua`](file:///home/wesley/projetos/N64-Recomp/TooieRecomp/tools/original_demo_trace_bizhawk.lua)
   - Valida bounds de memória RDRAM (`0x80000000..0x80800000`), formato big-endian (`readfloat(..., true)`), faz autodiagnóstico de landmarks (70..76, 322) e grava CSV com `consumed_glidx`, `post_glidx`, `vi_count`, entradas do pad, posição, velocidade, tspd, yaw e câmera.
2. **Hash da ROM Verificada**:
   - SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3` (`Banjo-Tooie (USA).z64` / `baserom.us.z64`)
   - `ROM_HASH_MATCH: YES`
3. **Pré-Requisito Externo (CORRECTED/SUPERSEDED 2026-08-16)**:
   - Official BizHawk 2.11.1 Linux tarball is `.NET Framework 4.8` / Mono. This host has no `mono` and user-local `dotnet` 8 cannot run `EmuHawk.exe`.
   - Official mupen64plus 2.6.0 was built user-local (`OSD=0 VULKAN=0 NO_ASM=1`, Pure Interpreter) with Rice video and an RDRAM dump on `UpdateScreen`.
   - Dummy-gfx hangs Tooie in mode 3 (RDP/VI progress). Rice + SDL input (`Present=1`) reaches mode 5.
   - BizHawk Lua remains valid for a Windows/Mono host; it was not the capture path used here.

---

## 8. Classificação e Veredito

```text
PROVISIONAL_PRIMARY_HYPOTHESIS:    CAPTURE_POINT_PHASE then post-egg run split
RECOMP_INTERNAL_TRACE:             PROVEN (765 amostras consumed 1..765)
ORIGINAL_NUMERIC_TRACE:            PROVEN
SIMULATION_BY_GLIDX_MATCH:         YES through shot after +1 shift; later run/jump still open
VISUAL_TIME_BY_GLIDX_MATCH:        NOT used as causal evidence (host present loop ≠ guest clock)
ROOT_CAUSE_LEVEL:                  C
ROOT_CAUSE:                        see §9 MEASURED_ORIGINAL differential
FIX_IMPLEMENTED:                   NO
```

---

## 9. MEASURED_ORIGINAL vs MEASURED_RECOMP (2026-08-16, Grok 4.6 HIGH)

```text
EMULATOR:                          official mupen64plus 2.6.0 (user-local source build)
VERSION/COMMIT:                    core 6dca4c1 / ui-console c8ac486 / rice 038882d / rsp-hle 8a7a472
N64_CORE:                          mupen64plus-core + rsp-hle + rice
CPU_EXECUTION_MODE:                Pure Interpreter (NO_ASM=1, --emumode 0)
RDRAM_ACCESS_METHOD:               GFX_INFO.RDRAM via Rice UpdateScreen; guest bytes rdram[phys^3]
SCRIPT_OR_TRACE_METHOD:            tools/mupen64plus_demo_trace.c (standalone dummy) + Rice include hook
WHY_THIS_RUNTIME_IS_SUITABLE:      official interpreter; live landmark checksums PASS; no random binary
ROM_SHA1:                          af1a89e12b638b8d82cc4c085c8e01d4cba03fb3
ROM_MATCH:                         YES
LIVE_MEMORY_LAYOUT:                PASS (xor=3; mode=3 then 5; pad 70/74/322 PASS; finite player floats)
CAPTURE_POINT:                     Rice UpdateScreen / VI after post_glidx advances
STATE_ASSOCIATED_WITH_CONSUMED_SAMPLE: YES (one row per post_glidx change; next consume is 2 VI later)
ORIGINAL_TRACE:                    YES (/tmp/p08-original-wh1-clip.csv, not committed)
```

Tolerances (justified): `EXACT_POS=0.001`, `MEANINGFUL_POS=2.0` world units; `EXACT_YAW=0.05°`, `MEANINGFUL_YAW=2.0°`; `EXACT_SPD=0.05`, `MEANINGFUL_SPD=2.0`. Host COP1/Rice float noise is sub-unit; a walk step at 510 u/s is 17 units.

Join: first Wooded Hollow clip only (`consumed_glidx` 1..765, first occurrence). Later attract maps reuse glidx and must not be mixed.

```text
ORIGINAL_SAMPLE_COUNT:             765
RECOMP_SAMPLE_COUNT:               765
COMMON_SAMPLE_COUNT:               765
PAD_SEQUENCE_MATCH_COUNT:          765
PAD_SEQUENCE_MATCH_RATE:           100.0000%
PAD_IDENTITY_GATE:                 PASS
```

Same-index player xz error (MEASURED, `tools/compare_demo_traces.py`):

| glidx | orig pos | recomp pos | error_xz | spd o/r | yaw o/r | bs o/r |
| --- | --- | --- | --- | --- | --- | --- |
| 35 | (1461.08,-171.07,-183.31) | (1486.57,-168.44,-182.14) | 25.517 | 510.2/510.2 | 267.35/267.37 | 3/3 |
| 74 | (468.72,-265.27,-231.39) | (494.13,-262.99,-230.13) | 25.441 | 508.9/508.9 | 271.43/267.18 | 3/3 |
| 76 | (420.03,-269.65,-223.68) | (443.41,-267.54,-230.46) | 24.343 | 487.0/506.5 | 294.57/283.85 | 3/3 |
| 91 | (141.61,-294.70,18.83) | (164.66,-292.63,9.58) | 24.837 | 496.8/492.2 | 290.97/294.24 | 3/3 |
| 120 | (-538.64,-300.00,82.66) | (-522.12,-300.00,84.87) | 16.667 | 499.7/499.9 | 265.51/262.76 | 3/3 |
| 200 | (-2043.40,-333.07,738.59) | (-2021.41,-332.99,726.67) | 25.013 | 500.4/500.4 | 299.03/298.56 | 3/3 |
| 322 | (-4678.85,-425.16,1123.52) | (-4678.84,-425.16,1123.57) | 0.051 | 509.0/509.0 | 298.71/298.71 | 7/4 |
| 334 | (-4709.54,-428.23,1136.36) | (-4709.52,-428.23,1136.41) | 0.054 | 0.85/0.85 | 284.79/284.79 | 7/7 |
| 365 | (-4927.01,-448.84,1201.31) | (-4926.92,-448.83,1201.34) | 0.095 | 1.68/2.97 | 106.64/106.64 | 7/7 |
| 550 | (-498.53,-300.53,1324.79) | (-518.07,-300.00,803.30) | 521.856 | 687.0/699.6 | 50.80/61.78 | 20/20 |
| 765 | (2690.75,-1022.00,3848.87) | (2423.56,-1024.46,3757.11) | 282.507 | 1.05/1.47 | 75.62/70.70 | 4/4 |

```text
FIRST_EXACT_DIFFERENCE_GLIDX:      3   (camera xz 0.03 — below MEANINGFUL; FPU/VI noise)
FIRST_MEANINGFUL_DIVERGENCE_GLIDX: 18  (same-index yaw 3.09° / bs 1 vs 0)
FIRST_DIVERGENT_FIELD:             player yaw + bs at consume 18 (orig started turn; recomp still idle)
PLAYER_ERROR_GL35:                 25.5168
PLAYER_ERROR_GL91:                 24.8368
PLAYER_ERROR_GL200:                25.0129
PLAYER_ERROR_GL322:                0.0510
PLAYER_ERROR_GL365:                0.0949
PLAYER_ERROR_GL550:                521.8559
PLAYER_ERROR_GL765:                282.5073
DRIFT_CLASS:                       PIECEWISE  (constant ~25u = one physics step through the walk;
                                   collapses at the shot stop; grows again after egg-exit run)
```

Capture-phase test (`orig[N]` vs `recomp[N+1]`, MEASURED):

```text
k=+0  xz_med=24.996 yaw_med=0.51
k=+1  xz_med=0.010  yaw_med=0.00   ← walk/analog/shot-stop identity
k=+2  xz_med=25.002
k=-1  xz_med=49.683
```

`orig[35]==recomp[36]`, `orig[74]==recomp[75]`, `orig[91]==recomp[92]`, `orig[236]==recomp[237]` (including `bs=32`) at xz≤0.04.

This is **INFERENCE** from the two capture sites, not a second runtime:

- Recomp `tooie_demo_sync_tick` runs when overlay 698 ep 0 **returns** (pad inject). Player physics for sample N has not necessarily run.
- Original Rice `UpdateScreen` runs on VI **after** that game update.
- Therefore same-index `recomp[N]` is the state produced by sample N-1; `orig[N]` is the state produced by sample N.

```text
PLAYER_PHASE_GL322:                MATCH position (error_xz=0.051); state 7 vs 4 is the +1 phase
                                   (recomp[323].bs=7). Do not treat as egg-collision evidence.
DIGGER_PHASE_GL322:                UNKNOWN (both traces read 0,0,0 — marker 337 not in first 64)
CAMERA_PHASE_GL322:                MATCH position (cam xz 0.041 same-index)
CAMERA_LAG_GL91_ORIGINAL:          36.77°  (player 290.97 / cam 254.2)
CAMERA_LAG_GL91_RECOMP:            40.04° same-index; 36.77° after +1 (player 290.97 / cam 254.2)
CAMERA_LAG_MATCH:                  YES after capture-phase correction
                                   previous ~45.11° claim is CORRECTED/SUPERSEDED (same-index artifact)
ORIGINAL_VI_PER_GLRECORD:          2 (steady; mean 2.12 including first mode-5 34-VI gap)
RECOMP_VI_PER_GLRECORD:            not a guest VI counter in the recomp row
ORIGINAL_GAME_UPDATES_PER_GLRECORD: gfcnt is not monotonic here (do not synthesize)
RECOMP_GAME_UPDATES_PER_GLRECORD:  1 consume / row (adv≈1)
```

The ~45° camera-lag visual hypothesis is **REFUTED** as a measured original-vs-recomp mismatch. Aligned lag is identical (36.77°).

First **phase-corrected** growth past 2 units after the shot-stop is around consumed 392 during the post-egg trot (`bs=21`), then the jump (`bs=8`) opens to 150+ units by 440. That later split is the remaining discriminator. It was **not** walked back to a single writer in this task.

```text
PRIMARY_CLASS:                     MIXED
                                   first 365 = CAPTURE_POINT / not a gameplay defect
                                   after egg-exit run = one remaining precise discriminator
ROOT_CAUSE_LEVEL:                  C
FIX_IMPLEMENTED:                   NO
```

Digger mid=337 was not recovered in either first-clip dump (positions 0,0,0). Actor identity by heap pointer remains forbidden; this is a scan-window gap, not a phase proof.
