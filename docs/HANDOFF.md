---
title: "Handoff — TooieRecomp"
description: "Estado global corrente do port, com prioridade P08 no handoff intro para attract demo."
status: current
type: handoff
project: TooieRecomp
updated: 2026-08-12
tags: [tooie-recomp, handoff, p08, intro, attract, type-4, clean-freeze]
---

# Handoff — TooieRecomp

> **CORREÇÃO (2026-08-12):** o clean-freeze type-4 snapshot (8/8 runs naturais,
> binário original, attach só pós-freeze) REFUTOU a narrativa de “type-4
> incompleta com warp gated”. A type-4 completa (`state 2`, `render 2`), o
> `warp=3` é consumido e a transição mode-5 é alcançada. O blocker corrente é
> downstream, na inicialização do mode-5/attract (`func_800A76F4 →
> func_800A7380 → func_800C2AB8`). Ver
> [[plan/evidence/p08-type4-transition-semantics-audit#clean-freeze-exact-type-4-snapshot]].

## Prioridade atual

**P08 ACTIVE BLOCKER:** a primeira attract/gameplay demo não começa depois da
intro, Jiggy, attract request e transition type-4 completa.

Estado clean-freeze corrente (8/8 runs determinísticos):

- request `map=335 / entrance=8 / warp=3` confirmado (`D_80127760` retém
  `map=335`, `entrance=8`, `kind=2`);
- transition type-4 **completa** (`current_state=2`, `render_frame_count=2`,
  gate do warp ABERTO), `+0x0C target=0.9`, `+0x10 cfg_sfx=2883`,
  `+0x18 cfg_param3=3.5`, `+0x28 elapsed=0` (reset pós-completion);
- `warp=3` consumido; `func_800A9924` e `func_800A76F4(5)` alcançados
  (mode machine `+0x1C=5`);
- MAIN capturada downstream: `func_800A76F4 -> func_800A7380 -> func_800C2AB8`;
- black freeze durante a inicialização do mode-5/attract;
- frame-sync/VI, event-3, cothemedll, scheduler, type-4 timing/progress stall e
  warp consumption: exonerados/refutados.

Descrição canônica:

> The opening/intro sequence completes visually through the Jiggy transition
> and the attract map request; the type-4 transition completes and warp 3 is
> consumed, but the subsequent gameplay demo/attract scene does not start — a
> black freeze occurs downstream during mode-5 / attract initialization.

Handoff técnico autossuficiente:
[[plan/evidence/p08-session-handoff]]. Evidência de correção:
[[plan/evidence/p08-type4-transition-semantics-audit]].

## NEXT

Confirmar o PC/callback/slot de actor exato da MAIN dentro de `func_800C2AB8`
no clean freeze, com inspeção somente pós-freeze, e determinar se um callback
falha em retornar ou se o actor loop falha em avançar. Não investigar BaKey
ainda (possible downstream blocker de runs perturbados).

## Estado do produto validado

1. O launcher/RecompFrontend, auto-start e RT64/Vulkan apresentam a janela do
   jogo; input do launcher funciona.
2. A música inicial toca corretamente no sink default do SO.
3. A intro renderiza céu/brilho, cena 3D, Dolby Surround e logo BANJO-TOOIE;
   cenário/câmera continuam avançando.
4. A Jiggy transition fecha a intro corretamente.
5. O attract request (`map=335`, `entrance=8`, `kind=2`) e a type-4 completa; o
   warp=3 é consumido; a transição mode-5 é alcançada.
6. Depois disso, a tela fica preta e a primeira gameplay demo não aparece
   (freeze downstream na inicialização do attract).
7. Title/file-select input continua diferido; a tentativa EEPROM anterior
   falhou validação humana.

A captura isolada `0060` não prova o instante exato do fim da Jiggy; a conclusão
vem da validação humana do fluxo mais os estados observados. Ver
[[plan/evidence/p08-visual-validation/README]].

## Arquitetura e regras

TooieRecomp recompila estaticamente Banjo-Tooie com:

- `N64Recomp` / `RSPRecomp`;
- `N64ModernRuntime`;
- `RT64`;
- `RecompFrontend`.

`RecompiledFuncs` é código gerado e não deve ser editado. Fixes devem estar na
camada proprietária da causa — generator, metadata, runtime ou integração
source/host — e precisam ser versionados/reproduzíveis.

Repo Git: `TooieRecomp/`, branch `main`. A raiz do workspace
`/home/wesley/projetos/N64-Recomp` não é um repositório Git.

## Resolved blockers / do not reopen

### Overlay data relocation / `LOOKUP_FUNC(0)`

Generator de metadata chamava `readelf` sem `-W`, truncava nomes longos de
seções relocatable e descartava data relocations. Corrigido; `ActorInfo[0x38]`
agora contém callback correto. Ver
[[plan/evidence/p08-overlay-data-relocation-diagnosis]].

### Actor manager e D6E54

O manager inicializa naturalmente por `chbaddieDll ep1 -> func800FF390 ->
func801065E0 -> vector_new`; `D80136EE0≈0x801DF080`. O nulo veio de spawn
artificial. `func800D6E54` é resource/reclaim diferido, não actor population.
Ver [[plan/evidence/p08-natural-actor-spawn-divergence]].

### Resident overlay cache / gcsection

`ovl_unload` deixava mapping stale em `s_resident_overlays` depois de liberar
heap. A invalidação pós-unload resolveu gcsection/glintrosync. Ver
[[plan/evidence/p08-gcsection-glintrosync-segv-diagnosis]].

### Scheduler antigo

N64ModernRuntime recebeu scan real de duplicata em insert, traversal correto em
remove e guard de reinserção em `osSetThreadPri`. Harness 13/13 PASS e múltiplos
runs sem `No threads left`, self-loop ou corrupção. Ver
[[plan/evidence/p08-no-threads-left-diagnosis]].

### IPL3 integrity gate

Corrigidos sign extension/endereço e seed one-shot dos resíduos esperados
`0xAD090010`/`0xAD170014`; natural actor creation restaurada em 3/3 runs. Ver
[[plan/evidence/p08-natural-actor-spawn-divergence]].

### R_MIPS_26 / flat func_map / do_dma

Same-section JAL conhecido em section relocatable agora vira direct C call no
generator. Fechado o path `chintrochar -> gsworld ep0 espúrio -> idx -0x3AB ->
bad DMA -> do_dma SIGSEGV`. Patch versionado:
`patches/runtime/n64recomp-same-section-jal.patch`. Ver
[[plan/evidence/p08-overlay-rmips26-binding-diagnosis]] e
[[plan/evidence/p08-do-dma-segv-diagnosis]].

## Current expected flow

```text
intro mode 3
 -> Jiggy transition
 -> gsattract ep5
 -> map 335, entrance 8
 -> create/retain type-4
 -> warp 3 pending
 -> func_800A7D84 tick
 -> type-4 completes / func_800C9510 returns 0
 -> func_800A9924 -> func_800A76F4(5)
 -> mode 5 -> func_800A9A14 -> gsattract ep1
 -> gameplay demo
```

## Current clean-freeze evidence

Ground truth usa binário original, sem debugger ou instrumentação durante a
execução:

```sh
TOOIE_AUTO_START=1 \
TOOIE_DIAG=1 \
TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
./build/TooieRecompiled
```

Freeze confirmado (8/8 runs determinísticos): black screen pós-intro, syscall
`d` estável (`178958`..`179320`) por `>=6s`, `last_ovl=635:2`, transition
count 1, type 4. Somente então anexar GDB/capturar snapshot (supervisor pai com
`PTRACE_ATTACH`/`/proc/pid/mem`; decoder Python independente valida os campos).
Não versionar o core de ~6.4 GB.

MAIN snapshot:

```text
OSThread 0x80045788, id 6, priority 20, queue 0

func_800124EC
 -> func_800123F4
 -> func_800123BC
 -> func_800A7D84
 -> func_800A76F4
 -> func_800A7380
 -> func_800C2AB8
 -> get_function(0x800C269C)
```

MAIN não estava bloqueada em MQ. A running queue continha IDLE porque MAIN era
current/running. Self-loop, duplicata e invalid pointer: NO. Scheduler bug:
NOT PROVEN.

Type-4 no clean freeze (corrigido): objeto heap `0x8027AAC0`, count 1,
`current_state=2`, `render_frame_count=2` (gate do warp ABERTO),
`+0x0C target=0.9`, `+0x10 cfg_sfx=2883`, `+0x18 cfg_param3=3.5`,
`+0x28 elapsed=0` (reset pós-completion), `+0x30 counter=1`, `subactor=NULL`.
`effective_dt=0.05` (`0x8012C760=0x3D4CCCCE`, `0x801275DC=1.0`,
`timing_flag=1`); 18 updates × 0.05 = 0.9 explica os runs perturbados.
`FIELD_SEMANTICS_VALIDATED=YES`.

## Exonerated current hypotheses

- **Frame-sync:** recv 57/57, send 57/57, host VI/producer 361/361,
  outstanding 0.
- **Event-3:** call/handler 3/3, ACK send/recv 3/3, outstanding 0, msg5
  dispatch 57.
- **cothemedll:** callback síncrono sem MQ/DMA wait;
  `LAST_OVL_IS_BLOCKING_PC=NO`.
- **Circular type-4 gate:** update continua no restante de `func_800A7D84`
  mesmo quando warp permanece gated.
- **Type-4 timing/progress stall (REFUTADO pelo clean snapshot):** a type-4
  completa no run natural; `elapsed=0` é reset pós-completion, não stall.
- **Warp consumption (REFUTADO):** warp=3 é consumido no clean freeze;
  `D_80127760` retém `map=335`/`entrance=8`/`kind=2`; `func_800A76F4(5)` e
  mode machine `+0x1C=5` confirmados.

## Timing warning

GDB, breakpoints e instrumentação em hot paths alteraram timing. Dados
perturbados servem para provar contratos locais, não para representar o freeze
natural.

```text
run clean -> confirm freeze -> attach after freeze -> capture snapshot/core
```

## Não provado / não concluir

- callback/slot/PC exato dentro de `func_800C2AB8` que não retorna/progride na
  inicialização do mode-5/attract;
- se o hang é um callback específico que falha em retornar ou o próprio actor
  loop que falha em avançar;
- por que este ambiente roda a ~20 Hz (`delta=0.05`);
- BaKey como root cause do clean freeze (possible downstream blocker apenas).

## Files to read

1. [[plan/evidence/p08-session-handoff]]
2. [[plan/evidence/p08-type4-transition-semantics-audit]]
3. [[plan/evidence/p08-intro-to-demo-transition-diagnosis]]
4. [[plan/evidence/p08-visual-validation/README]]
5. [[plans/BT/08-rt64-input-and-frontend]]
6. Evidências resolvidas ligadas na seção “do not reopen”.

## Scope do checkpoint documental

```text
SOURCE_CODE_CHANGED    = NO
RUNTIME_CODE_CHANGED   = NO
GENERATOR_CODE_CHANGED = NO
PATCH_CODE_CHANGED     = NO
CORE_INCLUDED          = NO
BUILD_ARTIFACT_INCLUDED= NO
```
