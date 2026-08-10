---
title: "BT P00 — Baseline e decisões"
description: "Congela ambiente, fontes, ROM e decisões antes da implementação do TooieRecomp."
status: done
type: implementation-stage
project: TooieRecomp
phase: P00
order: 0
updated: 2026-08-09
source_path: banjo-tooie-Recomp
tags: [banjo-tooie-Recomp, baseline, decisions]
---

# BT P00 — Baseline e decisões

## Resultado esperado

Uma baseline reproduzível: máquina, commits, submódulos, ROM, escopo inicial e
licenças identificados sem ambiguidade.

## Contexto verificado

- Host Linux x86-64 com CMake, Ninja, Clang, GCC, Python e Git.
- ROM `.z64` com SHA-1 `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`.
- A ROM tem 32 MiB, título `BANJO TOOIE`, ID `NB7E` e revisão `00`.
- `BanjoRecomp` e o novo fork local `banjo-tooie-Recomp` estão presentes.
- O fork BT está limpo em `master`, acompanhando `origin/master`, no commit
  `25033be6ec9e35975483f064d086391d2381ca4c`.
- `origin` aponta para `wesleytakatsu/banjo-tooie-Recomp`; `upstream` aponta
  para `Mr-Wiseguy/banjo-tooie`; a baseline atual está 0 ahead/0 behind.
- Os sete submódulos BT estão declarados, mas ainda não inicializados.
- O checkout local antigo `banjo-tooie/` não existe e não faz parte do plano.

## Tarefas

- [x] `P00-T01` Registrar distribuição, kernel, CPU, GPU, driver e APIs Vulkan/OpenGL disponíveis.
- [x] `P00-T02` Registrar commits, branch, tracking e remotes exatos de `BanjoRecomp`, do fork `banjo-tooie-Recomp` e de todos os submódulos.
- [x] `P00-T03` Registrar SHA-1/SHA-256 da ROM e rejeitar automaticamente qualquer entrada divergente.
- [x] `P00-T04` Definir o diretório e o nome do novo projeto `TooieRecomp`.
- [x] `P00-T05` Escolher estratégia de dependências: submódulos fixados e CMake como baseline.
- [x] `P00-T06` Separar requisitos de paridade, compatibilidade obrigatória e melhorias opcionais.
- [x] `P00-T07` Registrar licenças e restrições de redistribuição de cada componente.
- [x] `P00-T08` Definir a política Git: branches e pushes no `origin` do fork; sincronização somente leitura a partir de `upstream`.

## Artefatos

- Manifesto de versões e hashes: [[../../plan/baseline-manifest]]
- Registro de decisões arquiteturais: mesma nota (seções P00-T04..T08)
- Manifesto Git com remotes, branch, HEAD, gitlinks: [[../../plan/baseline-manifest]]
- Script somente leitura: `scripts/validate-baseline.sh`

## Verificação

O manifesto deve permitir que outra sessão identifique exatamente os mesmos
fontes e a mesma ROM. A validação deve falhar antes do build diante de hash ou
revisão incorretos.

Comandos mínimos de evidência para o fork:

```sh
git -C banjo-tooie-Recomp remote -v
git -C banjo-tooie-Recomp status --short --branch
git -C banjo-tooie-Recomp rev-parse HEAD
git -C banjo-tooie-Recomp submodule status --recursive
git -C banjo-tooie-Recomp rev-list --left-right --count upstream/master...HEAD
./scripts/validate-baseline.sh
```

## Evidências

Registrado em 2026-08-09T04:01:30Z. Detalhes completos em
[[../../plan/baseline-manifest]].

| Item | Resultado |
| --- | --- |
| Host | Linux Mint 22.2 / kernel 6.8.0-137 / i5-4690K / RX 580 (RADV Mesa 25.2.8) + iGPU HSW |
| Toolchain host | CMake 3.28.3, Ninja 1.11.1, Clang 18.1.3, GCC 13.3.0, Python 3.12.3, Git 2.43.0 |
| BT HEAD | `25033be6ec9e35975483f064d086391d2381ca4c` (0/0 vs upstream/master) |
| BK HEAD | `ec859632cfa584e2272d32e46051543429a2baf4` |
| ROM SHA-1 | `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3` |
| ROM SHA-256 | `9ec37fba6890362eba86fb855697a9cff1519275531b172083a1a6a045483583` |
| `./scripts/validate-baseline.sh` | **PASS** (exit 0); 7 submódulos BT ainda não inicializados (esperado até P01) |
| Projeto path | `TooieRecomp/` (criação adiada a P03+); executável alvo `TooieRecompiled` |

## Gate de saída

- [x] Baseline versionada e revisada.
- [x] Fork, upstream e commit-base identificados sem depender de `banjo-tooie/`.
- [x] Nenhuma decisão estrutural pendente para iniciar o toolchain.
- [x] ROM confirmada sem renomear o `.n64` byte-swapped como fonte principal.

## Próxima etapa

[[01-linux-toolchain]]
