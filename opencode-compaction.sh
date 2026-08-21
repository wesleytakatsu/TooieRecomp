#!/usr/bin/env bash
set -euo pipefail

project_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
flag_file="$project_root/.opencode/strict-compaction.enabled"
plugin_file="$project_root/.opencode/plugins/compaction.ts"
action="${1:-toggle}"

if [[ ! -f "$plugin_file" ]]; then
  echo "Erro: plugin nao encontrado em $plugin_file" >&2
  exit 1
fi

show_status() {
  if [[ -f "$flag_file" ]]; then
    echo "Compactacao personalizada: ATIVADA"
  else
    echo "Compactacao personalizada: DESATIVADA"
  fi
}

case "$action" in
  on|enable|ativar)
    touch "$flag_file"
    show_status
    ;;
  off|disable|desativar)
    rm -f -- "$flag_file"
    show_status
    ;;
  status)
    show_status
    ;;
  toggle)
    if [[ -f "$flag_file" ]]; then
      rm -f -- "$flag_file"
    else
      touch "$flag_file"
    fi
    show_status
    ;;
  *)
    echo "Uso: $0 [toggle|on|off|status]" >&2
    exit 2
    ;;
esac
