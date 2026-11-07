---
title: "Launcher startup, graphics backends and independent music"
description: "TK64 integration, Windows startup recovery, API selection, launcher audio and opt-in graphics logs."
status: current
area: "specs/Tooie"
updated: 2026-11-06
tags: [launcher, graphics, tk64, audio, config]
---

# Launcher startup and graphics backends

## Product integration and requirements

The vendored `lib/rt64` contains TK64 changes from `6476b4b944e2d0a7e584fd62d977d47c651e9d96`, merged over
TooieRecomp's existing renderer changes. The CMake target remains `rt64`; normal builds do not use sibling checkouts.
The integrated patch and subsequent host changes are recorded in `vendor/patches-applied/ORDER.tsv`.
SPIRV-Cross sources translate existing SPIR-V shaders; the DX11 raster ubershaders are baked during the build.
No new prebuilt binary or game asset is vendored for this integration.

| API | Requirement / scope |
| --- | --- |
| DirectX 12 | Compatible D3D12 device, driver and RT64 feature support; Windows. |
| Vulkan | Compatible Vulkan device, driver and required RT64 extensions/features. |
| DirectX 11 | Feature level 11_0 or newer; Windows. |
| OpenGL | Desktop OpenGL 4.3 or newer. This does not provide an OpenGL 3.3 path. |
| OpenGL ES | OpenGL ES 3.1 through EGL; handheld build (`TK64_MOBILE`) only. |

## Backends per target

`src/main/tooie_graphics_api.cpp` holds the policy for every target; `tooie_graphics_api_test` checks all of them on any host.

| Target | Compiled backends, in Auto order | Auto |
| --- | --- | --- |
| Windows x64 | DX12, Vulkan, DX11, OpenGL (Vulkan first under Wine) | Supervisor tries each until one passes probation; APIs that failed here go last |
| Linux x64 | Vulkan, OpenGL | Vulkan; OpenGL when the probe finds no Vulkan device but finds OpenGL |
| Handheld (`TK64_MOBILE`) | OpenGL ES | OpenGL ES |

The handheld build offers only OpenGL ES because its renderer (mobile shaders, weak profile) is validated only there.
`src/main/tooie_graphics_probe.cpp` checks what can start on the device without creating a renderer: a Vulkan 1.2
instance with a 1.2 physical device; for D3D12, a hardware DXGI adapter that accepts feature level 11_0; for D3D11, a
hardware device at 11_0; on Linux, the EGL and GL or GLES client libraries. On Windows, only `opengl32.dll` is checked,
and the worker that starts OpenGL checks the 4.3 context. The Windows supervisor probes once and passes the list to its
workers in `TOOIE_GRAPHICS_AVAILABLE`; setting that variable beforehand replaces the probe (tests use it).
The probe narrows only the menu. The Windows supervisor still gives every compiled backend its worker, so a wrong probe
result cannot leave the game without a renderer.

TK64's new backends retain their documented limitations, including unavailable MSAA and pending hardware/long-session
validation. Integrating a backend does not establish correctness on every GPU or driver.

## Windows startup and failure recovery

`src/main/tooie_startup.cpp` runs a lightweight supervisor before initializing SDL, preloading the executable or
starting the runtime. The supervisor reads the graphics preference and starts one renderer worker at a time.

An explicit preference is attempted first. Remaining candidates follow DX12, Vulkan, DX11, OpenGL, without duplicate
entries. Automatic mode uses RT64's Vulkan-first preference under Wine and DX12-first preference otherwise.
The first automatic attempt retains RT64's driver compatibility workarounds. A candidate can resolve to another API
through such a workaround; the UI and diagnostic log report the API actually created.

Each worker initializes the real renderer and frontend, including shader setup. Its window stays hidden, and launcher
music stays silent, while the renderer draws the launcher for a warm-up of at least 60 frames and one second
(`tooie_startup::launcher_frame`, called once per launcher frame). A driver that loses the device on its first
submitted frames therefore fails while nothing is shown, and the next API is tried without a visible window opening
and closing. After the warm-up the worker reports readiness and becomes the visible launcher and subsequent game
process. Unsupervised builds (Linux, handhelds) report readiness on the first frame. The supervisor remains waiting; it does not run a second game, poll per frame
or initialize a GPU. Successful startup does not require a second complete shader compilation.

Before readiness, process exit/crash or a 180-second timeout advances to the next candidate. Failed workers are
terminated/reaped. A Windows job bounds worker lifetime to the supervisor; only the readiness and stability handles are
inherited. Startup failures in a managed worker do not open the runtime's blocking renderer-error dialog. When all
candidates fail, the supervisor displays one native Windows error (`MessageBoxW`, Brazilian Portuguese when the Windows
display language is Portuguese, English otherwise). It says no graphics API worked on this computer and lists every
API tried (DirectX 12, Vulkan, DirectX 11, OpenGL) with its outcome: not found on this computer (the probe missed it),
could not start (exit code), closed unexpectedly (crash code), did not respond within 3 minutes, or the process could
not be created. It then says the video driver is probably missing, out of date or not working, asks to install the
latest driver from the graphics card maker and restart, and lists the minimum requirements. The opt-in log records the
same text as `all_failed`.

Missing ROM is not a startup failure: the launcher opens normally and offers Load ROM.

The warm-up does not prove the API either: a driver can create the device and lose it later
(`DXGI_ERROR_DEVICE_REMOVED`, after which an upload map returns null and the worker crashes). A ready worker is on
probation while the launcher is open and for 15 seconds after Start Game; the worker then signals the stability event.
A crash during probation (an exit code of `0x80000000` or above, which covers access violations and fast fail, or
`abort()`'s 3) advances to the next candidate, which opens a new launcher. Nothing can be saved during probation, so no
progress is lost. A normal exit during probation (closing the window) ends the application with that code.

After probation, a crash or device loss is a gameplay failure, not a request to boot another game: its exit code is
propagated and no other launcher opens. Changing the API does not restart the current application or game. Modern
graphics DLLs are delay-loaded so their absence does not prevent reaching the supervisor. Required packaged DXC
libraries remain required.

Every failed attempt (before readiness or during probation) is recorded in `startup.json` `failed_apis`; a later clean
exit or a passed probation removes that API from the list. **Auto** keeps its order but tries the recorded APIs last,
so a machine where one API fails shows one working launcher on the next launch instead of repeating the failure. An
explicit choice is still tried first. RT64's own automatic selection (`TOOIE_GRAPHICS_AUTOMATIC`, with its driver
workarounds) is used only by the worker for the policy's first backend, even when a failure moved that backend later.

Probation establishes that the renderer survives the launcher and the first seconds of play, not an exhaustive GPU
conformance test. A driver failure later in a session remains a runtime failure; a renderer hang (no crash) is not
detected.

## Configuration and launcher controls

The Graphics tab's `graphics_backend` preference is stored in `graphics.json` (including portable mode), using keys
`auto`, `d3d12`, `vulkan`, `d3d11`, `opengl`, `opengles` (enum values 0 to 5 on every target). **Auto** is the default. On
Windows it tries the complete compatibility chain DX12 -> Vulkan -> DX11 -> OpenGL, stopping at a usable backend; the
other targets follow the table above. Wine preference and known driver rules remain applicable.
This is a startup compatibility priority, not an FPS benchmark or a universal ranking of API performance.

`startup.json` records startup metadata and remains a migration input for the previous selector:

```json
{
  "version": 1,
  "preference": "auto",
  "last_successful_api": "vulkan",
  "failed_apis": ["d3d12"]
}
```

`failed_apis` is written only by the Windows supervisor after a worker exits; it is absent when nothing failed.
A crash during probation also removes `last_successful_api` when it names the crashed API.

Startup reads `graphics.json`'s `graphics_backend` first, then legacy `startup.json` preference, then legacy `api_option`.
Invalid values use automatic selection, including a backend this build does not have (`d3d12` in a
`graphics.json` copied to Linux or a handheld). A backend the menu does not list on this device is shown as Auto, and on
Linux and handhelds startup uses Auto for it. Fallback records the effective API separately and does not overwrite an
explicit preference. Last success is informational and never substitutes for initializing the current device.

API selection is located only in **Settings -> Graphics -> Graphics API**, available before and during gameplay.
The menu lists Auto and the compiled backends the probe accepts, with compact labels
**Auto / DX12 / Vulkan / DX11 / OpenGL / OpenGL ES**. The backend running now is always listed. Internal configuration
keys are unchanged.
It uses the existing Graphics **Apply/Cancel** behavior: pending changes do not affect startup until applied.
Apply saves the preference for the next application launch, without replacing the running renderer or restarting a
game session. The option description names the API currently in use after renderer initialization; diagnostic logs also
identify the actual API/GPU. No selector, status panel or restart button is added to the launcher artwork/menu.
The original hidden `api_option` setting remains a legacy migration input.

On Linux and handhelds there is no supervisor: startup sets `TK64_GRAPHICS_API` from the effective choice before
the renderer starts. A `TK64_GRAPHICS_API` already set to a backend this build has (a launcher or a test) wins. The
PortMaster launcher no longer sets it.

Normal game execution starts through the launcher's Start Game/game-mode action. A single-shot pre-start callback
stops launcher music and records the actual backend before requesting `recomp::start_game`. The explicit
`TOOIE_AUTO_START` / `TOOIE_LAUNCHER_MUSIC_SMOKE` test paths remain exceptions; neither is enabled normally.
The obsolete `--launcher-music-child` and control mode are rejected. No guest process exists to generate launcher music.

## Independent pause music

`src/main/tooie_launcher_audio.cpp` decodes a locally imported stereo MP3 with `dr_mp3` and streams it through its own
SDL queue. It automatically reads `<config>/launcher_audio/pause.mp3`; there is no music selection button or file
dialog in the launcher. The file is private user
content and is never embedded in the executable, Git, or public package. No imported file means a silent launcher.
Invalid MP3 input is not played. Audio device failure does not prevent launcher startup. No runtime import/selection
helper or test counter is part of the music player.

Optional loop metadata is configured locally at `<config>/launcher_audio/loop.json`:

```json
{"start_frame": 384000, "end_frame": 3148422}
```

Frames refer to decoded PCM at the MP3's sample rate; the end is exclusive. Playback begins at frame zero, then repeats
the selected region. Without loop metadata it repeats the whole recording. Invalid bounds suppress playback.
General imports require their own authored loop points; numbers above describe the private Pause Screen recording
prepared for the user's Windows test, not a universal MP3 preset.

Master and BGM volume multiply once. Volume changes use a 5 ms ramp; stopping fades over 120 ms and drains the bounded
queue before guest audio begins. Device shutdown has a two-second bound. Music stops on game start
and application exit. `TOOIE_DISABLE_LAUNCHER_MUSIC=1` disables it. Neither decoder nor launcher music runs the guest.

## Opt-in diagnostic logs and manual test

Normal startup does not create a graphics log. For a diagnostic session:

```powershell
pwsh -NoProfile -File tools/windows/Diagnose-Graphics.ps1
```

The script runs `build-win/TooieRecompiled.exe` with its asset directory as working directory. It creates a timestamped
folder under `build-win/graphics-diagnostics`, sets `TOOIE_GRAPHICS_LOG` for that session and clears automatic-game-start
test flags. Previous environment values are restored afterwards. An alternate executable/output directory can be
specified with `-Executable` / `-OutputDirectory`.

`graphics.jsonl` records startup preference, candidate attempts/failures, selected backend/GPU, readiness, game start
and exit. Each event has PID and timestamp. `selected` includes `guest_started: false`; `game_start` identifies the
backend used when Start Game was requested. `worker-<PID>.log` contains each attempted worker's stdout/stderr, including
driver/shader errors. No graphics log event is generated per frame or input sample. Direct use of
`TOOIE_GRAPHICS_LOG=<UTF-8 path>` is also supported; its parent directory must already exist.

The local test build additionally has `build-win/Diagnosticar API.cmd`. It is a convenience launcher, not a release asset.
The script creates only diagnostic files; the game retains its normal save/config behavior.

Manual acceptance: open Settings -> Graphics, inspect Graphics API, wait without starting, select/apply and reopen for each available API,
then start gameplay, pause, change map and close. Send the session directory if anything fails. Test persisted
window mode and existing Mods/Character Selection controls as regressions. Logs identify startup/API selection;
they do not prove visual fidelity or compatibility on another computer.

## Validation boundaries

- `tooie_startup_test` executes the actual Windows supervisor/config/log code with fake renderer workers. It covers
  normal failure, a driver-like process crash, readiness, retained preference, effective metadata, invalid config and
  absence of a game-start event. It also links real librecomp Config to test Graphics Apply/Cancel, persistence across
  reopening, automatic selection and unchanged window mode. A second process round verifies Auto reaches OpenGL after
  failures in DX12, Vulkan and DX11, then that the next Auto order starts with OpenGL and keeps the failed APIs behind it
  while an explicit choice stays first. Further rounds cover a worker that crashes after readiness (fallback to the next
  API, `failed_apis` and `last_successful_api` recorded) and a crash after the stability event (exit code propagated,
  no other attempt, the working API not demoted); the test build shortens probation to 300 ms. Each successful fake
  worker drives `launcher_frame` and must need at least 60 frames before readiness. Supervisor and worker append to
  the same opt-in log with one append-only write per line, so concurrent events cannot interleave. A last round runs
  a fresh supervisor process (the probe result is cached per process) where every API fails and the probe finds only
  DX11 and OpenGL: it checks the exit code and that the logged dialog text names all four APIs with their codes, marks
  exactly the two missed APIs as not found, and gives the driver advice and requirements. The test build logs the
  dialog instead of showing it. It does not initialize
  a real GPU or execute the game. It fixes the probe result with `TOOIE_GRAPHICS_AVAILABLE`.
- Real hardware, 2026-11-06 (Windows 11, GTX 1650 + AMD iGPU, D3D12 device removed right after readiness on that
  machine): with the warm-up, the DX12 worker crashed while hidden (its window never became visible) and the Vulkan
  launcher appeared about 6.6 s after launch; the next launch started Vulkan directly. Without the warm-up the DX12
  launcher was briefly visible before the fallback. Start Game and gameplay after the fallback were not exercised by
  the agent. In portable mode with an empty data folder (no ROM, no config) the same fallback reached the Vulkan
  launcher showing Load ROM. With every worker failing (the `TOOIE_GRAPHICS_LOG` crash below), the Portuguese error
  dialog appeared after about 5 s listing the four APIs.
- `tooie_graphics_api_test` checks the per-target policy on any host: compiled lists and Auto order (Windows, Wine,
  Linux, handheld), menu filtering, the running backend kept, foreign and unknown saved values becoming Auto, candidate
  order without duplicates, the `TK64_GRAPHICS_API` value for Linux and handhelds, and the descriptions.
- `tooie_launcher_audio_test <MP3>` runs the real decoder and SDL dummy audio queue, covering playback, bounded stop,
  restart, repeated short loops and invalid input. Observation is injected from `tests/launcher_audio_test_sink.h`
  into the test target only; production code has no counter or instrumentation branch. It is an argument-driven
  host test, separate from the required suite.
- Windows build and required host tests are automated. Full startup, backend visuals, loop listening and gameplay
  after this integration are `PENDING_USER`. Intel/old GPU compatibility and Linux runtime are not established here.
- Windows has supervised recovery. Linux retains its direct renderer initialization; OpenGL can be selected before
  initialization, and Auto uses OpenGL when the probe finds no Vulkan device. Process supervision/fallback on Linux is
  `NOT_IMPLEMENTED`. The user tested the Linux x64 build on 2026-11-06 and reported that it works (a general report;
  the menu contents and the probe result on Linux were not reported separately).
- Probe on real hardware: Windows (this machine) reported DX12, Vulkan, DX11 and OpenGL; XU10 on Panfrost started
  OpenGL ES with no launcher variable and with `d3d12` saved. The Windows menu on screen is `PENDING_USER`.

The handheld build adds its own startup rules behind `TK64_MOBILE` (OpenGL ES only, ROM from the folder, Select + Start): [handheld build](handheld-portmaster.md).
None of them exist in a desktop build.

Related: [window/UI/config](window-ui-and-config.md), [graphics](graphics-and-presentation.md),
[audio](audio-system.md), [tests](testing-and-oracle.md), [dependencies](packaging-and-dependencies.md).

## Diagnostic boundary after review

The TK64 per-frame timing/statistics logger (`TK64_FRAME_STATS`) and its present-loop call were removed from the
vendored product. The frontend's option-hover print was also removed. The game audio path no longer links PCM capture
or scans samples for diagnostic peak/nonzero counts. Functional FPS display, audio scheduling and shutdown timeouts
remain part of the game. The requested graphics startup log remains opt-in, with no per-frame events.
