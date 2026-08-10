---
title: "BT P02 — ROM, decomposição e ELF"
description: "Produz e valida os artefatos binários que serão a entrada da recompilação estática."
status: done
type: implementation-stage
project: TooieRecomp
phase: P02
order: 2
depends_on: P01
updated: 2026-08-09
source_path: banjo-tooie-Recomp
tags: [banjo-tooie-Recomp, rom, decompression, elf, splat]
---

# BT P02 — ROM, decomposição e ELF

## Resultado esperado

Pipeline determinístico da ROM original até a ROM decomposta, split do Splat,
ELF MIPS e reconstrução matching.

## Entradas

- ROM válida definida em [[00-baseline-and-decisions]]
- [[../../specs/BT/compression|Compressão BT]]
- [[../../specs/BT/splat|Splat BT]]
- [[../../specs/BT/build|Build BT]]

## Tarefas

- [x] `P02-T01` Copiar a ROM validada para `banjo-tooie-Recomp/baserom.us.z64` sem alterar o original arquivado.
- [x] `P02-T02` Executar `make -C banjo-tooie-Recomp setup` e guardar o log completo como evidência.
- [x] `P02-T03` Validar a ROM decomposta contra o SHA-1 `af479771ff5e91ce3732f3d84146614ab71d8d79` declarado pelo Splat.
- [x] `P02-T04` Confirmar geração dos mapas de símbolos, relocations e `overlays.us.toml`.
- [x] `P02-T05` Executar `make -C banjo-tooie-Recomp -j$(nproc)` e produzir `banjo-tooie-Recomp/build/us/banjotooie_decompressed.elf`.
- [x] `P02-T06` Executar `make -C banjo-tooie-Recomp check` para comparar a ROM reconstruída com a referência decomposta.
- [x] `P02-T07` Registrar hashes, tamanhos e comandos de todos os artefatos de saída.
- [x] `P02-T08` Automatizar falha rápida para ROM, hash, tamanho ou byte order incorretos.

## Evidências

Registrado em 2026-08-09. Logs em `docs/plan/evidence/p02-*`.

| Check | Resultado |
| --- | --- |
| baserom copiado | SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`; arquivo arquivado intacto |
| `make setup` | **exit 0** (~32s após tools) — decompress + Splat split 35 MB |
| decompressed SHA-1 | **`af479771ff5e91ce3732f3d84146614ab71d8d79`** (tamanho 35098224) |
| mapas | symbol/ovl/reloc/syscall + `overlays.us.toml` presentes |
| Splat tree | `asm/` (1134 dirs), `src/` (1108 `.c`), `assets/` |
| `make -j` | **exit 0** → ELF 32-bit MSB MIPS-III, 91855676 bytes |
| `make check` | **OK** (`diff decompressed.us.z64 build/us/banjotooie_decompressed.z64`) |
| matching byte-a-byte | decompressed == rebuilt decompressed.z64 |
| validador | `scripts/validate-p02-rom-pipeline.sh` **PASS** |

### Comandos

```sh
source .venv/bin/activate
cp "Banjo-Tooie (USA).z64" banjo-tooie-Recomp/baserom.us.z64
make -C banjo-tooie-Recomp setup -j$(nproc)
make -C banjo-tooie-Recomp -j$(nproc)
make -C banjo-tooie-Recomp check
./scripts/validate-p02-rom-pipeline.sh
```

### Hashes principais

| Artefato | Size | SHA-1 |
| --- | --- | --- |
| `baserom.us.z64` | 33554432 | `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3` |
| `decompressed.us.z64` | 35098224 | `af479771ff5e91ce3732f3d84146614ab71d8d79` |
| `build/us/banjotooie_decompressed.z64` | 35098224 | `af479771ff5e91ce3732f3d84146614ab71d8d79` (match) |
| `build/us/banjotooie_decompressed.elf` | 91855676 | `2b2d04c5b6484594f750b097daf670f7a8a24167` |
| `build/us/banjotooie.z64` (recompressed) | 33442400 | `413589c9a3fac403b90ff108eab669eb3ae397f7` |

Detalhes (SHA-256, seções ELF, contagem de símbolos ≈53498): `docs/plan/evidence/p02-artifact-hashes.txt`.

### Notas

- `make check` compara a ROM **decomposta** com o rebuild descomprimido, não o baserom de cartucho.
- O baserom original (32 MiB padded) e `banjotooie.z64` recomprimido (≈31.9 MiB) diferem em tamanho; isso é esperado e não invalida o matching do decomp.
- Entrada crítica para P03/P04: `build/us/banjotooie_decompressed.elf` + mapas de símbolos/relocs/overlays.

## Gate de saída

- [x] `make -C banjo-tooie-Recomp setup` é repetível a partir de checkout limpo (com venv + baserom + submódulos).
- [x] ELF contém seções e símbolos esperados (`.boot`, `.core1`, `.core2`, overlays em `0x80800000`, relocs emitidos).
- [x] Reconstrução matching é byte a byte no alvo decomposto.

## Próxima etapa

[[03-recompiler-metadata]]
