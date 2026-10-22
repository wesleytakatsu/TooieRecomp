"""Standalone path resolution shared by tools/*.py (mirror of tools/paths.sh). No sibling checkouts."""
import os
from pathlib import Path

TOOIE = Path(__file__).resolve().parents[1]
BUILD = Path(os.environ.get("TOOIE_BUILD_DIR") or TOOIE / "build-linux")


def _decomp() -> Path:
    # Explicit override > the decomp work tree created by tools/build_linux.sh > the vendored (source-only) decomp/.
    if os.environ.get("TOOIE_DECOMP_ROOT"):
        return Path(os.environ["TOOIE_DECOMP_ROOT"])
    work = BUILD / "decomp-work"
    return work if work.is_dir() else TOOIE / "decomp"


DECOMP = _decomp()
DECOMPRESSED_ROM = Path(os.environ.get("TOOIE_DECOMPRESSED_ROM") or BUILD / "extract" / "decompressed.us.z64")
N64RECOMP = Path(os.environ.get("N64RECOMP") or BUILD / "n64recomp" / "N64Recomp")
