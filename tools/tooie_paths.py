"""Standalone path resolution shared by tools/*.py (mirror of tools/paths.sh). No sibling checkouts."""
import os
from pathlib import Path

TOOIE = Path(__file__).resolve().parents[1]
BUILD = Path(
    os.environ.get("TOOIE_BUILD_DIR")
    or TOOIE / ("build-win" if os.name == "nt" else "build-linux")
)


def _decomp() -> Path:
    # Explicit override > the decomp work tree created by tools/build_linux.sh > the vendored (source-only) decomp/.
    if os.environ.get("TOOIE_DECOMP_ROOT"):
        return Path(os.environ["TOOIE_DECOMP_ROOT"])
    work = BUILD / "decomp-work"
    return work if work.is_dir() else TOOIE / "decomp"


DECOMP = _decomp()
DECOMPRESSED_ROM = Path(os.environ.get("TOOIE_DECOMPRESSED_ROM") or BUILD / "extract" / "decompressed.us.z64")
# CMake on Windows writes N64Recomp.exe; the env override still replaces the whole path.
_N64RECOMP_NAME = "N64Recomp.exe" if os.name == "nt" else "N64Recomp"
N64RECOMP = Path(os.environ.get("N64RECOMP") or BUILD / "n64recomp" / _N64RECOMP_NAME)


def rom_path() -> Path:
    """Explicit TOOIE_ROM, else <repo>/<ROM_BASENAME> from tools/toolchain.lock."""
    env = os.environ.get("TOOIE_ROM")
    if env:
        return Path(env)
    from toolchain_lock import load  # local: toolchain_lock must not import tooie_paths

    name = load().get("ROM_BASENAME")
    if not name:
        raise ValueError("toolchain.lock: missing ROM_BASENAME")
    return TOOIE / name
