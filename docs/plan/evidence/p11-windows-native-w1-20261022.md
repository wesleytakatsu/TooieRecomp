---
title: "BT P11 — Evidência W1: Pipeline de Geração Nativo no Windows 11"
description: "Evidência técnica da conclusão das etapas W1 (W1.1 a W1.8) do build nativo no Windows 11 x64: RecompiledFuncs gerado nativamente byte-idêntico ao baseline Linux (420/420), extract com miniz, patches binários e validação completa."
status: current
type: evidence
project: TooieRecomp
phase: P11
updated: 2026-10-22
tags: [windows, p11, w1, build, n64recomp, extract, miniz, generation, binary-output, powershell]
---

# BT P11 — Evidência W1: Pipeline de Geração Nativo no Windows 11

## 1. Host, Base e Sumário Executivo

- **Base de Código:** Commit C1 `6862e29` no clone dedicado `E:\projetos\TooieRecomp-win`.
- **Toolchain do Host:** `clang-cl` 20.1.8 (LLVM / Clang Toolset no Visual Studio 2026 18.4 Community), Ninja 1.12.1, Windows SDK 10.0.26100.0.
- **Python:** Python 3.13.7 (`C:\Python313\python.exe`; `python3` no PATH aponta para o stub da Microsoft Store e é ignorado pelos scripts).
- **Shell / Runtime:** Windows PowerShell 5.1 (e PowerShell 7.x).
- **Status Anterior:** W0 concluída ([[p11-windows-native-w0-20261022]]).

---

## 2. Resultado Principal: Geração Nativa Byte-Idêntica (`PROVEN`)

A pipeline de geração do código C recompilado (`RecompiledFuncs/`) foi executada **nativamente no Windows 11 sem WSL**:
- **Volume Gerado:** `generated 420 files (89.0 MB)`.
- **Integridade Categórica:** `verify_generated: OK 420 files match metadata/recomp-output.sha256` — rigorosamente byte-idêntico ao baseline oficial gerado em ambiente Linux.
- **Fidelidade de Quebra de Linha:** 0 bytes CR (`\r`) em `funcs.h` e em todos os arquivos emitidos (terminação estritamente LF).
- **Verificação de Atualidade:** `tools/check_generated_fresh.py` reportou status OK.
- **Arbitragem do Orquestrador:** Execução independente reproduzida e confirmada com exit code 0 (`rc=0`).

---

## 3. W1.1: Utilitários Centrais e Resolução de Caminhos (`PROVEN`)

Implementação por Grok (`tools/toolchain_lock.py`, `tools/rom_check.py` e `tools/tooie_paths.py`):
- `tools/toolchain_lock.py`: Parser robusto de `toolchain.lock` tratando pares `KEY=value`, comentários e aspas.
- `tools/rom_check.py`: Porte completo de `rom_check.sh` preservando estritamente a semântica de saída e códigos de retorno (exit 0 para sucesso, 1 para ROM inválida/ausente, 2 para erro de uso da CLI). Fornece a API programática com `check_raw()`, `check_decompressed()` e a exceção `RomCheckError`. Em total conformidade com as regras de confidencialidade, nunca lê, copia nem imprime bytes da ROM no terminal.
- `tools/tooie_paths.py`: Abstração de caminhos do workspace:
  - Diretório padrão de build definido como `build-win` no Windows, preservando `build-linux` intacto em ambientes Linux/POSIX.
  - Resolução unificada de ROM via `rom_path()`.
  - Resolução do executável do gerador (`N64RECOMP`) com extensão `.exe` automática sob Windows.

---

## 4. W1.2: Testes Automatizados e Correções na Suite (`PROVEN`)

Suite desenvolvida por Haiku em `tools/test_rom_check.py` (adotando a convenção de localização do repositório, em vez de `tests/tools/`):
- **Cobertura:** 37 testes automatizados baseados na biblioteca padrão `unittest` (sem dependência externa de `pytest`), cobrindo arquivos sintéticos (tamanho incorreto, hash divergente, arquivos ausentes) e a ROM canônica real.
- **Defeitos Reais Identificados e Corrigidos (W1.1b por Grok):**
  1. Quebra em `toolchain_lock.py` quando uma linha continha chave vazia `K=` (`IndexError` na separação de tokens).
  2. Comportamento divergente em `rom_check.py ""` ao receber string vazia como argumento, não acionando a ROM padrão como fazia a expansão `${1:-...}` do shell script original.
- **Resultado:** 37/37 testes executados com PASS.

---

## 5. W1.3: Patch N64Recomp em Modo Binário e Compilação Nativa (`PROVEN`)

Investigação e implementação por Sonnet:
- **Causa Raiz de Divergência CRLF:** O gerador N64Recomp abria streams de saída de texto sem a flag `std::ios::binary`, injetando quebras de linha CRLF (`\r\n`) quando compilado no Windows runtime MSVC.
- **Patch de Fidelidade:** Criado `vendor/patches-local/n64recomp-binary-output.patch` (SHA-256 `d916bf1142c7e4fd18e95607f3c308db32d0c790fe4282d0d04dccde2ffa418a`), convertendo 8 pontos de instanciação de `std::ofstream` para modo binário explícito:
  - 7 em `lib/N64ModernRuntime/N64Recomp/src/main.cpp`: saída temporária, 2 dumps de contexto, `funcs.h`, `funcs_N.c`, `lookup.cpp`, `recomp_overlays.inl`.
  - 1 em `lib/N64ModernRuntime/RSPRecomp/src/rsp_recomp.cpp`.
- **Auditoria de Determinismo:** Verificado que nenhum caminho absoluto ou relativo do host vaza para os arquivos emitidos (`lookup.cpp` utiliza apenas a variável lógica `rom_name`), ausência total de ordenações arbitrárias por `std::filesystem::directory_iterator` e independência de locale do sistema.
- **Gestão de Dependências Vendorizadas:**
  - Adicionada entrada em `vendor/patches-applied/ORDER.tsv`.
  - Atualizados 2 hashes de arquivos modificados em `vendor/MANIFEST.sha256`.
  - `vendor/MODES.tsv` mantido intocado: o script `tools/vendor_manifest.sh` não foi rodado no Windows para evitar corrupção das permissões POSIX pelos modos do sistema de arquivos NTFS.
  - Verificação de sanidade reversa: `bash tools/vendor_verify.sh --reverse-check` resultou em exit code 0 (`reverse-check: 20 patches reverse-applied`, `VENDOR VERIFY OK`).
- **Compilação Nativa do N64Recomp:**
  - Compilado com sucesso via `clang-cl` e Ninja (75 passos): `N64Recomp.exe`, `RSPRecomp.exe` e executáveis auxiliares gerados em `build-win/n64recomp/`.
  - Zero necessidade de patches adicionais de compatibilidade de plataforma.
- **Impacto em Ambientes POSIX (`STRONG_INFERENCE` / `NOT_RUN`):**
  - O uso de `std::ios::binary` em sistemas POSIX/Linux é historicamente um no-op (`STRONG_INFERENCE`). A regeneração dos metadados e código em Linux com este patch aplicado não foi executada nesta sessão (`NOT_RUN`).

---

## 6. W1.4: Descompressão Nativa da ROM com Miniz (`PROVEN`)

Engenharia de descompressão por Sonnet:
- **Resolução de Dependência:** Constatada a inexistência de zlib vendorizado no repositório. O utilitário foi viabilizado através do **miniz vendorizado** (`lib/N64ModernRuntime/thirdparty/miniz`), compilando unicamente `miniz.c` e `miniz_tinfl.c` com as diretivas `-DMINIZ_NO_DEFLATE_APIS` e `-DMINIZ_NO_ARCHIVE_APIS`.
- **Shims de Isolamento:** `tools/windows/extract/CMakeLists.txt` compila `decomp/tools/rom_decompressor.cpp`, `decomp/tools/tooie_utils.cpp` e o formatador `decomp/tools/fmt`, gerando cabeçalhos de compatibilidade `zlib.h` e `miniz_export.h` estritamente dentro da árvore de build. Nenhuma alteração ou arquivo espúrio foi escrito em `decomp/` ou `lib/`.
- **Script de Extração Seguro:** `tools/extract_rom.py` valida previamente a ROM canônica bruta, executa o binário descompressor com diretório de trabalho isolado (impedindo o vazamento de arquivos intermediários como `relocs.bin` para o repositório), grava em arquivo temporário, valida o hash de saída e realiza a renomeação atômica final.
- **Resultado Canônico Comprovado (`PROVEN`):**
  - Arquivo: `build-win/extract/decompressed.us.z64`.
  - Tamanho: `35098224` bytes.
  - SHA-1: `af479771ff5e91ce3732f3d84146614ab71d8d79`.

---

## 7. W1.5 a W1.7: Orquestração do N64Recomp e Estágios no PowerShell (`PROVEN`)

Implementação por Grok:
- `tools/run_recomp.py` (W1.5): Compila o N64Recomp nativamente caso desatualizado, sintetiza o arquivo de configuração `.tooie.recomp.gen.toml` garantindo que `rom_file_path` utilize barras POSIX (`/`) envolvidas em literal TOML (`'...'`), e executa checagens prévias (preflight via `rom_check.py --decompressed` e presença de `metadata/tooie.us.syms.toml`) **antes** de qualquer remoção do conteúdo existente em `RecompiledFuncs/`.
- `tools/verify_generated.py` (W1.6): Porte de `tools/verify_generated.sh` em Python, comparando a contagem de 420 arquivos e os hashes SHA-256 binários, implementando leitura resiliente a terminações de linha `\r\n` e caracteres `*` presentes no manifesto `metadata/recomp-output.sha256`.
- `Build-Tooie.ps1` (W1.7): Implementação dos estágios `-Stage Extract`, `-Stage Recomp` e `-Stage Verify` atuando como wrappers finos e diretos sobre a infraestrutura Python nativa.

---

## 8. W1.8: Execução End-to-End via Build-Tooie.ps1 (`PROVEN`)

Validação e coleta de métricas por Haiku executando nativamente em Windows PowerShell 5.1:
- `tools/test_rom_check.py`: 37 testes unitários executados com exit 0 (OK).
- `.\tools\windows\Build-Tooie.ps1 -Stage Extract`: exit 0; `decompressed.us.z64` verificado com sucesso.
- `.\tools\windows\Build-Tooie.ps1 -Stage Recomp -Jobs 8`: exit 0; build do N64Recomp já atualizado desde a W1.3 (ninja: "no work to do") e 420 arquivos de funções gerados com sucesso.
- `.\tools\windows\Build-Tooie.ps1 -Stage Verify`: exit 0; `OK 420 files match metadata/recomp-output.sha256`.

---

## 9. Desvio Operacional Registrado: Hardlink NTFS da ROM Canônica

- **Abordagem Adotada:** A ROM de entrada `Banjo-Tooie (USA).z64` foi materializada no clone de trabalho `E:\projetos\TooieRecomp-win` através de um **hardlink NTFS** (`New-Item -ItemType HardLink`) apontando para o arquivo no clone anterior (`E:\projetos\TooieRecomp\Banjo-Tooie (USA).z64`).
- **Justificativa Técnica:** O plano inicial previa a dependência estrita da variável de ambiente `TOOIE_ROM`. O uso do hardlink compartilha exatamente o mesmo payload físico em disco (zero duplicação de dados, sem cópia de 32 MB), respeita o `.gitignore:25` do projeto e garante que sub-agentes e ferramentas encontrem a ROM sem depender da propagação estável de variáveis de ambiente entre diferentes processos e sub-shells. O suporte a `TOOIE_ROM` permanece ativo como fallback transparente.

---

## 10. Lições de Orquestração dos Sub-Agentes

Durante a execução da fase W1, foram documentadas as seguintes particularidades de ambiente e automação:
1. **Verificação de Exit Codes no Haiku:** O agente Haiku inicialmente classificou testes como FAIL por não capturar explicitamente o exit code 0 na saída textual; os briefs subsequentes foram ajustados para exigir a impressão explícita do código de retorno.
2. **Restrições de Shell Composto:** Comandos encadeados com operadores compostos (como `;` ou `|`) foram bloqueados pelo allowlist do Claude CLI; tarefas de teste devem emitir comandos únicos ou delegar a scripts dedicados.
3. **Efeitos Colaterais dos Hooks do Graft:** A invocação das CLIs dispara hooks do sistema graft, que geram arquivos `.ignore` e modificam `opencode.json`. Esses artefatos permanecem estritamente fora da árvore rastreada e não são adicionados aos commits do repositório.

---

## 11. Escopo Não Executado e Status do Projeto

- As etapas subsequentes permanecem pendentes:
  - W2: Compilação nativa do executável do jogo (`TooieRecompiled.exe`).
  - W3: Paridade de funcionalidades de runtime (música de preview, logs, testes adaptados).
  - W4: Testes de host nativos, empacotamento (`.zip`) e CI.
  - W5: Smoke test interativo e validação manual.
- O status formal do projeto para o ambiente Windows permanece classificado como `WINDOWS_BUILD_PENDING_NATIVE_HOST`.
