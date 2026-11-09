# Banjo-Tooie Recompiled

[English](README.md) · [Português (Portugal)](README-PT.md) · [Português (Brasil)](README-BR.md) · **Español** · [Italiano](README-IT.md) · [日本語](README-JA.md)

Adaptación nativa no oficial de Banjo-Tooie (N64) para PC (Windows x64 y Linux x64) creada mediante recompilación estática (N64Recomp).

**Mod de música HD:** [descarga el mod de la banda sonora HD](https://drive.google.com/file/d/1GRaaK0vPQz0tbBfj6oja9Q0x98VPki4d/view?usp=drive_link) (v0.1.6 o posterior). Instala el ZIP desde
el menú `Mods` del juego, o cópialo sin extraer a `%LOCALAPPDATA%\TooieRecompiled\mods` (Linux:
`~/.config/TooieRecompiled/mods`), y actívalo en `Mods`.

---

## Importante: ROM Necesaria

Este proyecto **NO contiene ningún recurso del juego ni ninguna ROM**. Necesitas tu propia ROM de Banjo-Tooie (USA) para N64,
obtenida legalmente:

- **Nombre del archivo:** `Banjo-Tooie (USA).z64` (o `.n64`)
- **Tamaño del archivo:** `33554432` bytes (32 MiB)
- **Hash SHA-1:** `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`

Los gráficos, el audio y los datos del juego se leen y descomprimen directamente desde la ROM en tiempo de ejecución.

---

## Características

- **Pantalla panorámica 16:9:** Soporte panorámico nativo con relación de aspecto corregida.
- **Interpolación de fotogramas:** Jugabilidad fluida a 60 / 120 FPS mediante interpolación de transformaciones
  (`Graphics` → `Interpolation`).
- **Distancia de objetos (experimental):** `Graphics` → `Object Distance` dibuja y mantiene objetos a mayor distancia
  (2x, 4x, Unlimited). Estas opciones requieren `General` → `Expanded Memory (experimental)`, que proporciona al juego
  un área de memoria mayor; elegir una distancia superior la activa automáticamente, y surte efecto tras reiniciar
  el juego. La pantalla dividida en el modo historia siempre utiliza la distancia original.
- **Multijugador local (hasta 4 jugadores):** Se activa en `General` → `Multiplayer (up to 4 players)` (se aplica
  tras reiniciar el juego).
  - Entrada individual de mando y/o teclado para cada jugador.
  - Intercambio dinámico de mandos entre jugadores.
  - Mapeo de botones individual por jugador con opción `Reset to defaults`.
  - Herramientas de identificación de mandos en la página `Controls`: un indicador visual de entrada y el botón `Rumble 1 s` para probar la vibración.

  **Advertencia:** La pantalla dividida en el modo historia (Dev Mode → Experimental → `Two players (split screen)`) es
  experimental y puede provocar fallos graves en el juego (cuelgues, bloqueos de progresión, errores en la partida). Aún
  queda mucho por ajustar y ya existen numerosos fallos conocidos; se va mejorando poco a poco. Guarda tu partida antes
  de utilizarla; se recomienda activar `General` → `Expanded Memory (experimental)`.
- **Soporte para mandos modernos y teclado:** Compatibilidad con mandos mediante SDL2 y control configurable por teclado.
- **Lanzador nativo:** Interfaz gráfica integrada con selector de ROM.
- **Paquetes de texturas:** paquetes de texturas de RT64 (`.rtz`) desde el menú `Mods`.
- **Modo desarrollador (Dev Mode):** pestaña del menú ESC que se desbloquea tras un aviso. `Warp` guarda dónde está
  Banjo y lo teletransporta de vuelta más tarde, también desde otro nivel. Cambia el estado del juego directamente:
  guarda la partida antes de usarlo.

---

## Descarga y Cómo Jugar

### Windows (x64)

1. Descarga el paquete `BanjoTooieRecompiled-v<version>-Windows-x64.zip` desde la
   [página de Releases](https://github.com/wesleytakatsu/TooieRecomp/releases).
2. (Opcional) Verifica la integridad del archivo comparándolo con el archivo de verificación `.sha256`.
3. Extrae el archivo ZIP (se creará una carpeta llamada `BanjoTooieRecompiled`).
4. Ejecuta `TooieRecompiled.exe`.
5. Selecciona tu ROM en el lanzador.

**Requisitos y rutas:**
- Requiere una tarjeta gráfica compatible con **Direct3D 12**.
- La configuración y las partidas guardadas se almacenan en: `%LOCALAPPDATA%\TooieRecompiled`

### Linux (x64)

1. Descarga el paquete `BanjoTooieRecompiled-v<version>-Linux-x64.tar.gz` desde la
   [página de Releases](https://github.com/wesleytakatsu/TooieRecomp/releases).
2. Extrae el archivo comprimido y entra en la carpeta extraída.
3. Ejecuta `./run-tooie.sh` y selecciona tu ROM en el lanzador.

La release también incluye un paquete `.deb` (Debian/Ubuntu/Mint) y un `.AppImage`.

**Requisitos y rutas:**
- Requiere una tarjeta gráfica compatible con **Vulkan**.
- Dependencias del sistema: `SDL2`, `FreeType`, `GTK 3`.
- La configuración y las partidas guardadas se almacenan en: `~/.config/TooieRecompiled`

### Portátiles (PortMaster, aarch64)

1. Instala `BanjoTooieRecompiled-v<version>-Linux-arm64-PortMaster.zip` con PortMaster.
2. Copia tu ROM a `ports/tooie` e inicia **Banjo-Tooie** desde la lista de Ports. El primer inicio convierte la ROM una sola vez.

**Requisitos y rutas:**
- Requiere OpenGL ES 3.1 y un firmware con Wayland o X11. Probado solo en la MagicX XU10 con ROCKNIX; otros dispositivos no se han probado.
- Resolución original y 4:3. La primera vez que aparece cada escena, sus shaders se compilan y el juego puede pausarse un momento.
- Select + Start pregunta si quieres salir. La configuración y las partidas guardadas quedan en `ports/tooie`.

---

## Consejos y Notas

- **Menú dentro del juego:** Abre el menú de configuración en cualquier momento pulsando la tecla `Esc`.
- **Mandos de Xbox en Windows:** Los mandos de Xbox solo aceptan la prueba de vibración tras pulsar un botón en ellos
  ("Press a button on it first").
- **Mandos idénticos:** Si conectas varios mandos idénticos sin número de serie de hardware único, se reasignan en el
  orden en que fueron conectados al reiniciar el juego.

### Idiomas del juego

TooieRecomp admite varios idiomas de texto en el juego: **English** (predeterminado), **Português (Brasil)**, **Français**, **Deutsch**, **Español**, **Italiano** (borrador) y **日本語** (japonés).

- **Português (Brasil), Français, Deutsch, Español e Italiano:** Están incluidos en la versión; solo se necesita la ROM USA.
- **Italiano (borrador):** Combina la traducción al italiano Rulesless v0.9 con una traducción suplementaria a la espera de revisión editorial. Crédito: Rulesless (https://letraduzionidirulesless.wordpress.com/banjo-tooie/).
- **日本語 (Japonés):** NO está incluido en la versión. Se genera en tu ordenador a partir del volcado de tu propio cartucho japonés, "Banjo to Kazooie no Daibouken 2 (Japan)" (SHA-1 en big-endian tras la normalización `5a5172383037d171f121790959962703be1f373c`; se aceptan órdenes de bytes .z64/.n64/.v64). Pasos: ejecuta el juego una vez con tu ROM USA (esto creará `bt.n64.us.1.0.z64` en la carpeta de configuración), obtén el código fuente y, con Python 3.8+, ejecuta desde la raíz del repositorio:
  ```
  python tools/build_japanese_bundle.py --japan-rom "<Japan ROM>" --usa-image "<settings folder>/bt.n64.us.1.0.z64" --reference ja-reference.tlp --report ja-report.json
  ```
  Esto escribirá `languages/ja.tlpb` en la carpeta de configuración (`%LOCALAPPDATA%\TooieRecompiled` en Windows, `~/.config/TooieRecompiled` en Linux). El juego se sigue ejecutando desde la ROM USA.
- **El idioma japonés todavía requiere muchas comprobaciones y pruebas**: los saltos de línea y el ancho de los menús no se han comprobado en pantalla y es de esperar que contenga errores. Los informes son bienvenidos.
- **Aplicación del idioma:** Se elige en `Settings` → `General` → `Game Language`. La selección se aplica al iniciar el juego. Si se cambia durante la partida, reinicia el juego para que surta efecto.

### Paquetes de texturas

- **BT Reloaded** (de GhostlyDark, un proyecto aparte): descarga `bt-reloaded-v<versión>-rt64.rtz` desde sus
  [releases de GitHub](https://github.com/GhostlyDark/BT-Reloaded/releases) o desde el
  [sitio del paquete](https://evilgames.eu/texture-packs/bt-reloaded.htm).
- Instálalo desde el menú `Mods` del juego, o copia el archivo a `%LOCALAPPDATA%\TooieRecompiled\mods`
  (Linux: `~/.config/TooieRecompiled/mods`), y actívalo en `Mods`. El ZIP `gliden64-png` del paquete también funciona
  sin extraerlo.
- Los paquetes de texturas no se incluyen en las releases. BT Reloaded v0.1.1 reemplaza por ahora sobre todo los menús
  y el HUD; solo se probó en Windows.

---

## Compilación desde el Código Fuente

Para compilar TooieRecomp desde el código fuente, coloca tu ROM verificada `Banjo-Tooie (USA).z64` en la raíz del repositorio.

- **Windows:** Ejecuta `.\tools\windows\Build-Tooie.ps1 -Stage All` en PowerShell.
  Requisitos previos: Visual Studio 2022+ (con clang-cl), Python 3.8+ y Git for Windows.
  Consulta [tools/windows/README.md](tools/windows/README.md) para ver las instrucciones detalladas.
- **Linux:** Execute `tools/build_linux.sh` (probado en Ubuntu 24.04 / Linux Mint 22).
  Requisitos previos: Clang, CMake, Ninja, SDL2, FreeType, GTK 3 y Python 3.
  Consulta [tools/BUILDING.md](tools/BUILDING.md) para ver las instrucciones completas.
- **Portátiles (aarch64, PortMaster):** compilación cruzada desde Windows (`tools/handheld/build-handheld.ps1`) o Linux
  (`tools/handheld/build-handheld.sh`), luego `tools/handheld/package_portmaster.py`.
  Consulta [tools/BUILDING.md](tools/BUILDING.md) para el sysroot y los pasos.

---

## Créditos

- **Tecnologías principales:** Desarrollado con [N64Recomp](https://github.com/N64Recomp/N64Recomp) / N64ModernRuntime,
  [RT64](https://github.com/rt64/rt64), RecompFrontend ([RmlUi](https://github.com/mikke89/RmlUi)) y
  [SDL2](https://www.libsdl.org/).
- **Inspiraciones:** Inspirado en el trabajo pionero de [Zelda64Recomp](https://github.com/Zelda64Recomp/Zelda64Recomp) y
  [BanjoRecomp](https://github.com/BanjoRecomp/BanjoRecomp).
- **Descompilación:** Comunidad de descompilación de Banjo-Tooie por la ingeniería inversa del juego original.
- **Licencias de terceros:** Listadas en [`vendor/`](vendor/README.md) e incluidas en cada paquete de lanzamiento bajo
  `licenses/` y `THIRD-PARTY-MANIFEST.tsv`.

---

## Licencia

El código propio de TooieRecomp está licenciado bajo la [GNU General Public License v3.0](LICENSE). El código traducido
del juego, la descompilación en `decomp/` y el contenido del juego no están cubiertos por esta licencia. Los componentes
de terceros mantienen sus propias licencias (ver arriba).

---

## Aviso Legal

Banjo-Tooie es © Nintendo / Rare.

Este es un proyecto no oficial creado por fans, no afiliado ni respaldado por Nintendo ni por Rare.

El ejecutable contiene código traducido a partir del juego original; utiliza una ROM extraída de un cartucho original de tu propiedad. No pidas ROMs.
