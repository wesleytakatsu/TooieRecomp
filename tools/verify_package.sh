#!/usr/bin/env bash
# Verify a TooieRecompiled package: archive checksum (if <archive>.sha256 sits next to it) and the inside-package MANIFEST.sha256.
#   tools/verify_package.sh <archive.tar.gz | extracted package dir>
# MANIFEST.sha256 inside the package is package-root-relative on purpose (so `sha256sum -c` works from the package root).
set -euo pipefail
IN="${1:?archive.tar.gz or package dir}"
if [[ -f "$IN" ]]; then
    [[ ! -f "$IN.sha256" ]] || { ( cd "$(dirname "$IN")" && sha256sum -c "$(basename "$IN").sha256" ); }
    T="$(mktemp -d /tmp/tooie-verify.XXXXXX)"; trap 'rm -rf "$T"' EXIT
    tar -C "$T" -xzf "$IN"; DIR="$(echo "$T"/*/)"
else DIR="$IN"; fi
cd "$DIR"
sha256sum -c MANIFEST.sha256 --quiet && echo "MANIFEST OK ($(wc -l < MANIFEST.sha256) files)"
if find . -type f \( -iname '*.z64' -o -iname '*.n64' -o -iname '*.v64' -o -iname '*.elf' -o -iname 'baserom*' -o -iname 'decompressed*' \) | grep -q .; then
    echo "VERIFY FAIL: ROM/ELF-like file inside package"; exit 1; fi
echo "no ROM/ELF-like files"
