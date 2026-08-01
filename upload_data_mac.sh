#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

ENV_NAME="${1:-wemos_d1_mini32}"
UPLOAD_PORT="${2:-}"

if command -v pio >/dev/null 2>&1; then
  PIO_CMD="pio"
elif command -v platformio >/dev/null 2>&1; then
  PIO_CMD="platformio"
elif [[ -x "/opt/local/bin/pio" ]]; then
  PIO_CMD="/opt/local/bin/pio"
elif [[ -x "/opt/local/bin/platformio" ]]; then
  PIO_CMD="/opt/local/bin/platformio"
elif [[ -x "/opt/homebrew/bin/pio" ]]; then
  PIO_CMD="/opt/homebrew/bin/pio"
elif [[ -x "/usr/local/bin/pio" ]]; then
  PIO_CMD="/usr/local/bin/pio"
else
  echo "Erreur: ni 'pio' ni 'platformio' n'est disponible dans le PATH." >&2
  echo "Installez PlatformIO Core puis relancez ce script." >&2
  exit 1
fi

echo "Projet: $SCRIPT_DIR"
echo "Environnement: $ENV_NAME"

if [[ -n "$UPLOAD_PORT" ]]; then
  echo "Port: $UPLOAD_PORT"
  "$PIO_CMD" run -e "$ENV_NAME" -t uploadfs --upload-port "$UPLOAD_PORT"
else
  "$PIO_CMD" run -e "$ENV_NAME" -t uploadfs
fi

echo "Upload LittleFS termine."
