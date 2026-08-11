---
title: "P08 — diagnóstico de divergência no spawn de actors da intro"
description: "Identificação do primeiro ponto de divergência na cadeia de inicialização e spawn dos actors da abertura (chintroticker 419, chintrochar 440, chintrocam 441)."
status: evidence
phase: P08
updated: 2026-08-11
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

## 3.5 False lead — func_800D6E54

> Revisão semântica (Grok, P08) descartou `func_800D6E54` como actor population pass:

- `func_800D6E54(a0)` é um **processador de fila de setup/resource/reclaim deferido** (reclam de heap), não um enumerador de actors.
  - `a0=1` → pass leve/throttled (caller: `func_800EA170` no tick do gsworld).
  - `a0=2` → pass forçado/completo sob condições de heap (callers: `func_800A9D90` via `heap_alloc_sided`, `func_800C0FD8` via `func_800C0984`).
- O loop `L_800D6F30` processa records de resource/refcount/recycle (`func_800D6378` → `func_800D56C4`/`func_800DBB9C`/`func_800D61B4`).
- **NÃO há edge causal provado** do loop para `func_80108C90`, `func_80107C2C` ou `func_80106668`.
- `0 actors` + `0 loop hits` era **correlação, não causalidade**.
- Revisão Grok: DeepSeek CLASS B sobre D6E54 **REJECTED**; interpretação correta → C.

---

## 3.6 Status 2026-08-11 — root gate real (IPL3 integrity)

O caminho de population natural **não** passa por `func_800D6E54` como enumerator
de actors (false lead §3.5). Diagnóstico fechado do gate que bloqueia o pipeline:

**`func_8001DE64` → `D_8007DB79=0x14` → `func_800A5C28` skip `func_800BDCDC`**
→ zero `gccubeDll:5` / `gccollectDll:8` / `func_80108C90` / actors.

Detalhe completo (matrix, address +0x10000, missing guest words, fix A não
implementado): [[p08-natural-actor-spawn-divergence]].

§4 abaixo permanece como evidência **histórica** do setup path (gsmaker/AE0);
o item "Isolamento de Causa" que aponta D6E54 como mecanismo de spawn está
**superseded** por §3.5 + §3.6.

---

## 4. Evidências de Validação

- **Rastreamento GDB**: Breakpoints em `func_800D674C`, `func_800D71F4` e `_gsmaker_entrypoint_1` comprovaram que `func_800D674C(0xAE0)` é atingida durante a chamada `gsmaker:2(0x18B)`.
- **Rastreamento Syscall**: O log `[tooie-opening]` confirmou o estado estável em `mode=3`, `map=0x18B`, `stage=2`, `t=00000000` durante toda a execução.
- **Isolamento de Causa (histórico; superseded §3.5/§3.6)**: Na época desta nota, a hipótese apontava o setup manager (`func_800D6E54` / `D_8012B800`). Revisão posterior: D6E54 = false lead; root gate = integrity/IPL3 `func_8001DE64` — ver [[p08-natural-actor-spawn-divergence]]. RT64/math core1 continuam fora da causa.
