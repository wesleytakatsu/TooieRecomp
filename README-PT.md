# Banjo-Tooie Recompiled

[English](README.md) · **Português (Portugal)** · [Português (Brasil)](README-BR.md) · [Español](README-ES.md) · [Italiano](README-IT.md) · [日本語](README-JA.md)

Adaptação nativa não oficial de Banjo-Tooie (N64) para PC (Windows x64 e Linux x64) feita através de recompilação estática (N64Recomp).

**Mod de música em HD:** [transfira o mod da banda sonora em HD](https://drive.google.com/file/d/1GRaaK0vPQz0tbBfj6oja9Q0x98VPki4d/view?usp=drive_link) (v0.1.6 ou mais recente). Instale o ZIP
através do menu `Mods` no jogo, ou copie-o sem extrair para `%LOCALAPPDATA%\TooieRecompiled\mods` (Linux:
`~/.config/TooieRecompiled/mods`), e ative-o em `Mods`.

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
- **Distância dos objetos (experimental):** `Graphics` → `Object Distance` desenha e mantém objetos mais distantes
  (2x, 4x, Unlimited). Estas definições necessitam de `General` → `Expanded Memory (experimental)`, que concede ao jogo
  uma área de memória maior; escolher uma distância superior ativa-a automaticamente, e entra em vigor após reiniciar
  o jogo. O ecrã dividido no modo história utiliza sempre a distância original.
- **Multijogador local (até 4 jogadores):** Ativado em `General` → `Multiplayer (up to 4 players)` (entra em vigor
  após reiniciar o jogo).
  - Entrada individual por comando e/ou teclado para cada jogador.
  - Troca dinâmica de comandos entre jogadores.
  - Mapeamento de botões individual por jogador com opção `Reset to defaults`.
  - Ferramentas de identificação de comandos na página `Controls`: um indicador de entrada e o botão `Rumble 1 s` para testar a vibração.

  **Aviso:** O ecrã dividido no modo história (Dev Mode → Experimental → `Two players (split screen)`) é experimental e
  pode quebrar o jogo (bloqueios, encerramentos forçados, avanço na história corrompido). Há ainda muito a ajustar e
  existem já muitos erros conhecidos; a funcionalidade está a ser melhorada aos poucos. Guarde o seu jogo antes de a
  utilizar; recomenda-se ativar `General` → `Expanded Memory (experimental)`.
- **Suporte para comandos modernos e teclado:** Compatibilidade com comandos através de SDL2 e controlos por teclado.
- **Iniciador nativo:** Interface gráfica integrada com seleção de ROM.
- **Pacotes de texturas:** pacotes de texturas do RT64 (`.rtz`) através do menu `Mods` no jogo.
- **Pacotes de texturas em OpenGL ou DirectX 11:** as texturas dos mods ainda precisam de correção nesses modos
  (algumas aparecem pretas). Use Direct3D 12 ou Vulkan com pacotes de texturas.
- **Modo de programador (Dev Mode):** separador do menu ESC desbloqueado depois de um aviso. `Warp` guarda o sítio
  onde o Banjo está e teletransporta-o de volta mais tarde, também a partir de outro nível. Altera o estado do jogo
  diretamente: guarde o jogo antes de o usar.

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
   [página de Releases](https://github.com/wesleytakatsu/TooieRecomp/releases).
2. Extraia o arquivo comprimido e entre na pasta correspondente.
3. Execute `./run-tooie.sh` e selecione a sua ROM no iniciador.

A release inclui também um pacote `.deb` (Debian/Ubuntu/Mint) e um `.AppImage`.

**Requisitos e notas:**
- Requer uma placa gráfica com suporte para **Vulkan**.
- Dependências do sistema: `SDL2`, `FreeType`, `GTK 3`.
- As definições e os ficheiros de gravação são guardados em: `~/.config/TooieRecompiled`

### Portáteis (PortMaster, aarch64)

1. Instale `BanjoTooieRecompiled-v<version>-Linux-arm64-PortMaster.zip` com o PortMaster.
2. Copie a sua ROM para `ports/tooie` e inicie **Banjo-Tooie** na lista de Ports. O primeiro arranque converte a ROM uma única vez.

**Requisitos e notas:**
- Requer OpenGL ES 3.1 e um firmware com Wayland ou X11. Testado apenas na MagicX XU10 com ROCKNIX; outros aparelhos não foram testados.
- Resolução original e 4:3. Da primeira vez que cada cena aparece, os seus shaders são compilados e o jogo pode parar por instantes.
- Select + Start pergunta se quer sair. As definições e as gravações ficam em `ports/tooie`.

---

## Dicas e Notas

- **Menu no jogo:** Abra o menu de configuração a qualquer momento premindo a tecla `Esc`.
- **Comandos da Xbox no Windows:** Os comandos da Xbox só aceitam o teste de vibração após premir um botão neles
  ("Press a button on it first").
- **Comandos idênticos:** Se ligar múltiplos comandos idênticos sem número de série de hardware único, estes serão
  reatribuídos pela ordem de ligação após reiniciar o jogo.

### Idiomas do jogo

O TooieRecomp suporta múltiplos idiomas de texto no jogo: **English** (predefinido), **Português (Brasil)**, **Français**, **Deutsch**, **Español**, **Italiano** (versão preliminar) e **日本語** (Japonês).

- **Português (Brasil), Français, Deutsch, Español, Italiano, 日本語:** Estão incluídos no pacote de lançamento; apenas a ROM USA é necessária.
- **Italiano (versão preliminar):** Combina a tradução italiana Rulesless v0.9 com uma tradução suplementar a aguardar revisão editorial. Créditos: Rulesless (https://letraduzionidirulesless.wordpress.com/banjo-tooie/).
- **O idioma japonês ainda necessita de muitas verificações e testes**: as quebras de linha e as larguras dos menus não foram verificadas no ecrã, pelo que são esperados erros. Relatórios de problemas são bem-vindos.
- **Aplicação do idioma:** Escolhido em `Settings` → `General` → `Game Language`. A seleção entra em vigor ao iniciar o jogo. Se for alterada durante a partida, reinicie o jogo para ter efeito.

### Pacotes de texturas

- **BT Reloaded** (de GhostlyDark, um projeto à parte): transfira `bt-reloaded-v<versão>-rt64.rtz` nas
  [releases do GitHub](https://github.com/GhostlyDark/BT-Reloaded/releases) ou no
  [site do pacote](https://evilgames.eu/texture-packs/bt-reloaded.htm).
- Instale-o pelo menu `Mods` no jogo, ou copie o ficheiro para `%LOCALAPPDATA%\TooieRecompiled\mods`
  (Linux: `~/.config/TooieRecompiled/mods`), e ative-o em `Mods`. O ZIP `gliden64-png` do pacote também funciona sem
  ser extraído.
- Os pacotes de texturas não vêm incluídos nas releases. O BT Reloaded v0.1.1 substitui por enquanto sobretudo os menus
  e o HUD; só foi testado no Windows.

---

## Compilar a partir do Código-Fonte

Para compilar o TooieRecomp a partir do código-fonte, coloque a sua ROM verificada `Banjo-Tooie (USA).z64` na raiz do repositório.

- **Windows:** Execute `.\tools\windows\Build-Tooie.ps1 -Stage All` no PowerShell.
  Pré-requisitos: Visual Studio 2022+ (com clang-cl), Python 3.8+ e Git for Windows.
  Consulte [tools/windows/README.md](tools/windows/README.md) para obter instruções detalhadas.
- **Linux:** Execute `tools/build_linux.sh` (testado em Ubuntu 24.04 / Linux Mint 22).
  Pré-requisitos: Clang, CMake, Ninja, SDL2, FreeType, GTK 3 e Python 3.
  Consulte [tools/BUILDING.md](tools/BUILDING.md) para obter as instruções completas.
- **Portáteis (aarch64, PortMaster):** compilação cruzada a partir do Windows (`tools/handheld/build-handheld.ps1`) ou do Linux
  (`tools/handheld/build-handheld.sh`), depois `tools/handheld/package_portmaster.py`.
  Consulte [tools/BUILDING.md](tools/BUILDING.md) para o sysroot e os passos.

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
