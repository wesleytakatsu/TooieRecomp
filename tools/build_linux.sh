#!/usr/bin/env bash
# Reproducible Linux build from ONE checkout (all sources are vendored; no clone, submodule, sibling or download). KIT-FREE:
# the only private input is your ROM; code generation reads the decompressed ROM + the committed metadata/tooie.us.syms.toml (no ELF, no MIPS compiler).
#   [TOOIE_ROM="/path/Banjo-Tooie (USA).z64"] tools/build_linux.sh [stage ...]
# Stages (default = all, in order): preflight verify extract recomp configure build test package
#   extract = tools/extract_rom.sh (retail ROM -> <build>/extract/decompressed.us.z64, the image the launcher accepts)
# Maintainer-only stage (NOT in the default list): decomp = legacy IDO/gcc kit + ROM -> matching ELF (needed only to REGENERATE metadata/ via tools/generate_metadata.py;
#   requires TOOIE_TOOLCHAIN_KIT, see tools/toolchain.lock).
# ROM default: "<repo>/Banjo-Tooie (USA).z64" (git-ignored); TOOIE_ROM overrides. Content is always checked by tools/rom_check.sh (size+MD5+SHA1).
# Env: TOOIE_JOBS (default 2), TOOIE_BUILD_DIR (default <repo>/build-linux), TOOIE_ROM, TOOIE_TOOLCHAIN_KIT, TOOIE_DECOMP_ROOT (override)
# Serial by design: never run two builds against the same build dir at once. No path restriction for the normal build; only the maintainer-only decomp stage needs a work path without ".c" (upstream ultralib compile_sjis.py defect, checked by `tools/preflight.sh decomp`).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT/tools/toolchain.lock"; source "$ROOT/tools/paths.sh"
JOBS="${TOOIE_JOBS:-2}"
VENV="$BUILD/venv"
STAGES=("$@"); [[ ${#STAGES[@]} -gt 0 ]] || STAGES=(preflight verify extract recomp configure build test package)
TESTS=(tooie_menu_policy_test tooie_menu_config_test rt64_host_endpoint_current_test p09_interp_burst_test
       p09_world_interpolation_guard_test p09_local_pair_snapshot_test p09_exact_use_correlation_test rt64_present_decimator_test)
log() { echo "[build_linux] $(date -u +%H:%M:%SZ) $*"; }

stage_preflight() { "$ROOT/tools/preflight.sh" generate; }   # kit-free; `tools/preflight.sh decomp` adds the maintainer checks
stage_verify()    { "$ROOT/tools/vendor_verify.sh"; }
stage_decomp() { # private ROM -> decompressed ROM + ELF, in a WORK COPY of decomp/ (docs/specs/Tooie/build-and-generator.md)
    if [[ -f "$DECOMP_ROOT/build/us/banjotooie_decompressed.elf" && -f "$DECOMP_ROOT/decompressed.us.z64" ]]; then
        [[ ! -f "$TOOIE_ROM" ]] || "$ROOT/tools/rom_check.sh" "$TOOIE_ROM"       # a ROM that is present must still be the right one
        log "decomp outputs present, skipping"; return
    fi
    "$ROOT/tools/rom_check.sh" "$TOOIE_ROM" || exit 1                            # strict size+MD5+SHA1 BEFORE any output
    if [[ ! -f "$DECOMP_ROOT/Makefile" ]]; then
        mkdir -p "$DECOMP_ROOT"; cp -a "$ROOT/decomp/." "$DECOMP_ROOT/"        # vendored decomp/ is never written to
    fi
    if [[ ! -x "$DECOMP_ROOT/tools/ido/cc" || ! -x "$DECOMP_ROOT/lib/ultralib/tools/gcc/gcc" ]]; then
        [[ -n "${TOOIE_TOOLCHAIN_KIT:-}" ]] || { echo "legacy compiler kit missing: set TOOIE_TOOLCHAIN_KIT (see tools/toolchain.lock); nothing is downloaded"; exit 1; }
        "$ROOT/tools/toolchain_kit.sh" install "$TOOIE_TOOLCHAIN_KIT" "$DECOMP_ROOT"
    fi
    # Offline guard: the upstream Makefiles would wget a missing compiler. Make any such attempt fail loudly instead.
    mkdir -p "$BUILD/nonet-bin"
    printf '#!/bin/sh\necho "wget disabled: TooieRecomp builds never download (tools/toolchain.lock)" >&2\nexit 99\n' > "$BUILD/nonet-bin/wget"; chmod +x "$BUILD/nonet-bin/wget"
    [[ -d "$VENV" ]] || python3 -m venv "$VENV"
    "$VENV/bin/pip" install -q --no-cache-dir --no-index --find-links "$ROOT/tools/wheelhouse" --only-binary=:all: --require-hashes -r "$ROOT/tools/requirements.lock"
    cp "$TOOIE_ROM" "$DECOMP_ROOT/baserom.us.z64"                           # inside the (ignored) work tree
    export PYTHON3_BIN="$VENV/bin/python3" PATH="$BUILD/nonet-bin:$PATH"
    make -C "$DECOMP_ROOT" -j"$JOBS" setup
    make -C "$DECOMP_ROOT" -j"$JOBS"
    make -C "$DECOMP_ROOT" check
    [[ "$(sha1sum "$DECOMP_ROOT/decompressed.us.z64" | cut -d' ' -f1)" == "$DECOMPRESSED_ROM_SHA1" ]] || { echo "decompressed ROM sha1 mismatch"; exit 1; }
}
stage_extract()   { "$ROOT/tools/extract_rom.sh"; }
stage_recomp() { # N64Recomp (vendored, patched generator; built OUT of the source tree) + decompressed ROM + committed symbols file -> RecompiledFuncs/
    cmake -S "$LIB/N64ModernRuntime/N64Recomp" -B "$N64RECOMP_DIR" -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
    cmake --build "$N64RECOMP_DIR" -j"$JOBS"
    "$ROOT/tools/run_recomp.sh"
    python3 "$ROOT/tools/check_generated_fresh.py"
    "$ROOT/tools/verify_generated.sh"                                              # byte-identical to the accepted baseline (metadata/recomp-output.sha256)
}
stage_configure() { # -ffile-prefix-map keeps build-host paths out of __FILE__/asserts in the shipped binary
    local pm="-ffile-prefix-map=$ROOT=/tooie"
    cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_FLAGS="$pm" -DCMAKE_CXX_FLAGS="$pm"
}
stage_build()     { cmake --build "$BUILD" --target TooieRecompiled -j"$JOBS"; }
stage_test() {
    cmake --build "$BUILD" --target "${TESTS[@]}" -j"$JOBS"
    for t in "${TESTS[@]}"; do ( cd "$BUILD" && "./$t" >"$BUILD/$t.log" 2>&1 ) && echo "PASS $t" || { echo "FAIL $t (see $BUILD/$t.log)"; exit 1; }; done
}
stage_package() { TOOIE_BUILD_DIR="$BUILD" "$ROOT/tools/package_linux.sh"; }

for s in "${STAGES[@]}"; do log "== $s"; "stage_$s"; log "== $s done"; done
