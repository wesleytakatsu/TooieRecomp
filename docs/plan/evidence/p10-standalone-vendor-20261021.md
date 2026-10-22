---
title: "P10 Evidence — Standalone Repository: Banjo Recomp Dependencies Vendored, Offline Build Proven"
description: "All BanjoRecomp-shared sources and transitive libraries brought into the TooieRecomp checkout as regular files; offline Linux generation/build/test/package proof; Windows still pending native host."
status: active
type: evidence
project: TooieRecomp
phase: P10
date: 2026-10-21
tags: [tooie-recomp, evidence, p10, standalone, vendor, offline, licensing, packaging]
---

# P10 Evidence — Standalone Repository (vendored closure)

Supersedes the *external-bootstrap* design of [[plan/evidence/p10-windows-linux-clean-clone-readiness-20261021]] (`tools/deps.lock`,
`tools/bootstrap_deps.sh`, `tools/apply_runtime_patches.sh`, `third_party/` — all retired). That document stays as history.

## 1. Mandate and result
User mandate: bring every dependency on Banjo Recomp into this repository; no dependency on other external projects; standalone.
Classification: `STANDALONE_LINUX_OFFLINE_BUILD: PROVEN` (section 4). `WINDOWS_BUILD_PENDING_NATIVE_HOST: UNCHANGED` (section 6).
P09 glitch gate remains `FAILED`; P09/P10 remain open; interpolation defaults unchanged (OFF / `CURRENT30`, F8 disabled, burst diagnostics retained).

## 2. What is vendored (category A) — `PROVEN`
Source: the clean pinned + 17-patch closure of `/tooie-proof` (E1 closure check rc=0), **not** the older dirty sibling checkouts. Exported with
`git ls-files --cached --others --exclude-standard` per upstream repo (recursing gitlinks), symlinks skipped, deny-list in `vendor/EXCLUDES.txt`.

| Tree | Content | Files |
|---|---|---|
| `lib/N64ModernRuntime` (+`N64Recomp`, `thirdparty`) | runtime, ultramodern, generator (patched) | 80 + 28 + thirdparty |
| `lib/rt64` (+ plume, dxc, imgui, …) | renderer; patched; uses host `SDL2::SDL2` target | 310 + contrib |
| `lib/RecompFrontend` (+RmlUi, lunasvg, freetype-windows) | UI frontend (patched) | 147 + libs |
| `lib/SlotMap`, `lib/SDL2-windows` | header-only map; official SDL2-devel-2.30.3-VC (zip sha256 `89999a6f…`) | 2 / 86 |
| `assets/` (**real dir**; old `assets -> ../BanjoRecomp/assets` symlink removed), `recompcontrollerdb.txt` | UI assets, controller DB | 39 + 1 |
| `decomp/` | Banjo-Tooie decomp sources only (pin `25033be6`; no ROM, no build output, no compiler kit) | 1495 + tools/ultralib |
| `tools/wheelhouse/` + `tools/requirements.lock` | 19 hash-pinned wheels (cp312, manylinux x86_64) + `pylibyaml` sdist + `SHA256SUMS` + `SOURCES.txt` | 22 |

Totals: manifest `vendor/MANIFEST.sha256` = 8266 files, 118 exec bits (`vendor/MODES.tsv`), 14 allowlisted third-party binaries (`vendor/BINARIES.tsv`). (F5 resolution: nested `.gitrepo` metadata in `decomp/lib/ultralib/tools/asm_differ/` removed; rule added to `vendor/EXCLUDES.txt`; origin commit details kept in `vendor/provenance/decomp-ultralib-asm_differ-gitrepo.origin.txt`).
Not vendored: `mupen64plus-win32-deps` (replaced by `lib/SDL2-windows`), `spirv-cross` (macOS only), arm64/macOS DXC, tests/examples trees; 11 unneeded prebuilt
sample binaries removed (premake5 ×6, glfw3.lib ×2, 2 sample .exe, stb oversample.exe) — see `vendor/EXCLUDES.txt`.

Patch provenance: 17 historical patches integrated; chain + sha256 + base commits in `vendor/patches-applied/ORDER.tsv`; one local seam
`vendor/patches-local/rt64-use-host-sdl2-target.patch` (rt64 consumes the host `SDL2::SDL2`/`SDL2::SDL2main` targets, removing the second SDL on Windows).
`tools/vendor_verify.sh --reverse-check` reverse-applies all 18 patches (newest first) on temp copies: `18 patches reverse-applied`, `VENDOR VERIFY OK` (rc=0).
Derived-file measure (lower bound): `vendor/DERIVED-FILES.tsv` (40 Tooie files sharing a path with BanjoRecomp@`ec859632`: 30 byte-identical, 10 modified).

## 3. Explicit prerequisites (category B) and host packages (C)
ROM (`TOOIE_ROM`, SHA-1 `af1a89e1…`), legacy IDO/gcc/binutils kit (`TOOIE_TOOLCHAIN_KIT`, 34 files, `tools/toolchain.sha256`, `tools/toolchain_kit.sh verify|install`),
Python 3.12; OS packages (clang, cmake, ninja, lld, SDL2/GTK/FreeType dev, MIPS binutils, make). Nothing is downloaded by any script; a `wget` stub aborts upstream Makefile downloads.

## 4. Offline Linux proof — `PROVEN` (candidate tree, not the dev worktree)
Candidate: `tools/package_source.sh` archive extracted to `/home/wesley/tooie-standalone/tooie` (no `.c` in path; **no sibling directories exist next to it**; no cache sharing).
All stages ran through `tools/offline_sandbox.sh` (bubblewrap: `--unshare-all` ⇒ loopback only, empty `HOME`, `--tmpfs /home`; the host's `~/projetos`, `~/.cache`, other clones and the
BanjoRecomp/decomp siblings are **invisible**; only `/usr`, `/etc`, the candidate and the explicit ROM/kit mounts exist). Sanity: `curl https://github.com` → *Could not resolve host*.

| Stage | Command (inside sandbox) | Exit | Result |
|---|---|---|---|
| preflight+verify+decomp+recomp | `tools/build_linux.sh preflight verify decomp recomp` | 0 | vendor verify OK; kit 34 files verified; ROM sha1 ok; decomp built from the vendored sources (matching `decompressed.us.z64` sha1 `af479771…`); N64Recomp built out-of-tree; `generated 420 files (90M)` (417 C + `lookup.cpp` + `funcs.h` + `recomp_overlays.inl`; RSP `n_aspMain.cpp` is separate and tracked in git) |
| configure+build+test+package | `tools/build_linux.sh configure build test package` | 0 | `-j2`, 1158 ninja steps; 8/8 host tests PASS (`tooie_menu_policy`, `tooie_menu_config`, `rt64_host_endpoint_current`, `p09_interp_burst`, `p09_world_interpolation_guard`, `p09_local_pair_snapshot`, `p09_exact_use_correlation`, `rt64_present_decimator`) |

* Generated `RecompiledFuncs/` (420 files) is byte-identical (`diff -rq` rc=0) to the clean proof tree **and** to the prior private bundle `e7bb66d4…` ⇒ the vendored generator/decomp pipeline reproduces the validated output from the private ROM.
* Build graph: `build.ninja` / `compile_commands.json` (286 TUs) contain 0 references to `/home/wesley/projetos`, `BanjoRecomp`, `banjo-tooie-Recomp`, `third_party`; the only include/lib roots outside the checkout are `/usr/include`; 0 sources outside the checkout.
* Binary `TooieRecompiled` sha256 `0cfcd79206efc74b74e26836ce468c69bcadaf2c3950b40eb31971946026892b`; package `TooieRecompiled-linux-x86_64-nogit-private-local.tar.gz` sha256 `b6475879582aba0fc8b2d3c5d7b39805f08912fb272c71b17c67a959b8d1fa74` (101 regular archive files (99 at audit time), 100 in manifest; supersedes initial `64f130aa44e47d16fed1398d2c6157e7868d97e4fdc52001e4ecc706bff222af` which emitted 90 ldconfig warnings). F1 fix adds explicit `/sbin/ldconfig` lookup resolving to 0 warnings.
* `tools/verify_package.sh`: `OK`, `MANIFEST OK (100 files)`, no ROM/ELF-like files; 0 occurrences of `/home/wesley`, `/home/offline`, `/tooie-standalone` inside the package. Isolated system-libs smoke (`tools/smoke_package.sh`, empty HOME): `SMOKE PASS` (alive 20 s under default loader mapping host-shadow `/usr/local/lib/libz.so.1.3.1`, and alive 20 s under distro override `TOOIE_SMOKE_LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu` mapping `zlib1g 1:1.3.dfsg-3.1ubuntu2.2`). Smoke verifies process liveness only (alive 20s then sends SIGTERM to own PID; no gameplay, rendering, or clean normal exit claim).
* Candidate-tree gates: 0 symlinks, 0 `.git`/`.gitmodules`, 0 `.gitrepo`, 0 gitlinks, largest file 29.3 MB (DXC `libdxcompiler.so`), no CRLF in project scripts, vendored files not ignored by `.gitignore` (`git check-ignore --no-index` = 0 for lib/assets/decomp/wheelhouse/vendor), private paths ARE ignored (`decomp/baserom.us.z64`, `decomp/build`, `decomp/asm`, `decomp/tools/ido`, `RecompiledFuncs`, `build-linux`, `dist`, `*.z64`, `*.elf`).
  Known pre-existing: 10 already-tracked `docs/plan/evidence/p08-visual-validation/*.png` match the `docs/**/*.png` ignore rule (tracked at HEAD; fair-use of the screenshots is **UNKNOWN**).

Limits: `bwrap` blocks network and host paths but does not trace every `open()`; the claim rests on isolation (a read of a host sibling would fail) plus the build-graph scan. One build host (Ubuntu 24.04, Clang 18.1.3, kernel 6.8). Wheelhouse is cp312 manylinux x86_64 only.

## 5. Source archive
`tools/package_source.sh`: git-visible tree as regular files; excludes personal agent/editor state (`.claude .cursor .gemini .grok .serena .opencode .agents .codex graft GEMINI.md .ignore opencode* .mcp.json .vscode/mcp.json`), `cpu_math_check`, `dist/`, `build*/`, `RecompiledFuncs/`; refuses ROM-type files, decomp build dirs and compiler-kit paths. It **does** include the vendored `lib/ decomp/ assets/` and the allowlisted DXC/SDL2/FreeType binaries. Historical pre-documentation corrected archive `TooieRecomp-source-73bfd430-dirty.tar.gz` sha256 `4aab8d213dfe77c411db91d68441c6b34dce7b0bc8b3b3cb7528b23c2d1ee16f` (8793 entries, 0 symlinks, 0 `.gitrepo`, `assets/` real directory; supersedes initial `fdd07b6c...` and `397b33ef...`). Policy note on C2: 10 HEAD-tracked agent/MCP configuration files (`.mcp.json`, etc.) contain machine-local absolute paths (`/home/wesley/...`) and are deliberately excluded from source archives to protect user privacy and portability. This snapshot predates final documentation. The final source export is produced after the doc review into `dist/standalone-20261021/`, with its checksum in a sidecar. Public redistribution stays blocked by open licence rows (section 7).

## 6. Windows 11 x64 — `WINDOWS_BUILD_PENDING_NATIVE_HOST`
No PE of `TooieRecompiled.exe` exists in project artifacts (supporting SDK binaries such as `dxc.exe` and SDL2 DLLs intentionally vendored under `lib/` do not count as a game build). Probe on Linux (llvm-mingw cross, offline sandbox, vendored SDL2 only): CMake **configure of the WIN32 branch succeeded** (`Build files have been written`, 0 fetch/download lines; SDL2 include/libs resolve to `lib/SDL2-windows`; both `SDL2::SDL2` targets shared with rt64). Compiling TUs with mingw fails on third-party edges (`plume_d3d12.cpp`, `plume_vulkan.cpp`), so there is no all-TUs-pass claim (B1 resolved). Native clang-cl/MSVC proof is strictly required on Windows 11 x64 with Visual Studio 2022 Build Tools (clang-cl) and Windows 11 SDK:
```powershell
.\tools\windows\Build-Tooie.ps1 -Stage Check
.\tools\windows\Build-Tooie.ps1 -Stage ImportGenerated -GeneratedFrom C:\temp\RecompiledFuncs
.\tools\windows\Build-Tooie.ps1 -Stage Build
.\tools\windows\Build-Tooie.ps1 -Stage Package
```
(Note: `-Stage` accepts a single string from `[ValidateSet('Check','ImportGenerated','Build','Package')]`; do not pass a comma-separated list like `-Stage Build,Package`).

## 7. Open owner decisions (not resolved here)
* B2 Root Baseline Arbitration: initial B2 concern in `independent-review.initial.md` resolved as `CORRECTED/SUPERSEDED` by root baseline comparison (`root-baseline-arbitration.md`). Task-start full diff and Sonnet E0 full diff share exact SHA-256 `1b8f7b96f9d23a0629c43b8d4446b5a78c407d7e429833ebc08e1ab73ba7c462`; all 91 protected files in `src/ include/ tests/` are byte-identical; all 21 old patch bodies are preserved under `vendor/`. No user backup confirmation blocker.
* Project licence of TooieRecomp itself (none invented). RecompFrontend root licence, DXC prebuilt binaries, `decomp/` and `ultralib` source licences: `UNVERIFIED`/`UNKNOWN` (`vendor/UPSTREAMS.tsv`). Suplexmentary font: `UNKNOWN` (not assumed non-commercial). Screenshot fair use: unproven. SDL2 zip: no published digest (trust on first fetch, 2026-10-21).
* BanjoRecomp is GPL-3.0 (verified `COPYING` bytes): shared lineage of 40 app files is recorded; compatibility/relicensing decision is the owner's.
* Whether to commit: nothing was staged, committed or pushed. The worktree holds 120 status entries (prior dirty entries plus new vendored trees). Owner must choose commit split. C3 is now FIXED: a source export without Git metadata reports the dirty count as unavailable rather than zero.

## 8. Reproduce
`tools/vendor_verify.sh --reverse-check` · `TOOIE_ROM=… TOOIE_TOOLCHAIN_KIT=… tools/offline_sandbox.sh --ro <rom>:<dst> --ro <kit>:<dst> -- tools/build_linux.sh` · `tools/package_source.sh`.

## 9. Final delivery arbitration and checks (2026-10-21)

`PROVEN`: root correction2 package SHA-256 `b6475879582aba0fc8b2d3c5d7b39805f08912fb272c71b17c67a959b8d1fa74` supersedes correction1 `5aa71bc440ce02c0c849ee4344effac92358be9b47a1f00092b053b69840dce5`; only Git-availability metadata changed. Exactly **101 regular archive files**, **100 checksum-manifest entries** (manifest excludes itself), **99 files at the earlier AUDIT stage** (before the two manifests). These counts describe different stages, not conflicting totals.

Commands in candidate `/home/wesley/tooie-standalone/tooie`: `tools/offline_sandbox.sh -- tools/build_linux.sh package` exit 0; root `tools/verify_package.sh <archive>` exit 0; `tools/smoke_package.sh <stage> 20` and the same with `TOOIE_SMOKE_LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu` both exit 0, alive 20s then SIGTERM to own PID. Liveness only, no gameplay/render acceptance. Logs: `/home/wesley/.cache/tooie-standalone-vendor/logs/correction2-{package,verify,smoke-default,smoke-distro-zlib}.log`.

`PROVEN`: 5496 C/C++/header/shader/CMake inputs compared against the E6 source snapshot, zero differences (`root-final-compiled-input-comparison.json` in that task directory). Binary remains `0cfcd79206efc74b74e26836ce468c69bcadaf2c3950b40eb31971946026892b`; the earlier 1158-step build and 8 host-test passes remain applicable. C6 request to repeat the entire runtime build is unnecessary for helper/metadata/docs-only changes.

Haiku final prerequisite correction: Python 3.8+ is explicitly required; verifier resolves a working `python3`, `python`, or `py -3`. Linux reverse-check exit 0, 8266 manifest files, 18 patches (17 previous + 1 local SDL patch). Eight synthetic cases exercise missing/Python fallback, MSYS mode-skip, rejection of unlisted binary/symlink, and Linux executable-bit enforcement; separate CRLF allowlist case passes. These are Linux-hosted branch tests, **not native Windows proof**. On MSYS/MINGW/CYGWIN POSIX modes are explicitly `SKIPPED`; hashes/allowlist and other integrity checks remain enforced. Report: `windows-prereq-final-report.md` in the task directory.

Gemini completed two final documentation passes; root reconciled their count and C3 scope against actual tar/metadata and added the last Python prerequisite correction. `markdown_vault_audit`: 0 errors, 61 existing warnings, 302 files. Source archive is regenerated **after** these edits; delivery checksums remain outside the archive to avoid self-reference. Transfer-ready files and practical commands: `dist/standalone-20261021/LEIA-ME.md`. No ROM, staging, commit, push, generated-code edit or sibling worktree mutation. Native Windows build/launch and clean-host/GPU Linux validation remain pending; licensing decisions remain open, P09/P10 remain open.

`NEXT_SINGLE_ACTION`: native Windows build using the final source export plus private generated bundle, following the delivery guide; record exit codes and the actual game PE hash/launch result.
