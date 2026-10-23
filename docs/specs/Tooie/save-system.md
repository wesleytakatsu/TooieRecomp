---
title: "TooieRecomp Spec — Save System and Data Persistence"
description: "Technical specification for EEPROM emulation, save slot management, and filesystem persistence in TooieRecomp."
status: current
area: "specs/Tooie"
updated: 2026-10-23
tags: [specs, tooierecomp, saves, eeprom, persistence]
---

# Save System and Data Persistence Specification

## 1. Subsystem Architecture

Banjo-Tooie uses a **16 Kbit EEPROM** (2,048 bytes) for persistent progress storage. TooieRecomp virtualizes this memory using host filesystem storage with atomic file operations to protect against corrupted or torn writes.

```text
Game Engine Save/Load Routines
              |
              v
Ultra64 OS SDK EEPROM API (osEepromProbe / osEepromRead / osEepromWrite)
              |
              v
Guest Function Jump Intercepts (src/game/boot_hw_stubs.cpp)
              |
              v
Native SDK Emulation (N64ModernRuntime: osEepromProbe_recomp / osEepromRead_recomp / osEepromWrite_recomp)
              |
              v
Atomic Backup File Writer (open_output_file_with_backup)
              |
              v
Atomic Persistence -> <config_path>/saves/bt.n64.us.1.0.bin
```

---

## 2. Requirements Traceability

| Requirement ID | Description | Acceptance Criteria | Status |
| :--- | :--- | :--- | :--- |
| **REQ-SAV-01** | 16 Kbit EEPROM Emulation | Emulates 2,048 bytes (256 blocks $\times$ 8 bytes) with block-offset translation. | `IMPLEMENTED` |
| **REQ-SAV-02** | SDK Function Replacement | Intercepts `osEepromProbe`, `osEepromRead`, `osEepromWrite`, `osEepromLongRead`, `osEepromLongWrite`. | `IMPLEMENTED` |
| **REQ-SAV-03** | Save Slot Navigation | File selection menu navigates slots and confirms selections. | `IMPLEMENTED` |
| **REQ-SAV-04** | Title Progression | File selection confirms slot and transitions to opening cutscene (`MAP_A1_CS_INTRO`). | `IMPLEMENTED` |
| **REQ-SAV-05** | Atomic Host Persistence | Disk writes use atomic backup and rename to prevent torn writes on termination. | `IMPLEMENTED` |

---

## 3. Interface & Hardware Contracts

### 3.1 EEPROM SDK Emulation & Hardware Layer Ownership
- **SDK Device Identification & Probe Contract**:
  - In original N64 software, `osEepromProbe` queries the Serial Interface. In TooieRecomp, the Ultra64 SDK function is intercepted directly at guest address `0x80030170` by `osEepromProbe_recomp` (`N64ModernRuntime/librecomp/src/eep.cpp`).
  - Configured with `recomp::SaveType::Eep16k` in `src/main/main.cpp`.
  - For `SaveType::Eep16k` (and `SaveType::AllowAll`), `osEepromProbe_recomp` writes `0x02` (`EEPROM_TYPE_16K`) into guest register `r2` (`v0`).
  - For `SaveType::Eep4k`, it writes `0x01` (`EEPROM_TYPE_4K`); otherwise it writes `0x00`.
- **Block Addressing**:
  - Total Capacity: 2,048 bytes (256 blocks $\times$ 8 bytes per block).
  - Block Address Range: `0x00` to `0xFF` (`eeprom_block_size = 8`).
  - Read/Write offset mapping: `offset = eep_address * 8`, counted in multiples of 8 bytes.
- **Command Dispatch & Layer Ownership**:
  - In TooieRecomp, low-level Joybus command packets are not parsed for EEPROM. Instead, native SDK replacements are registered directly at guest entrypoints in `src/game/boot_hw_stubs.cpp`:
    - `0x80030170`: `osEepromProbe_recomp`
    - `0x80032B10`: `osEepromRead_recomp`
    - `0x80032D60`: `osEepromWrite_recomp`
    - `0x8002DE70`: `osEepromLongRead_recomp`
    - `0x8002DF00`: `osEepromLongWrite_recomp`
  - Tooie's raw SI hook (`tooie___osSiRawStartDma` at `0x80031A60`) is retained strictly for CIC-6105 challenge/response traffic; EEPROM operations completely bypass low-level SI DMA.
- **Atomic Persistence Semantics**:
  - Host persistence is managed in `N64ModernRuntime` (`librecomp/src/pi.cpp:update_save_file`).
  - File path: `<config_path>/saves/<game_id>.bin` (where `game_id = "bt.n64.us.1.0"`; on Windows `%LOCALAPPDATA%\TooieRecompiled\saves\bt.n64.us.1.0.bin`).
  - Writes execute via `recomp::open_output_file_with_backup` and `recomp::finalize_output_file_with_backup`, writing to a temporary file before atomically renaming to the final path.
  - This mechanism ensures host crash safety against incomplete or torn writes during filesystem flushes.

### 3.2 Save Slot Progression
- The 2,048-byte EEPROM contains global options (sound configuration) and 4 individual game save slots.
- Menu navigation in `chgameselect` (overlay 205) advances to `MAP_A1_CS_INTRO (161)` in cutscene mode upon slot confirmation.
- Full in-game save, copy, and erase lifecycle testing across extended play sessions is tracked as an ongoing validation item.