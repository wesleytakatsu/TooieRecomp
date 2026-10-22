---
title: "BT P11 — Evidência W2: Primeiro Build Nativo do Jogo no Windows 11"
description: "Evidência técnica da conclusão das etapas W2 (W2.1 a W2.6) do build nativo no Windows 11 x64: TooieRecompiled.exe (23.944.704 bytes) linkado nativamente via clang-cl/lld-link, resolução de conflitos de macros Win32 no RT64, experimentos de sobrescrita de símbolos COFF e /wrap:, descompressão transparente de ROM de cartucho (.z64/.n64) no launcher, e inicialização do jogo confirmada pelo usuário."
status: current
type: evidence
project: TooieRecomp
phase: P11
updated: 2026-10-22
tags: [windows, p11, w2, build, clang-cl, lld-link, ninja, rt64, recomp, weak-symbols, wrap, smoke-test, rom-cart, decompressor, user-confirmed]
---

# BT P11 — Evidência W2: Primeiro Build Nativo do Jogo no Windows 11

## 1. Host, Base e Sumário Executivo

- **Base de Código:** Commit C2 `9253367` no clone de trabalho dedicado `E:\projetos\TooieRecomp-win`.
- **Toolchain do Host:** `clang-cl` 20.1.8 + `lld-link` + Ninja 1.12.1 (LLVM / Clang Toolset no Visual Studio 2026 18 Community), Windows SDK 10.0.26100.0.
- **Python:** Python 3.13.7 (`C:\Python313\python.exe`; `python3` no PATH aponta para o stub da Microsoft Store e é ignorado pelos scripts).
- **Shell / Runtime:** Windows PowerShell 5.1 e PowerShell 7.x.
- **Entrada de Geração:** `RecompiledFuncs/` 420/420 arquivos verificado e validado byte a byte contra `metadata/recomp-output.sha256` (conclusão da etapa W1).
- **Status Anterior:** W1 concluída ([[p11-windows-native-w1-20261022]]).

---

## 2. Resultado Principal: Executável Nativo Linkado e Smoke Test (`PROVEN`)

O executável nativo do jogo foi completamente compilado e linkado no Windows 11 sem emulação POSIX ou WSL:
- **Artefato Gerado:** `build-win/TooieRecompiled.exe` com tamanho de `23.944.704` bytes.
- **DLLs Adjacentes:** `SDL2.dll`, `dxil.dll` e `dxcompiler.dll` copiadas automaticamente para `build-win/` pelo target de build.
- **Pipeline de Compilação:**
  - Invocação: `.\tools\windows\Build-Tooie.ps1 -Stage Configure` e `.\tools\windows\Build-Tooie.ps1 -Stage Build` concluídos com exit code 0 (`exit 0`).
  - Total de Passos Ninja: 1212 passos.
  - Diagnósticos de Compilação: 676 warnings de compilação, 0 avisos de `argument unused`.
  - Compatibilidade de Flags: As flags GNU configuradas para `RecompiledFuncs` (`-fno-strict-aliasing`, `-falign-functions=16`, `-Wno-*`) foram aceitas diretamente pelo driver `clang-cl` sem necessidade de envelopamento adicional.
- **Smoke Test de Execução (`PROVEN`):**
  - Executado via `smoke-exe.ps1` (helper do orquestrador executado externamente ao repositório).
  - Comportamento: Processo permaneceu vivo e responsivo após 15 segundos.
  - Janela Criada: Janela nativa com título `Banjo-Tooie: Recompiled`.
  - Consumo de Memória: Working set de 404 MB.
  - Término: Fechamento limpo reportando exit code 0 (`exit 0`).
- **Validação de Interface e Início do Jogo (`PROVEN`):** Confirmação do usuário (2026-10-22): *"Testei com Banjo-Tooie (USA).z64: o launcher aceitou e o jogo iniciou."* — substitui em definitivo o `PENDING_USER_VISUAL` da etapa W2.5 (launcher visualizado e jogo iniciado com sucesso utilizando a ROM original de cartucho).

---

## 3. Triagem de Falhas e Descoberta Inicial de TU (`PROVEN`)

Durante a primeira execução do build nativo coordenada pelo orquestrador:
- O estágio `-Stage Configure` concluiu com sucesso (`exit 0`).
- A compilação completa com tolerância a falhas (`ninja -k 0`) falhou com exit code 1 (`exit 1`) em 279 segundos, registrando 196 erros em 29 unidades de tradução (TUs).
- **Causa Raiz Única Identificada:**
  - Todos os 196 erros decorreram de uma única causa em `lib/rt64/src/hle/rt64_interp_burst.h` (código de interpolação P09 do TooieRecomp).
  - O cabeçalho declarava os enumeradores `DUPLICATE` e `ERROR`, que colidiam frontalmente com macros globais injetadas pelos cabeçalhos do Windows SDK:
    - `DUPLICATE`: definido como macro em `<nb30.h>`.
    - `ERROR`: definido como macro em `<wingdi.h>` (via `<windows.h>`).

---

## 4. Correções de TU e Gestão de Patches Vendorizados (`PROVEN`)

As correções de código em dependências vendorizadas foram implementadas por Sonnet e integradas estritamente sob as regras de `vendor/UPDATING.md`:
- Registradas entradas de ordem em `vendor/patches-applied/ORDER.tsv`.
- Registradas notas `LOCAL:` em `vendor/UPSTREAMS.tsv` para o rt64.
- Atualizadas exclusivamente as linhas de hash dos arquivos modificados em `vendor/MANIFEST.sha256` (o script `tools/vendor_manifest.sh` não foi executado no Windows para prevenir corrupção das permissões POSIX no NTFS).
- **Patches Implementados:**
  1. `vendor/patches-local/rt64-interp-burst-windows-macro-names.patch` (SHA-256 `fad6b8e4a87d0e572e6f8773410d1df122a729216e80116c44e12a0eb219739f`):
     - Renomeia `Reason::DUPLICATE` para `Reason::DUPLICATE_KEY`.
     - Renomeia `Controller::Result::ERROR` para `Controller::Result::FAILED`.
     - Preserva estritamente inalteradas as mensagens e strings textuais emitidas para logs.
     - Aplica a mesma renomeação ao teste de unidade `tests/p09_interp_burst_test.cpp`.
  2. `vendor/patches-local/rt64-transform-processor-windows-getpid.patch` (SHA-256 `44315cb400b466ff3ad1123335b6dca262ee41147210905e13e1866a4a0b07ae`):
     - Substitui `<unistd.h>` e a chamada `getpid()` por `<process.h>` e `_getpid()` estritamente sob a condicional `#if defined(_WIN32)`.
- **Verificação Reversa dos Patches:**
  - `bash tools/vendor_verify.sh --reverse-check` concluiu com exit code 0 (`exit 0`), aplicando reversamente e conferindo com sucesso 23 patches (`VENDOR VERIFY OK`).

---

## 5. Linkagem — Sobrescrita de Símbolos Fracos (`RECOMP_FUNC`) no Formato COFF (`PROVEN`)

### 5.1. O Problema das Definições Fracas no Modelo Windows COFF
- No formato objeto COFF utilizado pelo toolchain MSVC/Clang-cl no Windows, a declaração `extern inline __attribute__((weak))` emite símbolos COMDAT.
- Ao contrário do modelo ELF no Linux, o linker do Windows (`lld-link`) identificava colisão e emitia erro de `duplicate symbol` ao encontrar as ~31 definições fortes de substituição de funções recompiladas (presentes em `boot_hw_stubs.cpp`, `transform_hooks.cpp` e `backpack_lookup_diag.cpp`).
- **Decisão Arquitetural do Orquestrador:** Rejeitado terminantemente o uso da flag `/FORCE:MULTIPLE`. Essa flag mascara duplicações reais indesejadas e introduz comportamento não-determinístico dependente da ordem dos arquivos na linha de comando do linker. Exigida investigação causal e formulação de experimento discriminante mínimo.

### 5.2. Experimento A: Discriminante Mínimo de Símbolos Fracos em COFF (`PROVEN`)
Um harness isolado foi executado para avaliar a semântica de linkage:
- **A1 (Comportamento Original):** `extern inline __attribute__((weak))` → `duplicate symbol` no `lld-link`.
- **A2 (Modificador weak + noinline):** `__attribute__((weak, noinline))` → linka perfeitamente no `lld-link`. A definição forte substitui e vence a fraca tanto para chamadas originadas em `main` quanto para invocações internas dentro do próprio objeto gerado (`f=2 g=12`).
- **A3 (Fallback sem Definição Forte):** Quando nenhuma definição forte é provida no grafo de link, o linker utiliza normalmente a implementação fraca padrão (`f=1 g=11`).

### 5.3. Implementação da Correção
- Criado o patch `vendor/patches-local/n64recomp-coff-weak-recomp-func.patch` (SHA-256 `421fe06f5ac3ab2bca4989d75fc7e823a60f959a8a662fe9b928b2e476a75579`):
  - Em `lib/N64ModernRuntime/N64Recomp/include/recomp.h`, adicionado o ramo específico:
    ```c
    #elif defined(__clang__) && defined(_WIN32)
    #define RECOMP_FUNC extern __attribute__((weak, noinline))
    ```
  - O código C gerado em `RecompiledFuncs/*.c` permaneceu 100% inalterado (a macro é expandida no cabeçalho em tempo de compilação).
  - O ambiente Linux permanece usando o ramo original sem qualquer alteração comportamental.

---

## 6. Linkagem — Envolvimento de Símbolos (`--wrap` vs `/wrap:`) (`PROVEN`)

### 6.1. Divergência do Shim Histórico
- O mecanismo pré-existente de compatibilidade no Windows utilizava shims manuais (`TOOIE_REAL_SHIM` em `backpack_lookup_diag.cpp`), interceptando exclusivamente consultas via tabela de despacho de lookup.
- Chamadas diretas entre unidades de tradução ignoravam o shim e não alcançavam `__wrap_*`, criando assimetria comportamental entre os binários Linux e Windows.

### 6.2. Experimento B: Semântica de `/wrap:` no lld-link (`PROVEN`)
- O experimento confirmou que o linker `lld-link` aceita diretamente o argumento `/wrap:f`, reproduzindo a semântica do `--wrap` do GNU ld:
  - Referências a `__real_f` resolvem para a implementação original `f`.
  - Referências a `f` resolvem para o wrapper `__wrap_f`.
- **Diferença Semântica Identificada e Arbitrada:**
  - O `lld-link` envelopa inclusive chamadas originadas dentro da própria unidade de tradução onde o símbolo é definido, enquanto o GNU ld envelopa apenas chamadas entre diferentes objetos.
  - No TooieRecomp, essa divergência tem **impacto nulo** (`NO_IMPACT`): existem rigorosamente 0 chamadas diretas entre os 18 símbolos interceptados dentro dos arquivos `RecompiledFuncs/*.c`; todos os despachos transitam unicamente através dos ponteiros indexados na tabela `recomp_overlays.inl`.

### 6.3. Unificação no Sistema de Build
- `CMakeLists.txt` atualizado:
  - Criada a lista única e canônica `TOOIE_WRAPPED_SYMBOLS` contendo os 18 símbolos interceptados.
  - No Windows, as diretivas são emitidas como `LINKER:/wrap:<sym>`.
  - No Linux, as diretivas continuam emitidas como `-Wl,--wrap=<sym>`.
  - O mecanismo legado de shims manuais foi integralmente removido da base de código.

---

## 7. W2.1: Estágios de Build no PowerShell (`Build-Tooie.ps1`) (`PROVEN`)

Implementação por Grok:
- Adicionados os estágios `-Stage Configure`, `-Stage Build` e `-Stage All`.
- O estágio `-Stage All` encadeia a execução sequencial completa: `Check` → `Extract` → `Recomp` → `Configure` → `Build` (os estágios `Test` e `Package` serão ativados na etapa W4).
- Paridade de caminhos de compilação obtida passando `/clang:-ffile-prefix-map=<root>=/tooie` nas flags de C e C++.
- Paralelismo automático: o parâmetro `-Jobs` assume como padrão o número de processadores lógicos do sistema (`$env:NUMBER_OF_PROCESSORS`), aceitando override explícito pelo usuário.

---

## 8. W2.6 — Importação da ROM de Cartucho no Launcher (`PROVEN`)

### 8.1. Problema Relatado e Diagnóstico Causal (`PROVEN`)
- **Problema Relatado pelo Usuário:** O launcher nativo recusava a ROM original de cartucho NTSC-U `Banjo-Tooie (USA).z64` (33.554.432 bytes) bem como a imagem byteswapped `.n64`, emitindo a mensagem de erro:
  `"This ROM is the correct game, but the wrong version..."`
- **Causa Raiz (`PROVEN`, leitura de código):**
  - Em `src/main/main.cpp`, `GameEntry.rom_hash` foi configurado com o hash XXH3-64 da imagem **descomprimida** (`0x9F843AD3455EA395`, 35.098.224 bytes).
  - Em `librecomp`, a função `recomp::select_rom` comparava unicamente esse hash com a imagem fornecida.
  - Ao carregar a ROM de cartucho, o nome interno no cabeçalho N64 batia, porém o hash XXH3 do cartucho bruto (`0x00F70DE5F2D70EA2`) não conferia com o hash da imagem descompactada, disparando o retorno de erro `IncorrectVersion`.
  - Este comportamento não era um defeito específico do Windows, ocorrendo de forma idêntica no Linux.

### 8.2. Correção W2.6 e Decisão de Desenho do Orquestrador
A decisão de desenho do orquestrador determinou reutilizar o algoritmo de descompressão do estágio Extract (já comprovado pelo SHA-1 canônico `af479771ff5e91ce3732f3d84146614ab71d8d79`), implementando a descompressão transparente em memória no momento da seleção da ROM (implementação por Sonnet):

1. **Descompressor em Memória (`vendor/patches-local/decomp-rom-decompressor-in-memory.patch`):**
   - SHA-256: `d497b67ead6d616647ddf6236fabb479010a9687bc9f93108feabb9f3fde6d72`.
   - Registrado novo grupo `decomp` em `vendor/patches-applied/ORDER.tsv`.
   - Implementa `tooie_decompress_rom()` em memória em `decomp/tools/rom_decompressor_lib.h`, sem criar arquivos colaterais quando `relocs_dir == nullptr`, e com falha limpa para entradas inválidas.
   - O `main()` do Extract CLI mantém estritamente sua interface e saída (Extract reexecutado produziu SHA-1 `af479771…` rigorosamente igual).
2. **Hook de Descompressão no Runtime (`vendor/patches-local/n64modernruntime-select-rom-decompress-cart.patch`):**
   - SHA-256: `fae99e0a4971a035377cb92b6c5c6ab67ec8dcdf4089c398aebb91013636cc73`.
   - Em `recomp::select_rom`, se o hash não confere inicialmente, mas há `decompression_routine` registrada e `!has_compressed_code`, a rotina é invocada; a ROM só é aceita se o XXH3-64 do resultado coincidir com o hash esperado.
   - A lógica de erro original permanece intacta e o caminho de mods segue inalterado.
3. **Integração no Jogo (`src/game/rom_decompression.cpp` e CMake):**
   - Função `decompress_tooie` descomprime quando tamanho = 32 MiB (33.554.432 bytes) e XXH3-64 = `0x00F70DE5F2D70EA2` (medido na ROM comercial do usuário); qualquer outra entrada passa inalterada.
   - No `CMakeLists.txt`, criada a biblioteca `tooie_cart_decompress`, utilizando o `fmt` 11.1.0 e o `miniz` já linkados pelo projeto (a 1ª tentativa utilizando o `format.cc` do decomp resultou no erro de link `duplicate symbol fmt::v11`, resolvido com a reutilização das bibliotecas já presentes).
   - Adicionado o alvo de teste de unidade `rom_cart_import_test`.

### 8.3. Verificação Automatizada e Integridade (`PROVEN`)
- **Suite de Testes `rom_cart_import_test` (`PROVEN`):**
  - Execução: `PASS ... (decompressed 35098224 bytes, XXH3-64 0x9F843AD3455EA395)`.
  - Casos cobertos: ROMs em formato nativo big-endian `.z64`, formato byteswapped `.n64` (normalização de swap de 2 bytes), imagens já descomprimidas passadas inalteradas, e rejeição graciosa sem falha catastrófica de imagens inválidas, truncadas ou corrompidas.
- **Auditoria de Patches Vendorizados:**
  - `bash tools/vendor_verify.sh --reverse-check` concluiu com exit code 0 (`exit 0`), aplicando reversamente e validando 25 patches.
  - Verificação confirmada também pelo orquestrador.

### 8.4. Confirmação do Usuário (`PROVEN`, 2026-10-22)
- Validação interativa direta reportada pelo usuário:
  > *"Testei com Banjo-Tooie (USA).z64: o launcher aceitou e o jogo iniciou."*
- Essa evidência substitui em definitivo o status `PENDING_USER_VISUAL` da etapa W2.5 (launcher visualizado e início do jogo comprovado em ambiente real com a ROM de cartucho comercial).

### 8.5. Pendência Menor Identificada
- A dica final exibida ao término de `tools/extract_rom.py` (*"choose THAT decompressed.us.z64 (NOT the retail .z64)"*) tornou-se tecnicamente desatualizada agora que o cartucho comercial é aceito de forma nativa e transparente. Essa atualização textual foi registrada como a tarefa W3.5 da etapa W3.

---

## 9. Escopo Não Executado (`NOT_RUN`) e Status do Projeto

- **Build Linux Pós-Refatoração (`NOT_RUN` / `STRONG_INFERENCE`):** A compilação em Linux após a introdução de `TOOIE_WRAPPED_SYMBOLS` no `CMakeLists.txt` e a renomeação dos enumeradores em `rt64_interp_burst.h` não foi reexecutada nesta sessão; trata-se de modificação mecânica com forte inferência de equivalência funcional.
- **Teste Unitário `p09_interp_burst_test` (`NOT_RUN`):** Execução do teste unitário de interpolação não realizada nesta etapa.
- **Gameplay Estendido (`NOT_RUN`):** Nenhuma execução interativa do jogo além da abertura do launcher e início do jogo confirmados pelo usuário foi realizada.
- **Testes de Host Nativos (`NOT_RUN`):** A execução e validação da suite de testes de host pertencem ao escopo da etapa W4.
- **Status do Projeto para Windows:** Embora o binário nativo `TooieRecompiled.exe` esteja comprovadamente construído, aceite a ROM de cartucho e o jogo inicialize com sucesso comprovado pelo usuário, o status formal do projeto permanece categorizado como `WINDOWS_BUILD_PENDING_NATIVE_HOST` até a conclusão e validação manual integral da etapa W5.
