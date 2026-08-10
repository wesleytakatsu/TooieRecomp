---
title: "P08 — diagnóstico de divergência no spawn de actors da intro"
description: "Identificação do primeiro ponto de divergência na cadeia de inicialização e spawn dos actors da abertura (chintroticker 419, chintrochar 440, chintrocam 441)."
status: evidence
phase: P08
updated: 2026-08-09
tags: [banjo-tooie-recomp, p08, intro, actors, gsmaker, chintroticker, chintrochar, chintrocam, freelist]
---

# P08 — diagnóstico de divergência no spawn de actors da intro

## 1. Resumo Executivo

Durante a investigação da sequência de abertura de Banjo-Tooie no `TooieRecomp`, constatou-se que o motor executa o boot (`func_800A7BB8`), inicializa a máquina de attract em `stage=2` (`gsattract:4`), entra no modo normal de mundo `mode=3` via `func_800A759C(3, 1)`, e requisita o mapa de cutscene `MAP_18B_CS_STARTUP_INTRO` (`0x18B`).

O setup do mapa (`asset 0xAE0 = 0x18B + 0x955`) é requisitado via `gsmaker:2(0x18B)` -> `gsmaker:1(0xAE0)` -> `func_800D674C(0xAE0)`, e descompactado com sucesso via `gldbDll` (overlay 708). No entanto, **os actors responsáveis pela cena de abertura (`chintroticker` 419, `chintrochar` 440, `chintrocam` 441 e `chattract` 208) nunca são instanciados ou executados**.

---

## 2. Cadeia de Execução Mapeada

A cadeia original de inicialização de Banjo-Tooie mapeada a partir da descompilação (`src/core2/1E80A70.c`, `src/core2/1EC3810.c`, `src/core2/1EAF950.c`, `src/core2/1EB52C0.c`) segue os seguintes estágios:

```
1. Boot & Transição de Modo:
   func_800A7BB8
     ├── _gsattract_entrypoint_4 (stage=2, D_80135470)
     └── func_800A759C(3, 1) -> D_80127630.unk2 = 3 (Mode 3: World Active)
           └── func_800EA148(2) -> _gsworldDll_entrypoint_3(&D_80132DC0, 2) -> ativa D_80132DCA

2. Carga do Mapa e Setup:
   gcsectionDll (677:4) com a0=0x18B (MAP_18B_CS_STARTUP_INTRO)
     └── gsmaker:2 (0x18B)
           └── gsmaker:1 (0xAE0)
                 ├── func_800D674C(0xAE0)  [parse do setup asset via gldb 708:0]
                 │     └── preenche D_8012B800 e aloca freelist D_8012C7E0 (func_800DBA84/func_800DBBF8)
                 ├── func_800D70D0(1)
                 └── func_800D71F4(1)

3. Ciclo Principal do Mundo (Tick):
   func_800A7D84 (Mode 3 loop)
     └── func_800EA170
           ├── verifica D_80132DCA != 0
           └── func_800D6E54(1) [avaliador e dispatcher de itens do setup em D_8012B800]
                 ├── verifica D_8012BE62 (contagem de itens)
                 ├── checagem de heap: func_800A9F30 -> func_8001BDAC
                 └── loop D_8012B800 -> func_800D6378 / func_800D6408 -> spawn do actor / freelist
```

---

## 3. Primeiro Ponto Conhecido de Divergência

O ponto de divergência identificado está na **avaliação e ativação dos registros do setup em `func_800D6E54` e na resolução dos trampolines de syscall dos actors**:

1. **Carga do Setup Concluída**: O parser de setup `func_800D674C(0xAE0)` é executado com sucesso e insere as entradas na tabela global `D_8012B800`.
2. **Avaliação no Tick**: No loop de `func_800D6E54`, os itens do setup deveriam disparar a instanciação dos actors associados chamando suas rotinas de inicialização (`entrypoint_0` das DLLs 419, 440, 441 via trampolines de syscall em `0x80086B40`, etc.).
3. **Ausência de Despacho de Syscall**: No runtime do `TooieRecomp`, `recomp_syscall_handler` registra dezenas de milhares de despachos de DLLs de sistema (`gcsectionDll` 677, `gsworld` 726, `gsmaker` 2, `gldb` 708, `cadb*` 167-169), mas **zero chamadas** aos índices `419`, `440` ou `441`.
4. **Consequência Direta**: Sem a criação do actor `chintroticker` (419), o timer `D_80135474` não avança e a função `_gsattract_entrypoint_5` nunca é chamada. Como resultado, a câmera da cutscene (`chintrocam` 441) não se move, os modelos e logos (`chintrochar` 440, `chattract` 208) não aparecem, e o jogo permanece no estado estático do mapa 0x18B.

---

## 4. Evidências de Validação

- **Rastreamento GDB**: Breakpoints em `func_800D674C`, `func_800D71F4` e `_gsmaker_entrypoint_1` comprovaram que `func_800D674C(0xAE0)` é atingida durante a chamada `gsmaker:2(0x18B)`.
- **Rastreamento Syscall**: O log `[tooie-opening]` confirmou o estado estável em `mode=3`, `map=0x18B`, `stage=2`, `t=00000000` durante toda a execução.
- **Isolamento de Causa**: O problema não está no renderizador RT64 nem na inicialização matemática básica do core1, mas estritamente no mecanismo de dispatch/spawn dos actors pelo setup manager (`func_800D6E54` / `D_8012B800`).
