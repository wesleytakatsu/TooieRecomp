---
title: "BT — Setup"
status: current
area: "specs/BT"
updated: 2026-08-09
source_path: banjo-tooie-Recomp
source_repo: wesleytakatsu/banjo-tooie-Recomp
upstream_repo: Mr-Wiseguy/banjo-tooie
tags:
  - banjo-tooie-Recomp
  - setup
---

# BT — Setup (Historical Reference Specification)

> [!WARNING]
> **Reference Specification Only:** This document records the original standalone decompilation setup from August 2026. **Do NOT execute these commands for building TooieRecomp.** Under the current standalone architecture (P10), decompilation sources are vendored in `decomp/` and no external checkout, sibling, or clone of `banjo-tooie-Recomp` is required or used. To build TooieRecomp, refer directly to [`README.md`](../../../README.md) and [`tools/BUILDING.md`](../../../tools/BUILDING.md).

## Historical Checkout Context

| Item | Valor verificado em 2026-08-09 |
| ---- | ---- |
| Caminho histórico | `banjo-tooie-Recomp/` (agora vendorizado em `decomp/`) |
| `origin` | `https://github.com/wesleytakatsu/banjo-tooie-Recomp.git` |
| `upstream` | `https://github.com/Mr-Wiseguy/banjo-tooie.git` |
| Branch | `master`, acompanhando `origin/master` |
| HEAD | `25033be6ec9e35975483f064d086391d2381ca4c` |
| Relação com upstream | 0 ahead / 0 behind de `upstream/master` |
| Worktree | limpo |

O checkout antigo `banjo-tooie/` foi excluído. Ele não deve aparecer em
caminhos locais, scripts nem instruções. O nome sem `-Recomp` continua válido
somente para identificar o repositório upstream.

## Baserom

Place a Banjo-Tooie NTSC-U ROM in the repo root as `baserom.us.z64`.

| Item | Value |
| ---- | ----- |
| Filename | `baserom.us.z64` |
| SHA1 | `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3` |
| Needed by | `tools/rom_decompressor` |

> The splat config (`baserom.us.yaml`) lists a *different* SHA1
> (`af479771ff5e91ce3732f3d84146614ab71d8d79`); that is the checksum of the
> split/build target. The README asks for the SHA1 above.

## Historical Clone & Submodule Structure

*Historical reference only — in TooieRecomp, these sources are already vendored under `decomp/` and submodule updates are unnecessary:*

```text
Historical repository origin:
https://github.com/wesleytakatsu/banjo-tooie-Recomp.git
```

## Submodules (`.gitmodules`)

| Path | Upstream |
| ---- | -------- |
| `tools/splat` | `github.com/ethteck/splat` |
| `tools/asm-processor` | `github.com/simonlindholm/asm-processor` |
| `tools/elfio` | `github.com/serge1/ELFIO` |
| `tools/rarezip` | `github.com/Mr-Wiseguy/rarezip` |
| `tools/toml11` | `github.com/ToruNiina/toml11` |
| `tools/fmt` | `github.com/fmtlib/fmt` |
| `lib/ultralib` | `github.com/Mr-Wiseguy/ultralib` (branch `tooie`) |

## Dependencies (Ubuntu)

```sh
sudo apt install libfmt-dev libtoml11-dev gcc-mips-linux-gnu
```

`mips-linux-gnu-gcc / -objcopy / -ld / -as` provide the MIPS assembler and
linker. `libfmt` + `toml11` are required by the tools in `tools/`.

## Python requirements

```sh
python3 -m pip install -r tools/splat/requirements.txt
python3 -m pip install -r tools/requirements.txt
```

`tools/requirements.txt` pins: `spimdisasm==1.35.0`, `toml>=0.10.2`,
`mapfile-parser>=2.4.0,<3.0.0`.

## `make setup`

```sh
make setup
```

Performs:

1. Builds `lib/ultralib` (`make -C lib/ultralib setup NON_MATCHING=1`).
2. Builds `tools/` (downloads IDO `cc`, builds decompressor/compressor/etc.).
3. Decompresses the ROM: `tools/rom_decompressor baserom.us.z64 decompressed.us.z64`.
4. Splits with Splat: `python3 tools/splat/split.py baserom.us.yaml`.

`make distclean` resets `build/` `asm/` `assets/`.

*Historical reference only:* Originally executed via `make -C banjo-tooie-Recomp setup`, `make -C banjo-tooie-Recomp -j$(nproc)`, and `make -C banjo-tooie-Recomp check`. In TooieRecomp, decompilation and extraction are handled via `tools/extract_rom.sh` and `tools/build_linux.sh` (see [`tools/BUILDING.md`](../../../tools/BUILDING.md)).
