# Banjo-Tooie Recompiled

[English](README.md) · **Português (Portugal)** · [Português (Brasil)](README-BR.md) · [Español](README-ES.md)

Adaptação nativa não oficial de Banjo-Tooie (N64) para PC (Windows x64 e Linux x64) feita através de recompilação estática (N64Recomp).

---

## Importante: ROM Necessária

Este projeto contém **ZERO recursos do jogo e NENHUMA ROM**. Necessita da sua própria ROM de Banjo-Tooie (USA) para a N64,
obtida legalmente:

- **Nome do ficheiro:** `Banjo-Tooie (USA).z64` (ou `.n64`)
- **Tamanho do ficheiro:** `33554432` bytes (32 MiB)
- **Hash SHA-1:** `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`

Os gráficos, o áudio e os dados do jogo são lidos e descompactados diretamente a partir da ROM em tempo de execução.

---

## Funcionalidades

- **Ecrã panorâmico 16:9:** Suporte nativo para formato panorâmico com proporção de imagem corrigida.
- **Interpolação de fotogramas:** Jogabilidade fluida a 60 / 120 FPS através de interpolação de transformações
  (`Graphics` → `Interpolation`).
- **Multijogador local (até 4 jogadores):** Ativado em `General` → `Multiplayer (up to 4 players)` (entra em vigor
  após reiniciar o jogo).
  - Entrada individual por comando e/ou teclado para cada jogador.
  - Troca dinâmica de comandos entre jogadores.
  - Mapeamento de botões individual por jogador com opção `Reset to defaults`.
  - Ferramentas de identificação de comandos na página `Controls`: um indicador de entrada e o botão `Rumble 1 s` para testar a vibração.
- **Suporte para comandos modernos e teclado:** Compatibilidade com comandos através de SDL2 e controlos por teclado.
- **Iniciador nativo:** Interface gráfica integrada com seleção de ROM.
- **Suporte a modificações (mods):** Suporte extensível a modificações através do menu `Mods` no jogo.

---

## Transferência e Como Jogar

### Windows (x64)

1. Transfira o pacote `BanjoTooieRecompiled-v<version>-Windows-x64.zip` a partir da
   [página de Releases](https://github.com/wesleytakatsu/TooieRecomp/releases).
2. (Opcional) Verifique a integridade do ficheiro comparando com o ficheiro de verificação `.sha256`.
3. Extraia o ficheiro ZIP comprimido (cria a pasta `BanjoTooieRecompiled`).
4. Execute `TooieRecompiled.exe`.
5. Selecione a sua ROM no iniciador.

**Requisitos e notas:**
- Requer uma placa gráfica com suporte para **Direct3D 12**.
- As definições e os ficheiros de gravação são guardados em: `%LOCALAPPDATA%\TooieRecompiled`

### Linux (x64)

1. Transfira o pacote `BanjoTooieRecompiled-v<version>-Linux-x64.tar.gz` da
   [página de Releases](https://github.com/wesleytakatsu/TooieRecomp/releases) (quando disponível).
2. Extraia o arquivo comprimido e entre na pasta correspondente.
3. Execute `./run-tooie.sh` e selecione a sua ROM no iniciador.

**Requisitos e notas:**
- Requer uma placa gráfica com suporte para **Vulkan**.
- Dependências do sistema: `SDL2`, `FreeType`, `GTK 3`.
- As definições e os ficheiros de gravação são guardados em: `~/.config/TooieRecompiled`

---

## Dicas e Notas

- **Menu no jogo:** Abra o menu de configuração a qualquer momento premindo a tecla `Esc`.
- **Comandos da Xbox no Windows:** Os comandos da Xbox só aceitam o teste de vibração após premir um botão neles
  ("Press a button on it first").
- **Comandos idênticos:** Se ligar múltiplos comandos idênticos sem número de série de hardware único, estes serão
  reatribuídos pela ordem de ligação após reiniciar o jogo.

### Idiomas do jogo

O TooieRecomp suporta múltiplos idiomas de texto no jogo: **English** (predefinido), **Português (Brasil)**, **Français**, **Deutsch** e **Español**.

- **Português (Brasil):** Já vem integrado no jogo. Não necessita de ficheiros adicionais nem de ROM europeia.
- **Français, Deutsch, Español:** Selecione o idioma em **Settings** → **General** → **Game Language** (no iniciador ou no menu `Esc` dentro do jogo). O jogo solicitará a sua ROM europeia (`Banjo-Tooie (Europe) (En,Fr,De,Es).z64`, SHA-1 `93bf2fac1387320ad07251cb4b64fd36bac1d7a6`) apenas uma vez para extrair o pacote de idioma diretamente. A ROM europeia é utilizada APENAS para esta extração pontual; o jogo é sempre executado a partir da sua ROM USA.
- **Aplicação do idioma:** A seleção entra em vigor ao iniciar o jogo. Se for alterada durante a partida, reinicie o jogo para ter efeito.

---

## Compilar a partir do Código-Fonte

Para compilar o TooieRecomp a partir do código-fonte, coloque a sua ROM verificada `Banjo-Tooie (USA).z64` na raiz do repositório.

- **Windows:** Execute `.\tools\windows\Build-Tooie.ps1 -Stage All` no PowerShell.
  Pré-requisitos: Visual Studio 2022+ (com clang-cl), Python 3.8+ e Git for Windows.
  Consulte [tools/windows/README.md](tools/windows/README.md) para obter instruções detalhadas.
- **Linux:** Execute `tools/build_linux.sh` (testado em Ubuntu 24.04 / Linux Mint 22).
  Pré-requisitos: Clang, CMake, Ninja, SDL2, FreeType, GTK 3 e Python 3.
  Consulte [tools/BUILDING.md](tools/BUILDING.md) para obter as instruções completas.

---

## Créditos

- **Tecnologias principais:** Desenvolvido com [N64Recomp](https://github.com/N64Recomp/N64Recomp) / N64ModernRuntime,
  [RT64](https://github.com/rt64/rt64), RecompFrontend ([RmlUi](https://github.com/mikke89/RmlUi)) e
  [SDL2](https://www.libsdl.org/).
- **Inspirações:** Inspirado no trabalho pioneiro de [Zelda64Recomp](https://github.com/Zelda64Recomp/Zelda64Recomp) e
  [BanjoRecomp](https://github.com/BanjoRecomp/BanjoRecomp).
- **Descompilação:** Comunidade de descompilação de Banjo-Tooie pelo trabalho de engenharia inversa no jogo original.
- **Licenças de terceiros:** Listadas em [`vendor/`](vendor/README.md) e incluídas em cada pacote de lançamento sob
  `licenses/` e `THIRD-PARTY-MANIFEST.tsv`.

---

## Licença

O código próprio do TooieRecomp está licenciado sob a [GNU General Public License v3.0](LICENSE). O código traduzido do
jogo, a descompilação em `decomp/` e o conteúdo do jogo não são abrangidos por esta licença. Os componentes de
terceiros mantêm as suas próprias licenças (ver acima).

---

## Aviso Legal

Banjo-Tooie é © Nintendo / Rare.

Este é um projeto não oficial criado por fãs e não tem qualquer afiliação nem apoio da Nintendo ou da Rare.

O executável contém código traduzido a partir do jogo; utilize uma ROM extraída de um cartucho original que possua. Não peça ROMs.
