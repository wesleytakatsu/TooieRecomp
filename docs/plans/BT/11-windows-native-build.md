---
title: "BT P11 — Build nativo independente no Windows 11 x64"
description: "Pipeline nativo no Windows 11 x64 para compilação, extração e empacotamento sem WSL e sem checkouts externos."
status: active
type: implementation-stage
project: TooieRecomp
phase: P11
order: 11
depends_on: P10
updated: 2026-10-22
tags: [banjo-tooie-Recomp, windows, build, packaging, n64recomp, clang-cl]
---

# BT P11 — Build nativo independente no Windows 11 x64

Objetivo: Linux e Windows compilam e empacotam de forma independente, sem WSL, sem transferir arquivos gerados e sem checkout irmão. Os dois SOs devem produzir a mesma saída gerada (`RecompiledFuncs/`) a partir da mesma ROM.

Revisão 2 (2026-10-22): auditoria do código e do host feita pelo orquestrador (Claude Opus 5.5), só leitura, nada compilado. Esta revisão corrige premissas da revisão 1 e define quem executa cada tarefa (seção "Orquestração").

## Estado atual verificado

Código e CMake:
- Os ramos `_WIN32` já existem para janela, config, áudio (WASAPI), memória pré-carregada e shims de `--wrap`. Todo o código POSIX de `src/` está protegido por `#if defined(__linux__)`:
  - A música de preview do launcher no Windows é um stub que devolve `false` (`src/main/main.cpp:221-224`). Compila, mas não toca.
  - O caminho do executável no Windows usa `GetModuleFileNameW` (`src/main/main.cpp:771`), e os assets são resolvidos pelo `recompui::file::get_program_path()`.
  - `tooie_diag.cpp` e `transform_hooks.cpp` gravam em `/tmp/...` sem guarda. Só diagnóstico: no Windows o `fopen` falha em silêncio.
- O CMake recusa compilador que não seja Clang (`CMakeLists.txt:26`), trata o SDL2 vendorizado, o `dxc.exe`, o `/OPT:NOICF` e a cópia de DLLs.
- `RecompiledFuncs` usa flags no estilo GNU (`-falign-functions=16` e outras em `CMakeLists.txt:99-104`), que o driver `clang-cl` pode não aceitar sem o prefixo `/clang:`.

Pipeline:
- Extract, recomp, test e package só existem em bash. `Build-Tooie.ps1` tem os estágios `Check | ImportGenerated | Build | Package`, mas nunca rodou num Windows.
- `rsp/n_aspMain.cpp` é versionado, então o RSPRecomp não faz parte do build normal (fica para o mantenedor). A revisão 1 falava em "RSPRecomp se aplicável": fora de escopo.
- O gate de saída gerada é `tools/verify_generated.sh`: 420 arquivos contra `metadata/recomp-output.sha256`. `check_generated_fresh.py` só compara mtimes.

Três defeitos novos que bloqueiam o critério byte-idêntico (não estavam na revisão 1):
1. **O N64Recomp grava os `.c`/`.h` gerados em modo texto** (`lib/N64ModernRuntime/N64Recomp/src/main.cpp:118,161,162,483,605,901,920`, `std::ofstream` sem `std::ios::binary`). No Windows a saída sai com CRLF, e os hashes nunca vão bater com o manifesto Linux. A correção é um patch versionado que abre em modo binário (sem efeito no Linux).
2. **Não existe zlib vendorizado.** A revisão 1 citava "zlib do RT64", que não existe. `rom_decompressor.cpp` usa `inflateInit2(-MAX_WBITS)`/`inflate`. O que está vendorizado é o miniz (`lib/N64ModernRuntime/thirdparty/miniz`), com API compatível com zlib. O fmt está em `decomp/tools/fmt`.
3. **Este clone usa `core.autocrlf=true`.** 476 arquivos estão LF no índice e CRLF no disco, entre eles `metadata/recomp-output.sha256`, `metadata/tooie.us.syms.toml`, `metadata/recomp_stubs.txt`, `tooie.us.rev0.recomp.toml`, `RecompiledPatches/*`, `rsp/n_aspMain.cpp` e `CMakeLists.txt`. O `.gitattributes` atual protege só `*.sh`, `lib/`, `decomp/`, `assets/` e `vendor/`. Por isso `vendor_verify.sh` passa (8266 arquivos, OK), mas a entrada do gerador e o manifesto de saída ficam corrompidos.
   - `git ls-files --eol` não mostra nenhum arquivo `i/crlf` fora de caminhos `-text`, então `* text=auto eol=lf` não muda o conteúdo de nenhum blob.

Host (este PC, 2026-10-22):

| Item | Estado |
|------|--------|
| Visual Studio | 18 Community com o componente CMake (`VC.CMake.Project`). **Falta o `Microsoft.VisualStudio.Component.VC.Llvm.Clang`** |
| `cmake`, `ninja`, `clang-cl` | Ausentes do PATH comum. Aparecem só no Developer shell, e o clang só depois de instalar o componente |
| Windows SDK | 10.0.26100.0 presente |
| Python | `python` = 3.13.7. **`python3` aponta para o stub da Microsoft Store** (`WindowsApps\python3.exe`): os scripts não podem chamar `python3` no Windows |
| Git | 2.51 com Git Bash. `vendor_verify.sh` passa no Git Bash, e as checagens de exec-bit são puladas como já está documentado |
| WSL | Presente, mas usado só como diagnóstico opcional, nunca no caminho do build |
| ROM | `Banjo-Tooie (USA).z64` na raiz deste clone (git-ignored) |

Regras do projeto que valem aqui (ver `AGENTS.md`):
- Não editar `RecompiledFuncs/` à mão.
- Não commitar ROM nem saída gerada.
- Manter o vendor byte a byte (`vendor/MANIFEST.sha256`).
- Toda mudança em `lib/`/`decomp/` vira `vendor/patches-local/*.patch`, com o manifesto atualizado por `tools/vendor_manifest.sh`.
- Nunca marcar `PASS` sem ter rodado.
- Um objetivo por vez.
- Sem git destrutivo.

## Decisões tomadas (usuário, 2026-10-22)

| Decisão | Escolha |
|---------|---------|
| CRLF | Os agentes trabalham num **clone novo** `E:\projetos\TooieRecomp-win` criado com `core.autocrlf=false`. Este clone (`E:\projetos\TooieRecomp`) fica intacto. O `.gitattributes` ganha `* text=auto eol=lf` para que qualquer clone futuro fique LF |
| Local do plano | O plano vai para o vault como fase `docs/plans/BT/11-windows-native-build.md`, ligado ao index e ao HANDOFF, e este arquivo na raiz é removido |
| Git Bash | Permitido no build normal **só** para scripts que já funcionam nele (`vendor_verify.sh`, `package_manifest.sh`). Lógica nova (rom_check, extract, recomp, verify) é feita em Python, chamada igual pelo `.sh` e pelo `.ps1` |

Decisões do orquestrador (podem ser revistas):
- O critério byte-idêntico é atingido corrigindo o gerador (modo binário), não normalizando a comparação.
- A ROM não é copiada para o clone novo: `TOOIE_ROM` aponta para o arquivo existente.
- `ImportGenerated` continua como fallback até a W5 passar e depois é marcado como obsoleto.
- Os scripts Linux (`*.sh`) não são reescritos nesta fase: os novos `.py` são paralelos. Unificar (`.sh` chamando `.py`) é tarefa opcional da W6.

## Critério de pronto (global)

- `Build-Tooie.ps1 -Stage All` num Windows 11 (só VS Build Tools com Clang, Git for Windows e Python 3.8+) gera `TooieRecompiled.exe` a partir da ROM, sem WSL.
- `RecompiledFuncs/` gerado no Windows tem os 420 arquivos byte-idênticos a `metadata/recomp-output.sha256`.
- Testes de host passam nos dois SOs. No Windows, os 8 da lista do `build_linux.sh` são obrigatórios; os demais `*_test` são registrados como PASS/FAIL/SKIPPED.
- O pacote Windows (.zip) passa na auditoria (sem ROM nem dados do jogo, manifesto SHA-256) e no smoke test.
- O jogo abre, mostra o launcher, recusa a ROM comprimida e inicia com o `decompressed.us.z64` (confirmação visual do usuário).
- Documentação atualizada: README §1.5 e §2, `tools/BUILDING.md`, `tools/windows/README.md`, `docs/HANDOFF.md`, fase 11, specs e evidência.

## Orquestração

Papéis (definidos pelo usuário):

| Papel | Modelo / CLI | Usa para |
|-------|--------------|----------|
| Orquestrador | Claude Opus 5.5 (esta sessão) | Análise, quebra em tarefas, briefs, revisão de diff, gates de aceite, arbitragem, conversa com o usuário. Não implementa |
| Implementador | Grok 4.7 medium, `cursor-agent` | Scripts, CMake, `.ps1`, portes diretos |
| Documentação | Gemini 3.8 Flash High, `agy` | Ler, escrever e organizar `docs/` (plano, evidência, specs, handoff, README, BUILDING) |
| Testes | Claude Haiku 5.5 medium, `claude` CLI | Escrever e rodar testes automatizados, rodar estágios do build, coletar evidência (comando, exit, hashes) |
| Complexo | Claude Sonnet 5.5 medium, `claude` CLI | Determinismo do gerador, extract com miniz, erros de compilação e link do RT64/plume, IPC do launcher, triagem de falhas |

Invocação (detalhes em `E:\projetos\sub-agent-usage-instrucions.md`). Os modos só leitura e com escrita foram **verificados em 2026-10-22** (W0.1, sonda em repositório descartável: criar arquivo, comando permitido, comando negado).

```powershell
# Grok 4.7 medium (implementação). PowerShell obrigatório: no Git Bash o hook RTK importado do Claude Code quebra e bloqueia o shell.
# Escreve arquivos; shell = só o allowlist de ~/.cursor/cli-config.json (hoje só Shell(ls)) → na prática edita sem rodar nada.
cursor-agent -p --trust --output-format stream-json --model grok-4.7-medium (Get-Content brief.md -Raw)
```

```bash
# Gemini 3.8 Flash High (docs). Escreve arquivos; run_command sempre negado em headless.
# ATENÇÃO: negação encerra sem evento result e com exit 0 → sucesso só com result.status == SUCCESS.
agy -p "$(cat brief.md)" --model gemini-3.8-flash-high --mode accept-edits --output-format stream-json --print-timeout 900s < /dev/null
# Haiku 5.5 medium (testes) / Sonnet 5.5 medium (complexo). Todo padrão de Bash vai em dobro: o hook RTK reescreve `git x` → `rtk git x` antes da checagem.
claude -p --model claude-haiku-5-5 --effort medium --permission-mode acceptEdits \
  --allowedTools "Read" "Write" "Edit" "Bash(cmake *)" "Bash(rtk cmake *)" "Bash(python *)" "Bash(rtk python *)" \
  "Bash(git status*)" "Bash(rtk git status*)" "Bash(git diff*)" "Bash(rtk git diff*)" \
  --no-session-persistence --output-format stream-json --verbose "$(cat brief.md)" < /dev/null
```

Consequência para os papéis (decisão do orquestrador, W0.1):
- **Grok e Gemini editam, mas não executam.** Toda verificação (sintaxe, testes, build, `git diff --check`) é feita pelo Haiku ou pelo orquestrador. Liberar shell para eles exigiria mudar a configuração persistente (`~/.cursor/cli-config.json` ou o `settings.json` do AGY), o que só acontece com OK do usuário, se a falta fizer diferença.
- **Haiku e Sonnet** rodam só os comandos listados no brief (`--allowedTools` por tarefa). Cada brief traz a sua lista mínima.
- Os sub-agentes do Claude CLI disparam os hooks do graft, que criam `.ignore`/`graft/` no diretório de trabalho. Nunca fazer stage desses arquivos.

Observações das verificações:
- O Claude CLI 2.1.292 avisa `unrecognized_model` para o Haiku 5.5 (preço desconhecido), mas funciona.
- Cada chamada do Claude CLI carrega cerca de 42–49k tokens de contexto inicial (CLAUDE.md global, hooks, MCP). O Cursor fica em 10–23k.
- O init do Claude CLI não informa o effort, só o modelo.

Regras para todo sub-agente (vão em todo brief):
1. Diretório de trabalho: `E:\projetos\TooieRecomp-win`. Nunca escrever em `E:\projetos\TooieRecomp`.
2. Seguir `AGENTS.md`. Proibido:
   - commit, push, `reset --hard`, `checkout .`, `restore .`, `clean`, `stash`;
   - editar `RecompiledFuncs/`;
   - ler, copiar ou imprimir bytes da ROM;
   - `--force`/`--yolo`/`--dangerously-skip-permissions`/`bypassPermissions`.
3. Mudança em `lib/` ou `decomp/` só como patch em `vendor/patches-local/` mais `tools/vendor_manifest.sh`, e só quando o brief autorizar.
4. O brief lista os arquivos que podem ser tocados. Tocar fora da lista = parar e reportar.
5. Relatório final em formato fixo:
   - arquivos alterados;
   - comandos rodados com exit code;
   - resultado do critério de aceite (PASS, FAIL ou NOT_RUN);
   - dúvidas abertas.
   Teste não executado é `NOT_RUN`, nunca `PASS`.

Fluxo por tarefa:
1. O orquestrador escreve o brief no scratchpad: objetivo, contexto mínimo (arquivo:linha), arquivos permitidos, critério de aceite com o comando exato, formato do relatório.
2. Um único agente que escreve por vez no clone. Leitores (Gemini lendo, Haiku rodando teste sem editar) podem rodar em paralelo com um escritor só se não tocarem os mesmos arquivos. `build-win/` é serial: nunca dois builds ao mesmo tempo.
3. O orquestrador revisa `git diff` e `git diff --check` e confere o critério de aceite. O Haiku roda os testes quando há mais de um comando.
4. Escalada: se o Grok falha 2 vezes na mesma tarefa, ela passa para o Sonnet. Se o Sonnet trava, o orquestrador analisa e pergunta ao usuário se a decisão for dele.
5. O Gemini registra a evidência da tarefa (comando, exit, HEAD, hashes) em `docs/plan/evidence/p11-windows-native-*.md`.
6. Commit só no fim de cada checkpoint (C1–C5), com OK do usuário, feito pelo orquestrador com staging explícito.

## W0: Preparação (bloqueia tudo)

| # | Tarefa | Dono | Aceite |
|---|--------|------|--------|
| W0.1 | ✅ **Feito (2026-10-22).** As 3 CLIs escrevem arquivos. Cursor: shell só pelo allowlist (`Shell(ls)`) e só a partir do PowerShell. AGY: shell sempre negado, com exit 0 enganoso. Claude: `--allowedTools` com o prefixo `rtk` e `git init` negado. Guia atualizado | Orquestrador | Sonda criar/permitido/negado nas 3 CLIs |
| W0.2 | ✅ **Feito (2026-10-22)**: componentes Clang instalados no VS 18 (`clang-cl` 20.1.8 e `lld-link.exe` presentes via `setup.exe modify` elevado). Instalar no VS Installer (VS 18 Community → Modificar → Componentes individuais): **"C++ Clang Compiler for Windows"** e, se aparecer, **"MSBuild support for LLVM (clang-cl) toolset"**. Precisa de admin | **Usuário** | `vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Llvm.Clang -property installationPath` devolve o caminho |
| W0.3 | ✅ **Feito (2026-10-22)**: HEAD `b59ddae`, 0 arquivos `i/lf w/crlf`, `vendor_verify` OK (8266). Clone: `git clone -c core.autocrlf=false https://github.com/wesleytakatsu/TooieRecomp.git E:\projetos\TooieRecomp-win` no HEAD `b59ddae` ou mais novo. `TOOIE_ROM` aponta para a ROM existente | Orquestrador | `git ls-files --eol` sem `i/lf w/crlf`; `bash tools/vendor_verify.sh` OK |
| W0.4 | ✅ **Feito (2026-10-22)**: renormalize num índice temporário não muda nenhum blob; `vendor_verify` OK. `.gitattributes`: adicionar `* text=auto eol=lf` no topo, mantendo os overrides `-text` existentes | Grok | `git add --renormalize .` não muda nenhum blob além do `.gitattributes`; `vendor_verify` OK |
| W0.5 | ✅ **Feito (2026-10-22)**: corpo idêntico ao plano (só o título mudou); `git diff --check` limpo. Mover este plano para `docs/plans/BT/11-windows-native-build.md` (frontmatter do vault), ligar em `docs/plans/BT/index.md`, `docs/plan/index.md` e `docs/HANDOFF.md` (`NEXT_SINGLE_ACTION`). Depois remover `WINDOWS_NATIVE_PLAN.md` da raiz deste clone | Gemini (o orquestrador remove o arquivo) | Links resolvem; `git diff --check` limpo |
| W0.6 | ✅ **Feito (2026-10-22)**: `Build-Tooie.ps1 -Stage Check` exit 0 em PS7 e PS5.1 (clang-cl 20.1.8, python 3.13.7, cmake, ninja, bash, LF, vendor OK). `Build-Tooie.ps1 -Stage Check`: aceitar VS 18; achar Python como `py -3` e depois `python`, rejeitando o stub `WindowsApps`; cmake/ninja vindos do Developer shell; WSL só como nota | Grok | O Haiku roda `-Stage Check` → exit 0 |

**Checkpoint C1**: `.gitattributes`, `Build-Tooie.ps1` (Check) e a fase 11 no vault.

## W1: Pipeline de geração nativo (caminho crítico)

| # | Tarefa | Dono | Detalhe / Aceite |
|---|--------|------|------------------|
| W1.1 | `tools/toolchain_lock.py` mais `tools/rom_check.py` | Grok | Faz o parse do `toolchain.lock` (`KEY=value`, comentários, aspas). Mesma semântica e mesmos exit codes do `rom_check.sh` (0 OK, 1 inválido, 2 uso errado), modos raw e `--decompressed`. Nunca imprime bytes da ROM |
| W1.2 | Testes da W1.1 | Haiku | `tests/tools/test_rom_check.py` (stdlib `unittest`, sem pytest) com arquivos sintéticos (tamanho errado, hash errado, ausente) mais a ROM real via `TOOIE_ROM` (pula se não existir) |
| W1.3 | Patch do gerador em modo binário | Sonnet | Abrir com `std::ios::binary` todos os `ofstream` de texto do `N64Recomp/src/main.cpp` (e de outros `src/*.cpp` que emitam saída). Auditar se o caminho do host vaza para o conteúdo gerado (separadores, ordem de iteração de diretório). Patch `vendor/patches-local/n64recomp-binary-output.patch` mais `tools/vendor_manifest.sh`. No Linux (WSL, diagnóstico) os 420 hashes não podem mudar |
| W1.4 | Extract nativo | Sonnet | CMake pequeno em `tools/windows/extract/` que compila `decomp/tools/rom_decompressor.cpp` e `tooie_utils.cpp` com o fmt de `decomp/tools/fmt` e o **miniz** vendorizado (shim `zlib.h` só no build dir, nunca escrevendo em `decomp/`). Build fora da árvore em `build-win/extract/`. `tools/extract_rom.py` roda o binário com CWD num diretório de trabalho (os `relocs.bin` não podem cair no repo) e grava num temp; o `rom_check.py --decompressed` aceita; só então faz o rename. Aceite: 35098224 bytes, SHA-1 `af479771…`. Se o miniz não servir, parar e perguntar ao usuário (vendorizar zlib é decisão dele) |
| W1.5 | `tools/run_recomp.py` | Grok | Compila o N64Recomp com clang-cl e Ninja em `build-win/n64recomp`. Gera `.tooie.recomp.gen.toml` com `rom_file_path` em caminho POSIX dentro de string literal TOML (`'...'`). Faz o preflight (rom_check `--decompressed` mais `metadata/tooie.us.syms.toml`) **antes** de apagar `RecompiledFuncs/` e então roda |
| W1.6 | `tools/verify_generated.py` | Grok | Porte do `.sh`: conta 420 arquivos e confere SHA-256 em modo binário. O manifesto é lido tolerando `\r`. Mantém o `--update` só para o mantenedor |
| W1.7 | Estágios `Extract`, `Recomp`, `Verify` no `Build-Tooie.ps1` | Grok | Wrappers finos que chamam os `.py` |
| W1.8 | Rodar Extract → Recomp → Verify no Windows | Haiku | Relatório com tempo, contagem e resultado do `verify_generated.py`. Divergência vira lista de arquivos e primeiras linhas do diff para o Sonnet, nunca edição da saída |

Risco: se o N64Recomp ou o `rom_decompressor` tiverem outra suposição POSIX, o Sonnet registra a correção como patch local.

**Checkpoint C2**: pipeline de geração nativo, com 420/420 batendo.

## W2: Primeiro build nativo do jogo

| # | Tarefa | Dono | Detalhe / Aceite |
|---|--------|------|------------------|
| W2.1 | Estágios `Configure` e `Build` | Grok | `-G Ninja`, `clang-cl`, `Release`, `-Jobs`. Paridade com o `-ffile-prefix-map` do Linux via `/clang:-ffile-prefix-map=<root>=/tooie` |
| W2.2 | Flags de `RecompiledFuncs` com clang-cl | Sonnet | Validar `-fno-strict-aliasing`, `-falign-functions=16` e os `-Wno-*`; usar `/clang:` ou `if(MSVC)` quando preciso. Avaliar `/bigobj` nos `funcs_*.c` grandes |
| W2.3 | Erros reais de TU (RT64/plume e outros) | Sonnet | As falhas de `plume_d3d12.cpp`/`plume_vulkan.cpp` vieram de sondas cross-host sem os cabeçalhos do SDK. No host nativo podem nem existir: corrigir só o que falhar de verdade, via patch versionado |
| W2.4 | Link | Sonnet | `TOOIE_REAL_SHIM` igual à lista `-Wl,--wrap`, `/OPT:NOICF`, cópia de DLLs (SDL2, dxil, dxcompiler), ícone `.rc` |
| W2.5 | Build completo mais smoke de execução | Haiku | `-Stage Build` com exit 0; o exe sobe e fica vivo por N s, ou sai limpo com log. O launcher na tela fica para o usuário confirmar visualmente |

Riscos: caminhos longos (>260) dentro de `build-win/`, mitigados com build dir curto. O Defender deixa o build lento; excluir a pasta é decisão do usuário, nunca do agente.

**Checkpoint C3**: `TooieRecompiled.exe` linkado, com o launcher visto pelo usuário.

## W3: Paridade de funcionalidades

| # | Tarefa | Dono | Detalhe |
|---|--------|------|---------|
| W3.1 | Música de preview do launcher | Sonnet | Hoje é um stub no Windows. Implementar com `CreateProcessW` mais `--launcher-music-child`, um pipe anônimo como stdin de controle e um Job Object com `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`. Se ficar complexo demais, manter desligado e documentar (pede OK do usuário) |
| W3.2 | Diretório de diagnóstico | Grok | Helper único `tooie_diag_path(name)` (`TOOIE_DIAG_DIR` ou `std::filesystem::temp_directory_path()`) no lugar dos `"/tmp/..."` de `tooie_diag.cpp` e `transform_hooks.cpp`. Sem mudar o comportamento no Linux |
| W3.3 | Portar os testes `.sh`/`.py` | Haiku | `demo1_projectile_coverage_test.sh` e `n64recomp_zero_size_static_test.sh` viram Python portátil; conferir se `attract_demo2_transition_jal_test.py` roda no Windows |
| W3.4 | `run-tooie.ps1` | Grok | Equivalente ao `run-tooie.sh`. O Angrylion fica só no Linux (documentado) |

## W4: Testes, pacote e CI

| # | Tarefa | Dono | Detalhe |
|---|--------|------|---------|
| W4.1 | Estágio `Test` | Grok (wrapper) + Haiku (execução) | Os 8 testes de host obrigatórios, depois os demais alvos `*_test` (23 no CMake). Cada um com resultado PASS, FAIL ou SKIPPED e log |
| W4.2 | Estágio `Package` | Grok | O `.ps1` já existe e usa o Git Bash para o `package_manifest.sh`. Portar `verify_package.sh` e `smoke_package.sh` para Python. Revisar a regra de auditoria que bane `.json` (um `graphics.json` de usuário não pode entrar no pacote, mas nenhum asset `.json` legítimo pode ser barrado) |
| W4.3 | Smoke do pacote | Haiku | Extrair o zip para um temp, rodar `MANIFEST.sha256`, abrir o exe isolado e confirmar saída limpa |
| W4.4 | CI | Grok | `public-windows` roda os 8 testes de host; tirar `continue-on-error` só depois de 2 execuções verdes. Novo job `private-game-build-windows` (self-hosted, ROM no runner) publicando só checksums e auditoria. Agentes nunca disparam workflow |
| W4.5 | Tempo e disco do build | Gemini | Registrar na evidência e em `tools/BUILDING.md` |

**Checkpoint C4**: estágios Test e Package, e o CI.

## W5: Validação manual no Windows

- Build limpo do zero no clone `-win` (ou noutro clone novo com `autocrlf=false`): `Build-Tooie.ps1 -Stage All`. Roda o Haiku.
- O usuário valida:
  - o launcher abre;
  - a seleção do `decompressed.us.z64` funciona e a ROM comprimida é recusada;
  - o jogo inicia com vídeo D3D12 e Vulkan;
  - áudio WASAPI, teclado e controle SDL funcionam;
  - o `graphics.json` persiste no caminho certo do Windows.
- O Sonnet faz a triagem das falhas. Falha não vira `PASS`, e o que não foi executado fica `SKIPPED` ou `NOT_IMPLEMENTED`.

## W6: Documentação e fechamento (Gemini, revisão do orquestrador)

- Reescrever README §1.5 e §2 e `tools/BUILDING.md`: tirar "Native Host Pending" e os caminhos A/B **só depois** da W5.
- Criar `tools/windows/README.md` (o `build.yml` já referencia e ele não existe).
- Atualizar `docs/HANDOFF.md`, a fase 11, a fase 10 (status Windows), as specs `build-and-generator.md` e `packaging-and-dependencies.md` e as evidências `p11-windows-native-*`.
- `ImportGenerated` vira legado documentado.
- Opcional: os `.sh` do Linux passam a chamar os `.py` novos, para uma lógica única.
- Rodar `git diff --check` e `markdown_vault_audit`. O MCP `markdown-vault` não conectou nesta sessão: consertar ou usar o fallback documentado.

**Checkpoint C5**: documentação.

## Ordem e paralelismo

```
W0.1 ─┐
W0.2 (usuário) ─┼─> W0.3 -> W0.4 -> W0.6 ──> W1.1/W1.3/W1.4 -> W1.5/W1.6 -> W1.7 -> W1.8 -> W2 -> W3 ∥ W4 -> W5 -> W6
W0.5 (Gemini, docs) ─┘ (paralelo a W0.4/W0.6: arquivos diferentes)
```

- W1.1, W1.3 e W1.4 tocam arquivos disjuntos, mas são todas escritas no mesmo clone. Rodam uma de cada vez, ou em paralelo só com worktrees separados (decidir na hora).
- O Gemini registra a evidência ao fim de cada checkpoint, em paralelo com o próximo trabalho de código.

## Riscos

| Risco | Impacto | Mitigação |
|-------|---------|-----------|
| A saída gerada no Windows difere da do Linux | Alto | W1.3 (modo binário) mais o diff dirigido na W1.8. Corrigir no gerador via patch, nunca na saída |
| O miniz não substitui o zlib no `rom_decompressor` | Médio | Testar na W1.4. Plano B (vendorizar zlib) precisa de OK do usuário |
| RT64/plume não compila com clang-cl | Alto | Patches locais registrados (W2.3) |
| VS 18 sem o componente Clang | Médio | W0.2. Plano B: Build Tools 2022 lado a lado |
| Flags GNU rejeitadas pelo clang-cl | Baixo | `/clang:` ou condição por compilador (W2.2) |
| `python3` = stub da Store | Médio | Os scripts usam `sys.executable`; o `.ps1` resolve `py -3`/`python` (W0.6) |
| Divergência entre `.sh` e `.py` | Médio | Testes de paridade (W1.2) e unificação opcional na W6 |
| Sub-agente sai do escopo ou faz git destrutivo | Alto | Lista de arquivos permitidos, sem flags de bypass, revisão de diff a cada tarefa, clone separado |
| Custo e contexto dos sub-agentes | Médio | Briefs curtos com `arquivo:linha`; nada de "leia o repo todo" |
| Ferramentas de decomp (IDO, MIPS gcc) | Baixo | Continuam só no Linux/WSL, para o mantenedor. Fora do build normal |

## Fora de escopo

- Regeneração de decomp e metadados (`generate_metadata.py`, kit IDO) e o RSPRecomp: continuam no Linux/WSL, só para o mantenedor.
- Windows ARM64 e macOS.
- Oráculo Angrylion no Windows.
- O resíduo de geometria widescreen (P08) e o gate P09, que seguem como o trabalho ativo do projeto.

## Primeira ação concreta

W0 concluída (evidência: [[../../plan/evidence/p11-windows-native-w0-20261022]]). Próxima: checkpoint C1 (commit com OK do usuário), depois W1.1 (Grok) + W1.3 (Sonnet).
