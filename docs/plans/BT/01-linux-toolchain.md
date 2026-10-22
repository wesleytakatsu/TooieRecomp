---
title: "BT P01 — Toolchain Linux"
description: "Instala e valida compiladores, bibliotecas, Python e submódulos usados pelos dois projetos."
status: done
type: implementation-stage
project: TooieRecomp
phase: P01
order: 1
depends_on: P00
updated: 2026-08-09
source_path: banjo-tooie-Recomp
tags: [banjo-tooie-Recomp, linux, toolchain, dependencies]
---

# BT P01 — Toolchain Linux

> [!NOTE]
> **Histórico / Fase Concluída:** Esta fase inicial (agosto de 2026) utilizou a toolchain do repositório de referência BanjoRecomp para validar as ferramentas de recompilação. A partir de 2026-10-21 (fase P10), o projeto TooieRecomp tornou-se completamente **standalone**: todas as dependências e ferramentas vivem vendorizadas em `lib/`, `decomp/`, `assets/` e `tools/wheelhouse/`. Não é necessário nem suportado clonar ou compilar o repositório BanjoRecomp. Os comandos abaixo são preservados estritamente como registro histórico da validação original da toolchain. Para compilar o projeto atual, consulte [`README.md`](../../../README.md) e [`tools/BUILDING.md`](../../../tools/BUILDING.md).

## Resultado esperado

Um ambiente Linux capaz de construir o decomp Tooie, gerar código com
N64Recomp/RSPRecomp e compilar o runtime nativo.

## Entradas

- [[../../specs/BT/setup|Setup BT]]
- [[../../specs/BK/build|Build BK]]
- Manifesto produzido em [[00-baseline-and-decisions]]
- Fork BT em `banjo-tooie-Recomp/`, com sete submódulos inicialmente não
  inicializados

## Tarefas

- [x] `P01-T01` (Histórico 2026-08) Executar `git -C banjo-tooie-Recomp submodule update --init --recursive` e inicializar também os submódulos de `BanjoRecomp` (SUPERSEDED em 2026-10-21: repositório tornou-se standalone com fontes vendorizados em `lib/` e `decomp/`; submódulos eliminados).
- [x] `P01-T02` Instalar `libfmt-dev`, `libtoml11-dev` e `gcc-mips-linux-gnu`.
- [x] `P01-T03` Instalar `cmake`, `ninja-build`, `libsdl2-dev`, `libgtk-3-dev`, `lld`, `llvm` e `clang`.
- [x] `P01-T04` Criar ambiente Python isolado e instalar os requirements do Splat e do projeto BT.
- [x] `P01-T05` Confirmar compiladores MIPS `gcc`, `as`, `ld` e `objcopy` com o prefixo esperado pelo Makefile.
- [x] `P01-T06` Construir as ferramentas auxiliares do decomp e executar seus smoke tests.
- [x] `P01-T07` Configurar CMake/Ninja do runtime em build fora da árvore de fontes.
- [x] `P01-T08` Registrar versões efetivas e diferenças em relação às versões de CI upstream.
- [x] `P01-T09` Confirmar que cada submódulo BT está no gitlink fixado pelo commit `25033be6ec9e35975483f064d086391d2381ca4c`, sem atualizar para a ponta de sua branch remota.

## Verificação

Executar checks de versão e builds mínimos para host C++, alvo MIPS, Python e
CMake. Um teste deve provar que um objeto MIPS pode ser montado, ligado e
inspecionado pelo `objcopy`.

## Evidências

Registrado em 2026-08-09. Logs e dumps em `docs/plan/evidence/`.

| Check | Resultado |
| --- | --- |
| Submódulos BT (7) | pins exatos do baseline; 0 não inicializados |
| Submódulos BK (+ nested) | init recursive OK; top-level nos gitlinks |
| Python venv | `.venv/` com freeze versionado |
| MIPS smoke | `mips-linux-gnu-gcc` (como `AS` do Makefile) → `.o` ELF32 MSB MIPS-III → `ld` → `objcopy` |
| Tools BT | `rom_decompressor`, `rom_compressor`, `overlay_processor`, `syscall_builder`, `n64crc`, `ido/cc` |
| N64Recomp / RSPRecomp | build Release out-of-tree **exit 0** |
| CMake BanjoRecomp | detecta Clang/SDL2/GTK/Freetype/Vulkan; **generate falha** sem `RecompiledFuncs/` e `rsp/n_aspMain.cpp` (esperado até P04) |

### Comandos reproduzíveis históricos (agosto de 2026 — NÃO EXECUTAR no TooieRecomp standalone)

```sh
# T01 / T09
git -C banjo-tooie-Recomp submodule update --init --recursive
git -C BanjoRecomp submodule update --init --recursive
git -C banjo-tooie-Recomp submodule status --recursive

# T02 / T03
sudo ./scripts/install-p01-deps.sh

# T04
python3 -m venv .venv
source .venv/bin/activate
pip install -U pip wheel
pip install -r banjo-tooie-Recomp/tools/splat/requirements.txt
pip install -r banjo-tooie-Recomp/tools/requirements.txt   # pin final spimdisasm==1.35.0

# T05 (AS do Makefile é mips-linux-gnu-gcc, não binutils-as puro)
mips-linux-gnu-gcc -march=vr4300 -mabi=32 -mgp32 -mfp32 -mips3 -mno-abicalls -G0 -fno-pic \
  -c -x assembler-with-cpp -o smoke.o smoke.s
mips-linux-gnu-ld -o smoke.elf smoke.o
mips-linux-gnu-objcopy -O binary smoke.elf smoke.bin

# T06
make -C banjo-tooie-Recomp/tools -j$(nproc)

# T07 — recompiler tools
cmake -S BanjoRecomp/lib/N64ModernRuntime/N64Recomp -B BanjoRecomp/lib/N64ModernRuntime/N64Recomp/build \
  -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build BanjoRecomp/lib/N64ModernRuntime/N64Recomp/build -j$(nproc)

# T07 — runtime (falha esperada de generate sem código recompilado)
cmake -S BanjoRecomp -B BanjoRecomp/build-cmake \
  -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### Artefatos de evidência

| Arquivo | Conteúdo |
| --- | --- |
| `docs/plan/evidence/p01-python-freeze.txt` | `pip freeze` do venv |
| `docs/plan/evidence/p01-toolchain-versions.txt` | versões host + MIPS + dpkg |
| `docs/plan/evidence/p01-tools-build.log` | log `make -C tools` |
| `docs/plan/evidence/p01-cmake-configure.log` | configure BanjoRecomp (falha generate esperada) |
| `docs/plan/evidence/p01-n64recomp-cmake.log` | configure N64Recomp |
| `docs/plan/evidence/p01-n64recomp-build.log` | build N64Recomp/RSPRecomp |
| `docs/plan/evidence/p01-submodule-pins.txt` | dump completo de pins |
| `scripts/install-p01-deps.sh` | instalação apt alinhada ao CI BT + deps BK |

### Versões vs CI (P01-T08)

| Item | Host (Mint 22.2 / noble) | CI BT (`ubuntu-latest`) | Nota |
| --- | --- | --- | --- |
| MIPS GCC | 12.4.0 (`gcc-mips-linux-gnu` 4:12.2.0-4 meta) | `gcc-mips-linux-gnu` + `binutils-mips-linux-gnu` | mesmo pacote Ubuntu; versão flutuante do runner |
| libfmt | 9.1.0 (system) + fmt 11.2.0 submodule tools | `libfmt-dev` | tools linkam `-lfmt` system e compilam `format.cc` do submodule |
| Python deps | venv local; `spimdisasm==1.35.0` (pin BT) | `pip3 install -r tools/requirements.txt` + splat | ordem: splat depois tools **rebaixa** spimdisasm 1.42→1.35 |
| CMake / Ninja / Clang | 3.28.3 / 1.11.1 / 18.1.3 | implícito no image | suficientes para N64Recomp e detect do runtime |
| SDL2 | 2.30.0 dev | BK CI default input `2.30.3` | host um pouco mais antigo; configure OK |
| N64Recomp commit | `2b6f056…` (submodule) | BK validate default `2b6f056…` | **igual** ao default do CI BK |

## Gate de saída

- [x] Todos os submódulos no commit fixado e sem prefixo de não inicializado.
- [x] Nenhum comando ou script depende do diretório excluído `banjo-tooie/`.
- [x] Ferramentas do decomp e recompiler compilam sem workaround manual não documentado.
- [x] Ambiente Python recriável a partir de arquivos versionados (`tools/requirements.txt` + `tools/splat/requirements.txt`).
- [x] Nota: generate completo do executável BanjoRecomp permanece bloqueado até existirem fontes recompiladas (P04+) — o gate de toolchain considera configure/detect + build de N64Recomp/RSPRecomp.

## Próxima etapa

[[02-rom-decompression-and-elf]]
