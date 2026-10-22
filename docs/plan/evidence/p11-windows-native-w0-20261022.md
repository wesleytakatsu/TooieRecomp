---
title: "BT P11 — Evidência W0: Preparação do Ambiente e Validação do Check Nativo no Windows 11"
description: "Evidência técnica da conclusão das etapas preparatórias W0 (W0.1 a W0.6) para o build nativo do TooieRecomp no Windows 11 x64 com clang-cl 20.1.8 e Build-Tooie.ps1 Check exit 0."
status: current
type: evidence
project: TooieRecomp
phase: P11
updated: 2026-10-22
tags: [windows, p11, w0, build, clang-cl, environment, preparation, ps1, powershell]
---

# BT P11 — Evidência W0: Preparação do Ambiente e Validação do Check Nativo no Windows 11

## 1. Host e Configuração do Sistema

- **Sistema Operacional:** Windows 11 Pro for Workstations 10.0.22631.
- **IDE / Compilador:** Visual Studio Community 2026 (18.4.11626.88) em `C:\Program Files\Microsoft Visual Studio\18\Community`.
- **SDK:** Windows SDK 10.0.26100.0.
- **Python:** Python 3.13.7 (`C:\Python313\python.exe`; `python3` no PATH é o stub da Microsoft Store).
- **Git:** Git 2.51.0.windows.1.

---

## 2. W0.1: Sonda de Escrita e Execução das CLIs de Sub-Agente (`PROVEN`)

Sonda de escrita das CLIs de sub-agente realizada com sucesso:
- **Cursor (`grok-4.7-medium`):** escreve arquivos; shell somente via allowlist e apenas quando lançado do PowerShell (no Git Bash o hook `rtk hook claude` importado do Claude Code quebra com o erro `syntax error near unexpected token '&'`).
- **AGY (`gemini-3.8-flash-high`, `--mode accept-edits`):** escreve arquivos; `run_command` é sempre negado em modo headless e a execução encerra sem evento `result` com exit 0.
- **Claude CLI (`claude-haiku-5-5`, `--permission-mode acceptEdits`):** escreve arquivos; `--allowedTools` necessita da variante com prefixo `rtk ` (o hook RTK reescreve o comando antes da checagem); comandos só-leitura podem ser autoaprovados.
- **Guia externo:** `E:\projetos\sub-agent-usage-instrucions.md` (fora do repositório).

---

## 3. W0.2: Instalação do Compilador Clang no Visual Studio (`PROVEN`)

- Instalação dos componentes `Microsoft.VisualStudio.Component.VC.Llvm.Clang` e `Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset` via `setup.exe modify ... --passive --norestart`.
- **Tentativas:**
  - 1ª tentativa (sem elevação): falhou com exit 5007 (`"Commands with --quiet or --passive should be run elevated"`).
  - 2ª tentativa (elevada): exit 0.
- **Binários verificados:** `clang-cl` disponível (clang version 20.1.8, Target `x86_64-pc-windows-msvc`); `lld-link.exe` presente.

---

## 4. W0.3: Clone Limpo com LF Obrigatório (`PROVEN`)

- Materialização do clone de trabalho dedicado: `git clone -c core.autocrlf=false https://github.com/wesleytakatsu/TooieRecomp.git E:\projetos\TooieRecomp-win`, apontando para o HEAD `b59ddae` (`= origin/main`).
- `git ls-files --eol`: 0 arquivos com terminação mista `i/lf w/crlf`.
- Verificação de dependências vendorizadas: `bash tools/vendor_verify.sh` reportou `manifest: 8266 files checked`, `VENDOR VERIFY OK` (checagens de exec-bit são puladas [SKIP] em MINGW64, conforme comportamento documentado).
- Comparativo com clone anterior: o clone antigo em `E:\projetos\TooieRecomp` usava `core.autocrlf=true` e continha 476 arquivos `i/lf w/crlf`, incluindo arquivos críticos como `metadata/recomp-output.sha256` e `metadata/tooie.us.syms.toml`.

---

## 5. W0.4: Normalização de Final de Linha em `.gitattributes` (`PROVEN`)

- O arquivo `.gitattributes` recebeu a regra `* text=auto eol=lf` no topo, preservando todos os overrides `-text` existentes.
- **Prova:** execução de `git add --renormalize .` utilizando um índice temporário (`GIT_INDEX_FILE`), que modificou unicamente os arquivos já editados no worktree, sem normalizar nenhum blob rastreado existente. `bash tools/vendor_verify.sh` manteve o status OK após a alteração.

---

## 6. W0.5: Formalização da Fase P11 no Vault (`PROVEN`)

- Plano movido para `docs/plans/BT/11-windows-native-build.md` mantendo o corpo idêntico ao plano original (apenas o título ajustado para o padrão de fases do vault).
- Índices (`docs/plans/BT/index.md`, `docs/plan/index.md`) e `docs/HANDOFF.md` devidamente ligados.

---

## 7. W0.6: Validação do Estágio Check do `Build-Tooie.ps1` (`PROVEN`)

Execução de `tools/windows/Build-Tooie.ps1 -Stage Check` com exit 0 tanto em PowerShell 7 quanto em Windows PowerShell 5.1.

**Saída verificada:**
- `ok git 2.51.0.windows.1`
- `ok cmake 4.2.3-msvc3`
- `ok ninja 1.12.1`
- `ok clang-cl 20.1.8`
- `ok python C:\Python313\python.exe 3.13.7`
- `ok bash C:\Program Files\Git\bin\bash.exe`
- `ok line endings (LF)`
- `VENDOR VERIFY OK`
- `ok lib/SDL2-windows (vendored)`
- `MISS RecompiledFuncs (native Extract/Recomp stages: P11 W1; fallback: -Stage ImportGenerated)` (esperado nesta etapa).

**Defeitos corrigidos durante o desenvolvimento (3 iterações: implementação Grok, testes Haiku):**
1. Checagem de ferramentas `cmake` e `ninja` antes de invocar o Developer shell;
2. Novo `Find-Python` pulando o executável stub de `WindowsApps` (a 1ª versão achatava o array de candidatos, resultando em `'py -3'` tratado como nome único de comando; corrigido);
3. `vswhere.exe` ausente do PATH dentro de `VsDevCmd.bat` (o diretório do VS Installer agora é prefixado no PATH);
4. Defeito antigo pré-existente em `Find-Bash` corrigido (pipeline sem `@()` tratava string como array de caracteres, avaliando `$c[0]` como `'C'`).

---

## 8. Escopo Não Executado e Status do Projeto

- Nenhum estágio posterior de pipeline nativo foi executado até o momento: zero execuções de `Extract`, `Recomp`, `Build`, `Test` ou `Package` (escopo das tarefas W1 em diante).
- O status do build Windows permanece inalterado como `WINDOWS_BUILD_PENDING_NATIVE_HOST`.

---

## 9. Efeitos Colaterais Observados (Não Versionados)

- A invocação dos hooks do graft altera `opencode.json` (adicionando a configuração do servidor MCP `graft`) e gera o arquivo `.ignore`.
- Estes arquivos não integram as entregas da fase P11 e não devem ser adicionados ao stage nem versionados no Git.
