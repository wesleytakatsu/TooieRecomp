# Banjo-Tooie Recompiled

[English](README.md) · [Português (Portugal)](README-PT.md) · [Português (Brasil)](README-BR.md) · [Español](README-ES.md) · [Italiano](README-IT.md) · **日本語**

静的再コンパイル（N64Recomp）によって作成された、Windows x64 および Linux x64 向けの非公式『バンジョーとカズーイの大冒険2』（Banjo-Tooie, N64）ネイティブPC移植版です。

**HD音楽MOD:** [HDサウンドトラックMODをダウンロード](https://drive.google.com/file/d/1GRaaK0vPQz0tbBfj6oja9Q0x98VPki4d/view?usp=drive_link)（v0.1.6以降）。ゲーム内の `Mods` メニューからZIPをインストールするか、展開せずにそのまま `%LOCALAPPDATA%\TooieRecompiled\mods`（Linux: `~/.config/TooieRecompiled/mods`）にコピーし、`Mods` メニューで有効化してください。

---

## 重要: ROMが必要です

本プロジェクトには**ゲームのアセットおよびROMは一切含まれていません**。合法的に入手したご自身の北米版（USA）『バンジョーとカズーイの大冒険2』のROMが必要です:

- **ファイル名:** `Banjo-Tooie (USA).z64`（または `.n64`）
- **ファイルサイズ:** `33554432` バイト（32 MiB）
- **SHA-1 ハッシュ:** `af1a89e12b638b8d82cc4c085c8e01d4cba03fb3`

グラフィック、オーディオ、ゲームデータは実行時にROMから直接読み込まれ、展開されます。

---

## 主な機能

- **ワイドスクリーン 16:9:** アスペクト比補正を備えたネイティブのワイドスクリーン対応。
- **フレーム補間 (Frame Interpolation):** トランスフォーム補間（`Graphics` → `Interpolation`）による滑らかな 60 / 120 FPS 動作。
- **オブジェクト描画距離（実験的機能 / Object distance (experimental)）:** `Graphics` → `Object Distance` で、より遠くのオブジェクトを描画・維持します（2x、4x、Unlimited）。これらの設定にはゲームにより広いメモリ領域を割り当てる `General` → `Expanded Memory (experimental)`（拡張メモリ）が必要です。高い描画距離を選択すると自動的に有効になり、ゲームを再起動した後に反映されます。ストーリーモードの画面分割では常にオリジナルの描画距離が使用されます。
- **ローカルマルチプレイ（最大4人）:** `General` → `Multiplayer (up to 4 players)` で有効化（ゲームの再起動後に反映）。
  - プレイヤーごとのコントローラーおよび/またはキーボード入力。
  - プレイヤー間でのコントローラーの動的な交代。
  - `Reset to defaults`（デフォルトに戻す）オプション付きのプレイヤー個別ボタンマッピング。
  - `Controls` 画面のコントローラー識別ツール: 入力インジケーターおよび `Rumble 1 s`（1秒振動）テストボタン。

  **警告:** ストーリーモードの画面分割（Dev Mode → Experimental → `Two players (split screen)`）は実験的な機能であり、ゲームが正常に動作しなくなる可能性があります（クラッシュ、進行不能、セーブデータの不整合など）。現在も調整が必要な部分が多く、すでに多数の既知の不具合が存在します。少しずつ改善を進めている段階です。使用する前に必ずセーブを行ってください。また、`General` → `Expanded Memory (experimental)` を有効にすることを推奨します。
- **最新コントローラー＆キーボード対応:** カスタマイズ可能なキーボード操作に加え、SDL2によるゲームパッド対応。
- **ネイティブランチャー:** ROM選択機能を備えた組み込みのグラフィカルランチャー。
- **テクスチャパック:** ゲーム内の `Mods` メニュー経由での RT64 テクスチャパック（`.rtz`）対応。
- **OpenGL / DirectX 11 でのテクスチャパック:** これらのモードでは MOD のテクスチャにまだ修正が必要です（一部が黒く表示されます）。テクスチャパックは Direct3D 12 または Vulkan で使用してください。
- **Dev Mode（開発者モード）:** 警告画面の後に解放されるESCメニューのタブ。`Warp` はバンジョーの現在位置を保存し、別のステージからでも後でその場所にテレポートできます。ゲームの状態を直接変更するため、使用前に必ずセーブを行ってください。

---

## ダウンロードと起動方法

### Windows (x64)

1. [Releases ページ](https://github.com/wesleytakatsu/TooieRecomp/releases)から `BanjoTooieRecompiled-v<version>-Windows-x64.zip` をダウンロードします。
2. （任意）付属の `.sha256` ファイルと照合してアーカイブの整合性を確認します。
3. ZIPアーカイブを展開します（`BanjoTooieRecompiled` というフォルダーが作成されます）。
4. `TooieRecompiled.exe` を実行します。
5. ランチャーで用意したROMを選択します。

**動作要件と注意点:**
- **Direct3D 12** 対応のGPUが必要です。
- 設定およびセーブファイルは `%LOCALAPPDATA%\TooieRecompiled` に保存されます。

### Linux (x64)

1. [Releases ページ](https://github.com/wesleytakatsu/TooieRecomp/releases)から `BanjoTooieRecompiled-v<version>-Linux-x64.tar.gz` をダウンロードします。
2. アーカイブを展開し、作成されたディレクトリに移動します。
3. `./run-tooie.sh` を実行し、ランチャーで用意したROMを選択します。

リリースページには `.deb` パッケージ（Debian/Ubuntu/Mint）および `.AppImage` も用意されています。

**動作要件と注意点:**
- **Vulkan** 対応のGPUが必要です。
- システムの依存関係: `SDL2`, `FreeType`, `GTK 3`。
- 設定およびセーブファイルは `~/.config/TooieRecompiled` に保存されます。

### 携帯型ゲーム機（PortMaster, aarch64）

1. PortMaster を使って `BanjoTooieRecompiled-v<version>-Linux-arm64-PortMaster.zip` をインストールします。
2. ROMを `ports/tooie` にコピーし、Portsリストから **Banjo-Tooie** を起動します。初回起動時にROMが自動変換されます。

**動作要件と注意点:**
- OpenGL ES 3.1、および Wayland または X11 に対応したファームウェアが必要です。ROCKNIX を導入した MagicX XU10 でのみテスト済みです。他の端末は未テストです。
- オリジナル解像度および 4:3 表示です。各シーンが初めて表示される際にシェーダーがコンパイルされるため、ゲームが一瞬一時停止することがあります。
- Select + Start で終了の確認が表示されます。設定およびセーブデータは `ports/tooie` に保存されます。

---

## ヒントと注意点

- **ゲーム内メニュー:** `Esc` キーを押すことで、いつでもゲーム内メニューを開くことができます。
- **Windows 上の Xbox コントローラー:** Xbox ゲームパッドは、コントローラー上のいずれかのボタンを押した後でのみ振動テストを受け付けます（"Press a button on it first" と表示されます）。
- **同一のコントローラー:** シリアル番号のない同一のコントローラーを複数接続した場合、再起動後に接続順で再割り当てされます。

### ゲームの言語 (Game languages)

TooieRecomp はゲーム内テキストの言語として、**English**（英語、デフォルト）、**Português (Brasil)**（ブラジルポルトガル語）、**Français**（フランス語）、**Deutsch**（ドイツ語）、**Español**（スペイン語）、**Italiano**（イタリア語、下書き版）、および **日本語**（Japanese）に対応しています。

- **Português (Brasil), Français, Deutsch, Español, Italiano, 日本語:** リリースパッケージに含まれており、北米版（USA）ROMのみが必要です。
- **Italiano（下書き版）:** Rulesless v0.9 によるイタリア語翻訳と、校閲審査待ちの追加翻訳を統合したものです。クレジット: Rulesless (https://letraduzionidirulesless.wordpress.com/banjo-tooie/)。
- **日本語表示はまだ多くの検証とテストが必要です**: 改行位置やメニュー幅などの画面上での確認が行われておらず、表示崩れやエラーが予想されます。不具合報告を歓迎します。
- **言語の適用 (Applying language):** `Settings` → `General` → `Game Language` で選択します（ランチャーまたはゲーム内の `Esc` メニューからアクセス可能）。選択した言語はゲーム起動時に適用されます。プレイ中に変更した場合は、ゲームを再起動して反映させてください。

### テクスチャパック

- **BT Reloaded**（GhostlyDark 氏制作の外部プロジェクト）: [GitHub releases](https://github.com/GhostlyDark/BT-Reloaded/releases) または [配布サイト](https://evilgames.eu/texture-packs/bt-reloaded.htm) から `bt-reloaded-v<version>-rt64.rtz` をダウンロードします。
- ゲーム内の `Mods` メニューからインストールするか、ファイルを `%LOCALAPPDATA%\TooieRecompiled\mods`（Linux: `~/.config/TooieRecompiled/mods`）にコピーして `Mods` で有効化してください。同パックの `gliden64-png` ZIP も展開せずにそのまま利用可能です。
- テクスチャパックはリリースパッケージには含まれていません。BT Reloaded v0.1.1 では現在のところ主にメニューとHUDのみが差し替えられます。Windows でのみテストされています。

---

## ソースコードからのビルド

TooieRecomp をソースコードからビルドするには、検証済みの `Banjo-Tooie (USA).z64` ROM をリポジトリのルートディレクトリに配置してください。

- **Windows:** PowerShell で `.\tools\windows\Build-Tooie.ps1 -Stage All` を実行します。
  前提条件: Visual Studio 2022+（clang-cl 必須）、Python 3.8+、および Git for Windows。
  詳細な手順は [tools/windows/README.md](tools/windows/README.md) を参照してください。
- **Linux x64:** `./build-linux.sh` を実行して tar、Debian、AppImage、または全形式を選択します（Ubuntu 24.04 / Linux Mint 22 でテスト済み）。前提条件のチェックのみを行う場合は `./build-linux.sh all --check` を使用します。
  前提条件: Clang、CMake、Ninja、SDL2、FreeType、GTK 3、および Python 3。
  詳細な手順は [tools/BUILDING.md](tools/BUILDING.md) を参照してください。
- **携帯型ゲーム機（aarch64, PortMaster）:** Windows（`tools/handheld/build-handheld.ps1`）または Linux（`tools/handheld/build-handheld.sh`）からクロスビルドし、その後 `tools/handheld/package_portmaster.py` を実行します。
  sysroot および手順については [tools/BUILDING.md](tools/BUILDING.md) を参照してください。

---

## クレジット

- **主要技術:** [N64Recomp](https://github.com/N64Recomp/N64Recomp) / N64ModernRuntime、[RT64](https://github.com/rt64/rt64)、RecompFrontend ([RmlUi](https://github.com/mikke89/RmlUi))、および [SDL2](https://www.libsdl.org/) を使用して構築されています。
- **インスピレーション:** [Zelda64Recomp](https://github.com/Zelda64Recomp/Zelda64Recomp) および [BanjoRecomp](https://github.com/BanjoRecomp/BanjoRecomp) に影響を受けています。
- **逆コンパイル:** オリジナルゲームのリバースエンジニアリングを行ってくれた Banjo-Tooie 逆コンパイルコミュニティ。
- **サードパーティのライセンス:** [`vendor/`](vendor/README.md) および各リリースパッケージ内の `licenses/` と `THIRD-PARTY-MANIFEST.tsv` に記載されています。

---

## ライセンス

TooieRecomp 独自のコードは [GNU General Public License v3.0](LICENSE) の下でライセンスされています。ゲームから変換されたコード、`decomp/` 内の逆コンパイルコード、およびゲームのコンテンツ自体はこのライセンスの対象外です。サードパーティ製コンポーネントはそれぞれのライセンスを保持します（上記参照）。

---

## 法的告知

バンジョーとカズーイの大冒険2 (Banjo-Tooie) は © Nintendo / Rare です。

これは非公式のファンプロジェクトであり、任天堂または Rare との提携や承認を受けたものではありません。

本実行可能ファイルにはゲームから変換されたコードが含まれています。ご自身が所有するカートリッジから吸い出したROMを使用してください。ROMの配布を求めないでください。
