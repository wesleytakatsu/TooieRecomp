#!/usr/bin/env bash
# Run a command with NO network and NO view of the host filesystem except system dirs and this checkout (bubblewrap).
# This is how the "standalone / offline" claim is proven: sibling checkouts, ~/.cache, other clones and $HOME do not exist inside.
#   tools/offline_sandbox.sh [--ro SRC:DST]... [--rw SRC:DST]... -- CMD [ARG...]
# TOOIE_* variables are passed through (paths of explicit inputs must be mapped with --ro, e.g. TOOIE_ROM and TOOIE_TOOLCHAIN_KIT).
# Needs: bwrap (apt: bubblewrap) and unprivileged user namespaces. Linux only.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
a=(--unshare-all --die-with-parent --new-session --clearenv
   --ro-bind /usr /usr --symlink usr/bin /bin --symlink usr/sbin /sbin --symlink usr/lib /lib --symlink usr/lib64 /lib64
   --ro-bind /etc /etc --proc /proc --dev /dev --tmpfs /tmp --tmpfs /home --dir /home/offline-empty-home
   --setenv HOME /home/offline-empty-home --setenv PATH /usr/local/bin:/usr/bin:/bin --setenv LANG C.UTF-8
   --bind "$ROOT" "$ROOT")
[[ -d /var/lib/dpkg ]] && a+=(--ro-bind /var/lib/dpkg /var/lib/dpkg)
while [[ $# -gt 0 && "$1" != -- ]]; do
    case "$1" in
        --ro) a+=(--ro-bind "${2%%:*}" "${2#*:}"); shift 2;;
        --rw) a+=(--bind "${2%%:*}" "${2#*:}"); shift 2;;
        *) echo "offline_sandbox: unknown option $1" >&2; exit 2;;
    esac
done
[[ "${1:-}" == -- ]] || { echo "usage: $0 [--ro SRC:DST]... -- CMD..." >&2; exit 2; }; shift
while IFS='=' read -r k v; do [[ "$k" == TOOIE_* ]] && a+=(--setenv "$k" "$v"); done < <(env)
a+=(--chdir "$ROOT")
exec bwrap "${a[@]}" -- "$@"
