# Banjo-Tooie Recompiled

[English](README.md) · [Português (Portugal)](README-PT.md) · [Português (Brasil)](README-BR.md) · **Español**

Adaptación nativa no oficial de Banjo-Tooie (N64) para PC (Windows x64 y Linux x64) creada mediante recompilación estática (N64Recomp).

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
- **Multijugador local (hasta 4 jugadores):** Se activa en `General` → `Multiplayer (up to 4 players)` (se aplica
  tras reiniciar el juego).
  - Entrada individual de mando y/o teclado para cada jugador.
  - Intercambio dinámico de mandos entre jugadores.
  - Mapeo de botones individual por jugador con opción `Reset to defaults`.
  - Herramientas de identificación de mandos en la página `Controls`: un indicador visual de entrada y el botón `Rumble 1 s` para probar la vibración.
- **Soporte para mandos modernos y teclado:** Compatibilidad con mandos mediante SDL2 y control configurable por teclado.
- **Lanzador nativo:** Interfaz gráfica integrada con selector de ROM.
- **Paquetes de texturas:** paquetes de texturas de RT64 (`.rtz`) desde el menú `Mods`.

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

---

## Consejos y Notas

- **Menú dentro del juego:** Abre el menú de configuración en cualquier momento pulsando la tecla `Esc`.
- **Mandos de Xbox en Windows:** Los mandos de Xbox solo aceptan la prueba de vibración tras pulsar un botón en ellos
  ("Press a button on it first").
- **Mandos idénticos:** Si conectas varios mandos idénticos sin número de serie de hardware único, se reasignan en el
  orden en que fueron conectados al reiniciar el juego.

### Idiomas del juego

TooieRecomp admite varios idiomas de texto en el juego: **English** (predeterminado), **Português (Brasil)**, **Français**, **Deutsch** y **Español**.

- **Português (Brasil):** Viene integrado en el juego. No requiere archivos adicionales ni ROM europea.
- **Français, Deutsch, Español:** Selecciona el idioma en **Settings** → **General** → **Game Language** (en el lanzador o en el menú `Esc` durante la partida). El juego te pedirá tu ROM europea (`Banjo-Tooie (Europe) (En,Fr,De,Es).z64`, SHA-1 `93bf2fac1387320ad07251cb4b64fd36bac1d7a6`) una sola vez para extraer el paquete de idioma directamente. La ROM europea se utiliza ÚNICAMENTE para esta extracción; el juego siempre se ejecuta desde tu ROM USA.
- **Aplicación del idioma:** La selección se aplica al iniciar el juego. Si se cambia durante la partida, reinicia el juego para que surta efecto.

---

## Compilación desde el Código Fuente

Para compilar TooieRecomp desde el código fuente, coloca tu ROM verificada `Banjo-Tooie (USA).z64` en la raíz del repositorio.

- **Windows:** Ejecuta `.\tools\windows\Build-Tooie.ps1 -Stage All` en PowerShell.
  Requisitos previos: Visual Studio 2022+ (con clang-cl), Python 3.8+ y Git for Windows.
  Consulta [tools/windows/README.md](tools/windows/README.md) para ver las instrucciones detalladas.
- **Linux:** Ejecuta `tools/build_linux.sh` (probado en Ubuntu 24.04 / Linux Mint 22).
  Requisitos previos: Clang, CMake, Ninja, SDL2, FreeType, GTK 3 y Python 3.
  Consulta [tools/BUILDING.md](tools/BUILDING.md) para ver las instrucciones completas.

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
