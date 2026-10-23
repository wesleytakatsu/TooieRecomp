---
title: "TooieRecomp Spec — Runtime, Dynamic Overlays and Guest Preemption"
description: "Technical specification for N64ModernRuntime integration, the 884 dynamic overlay management system, syscall table dispatch, and Hybrid F' guest preemption."
status: current
area: "specs/Tooie"
updated: 2026-10-23
tags: [specs, tooierecomp, runtime, overlays, syscalls, preemption, scheduling]
---

# Runtime, Dynamic Overlays and Guest Preemption Specification

## 1. System Overview

TooieRecomp virtualizes Nintendo 64 hardware execution via `N64ModernRuntime`. *Banjo-Tooie* relies on three runtime architectures:
1. An 884-overlay dynamic code architecture executing out of a unified VRAM window (`0x80800000`). Indexed 1..884, slot 0 unused (`kTooieOverlayCount = 884`).
2. An indirect syscall dispatch mechanism mediated by core2 trampoline decoding.
3. Multi-threaded OSThread scheduling with cooperative guest preemption between the game logic thread and the audio synthesis thread.

```text
+---------------------------------------------------------------------------------+
|                       Guest Code Execution (ModernRuntime)                      |
|  - RDRAM Memory: 8 MiB (0x80000000 - 0x807FFFFF) + Virtual KSEG0/KSEG1          |
|  - OSThreads: Thread 1 (IDLE), Thread 2 (TIDY), Thread 3 (CIC6105),             |
|               Thread 4 (AUDIO), Thread 5 (SCHEDULER), Thread 6 (MAIN),          |
|               Thread 7 (JOY), Thread 8 (MOTOR), Thread 9 (CRASH)                |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                      Dynamic Overlay System (syscall_handler.cpp)               |
|  - Overlays: 884 modules loaded dynamically into heap memory (slots 1..884)     |
|  - Execution Window: VRAM 0x80800000 (Size: 0x00200000)                         |
|  - Core2 Trampoline Table: Base 0x800815C0 (ROM 0x01E5AEB0, kCore2Rom)          |
|  - Syscall Dispatch: recomp_syscall_handler decodes index and entry             |
+---------------------------------------------------------------------------------+
                                       |
                                       v
+---------------------------------------------------------------------------------+
|                      Guest Preemption Engine (Hybrid F')                        |
|  - Generator Backedge Safepoints: ultramodern_preemption_hint / service         |
|  - Thread-Local Context: TLS RDRAM and status_reg per thread                    |
|  - Control: ULTRAMODERN_GUEST_PREEMPTION env flag (default ON)                  |
+---------------------------------------------------------------------------------+
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-RUN-01** | Overlay Relocation | Relocates R_MIPS_26, R_MIPS_32, R_MIPS_HI16/LO16 upon loading into heap. | `IMPLEMENTED` |
| **REQ-RUN-02** | Nested Overlay Calls | Maintains a call stack (`s_overlay_stack`) preserving caller state on nested loads. | `IMPLEMENTED` |
| **REQ-RUN-03** | Core1 RA Preservation | Strong native symbol overrides (`func_800137D4`, `func_800137F4`, `func_80013A5C`, `func_80013A7C`) preserve `$ra` across jumps. | `IMPLEMENTED` |
| **REQ-RUN-04** | Audio Preemption | Audio thread (`Thread 4`) preempts main loop via generator loop backedges. | `IMPLEMENTED` |
| **REQ-RUN-05** | Boot Hardware Stubs | Seeds IPL3 boot residue at `0x802FB1F4` and `0x802FE1C0` for boot verification. | `IMPLEMENTED` |
| **REQ-RUN-06** | MMIO Trap Handling | Traps unmapped SI/PI MMIO writes (`0xA48xxxxx` / `0xA46xxxxx`) cleanly. | `IMPLEMENTED` |

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
static constexpr uint32_t kOvlTableRom = 0x01F00020u;
```

### 3.2 Overlay Tracking & State Flow
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

// Thread-local residency cache and active mapping stack
static thread_local std::array<ResidentOverlay, kTooieOverlayCount + 1> s_resident_overlays{};
static thread_local std::vector<ActiveOverlayMapping> s_overlay_stack;
static thread_local ActiveOverlayMapping s_canonical_mapping{};
```

State transitions are managed through `park_canonical_mapping()`, `activate_canonical_mapping()`, and `tooie_invalidate_resident_overlay()`.

### 3.3 Overlay Lifecycle & Syscall Dispatch Interface
- `ovl_load(uint8_t* rdram, recomp_context* ctx)`:
  - Reads overlay package metadata from ROM table at `0x01F00020` (`kOvlTableRom`). Packages with compression flag `0x80` are rejected (requires uncompressed ROM data).
  - Allocates heap via `heap_alloc_sided` (`0x8001ACEC`), copies overlay data via `do_rom_read`, and clears BSS (`0x80082380`).
  - Registers native recompiled functions with the runtime via `load_overlays(mapping_rom, text_start, size)`.
  - Caches resident overlays (`flags & 0x04`) in `s_resident_overlays`.
- `ovl_unload(uint8_t* rdram, recomp_context* ctx)`:
  - If overlay is resident (`flags & 0x04`), returns immediately without unmapping.
  - Otherwise resets syscalls (`0x80082428`, `ovl_reset_syscalls`), unregisters native functions via `unload_overlays`, frees heap memory (`0x8001B084`, `heap_free`), and clears the residency cache entry.
- `func_80082088(uint8_t* rdram, recomp_context* ctx)`:
  - Host-safe replacement for the guest overlay relocator. Validates overlay header, entry points, and relocations at `text_start`.
- `defragment_overlays`:
  - Host-safe no-op stub (`boot_hw_stubs.cpp`). `func_800815CC` rewrites the table base.
- Syscall Dispatch (`recomp_syscall_handler` in `src/game/syscall_handler.cpp`):
  - Traps guest syscall executions from the core2 trampoline.
  - Decodes `syscall_code = overlay index` and `imm >> 2 = entry index` by reading instruction words at `kCore2Rom`.
  - Dispatches calls via `g_tooie_overlay_dispatch[idx]` defined in `generated/overlay_dispatch.inc.h`.
  - First 128 dispatches unconditionally log `[tooie] syscall dispatch`. Additional logging is gated by `TOOIE_DIAG_OVERLAY_LOG`.
- Section Deduplication:
  - A local runtime patch (`vendor/patches-local/n64modernruntime-overlay-section-dedupe.patch`) modifies `librecomp/src/overlays.cpp` so that `load_overlay` skips duplicate `loaded_sections` entries for identical `(ram, section_index)`, eliminating multi-second stalls during map changes.
- Return Address Overrides:
  - Strong symbol definitions in `boot_hw_stubs.cpp` (`func_800137D4`, `func_800137F4`, `func_80013A5C`, `func_80013A7C`) interpose over weak compiler-generated stubs, correctly reproducing `$ra` preservation across `jr a1`/`jr a3` transitions.

---

## 4. Hybrid F' Guest Preemption

Static recompilation translates MIPS basic blocks into host C functions that execute cooperatively. Without preemption, CPU spinloops in game logic delay the audio synthesizer thread (`Thread 4`), causing audio buffer starvation.

### Preemption Architecture:
1. **Generator Safepoints**:
   - The N64Recomp C code generator emulates cooperative preemption at loop backedges:
     ```c
     if (ultramodern_preemption_hint()) {
         ultramodern_service_preemption(rdram, &ctx->status_reg);
     }
     ```
   - Emitted automatically by the code generator (`N64Recomp/src/cgenerator.cpp`).
2. **Runtime Scheduling & Synchronization**:
   - Implemented in `ultramodern/src/preemption.cpp` and `ultramodern/src/mesgqueue.cpp`.
   - Uses thread-local `rdram` and `status_reg` pointers, a global work epoch counter (`g_work_epoch`), and cause-aware message queue draining with RCP class masks.
3. **Control Flag**:
   - `ULTRAMODERN_GUEST_PREEMPTION`: Environment variable checked at process initialization (`main.cpp`).
   - Unset or empty: **ON** (default production behavior).
   - Value `0`, `false`, `off`, `no`: **OFF** (disables preemption across all threads).
   - Status is logged at boot: `[tooie] guest preemption: ON/OFF`.

---

## 5. Boot Hardware & Security Emulation

Implemented in `src/game/boot_hw_stubs.cpp`:
1. **IPL3 Boot Residue**:
   - `tooie_seed_ipl3_boot_residue(uint8_t* rdram)`: Seeds residual boot memory words at `0x802FB1F4` and `0x802FE1C0`, checked by `func_8001DE64`.
2. **CIC-6105 Emulation**:
   - Provides a 64-byte PIF-RAM mirror and executes the X-Scale 6105 challenge-response algorithm in `tooie___osSiRawStartDma` (hooked at `0x80031A60`).
3. **Lookup Gap Stubs**:
   - Unmapped boot helpers and compiler stubs are registered via `banjo::register_boot_lookup_stubs()`, replacing unmapped null callbacks with deterministic host stubs.