#!/usr/bin/env bash
# Build, test and package Linux x86_64 in the selected format.
# Prerequisites (including pinned AppImage tools): tools/BUILDING.md.
set -euo pipefail
# Vendor manifests require stable byte ordering in sort/comm.
export LC_ALL=C
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

FORMAT=""
CHECK_ONLY=0
usage() { echo "Uso: $0 [tar|deb|appimage|all] [--check]"; }
for arg in "$@"; do
    case "$arg" in
        --help|-h) usage; exit 0 ;;
        --check) CHECK_ONLY=1 ;;
        tar|deb|appimage|all)
            [[ -z "$FORMAT" ]] || { usage >&2; exit 2; }
            FORMAT="$arg" ;;
        *) usage >&2; exit 2 ;;
    esac
done
if [[ -z "$FORMAT" ]]; then
    printf '\nQual formato deseja construir?\n  1) .tar.gz\n  2) .deb\n  3) .AppImage\n  4) Todos\n'
    read -r -p 'Opção [4]: ' choice || { echo 'Sem resposta. Informe tar, deb, appimage ou all como argumento.' >&2; exit 2; }
    case "${choice:-4}" in
        1) FORMAT=tar ;; 2) FORMAT=deb ;; 3) FORMAT=appimage ;; 4) FORMAT=all ;;
        *) echo 'Opção inválida.' >&2; exit 2 ;;
    esac
fi
command -v python3 >/dev/null || { echo 'Falta python3: sudo apt install python3' >&2; exit 1; }
echo "== Analisando dependências para: $FORMAT"
package_status=0
python3 tools/package_linux_installers.py "$FORMAT" --check || package_status=$?
build_status=0
tools/preflight.sh generate || build_status=$?
if (( package_status || build_status )); then
    echo 'Build bloqueado: corrija os itens MISS/BAD acima e execute novamente.' >&2
    exit 1
fi
(( CHECK_ONLY )) && { echo 'Dependências verificadas; nenhuma compilação iniciada.'; exit 0; }
# Preflight already verified the vendored sources; the packager creates the tar once.
tools/build_linux.sh extract recomp configure build test
exec python3 tools/package_linux_installers.py "$FORMAT"
