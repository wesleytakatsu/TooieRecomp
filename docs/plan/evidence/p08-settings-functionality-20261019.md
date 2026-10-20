---
title: "P08 Evidence — Settings Functionality, Menu Policy Enforcement, Window Management & BGM Volume Control"
description: "Evidence document for user-authorized menu settings, policy enforcement (forced 30 FPS cap, disabled options, hidden legacy Note & Jinjo), resolution 3x/4x scaling, aspect ratio window resizing on the SDL event thread, bug resolution (dangling Config pointer from vector reallocation), launcher IPC preview channel, and integrated BGM volume control across sequence players."
status: evidence
phase: P08
updated: 2026-10-20
tags: [settings, menu, config, p08, ui, sdl2, rt64, resolution, aspect-ratio, forced-refresh, bgm-volume]
---

# P08 Evidence — Settings Functionality, Menu Policy Enforcement, Window Management & BGM Volume Control

## 1. Context, Task Scope & Canonical Base

- **Task Status**: `BOUNDED_TASK_COMPLETED`. Menu settings policy, disabled option protection, resolution 3x/4x, window auto-resizing, launcher socketpair IPC preview, and guest BGM sequence volume scaling are fully implemented and validated exit 0.
- **Phase P08 Gate Status**: Bounded settings functionality task is complete. Phase P08 controller gate remains pending; no global phase closure.
- **Canonical Tested Base HEAD**: `d4d1074016a819d7150923c2b19a91e69889b7a9`.
- **Capture-stage Build & Binary Verification**:
  - Command: `cmake --build build --target TooieRecompiled tooie_bgm_volume_test tooie_bgm_hooks_test tooie_menu_policy_test tooie_menu_config_test -j2`
  - Result: Exit code 0 (18.4s). All 4 unit tests exit code 0.
  - Binary SHA256: `106cb2048a68fd5284687e9bc63b0c8ba99f58f2a44494d099443fd499c26cda`.
- **Authorization & Rules**:
  - User authorized all menu functionality adjustments.
  - Commit and push explicitly authorized by the user on 2026-10-20, after the implementation and scoped runtime validation (tested base `d4d1074016a819d7150923c2b19a91e69889b7a9`).
  - Pre-existing local dirty tooling and config state preserved.
  - Parent submodules `RecompFrontend` (`d0d90ba`) and `N64ModernRuntime` (`ca568b6`) protected dirty.
  - No hand-edits to recompiled or generated files.

---

## 2. Menu Settings Policy & Adjustments (PROVEN)

User-authorized policy adjustments implemented in `src/game/config.cpp` and `include/tooie_menu_policy.h`:

1. **Note & Jinjo Saving Safely Hidden**:
   - `banjo::configkeys::general::note_saving_mode` schema preserved for config compatibility but removed from the UI using `hidden = true`.
   - Default forced to `banjo::NoteSavingMode::Off`.
   - Description updated to `"Currently unavailable in Banjo-Tooie."`.
2. **Disabled Optional Options (Non-Simple BT Integration)**:
   - Several options requiring complex guest-side hooks that have not yet been integrated into Banjo-Tooie are disabled (`config.update_option_disabled(id, true)`) with description `"Currently unavailable in Banjo-Tooie."`:
     - `analog_cam_mode` (Analog Camera)
     - `analog_camera_sensitivity` (Analog Camera Sensitivity)
     - `third_person_camera_invert_mode` (Invert Camera)
     - `first_person_invert_mode` (Invert First Person View)
     - `flying_and_swimming_invert_mode` (Invert Flying & Swimming)
     - `cutscene_aspect_ratio_mode` (Cutscene Aspect Ratio)
     - `hr_option` (HUD Placement)
   - Status classified as `UI_DISABLED / NOT_IMPLEMENTED_BT_CONSUMER`.
   - Final real drags in runtime testing keep Sensitivity 3 and Framerate 30, with Analog Camera Off; Note & Jinjo Saving safely hidden (legacy config schema valid).
3. **Forced 30 FPS Cap (Palliative Workaround Policy)**:
   - Presentation rate is forced to `Manual` with value `30` (`tooie_menu::kForcedRefreshRate = 30`).
   - UI option `rr_option` and `rr_manual_value` are disabled and marked with warning text: `"<recomp-color warning>Temporarily capped at 30 FPS in Banjo-Tooie.</recomp-color><br /><br />"`.
   - **Epistemic Classification**: The 30 FPS cap is a **user policy cap** resulting from the palliative `tfPos` CURRENT workaround (`tooie_rsp_endpoint_current`), which repeats vertex poses across intermediate presents when presenting above native rates. RT64 is technically capable of 60+ Hz presentation; this cap is NOT a technical limitation of "no 60Hz support".
4. **Resolution 3x, 4x and Downsampling Multiplication**:
   - Added `Original3x` (base 720p) and `Original4x` (base 960p) to `ultramodern::renderer::Resolution` enum and serialization.
   - Downsampling multiplies the selected base resolution:
     - Original 1x: 240p (DS 2x -> renders 480p, scales to 240p; DS 4x -> renders 960p, scales to 240p)
     - Original 2x: 480p (DS 2x -> renders 960p, scales to 480p; DS 4x -> renders 1920p, scales to 480p)
     - Original 3x: 720p (DS 2x -> renders 1440p, scales to 720p; DS 4x -> renders 2880p, scales to 720p)
     - Original 4x: 960p (DS 2x -> renders 1920p, scales to 960p; DS 4x -> renders 3840p, scales to 960p)
   - `Auto` resolution uses integer window VI scaling (not adaptive framerate); downsampling is disabled when Auto is active.
5. **Aspect Ratio Window Auto-Resize (User Confirmed Preference)**:
   - Selecting `Original` (4:3) or `Expand` (16:9) in the Aspect Ratio menu option resizes the host window in windowed mode while preserving the current window height.
   - Preserves height, computes target width, and clamps within display usable bounds (accounting for window decorations/borders via `SDL_GetWindowBordersSize`).
   - In Fullscreen or Borderless mode, the window resize request is ignored, keeping native display dimensions.

---

## 3. Bug Root Cause Analysis & Resolution (CORRECTED / PROVEN)

### 3.1 Initial App Behavioral Defects
During manual testing of the initial menu changes, two issues were observed:
1. Old existing configurations saved with `rr_manual_value: 240` loaded and remained at 240 FPS rather than reverting to the 30 FPS cap.
2. Clicking disabled numeric options (e.g. Sensitivity) altered values despite being disabled (e.g. changing from 3 to 9).

### 3.2 Refuting the Load-Order Hypothesis
A review by Claude Haiku hypothesized a "load-order risk" between configuration initialization and frontend callback hooks.
- **Investigation Result**: The load-order hypothesis was **REFUTED** as incomplete and incorrect.
- **Actual Proven Root Cause**:
  - `recompui` stores config instances in a dynamic vector: `std::vector<std::pair<std::string, recomp::config::Config>>`.
  - When `add_graphics_options` executed, taking a reference `recomp::config::Config&` to the graphics config captured the object in its current vector slot.
  - When subsequent configuration tabs were added (e.g. audio, input, mods), `std::vector::push_back` triggered memory reallocation, moving the `Config` instances in memory!
  - Consequently, the captured `Config&` reference became a **dangling pointer**, causing subsequent callbacks and load enforcement routines to target stale/corrupted memory.

### 3.3 The Resolution
1. **Dynamic Config Resolution**:
   - `tooie_menu::install_forced_refresh` was redesigned to take a lookup functor: `std::function<recomp::config::Config&()> resolve`.
   - On every load, change, or apply invocation, the callback calls `recompui::config::get_graphics_config()`, ensuring the live, valid `Config` reference is retrieved regardless of vector reallocations.
2. **Generic Frontend Disabled Option Guard & Numeric Style Fix**:
   - In `patches/frontend/recompfrontend-disabled-option-guard.patch`:
     - `ConfigPageOptionsMenu::on_set_option_value` checks `config->is_config_option_disabled`. If disabled, it calls `element->update_value()` to snap the UI widget back and returns immediately without mutating config state.
     - `Slider::do_step` in `ui_slider.cpp` exits immediately if `!is_enabled()`, preventing mouse clicks, drag events, or navigation keys from stepping disabled sliders.
     - Additionally, `ConfigOptionNumber::update_disabled` now calls `slider->set_enabled(!disabled)`. Earlier Claude Haiku visual session reported `PARTIAL` due to numeric sliders retaining active visual styling despite input blockage; root resolved this styling gap with `set_enabled(!disabled)`.
     - Claude Haiku final visual session `a7c1b458-e32c-4f51-9d80-eaf5f6b57aef` exit 0 confirmed `VISUALPASS` on the two final images (`general-disabled-final.png` and `graphics-locked-final.png`). Interaction performed strictly by root.
3. **Rigorous Regression Test (`tests/tooie_menu_config_test.cpp`)**:
   - Uses real `librecomp::config::Config` methods.
   - Creates a graphics config, registers forced refresh with the dynamic resolver, and then inserts 16 dummy configs into the registry vector to force memory reallocation.
   - Proves:
     - Config loading `{"rr_option":"Manual","rr_manual_value":240}` forces both temp and permanent values to `30.0`.
     - Frontend load callback executes after enforcement.
     - Direct `set_option_value` / `apply_option_value` attempts to write 240 are clamped back to 30.
     - Saving the config writes `{"rr_manual_value":30,"rr_option":"Manual"}` to disk.

---

## 4. SDL Event Thread Window Management Architecture (PROVEN)

Window resizing from UI callbacks requires strict thread-safety:
- **Thread Boundary**: Calling SDL window functions directly from UI callbacks (render or worker threads) causes race conditions and deadlocks with SDL event processing.
- **Cross-Thread Atomic Slot**:
  ```cpp
  inline std::atomic<double>& window_aspect_request_slot();
  inline void request_window_aspect(double target);
  inline double take_window_aspect_request();
  ```
- **Flow**:
  1. User changes Aspect Ratio in the UI. When the change context is `Permanent`, the callback calls `tooie_menu::request_window_aspect(target)`.
  2. The target aspect ratio is stored atomically with `std::memory_order_release`.
  3. On the SDL event thread inside `update_gfx` (`src/main/main.cpp`), `tooie_menu::take_window_aspect_request()` consumes the value with `std::memory_order_acq_rel`.
  4. If a valid aspect ratio is received:
     - Skips execution if in Fullscreen or Borderless mode (`SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP`).
     - Restores maximized windows (`SDL_RestoreWindow`).
     - Queries usable desktop bounds (`SDL_GetDisplayUsableBounds`) and decoration sizes (`SDL_GetWindowBordersSize`).
     - Derives new dimensions via `tooie_menu::aspect_window_size(target, cur_w, cur_h, max_w, max_h)`.
     - Calls `SDL_SetWindowSize` and clamps `SDL_SetWindowPosition` to ensure the window remains fully inside visible screen bounds.

---

## 5. Build & Host Unit Verification (PROVEN)

The complete build and all four unit tests were compiled and executed:
```bash
cmake --build build --target TooieRecompiled tooie_bgm_volume_test tooie_bgm_hooks_test tooie_menu_policy_test tooie_menu_config_test -j2
```
- **Result**: Exit code 0 (elapsed build time: 18.4s).
- **Binary SHA256**: `106cb2048a68fd5284687e9bc63b0c8ba99f58f2a44494d099443fd499c26cda`.
- **HEAD State**: `d4d1074016a819d7150923c2b19a91e69889b7a9` (unchanged; no commit/push this task).
- **Test Executions (all 4 exit code 0)**:
  - `./build/tooie_menu_policy_test`: PASS (exit code 0). Validates aspect ratios, clamping logic, atomic slot consumption, and resolution enums.
  - `./build/tooie_menu_config_test`: PASS (exit code 0). Validates vector reallocation survival, 240 -> 30 enforcement, and JSON serialization.
  - `./build/tooie_bgm_volume_test`: PASS (exit code 0). Validates 100% identity event count, 0%, 50%, repeat no compounding, restoration to 100%, idle live change, unknown 1234 pass-through, and context returns.
  - `./build/tooie_bgm_hooks_test`: PASS (exit code 0). Validates single-scale wrapper dispatch and post-update context preservation.

---

## 6. App QA, Visual & Audio Verification (PROVEN)

Runtime app QA was conducted in an isolated environment outside the repository using:
```bash
APP_FOLDER_PATH=/home/wesley/.cache/tooie-menu-settings-20261019/config ./build/TooieRecompiled
```
Original user configuration file hashes were strictly preserved.

### 6.1 Visual & Window Management Evidence
1. `general-disabled-final.png` & `graphics-locked-final.png`:
   - Note & Jinjo Saving is completely absent from the General settings menu.
   - Analog Camera is Off and disabled.
   - Sensitivity locked at 3, Framerate capped at 30 FPS.
   - Verified by Claude Haiku final visual session `a7c1b458-e32c-4f51-9d80-eaf5f6b57aef` exit 0 (VISUALPASS); interaction performed strictly by root.
2. `graphics-4x3.png` & `graphics-16x9-4x.png`:
   - Selecting `Original` (4:3) resized window from 1600x900 to 1200x900, keeping height 900.
   - Selecting `Expand` (16:9) resized window back to 1600x900, keeping height 900.
   - Original 3x (720p base) with DS 2x renders at 1440p downsampled to 720p.
   - Original 4x (960p base) with DS Off verified.
3. `fullscreen-original.png`:
   - In Fullscreen mode (1920x1080), changing Aspect Ratio to `Original` kept native display dimensions without invalid host window resizing.
4. `intro-4x-30fps.png`:
   - Intro cinematic running at Original 4x resolution under 30 FPS cap. Top-left counter showed **27 FPS** snapshot under background load (cap 30 is a ceiling, not a constant guarantee).

### 6.2 Host Launcher Preview IPC Architecture
- The host Linux launcher preview runs as a separate child process.
- Communication channel: datagram `socketpair` with nonblocking `CLOEXEC`.
- Child process dups read fd to `stdin` (fd 0) with explicit control flag and `SO_TYPE` validation (`SOCK_DGRAM`).
- Parent 5ms monitor loop sends 1 byte only when percentage changes with `MSG_NOSIGNAL`.
- Child `update_gfx` drains atomic gain.
- Zero filesystem polling or hot telemetry; no modifications to SDL audio queue or audio-tic-fix mechanisms.

### 6.3 Practical Audio Verification (Owned Null Sink & In-Game)
Audio verification was executed using owned null sink `tooie_bgm_menu_qa` and isolated configuration.
WAV and JSON evidence stored at `/home/wesley/.cache/tooie-menu-settings-20261019/`:
- **Launcher Preview Stream** (SAME child process stayed alive across live volume adjustments):
  - `preview-100.wav`: RMS `0.05225296629811854`.
  - `preview-0-real.wav`: RMS `0.0000017444902018703987`, peak 1 LSB `0.000030517578125` (after initial 0.5s fade, completely zero).
  - `preview-50-real.wav`: RMS `0.0167474789168988`.
  - `preview-100-restored.wav`: RMS `0.03735020994535605`.
  - Screenshots: `sound-preview-0-real.png`, `sound-preview-50-real.png`, `sound-preview-100-restored.png`.
- **In-Game Live Volume Restoration** (verified live in-game without restarting):
  - `game-0.wav`: 10.036s, RMS `0.0`, peak `0.0`.
  - `game-100-restored.wav`: 8.007s, RMS `0.05279053153091204`, peak `0.3997802734375`.
  - Screenshots: `game-sound-0.png`, `game-100-restored.png`.
- **Epistemic Classification & Boundaries**:
  - Music passages differ across capture intervals; do NOT claim an exact 50% PCM ratio between differing musical passages; linear attenuation math is strictly verified in unit tests.
  - Early captures renamed `preview-0-ui-not-applied.wav` and `preview-50-ui-not-applied.wav` represent `INVALID UI_NOT_APPLIED` (clicked rather than dragged; UI stayed at 100) and are excluded from comparisons.
  - Scope: No user audition yet performed this task; no full campaign or voice ID classification. Whether voice clips route through sequence players or SoundPlayer remains `UNKNOWN` (SoundPlayer path at `80020BE0` is untouched). Global audio parity is NOT claimed.
  - Teardown: Owned game processes terminated and PulseAudio null sink module `536870913` unloaded.

---

## 7. Versioned Clean-Pin Patches & Applicator

Three versioned clean-pin patches implement the external submodule modifications:
1. `patches/runtime/n64modernruntime-resolution-3x-4x.patch`:
   - SHA256: `6955ca324fa255ffbe0e61806cb500c602a5818638ddd13bcc05d076e3e18b91`.
   - Adds `Original3x` and `Original4x` to `ultramodern::renderer::Resolution` enum in `N64ModernRuntime` (pin `ca568b6`).
2. `patches/frontend/recompfrontend-resolution-3x-4x.patch`:
   - SHA256: `60eb31406dc9ac6f88368e492dcbd65f22fc00ccb0529c43fdd9c211d07a6faa`.
   - Adds 3x and 4x options to RecompFrontend graphics tab and calculates RT64 resolution/downsample multipliers (pin `d0d90ba`).
3. `patches/frontend/recompfrontend-disabled-option-guard.patch`:
   - SHA256: `9298ec60e89f4bb282b98d850e953285cf76af6fa0947d56a4f6d70c2eecc20c` (superseding old hash `867360a2...`).
   - Enforces disabled state in `ConfigPageOptionsMenu`, `Slider::do_step`, and `ConfigOptionNumber::update_disabled` calling `slider->set_enabled(!disabled)` (pin `d0d90ba`).

- **Clean-Base Replay Verification**:
  - Replay verified against temporary fixtures created from actual HEAD submodule files:
  - `git apply --check` and `git apply` produce exact match to live bytes (PASS).
  - `git apply --reverse --check` and `git apply --reverse` produce exact match to clean base (PASS).
- **Applicator Script Status**:
  - All three patches registered in `tools/apply_runtime_patches.sh`.
  - The full applicator script was NOT executed live against dirty submodules to protect uncommitted diagnostic state (do not claim old RT64 failure).
  - Protected runtime original diff hash `c54ccd3d6c2c825122b36dda0b4d1c020212f4b74e1d3b47b4089a6963e4e858` remains identical.
- **Static Code Review**:
  - Claude Sonnet final STATIC review `c4500e0c-e5c9-4b1b-bcf4-5a054a8fe733` exit 0: zero blockers identified within helper, context, and socket scope (not an independent MIPS reaudit; first review process 143 discarded, not quota).

---

## 8. BGM Volume Control Implementation & Verification (PROVEN)

Banjo-Tooie sequence player volume scaling is fully integrated and proven:

### 8.1 Banjo-Tooie Audio Contract
- **Players**: Six sequence players at `0x80079FA0` (stride `0x274`, `seqp = player + 0x1E0`).
- **Master Volume Storage**: Authored master volume (s16) is stored at `player + 0x0`.
- **Tracks**: Six music tracks at `0x801357D0` (stride `0x50`). Active track id (s16) is at `track + 0x28` (`< 0` indicates inactive/cleared slot).
- **Event Dispatch Paths**:
  - Music sequence player: `func_80025ED0` (`AL_SEQP_VOL_EVT`, event type 10).
  - Sound effect player: `func_80020BE0` (event type 3) — left completely untouched.
- **Guest Callers**:
  - `func_80017404(slot, vol)` writes unscaled volume to `player + 0x0`, then issues CSP `func_80025ED0`.
  - `func_80016ED0` (sequence start) calls CSP `func_80025ED0` directly.
  - `func_800FB968` handles per-frame high-level volume fades across active tracks.

### 8.2 Refuted Proposals & Integrated Architecture
- **Refuted Proposal**: Hooking `func_80017404` to post a second event was **REFUTED** (the original unscaled event overwrote the gain, and sequence start `func_80016ED0` bypassed `17404` entirely).
- **Integrated Solution**:
  1. **Lookup Overrides in `src/game/boot_hw_stubs.cpp`**:
     - `recomp::overlays::add_loaded_function(tooie_bgm::kCspSetVolVram, tooie_bgm_on_csp_set_vol)` (`0x80025ED0`).
     - `recomp::overlays::add_loaded_function(tooie_bgm::kUpdateVram, tooie_bgm_on_update)` (`0x800FB968`).
  2. **Single-Scale Wrapper (`tooie_bgm_on_csp_set_vol`)**:
     - Checks if incoming `r4` matches one of the 6 known sequence player pointers (`tooie_bgm::slot_for_seqp`).
     - If recognized, replaces `r5` with `apply_unscaled(authored, percent)` once, then invokes original `func_80025ED0`.
     - Unknown `seqp` pointers pass through unchanged.
  3. **Post-Update Hook (`tooie_bgm_on_update`)**:
     - Invokes original `func_800FB968`.
     - Iterates active slots (`track + 0x28 >= 0`), reads unscaled volume from `player + 0x0`.
     - Uses `take_reapply` to detect if the target scaled gain changed since the last post.
     - If changed, creates a local copy of `recomp_context` and calls the original `func_80025ED0` symbol.
     - Steady 100% volume adds zero extra events. Authored game fade curves are never mutated in RDRAM.
  4. **Frontend & IPC Integration**:
     - UI slider updates atomic integer `g_percent` (0..100, default 100) via `tooie_bgm::set_percent`.
     - Config callback does not capture any `Config` pointer.
     - Launcher preview child drains gain via datagram socketpair on `stdin`.

---

## 9. Next Single Action

`verify published HEAD against origin/main after authorized push`

## Final bounded-volume validation

- **Proteção numérica de entrada**: `set_percent(double)` limita a entrada antes da conversão para inteiro. Os testes cobrem `1e300`, `-1e300`, `NaN`, `Inf` e `49.9`; os valores normais 0, 50 e 100 permanecem inalterados.
- **Build final**: o mesmo comando com os cinco alvos acima e `-j2` terminou com exit 0 em 13.1s. Os quatro testes terminaram com exit 0.
- **Binário final**: SHA256 `e2059b516a90f09e50e77f0622218014798f6a155f6ffe0d2b6a66735427f9f9`. As capturas anteriores de preview/jogo e a revisão estática do Sonnet usaram `106cb2048a68fd5284687e9bc63b0c8ba99f58f2a44494d099443fd499c26cda`; não são atribuídas ao hash final.
- **Capturas no binário final**, no mesmo diretório de evidências externo: `final-game-100.wav`, 5.0016s, RMS `0.05596165497024496`, pico `0.349822998046875`; `final-game-0.wav`, 5.0150s, RMS `0.03568092841637808`, pico `0.3330078125`. A segunda captura ocorreu numa demo com áudio de saída não isolado: energia do mix total não mede somente a trilha musical. Capturas usam `APP_FOLDER_PATH` isolado e null-sink dedicado, sem modificar configurações pessoais e sem reiniciar o jogo para mudar o volume.
- **Integridade do pipeline**: `queue_samples` está byte a byte inalterada em relação ao HEAD; SHA256 do corpo `fab0fcf1d8ccc2b57e5cdbde253d92c3e8c983f6303da68b9a05c737cba17b00`. Código gerado e RSP inalterados; índices Graft/jCode atualizados.
- **Limites**: classificação de voice IDs e audição manual de campanha completa permanecem `UNKNOWN` / `NOT_TESTED`. A confirmação auditiva abaixo se restringe ao trecho de demo testado.
- **Confirmação auditiva do usuário (2026-10-20)**: o usuário ouviu `final-game-0.wav` com BGM 0% e confirmou "Apenas efeitos/vozes" (`USER_CONFIRMED_FOR_TESTED_DEMO_CLIP`, proveniência `USER_OBSERVED`), sendo a conclusão restrita exclusivamente a esse trecho testado, sem implicar paridade global de campanha. Registro redigido com Gemini 3.8 Flash high, sessão `212a8cd3-790d-47ec-8e97-1cccc19a29de`, exit 0; removido o caminho `/tmp` inventado pelo modelo, pois o artefato real está no diretório externo registrado acima.
- **Estado ao concluir a validação de implementação**: nenhum commit ou push havia sido realizado; a autorização posterior está registrada acima. Processos de teste encerrados, null-sink descarregado e hashes das configurações pessoais preservados.
- **Gates de documentação**: `markdown_vault_audit`: 294 arquivos, 0 erros, 61 avisos preexistentes; `git diff --check`: exit 0. `bash -n tools/apply_runtime_patches.sh`: exit 0.

## Revisão e checkpoint autorizado — 2026-10-20

- **Autorização e Base:** Usuário autorizou explicitamente `Revise, commit e push`. Base em `d4d1074016a819d7150923c2b19a91e69889b7a9` (`git fetch origin` exit 0, `HEAD...origin/main` 0/0). Tooling e configurações locais preexistentes permanecem fora do commit.
- **Validação Técnica:** Build `cmake --build build --target TooieRecompiled tooie_bgm_volume_test tooie_bgm_hooks_test tooie_menu_policy_test tooie_menu_config_test -j2` exit 0 (ninja sem trabalho); os quatro executáveis de teste rodados separadamente retornaram exit 0. Hash binário verificado: `e2059b516a90f09e50e77f0622218014798f6a155f6ffe0d2b6a66735427f9f9`, idêntico à versão testada. Os três novos patches passaram em reaplicação/reversão em fixtures limpas e igualdade byte a byte com dependências locais; `bash -n tools/apply_runtime_patches.sh` exit 0; `git diff --check` exit 0. `markdown_vault_audit`: 294 arquivos, 0 erros, 61 avisos preexistentes.
- **Revisão Auxiliar:** Sonnet 5.5 medium via Cursor CLI (sessão `236b921d-0c84-4da1-959a-556924a86550`) retornou exit 0 e `NO_BLOCKERS`. Tratou-se de análise puramente estática (declarada com `rg`/leitura, sem Graft/jCode nem execução); validação complementar à orquestração com Graft/jCode e checagens ativas. A revisão não exigiu mudanças adicionais no código.
- **Correções canônicas da revisão (CORRECTED):** a página de áudio passa a descrever `seqp` como endereço do estado embutido em `player+0x1E0`, conforme a aritmética efetiva de `seqp_addr`, e não como ponteiro armazenado nesse offset. A assinatura completa de `80016ED0` não é promovida sem contrato de argumentos; esclarecida a falha da proposta de evento duplo quando a segunda postagem era omitida; responsabilidades dos dois testes de BGM corrigidas. Apenas documentação foi ajustada.
- **Limites e Escopo:** Janela maximizada e alternância conjunta fullscreen/aspect no mesmo Apply não foram testadas (sem declaração de bug comprovado). Não há afirmação de paridade global de áudio, encerramento de P08 ou novo teste runtime nesta revisão.
- **Proveniência documental:** texto de revisão preparado por Gemini 3.8 Flash high, sessão `1e930143-3c50-47a3-a326-59c673da88f8`, exit 0. A chamada anterior de edição foi interrompida após cerca de dez minutos sem retorno e não é contabilizada como sucesso; a segunda chamada gerou texto sem ferramentas, integrado pelo orquestrador com correções mecânicas de autorização nos handoffs.
- **Status do Gate:** checkpoint preparado para commit e push autorizados; o resultado da publicação será confirmado pelas refs Git e registrado no retorno ao usuário. Nenhum commit de dependência externa ou arquivo de configuração local faz parte deste checkpoint.
- **Próxima Ação Concreta:** confirmar HEAD publicado em `origin/main` após o push autorizado.
