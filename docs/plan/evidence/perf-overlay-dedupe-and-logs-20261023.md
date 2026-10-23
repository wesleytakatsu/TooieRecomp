---
title: "P11 — Desempenho: correção de travamentos por sobrecarga de overlays e eliminação de logs em runtime"
description: "Causa raiz das travas no carregamento de fases e Demo 5 comprovada no crescimento descontrolado do vetor de seções de overlay no N64ModernRuntime (custo quadrático O(n) na descarga); deduplicação via patch local reduz travas de 110,8 s para 2,8 s no attract; auditoria estática elimina logs síncronos e getenv em loops críticos (FPS mediano 56 -> 72)."
status: current
type: evidence
project: TooieRecomp
phase: P11
updated: 2026-10-23
tags: [p11, performance, overlays, dedupe, n64modernruntime, logging, fps, benchmarks]
---

# Evidência P11: Correção de travamentos por sobrecarga de overlays e eliminação de logs em runtime (2026-10-23)

**Data da medição e diagnóstico:** 2026-10-23
**Ambiente:** Host nativo Windows 11 x64, árvore local de trabalho (código ainda não commitado), patch local `vendor/patches-local/n64modernruntime-overlay-section-dedupe.patch`.

---

## 1. Contexto e Relato do Usuário (`USER_OBSERVED`)

Durante sessões de teste e transições de fase no executável nativo Windows, o usuário relatou que o jogo apresentava sensação de lentidão e peso geral, com destaque para um travamento prolongado e severo ("freeze") ao carregar a Demo 5 (fase de gelo, mapa 296):
- "Jogo pesado; carregar a Demo 5 demora e trava."

---

## 2. Instrumentação e Medição Inicial da Linha de Base

Para isolar e quantificar os travamentos de forma reprodutível sem perturbar a execução do emulador, foram introduzidos:
1. **Instrumentação de telemetria de apresentação (`TOOIE_DIAG_PRESENT_LOG=1`):**
   - Implementado em `src/game/fps_overlay.cpp` (default OFF).
   - Emite no log uma amostragem a cada 250 ms contendo o carimbo de tempo absoluto e a leitura do contador acumulado de apresentações bem-sucedidas do RT64 (`RT64::presentSuccessCount`).
2. **Registro de log com carimbos de tempo de alta precisão.**

### Linha de Base (Attract Demo, 7 minutos de execução contínua)
- **Travamentos graves ($\ge 0,75$ s sem emissão de frames):** 7 ocorrências.
- **Tempo acumulado de congelamento:** **110,8 segundos**.
- **Pior travamento individual:** **91,0 segundos** ininterruptos de congelamento (ocorrido entre os segundos 223 e 314 da execução, janela que concentrou cerca de 250 descargas e 300 carregamentos de overlays).
- **Taxa de quadros mediana:** **56 FPS**.

---

## 3. Causa Raiz Comprovada (`PROVEN` no código e validação métrica)

A investigação identificou uma explosão de complexidade algorítmica no gerenciamento de overlays do runtime:

1. **Invocação redundante de mapeamento:**
   - Em `src/game/syscall_handler.cpp`, a função `activate_canonical_mapping` invoca incondicionalmente `load_overlays` a cada despacho de syscall originada de um overlay.
2. **Crescimento descontrolado do vetor (`loaded_sections`):**
   - Na biblioteca `lib/N64ModernRuntime/librecomp/src/overlays.cpp`, a rotina `load_overlay` realizava inserção incondicional via `loaded_sections.emplace_back(...)` sem checar se a seção já constava no vetor.
   - O vetor `loaded_sections` acumulava indefinidamente centenas e milhares de cópias duplicadas das mesmas seções a cada syscall despachada.
3. **Custo quadrático de descarregamento ($O(n^2)$):**
   - Ao trocar de fase ou mapa, a função `unload_overlays` percorre as seções e remove cada entrada do meio do vetor sequencial contíguo via `std::vector::erase`.
   - Cada remoção exige o deslocamento linear $O(n)$ dos elementos subsequentes. Com milhares de seções duplicadas acumuladas ao longo dos minutos de gameplay, o custo total de descarga crescia quadraticamente ($O(n^2)$), paralisando o thread principal do jogo por até 91 segundos consecutivos.

---

## 4. Correção no Runtime: Deduplicação de Seções de Overlay

A falha algorítmica foi corrigida no owning layer do runtime:
- **Patch local:** `vendor/patches-local/n64modernruntime-overlay-section-dedupe.patch`.
- **Lógica de inserção:** `load_overlay` verifica se o par `(ram, secao)` já está presente no vetor antes de efetuar o registro; a inserção só ocorre caso o par seja inédito.
- **Validação de cadeia de patches:** `tools/vendor_verify.sh --reverse-check` confirmou `VENDOR VERIFY OK` com os 29 patches locais aplicados.

### Medição Pós-Deduplicação (Attract Demo, 7 minutos)
- **Travamentos graves ($\ge 0,75$ s):** reduzidos de 7 para **2**.
- **Tempo total de congelamento:** reduzido de 110,8 s para **2,8 segundos** (−97,5%).
- **Pior travamento:** reduzido de 91,0 s para **2,0 segundos** (restrito à inicialização e carregamento inicial do aplicativo).
- **Taxa de quadros mediana:** elevada de 56 FPS para **68 FPS** (+21%).

---

## 5. Auditoria Estática e Eliminação de Overhead de Logs Síncronos

Uma auditoria estática detalhada na base de código do jogo (executada por Claude Sonnet 5.5 via Cursor Agent CLI) revelou que diversos módulos de diagnóstico e rotinas de I/O bloqueante permaneciam ativas de forma incondicional em builds de produção:

1. **Módulo de diagnóstico "USER_DEMO_5 MAP=296":**
   - Armava um latch automático e irreversível ao detectar o mapa 296, disparando chamadas bloqueantes `fprintf` seguidas de `fflush` a cada chamada de atualização de matriz de câmera.
   - As janelas de diagnóstico `ud5` operavam sem verificação prévia de variáveis de ambiente.
2. **Logs síncronos de overlay:**
   - Emissão de `fprintf` + `fflush` síncrono para cada overlay carregado e descarregado.
3. **Consultas repetitivas a variáveis de ambiente (`getenv`):**
   - Chamadas diretas a `getenv` sem cache local em `transform_hooks.cpp` no caminho crítico executado por frame.
4. **Contenção atômica em inicialização de diagnóstico:**
   - Operação atômica Compare-And-Swap (CAS) redundante executada a cada invocação de `tooie_diag::ensure_init`.

### Correções Implementadas (Grok 4.7)
As correções foram estritamente circunscritas à infraestrutura de logs e diagnósticos, sem qualquer alteração na lógica ou estado de simulação do jogo:
- Encapsulamento dos logs da Demo 5 sob a flag `TOOIE_DIAG_UD5=1` (default OFF);
- Encapsulamento dos logs de overlay sob `TOOIE_DIAG_OVERLAY_LOG=1` (default OFF);
- Armazenamento em cache de leitura de variáveis de ambiente em `transform_hooks.cpp`;
- Fast path com checagem booleana relaxada em `tooie_diag::ensure_init` evitando CAS em chamadas repetidas.

---

## 6. Resultados Finais Consolidados (Attract Demo, 7 minutos)

| Métrica | Base Inicial | Pós-Deduplicação de Overlays | Pós-Saneamento de Logs e I/O | Variação Global |
| :--- | :---: | :---: | :---: | :---: |
| **Travamentos ($\ge 0,75$ s)** | 7 | 2 | **2** | −71% |
| **Tempo total travado** | 110,8 s | 2,8 s | **2,5 s** | −97,7% |
| **Pior travamento individual** | 91,0 s | 2,0 s | **1,8 s** (abertura; 0,8 s em troca) | −98,0% |
| **Taxa de quadros mediana** | 56 FPS | 68 FPS | **72 FPS** | +28,6% |
| **Linhas totais de log emitidas** | 83.264 | — | **533** | −99,4% |
| **Linhas de log `ud5-`** | 52.249 | — | **0** | −100% |

---

## 7. Verificação de Regressão e Testes

A suíte completa de testes unitários do host foi executada e confirmou conformidade integral:
- **Testes obrigatórios:** 9/9 PASS.
- **Testes extras:** 17/17 PASS.

---

## 8. Hipóteses e Otimizações Futuras Identificadas (`HYPOTHESIS`)

A auditoria estática levantou duas oportunidades de otimização adicional para fases futuras:
1. **Matching do RT64 sob Interpolation OFF (`HYPOTHESIS`):**
   - Com a interpolação desativada na interface (`Interpolation` OFF), o RT64 pode continuar executando rotinas parciais de matching de frames em `GameFrame::matchScene`. Curto-circuitar essa rotina inteiramente quando a interpolação estiver inativa trará alívio adicional de CPU.
2. **Wrappers legados de investigação (`TOOIE_UD4_WRAP`):**
   - Aproximadamente 70 wrappers residuais de investigações preliminares de usuário continuam registrados no código. Seu impacto atual é desprezível, mas sua remoção futura simplificará o fluxo de símbolos interceptados.

---

## 9. Observação Crítica de Processo de Compilação

> [!IMPORTANT]
> O script `tools\windows\Build-Tooie.ps1 -Stage Test` executa os alvos de teste unitário do host, mas **não recompila nem atualiza** o executável principal do jogo (`build-win\TooieRecompiled.exe`).
> Para medições de desempenho, execução de benchmarks e validação visual de gameplay, é obrigatório disparar previamente a compilação do executável através do Ninja no diretório `build-win`.

---

## 10. Status da Alteração

- As correções de desempenho estão testadas e operacionais na árvore local de desenvolvimento, aguardando validação direta do usuário em gameplay real (especialmente no mapa 296 / Demo 5) para autorização formal de commit.
