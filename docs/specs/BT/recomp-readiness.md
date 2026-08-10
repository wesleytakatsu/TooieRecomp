---
title: "BT — Prontidão para recompilação estática"
description: "Inventário dos artefatos existentes no banjo-tooie-Recomp, contratos para N64Recomp e lacunas do futuro TooieRecomp."
status: current
type: specification
project: banjo-tooie-Recomp
updated: 2026-08-09
source_path: banjo-tooie-Recomp
source_repo: wesleytakatsu/banjo-tooie-Recomp
upstream_repo: Mr-Wiseguy/banjo-tooie
tags: [banjo-tooie-Recomp, n64recomp, readiness, symbols, overlays]
---

# BT — Prontidão para recompilação estática

O decomp BT já produz a maior parte dos metadados difíceis de uma recompilação
estática. Isso reduz risco, mas não fornece runtime nativo automaticamente.

A fonte ativa é o fork local `banjo-tooie-Recomp/`, no commit
`25033be6ec9e35975483f064d086391d2381ca4c`. O fork está alinhado ao upstream
nessa baseline, porém será o local autorizado para branches e implementações.
O upstream permanece referência para sincronização. Veja [[setup|Setup BT]].

## ROMs e hashes

| Artefato | Papel | SHA-1 esperado |
| --- | --- | --- |
| `baserom.us.z64` | ROM original NTSC-U Rev. 0 fornecida pelo usuário | `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3` |
| `decompressed.us.z64` | alvo decomposto consumido pelo Splat | `af479771ff5e91ce3732f3d84146614ab71d8d79` |

A ROM validada no workspace corresponde ao primeiro hash, tem 32 MiB, byte
order `.z64`, ID `NB7E` e revisão `00`.

## Insumos definidos pelo fork

| Insumo | Fonte | Uso no recomp |
| --- | --- | --- |
| Decompressor e compressor | `tools/rom_decompressor.cpp`, `rom_compressor.cpp` | obter layout decomposto e validar matching |
| Splat config | `baserom.us.yaml` | seções, VRAM, ROM, BSS e subsegmentos |
| ELF MIPS | build do decomp | símbolos, seções e relocations estruturadas |
| Símbolos | `symbol_addrs.us.txt` e variantes | delimitar funções e dados |
| Relocations | `reloc_addrs.us.txt` e variantes | reconstruir referências MIPS |
| Overlays | `overlays.us.toml` e gerador | índices, entrypoints e módulos vazios |
| Syscalls | `syscall_symbol_addrs.us.txt` | dispatch entre módulos |
| Código matching | `src/`, `asm/` e linker | nomes, estruturas e validação binária |

O decompressor possui rotinas que imprimem símbolos/relocations e geram a
configuração de overlays. `get_tooie_relocs` interpreta relocations diretamente
do ELF. Esses caminhos devem ser estendidos para emitir entradas N64Recomp em
vez de manter tabelas paralelas.

Na clonagem verificada, os arquivos-fonte e configurações acima existem, mas
os sete submódulos não estão inicializados e os artefatos de `make setup`/build
ainda não foram materializados. Portanto, “disponível” nesta especificação não
significa “já gerado”; P01 e P02 precisam provar isso com hashes e logs.

## Contrato de overlay

O loader original:

- localiza o overlay por índice na tabela ROM;
- lê tamanho, BSS e flags do header;
- descomprime quando necessário;
- desfaz a proteção XOR baseada em CRC;
- limpa BSS e registra o módulo carregado;
- suporta unload e movimentação durante defragmentação.

O runtime nativo pode mapear código para funções host, mas precisa preservar
índice, entrypoints, relocations, BSS, ciclo de vida e semântica de syscalls.

## Lacunas para um port nativo

Estado parcial (2026-08-09): P00–P05 fechados no workspace; **P06 parcial**
(syscall dispatch nativo em runtime — ver [[syscalls]] e
[[../../plans/BT/06-overlays-and-syscalls]]). Ainda aberto:

- ~~Configuração `tooie.us.rev0.toml` do N64Recomp.~~ (feito em P03/P04)
- ~~Conversão determinística dos mapas/ELF para tabelas do runtime.~~ (P03)
- ~~Código nativo gerado e build compile-only.~~ (P04)
- ~~Executável Tooie com launcher/RT64 e ROM descomprimida.~~ (P05; cart→decomp residual)
- ~~Registro de overlays/syscalls no caminho trampoline.~~ (P06 dispatch; falta `ovl_load` heap)
- Inventário e recompilação dos microcódigos RSP realmente usados (P07).
- Patches de compatibilidade restantes (PI/SI/KSEG1 parciais em `boot_hw_stubs.cpp`).
- Validação de RT64, áudio, input, saves, timing e campanha completa.
- Empacotamento Linux sem conteúdo proprietário.
- Scheduler/VI além do busy-loop pós-primeiros syscalls.

## Critérios de prontidão do recompilador

- ELF matching e hashes reproduzíveis.
- Cobertura mensurável de toda seção executável.
- Zero relocation ou syscall ignorada silenciosamente.
- Ordem e índice dos overlays preservados, incluindo entradas vazias.
- Geradores têm saída ordenada e testes de snapshot.
- Cada endereço gerado pode ser rastreado até ROM, ELF ou mapa de símbolos.

## Fontes locais

- [`README.md`](../../../../banjo-tooie-Recomp/README.md)
- [`baserom.us.yaml`](../../../../banjo-tooie-Recomp/baserom.us.yaml)
- [`tools/rom_decompressor.cpp`](../../../../banjo-tooie-Recomp/tools/rom_decompressor.cpp)
- [`tools/tooie_utils.cpp`](../../../../banjo-tooie-Recomp/tools/tooie_utils.cpp)
- [`src/core2/overlays.c`](../../../../banjo-tooie-Recomp/src/core2/overlays.c)
- [`overlays.us.toml`](../../../../banjo-tooie-Recomp/overlays.us.toml)

## Relações

- Índice BT: [[index]]
- Símbolos: [[symbols]]
- Overlays: [[overlays]]
- Metadados a implementar: [[../../plans/BT/03-recompiler-metadata]]
