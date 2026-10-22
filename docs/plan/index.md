---
title: "Plano mestre — Banjo-Tooie nativo no Linux"
description: "Plano executável para transformar os insumos BanjoRecomp e banjo-tooie-Recomp em um port nativo completo de Banjo-Tooie para Linux."
status: active
type: plan
project: TooieRecomp
platform: linux-x86_64
updated: 2026-08-15
source_path: banjo-tooie-Recomp
source_repo: wesleytakatsu/banjo-tooie-Recomp
upstream_repo: Mr-Wiseguy/banjo-tooie
tags:
  - banjo-tooie-Recomp
  - static-recompilation
  - linux
  - plan
aliases:
  - "Plano TooieRecomp"
  - "Banjo-Tooie Linux"
---

# Plano mestre — Banjo-Tooie nativo no Linux

Este é o ponto de entrada do trabalho. As especificações descrevem o que já
existe; as etapas em [[../plans/BT/index|Planos de implementação BT]] descrevem
o que deve ser produzido e como provar que está pronto.

## Objetivo

Entregar um executável Linux x86-64 de Banjo-Tooie gerado por recompilação
estática, usando a ROM legítima do usuário apenas como fonte de dados. O port
deve preservar o comportamento do N64, integrar `N64ModernRuntime` e `RT64`, e
funcionar sem emulador.

## Decisão de arquitetura

```text
Banjo-Tooie NTSC-U Rev. 0
            |
            v
banjo-tooie-Recomp: decompress + Splat + ELF + símbolos + overlays + relocations
            |
            v
N64Recomp + RSPRecomp
            |
            v
C/C++ gerado + patches nativos específicos de Tooie
            |
            v
N64ModernRuntime + RecompFrontend + RT64
            |
            v
TooieRecompiled (Linux x86-64)
```

Não é uma troca de ROM dentro do BanjoRecomp. O BanjoRecomp é a implementação
de referência para runtime, frontend, renderização, entrada, patches e
empacotamento; cada dado específico de Banjo-Kazooie precisa ser substituído.
Veja [[../specs/BK/reuse-contract|Contrato de reutilização do BanjoRecomp]] e
[[../specs/BT/recomp-readiness|Prontidão do banjo-tooie-Recomp para recompilação]].

## Estado inicial verificado

| Item | Estado | Evidência |
| --- | --- | --- |
| ROM NTSC-U Rev. 0 em `.z64` | pronto | `Banjo-Tooie (USA).z64` (33554432 bytes, MD5 `40e98faa24ac3ebe1d25cb5e5ddf49e4`, SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`, ID `NB7E`); launcher exige imagem descompactada gerada por `tools/extract_rom.sh` (35098224 bytes, SHA-1 `af479771ff5e91ce3732f3d84146614ab71d8d79`) |
| Fork `banjo-tooie-Recomp` | presente | `origin` = `wesleytakatsu/banjo-tooie-Recomp`, `upstream` = `Mr-Wiseguy/banjo-tooie` |
| Baseline BT | verificada | `master`/`origin/master`, commit `25033be6ec9e35975483f064d086391d2381ca4c`, 0 ahead/0 behind do `upstream/master`, worktree limpo |
| Insumos BT | presentes no fork | decompressor, Splat, definições de ELF, símbolos, relocations, overlays e syscalls; artefatos gerados ainda dependem de setup/build |
| Fonte `BanjoRecomp` | presente | runtime, RT64, RSP, frontend, patches e build Linux de referência |
| Toolchain host básica | **pronta (P01)** | MIPS cross, fmt/toml11/SDL2/lld, venv Python, tools BT, N64Recomp/RSPRecomp |
| Submódulos BT + BK | **aposentados / vendorizados** | substituídos por árvores regulares em `lib/`, `decomp/`, `assets/`, `tools/wheelhouse/` (0 submódulos no git) |
| Projeto `TooieRecomp` | **scaffold P03** | `TooieRecomp/` com configs, gerador e metadata; executável ainda em P05 |
| Baseline P00 | **done** | [[baseline-manifest|manifesto]] + `scripts/validate-baseline.sh` PASS |
| Toolchain P01 | **done** | [[../plans/BT/01-linux-toolchain|etapa]] + `docs/plan/evidence/p01-*` |
| ROM/ELF P02 | **done** | matching decomposto OK; ELF `build/us/banjotooie_decompressed.elf` |
| Metadados P03 | **done** | `TooieRecomp/metadata/` + dump N64Recomp; projeto `TooieRecomp/` criado |
| Recomp P04 | **done** | `RecompiledFuncs/` (~21k funcs) + `libRecompiledFuncs.a` Clang |
| Runtime P05 | **done** | `TooieRecompiled` linka RT64; launcher/Vulkan OK; frame de jogo → P06 |
| Overlays P06 | **done** | overlays/syscalls funcionais; lifecycle pós-P06 corrigido, mas gameplay ainda não alcançado |
| RSP/áudio P07 | **active** | intro áudio OK; soak/SFX residual |
| RT64/input P08 | **active** | RT64 presentation, input mapping, windowing (T07), graphics error fallback (T08), launcher preview, and static calls integrated; USER_DEMO_4 `0x800979D8 → BCE84 → P60 → ncbapivot` helper chain proven in bounded paired scope; USER_DEMO_1/5 movement residuals remain open; `ANGRYLION_CONTROL` in `tools/oracle/angrylion-control/` ([[../specs/BT/angrylion-control-oracle]]); manual validation pending |
| Interpolação P09 | **pending** | investigação causal ativa; opção Interpolation no menu ESC integrada e persistida; gate técnico Haiku 5.5 reprovado (FAILED); padrão estrito CURRENT/30 |
| Empacotamento P10 | **pending** | **standalone**: dependências vendorizadas em `lib/`/`decomp/`/`assets/` (proveniência em `vendor/`; pipeline kit-free provado com 420 arquivos idênticos, 1159 passos e 8 testes PASS; UI restaurada concluída com binário `0cfcd792…` exit 0; pacote Linux final `4f702d2b…` com 104 arquivos verificado exit 0); Windows 11 x64 classificado `WINDOWS_BUILD_PENDING_NATIVE_HOST` |

O nome `Mr-Wiseguy/banjo-tooie` identifica o upstream histórico de decompilação.
No TooieRecomp standalone, essas fontes residem vendorizadas diretamente em `decomp/`.

## Definição de “rodar perfeitamente”

O trabalho só está concluído quando todos os critérios de
[[definition-of-done|Definição de pronto]] estiverem satisfeitos. Em resumo:

- o jogo completa o boot e uma campanha inteira sem crash ou softlock;
- gráficos, áudio, entrada, saves, overlays e transições preservam o original;
- o timing original é a referência e modos de FPS alto não alteram gameplay;
- o build é reproduzível em Linux limpo e gera pacote redistribuível sem ROM;
- testes, hashes, logs e versões ficam registrados como evidência.

## Caminho crítico

| Ordem | Etapa | Gate de saída | Estado |
| ---: | --- | --- | --- |
| 00 | [[../plans/BT/00-baseline-and-decisions|Baseline e decisões]] | ambiente, hashes e versões registrados | done |
| 01 | [[../plans/BT/01-linux-toolchain|Toolchain Linux]] | dependências e submódulos validados | done |
| 02 | [[../plans/BT/02-rom-decompression-and-elf|ROM, decomposição e ELF]] | `make setup` e ELF reproduzíveis | done |
| 03 | [[../plans/BT/03-recompiler-metadata|Metadados do recompilador]] | símbolos, seções, overlays e relocations gerados | done |
| 04 | [[../plans/BT/04-static-recompilation|Recompilação estática]] | C gerado compila sem símbolos ausentes | done |
| 05 | [[../plans/BT/05-native-runtime-and-boot|Runtime e boot]] | executável alcança o primeiro frame | done |
| 06 | [[../plans/BT/06-overlays-and-syscalls|Overlays e syscalls]] | carregar, executar, mover e descarregar overlays funciona | done |
| 07 | [[../plans/BT/07-rsp-and-audio|RSP e áudio]] | microcódigos conhecidos e áudio estável | active |
| 08 | [[../plans/BT/08-rt64-input-and-frontend|RT64, entrada e frontend]] | logo/menu/gameplay renderizam e controles funcionam | active |
| 09 | [[../plans/BT/09-gameplay-parity-and-enhancements|Paridade e melhorias]] | campanha, saves e timing aprovados | pending |
| 10 | [[../plans/BT/10-linux-packaging-and-release|Empacotamento e release]] | pacote Linux limpo, reproduzível e sem assets do jogo | pending |

## Marcos de integração

1. **M0 — Insumos confiáveis:** ROM, commits, submódulos e toolchain fixados.
2. **M1 — Compile-only:** `RecompiledFuncs` gerado e compilado.
3. **M2 — First frame:** runtime inicializa RDRAM, entra no código recompilado e
   apresenta um frame.
4. **M3 — Logo da Rare:** CPU, RSP, RT64, áudio e entrada atravessam o boot.
5. **M4 — Gameplay:** uma área jogável carrega e troca overlays corretamente.
6. **M5 — Paridade:** campanha e saves aprovados em uma matriz de regressão.
7. **M6 — Release:** artefato Linux reproduzível e pronto para usuário final.

## Estratégia de execução

- Trabalhar uma etapa por vez e não iniciar a próxima sem o gate anterior.
- Registrar cada comando reproduzível no documento da etapa correspondente.
- Tratar arquivos gerados como produtos de ferramentas, não como fontes
  mantidas manualmente.
- Manter patches funcionais separados da camada de compatibilidade obrigatória.
- Preservar primeiro timing, resolução e comportamento originais; melhorias
  entram somente depois da paridade.
- Fixar SHAs dos repositórios e hashes dos artefatos usados em cada marco.
- Executar comandos BT pelo caminho `banjo-tooie-Recomp/`; não recriar nem
  depender do checkout excluído `banjo-tooie/`.
- Abrir branches de implementação no fork e manter `upstream` como referência.

## Riscos prioritários

| Risco | Efeito | Mitigação |
| --- | --- | --- |
| Modelo de overlays mais complexo que BK | chamadas inválidas e corrupção de RDRAM | gerar tabelas a partir do ELF/ROM e testar load/unload isoladamente |
| Relocations ou syscalls incompletos | falhas ao entrar em código dinâmico | validar cobertura e abortar geração diante de tipo desconhecido |
| Microcódigo RSP diferente | ausência de áudio ou tarefas desconhecidas | inventariar `OSTask`, recompilar cada variante e manter erro explícito |
| Dependência implícita de frame | física/timers incorretos em FPS alto | obter paridade no timing original antes de desacoplar updates |
| Divergência do decomp | símbolos e offsets inconsistentes | fixar commit, reconstruir ROM matching e guardar hashes |
| Dependência acidental de assets | release juridicamente impróprio | teste automático que rejeita ROM e assets no pacote |

## Fonte de verdade

- Especificações BK: [[../specs/BK/index]]
- Especificações BT: [[../specs/BT/index]]
- Etapas implementáveis: [[../plans/BT/index]]
- Critérios finais: [[definition-of-done]]
- Baseline P00: [[baseline-manifest]]
- Registro anterior, agora supersedido: [[../roadmap/tooie-recomp]]

## Evidência P08 / P09 / P10 corrente

- [[evidence/p10-kitfree-generation-and-ui-restoration-20261021]] — **CURRENT (P10, 2026-10-21):**
  Geração 100% Kit-Free comprovada em sandbox bubblewrap (Sonnet, 420/420 arquivos de `RecompiledFuncs/` byte a byte idênticos à rota ELF, 1159 passos de build Clang/Ninja, 8 testes unitários host PASS; sem kit IDO/gcc, sem MIPS tools, sem ELF no build normal). Interface de usuário, artes vetoriais (`Banjo.svg`, `Kazooie.svg`, `Cloud*.svg`, `Jiggy*.svg`), animações, ícones, fontes tipográficas (`Suplexmentary Comic NC.ttf`, OFL) e logo Tooie original (7300x2450) restaurados localmente pelo root a partir do backup de candidatos (correção formal do usuário D1/D2; UI neutra interina superada). Target gerenciado `TooieUIAssets` sincroniza `assets/` a cada build.

- [[evidence/p10-standalone-vendor-20261021]] — **P10 (anterior):**
  Dependências do BanjoRecomp vendorizadas como arquivos regulares (`lib/`, `decomp/`, `assets/`, `tools/wheelhouse/`); proveniência em `vendor/`; scripts legados aposentados.

- [[evidence/p08-user-demo1-movement-and-fire-egg-differential]] — **CURRENT (P08):**
  USER_DEMO1 single-feature causality PROVEN: on unchanged C56 binary, setting ONLY
  `TOOIE_TEST_OBJECT_FRUSTUM_ASPECT_AWARE=0` restored 355x12 NPC prefix, 39x14 record
  words, 14 RNG snapshots, and post 355 Fire Egg sweep hit (v0=0x80132f60); underlying
  mechanism UNKNOWN; next action is owning-layer separation of expanded DRAW from simulation.
- [[evidence/p08-user-demo4-800979d8-independent-review]] — **CURRENT:**
  helper contract, exact BCE84 ring-1 raw join, focused test, and bounded P60/
  ncbapivot chain match in content. The immutable log-hash gate is blocked;
  global attract parity is not claimed.
- [[evidence/p08-user-demo4-bainput-held-state-provenance]] — **CURRENT:**
  USER_DEMO_4 glidx 39 pad `0001` MATCH; ring-1 BCE84 word `0x115`,
  `bakey_held=1`, `0x800979D8=2`, and P60 after-write values match. Gemini
  pad-equals-held claim is CORRECTED/SUPERSEDED.

- [[evidence/p08-attract-movement-residuals]] — **CURRENT:** user-reproduced
  USER_DEMO_1 and USER_DEMO_5 movement residuals; Talon-Trot/locomotion link is
  an inference only. Next target is Demo 5 first divergence.

- [[evidence/p09-menu-interpolation-toggle-20261021]] — **CURRENT (2026-10-21):** opção Interpolation ON/OFF
  no menu ESC (persistida, padrão OFF, runtime sem restart via patch RT64 host-endpoint-current) e F8 removido;
  build/testes host e jogo isolado provados; não corrige glitch nem aprova o gate técnico.

- [[evidence/p09-articulated-world-guard-cause-and-fix-20261020]] — **P09 Iteration 3 (anterior):**
  RigidBody AUTO linear speed gate cosine amplification root cause A PROVEN in source
  (rt64_rigid_body.cpp:30-34); owning-layer fix lerpTranslationSpeedOnly; matched GPU frame
  pairs and chord overshoot metric 26,185->2,673; independent gate (resumed-gate.json) FAILED
  (unverified build logs, fatal ong2 SIGSEGV rc=-11 crash, unversioned diagnostic state in
  RT64 working tree, missing non-player articulated and 60 FPS gameplay check); production
  default strictly CURRENT/30.
- [[evidence/p09-fps-counter-and-settings-toggle]] — **CURRENT (P09-RES-05):**
  top-left present FPS counter and Graphics settings toggle verified; RT64 present
  success counter and RmlUi overlay hook documented; unit test PASS; runtime QA pending.
- [[evidence/p09-user-observed-parity-residuals]] — **PROVISIONAL_PRE_P09_PARITY_EVIDENCE:**
  Demo 0 only: 765/765 input, bounded physics/behavior evidence, Blue Egg parity at
  glidx 322, and Angrylion Z-buffer/classifier result. Fire Egg report remains
  `USER_REPORTED_UNLOCATED`; global visual drift remains `USER_REPORTED / OPEN`;
  Demo 1..3 are `UNTESTED`. P09 is not formally open.
- [[evidence/p08-demo-camera-eye-producer-differential]] — **PROVEN (A & B RESOLVED):**
  causal isolation of glidx 384 camera eye divergence; `ncbapivot` conditional invocation of `func_801138F4 -> func_80112E98` creates 3rd `SET74` in original N64, bypassed in Recomp.
- [[evidence/p08-t07-t08-frontend-system-validation]] — **P08-T07 & P08-T08 VALIDATED:**
  window/display integration, aspect ratio safety, graphics driver & Vulkan failure handling.
- [[evidence/p08-attract-loop-after-demo2-crash]] — **RESOLVED and ACCEPTED:**
  deterministic SIGSEGV after DEMO2; `bsrest` `LOOKUP_FUNC(0x808003FC)`
  resolved via N64Recomp CreateStatic ordering fix (Level A, verdict B).
- [[evidence/p08-stick-angle-thunk-fix]] — implementation evidence;
  native shared-tail override; cardinals/diagonals/third-arg PASS.
- [[evidence/p08-stick-angle-thunk-fix-rereview]] — **CURRENT:** focused
  independent re-review **VERDICT A**; missing `$r1` delay-slot side effect
  resolved and function-level fix accepted.
- [[evidence/p08-stick-angle-thunk-fix-independent-review]] — historical
  semantic review **VERDICT C**; blocker resolved by correction and re-review.
- [[evidence/p08-player-direction-facing-diagnosis]] — Level A empty-13B70
  diagnosis; resolution pointer to the fix.
- [[evidence/p08-original-timing-4x3-baseline]] — P08-T03
  original timing/4:3 baseline; source contract characterized, live gate blocked.
- [[evidence/p08-title-file-select-validation]] — P08-T10 **VALIDATED / COMPLETE**:
  intro -> file select -> slot navigation -> confirmation -> Spiral Mountain PASS.
- [[evidence/p08-directional-input-movement-diagnosis]] — right-only movement
  report not reproduced on e97; host→N64 pad conversion bidirectional.
- [[evidence/p08-80013a7c-lookup-zero-diagnosis]] — **CURRENT:** `func_80013A7C`
  lookup-zero root cause PROVEN (A); leaf wrapper saves RA in $a3, direct host calls do not write ctx->r31.
- [[evidence/p08-bcf28-binding-fix-independent-review]] — BCF28 fix independently accepted (verdict B).
- [[evidence/p08-bcf28-binding-fix]] — BCF28 helper bindings
  fix implemented & validated; exact Tooie-native recomp accessors bound.
- [[evidence/p08-bcf28-crash-diagnosis]] — `func_800BCF28` SIGSEGV
  root cause PROVEN (A); `get_function(0x800BCCC0)` = `lookup_gap_stub` (v0=0)
- [[evidence/p08-preemption-patch4-clean-build-independent-review]] — **CURRENT:**
  verdict B; tracked-state blocker resolved; clean regen/build and clean-built
  same-binary CLASS A; PATCH 4 accepted.
- [[evidence/p08-preemption-patch4-clean-build-correction]] — implementer
  correction; empty `RecompiledPatches` scaffolds tracked.
- [[evidence/p08-preemption-patch4-independent-review]] — historical verdict C
  preserved; behavior CLASS A; distribution finding resolved by current review.
- [[evidence/p08-preemption-patch4-validation]] — implementer evidence;
  preserved, with direct table[2] zero treated as inference.
- [[evidence/p08-preemption-patch35-rcp-independent-review]] —
  verdict B; per-OSThread RCP correction accepted.
- [[evidence/p08-preemption-patch35-rcp-correction]] — implementer correction
  evidence; TLS RCP, RCP-X1..X4 and P3-X6+.
- [[evidence/p08-preemption-patch35-independent-review]] — historical verdict
  C that proved the old process-global defect; finding resolved, preserved.
- [[evidence/p08-preemption-patch35-validation]] — implementer evidence;
  PATCH4 gate corrected/superseded by independent review.
- [[evidence/p08-preemption-patch35-thread-switch-review]] —
  thread-switch eligibility; verdict B; resume hook (design 4.8).
- [[evidence/p08-preemption-patch3-independent-review]] —
  PATCH 3 independent review B.
- [[evidence/p08-preemption-patch3-validation]] — PATCH 3 generator
  landed; feature OFF; T11 PASS; sem regen Tooie.
- [[evidence/p08-preemption-patch2-validation]] — PATCH 2 runtime
  service landed; feature OFF; T01–T10/T12 PASS.
- [[evidence/p08-preemption-implementation-plan]] — spec Hybrid F'
  (PATCH 1–4 + T01–T15). PATCH 4 landed.
- [[evidence/p08-preemption-design-adversarial-review]] — arquitetura F'
  aprovada (veredito C). DeepSeek Hybrid F rejeitado as-is.
- [[evidence/p08-permanent-preemption-design]] — DeepSeek Hybrid F;
  `CORRECTED/SUPERSEDED`.
- [[evidence/p08-guest-preemption-causal-validation]] — experimento
  limpo class A — o gap de preempção do runtime é provado causalmente: com
  `yield_self_1ms` em `func_800C2AB8`, a thread AUDIO roda, `func_800205D0`
  limpa `table[2].word0`, `func_800C269C(7)==1`, `func_800C16A4(7)` dispensa o
  slot 7, `func_800C2AB8` retorna e o attract progride até um SIGSEGV downstream
  novo em `func_800BCF28` (fora do escopo).
- [[evidence/p08-guest-preemption-runtime-audit]] — auditoria estrutural do gap
  N64 vs N64ModernRuntime (Gemini).
- [[evidence/p08-slot7-scheduled-event-validation]] — validação dinâmica
  clean-freeze do contrato do scheduler (3 runs): objeto de `table[2]`
  agendado na delta-queue do SoundManager (head, delta 0), handler
  `func_8001ED68` registrado, `func_8001EE34` pending, thread `[Game] AUDIO`
  estacionada + contadores de áudio congelados → **CLASS A** (callback agendado
  e devido, mas contexto de dispatch não pode rodar).
- [[evidence/p08-slot7-scheduled-event-driver]] — reconstrução estática do
  scheduler/event-driver (Gemini).
- [[evidence/p08-slot7-object-lifecycle-arbitration]] — arbitragem lifecycle
  (estrutura/clearer/ordem; origem UNKNOWN).
- [[evidence/p08-type4-transition-semantics-audit]] — auditoria estática +
  clean-freeze type-4 snapshot (8/8 runs): type-4 completa, campos
  `0.9/3.5/2883` corrigidos para `+0x0C/+0x18/+0x10`, warp consumido; blocker
  downstream no mode-5/attract init.
- [[evidence/p08-forced-cinematic-facing-diagnosis]] — historical USER_BUG_B
  one-actor facing evidence (Level B preserved historically).
- [[evidence/p08-forced-cinematic-facing-independent-review]] — **CURRENT:**
  original return-facing contract and selected node discriminator UNKNOWN;
  historical Level B CORRECTED/SUPERSEDED; no fix.
- [[evidence/p08-demo-movement-precision-diagnosis]] — recorded-pad demo path;
  first straight 510 is original slope-scaled physics; first analog
  transition `glidx 74..91` internally faithful; later residual deferred.
- [[evidence/p08-demo-camera-clearance-rays]] —
  bounded four-query clearance differential for func_80113D30 at glidx 384;
  measured Q1/Q2 return divergence preserved, but collision interpretation
  CORRECTED/SUPERSEDED by the independent Sol review.
- [[evidence/p08-demo-camera-collision-sol-review]] — independent original-MIPS
  arbitration: func_800C4C34 is projection/viewport clipping, not collision;
  historical CLEAR/HIT labels refuted; root level R, no fix.
- [[evidence/p08-demo-ncbapivot-branch-predicate]] —
  bounded branch-predicate differential for ncbapivot at glidx 384; proven
  case C (C0 branch not taken both; gate divergence at 0x808001D0 via func_801138A0
  and func_80113D30 clearance tests).
- [[evidence/p08-demo-camera-eye-producer-differential]] —
  empirical producer differential for Wooded Hollow camera eye vector divergence
  across glidx 380..388.
- [[evidence/p08-demo-zbuffer-oracle-sol-review]] — **CURRENT:** independent
  oracle arbitration; Rice-zero invalid for CPU-visible rendered Z, RT64 exact
  format refuted, root C-ORACLE; Angrylion Plus + CXD4 LLE run next.
- [[evidence/p08-session-handoff]] — estado autossuficiente e próximo single
  action para um novo chat.
- [[evidence/p08-intro-to-demo-transition-diagnosis]] — timeline histórica;
  narrativa antiga de type-4 incompleta marcada como SUPERSEDED.
- [[evidence/p08-visual-validation/README]] — inventário/interpretação humana
  dos frames da intro ao preto pós-Jiggy.

## Atualização de estado

Ao concluir uma tarefa, marque o checkbox no arquivo da etapa e anexe a
evidência na seção `Evidências`. Ao fechar uma etapa, altere `status` para
`done` e atualize sua linha neste plano. Isso permite ao Markdown Vault MCP
extrair tarefas, montar briefings e recuperar o contexto pelo campo `phase`.
