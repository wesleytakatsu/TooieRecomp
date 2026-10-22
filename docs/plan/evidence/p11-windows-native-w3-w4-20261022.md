---
title: "BT P11 — Evidência W3+W4: Paridade de Funcionalidades, Testes, Empacotamento e CI no Windows 11"
description: "Evidência técnica da conclusão das etapas W3 (W3.1 a W3.5) e W4 (W4.1 a W4.5) no Windows 11 x64: IPC de música do launcher via Win32 Job Object e pipes anônimos, helper de caminhos de diagnóstico tooie_diag_path, porte de scripts de teste para Python, run-tooie.ps1, suíte de 29 alvos de teste com resolução de 6 causas de portabilidade e pilha de 8 MiB (9/9 obrigatórios PASS, 15/15 extras PASS, 5 drivers com SKIP documentado), empacotamento completo com paridade ao Linux e smoke test de 15s, workflows GitHub Actions com job self-hosted, e medição de tempos de geração e build."
status: current
type: evidence
project: TooieRecomp
phase: P11
updated: 2026-10-22
tags: [windows, p11, w3, w4, tests, package, ci, launcher-music, job-object, smoke-test, diagnostics, clang-cl, ninja]
---

# BT P11 — Evidência W3+W4: Paridade de Funcionalidades, Testes, Empacotamento e CI no Windows 11

## 1. Host, Base e Sumário Executivo

- **Base de Código:** Base C3 commit `0706e70` (todas as alterações das etapas W3 e W4 ainda não commitadas no worktree) no clone de trabalho dedicado `E:\projetos\TooieRecomp-win`.
- **Toolchain do Host:** `clang-cl` 20.1.8 + `lld-link` + Ninja 1.12.1 (LLVM / Clang Toolset no Visual Studio 2026 18 Community), Windows SDK 10.0.26100.0.
- **Processador / Paralelismo:** Host com 14 processadores lógicos (threads de CPU).
- **Python:** Python 3.13.7 (`C:\Python313\python.exe`; stub `python3` da Microsoft Store ignorado).
- **Shell / Runtime:** Windows PowerShell 5.1 e PowerShell 7.x.
- **Status Anterior:** W2 concluída ([[p11-windows-native-w2-20261022]]).

---

## 2. W3: Paridade de Funcionalidades

### 2.1. W3.1 — Música de Preview do Launcher no Windows (`PROVEN` / `PENDING_USER`)
- **Implementador:** Sonnet.
- **Localização:** `src/main/main.cpp`.
- **Integridade de Plataforma:** Implementado estritamente com ramificações `#elif defined(_WIN32)`; o corpo existente para Linux permaneceu completamente intacto.
- **Arquitetura de IPC Win32:**
  - Criação do processo filho via `CreateProcessW` invocando o próprio binário executável com os argumentos `--launcher-music-child --launcher-music-control`.
  - Flags de criação: `CREATE_NO_WINDOW | CREATE_SUSPENDED`.
  - Canal de controle (`stdin`): Configurado como um pipe anônimo Win32, onde estritamente a ponta de leitura é herdável pelo processo filho; a ponta de escrita no processo pai opera em modo não-bloqueante (`PIPE_NOWAIT`).
  - Ciclo de vida e contenção de processos: O processo filho é inserido em um Windows Job Object configurado com `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` antes de sua retomada via `ResumeThread`. Isso garante o encerramento determinístico e automático do processo filho caso o processo pai termine inesperadamente.
  - Protocolo de controle: Mensagens de 1 byte representando o percentual de volume (`percent`), em paridade direta com o datagrama utilizado no Linux.
  - Leitura e encerramento no filho: O processo filho consome comandos via `PeekNamedPipe` e `ReadFile`. Ao detectar fim de arquivo (EOF, disparado pelo fechamento do pipe pelo pai), encerra-se imediatamente via `_Exit(0)`.
  - Procedimento de parada (`stop`): O pai fecha a ponta de escrita do pipe, aguarda até 1 segundo pelo encerramento gracioso e, em caso de timeout, invoca `TerminateProcess` como fallback seguro.
  - Defaults de configuração mantidos rigorosamente inalterados.
- **Sonda de Verificação (`PROVEN`):**
  - Monitoramento de processos executado com sucesso: `parent alive: True; music children: 1`.
  - Após o encerramento do processo pai, verificação confirmou o processo filho como `gone` (eliminado imediatamente pelo Job Object).
- **Status Perceptivo:**
  - Áudio audível e responsividade ao slider de volume: classificados como `PENDING_USER` (pendente de confirmação auditiva pelo usuário).

### 2.2. W3.2 — Diretório de Diagnóstico Portável (`PROVEN`)
- **Implementador:** Grok.
- **Arquivo de Cabeçalho:** `include/tooie_diag_path.h`.
- **Especificação da Função Helper `tooie_diag_path(name)`:**
  - Se a variável de ambiente `TOOIE_DIAG_DIR` estiver definida: retorna `TOOIE_DIAG_DIR/<name>`.
  - No Windows (`_WIN32`): resolve para `std::filesystem::temp_directory_path()` (`path.string()`, utilizando a code page ANSI compatível com `fopen`).
  - No Linux e macOS: resolve rigorosamente para `/tmp/<name>` (comportamento preexistente preservado sem qualquer alteração).
- **Substituições no Código-Fonte:**
  - Todas as ocorrências com caminhos fixos literais `"/tmp/tooie-..."` em `src/diagnostics/tooie_diag.cpp` e `src/game/transform_hooks.cpp` foram substituídas pela chamada centralizada a `tooie_diag_path()`.

### 2.3. W3.3 — Porte de Testes em Shell para Python Portável (`PROVEN`)
- **Implementador:** Haiku.
- **Portes Realizados:**
  - `tests/demo1_projectile_coverage_test.py` (porte portável de `tests/demo1_projectile_coverage_test.sh`).
  - `tests/n64recomp_zero_size_static_test.py` (porte portável de `tests/n64recomp_zero_size_static_test.sh`).
  - Os scripts `.sh` originais permanecem inalterados na árvore para uso em Linux.
- **Resultados no Windows 11:**
  - `tests/demo1_projectile_coverage_test.py`: `PASS`.
  - `tests/n64recomp_zero_size_static_test.py`: `PASS`.
  - `tests/attract_demo2_transition_jal_test.py`: `PASS` (`OVERALL: PASS`).
  - `tools/test_rom_check.py`: 37 testes executados com status `OK`.

### 2.4. W3.4 — Script de Execução PowerShell `run-tooie.ps1` (`PROVEN`)
- **Implementador:** Grok.
- **Arquivo Criado:** `run-tooie.ps1`.
- **Comportamento e Paridade:**
  - Suporta as mesmas opções de linha de comando e variáveis de ambiente documentadas em `run-tooie.sh`.
  - Aponta para o diretório de compilação do Windows (`build-win`).
  - O suporte ao emulador de referência Angrylion permanece restrito ao ambiente Linux conforme documentado.
  - Validação: Invocação de ajuda `.\run-tooie.ps1 -h` executada com sucesso e saída verificada OK.

### 2.5. W3.5 — Atualização de Orientação em `tools/extract_rom.py` (`PROVEN`)
- **Implementador:** Grok.
- **Modificação:** Mensagem de orientação final emitida ao término de `tools/extract_rom.py` atualizada para refletir a nova capacidade do launcher nativo (introduzida na etapa W2.6), que aceita de forma transparente tanto a ROM de cartucho comercial (`.z64`/`.n64`) quanto a imagem descomprimida.

---

## 3. W4: Testes, Empacotamento e CI

### 3.1. W4.1 — Estágio `Test` e Resolução de Portabilidade da Suíte (`PROVEN`)
- **Implementação:** Wrapper no `Build-Tooie.ps1` por Grok; portabilidade da suíte e resolução de defeitos por Sonnet.
- **Descoberta Inicial do Orquestrador:**
  - Varredura de 29 alvos executáveis de teste `*_test` definidos no CMake.
  - Execução inicial resultou em 10 falhas causadas por 6 problemas distintos de portabilidade POSIX vs Windows:
    1. Inclusão incondicional do cabeçalho POSIX `<unistd.h>`.
    2. Utilização de `fmemopen()` exclusiva no código de teste (inexistente na runtime MSVC).
    3. Invocação da função POSIX `setenv()`.
    4. Inclusão de `<sys/mman.h>` e chamadas `mprotect` para teste de páginas de guarda.
    5. Diretórios de include, fontes auxiliares e diretivas de linkagem faltantes em 3 alvos que compilavam `boot_hw_stubs.cpp`.
    6. Estouro de pilha (`0xC00000FD`, `STATUS_STACK_OVERFLOW`) no teste `p09_exact_use_correlation_test` devido ao limite padrão de 1 MiB da stack alocada por padrão no Windows (em contraste aos 8 MiB habituais no Linux).
- **Correções Aplicadas (sem enfraquecer asserções):**
  - Ramo `_WIN32`: Substituição de `<unistd.h>` por `<process.h>` e chamada a `_getpid()`.
  - Substituição de `fmemopen()` nos harnesses de teste por arquivos temporários binários criados via `std::tmpfile()`.
  - Substituição de `setenv()` por `_putenv_s()` no Windows.
  - Implementação de guard pages no Windows via API `VirtualAlloc(..., PAGE_NOACCESS)`, preservando integralmente o teste de proteção de memória.
  - Configuração da flag de linkagem `/STACK:8388608` (8 MiB de pilha) para o executável `p09_exact_use_correlation_test`.
  - Ajuste de includes, adição de fontes e diretivas de wrap nos 3 alvos de teste afetados, acompanhado da criação do stub de interface gráfica `tests/support/recompui_window_stub.cpp` (janela emulada 640x480).
  - Nenhuma asserção de teste foi relaxada, comentada ou modificada.
- **Resultado da Suíte Completa (`PROVEN`):**
  - Comando: `.\tools\windows\Build-Tooie.ps1 -Stage Test -AllTests`.
  - Sumário de Execução:
    - **Testes Obrigatórios:** **9/9 PASS**.
    - **Testes Extras:** **15/15 PASS**.
    - **Testes Pulados:** **5 SKIPPED** com justificativa documentada.
      - Alvos pulados: `tooie_demo1_parity_test`, `tooie_demo1_projectile_test`, `tooie_demo1_target_selector_test`, `tooie_demo1_upd_layout_test`, `p09_local_pair_snapshot_io_test`.
      - Motivo: Trata-se de drivers executáveis que exigem parâmetros de linha de comando ou fixtures externas específicas. Foram todos compilados com sucesso (`exit 0`), mas são pulados por desenho durante a execução da suíte automatizada padrão (comportamento rigorosamente idêntico ao do Linux).
  - Desempenho no teste de correlação:
    - `p09_exact_use_correlation_test`: **1355/1355 passed**.

### 3.2. W4.2 — Estágio `Package` com Paridade Linux (`PROVEN`)
- **Implementador:** Grok.
- **Comportamento e Artefatos Gerados:**
  - Nome do arquivo de distribuição: `TooieRecompiled-windows-x64-<HEAD>[-dirty]-private-local.zip`.
  - `BUILD-INFO.txt`: Contém todos os campos emitidos pelo script Linux homólogo, acrescido do campo `build_time_utc`.
  - `DEPENDENCIES.txt`: Mapeia formalmente as dependências locais incluídas (`SDL2.dll` e `dxcompiler.dll`) e bibliotecas dinâmicas providas pelo sistema operacional (`d3d12`, `dxgi`, `VCRUNTIME140.dll`, `MSVCP140.dll`, `api-ms-win-crt-*`).
  - `AUDIT.txt`: Auditoria automatizada concluída com `result=PASS` cobrindo 104 arquivos, sem presença de arquivos proibidos (a restrição contra arquivos `.json` foi adequadamente delimitada à raiz do pacote e a nomes de arquivos de configuração do usuário, sem bloquear assets de interface legítimos).
  - `THIRD-PARTY-MANIFEST.tsv`: 105 arquivos inventariados; 42 entradas classificadas como `UNKNOWN`/`UNVERIFIED` (decisões de licenciamento e termos de redistribuição preservados formalmente para deliberação do proprietário).
  - Checksum: Geração de hash SHA-256 no arquivo `.zip.sha256`.
  - Tamanho aproximado do pacote gerado: ~19,2 MB.
  - Nota de reproducibilidade: O arquivo zip empacotado via `Compress-Archive` do PowerShell não apresenta reproducibilidade binária estrita byte a byte como o tarball normalizado em Linux.
- **Porte de Ferramentas de Auditoria e Correção W4.2b (`PROVEN`):**
  - `tools/verify_package.py` e `tools/smoke_package.py` criados em Python multiplataforma.
  - Defeito corrigido durante o ciclo (W4.2b): Identificado e corrigido erro de deslocamento off-by-one no parsing de `sys.argv` em ambos os scripts.
- **Resultado do Estágio Package (`PROVEN`):**
  - Execução: `.\tools\windows\Build-Tooie.ps1 -Stage Package` concluiu com exit code 0 (`exit 0`).
  - Validação de manifesto: `MANIFEST OK (106 files)`.
  - Auditoria de conteúdo: `no ROM/ELF-like files`.

### 3.3. W4.3 — Smoke Test do Pacote Empacotado (`PROVEN`)
- **Implementação e Teste:** Haiku.
- **Comando:** Invocação de `python tools/smoke_package.py ... 15`.
- **Resultado:** `SMOKE PASS: alive 15s`.
- **Ressalva de Isolamento no Windows:**
  - Registrado que, no ambiente Windows, o smoke test do pacote não é estritamente isolado do sistema hospedeiro.
  - Causa: A persistência de configurações do runtime grava em `%LOCALAPPDATA%\TooieRecompiled`, uma vez que a chamada Win32 `SHGetKnownFolderPath` resolve caminhos de pastas de perfil do usuário ignorando variáveis de ambiente de redirecionamento.

### 3.4. W4.4 — Integração Contínua (CI) no GitHub Actions (`NOT_RUN`)
- **Implementador:** Grok.
- **Arquivo Modificado:** `.github/workflows/build.yml`.
- **Estrutura de Jobs:**
  - `public-windows`: Executa compilação e a suíte de 9 testes obrigatórios + `tools/test_rom_check.py` (propriedade `continue-on-error: true` mantida temporariamente até a obtenção de duas execuções consecutivas com sucesso).
  - `public-linux`: Atualizado com a inclusão de `rom_cart_import_test` e `tools/test_rom_check.py`.
  - `private-game-build-windows`: Novo job executado em runner self-hosted dedicado (com ROM local), disparando `Build-Tooie.ps1 -Stage All` e publicando exclusivamente artefatos de controle (checksums, auditoria, build-info e manifesto de dependências, sem redistribuição de dados do jogo).
- **Status de Execução:**
  - O fluxo foi devidamente estruturado e alterado na árvore de código, porém **NÃO disparado** (`NOT_RUN`), em observância às diretrizes operacionais de agentes.

### 3.5. W4.5 — Métricas de Tempo e Recursos de Compilação (`PROVEN`)
- **Ambiente de Medição:** Host Windows 11 com processador de 14 threads lógicas e armazenamento SSD local.
- **Tempos Medidos:**
  - Estágio de geração de código C recompilado (`Recomp`): ~1 minuto.
  - Estágio de compilação completa do jogo e geração do executável (`Build`): ~5 minutos (utilizando 14 jobs paralelos via Ninja).

---

## 4. Escopo Não Executado (`NOT_RUN`) e Pendências

1. **Validação em Ambiente Linux Nativo (`NOT_RUN`):**
   - Todas as modificações estruturais que impactam o código compartilhado ou o pipeline Linux não foram testadas no Linux nesta etapa:
     - Refatoração da linkagem de wrappers de símbolos (`TOOIE_WRAPPED_SYMBOLS` com `/wrap:` no Windows e `--wrap` no Linux no `CMakeLists.txt`).
     - Renomeação dos enumeradores P09 em `lib/rt64/src/hle/rt64_interp_burst.h` (`Reason::DUPLICATE_KEY` e `Controller::Result::FAILED`).
     - Inclusões de cabeçalhos, fontes auxiliares, wraps e stub de janela gráfica `tests/support/recompui_window_stub.cpp` nos 3 alvos de teste.
     - Implementação da biblioteca de descompressão em memória `tooie_cart_decompress` e hook em `recomp::select_rom`.
     - Novo cabeçalho e resolução de caminhos de diagnóstico em `include/tooie_diag_path.h`.
   - **Procedimento Definido:** Esta validação será realizada diretamente **PELO USUÁRIO** em sua máquina física com Linux (o WSL não é utilizado no fluxo de trabalho).
2. **Confirmação Auditiva da Música do Launcher (`PENDING_USER`):**
   - O mecanismo de ciclo de vida do processo filho e o canal de controle IPC Win32 estão plenamente comprovados (`PROVEN`), restando a confirmação auditiva da reprodução e do ajuste de volume no slider pelo usuário.
3. **Disparo do CI (`NOT_RUN`):**
   - Os fluxos do GitHub Actions permanecem como alteração de configuração não acionada.
4. **Status Operacional do Projeto:**
   - Com as etapas W3 e W4 finalizadas, conclui-se o escopo técnico do **Checkpoint C4**.
   - As próximas ações correspondem ao checkpoint C4 e validação Linux pelo usuário, prosseguindo subsequentemente para a validação manual do usuário no Windows (W5) e a atualização documental de encerramento (W6).
