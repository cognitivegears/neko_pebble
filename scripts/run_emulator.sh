#!/usr/bin/env bash
set -euo pipefail
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PATH="$HOME/.local/bin:$PATH"
export PBL_GENERATE_MEMORY_USAGE=0
TARGET="basalt"
HEADLESS=false

for arg in "$@"; do
  case "$arg" in
    --headless)
      HEADLESS=true
      ;;
    *)
      TARGET="$arg"
      ;;
  esac
done

if ! command -v pebble >/dev/null 2>&1; then
  echo "pebble SDK CLI not found. Run scripts/install_pebble_sdk.sh first." >&2
  exit 1
fi

CMD_PREFIX=()
if $HEADLESS; then
  if ! command -v xvfb-run >/dev/null 2>&1; then
    echo "xvfb-run is required for headless mode. Install xvfb (apt install xvfb)." >&2
    exit 1
  fi
  CMD_PREFIX=(xvfb-run -a)
fi

cd "$PROJECT_ROOT"
pebble build
"${CMD_PREFIX[@]}" pebble install --emulator "$TARGET"
echo "Streaming logs from $TARGET emulator. Press Ctrl+C to stop."
pebble logs --emulator "$TARGET"
