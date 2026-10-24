# Banjo-Tooie Recompiled

[English](README.md) · [Português (Portugal)](README-PT.md) · **Português (Brasil)** · [Español](README-ES.md)

Port nativo não oficial de Banjo-Tooie (N64) para PC (Windows x64 e Linux x64) feito com recompilação estática (N64Recomp).

---

## Importante: ROM Necessária

Este projeto **não contém nenhum recurso (asset) do jogo nem ROM**. Você precisa da sua própria ROM de Banjo-Tooie (USA) de N64,
obtida legalmente:

- **Nome do arquivo:** `Banjo-Tooie (USA).z64` (ou `.n64`)
- **Tamanho do arquivo:** `33554432` bytes (32 MiB)
- **Hash SHA-1:** `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`

Gráficos, áudio e dados do jogo são lidos e descompactados diretamente da ROM durante a execução.

---

## Recursos

- **Widescreen 16:9:** Suporte nativo a telas panorâmicas (widescreen) com proporção de tela corrigida.
- **Interpolação de quadros:** Jogabilidade fluida a 60 / 120 FPS via interpolação de transformações
  (`Graphics` → `Interpolation`).
- **Multijogador local (até 4 jogadores):** Ativado em `General` → `Multiplayer (up to 4 players)` (passa a valer
  após reiniciar o jogo).
  - Controle e/ou teclado individual por jogador.
  - Troca dinâmica de controles entre os jogadores.
  - Mapeamento de botões individual por jogador com opção `Reset to defaults`.
  - Ferramentas de identificação na página `Controls`: indicador visual de entrada e botão de teste `Rumble 1 s` (vibração).
- **Suporte a controles modernos e teclado:** Compatibilidade total com controles via SDL2 e comandos customizáveis por teclado.
- **Launcher nativo:** Interface gráfica integrada para seleção da ROM e ajustes.
- **Suporte a mods:** Modificações de jogo extensíveis através do menu `Mods`.

---

## Download e Como Jogar

### Windows (x64)

1. Baixe o pacote `BanjoTooieRecompiled-v<version>-Windows-x64.zip` na
   [página de Releases](https://github.com/wesleytakatsu/TooieRecomp/releases).
2. (Opcional) Verifique a integridade do pacote conferindo o hash com o arquivo `.sha256` correspondente.
3. Extraia o arquivo ZIP (será criada uma pasta chamada `BanjoTooieRecompiled`).
4. Execute `TooieRecompiled.exe`.
5. Selecione sua ROM no launcher.

**Requisitos e caminhos:**
- Requer uma placa de vídeo compatível com **Direct3D 12**.
- Configurações e saves são salvos em: `%LOCALAPPDATA%\TooieRecompiled`

### Linux (x64)

1. Baixe o pacote `BanjoTooieRecompiled-v<version>-Linux-x64.tar.gz` na
   [página de Releases](https://github.com/wesleytakatsu/TooieRecomp/releases) (quando disponível).
2. Extraia o pacote compactado e acesse a pasta extraída.
3. Execute `./run-tooie.sh` e selecione sua ROM no launcher.

**Requisitos e caminhos:**
- Requer uma placa de vídeo compatível com **Vulkan**.
- Bibliotecas do sistema: `SDL2`, `FreeType`, `GTK 3`.
- Configurações e saves são salvos em: `~/.config/TooieRecompiled`

---

## Dicas e Observações

- **Menu no jogo:** Abra o menu de configuração a qualquer momento pressionando `Esc`.
- **Controles de Xbox no Windows:** Os controles de Xbox só respondem ao teste de vibração após pressionar um botão neles
  ("Press a button on it first").
- **Controles idênticos:** Se você conectar múltiplos controles idênticos sem número de série próprio, eles serão
  reatribuídos na ordem de conexão ao reiniciar o jogo.

---

## Compilação a partir do Código-Fonte

Para compilar o TooieRecomp a partir do código-fonte, coloque sua ROM verificada `Banjo-Tooie (USA).z64` na raiz do repositório.

- **Windows:** Execute `.\tools\windows\Build-Tooie.ps1 -Stage All` no PowerShell.
  Pré-requisitos: Visual Studio 2022+ (com clang-cl), Python 3.8+ e Git for Windows.
  Consulte [tools/windows/README.md](tools/windows/README.md) para instruções detalhadas.
- **Linux:** Execute `tools/build_linux.sh` (testado no Ubuntu 24.04 / Linux Mint 22).
  Pré-requisitos: Clang, CMake, Ninja, SDL2, FreeType, GTK 3 e Python 3.
  Consulte [tools/BUILDING.md](tools/BUILDING.md) para instruções completas.

---

## Créditos

- **Tecnologias principais:** Desenvolvido com [N64Recomp](https://github.com/N64Recomp/N64Recomp) / N64ModernRuntime,
  [RT64](https://github.com/rt64/rt64), RecompFrontend ([RmlUi](https://github.com/mikke89/RmlUi)) e
  [SDL2](https://www.libsdl.org/).
- **Inspirações:** Inspirado no trabalho pioneiro de [Zelda64Recomp](https://github.com/Zelda64Recomp/Zelda64Recomp) e
  [BanjoRecomp](https://github.com/BanjoRecomp/BanjoRecomp).
- **Descompilação:** Comunidade de descompilação de Banjo-Tooie pela engenharia reversa do jogo original.
- **Licenças de terceiros:** Listadas em [`vendor/`](vendor/README.md) e incluídas em cada pacote de lançamento sob
  `licenses/` e `THIRD-PARTY-MANIFEST.tsv`.

---

## Aviso Legal

Banjo-Tooie é © Nintendo / Rare.

Este é um projeto não oficial feito por fãs e não possui qualquer vínculo ou endosso da Nintendo ou da Rare.

O executável contém código traduzido a partir do jogo original; utilize uma ROM extraída de um cartucho próprio que você possua. Não peça ROMs.
