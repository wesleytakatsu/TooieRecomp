---
title: "BT P03 — Metadados do recompilador"
description: "Converte ELF, símbolos, relocations, overlays e syscalls do Tooie em entradas determinísticas para N64Recomp."
status: done
type: implementation-stage
project: TooieRecomp
phase: P03
order: 3
depends_on: P02
updated: 2026-08-09
tags: [banjo-tooie-Recomp, symbols, relocations, overlays, n64recomp]
---

# BT P03 — Metadados do recompilador

## Resultado esperado

Configurações e tabelas geradas que descrevem cada função, seção, overlay,
entrypoint, relocation e syscall necessários pela recompilação estática.

## Contexto verificado

O decompressor BT já consegue emitir símbolos/relocations para Splat e
`overlays.us.toml`. `tooie_utils.cpp` também interpreta relocations MIPS no
ELF. Esses dados devem ser convertidos; não duplicados manualmente.

## Tarefas

- [x] `P03-T01` Definir o schema de `tooie.us.rev0.toml` para a ROM decomposta.
- [x] `P03-T02` Criar gerador de `tooie.us.syms.toml` a partir do ELF e dos mapas BT.
- [x] `P03-T03` Criar gerador de símbolos de dados, exports e eventos necessários pelo runtime.
- [x] `P03-T04` Converter relocations MIPS suportadas e falhar explicitamente para tipos desconhecidos.
- [x] `P03-T05` Gerar `section_table` e a tabela de overlays por índice usada pelo runtime.
- [x] `P03-T06` Preservar overlays vazios, entrypoints e ordem original de índices.
- [x] `P03-T07` Mapear a tabela de syscalls de overlays para funções recompiladas.
- [x] `P03-T08` Adicionar testes de snapshot e invariantes para todos os arquivos gerados.
- [x] `P03-T09` Emitir relatório de cobertura: bytes executáveis classificados versus lacunas.

## Implementação

Projeto criado em `TooieRecomp/` (sem editar o código-fonte versionado de
`banjo-tooie-Recomp`).

| Peça | Caminho |
| --- | --- |
| Gerador | `TooieRecomp/tools/generate_metadata.py` |
| Testes | `TooieRecomp/tools/test_metadata_invariants.py` |
| Config dump (ELF) | `TooieRecomp/tooie.us.rev0.dump.toml` |
| Config recomp (syms) | `TooieRecomp/tooie.us.rev0.toml` |
| Símbolos de funções | `TooieRecomp/metadata/tooie.us.syms.toml` |
| Símbolos de dados | `TooieRecomp/metadata/tooie.us.datasyms.toml` |
| Seções relocáveis | `TooieRecomp/metadata/relocatable_sections.txt` |
| Overlays (índice estável) | `TooieRecomp/metadata/overlays.table.toml` |
| Syscalls | `TooieRecomp/metadata/syscalls.table.toml` |
| Cobertura | `TooieRecomp/metadata/coverage_report.md` |
| Snapshot | `TooieRecomp/tests/snapshots/metadata_snapshot.json` |

### Pipeline

```sh
# pré-requisitos: P01 (N64Recomp) + P02 (ELF + decompressed ROM)
python3 TooieRecomp/tools/generate_metadata.py
python3 TooieRecomp/tools/test_metadata_invariants.py
```

1. Valida SHA-1 da ROM decomposta (`af479771…`).
2. Extrai seções do ELF (`readelf -S`) e lista overlays em VRAM `0x80800000` como relocáveis.
3. Invoca `N64Recomp tooie.us.rev0.dump.toml --dump-context` → dump de funções/dados/relocs.
4. Falha se aparecer relocation fora de `{R_MIPS_HI16, R_MIPS_LO16, R_MIPS_26}`.
5. Converte `overlays.us.toml` e `syscall_symbol_addrs.us.txt` preservando ordem/índices (incl. vazios).
6. Escreve relatório de cobertura + `manifest.json`.

## Evidências (2026-08-09)

| Check | Resultado |
| --- | --- |
| `N64Recomp --dump-context` | exit 0 — “Dumping context” |
| Relocs | HI16=8066, LO16=8068, MIPS_26=11476; **0 desconhecidos** |
| Overlays | **884** slots, **33** empty — paridade com `overlays.us.toml` |
| Syscalls | **4234** entradas |
| Relocatable sections | **830** nomes únicos (VRAM 0x80800000) |
| Invariant tests | **PASS** |
| Logs | `docs/plan/evidence/p03-generate-metadata.log`, `p03-metadata-tests.log` |

> **Follow-up P08 (2026-08-10):** os **830** registrados na evidência original
> eram subestimados por truncamento de nome no `readelf -S` (sem `-W`). Nomes de
> seção com ≥ 18 caracteres eram truncados para 12 caracteres + `[...]`, e o
> subconjunto `relocatable_sections.recomp.txt` (750) descartava as seções
> longas — incluindo `.chcollballcontrol` — o que fez o N64Recomp descartar as
> relocations dessas seções. Após o diagnóstico P08, o parser foi corrigido com
> `-W` e o conjunto correto é **851**. Detalhe:
> [[../../plan/evidence/p08-overlay-data-relocation-diagnosis]].

## Critérios de qualidade

- [x] Saída ordenada e estável entre execuções (lista relocável sorted; overlays por índice).
- [x] Cada artefato aponta para fonte (ELF / overlays.us.toml / syscall map / N64Recomp dump).
- [x] Sem tabelas manuais paralelas de endereços de overlay.
- [x] Relocation desconhecida interrompe o gerador.

## Gate de saída

- [x] N64Recomp aceita a configuração **ELF** e percorre as seções no modo
  `--dump-context` sem erro de parsing / símbolo de entrypoint ausente
  (`boot_entrypoint` @ `0x80000400`).
- [x] Artefatos `tooie.us.syms.toml` + `tooie.us.rev0.toml` prontos para P04
  (recompilação com `symbols_file_path`).

> A recompilação completa de `RecompiledFuncs/` é o gate da **P04**, não desta etapa.

## Próxima etapa

[[04-static-recompilation]]
