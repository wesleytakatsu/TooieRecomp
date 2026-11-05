#!/usr/bin/env sh
# pkg-config for the handheld cross build on a Linux host: reads .pc files from HANDHELD_SYSROOT (see pkg_config.py).
exec python3 -I "$(dirname "$0")/pkg_config.py" "$@"
