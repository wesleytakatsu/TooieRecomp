# Banjo-Tooie Recompiled

[English](README.md) · [Português (Portugal)](README-PT.md) · [Português (Brasil)](README-BR.md) · [Español](README-ES.md) · **Italiano** · [日本語](README-JA.md)

Porting nativo non ufficiale di Banjo-Tooie (N64) per PC (Windows x64 e Linux x64) realizzato mediante ricompilazione statica (N64Recomp).

**Mod per la musica in HD:** [scarica la mod della colonna sonora in HD](https://drive.google.com/file/d/1GRaaK0vPQz0tbBfj6oja9Q0x98VPki4d/view?usp=drive_link) (v0.1.6 o successiva). Installa il file ZIP
tramite il menu `Mods` nel gioco, oppure copialo senza estrarlo in `%LOCALAPPDATA%\TooieRecompiled\mods` (Linux:
`~/.config/TooieRecompiled/mods`), quindi attivalo in `Mods`.

---

## Importante: ROM richiesta

Questo progetto **NON contiene risorse di gioco né alcuna ROM**. È necessaria la propria ROM di Banjo-Tooie (USA) per N64 ottenuta legalmente:

- **Nome file:** `Banjo-Tooie (USA).z64` (o `.n64`)
- **Dimensioni file:** `33554432` byte (32 MiB)
- **Hash SHA-1:** `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`

Grafica, audio e dati di gioco vengono letti e decompressi direttamente dalla ROM durante l'esecuzione.

---

## Caratteristiche

- **Widescreen 16:9:** Supporto nativo per formato panoramico con aspect ratio corretto.
- **Interpolazione dei fotogrammi:** Esperienza di gioco fluida a 60 / 120 FPS tramite interpolazione delle trasformazioni
  (`Graphics` → `Interpolation`).
- **Distanza degli oggetti (sperimentale):** `Graphics` → `Object Distance` disegna e mantiene gli oggetti a distanze maggiori
  (2x, 4x, Unlimited). Queste impostazioni richiedono `General` → `Expanded Memory (experimental)`, che assegna al gioco
  un'area di memoria più ampia; la selezione di una distanza maggiore la attiva automaticamente, ed ha effetto dopo aver riavviato
  il gioco. Lo schermo condiviso nella modalità storia utilizza sempre la distanza originale.
- **Multigiocatore locale (fino a 4 giocatori):** Si attiva in `General` → `Multiplayer (up to 4 players)` (ha effetto
  dopo aver riavviato il gioco).
  - Input controller e/o tastiera indipendente per ciascun giocatore.
  - Scambio dinamico dei controller tra i giocatori.
  - Mappatura tasti indipendente per giocatore con opzione `Reset to defaults`.
  - Strumenti di identificazione dei controller nella pagina `Controls`: un indicatore visivo di input e il pulsante `Rumble 1 s` per testare la vibrazione.

  **Attenzione:** Lo schermo condiviso nella modalità storia (Dev Mode → Experimental → `Two players (split screen)`) è
  sperimentale e può causare problemi nel gioco (crash, blocchi della progressione, salvataggi compromessi). C'è ancora
  molto da rifinire e sono già noti numerosi bug; la funzione viene migliorata poco alla volta. Salva la partita prima
  di utilizzarla; si raccomanda di abilitare `General` → `Expanded Memory (experimental)`.
- **Supporto per controller moderni e tastiera:** Supporto gamepad basato su SDL2 insieme a comandi da tastiera personalizzabili.
- **Launcher nativo:** Launcher grafico integrato con selezione della ROM.
- **Pacchetti di texture:** Pacchetti di texture RT64 (`.rtz`) tramite il menu di gioco `Mods`.
- **Dev Mode:** scheda del menu ESC sbloccata dopo un avviso. `Warp` salva la posizione attuale di Banjo ed esegue il
  teletrasporto in quel punto in seguito, anche a partire da un altro livello. Modifica direttamente lo stato del gioco:
  salva la partita prima di usarlo.

---

## Download e Come Giocare

### Windows (x64)

1. Scarica `BanjoTooieRecompiled-v<version>-Windows-x64.zip` dalla
   [pagina delle Releases](https://github.com/wesleytakatsu/TooieRecomp/releases).
2. (Facoltativo) Verifica l'integrità dell'archivio confrontandola con il file `.sha256` allegato.
3. Estrai l'archivio ZIP (crea una cartella denominata `BanjoTooieRecompiled`).
4. Esegui `TooieRecompiled.exe`.
5. Seleziona la tua ROM nel launcher.

**Requisiti e note:**
- Richiede una scheda video con supporto a **Direct3D 12**.
- Impostazioni e file di salvataggio sono memorizzati in: `%LOCALAPPDATA%\TooieRecompiled`

### Linux (x64)

1. Scarica `BanjoTooieRecompiled-v<version>-Linux-x64.tar.gz` dalla
   [pagina delle Releases](https://github.com/wesleytakatsu/TooieRecomp/releases).
2. Estrai l'archivio ed entra nella directory.
3. Esegui `./run-tooie.sh` e seleziona la tua ROM nel launcher.

La release include anche un pacchetto `.deb` (Debian/Ubuntu/Mint) e un `.AppImage`.

**Requisiti e note:**
- Richiede una GPU compatibile con **Vulkan**.
- Dipendenze di sistema: `SDL2`, `FreeType`, `GTK 3`.
- Impostazioni e file di salvataggio sono memorizzati in: `~/.config/TooieRecompiled`

### Dispositivi portatili (PortMaster, aarch64)

1. Installa `BanjoTooieRecompiled-v<version>-Linux-arm64-PortMaster.zip` con PortMaster.
2. Copia la tua ROM in `ports/tooie` e avvia **Banjo-Tooie** dall'elenco dei Ports. Il primo avvio converte la ROM una sola volta.

**Requisiti e note:**
- Richiede OpenGL ES 3.1 e un firmware con Wayland o X11. Testato solo su MagicX XU10 con ROCKNIX; altri dispositivi non sono stati testati.
- Risoluzione originale e 4:3. La prima volta in cui appare ciascuna scena, i relativi shader vengono compilati e il gioco può subire una breve pausa.
- Select + Start chiede se uscire. Impostazioni e salvataggi rimangono in `ports/tooie`.

---

## Consigli e Note

- **Menu nel gioco:** Apri il menu in qualsiasi momento premendo `Esc`.
- **Controller Xbox su Windows:** I gamepad Xbox accettano il test di vibrazione solo dopo aver premuto un pulsante su di essi
  ("Press a button on it first").
- **Controller identici:** Se colleghi più controller identici privi di numero di serie hardware univoco, all'avvio successivo
  verranno riassegnati nell'ordine di connessione.

### Lingue del gioco

TooieRecomp supporta diverse lingue per i testi di gioco: **English** (predefinito), **Português (Brasil)**, **Français**, **Deutsch**, **Español**, **Italiano** (bozza) e **日本語** (giapponese).

- **Português (Brasil), Français, Deutsch, Español e Italiano:** Sono inclusi nella release; è necessaria unicamente la ROM USA.
- **Italiano (bozza):** Combina la traduzione italiana Rulesless v0.9 con una traduzione supplementare in attesa di revisione editoriale. Credito: Rulesless (https://letraduzionidirulesless.wordpress.com/banjo-tooie/).
- **日本語 (Giapponese):** NON è incluso nella release. Viene generato sul proprio computer a partire dal dump della propria cartuccia giapponese, "Banjo to Kazooie no Daibouken 2 (Japan)" (SHA-1 in big-endian dopo la normalizzazione `5a5172383037d171f121790959962703be1f373c`; sono accettati i formati di byte .z64/.n64/.v64). Passaggi: esegui il gioco una volta con la tua ROM USA (questo crea `bt.n64.us.1.0.z64` nella cartella delle impostazioni), ottieni il codice sorgente e, con Python 3.8+, esegui dalla radice del repository:
  ```
  python tools/build_japanese_bundle.py --japan-rom "<Japan ROM>" --usa-image "<settings folder>/bt.n64.us.1.0.z64" --reference ja-reference.tlp --report ja-report.json
  ```
  Questo scrive `languages/ja.tlpb` nella cartella delle impostazioni (`%LOCALAPPDATA%\TooieRecompiled` su Windows, `~/.config/TooieRecompiled` su Linux). Il gioco continua a essere eseguito dalla ROM USA.
- **La lingua giapponese necessita ancora di molte verifiche e test**: le interruzioni di riga e le larghezze dei menu non sono state verificate a schermo e sono attesi errori. Le segnalazioni sono benvenute.
- **Applicazione della lingua:** Si sceglie in `Settings` → `General` → `Game Language` (accessibile dal launcher o dal menu `Esc` in gioco). La lingua selezionata viene applicata all'avvio del gioco. Se modificata durante la partita, riavviare il gioco affinché abbia effetto.

### Pacchetti di texture

- **BT Reloaded** (di GhostlyDark, un progetto a parte): scarica `bt-reloaded-v<versione>-rt64.rtz` dalle
  [release di GitHub](https://github.com/GhostlyDark/BT-Reloaded/releases) o dal
  [sito web del pacchetto](https://evilgames.eu/texture-packs/bt-reloaded.htm).
- Installalo tramite il menu `Mods` nel gioco, o copia il file in `%LOCALAPPDATA%\TooieRecompiled\mods`
  (Linux: `~/.config/TooieRecompiled/mods`), quindi attivalo in `Mods`. Funziona anche il file ZIP `gliden64-png` del pacchetto
  senza estrarlo.
- I pacchetti di texture non sono inclusi nelle release. BT Reloaded v0.1.1 sostituisce per il momento soprattutto i menu
  e l'HUD; è stato testato solo su Windows.

---

## Compilazione dal Codice Sorgente

Per compilare TooieRecomp dal codice sorgente, inserisci la tua ROM verificata `Banjo-Tooie (USA).z64` nella directory principale del repository.

- **Windows:** Esegui `.\tools\windows\Build-Tooie.ps1 -Stage All` in PowerShell.
  Prerequisiti: Visual Studio 2022+ (con clang-cl), Python 3.8+ e Git for Windows.
  Consulta [tools/windows/README.md](tools/windows/README.md) per le istruzioni complete.
- **Linux x64:** Esegui `./build-linux.sh` per selezionare tar, Debian, AppImage o tutti i formati (testato su Ubuntu 24.04 / Linux Mint 22). Usa `./build-linux.sh all --check` solo per verificare i prerequisiti.
  Prerequisiti: Clang, CMake, Ninja, SDL2, FreeType, GTK 3 e Python 3.
  Consulta [tools/BUILDING.md](tools/BUILDING.md) per le istruzioni complete.
- **Dispositivi portatili (aarch64, PortMaster):** cross-compilazione da Windows (`tools/handheld/build-handheld.ps1`) o Linux
  (`tools/handheld/build-handheld.sh`), poi `tools/handheld/package_portmaster.py`.
  Consulta [tools/BUILDING.md](tools/BUILDING.md) per il sysroot e i passaggi.

---

## Riconoscimenti

- **Tecnologie fondamentali:** Realizzato con [N64Recomp](https://github.com/N64Recomp/N64Recomp) / N64ModernRuntime,
  [RT64](https://github.com/rt64/rt64), RecompFrontend ([RmlUi](https://github.com/mikke89/RmlUi)) e
  [SDL2](https://www.libsdl.org/).
- **Ispirazioni:** Ispirato a [Zelda64Recomp](https://github.com/Zelda64Recomp/Zelda64Recomp) e
  [BanjoRecomp](https://github.com/BanjoRecomp/BanjoRecomp).
- **Decompilazione:** La community di decompilazione di Banjo-Tooie per il reverse engineering del gioco originale.
- **Licenze di terze parti:** Elencate in [`vendor/`](vendor/README.md) e all'interno di ciascun pacchetto di rilascio sotto `licenses/`
  e `THIRD-PARTY-MANIFEST.tsv`.

---

## Licenza

Il codice sorgente di TooieRecomp è concesso in licenza sotto la [GNU General Public License v3.0](LICENSE). Il codice tradotto dal
gioco, la decompilazione in `decomp/` e i contenuti del gioco non sono coperti da questa licenza. I componenti di terze parti
mantengono le rispettive licenze (vedi sopra).

---

## Note Legali

Banjo-Tooie è © Nintendo / Rare.

Questo è un progetto non ufficiale realizzato da fan e non è affiliato né approvato da Nintendo o Rare.

L'eseguibile contiene codice tradotto dal gioco; utilizza una ROM estratta da una cartuccia di tua proprietà. Non chiedere ROM.
