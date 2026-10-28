#!/usr/bin/env bash
# Stage + audit a public Linux x64 archive. Contains ROM-derived recompiled code; no ROM inside.
# Optional TOOIE_RELEASE_VERSION: letters, digits, dots and dashes; no leading v. Otherwise uses git describe --always --dirty.
#   TOOIE_RELEASE_VERSION=0.1.0 TOOIE_BUILD_DIR=<cmake build dir> tools/package_linux.sh -> dist/BanjoTooieRecompiled-v0.1.0-Linux-x64.tar.gz
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck disable=SC1091
source "$ROOT/tools/toolchain.lock"; source "$ROOT/tools/paths.sh"
RELEASE_VERSION="${TOOIE_RELEASE_VERSION:-}"
if [[ -n "$RELEASE_VERSION" ]] && { [[ ! "$RELEASE_VERSION" =~ ^[A-Za-z0-9.-]+$ ]] || [[ "$RELEASE_VERSION" == [vV]* ]]; }; then
    echo "package: invalid TOOIE_RELEASE_VERSION (use letters, digits, dots and dashes; no leading v)" >&2
    exit 1
fi
[[ -x "$BUILD/TooieRecompiled" ]] || { echo "package: missing $BUILD/TooieRecompiled"; exit 1; }
# ldconfig lives in /sbin (a /usr/sbin symlink on Debian/Ubuntu), which a reduced PATH may omit. Resolve it explicitly, never silently.
LDCONFIG="$(command -v ldconfig || command -v /sbin/ldconfig || command -v /usr/sbin/ldconfig || true)"
[[ -n "$LDCONFIG" ]] || { echo "package: ldconfig not found on PATH, /sbin or /usr/sbin; install glibc tools (libc-bin)"; exit 1; }
VER="$(git -C "$ROOT" describe --always --dirty 2>/dev/null || echo nogit)"
NAME="BanjoTooieRecompiled-dev-$VER-Linux-x64"
BUILD_LABEL="dev build $VER"
if [[ -n "$RELEASE_VERSION" ]]; then
    NAME="BanjoTooieRecompiled-v$RELEASE_VERSION-Linux-x64"
    BUILD_LABEL="v$RELEASE_VERSION"
fi
OUT="${TOOIE_DIST_DIR:-$ROOT/dist}"; STAGE="$OUT/$NAME/BanjoTooieRecompiled"
rm -rf "$STAGE"; mkdir -p "$STAGE/licenses"

install -m755 "$BUILD/TooieRecompiled" "$STAGE/TooieRecompiled"
cp -rL "$BUILD/assets" "$STAGE/assets"
mkdir -p "$STAGE/ui_assets"; install -m644 "$ROOT/ui_assets/TooieLogo.svg" "$STAGE/ui_assets/"
install -m644 "$ROOT/recompcontrollerdb.txt" "$STAGE/"
install -m644 "$ROOT/LICENSE" "$STAGE/LICENSE.txt"
cat > "$STAGE/run-tooie.sh" <<'LAUNCH'
#!/usr/bin/env bash
# Run from anywhere; the game resolves assets/ next to the executable. Select your own retail or decompressed ROM in the launcher.
cd "$(dirname "$(readlink -f "$0")")" && exec ./TooieRecompiled "$@"
LAUNCH
chmod 755 "$STAGE/run-tooie.sh"

# License texts of the code linked into the executable (gathered from the vendored tree; paths stay repo-relative).
( cd "$ROOT" && find vendor/licenses lib/rt64 lib/N64ModernRuntime lib/RecompFrontend lib/SlotMap lib/SDL2-windows -maxdepth 7 -type f \
    \( -iname 'LICENSE*' -o -iname 'COPYING*' -o -iname 'LICENCE*' \) ) |
  while read -r f; do mkdir -p "$STAGE/licenses/$(dirname "$f")"; cp "$ROOT/$f" "$STAGE/licenses/$f"; done

# Dependency closure: NEEDED sonames, resolved to the distro-owned library (ldconfig, x86_64-linux-gnu dirs only) and its
# Debian package. A library that ldd finds elsewhere (e.g. /usr/local/lib/libz.so.1 from a third-party app) is a HOST-SHADOW:
# it is reported, never mapped to a package of its own, and never bundled.
EXE="$STAGE/TooieRecompiled"
{ echo "# Runtime dependencies of TooieRecompiled (host libraries are NOT bundled; install the distro packages below)."
  echo "# Direct NEEDED: $(readelf -d "$EXE" | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p' | tr '\n' ' ')"
  echo "# Build host: $(. /etc/os-release; echo "$PRETTY_NAME") ; columns: soname<TAB>package<TAB>version<TAB>distro path<TAB>note"
  ldd "$EXE" | awk '/=>/ {print $1, $3}' | sort -u | while read -r so res; do
      [[ -e "$res" ]] || continue
      sys="$("$LDCONFIG" -p | awk -v s="$so" '$1==s && /x86-64/ && $NF ~ /^\/(usr\/)?lib\/x86_64-linux-gnu\// {print $NF; exit}')" || true
      [[ -n "$sys" ]] || sys="$res"
      real="$(readlink -f "$sys")"
      pkg="$(dpkg -S "$real" 2>/dev/null | head -1 | cut -d: -f1 | cut -d, -f1 || true)"
      ver="$([[ -n "$pkg" ]] && dpkg-query -W -f='${Version}\n' "$pkg" 2>/dev/null | head -1 || true)"
      note=""
      if [[ "$(readlink -f "$res")" != "$real" ]]; then
          owner="$(dpkg -S "$(readlink -f "$res")" 2>/dev/null | head -1 | cut -d: -f1 || true)"
          note="HOST-SHADOW: ldd on the build host resolved $res (owner: ${owner:-none}); not a dependency of this package"
      fi
      printf '%s\t%s\t%s\t%s\t%s\n' "$so" "${pkg:-UNOWNED}" "$ver" "$real" "$note"
  done; } > "$STAGE/DEPENDENCIES.txt"

{ echo "TooieRecompiled build info"
  echo "tooie_commit: $(git -C "$ROOT" rev-parse HEAD 2>/dev/null || echo unknown)"
  if tree_status="$(git -C "$ROOT" status --short 2>/dev/null)"; then
    if [[ -z "$tree_status" ]]; then dirty_files=0; else dirty_files="$(printf '%s\n' "$tree_status" | wc -l)"; fi
    echo "tooie_tree_dirty_files: $dirty_files"
  else
    echo "tooie_tree_dirty_files: unavailable (source export without Git metadata)"
  fi
  echo "release_version: ${RELEASE_VERSION:-none}"
  echo "ROM_SHA1: $ROM_SHA1"
  echo "vendored_upstreams (path, pin sha; full table in vendor/UPSTREAMS.tsv):";  awk -F'\t' '!/^#/ && NF && $1!="path" {print "  " $1 " " $3}' "$ROOT/vendor/UPSTREAMS.tsv"
  echo "vendor_manifest_sha256: $(sha256sum "$ROOT/vendor/MANIFEST.sha256" | cut -d' ' -f1)"
  echo "patches_applied_sha256 (already integrated in lib/; provenance only):"; (cd "$ROOT/vendor" && sha256sum patches-applied/*/*.patch patches-local/*.patch)
  echo "compiler: $(clang --version | head -1)"; echo "cmake: $(cmake --version | head -1)"
  echo "binary_sha256: $(sha256sum "$STAGE/TooieRecompiled" | cut -d' ' -f1)"; } > "$STAGE/BUILD-INFO.txt"

cat > "$STAGE/README-PACKAGE.txt" <<EOF2
Banjo-Tooie Recompiled $BUILD_LABEL (Linux x64)

A native PC port of Banjo-Tooie made with static recompilation (N64Recomp).
Contains no game assets and no ROM: graphics, audio and other game data are read from your own ROM at run time.

Requirements: your own Banjo-Tooie (USA) rev 0 cartridge dump
(SHA-1 af1a89e12b638b8d82cc4c085c8e01d4cba03fb3, .z64 or .n64).
The launcher accepts that retail cart (it decompresses internally) or the decompressed image
(35098224 bytes, SHA-1 af479771ff5e91ce3732f3d84146614ab71d8d79).
GPU: Vulkan.

How to play: run ./run-tooie.sh and select your ROM in the launcher.
Host needs: libSDL2, libfreetype, GTK 3, a Vulkan driver, zlib1g - see DEPENDENCIES.txt.
Data: user config and saves go to ~/.config/TooieRecompiled, never into this directory.

Banjo-Tooie is (c) Nintendo / Rare. This is an unofficial fan project, not affiliated with or endorsed by
Nintendo or Rare. The executable contains code translated from the game; use a ROM dumped from a cartridge you own.
TooieRecomp's own code is licensed under the GNU GPL v3.0 (LICENSE.txt); the code translated from the game is not.
Third-party components and their licences: licenses/ and THIRD-PARTY-MANIFEST.tsv.

Verify: from this directory: sha256sum -c MANIFEST.sha256
Source: https://github.com/wesleytakatsu/TooieRecomp
EOF2

# Audit
fail=0
if find "$STAGE" -type f \( -iname '*.z64' -o -iname '*.n64' -o -iname '*.v64' -o -iname '*.elf' -o -iname '*.rom' -o -iname 'baserom*' -o -iname 'decompressed*' -o -iname '*.sav' -o -iname '*.eep' -o -iname '*.sra' -o -iname '*.core' -o -iname '*.log' -o -iname '*.json' \) | grep -q .; then
    echo "AUDIT FAIL: forbidden file types in package:"; find "$STAGE" -type f \( -iname '*.z64' -o -iname '*.n64' -o -iname '*.v64' -o -iname '*.elf' -o -iname '*.rom' -o -iname 'baserom*' -o -iname 'decompressed*' -o -iname '*.sav' -o -iname '*.eep' -o -iname '*.sra' -o -iname '*.core' -o -iname '*.log' -o -iname '*.json' \); fail=1
fi
big="$(find "$STAGE" -type f -size +64M ! -name TooieRecompiled | head -3)"; [[ -z "$big" ]] || { echo "AUDIT FAIL: unexpectedly large file(s): $big"; fail=1; }
home_refs="$(strings -a "$STAGE/TooieRecompiled" | grep -c "/home/" || true)"
text_refs="$(grep -rIl "/home/" "$STAGE" 2>/dev/null | grep -v '^$' | wc -l || true)"
[[ "$home_refs" == 0 || -n "${TOOIE_ALLOW_HOME_STRINGS:-}" ]] || { fail=1; echo "AUDIT FAIL: $home_refs binary strings contain /home/ (developer paths leaked; see rt64-p09-local-pair-snapshot-portable-paths.patch)"; }
{ echo "audit: forbidden_types=$([[ $fail == 0 ]] && echo none || echo FOUND)"; echo "audit: binary strings containing /home/: $home_refs"; echo "audit: text files containing /home/: $text_refs"
  echo "audit: file_count=$(find "$STAGE" -type f | wc -l) size=$(du -sh "$STAGE" | cut -f1)"; } | tee "$STAGE/AUDIT.txt"
(( fail )) && exit 1

"$ROOT/tools/package_manifest.sh" "$STAGE"
( cd "$STAGE" && find . -type f ! -name MANIFEST.sha256 -print0 | sort -z | xargs -0 sha256sum > MANIFEST.sha256 )
tar --sort=name --owner=0 --group=0 --numeric-owner -C "$OUT/$NAME" -czf "$OUT/$NAME.tar.gz" BanjoTooieRecompiled
( cd "$OUT" && sha256sum "$NAME.tar.gz" | tee "$NAME.tar.gz.sha256" )
echo "package: $OUT/$NAME.tar.gz (contains ROM-derived recompiled code; no ROM inside)"
