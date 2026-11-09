# Banjo-Tooie Recompiled

[English](README.md) · [Português (Portugal)](README-PT.md) · **Português (Brasil)** · [Español](README-ES.md) · [Italiano](README-IT.md) · [日本語](README-JA.md)

Port nativo não oficial de Banjo-Tooie (N64) para PC (Windows x64 e Linux x64) feito com recompilação estática (N64Recomp).

**Mod de música em HD:** [baixe o mod da trilha sonora em HD](https://drive.google.com/file/d/1GRaaK0vPQz0tbBfj6oja9Q0x98VPki4d/view?usp=drive_link) (v0.1.6 ou mais nova). Instale o ZIP pelo
menu `Mods` no jogo, ou copie-o sem extrair para `%LOCALAPPDATA%\TooieRecompiled\mods` (Linux:
`~/.config/TooieRecompiled/mods`), e ative em `Mods`.

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
- **Distância dos objetos (experimental):** `Graphics` → `Object Distance` renderiza e mantém objetos mais distantes
  (2x, 4x, Unlimited). Essas opções precisam de `General` → `Expanded Memory (experimental)`, que concede ao jogo uma
  área de memória maior; escolher uma distância maior ativa a opção automaticamente, entrando em vigor após reiniciar
  o jogo. A tela dividida no modo história sempre usa a distância original.
- **Multijogador local (até 4 jogadores):** Ativado em `General` → `Multiplayer (up to 4 players)` (passa a valer
  após reiniciar o jogo).
  - Controle e/ou teclado individual por jogador.
  - Troca dinâmica de controles entre os jogadores.
  - Mapeamento de botões individual por jogador com opção `Reset to defaults`.
  - Ferramentas de identificação na página `Controls`: indicador visual de entrada e botão de teste `Rumble 1 s` (vibração).

  **Aviso:** A tela dividida no modo história (Dev Mode → Experimental → `Two players (split screen)`) é experimental
  e pode quebrar o jogo (crashes, soft-locks, perda de progresso). Ainda há muito a ajustar e já existem muitos bugs
  conhecidos; ela está sendo melhorada aos poucos. Salve o jogo antes de usá-la; ativar `General` →
  `Expanded Memory (experimental)` é recomendado.
- **Suporte a controles modernos e teclado:** Compatibilidade total com controles via SDL2 e comandos customizáveis por teclado.
- **Launcher nativo:** Interface gráfica integrada para seleção da ROM e ajustes.
- **Pacotes de texturas:** pacotes de texturas do RT64 (`.rtz`) pelo menu `Mods`.
- **Modo desenvolvedor (Dev Mode):** aba do menu ESC liberada depois de um aviso. `Warp` salva onde o Banjo está e
  teleporta de volta para lá depois, inclusive a partir de outra fase. Mexe direto no estado do jogo: salve o jogo antes
  de usar.

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
   [página de Releases](https://github.com/wesleytakatsu/TooieRecomp/releases).
2. Extraia o pacote compactado e acesse a pasta extraída.
3. Execute `./run-tooie.sh` e selecione sua ROM no launcher.

A release também tem um pacote `.deb` (Debian/Ubuntu/Mint) e um `.AppImage`.

**Requisitos e caminhos:**
- Requer uma placa de vídeo compatível com **Vulkan**.
- Bibliotecas do sistema: `SDL2`, `FreeType`, `GTK 3`.
- Configurações e saves são salvos em: `~/.config/TooieRecompiled`

### Portáteis (PortMaster, aarch64)

1. Instale `BanjoTooieRecompiled-v<version>-Linux-arm64-PortMaster.zip` com o PortMaster.
2. Copie sua ROM para `ports/tooie` e abra **Banjo-Tooie** na lista de Ports. A primeira abertura converte a ROM uma única vez.

**Requisitos e caminhos:**
- Requer OpenGL ES 3.1 e um firmware com Wayland ou X11. Testado só no MagicX XU10 com ROCKNIX; outros aparelhos não foram testados.
- Resolução original e 4:3. Na primeira vez que cada cena aparece, os shaders dela são compilados e o jogo pode parar por um instante.
- Select + Start pergunta se você quer sair. Configurações e saves ficam em `ports/tooie`.

---

## Dicas e Observações

- **Menu no jogo:** Abra o menu de configuração a qualquer momento pressionando `Esc`.
- **Controles de Xbox no Windows:** Os controles de Xbox só respondem ao teste de vibração após pressionar um botão neles
  ("Press a button on it first").
- **Controles idênticos:** Se você conectar múltiplos controles idênticos sem número de série próprio, eles serão
  reatribuídos na ordem de conexão ao reiniciar o jogo.

### Idiomas do jogo

O TooieRecomp suporta múltiplos idiomas de texto no jogo: **English** (padrão), **Português (Brasil)**, **Français**, **Deutsch**, **Español**, **Italiano** (rascunho) e **日本語** (Japonês).

- **Português (Brasil), Français, Deutsch, Español, Italiano, 日本語:** Estão incluídos no lançamento; apenas a ROM USA é necessária.
- **Italiano (rascunho):** Combina a tradução italiana Rulesless v0.9 com uma tradução suplementar aguardando revisão editorial. Créditos: Rulesless (https://letraduzionidirulesless.wordpress.com/banjo-tooie/).
- **O idioma japonês ainda precisa de muitas verificações e testes**: quebras de linha e larguras de menu não foram verificadas na tela, e erros são esperados. Relatórios de problemas são bem-vindos.
- **Aplicação do idioma:** Escolhido em `Settings` → `General` → `Game Language`. O idioma selecionado entra em vigor ao iniciar o jogo. Se for alterado durante a partida, reinicie o jogo para aplicá-lo.

### Pacotes de texturas

- **BT Reloaded** (de GhostlyDark, um projeto à parte): baixe `bt-reloaded-v<versão>-rt64.rtz` nas
  [releases do GitHub](https://github.com/GhostlyDark/BT-Reloaded/releases) ou no
  [site do pacote](https://evilgames.eu/texture-packs/bt-reloaded.htm).
- Instale pelo menu `Mods` no jogo, ou copie o arquivo para `%LOCALAPPDATA%\TooieRecompiled\mods`
  (Linux: `~/.config/TooieRecompiled/mods`), e ative em `Mods`. O ZIP `gliden64-png` do pacote também funciona sem
  extrair.
- Os pacotes de texturas não vêm nas releases. O BT Reloaded v0.1.1 troca por enquanto principalmente os menus e o HUD;
  só foi testado no Windows.

---

## Compilação a partir do Código-Fonte

Para compilar o TooieRecomp a partir do código-fonte, coloque sua ROM verificada `Banjo-Tooie (USA).z64` na raiz do repositório.

- **Windows:** Execute `.\tools\windows\Build-Tooie.ps1 -Stage All` no PowerShell.
  Pré-requisitos: Visual Studio 2022+ (com clang-cl), Python 3.8+ e Git for Windows.
  Consulte [tools/windows/README.md](tools/windows/README.md) para instruções detalhadas.
- **Linux:** Execute `tools/build_linux.sh` (testado no Ubuntu 24.04 / Linux Mint 22).
  Pré-requisitos: Clang, CMake, Ninja, SDL2, FreeType, GTK 3 e Python 3.
  Consulte [tools/BUILDING.md](tools/BUILDING.md) para instruções completas.
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
- **Descompilação:** Comunidade de descompilação de Banjo-Tooie pela engenharia reversa do jogo original.
- **Licenças de terceiros:** Listadas em [`vendor/`](vendor/README.md) e incluídas em cada pacote de lançamento sob
  `licenses/` e `THIRD-PARTY-MANIFEST.tsv`.

---

## Licença

O código próprio do TooieRecomp está licenciado sob a [GNU General Public License v3.0](LICENSE). O código traduzido do
jogo, a decompilação em `decomp/` e o conteúdo do jogo não são cobertos por esta licença. Componentes de terceiros
mantêm as suas próprias licenças (veja acima).

---

## Aviso Legal

Banjo-Tooie é © Nintendo / Rare.

Este é um projeto não oficial feito por fãs e não possui qualquer vínculo ou endosso da Nintendo ou da Rare.

O executável contém código traduzido a partir do jogo original; utilize uma ROM extraída de um cartucho próprio que você possua. Não peça ROMs.
