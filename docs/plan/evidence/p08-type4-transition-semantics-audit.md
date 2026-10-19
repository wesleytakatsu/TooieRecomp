---
title: "P08 — Auditoria Estática da Semântica Interna da Transition Type-4"
description: "Auditoria estática e snapshot clean-freeze da transição Type-4 intro para attract demo."
status: proven
type: evidence
project: TooieRecomp
phase: P08
date: 2026-08-12
tags: [p08, transition, type4, intro, attract, clean-freeze]
---

# P08 — Auditoria Estática da Semântica Interna da Transition Type-4 (INTRO → ATTRACT DEMO)

## HEAD OBRIGATÓRIO
`7f112fe2ee7d38fca1b2678b2cbf8fe09d1183e6`

## STATUS DO DOCUMENTO
Canonical static analysis checkpoint — P08 Type-4 Internal Semantics.

> **CORRECTED / SUPERSEDED (2026-08-12):** o clean-freeze exact type-4 snapshot
> (seção 11) REFUTA a narrativa de type-4 incompleta. Nos 8/8 runs naturais a
> type-4 **completa** (`current_state=2`, `render_frame_count=2`, gate do warp
> ABERTO). Os valores `0.9 / 3.5 / 2883` são `+0x0C target_duration` /
> `+0x18 cfg_param3` / `+0x10 cfg_sfx_id`. O `2883` **não** é tick counter; o
> `+0x30 state_frame_counter` real vale `1`. O freeze natural é downstream
> (mode-5/attract init, `func_800C2AB8`). As seções 1–10 abaixo permanecem como
> análise estática histórica; a seção 11 é a fonte corrente.

---

## 1. PROVEN — LAYOUT DO OBJETO TRANSITION (SIZE 0x34 / 52 BYTES)

Alocado via `func_8001ACCC(0x34)` e zerado via `func_8002D6E0(ptr, 0x34)` em `func_800C9150`.

| Offset | Width | Type | Readers | Writers | Proven Semantic | Confidence |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `+0x00` | 4 | `u32` | `func_800C91C8`, `ep0` | `ep8` | **Transition Type / Slot ID** (e.g. 4 para type-4) | HIGH (100%) |
| `+0x04` | 4 | `u32` | `ep4` (`slti < 2`), `ep0` | `func_808000D8` (zera), `ep0` (`+1` por render) | **Render Frame / Phase Counter** (incrementado em cada chamada de render `ep0`; `ep4` exige `>= 2` no state 2 para liberar o warp) | HIGH (100%) |
| `+0x08` | 1 | `u8` | `ep9` (se 0 sai sem tick), `func_808000D8`, `ep0` | `ep8` (zera), `func_808000D8` (`memcpy` do config `+0x0`) | **Active Kind / Transition Subtype** (0 = inativo/skip tick, `>0` = ativo; copiado da tabela de configs) | HIGH (100%) |
| `+0x09` | 1 | `u8` | `ep0` | `func_808000D8` (`memcpy` do config `+0x1`) | **Config State ID** | HIGH (100%) |
| `+0x0A` | 1 | `u8` | `ep9` (`lbu $a1, 0xA($a3)` -> next_state ao completar) | `func_808000D8` (`memcpy` do config `+0x2`) | **Next State ID** (estado de destino quando `progress >= duration`, e.g. state 2) | HIGH (100%) |
| `+0x0B` | 1 | `u8` | `ep0` | `func_808000D8` (`memcpy` do config `+0x3`) | **Config Padding / Flags** | HIGH (100%) |
| `+0x0C` | 4 | `f32` | `ep9` (`lwc1 $f0, 0xC` comparado com `0x28`) | `func_808000D8` (`memcpy` do config `+0x4`) | **Target Duration (segundos)** (e.g. `0.90f` no type 6, `3.50f` no type 10, `2.00f` no type 5) | HIGH (100%) |
| `+0x10` | 4 | `u32` | `func_808000D8`, `ep0` | `func_808000D8` (`memcpy` do config `+0x8`) | **Config Parameter 1 / Default SFX ID** (e.g. `2883` no type 5/6, `2887` no type 10) | HIGH (100%) |
| `+0x14` | 4 | `u32` | `func_808000D8` (passado a `func_8008AED4`) | `func_808000D8` (`memcpy` do config `+0xC`) | **Config Parameter 2 / Subactor Animation ID** | HIGH (100%) |
| `+0x18` | 4 | `u32/f32`| `func_808000D8`, `ep0` | `func_808000D8` (`memcpy` do config `+0x10`) | **Config Parameter 3 / Visual Scale / Duration** | HIGH (100%) |
| `+0x1C` | 1 | `u8` | `ep2` (`==1`), `ep3` (`!=1`), `ep4` (`{2,4,6,7,9}`), `ep9`, `func_808000D8` | `ep8` (zera), `func_808000D8` (`sb $t2, 0x1C`) | **Current State ID** (0=uninitialized, 1=idle/removal, 2=fade-out hold, 5/6=active spin/fade) | HIGH (100%) |
| `+0x1D` | 1 | `u8` | `ep9` (se 0 -> `func_800D8FF8`, se !=0 -> `glintrosyncDll_ep5`) | `ep8` (`sb $v0` retornado de `func_800E6A00()`) | **Timing Source Flag** (0 = delta global frame, 1 = intro sync delta) | HIGH (100%) |
| `+0x1E` | 1 | `u8` | - | - | **Padding** | HIGH |
| `+0x1F` | 1 | `u8` | - | - | **Padding** | HIGH |
| `+0x20` | 4 | `u32` | `ep0` | `ep8` (zera), `func_808000D8` (SFX `0xB45`/`0xB46`/`config->arg1`) | **Active SFX / Sound ID** (copiado de `config->arg1`, e.g. **2883** para type 5 e 6) | HIGH (100%) |
| `+0x24` | 4 | `f32` | `ep9` (`lwc1 0x24`, subtrai 90.0° no state 5/6), `ep0` | `ep8` (zera), `ep9` (atualiza ângulo) | **Rotation Angle (graus)** (usado no efeito visual de rotação do Jiggy) | HIGH (100%) |
| `+0x28` | 4 | `f32` | `ep9` (`lwc1 0x28` comparado com duration `+0x0C`) | `func_808000D8` (zera na entrada de state), `ep9` (`+dt` a cada frame) | **Elapsed Time / Progress (segundos)** | HIGH (100%) |
| `+0x2C` | 4 | `ptr` | `ep9` (`func_8008ADE4`), `func_808000D8` (`func_8008ADBC`), `ep0` | `func_808000D8` (`func_8008AD80` ou 0) | **Subactor Pointer / Animation Controller (`anctrl*`)** | HIGH (100%) |
| `+0x30` | 4 | `u32` | `ep9` (jump table `0x30 < 5` no state 5; `0x30 < 3` no state 6) | `func_808000D8` (zera na mudança de estado), `ep9` (`+1` por tick) | **State Frame / Tick Counter** (número de frames executados dentro do estado corrente) | HIGH (100%) |

---

## 2. PROVEN — CICLO DE CRIAÇÃO E INICIALIZAÇÃO

1. `func_800C9610`:
   - Chama `func_800C91C8(4)`.
   - Se não existir, chama `func_800C9150(4)`:
     - `malloc(0x34)` -> `bzero(ptr, 0x34)`.
     - Registra `ptr` na lista global de transitions em `0x8012AB40`.
     - Incrementa o contador de transitions em `0x8012AB54`.
     - Chama `_gctransitionDll_entrypoint_8(ptr, 4)`.
   - `_gctransitionDll_entrypoint_8(ptr, 4)`:
     - `ptr->type (+0x00) = 4`
     - `ptr->active (+0x08) = 0`
     - `ptr->state (+0x1C) = 0`
     - `ptr->angle (+0x24) = 0.0f`
     - `ptr->timing_flag (+0x1D) = func_800E6A00()` (`*(u8*)0x801275E0`)
     - Chama `func_808000D8(ptr, new_state=1, config=0)` -> inicializa para `state = 1`, `previous_state = 0` (não desaloca).
2. `func_800C9610` em seguida chama `_gctransitionDll_entrypoint_6(ptr)`:
   - Chama `func_800EA05C()` -> obtém ID da transição do frontend/cena.
   - Chama `func_8080004C(mode)` -> busca linha na Tabela `0x8080105C`.
   - Obtém `exit_type` (`u8` em `entry+3`).
     - Para transição padrão/jiggy (mode 0): `exit_type = 6`.
     - Para mode 167 (0x00A7): `exit_type = 10`.
   - Chama `func_80800000(exit_type)` -> busca configuração na Tabela `0x80800CB0`.
     - Para `type 6`: `state = 6, next_state = 2, duration = 0.90f, SFX = 2883 (0x0B43)`.
     - Para `type 10`: `state = 6, next_state = 2, duration = 3.50f, SFX = 2887 (0x0B47)`.
   - Chama `func_808000D8(ptr, new_state=config->state, config_ptr)`:
     - Copia 20 bytes do config para `ptr + 0x8`.
     - `ptr->state (+0x1C) = 6`.
     - `ptr->duration (+0x0C) = config->duration` (`0.90f` ou `3.50f`).
     - `ptr->sfx (+0x20) = config->arg1` (`2883` ou `2887`).
     - `ptr->progress (+0x28) = 0.0f`.
     - `ptr->frame_counter (+0x30) = 0`.
     - Cria subactor se `config->arg2 != 0`.

---

## 3. PROVEN — PSEUDOCÓDIGO DE `gctransitionDll_entrypoint_9` (TICK UPDATE)

```c
void gctransitionDll_entrypoint_9(TransitionObj* obj) {
    float dt;

    // A. Obtenção do delta temporal
    if (obj->timing_flag != 0) {
        dt = glintrosyncDll_entrypoint_5(); // func_800D8FF8() * (*(float*)0x801275DC)
    } else {
        dt = func_800D8FF8();               // *(float*)0x8012C760 (global frame delta)
    }

    // B. Verificação de objeto ativo
    if (obj->active_kind == 0) {
        return; // Retorna imediatamente sem alterar estado ou contadores
    }

    // C. Subactor e atualização de tempo por estado
    if (obj->subactor != NULL) {
        func_8008ADE4(obj->subactor); // anctrl_update

        if (obj->state == 5) {
            // State 5: primeiros 4 frames executam setup via jump table, depois acumula dt
            if (obj->frame_counter < 5) {
                switch (obj->frame_counter) {
                    case 0: break;
                    case 1: func_80090684(1); func_800EA334(0); break;
                    case 2: func_800EA34C(0); break;
                    case 3: break;
                    case 4: func_8002FBB0(0); break;
                }
            } else {
                obj->elapsed_time += dt;
            }
        } else {
            // Outros states (ex: State 6): frames 0..2 ignoram dt, frame >= 3 acumula dt
            if (obj->frame_counter == 0) {
                // pular acumulação
            } else if (obj->frame_counter == 1) {
                // pular acumulação
            } else if (obj->frame_counter == 2) {
                func_800EA34C(0);
            } else {
                obj->elapsed_time += dt;
            }
        }
    } else {
        // Sem subactor: acumula dt diretamente
        obj->elapsed_time += dt;
    }

    // D. Avaliação de completion / avanço temporal
    bool completed = false;
    if (obj->elapsed_time > obj->target_duration) {
        completed = true;
    } else if (obj->subactor != NULL) {
        // Se subactor parou antes do tempo, força completion antecipada
        if (anctrl_isStopped(obj->subactor)) {
            completed = true;
        }
    }

    if (completed) {
        obj->elapsed_time = obj->target_duration; // clamp

        // E. State transition
        if (obj->state == 5 || obj->state == 6) {
            obj->angle -= 90.0f;
            if (obj->angle < -360.0f) obj->angle += 360.0f;
            if (obj->angle > 360.0f)  obj->angle -= 360.0f;
        }

        uint8_t next = obj->next_state; // lido de +0x0A (ex: 2)
        func_808000D8(obj, next, NULL); // transiciona para state 2!
    }

    if (obj->subactor != NULL) {
        func_800EA170(obj);
    }

    // F. Incremento do contador de ticks no estado corrente
    obj->frame_counter += 1;
}
```

---

## 4. PROVEN — DESMISTIFICAÇÃO DOS TRÊS CAMPOS (0.9, 3.5, 2883)

### A. Campo `≈0.9` (`+0x0C` ou `+0x28`)
- **Origem Estática Provada:** Na Tabela de Configurações `0x80800CB0`, a entrada `type = 6` (Jiggy exit transition) tem `duration = 0.90f`!
- **Significado:** `0.90` é a **duração nominal exata em segundos** da transição type-6 (ou o progresso `+0x28` atingindo o clamp `0.90f` ao completar).

### B. Campo `≈3.5` (`+0x0C`)
- **Origem Estática Provada:** Na Tabela de Configurações `0x80800CB0`, a entrada `type = 10` (utilizada pelo mode 167) tem `duration = 3.50f`.
- **Significado:** `3.50` é a **duração nominal exata em segundos** da transição type-10.

### C. Campo `≈2883` (`+0x20` e `+0x30`)
> **CORRECTED (2026-08-12):** o clean snapshot prova `+0x20=2885`,
> `+0x30=1`; o `2883` real está em **`+0x10` (`cfg_sfx_id`)**. A especulação
> abaixo de “+0x30 ≈ 2883 ticks” está REFUTADA.
- **Origem Estática Provada 1 (`+0x20`):** Na Tabela `0x80800CB0`, as entradas `type = 5` e `type = 6` possuem `arg1 = 2883 (0x0B43)`. Esse valor é copiado diretamente para `obj + 0x20` como o **SFX ID do som de fechamento/abertura do Jiggy**. (CORRECTED: o `arg1 = 2883` é copiado para `obj + 0x10`; `+0x20 active_sfx` vale `2885` no snapshot limpo.)
- **Origem Estática Provada 2 (`+0x30`):** `obj + 0x30` é o contador de frames do estado (`frame_counter`). (CORRECTED: `+0x30` vale `1` no snapshot limpo; NÃO contém 2883.)

---

## 5. PROVEN — CONDIÇÃO REAL DE COMPLETION E REMOÇÃO

O desbloqueio do warp e a remoção da transition type-4 ocorrem em 2 estágios independentes:

### Estágio 1: Desbloqueio do Warp (`func_800C9510() == 0`)
`func_800C9510()` consulta `gctransitionDll_entrypoint_4(obj)`:
```c
bool is_blocking(TransitionObj* obj) {
    switch (obj->state) {
        case 4: return true;
        case 6: return true;
        case 9: return true;
        case 2: return (obj->render_frame_count < 2); // +0x04 deve ser >= 2
        case 7: return (obj->render_frame_count < 2);
        default: return false;
    }
}
```
1. Enquanto `state == 6`, o warp está bloqueado (`func_800C9510() == 1`).
2. Quando `progress >= duration`, `entrypoint_9` chama `func_808000D8(obj, next_state=2, NULL)`.
3. No `state == 2`, `obj->render_frame_count (+0x04)` inicia em 0.
4. A cada frame renderizado por `gctransitionDll_entrypoint_0`, `+0x04` é incrementado em 1 (`sw $t7, 0x4($a0)`).
5. Após 2 frames de renderização (`+0x04 >= 2`), `entrypoint_4` retorna `0`.
6. `func_800C9510()` retorna `0`, liberando `func_800A9924` para consumir o warp e iniciar o Attract Demo mode 5!

### Estágio 2: Remoção Física do Objeto (`free`)
Quando o novo mapa/modo termina de carregar e a tela é aberta, uma transição para `state = 1` é solicitada via `func_808000D8(obj, 1, NULL)`.
Em `func_808000D8`:
```mips
lbu  $t1, 0x1C($s0)        // previous state
beql $t1, $zero, L_80800308 // se previous state == 0 (init), não desaloca
jal  0x800C9214            // SE previous state != 0: DESALOCA E REMOVE DA LISTA
```
A transição é desalocada e `transition_count` decrementa de 1 para 0.

---

## 6. PROVEN — TIMING SOURCE E MULTIPLIER INTRO SYNC

1. `func_800D8FF8`: Retorna `*(float*)0x8012C760` (delta time do jogo por frame em segundos, tipicamente 0.01667f / 0.03333f).
2. `_glintrosyncDll_entrypoint_5`:
   - Verifica `func_800C954C()` (`state != 1`).
   - Se `true`: obtém `dt = func_800D8FF8()` e multiplica por `*(float*)0x801275DC`.
   - Se `false`: obtém `dt = func_800D9004()` e multiplica por `*(float*)0x801275DC`.
3. `*(float*)0x801275DC` é o fator de escala de velocidade de sincronia da Intro.
   - **Vulnerabilidade Estática Crucial:** Se `*(float*)0x801275DC == 0.0f`, o delta time retornado para `entrypoint_9` será **rigorosamente 0.0f** em todos os frames, congelando `obj->elapsed_time` no valor em que estiver!

---

## 7. PROVEN — AUDITORIA DE `func_800C2AB8` / `get_function(0x800C269C)`

- **Função `func_800C2AB8`:** Loop de atualização per-frame dos 60 slots da tabela de atores/itens (`0x80128C10`).
- **Função `func_800C269C`:** Função auxiliar de consulta de flags de cada ator.
- **Duração:** Execução de fração de microssegundo; sem loops infinitos nem recursão.
- **Relação com Transition Type-4:** **INCIDENTAL**. A captura no post-mortem do GDB simplesmente ocorreu durante a execução normal do loop de atores no meio do game tick.

---

## 8. CORRECTED — CORREÇÕES EM RELAÇÃO À AUDITORIA ANTERIOR

> **REFINADO (2026-08-12):** o clean snapshot refina o item 1 (2883 está em
> `+0x10`, não `+0x20`; `+0x30` vale `1`, não 2883) e o item 2 (0.9 é o
> `target_duration` em `+0x0C`; 3.5 é `cfg_param3` em `+0x18`, não a duração da
> type-4 deste freeze).

1. `2883` **NÃO É um tick counter aleatório:** É o **SFX ID literal `2883 (0x0B43)`** da tabela de transições ROM `0x80800CB0` (transições 5 e 6). CORRECTED: no snapshot limpo o `2883` está em **`+0x10` (`cfg_sfx_id`)**; `+0x20` vale `2885`; `+0x30` vale `1`.
2. `0.9` e `3.5` **NÃO SÃO valores arbitrários:** `0.9` é o **`target_duration`** (em `+0x0C`, config type 6) e `3.5` é o **`cfg_param3`** (em `+0x18`) da type-4 deste freeze.
3. `+0x04` **NÃO É phase genérico:** É o **contador de frames de renderização** (`render_frame_count`). O gate de saída do `state 2` (`func_800C9510`) aguarda `+0x04 >= 2`. No snapshot limpo `+0x04 = 2` → gate ABERTO.

---

## 9. UNKNOWN — QUESTÕES QUE REQUEREM DISCRIMINADOR DINÂMICO LIMPO

> **RESOLVIDO pelo clean snapshot (seção 11):** (1) a type-4 alcançou o
> `state 2` com `+0x04 = 2` (render funcionou); (2) `0x801275DC = 1.0f`;
> (3) `timing_flag (+0x1D) = 1`.

1. No Clean Freeze, a transition type-4 parou no `state 6` (com `elapsed_time` congelado por `dt == 0`) ou alcançou o `state 2` mas `+0x04` nunca atingiu `2` porque o render `ep0` parou de ser chamado após o encerramento do frame da intro?
2. O valor de `*(float*)0x801275DC` (sync speed) no clean freeze é `0.0f` ou `1.0f`?
3. O byte `ptr->timing_flag (+0x1D)` na transição type-4 do clean freeze vale `0` ou `1`?

---

## 10. NEXT TEST — MINIMAL DISCRIMINATOR SNAPSHOT

Realizar uma única inspeção post-mortem (via dump de memória / snapshot limpo sem hooks em hot paths) logo após o congelamento da intro:

Inspecionar exatamente os 6 campos no objeto type-4 (`~0x8027AAC0`):
1. `+0x04`: `render_frame_count` (uint32)
2. `+0x0C`: `target_duration` (float)
3. `+0x1C`: `current_state` (uint8)
4. `+0x1D`: `timing_flag` (uint8)
5. `+0x28`: `elapsed_time` (float)
6. `+0x30`: `frame_counter` (uint32)
E os 2 endereços globais de timing:
7. `*(float*)0x801275DC`: Intro sync playback speed
8. `*(float*)0x8012C760`: Global frame delta time

Com esses 8 valores pontuais capturados uma única vez após o freeze, a causa raiz exata entre (a) parada de avanço temporal por `dt == 0`, (b) congelamento do render no state 2, ou (c) divergência de estado será 100% discriminada.

---

## 11. Clean-freeze exact type-4 snapshot

### MODO

DYNAMIC POST-MORTEM ONLY, sem debugger/hook/counter/instrumentação durante a
execução. Supervisor pai em C (`/tmp/opencode/p08/supervisor.c`) que executa o
binário original, confirma o freeze natural pelo log diag e só então faz
`PTRACE_ATTACH` (parent, `ptrace_scope=1`) e lê `/proc/<pid>/mem`. Nenhum
breakpoint, nenhum hook, nenhuma instrumentação; o jogo roda livre até o freeze.
Decoder independente em Python (`/tmp/opencode/p08/decode_snapshot.py`) valida o
snapshot `.bin` com o memory model dos macros `MEM_W/MEM_H/MEM_BU` do N64Recomp
(u32 nativo LE; byte com `^3`; halfword com `^2`).

```sh
TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
./build/TooieRecompiled
```

Resultado: **8/8 runs** congelaram de forma idêntica e determinística. Assinatura
do freeze: `d` estável (`178958`..`179320`), `last_ovl=635:2`, black screen
pós-intro. Endereço do objeto e todos os campos idênticos em todos os runs.

### STATIC EXPECTATION (auditoria estática anterior)

- type-4 "incompleta": `elapsed/progress ≈ 0.9`, `duration/target ≈ 3.5`,
  raw field `≈ 2883`, no `state 6`; warp permanece gated.
- Hipotese de gate: `elapsed` não avança até `>= target`, ou `dt == 0`, ou
  render não incrementa `+0x04` no state 2.

### CLEAN SNAPSHOT (ground truth, run 8 — idêntico nos 8 runs)

Objeto localizado via transition manager, não por endereço assumido:

```text
TRANSITION_OBJECT: 0x8027AAC0
TYPE:              4
COUNT:             1
```

| Offset | Campo | Valor (hex raw) | Interpretação |
| :--- | :--- | :--- | :--- |
| `+0x00` | type | `0x00000004` (4) | transition type 4 |
| `+0x04` | render_frame_count | `0x00000002` (2) | 2 renders no state 2 |
| `+0x08` | active_kind | `0x00` (0) | inativo (pós-completion) |
| `+0x09` | cfg_state | `0x06` (6) | config type-6 (Jiggy) |
| `+0x0A` | next_state | `0x02` (2) | destino = state 2 |
| `+0x0C` | target_duration | `0x3F666666` → `0.899999976` | **0.9 s** |
| `+0x10` | cfg_sfx_id | `0x00000B43` (2883) | **2883 = SFX config** |
| `+0x14` | cfg_subactor_id | `0x00000000` (0) | sem subactor |
| `+0x18` | cfg_param3 | `0x40600000` → `3.500000000` | **3.5 = visual param** |
| `+0x1C` | current_state | `0x02` (2) | **state 2** |
| `+0x1D` | timing_flag | `0x01` (1) | usa delta * sync |
| `+0x20` | active_sfx_id | `0x00000B45` (2885) | **2885 ≠ 2883** |
| `+0x24` | rotation_angle | `0xC2B40000` → `-90.0` | completou (6→2) |
| `+0x28` | elapsed_time | `0x00000000` → `0.0` | zerado na entrada do state 2 |
| `+0x2C` | subactor_ptr | `0x00000000` (NULL) | sem subactor |
| `+0x30` | state_frame_counter | `0x00000001` (1) | **1 tick no state 2** |

Timing global no mesmo snapshot:

```text
DELTA_GLOBAL_ADDR: 0x8012C760   raw=0x3D4CCCCE  float=0.050000004
SYNC_SPEED_ADDR:   0x801275DC   raw=0x3F800000  float=1.000000000
TIMING_FLAG:       +0x1D = 1
EFFECTIVE_DT:      delta * sync = 0.050000004   (glintrosync branch)
EFFECTIVE_DT_ZERO: NO
EFFECTIVE_DT_REASONABLE: YES  (~20 Hz de frame delta neste ambiente)
```

Estado do flow (mesmo snapshot, addresses da mode machine e do attract):

```text
MODE MACHINE 0x80127630:
  +0x02 diag_mode = 2      +0x08 timer_f = 51.07
  +0x10 map = 0 (limpo)    +0x12 warp = 0 (consumido)   +0x13 entr = 0
  +0x1C stored_mode_arg = 5   <- gravado por func_800A76F4(5)
ATTRACT D_80127760:
  +0x0 kind = 2   +0x4 flag = 0   +0x6 map = 335   +0x8 entrance = 8   +0xA = 1
```

### MATCH

- Freeze natural reproduzido: `d` estável >= 5 s, `last_ovl=635:2`, black
  pós-intro, `transition count=1`, `type=4`, objeto `0x8027AAC0`.
- O objeto tem `+0x0C=0.9` e `+0x18=3.5` — valores numéricos iguais aos
  documentados (0.9 / 3.5).
- `2883` está presente no objeto (em `+0x10`, `cfg_sfx_id`).

### DIVERGENCE (semântica, em relação à auditoria anterior)

1. **O antigo "elapsed ≈ 0.9" NÃO é `+0x28`.** No snapshot limpo `+0x28=0.0`
   (zerado por `func_808000D8` na entrada do state 2). O `≈ 0.9` lido antes é
   `+0x0C` (**target** 0.9).
2. **O antigo "duration ≈ 3.5" NÃO é `+0x0C`.** O `3.5` é `+0x18`
   (**cfg_param3** = 3.5f), não a duração. A duração real é 0.9 s (config
   type-6 do Jiggy).
3. **O antigo "raw field ≈ 2883" NÃO é `+0x20` nem `+0x30`.** No snapshot:
   `+0x20=2885`, `+0x30=1`. O `2883` é `+0x10` (`cfg_sfx_id`).
4. **A type-4 NÃO está incompleta: completou.** `current_state=2`,
   `render_frame_count=2` → por `gctransitionDll ep4`, o gate do warp está
   ABERTO (`state 2` + `render >= 2`). `rotation=-90`, `sfx 2883→2885`,
   `elapsed` zerado, `active_kind=0` são o estado pós-completion.
5. **O freeze NÃO é no progresso da type-4.** O attract foi pedido (struct
   `D_80127760`: `map=335`, `entrance=8`, `kind=2`), a type-4 completou, o
   `warp=3` foi consumido (`+0x12=0`), `func_800A9924(map,entr)` + 
   `func_800A76F4(5)` executaram (mode machine `+0x1C=5`, map/warp limpos,
   `diag_mode=2` intermediário), e a MAIN congelou durante a transição para o
   mode 5 — conforme o backtrace documentado `func_800A76F4 → func_800A7380 →
   func_800C2AB8` (actor loop). `func_800C2AB8` não é "incidental": é o PC de
   hang consistente com `d` congelado sem dispatch de overlay.
6. **mode/warp/map da fronteira documentada (mode=3, warp=3, map=335) não é o
   estado do snapshot limpo deste ambiente.** Aqui o snapshot capturou o estado
   DURANTE/DEPOIS do consumo do warp: mode=2, warp=0, map=0 (limpos por
   `func_800A7D84`/`func_800A76F4`). O attract struct retém `map=335`/`entr=8`.

### UNKNOWN

- Não capturamos o PC/backtrace da MAIN **neste** snapshot (contexto do
  `OSThread` não guarda PC guest; requer GDB host, o que perturba o timing).
  O estado da mode machine (`+0x1C=5` gravado, map/warp limpos) PROVA que a
  MAIN passou pelo `func_800A76F4`; o PC de hang no actor loop
  (`func_800C2AB8`) vem do backtrace documentado de sessão anterior.
- Por que `func_800C2AB8`/um callback de actor não retorna (loop infinito ou
  actor em estado inválido) durante a transição para o gameplay demo — causa
  raiz ainda aberta.
- Por que este ambiente roda a ~20 Hz (`delta=0.05`) em vez de 60/30 Hz —
  provável overhead de render, não investigado aqui.
- Se o freeze "documentado" (type-4 em state 6, elapsed 0.9) foi realmente
  observado em outro run ou é o mesmo estado com offsets trocados — não
  reproduzível neste ambiente (8/8 runs determinísticos no estado acima).

### CONCLUSÃO DO SNAPSHOT

- `VALUE_2883_AT_0x20: NO` (`0x20=2885`)
- `VALUE_2883_AT_0x30: NO` (`0x30=1`)
- `OLD_RAW_2883_ACTUAL_OFFSET: +0x10` (`cfg_sfx_id = 2883`)
- `TARGET=0.9` `ELAPSED=0.0` `REMAINING=0.9` `ELAPSED_GE_TARGET: NO` (elapsed é o
  reset pós-completion, não progresso travado)
- `CURRENT_WARP_GATE: OPEN` (state 2 + render 2)
- `SUBACTOR_PREVENTS_COMPLETION: NO` (sem subactor)
- `FRAME_TIME_DIVERGENCE: NO` (frame_counter=1, elapsed=0, consistente)
- `PRIMARY_CLASS: J` — a type-4 completou e liberou o warp; o freeze é
  downstream (transição de modo / actor loop). Secundário: `I` — a expectativa
  estática de type-4 incompleta é contradita pelo snapshot limpo.
- `FIRST_PROVEN_SEMANTIC_DIVERGENCE`: type-4 em `state 2`/`render 2` (gate
  aberto) com `elapsed=0`, `target=0.9` em `+0x0C` e `cfg_param3=3.5` em
  `+0x18` — os valores documentados "0.9/3.5/2883" são `+0x0C`/`+0x18`/`+0x10`.

Artefatos (não versionados): `/tmp/opencode/p08/supervisor.c`,
`/tmp/opencode/p08/decode_snapshot.py`, `/tmp/opencode/p08/snapshot.bin`,
`/tmp/opencode/p08/derived_run8.txt`, `/tmp/opencode/p08/game.log`.

```text
CODE_CHANGED             = NO
RUNTIME_CODE_CHANGED     = NO
GENERATOR_CODE_CHANGED   = NO
PATCH_CODE_CHANGED       = NO
CORE_INCLUDED            = NO
BUILD_ARTIFACT_INCLUDED  = NO
```
