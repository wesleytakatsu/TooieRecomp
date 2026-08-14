---
title: "P08 — func_800BCF28 SIGSEGV diagnosis"
description: "Proven root cause: func_800BCF28 jal 0x800BCCC0 resolves to a no-op lookup_gap_stub (v0=0) registered in src/game/boot_hw_stubs.cpp, instead of the real array-accessor at 0x800BCCC0. Wrong function binding → pointer arithmetic over a null base → SIGSEGV."
status: evidence
phase: P08
updated: 2026-08-14
tags: [tooie-recomp, p08, bcf28, function-binding, gap-stub, bootstrap, diagnosis]
---

# P08 — func_800BCF28 SIGSEGV DIAGNOSIS

```text
BASE_HEAD:   7c11aaa28d85945abf633cf9abc5e112a0e2b91d
MODEL:       DeepSeek V4 Flash HIGH
MODE:        DIAGNOSIS ONLY (no fix authored)
PATCH4_ACCEPTED: YES
CRASH_REPRODUCED: YES
CRASH_DETERMINISTIC: YES
ROOT_CAUSE_LEVEL: A — ROOT CAUSE PROVEN
```

PATCH 4 is accepted and NOT re-reviewed. Overlay relocation, scheduler, and
Hybrid F' internals are **not** reopened. The findings here are confined to
reproducing and tracing the `func_800BCF28` SIGSEGV.

## 1. Canonical state

```text
HEAD:       7c11aaa28d85945abf633cf9abc5e112a0e2b91d
BRANCH:     main == origin/main
WORKTREE:   clean
BINARY:     build/TooieRecompiled
BINARY_SHA256: 384c8261cd3327ef9441e7b547649d6ddd16c71627b06b5da39bc82af9fe5801
             (matches accepted PATCH 4 binary hash)
```

## 2. Reproduction (natural run)

```sh
TOOIE_AUTO_START=1 TOOIE_DIAG=1 TOOIE_DISABLE_LAUNCHER_MUSIC=1 \
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.json \
./build/TooieRecompiled
```

`ULTRAMODERN_GUEST_PREEMPTION` left unset (default ON, PATCH 4).

```text
CRASH_DETERMINISTIC: YES  (reproduced across natural + gdb runs, same PC)
SIGNAL:   SIGSEGV (11)
HOST_FAULT_PC: 0x0000555555cac331  == func_800BCF28 + 0xE1 (+225)
GUEST_PC:  0x800BCF58  (lw $t0, 0x0($t9))
STACK:
  #0 func_800BCF28
  #1 bakey_heldCount
  #2 bainput_func_80097B4C
  #3 func_800A4878
  #4 func_800A4A74
  #5 func_800A4B08
  #6 func_800F452C
  #7 basetup_entrypoint_2
  #8 recomp_syscall_handler
  #10 gsworldDll_entrypoint_2
  #14 func_800A76F4
```

The stack matches the documented PATCH 4 crash and the 2026-08-12
temporary-yield observation.

## 3. Map host fault to guest operation

Generated file: `RecompiledFuncs/funcs_350.c` functions `func_800BCF28`
(lines 207–276). CRASH read-only inspection only; never edited.

Guest `func_800BCF28`:

```text
0x800BCF58:  lw  $t0, 0x0($t9)      <-- FAULT
```

Host fault instruction (from gdb `x/8i $pc`):

```text
movslq (%rdx,%rsi,1),%r10
  rdx = 0x80000000  (constant KSEG0-form folded by compiler)
  rsi = rdram + guest_offset
effective = 0x80000000 + rdram + guest_off = rdram + 0x8000000c
```

Fault operation category: **MEM_W(...)** load (`lw`).

### Exact fault expression

```text
base          = $v0  (return of the callee that func_800BCF28 calls)
offset_rule   = (index << 2)      # 0x800BCF50 sll $t8,$v1,2; 0x800BCF54 addu $t9,$v0,$t8
guest_address(t9) = base + (index << 2)

Captured at fault:
  t9 (guest) = 0xc = 12
  index      = 3   (because (index<<2)=0xc -> index=3)
  → base ($v0) = 0
```

MEM_W translation (recomp.h): `rdram + ((reg + offset) - 0xFFFFFFFF80000000)`.

```text
reg=0xc -> rdram + 0x8000000c   (0x8000000c bytes into region, beyond 8 MiB rdram)
fault offset into rdram = 0x8000000c   — MATCHES observed crash (COMPUTED)
```

## 4. Reconstructed original function

`banjo-tooie-Recomp/build/us/banjotooie_decompressed.elf` (MIPS-III):

```
0x800BCF28 <func_800BCF28>:
  addiu sp,sp,-24
  sw    ra,20(sp)
  sw    a1,28(sp)
  addu  t7,a0,a1
  lbu   a1,8(t7)          ; a1 := byte at (a0+a1+8)
  jal   0x800BCCC0        ; <-- targets the array-accessor
  sw    a0,24(sp)
  lw    v1,28(sp)
  move  a1,v0
  lw    a0,24(sp)
  sll   t8,v1,2
  addu  t9,v0,t8
  lw    t0,0(t9)          ; <-- FAULT: dereference the accessor result + index*4
  ...
```

Callee `0x800BCCC0` (objdump labels it `func_800BCC90+0x30` because it is the
nearest preceding symbol; it is a **separate unnamed helper**):

```
0x800BCCC0:  sll   t6,a1,3
             subu  t6,t6,a1          ; t6 = a1*7
             sll   t6,t6,3           ; t6 = a1*56
             addu  v0,a0,t6          ; v0 = a0 + a1*56
             jr    ra
             addiu v0,v0,36          ; v0 = a0 + a1*56 + 0x24
```

`0x800BCCC0` is an element accessor: `v0 = base + idx*56 + 0x24`
(stride 56, return the address of field at +0x24). A second helper
`0x800BCCD8` returns `v0 = base + (BYTE@base+4)*56 + 0x24`.

Both are **legitimate, heavily tail-called helpers** (JAL sites to 0x800BCCC0
exist across funcs_137/143/172/201/221/303/350/394; 0x800BCCD8 across 162/221).

## 5. Callee resolution at runtime — decisive

The generated `func_800BCF28` emits `LOOKUP_FUNC(0x800BCCC0)(rdram, ctx)`.
`LOOKUP_FUNC` → `get_function(0x800BCCC0)`.

A gdb run with a breakpoint immediately after `get_function(0x800BCCC0)`
returned inside `func_800BCF28` proved:

```text
AFTER-GF rax=0x555555671260  void lookup_gap_stub<0x800BCCC0>(uint8_t*, recomp_context*)
  body:  movq $0x0,0x10(%rsi); ret   # writes ctx->r2 = 0  (v0 = 0)
```

So **`get_function(0x800BCCC0)` resolves to a no-op gap stub that returns
`v0 = 0`, NOT to the real accessor** at 0x800BCCC0.

## 6. Why there is no generated function at 0x800BCCC0

`func_800BCCC0` has no symbol in the ELF (it is unnamed helper code inside the
`func_800BCC90` rom_size 0x30 window, i.e. 0x800BCC90..0x800BCCBF; the helper
lives at 0x800BCCC0 onward). N64Recomp emits functions only for symbols, so no
`func_800BCCC0` was generated (confirmed: absent from
`RecompiledFuncs/funcs.h`; overlay `.inl` has no entry at offset 0x3B700 for
this core). The recompiler therefore routes the call through
`LOOKUP_FUNC`/`get_function`.

Tooie integration handles unknown LOOKUP targets in
`src/game/boot_hw_stubs.cpp` by registering **no-op gap stubs**:

```text
src/game/boot_hw_stubs.cpp:
  template<int32_t Address> lookup_gap_stub(rdram, ctx){ ctx->r2 = 0; }  (line 179-183)
  kLookupGapAddrs contains 0x800BCCC0 and 0x800BCCD8 (lines 192-193)
  registered via add_loaded_function in banjo::register_boot_lookup_stubs (line 1087-1089)
```

This shadows the real semantics: 0x800BCCC0 / 0x800BCCD8 are **real game
accessors**, not unknown/optional helpers, so stubbing them `v0=0` corrupts the
caller's pointer arithmetic.

## 7. Full fault chain

```text
func_800BCF28(a0, a1):
  a1 = byte@(a0+a1+8)
  v0 = get_function(0x800BCCC0)(a0, a1)   # gap stub -> v0 = 0   <<-- FIRST BAD VALUE
  t8 = a1_saved << 2
  t9 = v0 + t8                                 = 0 + 0xc = 0xc   (index=3)
  t0 = MEM_W(0, t9)   # rdram + 0x8000000c -> SIGSEGV
```

```text
BAD_VALUE            = $v0 = 0   (returned by lookup_gap_stub<0x800BCCC0>)
EXPECTED_VALUE_CLASS = KSEG0 guest pointer (0x801d2xxx), valid inside rdram
BAD_VALUE_CLASS      = scalar 0 (not a pointer)
FIRST_BAD_FIELD      = $v0 (call return) in func_800BCF28
FIRST_PROVEN_DIVERGENCE = get_function(0x800BCCC0) returns lookup_gap_stub
                          instead of the real accessor (root binding error).
```

## 8. Validation that the real accessor would be safe

Computing the accessor in 32-bit guest arithmetic with the captured base:

```text
a0 = 0x801d2560 (captured), for each index k:
  v0 = a0 + k*56 + 0x24  -> 0x801d2584 / 1bc / 1f4 / 22c ...  (all valid KSEG0)
  t9 = v0 + (k<<2)       -> 0x801d2584..2638  (valid, inside rdram)
```

With the real accessor the dereference lands inside RDRAM and does **not**
fault. Only the stub's `v0=0` turns the address into `rdram+0x8000000c`.

## 9. Root-cause classification

```text
ROOT_CAUSE_LEVEL: A — ROOT CAUSE PROVEN

Mechanism: wrong binding of a real, unnamed helper to a no-op gap stub whose
return value (0) is consumed as a pointer.
```

- Function binding / missing generated function: **YES** — driving cause.
- Host/guest pointer confusion: NO (the value is a plain scalar 0, not a
  mistranslated pointer).
- Struct layout problem: NO (offsets consistent with accessor arithmetic).
- ABI problem: NO.
- Overlay relocation: NO (reopened **NO**; no relocation leak evidence).
- Preemption regression: **NO** — this is a pure function-binding error. The
  same crash signature existed in the 2026-08-12 temporary-yield experiment,
  and PATCH4 does not touch this binding. Independent of PATCH4 internals.
- BaKey/heldCount: NOT the root cause — `bakey_heldCount` is only the caller;
  the corrupted value (v0) comes from the stub, not from input state.

## 10. Refuted candidates

```text
- Scheduler regression: REFUTED (no scheduler involvement; root is a data binding)
- Input/BaKey uninitialized: REFUTED (v0=0 is the stub return, not input state)
- Missing player/world object init: REFUTED (caller data is valid; the bad
  value is the callee return)
- Host vs guest pointer confusion: REFUTED (scalar 0, not a translated pointer)
- Preemption corruption: REFUTED (same crash with/without; binding is static)
```

## 11. Writer/reader arbitration

```text
CREATOR (expected): N64Recomp generated function for 0x800BCCC0 — does NOT exist
                     (no symbol)
ACTUAL binder:      src/game/boot_hw_stubs.cpp lookup_gap_stub<0x800BCCC0>
                     (v0 = 0)
READER:             func_800BCF28 (uses v0 as pointer base)
LAST KNOWN GOOD:    if real accessor bound -> valid pointer (computed above)
FIRST KNOWN BAD:    v0 = 0 from the gap stub
```

## 12. Next minimal fix plan (NOT IMPLEMENTED — diagnosis only)

```text
FIX_OWNER:        Tooie tracked host source (runtime binding) OR metadata/generator
FILES:            src/game/boot_hw_stubs.cpp  (primary candidate, project-owned)
FUNCTIONS:        replace lookup_gap_stub entries for 0x800BCCC0 and 0x800BCCD8
                   with a real accessor implementation, OR
                  add these unnamed helpers to the symbol metadata so N64Recomp
                  emits them as generated functions
SEMANTIC_CHANGE:  0x800BCCC0(a0,a1) := a0 + a1*56 + 0x24  (return pointer)
                  0x800BCCD8(a0)    := a0 + byte(a0+4)*56 + 0x24
WHY_THIS_LAYER:   These are games' own core helpers; the project-owned stub table
                  currently masks them. Restoring the real body (or emitting the
                  generated function) fixes every JAL site at once.
TEST_THAT_WOULD_FAIL_BEFORE: world/basetup reaches func_800BCF28 and SIGSEGVs at
                  rdram+0x8000000c; with live accessor it no longer faults.
REGRESSION_RISK:  All other JAL callers of 0x800BCCC0/0x800BCCD8 (camera/actor/
                  bainput paths) would now receive real pointers instead of 0,
                  changing their prior stub-only behavior — expected correct.
```

Preference order for the fix: implement the two accessor bodies at the Tooie
tracked layer (smallest surface, no regen) OR add the unnamed helper symbols to
the metadata/symbol source so the generator emits them (most faithful, requires
regen). Do not gate on a discriminator.

## 13. Validation plan for a later fix

```text
PATCH4 stays ON
intro / audio unchanged
C2AB8 still crossed
func_800BCF28 no longer SIGSEGVs
execution progresses past basetup/bakey to the next genuine blocker or a
visible attract/demo
```

## 14. Same-as-temp-yield-crash comparison

```text
SAME_CRASH_SIGNATURE: YES
  Stack (func_800BCF28 -> bakey_heldCount -> bainput_func_80097B4C -> ... ->
  basetup -> gsworldDll -> func_800A76F4) is byte-for-byte the documented
  2026-08-12 temporary-yield observation and the PATCH4 stack.
Interpretation: this crash is downstream state exposure of the same binding
error, present regardless of yield/preempt. It is NOT a PATCH4 regression.
```

## 15. Open / judged not needed

```text
OPEN_CANDIDATES: none remaining that explain the first bad value; the only
                 unresolved item is which fix layer is chosen (see 12).
```

## 16. Docs / spec status

```text
SPECS_REVIEWED:   recomp-readiness, build, matching, overlays, input
SPECS_CHANGED:    NONE (no stable-contract violation proven; this is a runtime
                  binding defect, not a spec contract change)
```

## Handoff

```text
=== P08 func_800BCF28 DIAGNOSIS ===
BASE_HEAD:               7c11aaa28d85945abf633cf9abc5e112a0e2b91d
MODEL:                   DeepSeek V4 Flash HIGH
PATCH4_ACCEPTED:         YES
CRASH_REPRODUCED:        YES
CRASH_DETERMINISTIC:     YES
SIGNAL:                  SIGSEGV (11)
HOST_FAULT_PC:           0x0000555555cac331  (func_800BCF28 + 225)
GUEST_FUNCTION:          func_800BCF28
GUEST_PC:                0x800BCF58  (lw $t0, 0($t9))
GENERATED_FILE:          RecompiledFuncs/funcs_350.c
GUEST_INSTRUCTION:       lw $t0, 0x0($t9)
FAULT_OPERATION:         MEM_W load
FAULT_ADDRESS:           rdram + 0x8000000c
FAULT_EXPRESSION:        t9 = v0 + (index<<2); v0=0 (stub), index=3 -> t9=0xc
REGISTER_STATE:          rdx=0x80000000 rsi=rdram+0xc r9=0xc r14=rdram
CALL_CHAIN:              func_800BCF28<-bakey_heldCount<-bainput_func_80097B4C
                         <-func_800A4878..A4B08<-func_800F452C<-basetup_entrypoint_2
                         <-gsworldDll_entrypoint_2<-func_800A76F4
SAME_AS_TEMP_YIELD_CRASH: YES
FUNC_BCF28_CONTRACT:     dereference element accessor result + index*4
BAKEY_HELDCOUNT_CONTRACT: caller; not root cause
BAD_ARGUMENT_OR_GLOBAL:  $v0 (callee return) = 0
BAD_VALUE:               0
EXPECTED_VALUE_CLASS:    KSEG0 guest pointer (0x801d2xxx)
BAD_VALUE_CLASS:         scalar 0
FIRST_BAD_FIELD:         $v0 (call return) in func_800BCF28
EXPECTED_INITIALIZER:    N64Recomp-generated function for 0x800BCCC0 (none)
INITIALIZER_EXECUTED:    NO (no generated symbol)
STATE_LATER_CORRUPTED:   N/A (never correctly bound at runtime)
FIRST_PROVEN_DIVERGENCE: get_function(0x800BCCC0) = lookup_gap_stub (v0=0)
HOST_GUEST_POINTER_CONFUSION: NO
STRUCT_LAYOUT_PROBLEM:   NO
ABI_PROBLEM:             NO
OVERLAY_RELOCATION_REOPENED: NO
FUNCTION_BINDING_REOPENED: YES — specifically reopened for 0x800BCCC0/0x800BCCD8
                         with concrete evidence (stub != real accessor)
PREEMPTION_REGRESSION:   NO
ROOT_CAUSE_LEVEL:        A
ROOT_CAUSE:              no-op lookup_gap_stub masks real element accessor
                         0x800BCCC0 (and 0x800BCCD8), returning v0=0; caller
                         dereferences it as a pointer -> SIGSEGV
REFUTED_CANDIDATES:      scheduler regression; BaKey/input; host/guest pointer;
                         struct layout; preemption corruption
OPEN_CANDIDATES:         fix layer choice only (stub impl vs symbol metadata)
MINIMAL_FIX_OWNER:       Tooie tracked host source (src/game/boot_hw_stubs.cpp)
                         OR symbol metadata so generator emits the helper
MINIMAL_FIX_FILES:       src/game/boot_hw_stubs.cpp (and metadata if chosen)
MINIMAL_FIX_FUNCTIONS:   accessors at 0x800BCCC0 / 0x800BCCD8
FIX_IMPLEMENTED:         NO
NEXT_ACCEPTANCE_TEST:    PATCH4 ON; func_800BCF28 no longer SIGSEGVs; execution
                         advances past basetup/bakey to next blocker or attract
CURRENT_PRODUCT_STATE:   intro -> attract -> C2AB8 resolved -> world init ->
                         BCF28 crash
EVIDENCE_CREATED:        docs/plan/evidence/p08-bcf28-crash-diagnosis.md
SESSION_HANDOFF_UPDATED: YES
GLOBAL_HANDOFF_UPDATED:  YES
PHASE_P08_UPDATED:       YES
SPECS_REVIEWED:          recomp-readiness, build, matching, overlays, input
SPECS_CHANGED:           NONE
MARKDOWN_VAULT_AUDIT:    repo-local structural checks + git diff --check
DOC_COMMIT:              docs(p08): diagnose func_800BCF28 crash
PUSHED:                  NO (docs commit pending explicit push)
WORKTREE:                clean except intended docs changes
NEXT_SINGLE_ACTION:      implement the minimal binding fix (restore the two
                         accessors) when authorized; otherwise run the single
                         highest-value remainder (none required — root proven)
=== END HANDOFF ===
```
