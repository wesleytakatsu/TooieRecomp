# Índice da documentação

## Notas

- [[AGENTS]] — Regras para agentes — TooieRecomp — Regras de navegação, implementação e evidência para agentes que trabalham no plano TooieRecomp.
- [[HANDOFF]] — Handoff — TooieRecomp — USER_BUG_A WASD resolved; first-straight 510 and first analog PASS; USER_BUG_B diagnosed on one sucoaster chdiggerground (Level B).
- [[MARKDOWN_VAULT_TOOLS]] — Guia de uso do MCP markdown-vault — Este vault usa as tools do MCP `markdown-vault` (`/home/wesley/projetos/markdown-vault-mcp-standalone`).
- [[README]] — Documentação — Esta pasta descreve o funcionamento do projeto de recompilação estática do Banjo-Tooie para PC (`N64Recomp` + `N64ModernRuntime` + `RT64`).

## plan

- [[plan/evidence/p08-user-demo4-ca8b4-first-seed]] — P08 — USER_DEMO_4 CA8B4 first seed — Bit-level first follow-camera difference at glidx 1 (`ncbapivot` output; init MATCH; CA8B4 identity copier).
- [[plan/evidence/p08-user-demo4-follow-camera-first-discriminator]] — P08 — USER_DEMO_4 follow-camera CA628 writer/input discriminator — CA628 stores src2[1] at +0x1C; first window sample already differs at glidx 36.
- [[plan/evidence/p08-attract-loop-after-demo2-crash]] — P08 — attract-loop crash after DEMO2 — Deterministic SIGSEGV: bsrest_entrypoint_11 LOOKUP_FUNC(0x808003FC) binds baflameline_entrypoint_5.
- [[plan/evidence/p09-user-observed-parity-residuals]] — provisional pre-P09 evidence — Demo 0-only input/physics/behavior and Blue Egg parity; Fire Egg and global visual-drift reports remain open; Demo 1..3 untested.
- [[plan/baseline-manifest]] — Baseline manifest — TooieRecomp P00 — Ambiente, commits, remotes, ROM, decisões e licenças congelados na P00.
- [[plan/definition-of-done]] — Definição de pronto — TooieRecomp — Critérios objetivos para considerar Banjo-Tooie concluído como port nativo no Linux.
- [[plan/evidence/p07-boot-hang-audio-diag]] — P07 boot hang + audio silence diagnosis (2026-08-09) — With `TOOIE_DIAG=1` (~12–18 s after auto-start):
- [[plan/evidence/p08-forced-cinematic-facing-diagnosis]] — P08 — forced/cinematic actor facing diagnosis — USER_BUG_B one-actor: Wooded Hollow sucoaster `chdiggerground` reverse walk, heading 294.21° vs yaw 114.21° (Level B).
- [[plan/evidence/p08-forced-cinematic-facing-independent-review]] — P08 — independent review: return contract and selected node discriminator remain UNKNOWN; historical Level B is CORRECTED/SUPERSEDED, no fix.
- [[plan/evidence/p08-demo-movement-precision-diagnosis]] — P08 — demo movement precision diagnosis — Wooded Hollow first-straight 510 is original slope-scaled physics (`func_800EF04C` from `func_8009AE08`); post-physics writer REFUTED; FIRST_STRAIGHT_INTERNAL_FIDELITY PASS.
- [[plan/evidence/p08-stick-angle-thunk-fix-correction]] — P08 — func_80013B70 review correction — Minimal semantic correction reproducing $at/$r1=0x43340000 delay slot on all y==0 paths.
- [[plan/evidence/p08-stick-angle-thunk-fix]] — P08 — stick-angle secondary-entry fix — Native shared-tail override for empty generated func_80013B70; cardinals 0/90/180/270 and third-argument contract PASS.
- [[plan/evidence/p08-stick-angle-thunk-fix-rereview]] — P08 — focused independent re-review of corrected func_80013B70 — Verdict A; prior `$r1` delay-slot blocker resolved; function-level fix accepted, runtime player validation pending.
- [[plan/evidence/p08-stick-angle-thunk-fix-independent-review]] — P08 — historical independent review of the func_80013B70 secondary-entry fix — Verdict C preserved; prior `$r1` delay-slot blocker resolved by correction and re-review.
- [[plan/evidence/p08-player-direction-facing-diagnosis]] — P08 — player direction / facing diagnosis — Level A empty 13B70; USER_BUG_A WASD RESOLVED; attract-bypass claim corrected; USER_BUG_B separate.
- [[plan/evidence/p08-directional-input-movement-diagnosis]] — P08 — directional input / movement diagnosis — Host→N64 pad conversion proven bidirectional; live-player angle later owned by func_80013B70.
- [[plan/evidence/p08-original-timing-4x3-baseline]] — P08-T03 — original timing / 4:3 baseline — 60Hz VI, 20/30Hz logic, 30Hz animation, host decouple, and 4:3 content aspect validated with live measurements.
- [[plan/evidence/p08-render-submission-matrix-fix]] — P08 — Tooie render-submission matrix fix — Implementation of the proven incremental projection and view matrix submission split in TooieRecomp, avoiding RT64 viewproj decomposition and restoring correct camera/actor view extraction.
- [[plan/evidence/p08-render-submission-matrix-diagnosis]] — P08 — render-submission matrix diagnosis — Rare view-in-projection vs RT64 G_MTX_PROJECTION LOAD decompose. Banjo incremental MUL avoids the path Tooie hits.
- [[plan/evidence/p08-actor-camera-transform-discriminator]] — P08 — actor and camera transform discriminator — Telemetry discrimination between Actor.rotation simulation state, model matrix construction, and camera view transform during the first visible Wooded Hollow attract demo.
- [[plan/evidence/p08-visible-demo-movement-diagnosis]] — P08 — visible demo movement diagnosis — Visible attract/gameplay demo reproduced after the RA-wrapper fix; first fidelity gap is character/camera transform (Level C). Historical 150s DIAG “no demo” run is preserved.
- [[plan/evidence/p08-core1-ra-wrapper-fix]] — P08 — core1 RA wrapper fix — Tooie-native strong-symbol overrides for 0x800137D4 / 0x80013A5C / 0x80013A7C; old LOOKUP_FUNC(0) resolved.
- [[plan/evidence/p08-80013a7c-lookup-zero-diagnosis]] — P08 — func_80013A7C lookup-zero diagnosis — Proven root cause (Level A): leaf wrapper saves RA in $a3, but direct host calls do not write ctx->r31, causing LOOKUP_FUNC(0). Fix implemented.
- [[plan/evidence/p08-bcf28-binding-fix-independent-review]] — P08 — func_800BCF28 helper binding fix independent review — Verdict B: native 0x800BCCC0/0x800BCCD8 bindings accepted; old SIGSEGV resolved; next blocker func_80013A7C (not diagnosed).
- [[plan/evidence/p08-bcf28-binding-fix]] — P08 — func_800BCF28 helper binding fix — Tooie-native recomp helpers bound at 0x800BCCC0 and 0x800BCCD8, replacing erroneous gap stubs and resolving func_800BCF28 SIGSEGV.
- [[plan/evidence/p08-bcf28-crash-diagnosis]] — P08 — func_800BCF28 SIGSEGV root cause PROVEN (A) — `get_function(0x800BCCC0)` = `lookup_gap_stub` (v0=0) masks real element accessor (a0+a1*56+0x24); caller dereferences null base → SIGSEGV at rdram+0x8000000c. Binding fix later implemented and independently accepted.
- [[plan/evidence/p08-active-banjo-reuse-audit]] — P08 — Active BanjoRecomp reuse audit — Auditoria diagnóstico-only: risco de código/estado Banjo-Kazooie ativo no executável TooieRecompiled. Verdict: nenhum behavior BK alcançável (code recompilado Tooie só chama recomp_syscall_handler/func_*; exports recomp_get_* BK não registrados/chamados → DORMANT). N64ModernRuntime DIRTY (threadqueue.cpp + N64Recomp use_lookup_for_all_function_calls). RecompiledPatches = scaffold vazio. Build não reproduzível (5 itens). Scheduler bug = LOCAL TOOIE PATCH IN SHARED RUNTIME. Recomendação: CONTINUE P08.
- [[plan/evidence/p08-actor-init-segv-diagnosis]] — P08 — diagnóstico do SIGSEGV na inicialização de actor — Prova com GDB que o SIGSEGV em vector_push_back (via func_80106668/func_80107C2C) é artefato do spawn diagnóstico: D_80136EE0 (vetor global de Actors) está NULL porque o initializer (chbaddieDll ep1 → func_800FF390 → func_801065E0) nunca executou. Natural flow diverge antes. Não é bug de vector/runtime.
- [[plan/evidence/p08-actor-manager-init-divergence]] — P08 — actor manager init: premissa desatualizada (cadeia executa no HEAD) — Diagnóstico: a cadeia legítima chbaddieDll ep1 → func_800FF390 → func_801065E0 → D_80136EE0 executa NATURALMENTE no HEAD c43e89b. A evidência anterior de 0 hits (a6053a0) foi coletada antes dos fixes de relocations/overlay-cache/scheduler. Não existe primeira divergência neste caminho no HEAD.
- [[plan/evidence/p08-actor-null-callback-diagnosis]] — P08 — diagnóstico: callback nulo em func_80108C90 (ActorInfo[0x38]) — Prova com runtime + ROM que o valor 0 lido como callback de actor no spawn forçado não é um callback opcional legítimo; a origem é o mapeamento de data de overlay.
- [[plan/evidence/p08-controller-host-path]] — P08 — caminho host dos controles — Data: 2026-08-09
- [[plan/evidence/p08-do-dma-segv-diagnosis]] — P08 — SIGSEGV do_dma: dinamic diagnosis — **HEAD:** `f865c038455da1e5fafb5257ece974e1e15c9399` **Modo:** diagnóstico somente. Zero código alterado. **Status:** ROOT HIGH — overlay base-vram collision no func_map.
- [[plan/evidence/p08-first-area-playable]] — P08 — cena 3D após a transição — Smoke dirigido: boot → jiggy → terreno/água da abertura; câmera e sequência não progridem.
- [[plan/evidence/p08-gcsection-glintrosync-segv-diagnosis]] — P08 — SIGSEGV natural glintrosync:6 → gcsection:4 (diagnóstico) — SIGSEGV natural glintrosyncDll:6 → gcsectionDll:4 — func_808000F0 retornava NULL porque ovl_unload não invalidava s_resident_overlays (cache stale; heap reutilizado por overlay 709). FIX aplicado. ROOT CAUSE E, HIGH.
- [[plan/evidence/p08-intro-actor-spawn-diagnosis]] — P08 — diagnóstico de divergência no spawn de actors da intro — Identificação do primeiro ponto de divergência na cadeia de inicialização e spawn dos actors da abertura (chintroticker 419, chintrochar 440, chintrocam 441).
- [[plan/evidence/p08-intro-to-demo-transition-diagnosis]] — P08 — INTRO → ATTRACT DEMO transition diagnosis — Checkpoint da validação visual, warp/type-4 e post-mortem do freeze natural depois da Jiggy transition.
- [[plan/evidence/p08-natural-actor-spawn-divergence]] — P08 — divergência no spawn natural de actors (IPL3 integrity gate) — Diagnóstico fechado: gate anti-tamper func_8001DE64 / D_8007DB79 bloqueia pipeline func_800A5C28 → func_800BDCDC → gccubeDll:5 → gccollectDll:8 → actor creation. Stub address +0x10000 + missing guest words. Discriminator matrix. Fix A decidido, NÃO implementado.
- [[plan/evidence/p08-no-threads-left-diagnosis]] — P08 — Diagnóstico: "No threads left to run!" — std::runtime_error No threads left to run! = run_next_thread com a running queue vazia (NÃO é esgotamento de threads). ROOT CAUSE E, HIGH: patch local de threadqueue.cpp rejeita re-inserção após osSetThreadPri porque thread_queue_remove não limpa t->queue; fila drena até vazia e a thread SCHEDULE bloqueia em osRecvMesg. Diagnosis-only; fix estreito externo pendente de autorização.
- [[plan/evidence/p08-overlay-data-relocation-diagnosis]] — P08 — diagnóstico: relocation de data de overlay perdida (chcollballcontrol) — Prova que o construtor recompilado devolvia 0x80804FA0 (link-time) porque o pipeline de metadados truncava nomes de seção; relocs existiam no ELF. Fix na camada generator/metadata; ActorInfo[0x38] == 0x80107C2C.
- [[plan/evidence/p08-overlay-rmips26-binding-diagnosis]] — P08 — corrigir binding de R_MIPS_26 intra-overlay no GENERATOR — Fix estrutural no N64Recomp: jal same-section em seções relocatable agora resolve para chamada direta a C, mesmo com use_lookup_for_all_function_calls=true. Elimina a colisão de base vram 0x80800000 (chintrochar→gsworldDll). Scheduler preservado via restrição a relocatable. Validado 3/3 smokes + 140s run.
- [[plan/evidence/p08-runtime-frontend-smoke]] — BT P08 — runtime, launcher e música — Evidência do desbloqueio DP/VI e do ciclo de música do launcher.
- [[plan/evidence/p08-demo-zbuffer-oracle-sol-review]] — P08 — independent Z-buffer oracle-validity review — Rice-zero is not a CPU-visible rendered-Z oracle; RT64 ownership/order is valid but exact format is refuted; C-ORACLE.
- [[plan/evidence/p08-session-handoff]] — P08 — session handoff: attract demo e render submission — Visible demo and matrix fix validated; right-only input report not reproduced on e97.
- [[plan/evidence/p08-slot7-object-lifecycle-arbitration]] — P08 — slot7 table[2] object lifecycle arbitration (Grok) — Arbitragem Gemini×DeepSeek: creator 20790, clearer 205D0, countdown +0x34, C2AB8 progress A|C; origem UNKNOWN; ROOT_CAUSE_PROVEN=NO.
- [[plan/evidence/p08-guest-preemption-runtime-audit]] — P08 — guest preemption & runtime scheduler structural audit — Auditoria estrutural do gap N64 (interrupt preemption) vs N64ModernRuntime (cooperative token único).
- [[plan/evidence/p08-guest-preemption-causal-validation]] — P08 — guest preemption causal validation (clean experiment, class A) — Experimento limpo reversível: yield_self_1ms em func_800C2AB8 → AUDIO roda, func_800205D0 limpa table[2].word0, C2AB8 retorna, attract progride (até SIGSEGV downstream novo em func_800BCF28). Gap de preempção provado causalmente.
- [[plan/evidence/p08-permanent-preemption-design]] — P08 — permanent guest-preemption design (DeepSeek Hybrid F) — `CORRECTED/SUPERSEDED` pela revisão adversarial.
- [[plan/evidence/p08-preemption-design-adversarial-review]] — P08 — adversarial review of generic guest-preemption design — Hybrid F quebrado (veredito C); Hybrid F' aprovado.
- [[plan/evidence/p08-preemption-implementation-plan]] — P08 — preemption Hybrid F' implementation plan — PATCH 3.5 accepted; PATCH4_READY YES.
- [[plan/evidence/p08-preemption-patch4-clean-build-independent-review]] — P08 — independent review of PATCH 4 clean-build correction — Verdict B; tracked-state blocker resolved; PATCH 4 accepted.
- [[plan/evidence/p08-preemption-patch4-clean-build-correction]] — P08 — PATCH 4 clean-build / tracked-state correction — Track empty RecompiledPatches scaffolds so a clean clone can build TooieRecompiled. Independent review verdict C is preserved; this is the implementer correction.
- [[plan/evidence/p08-preemption-patch4-independent-review]] — P08 — independent review of PATCH 4 — Behavior CLASS A; verdict C because clean tracked Tooie build lacks reproducible `RecompiledPatches` scaffolds.
- [[plan/evidence/p08-preemption-patch4-validation]] — P08 — PATCH 4 Hybrid F' activation / regen / T13–T15 — Tooie-scoped startup ON; 4298 safepoints; C2AB8 CLASS A; current blocker func_800BCF28.
- [[plan/evidence/p08-preemption-patch35-rcp-independent-review]] — P08 — independent review of PATCH 3.5 RCP correction — Verdict B; old-global discriminator and regressions PASS; PATCH4_READY YES.
- [[plan/evidence/p08-preemption-patch35-rcp-correction]] — P08 — PATCH 3.5 RCP per-OSThread correction — TLS `current_mi_mask`; RCP-X1..X4 and P3-X6+ PASS; flag OFF.
- [[plan/evidence/p08-preemption-patch35-independent-review]] — P08 — historical independent review of preemption PATCH 3.5 — Verdict C; process-global RCP defect proven, now resolved by the preserved correction review.
- [[plan/evidence/p08-preemption-patch35-validation]] — P08 — PATCH 3.5 implementer validation — A+ hold-until-eligible claims; PATCH4 gate corrected/superseded by independent review.
- [[plan/evidence/p08-preemption-patch35-design]] — P08 — PATCH 3.5 semantic-closure design — implemented/validated.
- [[plan/evidence/p08-preemption-patch3-independent-review]] — P08 — independent review of preemption PATCH 3 — Veredito B; PATCH 3 aceite; branch-likely PROVEN; JR genérico INCOMPLETE; PATCH 3.5 design next; PATCH4_READY NO.
- [[plan/evidence/p08-preemption-patch3-validation]] — P08 — preemption PATCH 3 generator safepoint validation — T11 static/live/ABI PASS; flag OFF; sem regen Tooie.
- [[plan/evidence/p08-preemption-patch2-independent-review]] — P08 — independent review of preemption PATCH 2 — Veredito B; PATCH 3 ready; X1 cause/class before PATCH 4; C ABI must wrap status_reg.
- [[plan/evidence/p08-preemption-patch2-validation]] — P08 — preemption PATCH 2 runtime service validation — epoch/service/intmask; T01–T10/T12 PASS; flag OFF; cumulative artifact.
- [[plan/evidence/p08-preemption-patch1-independent-review]] — P08 — independent review of preemption PATCH 1 — Veredito B; PATCH 2 ready; T01-E WEAK; cumulative artifact B; PATCH 3 void*/recomp_context* collision.
- [[plan/evidence/p08-preemption-patch1-validation]] — P08 — preemption PATCH 1 scaffold validation — stubs, flag OFF, harness T01 PASS, dirty files untouched.
- [[plan/evidence/p08-slot7-scheduled-event-validation]] — P08 — slot7 scheduled-event dynamic validation (clean freeze, class A) — 3 clean freezes: objeto de table[2] agendado na delta-queue do SoundManager (head, delta 0), handler func_8001ED68, func_8001EE34 pending, thread [Game] AUDIO estacionada + contadores de áudio congelados → CLASS A (callback agendado e devido, mas contexto de dispatch não pode rodar).
- [[plan/evidence/p08-slot7-scheduled-event-driver]] — P08 — slot7 scheduled event driver reconstruction (func_8001EE34 / func_8001ED68) — Reconstrução estática do scheduler, delta-queue e dispatch indireto (Gemini).
- [[plan/evidence/p08-attract-actor-loop-diagnosis]] — P08 — attract actor-loop / slot7 readiness busy-wait — MAIN em C2AB8; slot7 pending; func_80026500 refutado como hang PC.
- [[plan/evidence/p08-type4-transition-semantics-audit]] — P08 — auditoria estática + clean-freeze type-4 snapshot — Layout/estados da type-4; snapshot limpo (8/8 runs) prova type-4 completa, warp consumido e corrige 0.9/3.5/2883 para +0x0C/+0x18/+0x10. Blocker downstream no mode-5/attract init.
- [[plan/evidence/p08-threadqueue-b-fix-validation]] — P08 — Fix B threadqueue: validação — Validação do fix B em threadqueue.cpp (scan real de links, sem field-equality guard). Contrato de thread_queue_remove provado: HEAD PASS, NON-HEAD/TAIL FAIL (infinite loop). SECOND SCHEDULER BUG CONFIRMED: remove só checa o HEAD. NO_THREADS_LEFT FIXED (0/2 smoke). Novo blocker: SIGSEGV em thread_queue_insert com queue==NULLPTR via osSetThreadPri sobre thread RUNNING (JOY).
- [[plan/evidence/p08-title-input-eeprom]] — P08 — input title/file select — **CORRECTED/SUPERSEDED** by [[plan/evidence/p08-title-file-select-validation]]; historical blocked wording retained for history only.
- [[plan/evidence/p08-title-file-select-validation]] — P08-T10 — title/file-select — **VALIDATED / COMPLETE**; intro -> file select -> slot navigation -> confirmation -> Spiral Mountain PASS.
- [[plan/evidence/p08-visual-validation/README]] — P08 — visual validation: intro → attract handoff — Inventário das capturas e interpretação humana validada da abertura até a tela preta pós-Jiggy.
- [[plan/index]] — Plano mestre — Banjo-Tooie nativo no Linux — Plano executável para transformar os insumos BanjoRecomp e banjo-tooie-Recomp em um port nativo completo de Banjo-Tooie para Linux.

## plans

- [[plans/BT/00-baseline-and-decisions]] — BT P00 — Baseline e decisões — Congela ambiente, fontes, ROM e decisões antes da implementação do TooieRecomp.
- [[plans/BT/01-linux-toolchain]] — BT P01 — Toolchain Linux — Instala e valida compiladores, bibliotecas, Python e submódulos usados pelos dois projetos.
- [[plans/BT/02-rom-decompression-and-elf]] — BT P02 — ROM, decomposição e ELF — Produz e valida os artefatos binários que serão a entrada da recompilação estática.
- [[plans/BT/03-recompiler-metadata]] — BT P03 — Metadados do recompilador — Converte ELF, símbolos, relocations, overlays e syscalls do Tooie em entradas determinísticas para N64Recomp.
- [[plans/BT/04-static-recompilation]] — BT P04 — Recompilação estática — Gera C/C++ nativo a partir do MIPS de Banjo-Tooie e fecha o primeiro build compile-only.
- [[plans/BT/05-native-runtime-and-boot]] — BT P05 — Runtime nativo e boot — Cria o executável TooieRecomp e integra ROM, memória, callbacks e entrada do jogo.
- [[plans/BT/06-overlays-and-syscalls]] — BT P06 — Overlays e syscalls — Implementa o ciclo completo dos módulos dinâmicos de Banjo-Tooie no runtime nativo.
- [[plans/BT/07-rsp-and-audio]] — BT P07 — RSP e áudio — Inventaria e integra os microcódigos RSP e o pipeline de áudio necessários por Banjo-Tooie.
- [[plans/BT/08-rt64-input-and-frontend]] — BT P08 — RT64, entrada e frontend — Completa renderização, janela, controles e experiência de inicialização no Linux.
- [[plans/BT/09-gameplay-parity-and-enhancements]] — BT P09 — Paridade de gameplay e melhorias — Fecha campanha, saves e timing antes de habilitar widescreen, FPS alto e extensões.
- [[plans/BT/10-linux-packaging-and-release]] — BT P10 — Empacotamento e release Linux — Produz, testa e publica um artefato Linux reproduzível sem conteúdo proprietário do jogo.
- [[plans/BT/index]] — Etapas de implementação — TooieRecomp — Índice ordenado das etapas necessárias para entregar Banjo-Tooie nativo no Linux.

## roadmap

- [[roadmap/tooie-recomp]] — TooieRecomp Roadmap — > [!NOTE] > Este roadmap histórico foi substituído pelo [[../plan/index|Plano mestre]] e > pelas [[../plans/BT/index|etapas de implementação]]. Ele permanece como > registro do raciocínio inicial.

## specs

- [[specs/BK/build]] — BK — Build — Full pipeline to go from the Banjo-Kazooie US ROM to a native executable, based on `BUILDING.md`.
- [[specs/BK/ci]] — BK — CI/CD & Release — The four GitHub Actions workflows under `BanjoRecomp/.github/workflows/`. The repo's builds are fully automated, so a portable release of the recomp can be regenerated from CI alone.
- [[specs/BK/config]] — BK — Recompiler Config — `banjo.us.rev0.toml` configures `N64Recomp` for the Banjo-Kazooie NTSC 1.0 ROM. All paths are relative to the TOML location.
- [[specs/BK/features]] — BK — Features — Feature set of the port, mostly provided by the `RT64` rendering engine, `N64ModernRuntime` runtime, `RecompFrontend` UI, and the native patch layer.
- [[specs/BK/index]] — Banjo: Recompiled — Specifications for the `BanjoRecomp` folder — the reference port of Banjo-Kazooie built via static recompilation.
- [[specs/BK/input]] — BK — Input & Controllers — `create_gfx()` / `main()` configure SDL2 for the whole session:
- [[specs/BK/mods]] — BK — Mods & Extension APIs — How `BanjoRecomp` lets third-party code attach to the recompiled game: per-object extension data, container APIs, and modI/O. This is the layer between the native [Patches](patches.md) runtime and full community mods.
- [[specs/BK/overview]] — BK — Static Recompilation Overview — The game's own assets are **not** shipped. The executable grabs them from a user-provided ROM at runtime. The repository and releases contain no game assets.
- [[specs/BK/patches]] — BK — Native Patches — BanjoRecomp changes game behavior by compiling **native C patches** into a MIPS ELF, then recompiling that ELF with `N64Recomp` and registering the result as overlays. This is the mechanism that powers features like widescreen, note saving, and camera changes.
- [[specs/BK/reuse-contract]] — BK — Contrato de reutilização para TooieRecomp — Separa componentes reutilizáveis do BanjoRecomp de dados e comportamento exclusivos de Banjo-Kazooie.
- [[specs/BK/rsp]] — BK — RSP ASP Microcode — Banjo uses the Nintendo *Audio Sample Processor* DSP microcode (`n_aspMain`) on the N64 RSP. The `RSPRecomp` tool converts that RSP binary code into C just like `N64Recomp` does for MIPS CPU code.
- [[specs/BK/runtime]] — BK — Runtime — How `BanjoRecomp` boots, registers recompiled code, and wires overlays, patches and game features through `N64ModernRuntime` / `RecompFrontend`.
- [[specs/BK/symbols]] — BK — BanjoRecompSyms — The `BanjoRecompSyms` submodule provides the symbol maps that let `N64Recomp` know which bytes map to which function. It is the BK analog of what `banjo-tooie-Recomp` produces via `symbol_addrs.us.txt` + ELF.
- [[specs/BK/ui]] — BK — UI Layer — A scene graph of animated SVGs driven by keyframe curves — no video asset, pure transform animation:
- [[specs/BT/build]] — BT — Build — The `Makefile` produces an ELF that matches the original ROM layout, then a recompressed runnable ROM. The invariant is byte-exact matching.
- [[specs/BT/ci]] — BT — CI/CD — The decomp repo's single workflow `.github/workflows/ci.yml`.
- [[specs/BT/compression]] — BT — ROM Compression & Recompression — Banjo-Tooie ships a **compressed cartridge**: boot is flat, but `core1`, `core2` and every overlay are deflated. The decomp repo must read the compressed baserom (decompress) and later rebuild the compressed ROM from the built ELF byte-for-byte (recompress).
- [[specs/BT/index]] — Banjo Tooie Decomp Fork — Especificações do fork local wesleytakatsu/banjo-tooie-Recomp e de sua relação com o upstream Mr-Wiseguy/banjo-tooie.
- [[specs/BT/input]] — BT — Input e controles nativos — Contrato de controles do TooieRecomp, deliberadamente alinhado ao BanjoRecomp.
- [[specs/BT/matching]] — BT — Matching & Decomp Workflow — How a `#pragma GLOBAL_ASM` stub becomes matched C code in `banjo-tooie-Recomp`, and the tools + CI that verify byte-exact output.
- [[specs/BT/overlays]] — BT — Overlay System — Banjo-Tooie makes heavy use of **dynamically loaded code overlays** — the largest complicating factor for a static recomp. Overlays are code banks loaded into a fixed 0x80800000 VRAM region at runtime.
- [[specs/BT/overview]] — BT — Decompilation Overview — WIP decompilation of Banjo-Tooie (NTSC-U) by **Mr-Wiseguy** (author of `N64Recomp` / `N64ModernRuntime` / `rarezip`). Uses **Splat** to split the ROM and rebase the game to an ELF.
- [[specs/BT/recomp-readiness]] — BT — Prontidão para recompilação estática — Inventário dos artefatos existentes no banjo-tooie-Recomp, contratos para N64Recomp e lacunas do futuro TooieRecomp.
- [[specs/BT/rsp-and-audio]] — BT — RSP e áudio — Config: `TooieRecomp/n_aspMain.us.rev0.toml`
- [[specs/BT/setup]] — BT — Setup — O checkout antigo `banjo-tooie/` foi excluído. Ele não deve aparecer em caminhos locais, scripts nem instruções. O nome sem `-Recomp` continua válido somente para identificar o repositório upstream.
- [[specs/BT/sources]] — BT — Source Tree — Layout of `banjo-tooie-Recomp/src/` (generated by Splat), organized by memory segment.
- [[specs/BT/splat]] — BT — Splat configuration — `baserom.us.yaml` drives **Splat**, which splits the decompressed ROM into `src/`, `asm/`, and `assets/`.
- [[specs/BT/symbols]] — BT — Symbols and Relocations — The repo ships plain-text symbol and relocation maps used by Splat at build time. These are the raw material for a static-recomp symbol table.
- [[specs/BT/syscalls]] — BT — Overlay Syscall Dispatch — Generates `overlay_syscalls.s` from `overlays.us.toml` (banjo-tooie-Recomp):
- [[specs/BT/tools]] — BT — Tooling — C++ and Python tools under `banjo-tooie-Recomp/tools/`, built by `tools/Makefile` and used by the main Makefile.
- [[specs/index]] — Specifications Index — Technical specifications for the two reference projects used to build a Banjo-Tooie static recompilation.
