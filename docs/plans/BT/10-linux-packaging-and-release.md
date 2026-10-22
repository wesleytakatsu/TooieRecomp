---
title: "BT P10 — Empacotamento e release Linux"
description: "Produz, testa e publica um artefato Linux reproduzível sem conteúdo proprietário do jogo."
status: pending
type: implementation-stage
project: TooieRecomp
phase: P10
order: 10
depends_on: P09
tags: [banjo-tooie-Recomp, linux, packaging, release, candidate]
---

# BT P10 — Empacotamento e release Linux

## Resultado esperado

Um pacote instalável/portátil para Linux x86-64 que pede a ROM do usuário,
valida sua revisão e executa sem toolchain de desenvolvimento.

> [!WARNING]
> **Fase P10 não encerrada (`status: pending`):** O trabalho recente produziu e validou um pacote candidato privado local (`LINUX_CANDIDATE_PACKAGE_VERIFIED`), mas não constitui uma release de produção aprovada. O gate técnico de interpolação P09 permanece reprovado (`FAILED`), a completude do repositório é parcial (dependências uncommitted), e o build nativo do Windows 11 x64 permanece pendente (`WINDOWS_BUILD_PENDING_NATIVE_HOST`).

## Tarefas

- [ ] `P10-T01` Definir artefato primário Linux e um formato secundário apenas se necessário.
  - *Evidência preliminar (2026-10-21):* Tarball portátil `TooieRecompiled-linux-x86_64-73bfd43-dirty-private-local.tar.gz` (SHA-256 `4f702d2ba0a0472345f6f1317bfe75eb0df462e96b2352d6d875edaa5c4ebcf9`, 104 arquivos regulares no tar, `tools/verify_package.sh` exit 0, zero ROM, UI exata; supersedendo os interinos `b6475879…`, `64f130aa…` e `a327666f…`) gerado e verificado por `tools/package_linux.sh` e `tools/verify_package.sh`. AppImage / Flatpak seguem pendentes.
- [ ] `P10-T02` Instalar executável, bibliotecas permitidas, shaders, fontes, controller DB e metadados.
  - *Evidência preliminar (2026-10-21):* `tools/package_linux.sh` empacota binário restaurado (`0cfcd792…`), `assets/` (40 arquivos de recursos de interface restaurados), `ui_assets/TooieLogo.svg` (7300x2450), `recompcontrollerdb.txt`, `README-PACKAGE.txt`, `THIRD-PARTY-MANIFEST.tsv` e arquivos de licença em `licenses/`. Integridade verificada por `tools/verify_package.sh` (exit 0).
- [ ] `P10-T03` Implementar seleção inicial e cache seguro da ROM com validação por hash.
- [ ] `P10-T04` Garantir que ROM e assets extraídos nunca entrem no pacote ou cache de CI.
  - *Evidência preliminar (2026-10-21):* Auditoria automatizada do tarball (`AUDIT.txt`) confirmou zero arquivos `.z64/.n64/.v64/.rom/.elf/.sav/.eep/.log/.json` e ausência de `baserom*`.
- [ ] `P10-T05` Adicionar desktop file, ícone, licença, créditos e notas de compatibilidade.
  - *Evidência preliminar (2026-10-21):* Criada especificação canônica [[../../specs/Tooie/asset-and-licensing-manifest|Asset and Licensing Manifest]]; regras em `tools/package_licenses.tsv`; licença formal do TooieRecomp permanece decisão do proprietário.
- [ ] `P10-T06` Construir em ambiente Linux limpo e testar em GPU/driver diferente quando disponível.
  - *Evidência preliminar (2026-10-21):* Construído com Clang + Ninja (`build_linux.sh`, 1159 passos, exit 0). Binário auditado SHA-256 `0cfcd79206efc74b74e26836ce468c69bcadaf2c3950b40eb31971946026892b` (rebuild da UI restaurada exit 0, visual do launcher inspecionado). 8 testes de unidade host executados com PASS. Preflight kit-free do root exit 0. Prova limpa kit-free (neutral 5c2) separada do build da UI restaurada.
- [ ] `P10-T07` Executar smoke test automatizado do pacote instalado e do modo portátil.
  - *Evidência preliminar (2026-10-21):* `tools/smoke_package.sh` executou o pacote corrigido extraído (`TooieRecompiled-linux-x86_64-nogit-private-local.tar.gz`, SHA-256 `b6475879582aba0fc8b2d3c5d7b39805f08912fb272c71b17c67a959b8d1fa74`) em sandbox isolada (HOME/XDG limpos) no driver RADV/X11 por 20 segundos; processo permaneceu vivo e enviou SIGTERM ao próprio PID (exit 0). Validado tanto sob loader padrão (mapeando libz host shadow) quanto sob override do sistema (`TOOIE_SMOKE_LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu`). *Limitação:* Teste de liveness apenas; renderização in-game e gameplay não foram avaliados.
- [ ] `P10-T08` Gerar SBOM/lista de dependências e checksums do release.
  - *Evidência preliminar (2026-10-21):* Gerados `MANIFEST.sha256` (100 arquivos no pacote, 8266 na árvore vendorizada), `BUILD-INFO.txt`, `THIRD-PARTY-MANIFEST.tsv` e `DEPENDENCIES.txt`. Correção F1 elimina avisos de ldconfig; correção F5 remove metadado `.gitrepo`. Mapeamento de `libz.so.1` validado para o pacote de distribuição `zlib1g 1:1.3.dfsg-3.1ubuntu2.2` com nota de `HOST-SHADOW` sobre a biblioteca local.
- [ ] `P10-T09` Publicar instruções de diagnóstico e coleta de logs sem dados pessoais.
  - *Evidência preliminar (2026-10-21):* Vazamento de paths `/home/wesley` no binário resolvido pelo patch de snapshot portátil (0 strings `/home/` no executável). Política C2: 10 arquivos de configuração de agentes/MCP rastreados no HEAD do git que contêm caminhos locais são explicitamente excluídos do arquivo fonte (prova histórica pré-documentação `4aab8d21...`; entrega final em `dist/standalone-20261021/TooieRecomp-source-73bfd430-dirty.tar.gz`, checksum no sidecar).
- [ ] `P10-T10` Fechar todos os gates de [[../../plan/definition-of-done]].

## Gate de saída

- Build reproduzível a partir de checkout limpo. *(PARCIAL: provado offline em árvore candidata standalone via `tools/build_linux.sh`; requer commit do delta)*
- Pacote inicia em máquina sem dependências de desenvolvimento. *(PENDENTE: requer teste em máquina sem boosteroid/usr-local zlib)*
- Auditoria confirma ausência de ROM e assets do jogo. *(VALIDADO no pacote candidato)*
- Checksum, licenças, instruções e matriz de compatibilidade acompanham o release. *(PARCIAL)*
- Todos os critérios da definição de pronto estão atendidos. *(PENDENTE: P08/P09 abertos)*

## Evidência e Histórico de Descobertas (2026-10-21)

Registrado em [[../../plan/evidence/p10-windows-linux-clean-clone-readiness-20261021]]:
- **F-A:** `ultralib/tools/compile_sjis.py` falha se o caminho de checkout contiver `.c` (`FileNotFoundError`). Contornado via preflight.
- **F-B (histórico; script aposentado — patches agora integrados em `lib/`, ver `vendor/`):** `apply_runtime_patches.sh` não era idempotente para patches empilhados; corrigido com marcador sha em `.git/`.
- **F-C:** Shims `_WIN32` para `__real_*` adicionados em `backpack_lookup_diag.cpp` para evitar 18 símbolos indefinidos do GNU `--wrap`.
- **F-D:** Headers Windows limpos (`<unistd.h>`, `getpid()`, `mkdir`).
- **F-E:** Dois paths `/home/wesley` no patch de snapshot foram compilados no binário (Blocker B4).
- **F-F:** 10 PNGs de gameplay em `docs/plan/evidence/` pertencem à história de RE e não entram no pacote.
- **F-G (atualizado 2026-10-21):** Conforme esclarecimento formal do usuário, a restrição de recursos livres/próprios se aplica estritamente aos dados do jogo extraídos da ROM (texturas, áudio, geometria, scripts). Todos os assets de menu, artes vetoriais (`Banjo.svg`, `Kazooie.svg`, `Cloud*.svg`, `Jiggy*.svg`), animações, ícones, fontes tipográficas (`Suplexmentary Comic NC.ttf`, `InterVariable.ttf`, `NotoEmoji-Regular.ttf`, `promptfont.ttf`) e o logo Tooie original (7300x2450) foram preservados e restaurados localmente em `assets/` e `ui_assets/`; o neutral UI interino foi superado. Zero dados do jogo são empacotados.
- **F-I (atualizado 2026-10-21):** Pipeline de geração 100% kit-free validado por Sonnet (`status: KITFREE_VALIDATED`, 420 arquivos de `RecompiledFuncs` gerados byte a byte idênticos à rota ELF; build limpo 1159 passos exit 0 em sandbox bubblewrap sem kit/rede/ELF; rebuild final com UI restaurada em andamento pelo root).
- **F-H:** Inventário e verificação independente pelo Claude Haiku 5.5 em `gate.json` (`accepted: false` para Windows e completude do repo).

## Encerramento

Atualizar [[../../plan/index]] para `done`, congelar as versões do marco M6 e
preservar a matriz de evidências da release.
