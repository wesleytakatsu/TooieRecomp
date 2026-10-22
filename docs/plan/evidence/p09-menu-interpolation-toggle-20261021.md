---
title: "P09 — Opção Interpolation no menu ESC e remoção do atalho F8"
description: "Opção persistida Interpolation ON/OFF nos gráficos do menu ESC, aplicada em runtime ao RT64 sem restart; atalho F8 de captura removido. Prova host + jogo isolado. Não afirma correção de glitch nem gate técnico aprovado."
status: current
type: evidence
project: TooieRecomp
phase: P09
updated: 2026-10-21
tags: [p09, interpolation, menu, graphics, config, rt64, patches, f8-removed]
---
# Evidência P09: opção Interpolation (ESC → Graphics) e F8 removido

**Autorização:** pedido explícito do usuário (2026-10-21) substitui a antiga política congelada “presentation travada em 30”. Escopo: toggle persistido + remoção do F8. **Não** é correção de glitch: o último gate técnico documental continua `passed: false` (ver [[../../HANDOFF]]); ligar Interpolation expõe os glitches articulados experimentais ainda abertos (`HYPOTHESIS`/gate aberto). Nenhum guard experimental, geometria ou hipótese não relacionada foi tocado.

## Comportamento implementado (`PROVEN` em host + jogo isolado)

- Opção gráfica `interpolation` (bool, `graphics.json`). Ausente/inválida/legada ⇒ **OFF**. OFF = Manual 30 travado + endpoint CURRENT (caminho estável anterior). ON = controles de Framerate liberados; Original ou Manual ≤ 30 viram Manual **60**; um Manual 120 salvo sobrevive ao boot; Display é mantido.
- Aplicação em runtime: o valor **aplicado** (Apply do menu) vira `RT64::requestHostEndpointCurrent(!enabled)`; o render thread faz `latchHostEndpointCurrent()` uma vez por workload de jogo (frame 0 do laço de display), então não há restart, `setenv` nem mistura no mesmo workload. Toggle pendente (antes do Apply) só muda a UI; Cancel restaura o estado aplicado.
- Camadas que forçavam CURRENT/30 e o que foi feito: (1) `install_forced_refresh` (30) → `install_interpolation_policy` (`include/tooie_menu_policy.h`); (2) `setenv("TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT","1")` em `main.cpp` → removido (módulo `tooie_rsp_endpoint_current`, teste e alvo CMake removidos; cópias em `/home/wesley/.cache/tooie-menu-interpolation-toggle/removed-sources/`); (3) caches estáticos `getenv` em RT64 (`RSPProcessor::process`/`processCB.forceEndpointCurrent`, `p09WorldGuardEndpointCurrentActive`, `interpBurst::forceEndpointEnv`) → `RT64::endpointCurrentActive()`; (4) shader constant já era reavaliada por chamada; (5) `TOOIE_DIAG_FORCE_CURRENT_FRAME` é diagnóstico default-OFF, intocado e não definido pelo Tooie.
- Precedência: env explícito `TOOIE_DIAG_RSP_FORCE_ENDPOINT_CURRENT=0|1` vence a opção (override legado, lido uma vez); `TOOIE_TEST_INTERPOLATION_FPS=60|120` força ON naquela taxa e imprime um aviso quando contradiz a opção salva. Documentado nas specs.
- Telemetria (uma linha por mudança, nunca por frame): `[tooie-interp] workload=N endpointCurrent=E generate=G targetRate=T viRate=V displayFrames=D` e `first interpolated render since last state change: ... curW=`. `endpointCurrent=1` com `curW<1` é interpolação **mascarada**; interpolação real é `endpointCurrent=0`. No OFF, trechos em que o VI do jogo é 20 Hz e o alvo é 30 geram frames (`generate=1`) mascarados por CURRENT — comportamento estável preexistente, preservado.
- **F8 removido**: sem `SDL_AddEventWatch`/`register_interp_burst`, `src/game/tooie_interp_burst.cpp` removido do CMake e do disco (cópia no cache); `include/tooie_interp_burst.h` mantém só `pictures_dir_from` (usado por `p09_interp_burst_test`). Infra RT64 de captura e seus testes ficam dormentes. Sem tecla substituta.

## Fontes tocadas

Tooie: `include/banjo_config.h`, `include/tooie_menu_policy.h`, `include/tooie_interp_burst.h`, `src/game/config.cpp`, `src/main/main.cpp`, `CMakeLists.txt`, `tools/apply_runtime_patches.sh`, `patches/rt64/rt64-p09-host-endpoint-current-toggle.patch` (novo), `tests/tooie_menu_config_test.cpp`, `tests/tooie_menu_policy_test.cpp`, `tests/rt64_host_endpoint_current_test.cpp` (novo); removidos `include/tooie_rsp_endpoint_current.h`, `src/game/tooie_rsp_endpoint_current.cpp`, `tests/tooie_rsp_endpoint_current_test.cpp`, `src/game/tooie_interp_burst.cpp`. RT64 (árvore suja preservada): novo `src/hle/rt64_host_endpoint_current.h`; edições mínimas em `rt64_workload_queue.cpp`, `rt64_rsp_processor.cpp`, `rt64_p09_world_interpolation_guard.h`, `rt64_interp_burst.h`. Nada em `RecompiledFuncs/**`, N64ModernRuntime, RecompFrontend ou plume (diffs idênticos ao baseline). Patch: SHA256 `b96e2b703ae0d4782b2debf1178584355590865a7a17d7d74a128745cd5d8f15`, empilhado após guard/burst, `git apply --check` OK na cadeia limpa `rt64-clean-pin` (6f1c2d99) e `--reverse --check` OK na árvore suja; replay completo com compilação na cadeia limpa **não** foi feito. `tools/apply_runtime_patches.sh` já falha **antes** do novo patch na árvore suja (`rsp endpoint current` “does not apply”), condição preexistente (replay valido só em clone limpo).

## Comandos e resultados (HEAD Tooie `73bfd430`, árvore suja preservada)

- Build serial: `cmake --build build --target TooieRecompiled <testes> -j2` → exit 0 (`/home/wesley/.cache/tooie-menu-interpolation-toggle/build-final.log`); binário `build/TooieRecompiled` SHA256 `3589d7346a32d161022228501e90aec14118fe332a697d00c0daf347f0518ab2`.
- Testes (exit 0): `tooie_menu_policy_test`, `tooie_menu_config_test` (OFF legado/sem arquivo/inválidos, ON persistido 120/30/Original/Display, ON→OFF→ON via Apply, pendente vs aplicado, cancel, override env, JSON salvo), `rt64_host_endpoint_current_test` (env unset/0/1, latch), `p09_interp_burst_test`, `p09_world_interpolation_guard_test`, `p09_local_pair_snapshot_test`, `p09_exact_use_correlation_test`, `rt64_present_decimator_test`, `p09_local_pair_snapshot_io_test --write-fixture` + `--replay-file` (0/0). Lista: `test-exits.txt` no cache.
- Jogo isolado (cópia do binário, `APP_FOLDER_PATH` próprio, clone do `graphics.json` legado `rr=Manual/30`, ROM por symlink, saves próprios vazios; instâncias terminadas com SIGTERM→SIGKILL só no PID próprio, o processo alheio `2005748` não foi tocado):
  - boot legado: `endpointCurrent=1 targetRate=30`, overlay “30 FPS”, Interpolation OFF, Framerate “Locked to 30 FPS”.
  - ESC→Graphics→Interpolation (último item; scroll) ON pendente: slider mostra 60, controles liberados, runtime inalterado; Apply: `workload=735 endpointCurrent=0 generate=1 targetRate=60 displayFrames=3` + `first interpolated render ... curW=0.333 endpointCurrent=0`, overlay “60 FPS”, `graphics.json` `interpolation:true, rr_manual_value:60`.
  - OFF + Apply: `endpointCurrent=1 targetRate=30`, JSON `false/30`; ON + Apply de novo: `endpointCurrent=0 targetRate=60 displayFrames=2`, `curW=0.500` (ciclo ON→OFF→ON no mesmo processo, sem restart).
  - relançamento com ON persistido: `workload=1 endpointCurrent=0 targetRate=60`; com OFF persistido: `workload=1 endpointCurrent=1 targetRate=30`.
  - F8 (XTest, 2×, menu fechado, ON): 0 arquivos em Pictures isolado, 0 diretórios novos em `~/Imagens/TooieRecomp`, 0 linhas “burst” no log; `nm` sem `interp_burst_event_watch`/`register_interp_burst`.
  - Screenshots/logs: `/home/wesley/.cache/tooie-menu-interpolation-toggle/shots/` e `logs/` (`final1-*`, `final2-*`, e runs anteriores `run1..run4` com binário pré-ajuste de redraw do slider).

## Limites e riscos (`UNKNOWN`/não provado)

- Prova de jogo foi na cena de título/attract com guest em execução, não jogando um save; nenhuma medição de glitch, FPS sustentado fora do contador de present, áudio ou timing do guest. Nenhum teste `USER_OBSERVED`.
- ON ativa interpolação real com os guards experimentais agora não inertes; os glitches articulados e os blockers B1–B7 do gate técnico seguem abertos. Não promovido.
- O toggle é a última linha da lista Graphics (requer scroll); não há reordenação via API.
- SIGTERM do processo isolado não encerrou em 30 s em várias runs (teardown race já registrado no HANDOFF); o SIGKILL foi aplicado apenas ao PID próprio.
- Compilação sobre a cadeia limpa do RT64 não foi repetida; só `git apply --check`.
- Commit/push: não realizados.
