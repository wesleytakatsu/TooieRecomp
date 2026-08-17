---
title: "BT P08 — RT64, entrada e frontend"
description: "Completa renderização, janela, controles e experiência de inicialização no Linux."
status: active
type: implementation-stage
project: TooieRecomp
phase: P08
order: 8
depends_on: P07
updated: 2026-08-16
tags: [banjo-tooie-Recomp, rt64, input, frontend, linux]
---

# BT P08 — RT64, entrada e frontend

## Resultado esperado

O jogo renderiza corretamente pelo RT64, aceita teclado/controle e oferece um
launcher Linux suficiente para selecionar ROM e configurações essenciais.

## Estado corrente

**ATTRACT DEMO2 CRASH RESOLVED (2026-08-16):** deterministic SIGSEGV after DEMO2
([[../../plan/evidence/p08-attract-loop-after-demo2-crash]]) is RESOLVED and VALIDATED.
`INTRO → 335 → 278 → 314 → 395 → INTRO → 235 → 296` succeed; `bsrest_entrypoint_11`
calls `static_344_808003FC` directly without lookup collision. Level A fix validated.

**P08 ACTIVE INTEGRATION STATE:** intro/Jiggy/type-4/warp OK;
**visible attract/gameplay demo reproduced** (mode-5, map 335 Wooded
Hollow). Hybrid F' PATCH 4 is independently CLASS A. PATCH 4 is accepted.
The `func_800BCF28` binding fix is **ACCEPTED (verdict B)** by
[[../../plan/evidence/p08-bcf28-binding-fix-independent-review]]. The
`func_80013A7C` lookup-zero is **RESOLVED**
([[../../plan/evidence/p08-core1-ra-wrapper-fix]]).
**Render-submission matrix split RESOLVED and VALIDATED**
([[../../plan/evidence/p08-render-submission-matrix-fix]]).
The user right-only live-player report is owned by the empty
`func_80013B70` secondary entry. The Tooie-native implementation is accepted
at function level after focused independent re-review verdict A
([[../../plan/evidence/p08-stick-angle-thunk-fix-rereview]]). Manual WASD
runtime is **RESOLVED**. Full analog/gamepad remains **PENDING**. The
gameplay-demo player uses `glrecord` recorded pad through bastick; the
first-straight ~510 is **original slope-scaled physics**
([[../../plan/evidence/p08-demo-movement-precision-diagnosis]] §14) and
the first later analog transition (`glidx = 74..91`) is **internally faithful**
([[../../plan/evidence/p08-demo-movement-precision-diagnosis]] §15).
**MEASURED_ORIGINAL vs MEASURED_RECOMP (2026-08-16):** official mupen64plus
Pure Interpreter, ROM SHA-1 match, pad 100% on consumed 1..765
([[../../plan/evidence/p08-demo-full-motion-sync-diagnosis]] §9). Same-index
~25u lead is capture-point phase (one physics step). First straight and
first analog **MATCH** after +1 shift. Shot `glidx=322` position **MATCH**.
The historical 45.11° camera-lag mismatch is **CORRECTED/SUPERSEDED**.
Remaining discriminator is the post-egg run/jump after ~392. Level C, no fix.
The first user-confirmed demo shot (`glidx = 322` fire-egg) remains a
**CLASS G** collision-space miss versus the downhill digger in the earlier
recomp-only study
([[../../plan/evidence/p08-demo-movement-precision-diagnosis]] §16);
this task did not re-diagnose egg collision.
No fix is implemented. `USER_BUG_B_CINEMATIC_FACING` is
**CURRENTLY_NOT_REPRODUCIBLE_BY_USER**.
The earlier `USER_BUG_B` one-actor Level B classification is
**CORRECTED/SUPERSEDED** by an independent review: original-MIPS case 11
matches generated instruction-for-instruction, advances the return position,
and preserves yaw/`unk54`. Wooded Hollow sucoaster `chdiggerground` therefore
has original actor/state semantics, not a recompilation defect. See
[[../../plan/evidence/p08-forced-cinematic-facing-independent-review]].
`USER_BUG_B_GLOBAL` remains **OPEN**; no fix is implemented.
The selected node discriminator is **RESOLVED** (2026-08-16, DeepSeek V4
Flash Free): `func_80802760` returns node 0 (`B_LIVE_NODE`) on all 3438
captured calls and never `-1` (`A_KEEP_OLD` REFUTED); the 180° freeze is
produced by the return-leg dispatch case 11 (`u72=0x3008` -> `L_80800A3C`)
which moves the digger back without writing `unk54`/`rotation.y`. Original
return-leg MIPS is PROVEN for this case; `func_80802760` equivalence remains
UNKNOWN because that separate function was not compared.
The historical verdict C and its narrow missing `$at` side-effect finding
are preserved in
[[../../plan/evidence/p08-stick-angle-thunk-fix-independent-review]];
host→N64 pad conversion
remains bidirectional and is not reopened. Attract/cinematic facing is a
separate bug. **P08-T10 frontend input is VALIDATED / PASS**. The later
"direct Spiral Mountain / synthetic confirm → mode=1 FAIL" wording is
**CORRECTED/SUPERSEDED** / **REFUTED**
([[../../plan/evidence/p08-title-file-select-validation]]): confirm
warps to `MAP_A1_CS_INTRO` (161) / `mode=3`; the sequencer does not
inject the later START presses that skip the opening story cutscenes.
**P08-T03 is VALIDATED** ([[../../plan/evidence/p08-original-timing-4x3-baseline]]).

Reconstrução do Scheduler/Driver e Preempção: A cadeia completa de despacho
indireto de áudio e o gap estrutural de preempção foram provados:
- Hardware N64: Hardware interrupt preempta `OSThread 6` MAIN (Pri 20) imediatamente para `OSThread 4` AUDIO (Pri 50).
- `N64ModernRuntime`: Agendamento puramente cooperativo; o busy-wait puro em `func_800C2AB8` retém o token de execução e impede que mensagens externas acordem a thread AUDIO.
- **Causa validada (experimento limpo 2026-08-12):** strong override temporário
  de `func_800C2AB8` com `yield_self_1ms(rdram)` → AUDIO roda, `func_800205D0`
  limpa `table[2].word0`, `func_800C269C(7)==1`, `func_800C16A4(7)` dispensa o
  slot 7, `func_800C2AB8` retorna, attract progride. Patch revertido; resolvido
  permanentemente por PATCH 4.

Evidência corrente:

- [[../../plan/evidence/p08-attract-loop-after-demo2-crash]] (**RESOLVED and ACCEPTED** — DEMO2 attract SIGSEGV; Level A, verdict B)
- [[../../plan/evidence/p08-forced-cinematic-facing-diagnosis]] (historical USER_BUG_B one-actor diagnosis; corrected/superseded)
- [[../../plan/evidence/p08-forced-cinematic-facing-independent-review]] (**CURRENT:** original-MIPS case-11 contract; actor refuted, global open)
- [[../../plan/evidence/p08-demo-full-motion-sync-diagnosis]] (**CURRENT:** MEASURED_ORIGINAL vs MEASURED_RECOMP; capture-phase + later run split)
- [[../../plan/evidence/p08-demo-movement-precision-diagnosis]] (gameplay-demo residual; first straight + first analog PASS; original numeric now measured)
- [[../../plan/evidence/p08-stick-angle-thunk-fix]] (implementation evidence; accepted by focused re-review)
- [[../../plan/evidence/p08-stick-angle-thunk-fix-rereview]] (**CURRENT REVIEW** — verdict A; `$r1` delay-slot blocker resolved)
- [[../../plan/evidence/p08-stick-angle-thunk-fix-independent-review]] (historical verdict C; blocker resolved)
- [[../../plan/evidence/p08-player-direction-facing-diagnosis]] (Level A stick-angle diagnosis)
- [[../../plan/evidence/p08-directional-input-movement-diagnosis]] (host→pad bidirectional; not reopened)
- [[../../plan/evidence/p08-render-submission-matrix-fix]] (**CURRENT** — render matrix fix)
- [[../../plan/evidence/p08-render-submission-matrix-diagnosis]] (render matrix diagnosis)
- [[../../plan/evidence/p08-actor-camera-transform-discriminator]] (transform discriminator)
- [[../../plan/evidence/p08-visible-demo-movement-diagnosis]] (visible demo + movement diagnosis)
- [[../../plan/evidence/p08-core1-ra-wrapper-fix]] (core1 RA-wrapper fix; resolved)
- [[../../plan/evidence/p08-80013a7c-lookup-zero-diagnosis]] (Level A diagnosis; resolved)
- [[../../plan/evidence/p08-bcf28-binding-fix-independent-review]] (verdict B; BCF28 fix accepted)
- [[../../plan/evidence/p08-bcf28-binding-fix]] (implementer binding-fix evidence; preserved)
- [[../../plan/evidence/p08-bcf28-crash-diagnosis]] (root cause A diagnosis; preserved)
- [[../../plan/evidence/p08-preemption-patch4-clean-build-independent-review]] (verdict B; PATCH 4 accepted; tracked-state blocker resolved)
- [[../../plan/evidence/p08-preemption-patch4-clean-build-correction]] (implementer correction)
- [[../../plan/evidence/p08-preemption-patch4-independent-review]] (historical verdict C preserved; behavioral CLASS A; finding resolved)
- [[../../plan/evidence/p08-preemption-patch4-validation]] (implementer evidence; preserved)
- [[../../plan/evidence/p08-preemption-patch35-rcp-independent-review]] (verdict B; PATCH35 accepted)
- [[../../plan/evidence/p08-preemption-patch35-rcp-correction]] (implementer TLS RCP correction evidence)
- [[../../plan/evidence/p08-preemption-patch35-independent-review]] (pre-correction verdict C; finding resolved, history preserved)
- [[../../plan/evidence/p08-preemption-patch35-validation]] (implementer evidence; RCP/P3-X6 claims superseded)
- [[../../plan/evidence/p08-preemption-patch35-design]] (PATCH 3.5 design contract; implemented)
- [[../../plan/evidence/p08-preemption-patch35-thread-switch-review]] (thread-dispatch resume hook)
- [[../../plan/evidence/p08-preemption-patch3-independent-review]] (PATCH 3 review B)
- [[../../plan/evidence/p08-preemption-patch3-validation]] (PATCH 3 generator; T11 PASS; flag OFF)
- [[../../plan/evidence/p08-preemption-patch2-independent-review]] (PATCH 2 review B; inherited pre-PATCH4 blockers)
- [[../../plan/evidence/p08-preemption-patch2-validation]] (PATCH 2 runtime service)
- [[../../plan/evidence/p08-preemption-patch1-independent-review]] (PATCH 1 review B)
- [[../../plan/evidence/p08-preemption-patch1-validation]] (PATCH 1 implementer evidence)
- [[../../plan/evidence/p08-preemption-implementation-plan]] (spec Hybrid F'; PATCH 2 landed, PATCH 3+ pending)
- [[../../plan/evidence/p08-preemption-design-adversarial-review]] (arquitetura F' aprovada)
- [[../../plan/evidence/p08-permanent-preemption-design]] (DeepSeek Hybrid F, `CORRECTED/SUPERSEDED`)
- [[../../plan/evidence/p08-guest-preemption-causal-validation]]
- [[../../plan/evidence/p08-guest-preemption-runtime-audit]]
- [[../../plan/evidence/p08-slot7-scheduled-event-validation]]
- [[../../plan/evidence/p08-slot7-scheduled-event-driver]]
- [[../../plan/evidence/p08-slot7-object-lifecycle-arbitration]]
- [[../../plan/evidence/p08-session-handoff]]
- [[../../plan/evidence/p08-original-timing-4x3-baseline]] (**CURRENT:** P08-T03 baseline; blocked pending clean live measurements)
- [[../../plan/evidence/p08-attract-actor-loop-diagnosis]]
- [[../../plan/evidence/p08-type4-transition-semantics-audit]]

> **VALIDAÇÃO DINÂMICA CLEAN FREEZE (2026-08-12):** 3 clean freezes
> determinísticos confirmam **CLASS A**: o objeto de `table[2]` está agendado na
> delta-queue do SoundManager (head, delta 0, payload.p4 == table[2].word0),
> handler `mgr+0x08 = func_8001ED68` (MATCH), `func_8001EE34` pending; a thread
> `[Game] AUDIO` está estacionada em `LightweightSemaphore::waitWithPartialSpinning`
> e os contadores de áudio ficam congelados no freeze → o dispatch não pode rodar
> enquanto MAIN gira em `func_800C2AB8`. Ver
> [[../../plan/evidence/p08-slot7-scheduled-event-validation]].


## Tarefas

- [x] `P08-T01` Integrar RT64 e validar Vulkan nesta máquina.
- [x] `P08-T02` Catalogar display lists/efeitos Tooie; scheduler DP/VI OK ([[../../plan/evidence/p08-display-effect-catalog]]).
- [x] `P08-T03` Reproduzir 4:3 e timing originais sem melhorias ativas
  ([[../../plan/evidence/p08-original-timing-4x3-baseline]]). 60Hz VI, 20/30Hz logic,
  30Hz animação, desacoplamento do host gfx e aspecto 4:3 validados ao vivo.
- [x] `P08-T04` Integrar path SDL/RecompFrontend do BanjoRecomp.
- [x] `P08-T05` Defaults/persistência BanjoRecomp; `controls.json` lazy.
- [x] `P08-T06` Esconder launcher no auto-start e título Tooie.
- [x] `P08-T07` Janela, fullscreen, foco, multi-monitor e escalas (`tooie_window` helpers + SDL/RT64 path; manual multi-monitor/fullscreen validation pending).
- [x] `P08-T08` Fallback/mensagem para driver/API incompatível (`tooie_error` reporting & negative tests PASS).
- [x] `P08-T09` Evidência visual: intro/Jiggy OK; **first demo visible & upright**
  (render submission matrix split resolved via `transform_hooks.cpp`).
- [x] `P08-T10` Input title/file select VALIDATED
  ([[../../plan/evidence/p08-title-file-select-validation]]).
  `P08_T10_FRONTEND_INPUT=PASS`. Direct Spiral Mountain claim
  **CORRECTED/SUPERSEDED**. Current synthetic `mode=1` FAIL
  **REFUTED**.
- [x] `P08-T11` Máquina de estados da abertura: type-4+warp OK; **C2AB8
  RESOLVED by PATCH 4** (CLASS A ON/OFF). Hybrid F' production ON for
  Tooie. **`func_800BCF28` RESOLVED and accepted** (verdict B).
  **`func_80013A7C` core1 RA wrappers RESOLVED**.
  **Render-submission matrix split RESOLVED and VALIDATED** ([[../../plan/evidence/p08-render-submission-matrix-fix]]).


## Implementado / validado

### Frontend, render e áudio

- RT64 + Vulkan, auto-start, janela “Banjo-Tooie: Recompiled”.
- Música inicial validada.
- Input host equivalente ao BanjoRecomp. **P08-T10 VALIDATED / COMPLETE**
  for frontend input: intro -> file select -> slot navigation -> `CONT_A`
  confirm PASS. Post-confirm destination is `MAP_A1_CS_INTRO` (161) /
  `mode=3`. Direct Spiral Mountain and later synthetic `mode=1` FAIL
  claims are **CORRECTED/SUPERSEDED** / **REFUTED**
  ([[../../plan/evidence/p08-title-file-select-validation]]).
- P08-T03 source contract and 4:3 guest dimensions are characterized, but the
  clean runtime baseline is **BLOCKED** by the active Expand/Clamp16x9 config
  and unavailable live display; [[../../plan/evidence/p08-original-timing-4x3-baseline]].

### Blockers históricos fechados

- data relocation overlays / `LOOKUP_FUNC(0)`;
- actor manager / D6E54 false lead;
- resident overlay / gcsection;
- scheduler queue / `No threads left` (sem nova prova);
- IPL3 integrity gate;
- R_MIPS_26 / do_dma;
- type-4 incomplete e warp não consumido (clean freeze);
- `func_80026500` como PC do hang;
- `func_800A907C → func_8001EE34` como driver do objeto (refutado);
- `func_800BCF28` binding / null dereference (resolvido e aceito, verdict B:
  [[../../plan/evidence/p08-bcf28-binding-fix-independent-review]]);
- `func_80013A7C` LOOKUP_FUNC(0) / core1 RA wrappers (resolvido:
  [[../../plan/evidence/p08-core1-ra-wrapper-fix]]).

## Evidências P08

| evidência | estado/finalidade |
| --- | --- |
| [[../../plan/evidence/p08-attract-loop-after-demo2-crash]] | **RESOLVED and ACCEPTED:** DEMO2 attract SIGSEGV; bsrest 0x808003FC resolved via CreateStatic (Level A, verdict B) |
| [[../../plan/evidence/p08-forced-cinematic-facing-diagnosis]] | historical USER_BUG_B one-actor diagnosis; corrected/superseded |
| [[../../plan/evidence/p08-forced-cinematic-facing-independent-review]] | **CURRENT:** original-MIPS case-11 contract; chdiggerground actor REFUTED; global USER_BUG_B OPEN |
| [[../../plan/evidence/p08-demo-movement-precision-diagnosis]] | first straight + first analog PASS; first demo fire-egg glidx 322 target collision volume resolved (TRUE_MISS_MARGIN = +36.85f, PROVEN NO HIT) |
| [[../../plan/evidence/p08-stick-angle-thunk-fix]] | implementation evidence; accepted by focused re-review |
| [[../../plan/evidence/p08-stick-angle-thunk-fix-independent-review]] | **CURRENT REVIEW:** verdict C; missing `$r1` delay-slot side effect |
| [[../../plan/evidence/p08-player-direction-facing-diagnosis]] | Level A empty 13B70 diagnosis; fix implemented |
| [[../../plan/evidence/p08-directional-input-movement-diagnosis]] | host→pad bidirectional; not reopened |
| [[../../plan/evidence/p08-render-submission-matrix-fix]] | **CURRENT:** Tooie render-submission matrix split validated |
| [[../../plan/evidence/p08-render-submission-matrix-diagnosis]] | RT64 LOAD-path viewproj decompose (resolved by the fix) |
| [[../../plan/evidence/p08-visible-demo-movement-diagnosis]] | visible demo reproduced; movement diagnosis |
| [[../../plan/evidence/p08-core1-ra-wrapper-fix]] | core1 RA-wrapper native fix (resolved) |
| [[../../plan/evidence/p08-80013a7c-lookup-zero-diagnosis]] | Level A diagnosis; resolved by the fix |
| [[../../plan/evidence/p08-bcf28-binding-fix-independent-review]] | verdict B; BCF28 helper binding fix accepted |
| [[../../plan/evidence/p08-bcf28-binding-fix]] | implementer binding-fix evidence (preserved) |
| [[../../plan/evidence/p08-bcf28-crash-diagnosis]] | root cause A diagnosis (preserved) |
| [[../../plan/evidence/p08-preemption-patch4-clean-build-independent-review]] | verdict B; PATCH 4 accepted; tracked-state blocker resolved |
| [[../../plan/evidence/p08-preemption-patch4-independent-review]] | historical verdict C preserved; CLASS A behavior; finding resolved by clean-build review |
| [[../../plan/evidence/p08-preemption-patch4-validation]] | implementer evidence; history preserved |
| [[../../plan/evidence/p08-preemption-patch35-independent-review]] | historical verdict C; RCP finding resolved |
| [[../../plan/evidence/p08-preemption-patch35-validation]] | implementer evidence; PATCH4 gate superseded |
| [[../../plan/evidence/p08-preemption-patch35-design]] | PATCH 3.5 design contract (implemented) |
| [[../../plan/evidence/p08-preemption-patch35-thread-switch-review]] | thread-switch eligibility; verdict B; resume hook |
| [[../../plan/evidence/p08-preemption-patch3-independent-review]] | PATCH 3 independent review B |
| [[../../plan/evidence/p08-preemption-patch3-validation]] | PATCH 3 generator; T11 PASS; flag OFF |
| [[../../plan/evidence/p08-preemption-patch2-independent-review]] | PATCH 2 independent review B; pre-PATCH4 blockers |
| [[../../plan/evidence/p08-preemption-patch2-validation]] | PATCH 2 runtime service |
| [[../../plan/evidence/p08-preemption-patch1-independent-review]] | PATCH 1 independent review B |
| [[../../plan/evidence/p08-preemption-patch1-validation]] | PATCH 1 scaffold + T01 PASS |
| [[../../plan/evidence/p08-preemption-implementation-plan]] | spec file/function + T01–T15; PATCH 2 landed |
| [[../../plan/evidence/p08-preemption-design-adversarial-review]] | arquitetura F' aprovada (veredito C) |
| [[../../plan/evidence/p08-permanent-preemption-design]] | DeepSeek Hybrid F; `CORRECTED/SUPERSEDED` |
| [[../../plan/evidence/p08-guest-preemption-causal-validation]] | experimento limpo class A — gap de preempção provado causalmente |
| [[../../plan/evidence/p08-guest-preemption-runtime-audit]] | auditoria estrutural do gap (Gemini) |
| [[../../plan/evidence/p08-slot7-scheduled-event-validation]] | validação dinâmica class A (evento agendado+due, dispatch AUDIO estacionado) |
| [[../../plan/evidence/p08-slot7-scheduled-event-driver]] | reconstrução estática do scheduler/driver |
| [[../../plan/evidence/p08-slot7-object-lifecycle-arbitration]] | arbitragem Gemini×DeepSeek; struct/driver/order |
| [[../../plan/evidence/p08-session-handoff]] | **CURRENT:** handoff para chat zerado |
| [[../../plan/evidence/p08-original-timing-4x3-baseline]] | **CURRENT:** P08-T03 original timing/4:3 baseline; BLOCKED pending clean live measurements |
| [[../../plan/evidence/p08-display-effect-catalog]] | **CURRENT:** P08-T02 catálogo de display lists e efeitos gráficos |
| [[../../plan/evidence/p08-demo-full-motion-sync-diagnosis]] | **CURRENT:** diagnóstico diferencial de sincronização e movimento da demo de gameplay |
| [[../../plan/evidence/p08-attract-actor-loop-diagnosis]] | freeze mechanism slot7 / C2AB8 |
| [[../../plan/evidence/p08-type4-transition-semantics-audit]] | type-4 complete, warp consumido |
| [[../../plan/evidence/p08-intro-to-demo-transition-diagnosis]] | timeline histórica |
| [[../../plan/evidence/p08-visual-validation/README]] | frames humanos |
| demais p08-* | resolved/deferred (ver handoff) |

## Fluxo esperado

```text
intro -> Jiggy -> gsattract -> map 335/entrance 8 -> type-4 complete
 -> warp 3 -> A9924 -> A76F4(5) -> mode 5 -> A9A14 -> gsattract ep1
 -> gameplay demo
```

## P08 unnamed same-section JAL fix — independent acceptance (2026-08-16)

Independent review result: `FIX_VERDICT=B`, `FIX_ACCEPTED=YES`, and the old
DEMO2/CCL SIGSEGV blocker is closed. The B classification records the test-only
hardening and the fact that this host could not reproduce the transition runtime
under Xvfb/lavapipe; it does not identify a generator blocker.

Verified:

- exact base `1f2aba14665c064ce5af829955f1ac089792de77`, patch base/hash contract,
  and active durable ownership in `patches/runtime/n64recomp-tooie.patch`;
- `resolve_jal` policy cases preserved: named same-section direct, unnamed
  same-section `CreateStatic`, cross-section lookup semantics, and non-relocatable
  host-hook lookup semantics;
- all seven section-344 CreateStatic boundaries, original-MIPS helper semantics,
  17/17 direct call-site mappings, 7/7 generated bodies, and zero vulnerable
  lookups after clean regeneration;
- live generator unit test 4/4 cases, all five native tests, and `TooieRecompiled`
  build: PASS;
- regression test strengthened only: exact 17-call mapping, exact section-344
  helper definitions, and zero wrong/missing/lookup calls are now required.

The independent 30-second software-Vulkan/Xvfb run reached `T_INTRO`, emitted no
SIGSEGV, and timed out before CCL. The Radeon/Xvfb path was unavailable because
of missing DRI3 presentation. The documented 300-second Vulkan soak remains
preserved as historical implementation evidence.

```text
ROOT_CAUSE_LEVEL:                           A
FIX_OWNER:                                  N64RECOMP
FIX_IMPLEMENTED:                            YES
FIX_ACCEPTED:                               YES
ATTRACT_DEMO2_CRASH:                        RESOLVED
CREATESTATIC_BOUNDARIES:                    7/7 PASS
AUDITED_DIRECT_CALLS:                       17/17
VULNERABLE_LOOKUPS:                         0
MARKDOWN_VAULT_AUDIT:                       PASS (0 errors; pre-existing warnings/tasks)
NEXT_SINGLE_ACTION:                         return to the next P08 residual
```

## Estado observado (clean)

```text
... warp consumido, A76F4(5) ...
 -> C2AB8 returns (PATCH 4 Hybrid F')
 -> gsworldDll / basetup
 -> func_800BCF28 completes cleanly
 -> func_80013A7C native body entered; old LOOKUP_FUNC(0) absent
 -> 150s continuation at opening mode=3 / map=0 (not diagnosed)
```

## Residual prioritário

1. **USER_BUG_B `chdiggerground` actor — REFUTED (original-MIPS review,
   2026-08-16)** ([[../../plan/evidence/p08-forced-cinematic-facing-independent-review]]).
   The original case-11 return path advances position while preserving
   `Actor.rotation.y` / `unk54`; no fix is authorized. `USER_BUG_B_GLOBAL`
   remains OPEN for another user-confirmed cinematic actor.
2. **Demo-path first straight and first later analog transition — PASS**
   ([[../../plan/evidence/p08-demo-movement-precision-diagnosis]] §14, §15).
   Later demo-path visual residual:
   `DEFERRED_PENDING_SPECIFIC_VISUAL_SEGMENT`.
3. **Analog/gamepad validation of USER_BUG_A** — WASD runtime is RESOLVED;
   full controller is PENDING.
4. **Fix file-select → Spiral Mountain transition** — mode=1 not reached on
   current HEAD (regression from P08-T10 validated state).
5. Generic non-RA JR safepoints, Timer/PI approximations — deferred.
6. Origem do objeto `table[2]` (intro/attract/handoff) — permanece UNKNOWN;
   PATCH 4 direct word-zero label is INFERENCE.

## Gate de saída

- [x] Abertura até Jiggy/type-4/warp; C2AB8 behavior CLASS A; clean tracked
  Clang build PASS; PATCH 4 distribution independently accepted (verdict B).
- [x] `func_800BCF28` binding fix implementado, validado e aceito (verdict B).
- [x] `func_80013A7C` lookup-zero resolvido (Tooie-native RA wrappers).
- [x] RT64 intro OK; first demo visible; Wooded Hollow render-submission fix validated.
- [x] mode 5 / gsattract ep1 / gameplay demo renderizam (upright after matrix split).
- [x] Entrada title/file select validada; rumble/hotplug remains separate.
- [x] P08-T03 original timing/4:3 baseline validado e desacoplado da janela.
- [x] P08-T07 window, fullscreen, focus, multi-monitor, scale/DPI helpers implemented (`tests/window_display_test.cpp`, 38/38 PASS; manual physical display validation pending).
- [x] P08-T08 graphics driver / Vulkan fallback error handling validado (`tests/graphics_failure_test.cpp`, 25/25 PASS; negative env test PASS).
- [x] Persistência BanjoRecomp; `controls.json` lazy.
- [x] Sem melhorias que alterem timing/paridade de referência.

## Próxima etapa

**P08 remains ACTIVE (MANUAL_VALIDATION_PENDING under Verdict B).**
- **Implemented Scope**: RT64 presentation baseline, input mapping, window/scale integration, graphics/Vulkan error handling, launcher music preview, static call correctness.
- **Pending Before P09 Formal Open**: Physical gamepad hardware verification and live multi-monitor physical display verification.
- **P08 / P09 Invariants (Protected)**: attract DEMO2 crash, bsrest 0x808003FC, N64Recomp CreateStatic fix, first-straight ~510, fire-egg miss, cinematic facing, stick-angle 13B70, render-submission matrix, BCF28, core1 RA wrappers, PATCH 4 / preemption, P08-T10, original timing/4:3 baseline.

## P08 Implementation Status (2026-08-16, Gemini 3.7 Flash HIGH)

- **P08-T07**: Window sanitization, aspect ratio contracts, and display validation helpers implemented via `tooie_window` module and verified via `tests/window_display_test.cpp` (38/38 PASS). Fullscreen/focus/event handling verified on reference SDL/RT64 path.
- **P08-T08**: Graphics driver, Vulkan, and window initialization error handling validated via `tooie_error` module, `tests/graphics_failure_test.cpp` (25/25 PASS), and negative environment tests (`VK_ICD_FILENAMES=/nonexistent.json` exits code 1 with clean diagnostics and no crash).
- **Canonical Tasks**: All 11 canonical tasks audited. T01, T03..T06, T08..T11 DONE; T02 PARTIAL; T07 IMPLEMENTED_MANUAL_VALIDATION_PENDING.
- **Evidence Reference**: [[../../plan/evidence/p08-t07-t08-frontend-system-validation]].

## P08 measured post-egg movement discriminator (2026-08-17)

The current measured-original/recomp differential isolates a Level C residual,
without production changes: `ideal_yaw` first differs at glidx 385 and position
first exceeds 2 units at glidx 392 under `ORIGINAL[N] vs RECOMP[N+1]`. Pads are
`765/765`, `bs=21`, `tspd=700`, and slope multiplier `1.0000` match at the
persistent split. The exact writer PC is UNKNOWN; P08 stays active and P09 is
not started. See [[../../plan/evidence/p08-demo-post-egg-motion-divergence]].
