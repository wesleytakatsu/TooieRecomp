---
title: "TooieRecomp Spec — Runtime, Dynamic Overlays and Guest Preemption"
description: "Comprehensive technical specification for N64ModernRuntime integration, the 884 dynamic overlay management system (kTooieOverlayCount = 884), syscall table dispatch, and Hybrid F' guest preemption."
status: current
area: "specs/Tooie"
updated: 2026-10-16
tags: [specs, tooierecomp, runtime, overlays, syscalls, preemption, scheduling]
---

# Runtime, Dynamic Overlays and Guest Preemption Specification

## 1. System Overview

TooieRecomp virtualizes Nintendo 64 system hardware via `N64ModernRuntime`. Unlike Banjo-Kazooie, *Banjo-Tooie* features:
1. An extensive 884-overlay dynamic code architecture executing out of a unified VRAM window (`0x80800000`). Indexed 1..884, slot 0 unused (`kTooieOverlayCount = 884`). (Earlier decompilation estimates referenced 851).
2. An indirect syscall trampoline mechanism mediated by the core2 engine.
3. Multi-threaded OSThread scheduling that requires active guest preemption between the game logic thread and the audio synthesizer thread.

```text
+---------------------------------------------------------------------------------+
|                       Guest Code Execution (ModernRuntime)                      |
|  - RDRAM Memory: 8 MiB (0x80000000 - 0x807FFFFF) + Virtual KSEG0/KSEG1          |
|  - OSThreads: Thread 1 (idle), Thread 3 (scheduler), Thread 4 (audio),         |
|               Thread 5 (game), Thread 6 (graphics)                              |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                      Dynamic Overlay System (syscall_handler.cpp)               |
|  - Overlays: 884 modules loaded dynamically into heap memory (slots 1..884)     |
|  - Execution Window: VRAM 0x80800000 (Size: 0x00200000)                         |
|  - Core2 Trampoline Table: Base 0x800815C0 (ROM 0x01E5AEB0)                     |
|  - Syscall Dispatch: Traps guest syscalls, verifies residency, applies relocs   |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                      Guest Preemption Engine (Hybrid F')                        |
|  - Thread-Local RCP Contexts: Independent register sets per OSThread            |
|  - Cooperative Safepoints: yield_self_1ms in scheduler loops (func_800C2AB8)   |
|  - Backedge Injection: Lightweight yield checks in recompiled function loops     |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Implementation Status |
| :--- | :--- | :--- | :--- |
| **REQ-RUN-01** | Overlay Relocation | Relocates R_MIPS_26, R_MIPS_32, R_MIPS_HI16/LO16 upon loading into heap. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-RUN-02** | Nested Overlay Calls | Maintains a call stack (`s_overlay_stack`) preserving caller state on nested loads. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-RUN-03** | Core1 RA Preservation | Wrapper functions save `$ra` in `$a3` to prevent direct host return corruption. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-RUN-04** | Audio Thread Preemption | Audio thread (`Thread 4`) preempts main loop; SoundManager delta events fire. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-RUN-05** | Boot Hardware Stubs | Seeds IPL3 boot residue at `0x802FB1F4` and `0x802FE1C0` for retail probe pass. | `IMPLEMENTED` / `VALIDATED` |
| **REQ-RUN-06** | MMIO Trap Handling | Traps unmapped SI/PI MMIO writes (`0xA48xxxxx` / `0xA46xxxxx`) cleanly. | `IMPLEMENTED` / `VALIDATED` |

---

## 3. Interface & Memory Contracts

### 3.1 Overlay Constants & Memory Layout
```cpp
// Core2 hosts the overlay_syscalls trampoline table (src/game/syscall_handler.cpp)
static constexpr uint32_t kCore2Ram = 0x800815C0u;
static constexpr uint32_t kCore2Rom = 0x01E5AEB0u;
static constexpr int32_t  kOverlayVram = static_cast<int32_t>(0x80800000u);
static constexpr uint32_t kOverlayWindowSize = 0x00200000u;
static constexpr uint32_t kTooieOverlayCount = 884;
```

### 3.2 Overlay Tracking Structures
```cpp
struct ActiveOverlayMapping {
    uint32_t overlay_index = 0;
    uint32_t rom_addr = 0;
    uint32_t code_size = 0;
    gpr package = 0;
    int32_t text_start = 0;
    uint32_t text_size = 0;
    bool dynamic_alive = false;
};

struct ResidentOverlay {
    gpr package = 0;
    ActiveOverlayMapping mapping{};
};

// Thread-local residency cache ensuring thread-safe overlay access
static thread_local std::array<ResidentOverlay, kTooieOverlayCount + 1> s_resident_overlays{};
static thread_local std::vector<ActiveOverlayMapping> s_overlay_stack;
```

### 3.3 Syscall Dispatch Interface
- `ovl_load(uint8_t* rdram, recomp_context* ctx)`: Allocates heap memory, decodes compressed overlay data, writes code to RDRAM, and resolves relocations.
- `ovl_unload(uint8_t* rdram, recomp_context* ctx)`: Frees heap allocation and invalidates residency mapping.
- `func_80082088(uint8_t* rdram, recomp_context* ctx)`: Main overlay entry dispatcher invoked via syscall trampoline.
- Return Address Wrapper: Leaf wrappers (`core1_ra_wrappers`) guarantee `$ra` is moved to a preserved register (`$a3`) before host dispatch, resolving the lookup-zero defect (`p08-core1-ra-wrapper-fix.md`).

---

## 4. Guest Preemption Architecture (Hybrid F')

Static recompilation translates MIPS instructions into host C code blocks that run synchronously until completion. Without preemption, spinloops in the game thread prevent the audio synthesis thread from executing, causing buffer starvation and event deadlocks:

### Preemption Mechanism & Components:
1. **Safepoint Yields (PATCH 1)**:
   - Injected into scheduler spinwaits (e.g. `func_800C2AB8`).
   - Calls `ultramodern::yield_self_1ms()`, allowing `Thread 4 [AUDIO]` to execute pending delta-queue items.
2. **Runtime Thread Switching Service (PATCH 2)**:
   - Hooks `N64ModernRuntime` thread scheduler.
   - Evaluates thread priority queues on message enqueue (`osSendMesg`, `osRecvMesg`).
3. **Loop Backedge Yields (PATCH 3)**:
   - Generator injects periodic execution checks into loop backedges of recompiled functions.
4. **Thread-Local RCP Register Contexts (PATCH 3.5)**:
   - Each `OSThread` maintains its own RCP execution context structure.
   - Eliminates process-global register corruption during preemptive context switches.
5. **Clean-Build Validation (PATCH 4)**:
   - Validated independently (**VERDICT B**, behavior **CLASS A**).
   - Resolves Slot 7 SoundManager event processing (`p08-preemption-patch4-clean-build-independent-review.md`).

---

## 5. Boot Hardware & Security Emulation

Implemented in `src/game/boot_hw_stubs.cpp`:
1. **IPL3 Boot Residue**:
   - `tooie_seed_ipl3_boot_residue(uint8_t* rdram)`: Seeds residual boot memory words checked by `func_8001DE64`:
     - `0x802FB1F4`: Matches IPL3 opcode residual.
     - `0x802FE1C0`: Matches IPL3 checksum comparator.
2. **CIC-6105 Emulation**:
   - Host intercept returns expected challenge seeds, satisfying boot integrity checks in `gldb`, `gsmaker`, and `gsworld`.
3. **Lookup Gap Stubs**:
   - Unnamed boot helpers and compiler stubs are registered via `banjo::register_boot_lookup_stubs()`, replacing unmapped null callbacks with deterministic handlers.
