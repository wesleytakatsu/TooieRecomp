# Sourced by tools/*.sh. Standalone layout: every source lives in this repository (lib/, decomp/, assets/); no sibling checkouts.
ROOT="${ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
LIB="$ROOT/lib"
BUILD="${TOOIE_BUILD_DIR:-$ROOT/build-linux}"
# Tree that holds decomp OUTPUTS (decompressed ROM, ELF). Default: the work copy that tools/build_linux.sh makes, so the vendored
# decomp/ stays pristine. TOOIE_DECOMP_ROOT is an explicit override only (a complete, already prepared decomp tree).
DECOMP_ROOT="${TOOIE_DECOMP_ROOT:-$BUILD/decomp-work}"
N64RECOMP_DIR="$BUILD/n64recomp"
# Decompressed ROM image made by tools/extract_rom.sh (the generator input and the only image the launcher accepts).
DECOMPRESSED_ROM="${TOOIE_DECOMPRESSED_ROM:-$BUILD/extract/decompressed.us.z64}"
# Input ROM: explicit TOOIE_ROM wins; otherwise the canonical root file (ROM_BASENAME from tools/toolchain.lock, must be sourced first). Content is
# always validated by tools/rom_check.sh (size + MD5 + SHA1); this only picks the path.
TOOIE_ROM="${TOOIE_ROM:-$ROOT/${ROM_BASENAME:-Banjo-Tooie (USA).z64}}"
