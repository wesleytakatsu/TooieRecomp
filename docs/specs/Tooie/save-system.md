---
title: "TooieRecomp Spec — Save System and Data Persistence"
description: "Comprehensive technical specification for EEPROM emulation, save slot management, and filesystem persistence in TooieRecomp."
status: current
area: "specs/Tooie"
updated: 2026-10-17
tags: [specs, tooierecomp, saves, eeprom, persistence]
---

# Save System and Data Persistence Specification

## 1. Subsystem Architecture

Banjo-Tooie uses a **16 Kbit EEPROM** (2,048 bytes) for persistent progress storage. TooieRecomp virtualizes this memory using host filesystem storage with atomic file operations to provide host crash-safety against torn writes.

```text
Game Engine Save/Load Routines
              |
              v
Ultra64 OS SDK EEPROM API (osEepromProbe / osEepromRead / osEepromWrite)
              |
              v
Guest Jump Dispatch (src/game/boot_hw_stubs.cpp: 0x80030170, 0x80032B10, 0x80032D60, etc.)
              |
              v
Native SDK Emulation (N64ModernRuntime: osEepromProbe_recomp / osEepromRead_recomp / osEepromWrite_recomp)
              |
              v
Atomic Temporary File Writer (tooie.eep.tmp via open_output_file_with_backup)
              |
              v
Atomic Rename -> User Profile (tooie.eep via finalize_output_file_with_backup)
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Implementation Status |
| :--- | :--- | :--- | :--- |
| **REQ-SAV-01** | 16 Kbit EEPROM Emulation | Emulates 2,048 bytes (256 blocks $\times$ 8 bytes) with block-offset translation. | `IMPLEMENTED` / `BOOT_VALIDATED` |
| **REQ-SAV-02** | SDK EEPROM Function Replacement | Intercepts `osEepromProbe`, `osEepromRead`, `osEepromWrite`, `osEepromLongRead`, and `osEepromLongWrite` with correct status/return codes. | `IMPLEMENTED` / `BOOT_VALIDATED` |
| **REQ-SAV-03** | Save Slot Navigation | File selection menu navigates and confirms slots; copy and erase operations duplicate and clear slot payloads without memory corruption. | `PARTIAL` / `PENDING_QA` |
| **REQ-SAV-04** | Title-to-Game Progression | Intro cutscene -> file select -> confirm warps to opening story cutscene (`MAP_A1_CS_INTRO`). | `IMPLEMENTED` / `VALIDATED` |
| **REQ-SAV-05** | Atomic Host Persistence | Disk writes use atomic file replacement (`.tmp` -> final) to prevent torn writes on abrupt host termination. | `IMPLEMENTED` / `PENDING_QA` |

---

## 3. Interface & Hardware Contracts

### 3.1 EEPROM SDK Emulation & Hardware Layer Ownership
- **SDK Device Identification & Probe Contract**:
  - In original N64 software, `osEepromProbe` queries the Serial Interface. In TooieRecomp, the Ultra64 SDK function is intercepted directly at guest address `0x80030170` by `osEepromProbe_recomp` (`N64ModernRuntime/librecomp/src/eep.cpp:11-24`).
  - For `recomp::SaveType::Eep16k` (and `recomp::SaveType::AllowAll`), `osEepromProbe_recomp` writes `0x02` (`EEPROM_TYPE_16K`) into guest register `r2` (`v0`).
  - For `recomp::SaveType::Eep4k`, it writes `0x01` (`EEPROM_TYPE_4K`); otherwise it writes `0x00`.
  - **Distinction from Raw Joybus Status**: This `0x02` value is strictly the Ultra64 OS SDK return code. It must not be conflated with the raw Joybus/PIF serial bus device status word (where `0x0080` historically appeared as a raw Joybus device type/status bit). TooieRecomp replaces the SDK function directly and does not guess or require low-level Joybus raw protocol status values for EEPROM.
- **Block Addressing**:
  - Total Capacity: 2,048 bytes (256 blocks $\times$ 8 bytes per block).
  - Block Address Range: `0x00` to `0xFF` (`eeprom_block_size = 8`).
  - Read/Write offset mapping: `offset = eep_address * 8`, count in multiples of 8 bytes.
- **Command Dispatch & Layer Ownership**:
  - In hardware, the Ultra64 SDK translated calls into Joybus SI command bytes (`0x04` for read, `0x05` for write).
  - In TooieRecomp, these low-level Joybus command packets are **not** parsed or emulated for EEPROM. Instead, native SDK replacements are registered directly at the guest function entrypoints in `src/game/boot_hw_stubs.cpp:656-668`:
    - `0x80030170`: `osEepromProbe_recomp`
    - `0x80032B10`: `osEepromRead_recomp`
    - `0x80032D60`: `osEepromWrite_recomp`
    - `0x8002DE70`: `osEepromLongRead_recomp`
    - `0x8002DF00`: `osEepromLongWrite_recomp`
  - Tooie's raw SI hook (`tooie___osSiRawStartDma` at `0x80031A60`) is deliberately retained *only* for CIC-6105 PIF challenge/response traffic; ordinary EEPROM storage and controller input completely bypass low-level SI DMA.
  - Custom data structures, mod storage, and object payload extensions are managed separately via `src/game/recomp_data_api.cpp` (`recomputil::init_extended_object_data`).
- **Atomic Persistence Semantics**:
  - Host persistence is handled in `N64ModernRuntime` (`librecomp/src/pi.cpp:update_save_file`) via `recomp::open_output_file_with_backup` and `recomp::finalize_output_file_with_backup`.
  - Writes are coalesced in a dedicated saving thread and written to a temporary file (`.tmp`) before being atomically renamed to the final save path (`ultramodern::get_save_file_path()`).
  - **Scope Limitation**: Atomic file replacement is a **host crash-safety and torn-write prevention mechanism**. It protects the save file from truncation if the process or host OS terminates abruptly during a flush. It is **not** a universal prevention against in-game save corruption (e.g., guest game logic errors, corrupted memory state prior to write, or bad checksum calculations).

### 3.2 Save Slot Structure
Banjo-Tooie partitions the 2,048-byte EEPROM into:
- **Global Options & Sound Configuration**: Master volume, camera options, stereo/surround flags.
- **Save Slots 1 through 4**:
  - Jiggy counts and collection flags.
  - Musical notes, Honeycomb pieces, Cheato pages, Glowbos.
  - Jinjo collection states per family.
  - World unlock flags and warp pad activations.
  - Checksum validation bytes ensuring integrity on boot.

---

## 4. Verification Evidence & QA Boundaries (P08-T10)

Frontend file selection and save slot confirmation were validated in `docs/plan/evidence/p08-title-file-select-validation.md`:
- **Validated Frontend Sequence**:
  1. Boot from cold start through opening cutscenes (`mode=3, map=0`).
  2. Joybus/EEPROM initialization in JOY thread (`func_800166DC`): Requires `osEepromProbe >= EEPROM_TYPE_16K (0x02)`. `osEepromProbe_recomp` returning `0x02` satisfies this check, avoiding the `errno = 8` fallback that would disable controller reading.
  3. Synthetic `CONT_START` advances title sequence to File Selection menu (`chgameselect`, overlay 205, `mode=2`).
  4. Navigate between slots using analog stick (`STICK_LEFT` / `STICK_RIGHT`).
  5. Confirm selection via `CONT_A` at $t=26.03\text{s}$.
  6. Destructor of `chgameselect` unloads cleanly; destination is `MAP_A1_CS_INTRO (161)` in `mode=3` (opening story cutscene), which eventually transitions to Spiral Mountain gameplay upon completing or skipping the sequence. (Prior wording claiming direct transition to live mode=1 Spiral Mountain gameplay is `CORRECTED/SUPERSEDED`).
- **QA Boundaries & Open Scope**:
  - While native SDK EEPROM probe return and frontend slot selection confirmation are validated, **comprehensive in-game save QA is UNTESTED / PENDING**.
  - In-game save writes, copy slot, erase slot, and power-cycle reboot verification across all 4 save slots have not been exhaustively tested and remain tracked for P09/P10 validation.
