---
title: "P08 — title/save input (FAILED attempt + handoff)"
status: blocked
phase: P08
updated: 2026-08-09
owner: next-agent
tags: [tooie, p08, input, eeprom, handoff]
---

# P08 — input title/file select — **NÃO RESOLVIDO**

**Status:** `blocked` / falhou validação do utilizador (2026-08-09).  
**Ação:** **outro agente assume** este ajuste. Não reabrir como “done”.

## Sintoma (confirmado)

- Inputs do **launcher** (RecompFrontend UI) funcionam.
- Na **tela inicial do jogo** (title / file select / entrar no save) o pad
  **não** avança o fluxo de start/save.
- Utilizador testou após o fix EEPROM abaixo: **ainda não funciona**.

## Tentativa desta sessão (insuficiente)

Hipótese: thread JOY (`func_800166DC` em
`banjo-tooie-Recomp/src/core1/1E2D890.c`) faz:

```c
s32 eeprom_type = osEepromProbe(&D_80079B60);
if (eeprom_type < EEPROM_TYPE_16K) {
    D_80079B90[0].errno = 8;  // CONT_NO_RESPONSE
}
// depois só lê pad se errno == 0
osContGetReadData(...);
```

`LOOKUP_FUNC(0x80030170)` sem hook → probe 0 → errno=8 → nunca lê pad.

**Código aplicado e mantido** em `TooieRecomp/src/game/boot_hw_stubs.cpp`:

| VRAM | hook |
| --- | --- |
| `0x80030170` | `osEepromProbe_recomp` |
| `0x8002DE70` | `osEepromLongRead_recomp` |
| `0x8002DF00` | `osEepromLongWrite_recomp` |
| `0x80032B10` | `osEepromRead_recomp` |
| `0x80032D60` | `osEepromWrite_recomp` |

Build linkou; smoke boot OK (`registered 22 ultra LOOKUP hooks`).  
**Validação humana do title/save: FAIL.**

A revisão confirmou os cinco VRAMs contra `tooie.us.syms.toml` e confirmou
`SaveType::Eep16k` no `GameEntry`; assim, `osEepromProbe_recomp` retorna `2`.
Os hooks são substituições libultra corretas e necessárias para save, embora não
sejam suficientes para resolver o input do title/file select.

GDB confirmou o registro em execução na thread correta:

```text
Thread "[Game] JOY" hit osEepromProbe_recomp
#1 func_800166DC
#2 run_thread_function
```

## O que o próximo agente deve assumir

1. **Não assumir** que EEPROM sozinho basta — causa real ainda aberta.
2. Diagnosticar com evidência runtime:
   - GDB: breakpoints em `osEepromProbe_recomp`, `osContInit_recomp`,
     `osContStartReadData_recomp`, `osContGetReadData_recomp`, `func_800165F0`,
     `func_800166DC`, `func_80016810`.
   - Dump `D_80079B90[0].errno`, `D_80079B40` pads, `D_80079BA0`, bitpattern
     `D_80079D68` **na tela de title/file select**.
   - Confirmar se probe retorna `2` (16K) e se `GetReadData` é chamado com
     botões ≠ 0 quando o utilizador pressiona teclas/gamepad.
3. Hipóteses alternativas a investigar:
   - Input SDL/RecompFrontend **só** enquanto UI launcher tem foco/context;
     após `hide_all_contexts` o poll/bindings não alimentam `get_n64_input`.
   - `osContGetReadData_recomp` só escreve pad se `err_no == 0` no dummy; falha
     de `get_input` → pads vazios.
   - Caminho de UI do title/save não usa `D_80079B40` / JOY thread (outro
     overlay / demo input / flag).
   - SI event queue / JOY stop-start (`func_80016810`) perde mensagens SI.
   - Defaults de teclado Tooie ≠ o que o utilizador pressiona; gamepad não
     mapeado; single-player device info ok mas profile bindings vazios.
   - EEPROM hooks corretos mas save/file select bloqueado por outro gate
     (PFS, flags de progressão, overlay de frontend do jogo).
4. Comparar com BanjoRecomp: como BK regista eeprom/cont e se title input
   funciona no mesmo frontend.
5. Atualizar esta nota + `plans/BT/08-…` residual quando houver fix **validado
   pelo utilizador** no title/file select.

## Como reproduzir

```sh
cd /home/wesley/projetos/N64-Recomp
./run-tooie.sh -a
# Esperar title / file select do jogo (não o launcher RmlUi)
# Pressionar A/Start/stick — utilizador reporta: não entra no save/jogo
```

## Relacionado

- Path cont host (parcial): [[p08-controller-host-path]]
- Cena 3D pós-transição: [[p08-first-area-playable]]
- Plano: [[../../plans/BT/08-rt64-input-and-frontend]]
- Handoff repo: `TooieRecomp/docs/HANDOFF.md`
