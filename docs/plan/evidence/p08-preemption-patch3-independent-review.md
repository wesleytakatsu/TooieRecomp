---
title: "P08 — independent review of preemption PATCH 3"
description: "Revisão independente (Grok 4.6 HIGH) do Hybrid F' PATCH 3. Veredito B: aprovar com issues não bloqueantes. PATCH 3 aceite; PATCH 3.5 design next; PATCH4_READY NO."
status: evidence
phase: P08
updated: 2026-08-13
tags: [tooie-recomp, p08, preemption, patch3, independent-review, n64recomp, liverecomp]
---

# P08 — INDEPENDENT REVIEW OF PREEMPTION PATCH 3

```text
MODE:                 READ / TEST / DOCS ONLY
MODEL:                Grok 4.6 HIGH
ROLE:                 Independent senior compiler / runtime reviewer
                      (did not implement PATCH 3)
TOOIE_HEAD:           e94d89a13680f720cd376245e5fceed3746a3eb1
RUNTIME_BASE:         ca568b6ad79b9029d14077f0c3ffa757727c5559
N64RECOMP_BASE:       2b6f05688de2abc7d86da5b4a89b84c2c6acbabe
ARCHITECTURE:         Hybrid F' (not reopened)
VERDICT:              B — APPROVE WITH NONBLOCKING ISSUES
PATCH3_ACCEPTED:      YES
PRE_PATCH4_SEMANTIC_WORK_READY: YES
PATCH4_READY:         NO
SOURCE_CODE_CHANGED:  NO
```

Do not treat inherited PATCH 2 issues (X1 cause/class, MTC0 re-enable) as
PATCH 3 defects. Do not treat missing Tooie regeneration as a PATCH 3 defect.
DeepSeek Hybrid F remains `CORRECTED/SUPERSEDED`. Do not enable the
production flag. Do not implement PATCH 3.5 in this review.

---

## 0. Scope and sources

Reviewed the **versioned artifacts and reconstructed sources**, not the
implementer report as proof.

```text
TOOIE_HEAD:           e94d89a13680f720cd376245e5fceed3746a3eb1
TOOIE_WORKTREE:       clean (before and after this review)
DIRTY_CHECKOUT:       BanjoRecomp/lib/N64ModernRuntime @ ca568b6
                      N64Recomp @ 2b6f056
                      NOT reset
RECONSTRUCTION:       /tmp/p08-p3-review-recon  (independent of /tmp/p08-p3-reconstruction)
```

Mandatory docs were read in canonical order, then discarded as evidence.
Code review used the reconstructed tree after applying the active artifacts
twice, then byte-compared every patched path against the real dirty checkout.

```text
DIRTY_VS_RECON_MATCH: YES
  (all PATCH 3 / JAL / scheduler / preemption runtime files identical)
```

Protected dirty files remain present and were not treated as PATCH 3:

```text
threadqueue.cpp / threads.cpp     scheduler Fix B / NULLPTR (separate artifact)
N64Recomp src/main.cpp            JAL force-lookup (in cumulative n64recomp-tooie.patch)
```

---

## 1. Artifact policy and hashes

```text
RUNTIME_CUMULATIVE_SHA256:
  expected 93e34a43bbc55f9da563ed0cd195719e055375764fcc52fecf80a66f19d779b9
  actual   93e34a43bbc55f9da563ed0cd195719e055375764fcc52fecf80a66f19d779b9
N64RECOMP_CUMULATIVE_SHA256:
  expected da84f056b371f1bf229cae3cabdcead528288a2739aa066653b24803cb0ab712
  actual   da84f056b371f1bf229cae3cabdcead528288a2739aa066653b24803cb0ab712
PATCH_HASHES_VALID: YES
```

`tools/apply_runtime_patches.sh` applies exactly three active patches:

1. `n64modernruntime-scheduler.patch` (protected Fix B / NULLPTR)
2. `n64modernruntime-preemption.patch` (PATCH 1+2+3 C ABI bridge + live callbacks)
3. `n64recomp-tooie.patch` (same-section JAL + PATCH 3 generator)

`n64recomp-same-section-jal.patch` is historical / inactive. Script comments
and apply list agree.

```text
ONE_ACTIVE_N64RECOMP_CUMULATIVE: YES
ONE_ACTIVE_RUNTIME_PREEMPTION:   YES
SCHEDULER_PATCH_SEPARATE:        YES
ARTIFACT_POLICY_VALID:           YES
```

### Path classification — `n64recomp-tooie.patch`

| path | class |
| --- | --- |
| `src/recompilation.cpp` | EXPECTED (JAL + shared request sites) |
| `src/cgenerator.cpp` | EXPECTED |
| `LiveRecomp/live_generator.cpp` | EXPECTED |
| `include/recompiler/generator.h` | EXPECTED |
| `include/recompiler/live_recompiler.h` | EXPECTED |
| `include/recomp.h` | EXPECTED |
| `src/main.cpp` | EXPECTED (pre-existing JAL policy, versioned together) |
| `CMakeLists.txt` | EXPECTED (T11 target) |
| `LiveRecomp/preemption_safepoint_test.cpp` | EXPECTED |
| `LiveRecomp/live_recompiler_test.cpp` | EXPECTED (callback wiring) |

### Path classification — `n64modernruntime-preemption.patch`

| path | class |
| --- | --- |
| `ultramodern/src/preemption.cpp` | EXPECTED (PATCH 2 + C `void*` bridge) |
| `ultramodern/include/ultramodern/ultramodern.hpp` | EXPECTED |
| `librecomp/src/mods.cpp` | EXPECTED (PATCH 3 live callbacks) |
| `librecomp/src/recomp.cpp` | EXPECTED (inherited PATCH 2; not a new X1/MTC0 fix) |
| `librecomp/src/ultra_translation.cpp` | EXPECTED (inherited PATCH 2) |
| `ultramodern/src/intmask.cpp` | EXPECTED (inherited PATCH 2) |
| `ultramodern/src/mesgqueue.cpp` | EXPECTED (inherited PATCH 2) |
| `ultramodern/CMakeLists.txt` | EXPECTED |
| `ultramodern/test/preemption_harness.cpp` | EXPECTED (T11-F) |

```text
RECOMPILEDFUNCS_IN_PATCH:     NO
GAME_SOURCE_IN_PATCH:         NO
X1_OR_MTC0_SEMANTIC_FIX_MIXED: NO
JAL_POLICY_PRESERVED:         YES  (semantic compare vs n64recomp-same-section-jal.patch)
```

The JAL hunks in `n64recomp-tooie.patch` match the inactive historical JAL
patch: relocatable same-section generated target → direct `Match`;
`use_lookup_for_all_function_calls` still early-returns `Ambiguous` after
that special case; reimplemented/ignored/stubbed stay on `LOOKUP_FUNC`;
non-reloc core lookup is unchanged. No dedicated JAL binary was re-run
this session; the proof is source identity with the already-proven patch.

---

## 2. Clean reconstruction

Independent disposable tree `/tmp/p08-p3-review-recon` from exact bases.
Did **not** reuse the implementer's `/tmp/p08-p3-reconstruction`.

```text
APPLY PASS 1:  all three patches applied
APPLY PASS 2:  all three "already applied"
CLEAN_RECONSTRUCTION:    PASS
PATCH_SCRIPT_IDEMPOTENT: YES
N64RECOMP_BUILD:         PASS  (ninja PreemptionSafepointTest LiveRecompTest)
PATCH2_HARNESS:          PASS  (pass=21 fail=0 skipped=0)
T11:                     PASS  (pass=39 fail=0)
```

Harness was built from `ultramodern/` directly (`ULTRAMODERN_BUILD_TESTS=ON`).
Top-level `N64ModernRuntime` cmake in a fresh clone fails without the
`thirdparty/miniz` submodule; that is a reconstruction convenience issue,
not a PATCH 3 defect. The versioned ultramodern target builds and runs.

---

## 3. ABI

```c
bool ultramodern_preemption_hint(void);
void ultramodern_service_preemption(uint8_t* rdram, void* status_reg);
```

Static emit: `(void*)&ctx->status_reg`.
Live emit: `ctx + offsetof(recomp_context, status_reg)`.
C bridge in `preemption.cpp` casts `void*` → `uint32_t*` and forwards.
No `recomp_context` layout in ultramodern.

```text
STATIC_ABI_MATCH:          YES
LIVE_ABI_MATCH:            YES
HEADER_COEXISTENCE:        YES
C_LINKAGE_MATCH:           YES
STATUS_POINTER_IDENTITY:   YES   (live T11 + P3-X1: last_status == &ctx.status_reg)
STATIC_C_PORTABILITY:      PASS
```

Disposable P3-X7 compiled the generated call shape as C11 and C++20, and
included `recomp.h` from both C11 and C++20. `stdbool.h` is the right C
bridge; C++ uses the language `bool` under `extern "C"`.

`recomp_context.status_reg` is `uint32_t` after `uint32_t* f_odd`. Offset
is computed, not hardcoded. No 32/64-bit layout constant in the live
generator. Alignment is the natural alignment of `uint32_t` after a
pointer.

C vs C++ `bool` size: both 1 on this toolchain. The live path AND-masks
the 32-bit return with 1, so a C `_Bool` that only defines AL/W0 bit 0
is well-defined.

---

## 4. Live SLJIT call ABI

Actual sequence in `LiveGenerator::emit_preemption_safepoint`:

```text
icall hint          SLJIT_CALL  SLJIT_ARGS0(32)
AND32 R0, R0, 1
cmp32 R0 == 0  -> skip
R0 = rdram + rdram_offset
R1 = ctx + offsetof(status_reg)
icall service       SLJIT_CALL  SLJIT_ARGS2V(P, P)
skip:
```

`emit_function_start` uses `sljit_emit_enter(..., 4 scratches, 5 saved)`.
Saved set is `S0..S4` = rdram, ctx, c1cs, hi, lo. Those survive `SLJIT_CALL`.
`R0..R3` are temps; they are dead at a safepoint (delay slot already
executed; `emit_switch` reloads the switch register from `ctx` afterwards).

`SLJIT_CALL` is the same C convention used by `pause_self`, `trigger_event`,
and hooks. Linux x64 is **PROVEN** by T11 + disposable live tests.

```text
LIVE_ICALL_ABI:  CONDITIONAL
PLATFORM_RISK:   Linux x64 PROVEN.
                 Windows x64 / ARM64 not independently executed this review.
                 They inherit the same SLJIT_CALL family already used by
                 pause_self. AND32-1 is conservative for bool. Residual
                 risk is an sljit/platform bool-return quirk, not a Tooie
                 Linux-x64 defect.
```

---

## 5. Context safety

### Static

Guest GPRs/FPU/`status_reg` live in `recomp_context`. `hi`/`lo`/`result`/`c1cs`
are C locals in the generated function. A C call is an ABI spill; parking the
native thread preserves that C stack.

### Live

GPRs/FPU/`status_reg` live in `ctx`. `hi`/`lo`/`c1cs` live in sljit saved
regs. Same park + C-call ABI.

### What park does *not* automatically save

Another guest thread can mutate shared RDRAM, overlay maps,
`local_section_addresses`, and function tables while this native thread is
parked. That is the cooperative-token model, not a PATCH 3 introduction.

```text
A. guest architectural registers:          YES (ctx + saved/locals)
B. native local lifetime:                  YES (C stack / sljit saved)
C. pointers into mutable shared structures: CONDITIONAL
   (rdram base and this thread's ctx are stable; overlay maps and
    LOOKUP_FUNC tables are not frozen across a swap)
D. runtime reentrancy:                     CONDITIONAL
   (service may swap; inherited PATCH 2 protocol, flag currently OFF)
```

```text
STATIC_CONTEXT_SAFE: CONDITIONAL
LIVE_CONTEXT_SAFE:   CONDITIONAL
```

Missing proof for YES: a service callback that actually swaps guest threads
and then resumes the parked live/static function with locals and switch
addend intact (P3-X6). Not a PATCH 3 blocker while the production flag is
OFF. Required before claiming generic preemption after PATCH 4.

---

## 6. Delay-slot placement

Shared request sites, all after `process_delay_slot` except self `j`/`b`:

| family | delay then safepoint? |
| --- | --- |
| `j` / `b` intra-function, not self | YES (`print_goto_with_delay_slot`) |
| `j` / `b` self | N/A — `pause_self` only; delay slot **not** executed (pre-existing) |
| `beq`/`bne`/`blez`/`bgtz`/`bltz`/`bgez` | YES (`print_branch`) |
| `beql`/`bnel`/`blezl`/`bgtzl`/`bltzl`/`bgezl`/`bc1fl`/`bc1tl` | YES, inside taken block |
| `bgezal`/`bltzal`/`bgezall`/`bltzall` | N/A — function call, no backedge safepoint |
| `jal` / `jalr` | N/A — native call, no backedge safepoint |
| jump-table `jr` | YES, after delay, before `emit_switch` |
| `jr ra` | N/A — return |
| generic `jr` | N/A — compiled as tail-call (see §11) |

```text
DELAY_SLOT_PLACEMENT: PASS
```

Self-`j` skipping the delay slot is a pre-existing N64Recomp approximation,
not introduced by PATCH 3.

---

## 7. Branch-likely — high priority

```text
BRANCH_LIKELY_SUPPORTED:       YES
PATCH3_BRANCH_LIKELY_CORRECT:  YES
```

`N64Recomp/src/operations.cpp` maps all R4300 likely families
(`beql`, `bnel`, `blezl`, `bgtzl`, `bltzl`, `bgezl`, `bltzall`, `bgezall`,
`bc1fl`, `bc1tl`) with `likely=true`. `recompile_function_impl` emits
`goto skip_N` around the standalone delay-slot copy when
`in_likely_delay_slot`.

PATCH 3 inserts the safepoint **inside** `print_branch`, after
`process_delay_slot`, which is itself inside the taken-condition block.
Not-taken likely does not enter that block, so it does not execute the
delay slot and does not hit a safepoint.

This is not a toy-only claim. Production `func_800C2AB8` already contains
the outer hang backedge as **`bnel $s3, $zero, L_800C2AF4`** at
`0x800C2B50`. After a future regen, that taken likely path is exactly
the PATCH 3 safepoint.

Disposable P3-X1/X2 (not committed):

```text
BNEL backward, a0 starts at 3, delay addiu t0, 0x1234:
  static: hint is inside if, after 0x1234, before goto; skip_ label after
  live:   hint=2 service=2 last_status=&ctx.status_reg
          t0 == 0x1234*2   (final not-taken annulled the delay)
BEQL not taken:
  static: no hint
  live:   hint=0 service=0 t0==0
```

```text
class: PROVEN
```

---

## 8. Backedge coverage

Rule: `target_vram <= instr_vram` on direct transfers; always before
`emit_switch`.

| transfer | coverage |
| --- | --- |
| DIRECT J / B intra-function | covered |
| CONDITIONAL BRANCH (incl. likely) | covered |
| SELF LOOP `j`/`b` | `pause_self` (not a safepoint) |
| JAL / JALR recursion | not-required by the approved min architecture |
| RETURN JR RA | not a backedge |
| JUMP TABLE | pre-switch covered |
| GENERIC INDIRECT JR | **not covered** — compiled as tail call |

```text
DIRECT_BACKEDGE_COVERAGE: covered
```

---

## 9. Generic indirect JR — high priority

N64Recomp does **not** reject a `jr` whose target is not `$ra` and not a
recognized jump table. It prints `[Info] Indirect tail call` and emits:

```c
/* delay slot */
LOOKUP_FUNC(ctx->rN)(rdram, ctx);
return;
```

Disposable P3-X3 (not committed) generated exactly that for:

```text
lui  t0, 0x8000
ori  t0, t0, 0x1000     # t0 = own function
jr   t0
addiu a0, a0, 1
```

No safepoint. A computed intra-function loop or a `jr` that tail-calls the
same function therefore becomes native recursion on the **host C stack**,
with no backedge, no pre-switch, and no `pause_self`.

Concrete infinite form:

```text
loop:
    /* no intra-function backward j/b */
    lui  t0, %hi(loop)
    ori  t0, t0, %lo(loop)
    jr   t0
    nop
```

Compilers usually emit `j`/`b` for this, so it is uncommon in matching
MIPS, but N64Recomp **accepts** it.

```text
GENERIC_INDIRECT_JR_GAP:           YES
PATCH3_CONTROL_FLOW_COVERAGE:      INCOMPLETE
class:                             PROVEN (generator output)
severity_before_PATCH4:            nonblocking for PATCH 3 compiler
                                   capability; must be named in PATCH 3.5
                                   if the permanent claim stays "generic
                                   preemption". Do not silently reopen
                                   backedge+switch.
```

---

## 10. Cross-function cycles / recursion

`jal` / `jalr` / linking `bgezal*` become native C calls with no safepoint.

```text
A -> B -> A
```

with only forward native calls and no intra-function backedge can run
indefinitely until the **host C stack** overflows. Guest RDRAM stack would
also grow if the functions allocate frames; host overflow is typically
first.

Realistic? Mutual recursion and event-dispatch cycles exist. Unbounded
no-return cycles are already fatal on N64. The difference is that N64
hardware would still preempt the CPU; recompiled C calls will not.

```text
CROSS_FUNCTION_LOOP_GAP: INFERENCE
severity_before_PATCH4:  same as §9 — document in 3.5; do not treat as
                         a PATCH 3 generator bug against the approved
                         backedge+switch minimum.
```

---

## 11. Self-loop / `pause_self`

Self `j`/`b` emit only `pause_self()`. `pause_self` is:

```c
while (true) {
    ultramodern::wait_for_external_message(rdram);
    ultramodern::check_running_queue(rdram);
}
```

That is an existing cooperative wait: it drains external messages and may
swap. A separate PATCH 3 safepoint would be redundant **and** would never
run, because `pause_self` does not return.

```text
SELF_LOOP_COVERED_BY_EXISTING_RUNTIME_POINT: YES
SELF_LOOP_RUNTIME_POINT_VALID:               YES
class: PROVEN
```

T11 only proves "no duplicate hint". The runtime proof is `scheduling.cpp`,
not T11.

---

## 12. Jump-table safepoint

Delay slot runs first. Then safepoint. Then `emit_switch`.

Static switch expression is `jr_addend_XXXXXXXX`, a C local captured at the
ADDU (`emit_jtbl_addend_declaration`). That local survives a park.

Live `emit_switch` reloads the guest register from `ctx` **after** the
safepoint, then subtracts the (possibly relocated) table base. R0/R1
clobber from the icall is therefore not a bug.

```text
SWITCH_STATE_SURVIVES_PREEMPTION: CONDITIONAL
```

Missing proof: P3-X4 / P3-X6 — service that swaps, then the addend/index
still dispatches to the same case. Additional caveat: if a relocatable
overlay moves during the park, live subtraction uses the new base against
an old entry address. That window existed between ADDU and JR already;
PATCH 3 widens it to include a possible thread swap. Static (captured
integer addend) does not have that live-subtract race.

T11 switch executes only destination 0.

---

## 13. Live callback null policy

In-tree `LiveGeneratorInputs` constructors:

| caller | classification |
| --- | --- |
| `librecomp/src/mods.cpp` | BOTH CALLBACKS PROVIDED |
| `LiveRecomp/preemption_safepoint_test.cpp` | BOTH CALLBACKS PROVIDED |
| `LiveRecomp/live_recompiler_test.cpp` | BOTH CALLBACKS PROVIDED (hint always false) |

No UNUPDATED in-tree caller. No other `LiveGeneratorInputs` sites.

`emit_preemption_safepoint` returns if either pointer is null. Disposable
P3-X5: static still emits the call; live with null pointers executes the
same backedge with `hint==0` and `service==0`.

```text
NULL_CALLBACK_POLICY:                 A — backward-compatible "feature unavailable"
NULL_CALLBACK_POLICY_SAFE:            CONDITIONAL
STATIC_LIVE_SEMANTIC_DIVERGENCE_POSSIBLE: YES
```

A forgotten designated-init field is value-initialized to null in C++.
Positional initialization of `LiveGeneratorInputs` would silently shift
`trigger_event` into the new fields — a downstream ABI footgun. Production
`mods.cpp` is designated and updated.

Current policy is acceptable for PATCH 3 (flag OFF, tests updated). PATCH
3.5 should decide whether a debug assert is wanted. Do not change it here.

---

## 14. Flag OFF is not zero effect

```text
STATIC_TOOIE_BEHAVIOR_CHANGED:        NO
LIVE_RECOMP_EXECUTION_PATH_CHANGED:   YES
LIVE_RECOMP_SEMANTIC_BEHAVIOR_CHANGED: NO   (hint false; service not entered)
LIVE_RECOMP_PERFORMANCE_CHANGED:      NEGLIGIBLE
```

Production `RecompiledFuncs/**` was not regenerated (dated 2026-08-11;
`ultramodern_preemption_hint` grep empty; `func_800C2AB8` has no new call).

Live-recompiled mods/functions generated after PATCH 3 **do** emit hint
icalls whenever both callbacks are non-null. `mods.cpp` now supplies them.
With the flag OFF, hint returns false and service does not run, but the
icall itself executes on every backedge/switch of live code.

"Production Tooie unchanged" is true for **static generated C**. It is too
broad if it is read as "no live-recomp execution-path change".

Not a blocker.

---

## 15. Generator API compatibility

`Generator::emit_preemption_safepoint()` is a new **pure virtual**.

In-tree subclasses: `CGenerator`, `LiveGenerator`. Both implement it.

```text
IN_TREE_API_COMPATIBLE: YES
EXTERNAL_API_BREAK:     POSSIBLE
SEVERITY:               moderate for any out-of-tree custom Generator;
                        intended in-tree. A default no-op would let a
                        downstream generator silently omit safepoints and
                        undermine the compiler contract. Do not change
                        this in PATCH 3.
```

---

## 16. T11 quality

39 PASS is real (independent reconstruction). It is not 39 strong proofs.

| subtest | rating | why |
| --- | --- | --- |
| FORWARD | ADEQUATE | string `hint==0` + live `service==0`; one `beq` toy |
| BACKEDGE | STRONG | live actually iterates (`a0=3` → two taken) |
| DELAY SLOT | ADEQUATE static / WEAK live | static is comment+opcode string order; live never observes `t0` in the service callback |
| SELF LOOP | ADEQUATE / WEAK runtime | proves no extra hint; does not execute production `pause_self` (test stub returns) |
| SWITCH | ADEQUATE placement / WEAK survival | one destination; service is a counter |
| LIVE PARITY | ADEQUATE | same counts on toys |
| ABI STATUS POINTER | STRONG live / ADEQUATE static | live pointer identity; static is a string |
| FLAG OFF | STRONG live | hint-false path; production flag covered by harness T11-G |

```text
T11_TEST_QUALITY: MIXED (STRONG backedge/ABI-live; WEAK delay-live/switch-survival)
FALSE-PASS-RISK:  static is string search; a comment containing 0x1234
                  before the real addiu would still keep order because
                  the comment is emitted immediately before the instr.
                  Not a false pass on the current toys.
```

T11 does **not** cover branch-likely. That hole is closed for this review
by disposable P3-X1/X2, not by T11.

---

## 17. Missing tests (do not commit; for PATCH 3.5+)

| id | status this review | keep for later |
| --- | --- | --- |
| P3-X1 branch-likely backward taken | ran disposable; PASS | promote into T11 |
| P3-X2 branch-likely not taken | ran disposable; PASS | promote into T11 |
| P3-X3 generic indirect JR | ran disposable; PASS (documents the gap) | keep as coverage witness |
| P3-X4 jump-table value survives service | NOT RUN | yes |
| P3-X5 live callbacks null | ran disposable; PASS | promote if policy stays "silent omit" |
| P3-X6 service performs a real guest thread switch | NOT RUN | yes, needed to close CONTEXT_SAFE |
| P3-X7 static sample compiled as C/C++ | ran disposable; PASS | optional smoke |
| P3-X8 all Generator subclasses | in-tree census only | yes if N64Recomp grows another backend |

---

## 18. Production Tooie

```text
TOOIE_REGEN_RUN:                          NO
RECOMPILEDFUNCS_CHANGED:                  NO  (gitignored generated tree; dated 2026-08-11)
STATIC_GENERATED_SAFEPOINTS_PRESENT_IN_TOOIE: NO
PRODUCTION_PREEMPTION_ACTIVE:             NO
```

`func_800C2AB8` in `RecompiledFuncs/funcs_327.c` still has the original
`bne` / `bnel` backedges and no `ultramodern_preemption_hint`. Existing
`build/TooieRecompiled` (2026-08-13 02:30) is a relink of that tree.

---

## 19. Performance pre-analysis (not T15)

Heuristic over current generated C (20838 functions, 417 files):

```text
backward direct gotos ≈ 3559
switch sites          = 739
pause_self            = 3
forward gotos         ≈ 70554
```

After regen, safepoint sites ≈ **4300** (backward + switch). Per-frame
count is UNKNOWN without a profile. The hang path in `func_800C2AB8` will
hit the outer `bnel` backedge every pending iteration — that is the
intended density for that function.

```text
SAFEPOINT_SITE_COUNT_ESTIMATE: ~4300 static sites after regen
                               (3559 backward + 739 switch); per-frame UNKNOWN
PERFORMANCE_RISK:              MEDIUM
```

No invented acceptance threshold. T15 remains future work.

---

## 20. Inherited PATCH 2 issues (not PATCH 3 blockers)

Still open, still `PATCH4_READY: NO`:

- X1 interrupt cause/class fidelity
- MTC0 Status re-enable scheduling/service
- T04 return WEAK
- T07 ordering WEAK
- status initialization APPROXIMATE
- TLS epoch model CONDITIONAL

`cpu_preemptible` is still "any unmasked class". `cop0_status_write` still
does not immediately service. Confirmed by reading the reconstructed
PATCH 2 sources; no accidental fix was mixed into PATCH 3.

---

## 21. Verdict

```text
VERDICT:              B — APPROVE WITH NONBLOCKING ISSUES
PATCH3_ACCEPTED:      YES
PRE_PATCH4_SEMANTIC_WORK_READY: YES
PATCH4_READY:         NO
```

No concrete PATCH 3 compiler / ABI / delay-slot / branch-likely /
LiveRecomp-parity / runtime-safety **blocker** was found. The
high-priority likely-branch question is **PROVEN correct**. The
high-priority generic-JR question is a **coverage gap**, not a
mistranslation of a supported backedge.

Nonblocking issues to carry into PATCH 3.5 design:

1. Generic indirect JR / tail-call host recursion (PROVEN gap).
2. Cross-function call cycles (INFERENCE).
3. CONTEXT_SAFE / SWITCH_STATE still CONDITIONAL without a real swap test.
4. T11 holes (likely, delay-effect observation, switch survival, null callbacks).
5. LiveRecomp execution path now contains hint icalls with flag OFF.
6. Pure-virtual `Generator` is an external API break (acceptable).
7. Null live callbacks silently omit safepoints.

```text
NEXT_SINGLE_ACTION:
  design the smallest PATCH 3.5 semantic closure before implementation
```

Do **not** jump to PATCH 4. Do **not** enable the flag. Do **not**
regenerate Tooie. Do **not** investigate `func_800BCF28`.

---

## Classification table

| claim | class |
| --- | --- |
| Versioned hashes match expected | PROVEN |
| Clean reconstruction + idempotent apply | PROVEN |
| T11 39/0 and harness 21/0 on independent tree | PROVEN |
| Static/live ABI + status pointer identity | PROVEN |
| Safepoint after delay slot on direct backedges | PROVEN |
| Branch-likely taken/not-taken placement correct | PROVEN |
| Self `j`/`b` covered by existing `pause_self` | PROVEN |
| JAL same-section policy preserved | PROVEN |
| No Tooie regen / no production safepoints / flag OFF | PROVEN |
| Generic JR compiled as tail-call without safepoint | PROVEN |
| Cross-function loop can avoid safepoints | INFERENCE |
| Live icall ABI portable beyond Linux x64 | INFERENCE |
| Guest thread swap preserves switch addend | UNKNOWN (P3-X6 not run) |
| PATCH 4 ready | REFUTED |
| Production guest preemption active | REFUTED |
| DeepSeek Hybrid F | CORRECTED/SUPERSEDED |

=== END PATCH 3 INDEPENDENT REVIEW ===
