---
title: "Baseline manifest — TooieRecomp P00"
description: "Ambiente, commits, remotes, ROM, decisões e licenças congelados na P00."
status: active
type: baseline
project: TooieRecomp
phase: P00
updated: 2026-08-09
recorded_at: 2026-08-09T04:01:30Z
source_path: banjo-tooie-Recomp
source_repo: wesleytakatsu/banjo-tooie-Recomp
upstream_repo: Mr-Wiseguy/banjo-tooie
tags:
  - baseline
  - tooierecomp
  - banjo-tooie-Recomp
---

# Baseline manifest — TooieRecomp P00

Registro reproduzível da máquina, fontes, ROM e decisões estruturais antes da
toolchain (P01). Validação automática: `scripts/validate-baseline.sh`.

## Host

| Campo | Valor |
| --- | --- |
| Distro | Linux Mint 22.2 (Zara), `UBUNTU_CODENAME=noble` |
| Kernel | `6.8.0-137-generic` x86_64 |
| Hostname | `wesley-H97-D3H` |
| CPU | Intel Core i5-4690K @ 3.50GHz (4 cores / 4 threads) |
| GPU 0 (discrete) | AMD Radeon RX 580 Series (RADV POLARIS10), 8 GiB |
| GPU 1 (iGPU) | Intel HD Graphics 4600 (HSW GT2) |
| Vulkan | Instance 1.3.275; GPU0 api 1.4.318, driver **radv** Mesa 25.2.8 |
| OpenGL | Mesa 25.2.8, renderer AMD Radeon RX 580, core 4.6 |
| DRM nodes | `card1`/`card2`, `renderD128`/`renderD129` |
| CMake | 3.28.3 |
| Ninja | 1.11.1 |
| Clang | 18.1.3 |
| GCC | 13.3.0 |
| Python | 3.12.3 |
| Git | 2.43.0 |

## ROM (fonte de dados do usuário)

| Campo | Valor |
| --- | --- |
| Arquivo primário | `Banjo-Tooie (USA).z64` (raiz do workspace) |
| Também presente | `Banjo-Tooie (USA).n64` (byte-swapped; **não** é fonte principal) |
| Tamanho | 33554432 (32 MiB) |
| Byte order | big-endian `.z64` magic `80371240` |
| Título header | `BANJO TOOIE` |
| Cart ID | `NB7E` |
| Revision | `00` |
| SHA-1 | `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3` |
| SHA-256 | `9ec37fba6890362eba86fb855697a9cff1519275531b172083a1a6a045483583` |
| SHA-1 decomposta (alvo Splat, P02) | `af479771ff5e91ce3732f3d84146614ab71d8d79` |

Qualquer divergência de hash, tamanho, magic ou revisão deve falhar **antes** do build.

## Git — banjo-tooie-Recomp (insumos BT)

| Campo | Valor |
| --- | --- |
| Path | `banjo-tooie-Recomp/` |
| Branch | `master` tracking `origin/master` |
| HEAD | `25033be6ec9e35975483f064d086391d2381ca4c` |
| Tip subject | Ported Matched Functions… (#45) |
| origin | `https://github.com/wesleytakatsu/banjo-tooie-Recomp.git` |
| upstream | `https://github.com/Mr-Wiseguy/banjo-tooie.git` |
| vs upstream/master | `0 ahead / 0 behind` |
| Worktree | limpo |
| Checkout legado `banjo-tooie/` | **ausente** (proibido no plano) |

### Gitlinks BT (inicializados na P01 — pins inalterados)

| Path | Commit fixado |
| --- | --- |
| `lib/ultralib` | `97c8b26282730fcec7109035e79e60a5158a8e2c` |
| `tools/asm-processor` | `8db39e1638bad6e532e664ffb8daa1e1abf96817` |
| `tools/elfio` | `8ae6cec5d60495822ecd57d736f66149da9b1830` |
| `tools/fmt` | `40626af88bd7df9a5fb80be7b25ac85b122d6c21` |
| `tools/rarezip` | `e22df74bc9497db23e67062b98711362de023f7e` |
| `tools/splat` | `78cf5dd483533eb22a6fc776d2e9b5d6c738fb61` |
| `tools/toml11` | `be08ba2be2a964edcdb3d3e3ea8d100abc26f286` |

## Git — BanjoRecomp (referência BK)

| Campo | Valor |
| --- | --- |
| Path | `BanjoRecomp/` |
| Branch | `main` tracking `origin/main` |
| HEAD | `ec859632cfa584e2272d32e46051543429a2baf4` |
| Tip subject | Bump version to 1.0.2 |
| origin | `https://github.com/BanjoRecomp/BanjoRecomp.git` |

### Gitlinks BK (inicializados na P01 — pins inalterados)

| Path | Commit fixado |
| --- | --- |
| `BanjoRecompSyms` | `6820055ca076e94e30e53d917bd9e5f71c28ca20` |
| `lib/N64ModernRuntime` | `ca568b6ad79b9029d14077f0c3ffa757727c5559` |
| `lib/RecompFrontend` | `d0d90ba49f46f4896aaeda362056c21b1e342561` |
| `lib/bk-decomp` | `351ca1580c10e550160ac11c77824fa9a498015e` |
| `lib/rt64` | `6f1c2d99a4ea571c139f449c326fd176ba8f3496` |

## Decisões arquiteturais (P00)

### P00-T04 — Nome e diretório do projeto

| Campo | Decisão |
| --- | --- |
| Nome do projeto | `TooieRecomp` |
| Nome do executável alvo | `TooieRecompiled` (Linux x86-64) |
| Path no workspace | `TooieRecomp/` (irmão de `BanjoRecomp/` e `banjo-tooie-Recomp/`) |
| Criação do tree | **depois** de fechar P00–P03 (conforme plano mestre) |

### P00-T05 — Dependências

- **Git submodules** com commits fixados (gitlinks do HEAD baseline); nunca “floating” na ponta remota sem revisão.
- **CMake + Ninja** para o runtime nativo e layout do executável.
- Makefile do decomp BT permanece a autoridade do pipeline MIPS/Splat/matching.
- Builds de runtime fora da árvore de fontes (`-B build/...`).

### P00-T06 — Camadas de requisito

| Camada | Conteúdo | Quando |
| --- | --- | --- |
| **Paridade obrigatória** | Boot, overlays, syscalls, áudio, saves, timing original, 4:3, campanha sem crash/softlock | Gates M2–M5 e definition-of-done |
| **Compatibilidade obrigatória** | Validação de ROM por hash, falha em símbolo/reloc/syscall/RSP desconhecido, sem assets no pacote | Desde M1/M6 |
| **Melhorias opcionais** | Widescreen, high FPS (sem acelerar física), UI extra, mods | **Somente após** paridade (P09) |

### P00-T08 — Política Git

| Remote / ação | Política |
| --- | --- |
| `origin` no fork BT | branches de implementação e pushes |
| `upstream` Mr-Wiseguy | sincronização e comparação somente leitura |
| BanjoRecomp `origin` | referência; não enviar código gerado por IA (restrição CONTRIBUTING upstream) |
| Workspace `N64-Recomp/` | meta-pasta de docs + insumos; não substitui os repos git internos |
| Comandos BT | sempre sob `banjo-tooie-Recomp/`; nunca recriar `banjo-tooie/` |

## Licenças e redistribuição (P00-T07)

| Componente | Licença / restrição observada | Redistribuição |
| --- | --- | --- |
| `BanjoRecomp` | `COPYING` = **GPL-3.0** | Código do port sob GPL-3; **sem** assets do jogo |
| `banjo-tooie-Recomp` / upstream decomp | Código de decomp; ROM **não** é redistribuível | Só código/ferramentas; usuário fornece ROM |
| `N64ModernRuntime`, `RecompFrontend`, `rt64`, tools | Licenças próprias nos submódulos (a copiar no pacote em P10) | Incluir textos de licença no release |
| ROM Banjo-Tooie | Propriedade da Nintendo / Rare | **Nunca** versionar, empacotar ou publicar |
| Assets (texturas, áudio, modelos) | Extraídos da ROM em runtime | **Nunca** no repositório ou no pacote |

Regra de ouro: o release contém apenas software livre/redistribuível + instruções para a ROM do usuário.

## Comandos de evidência (executados em 2026-08-09)

```sh
uname -a
cat /etc/os-release
lscpu
lspci | grep -iE 'vga|3d|display'
vulkaninfo --summary
glxinfo -B
cmake --version; ninja --version; clang --version; gcc --version; python3 --version; git --version

git -C banjo-tooie-Recomp remote -v
git -C banjo-tooie-Recomp status --short --branch
git -C banjo-tooie-Recomp rev-parse HEAD
git -C banjo-tooie-Recomp submodule status --recursive
git -C banjo-tooie-Recomp rev-list --left-right --count upstream/master...HEAD

git -C BanjoRecomp remote -v
git -C BanjoRecomp rev-parse HEAD
git -C BanjoRecomp submodule status --recursive

sha1sum "Banjo-Tooie (USA).z64"
sha256sum "Banjo-Tooie (USA).z64"
./scripts/validate-baseline.sh
```

## Relações

- Plano mestre: [[index]]
- Etapa: [[../plans/BT/00-baseline-and-decisions]]
- DoD: [[definition-of-done]]
- Prontidão BT: [[../specs/BT/recomp-readiness]]
