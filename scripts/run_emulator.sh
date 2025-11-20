#!/usr/bin/env bash
set -euo pipefail
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="${1:-basalt}"

if ! command -v pebble >/dev/null 2>&1; then
  echo "pebble SDK CLI not found. Install it via https://developer.rebble.io/developer.pebble.com/sdk/install/ before running the emulator." >&2
  exit 1
fi

cd "$PROJECT_ROOT"
pebble build
pebble install --emulator "$TARGET"
echo "Streaming logs from $TARGET emulator. Press Ctrl+C to stop."
pebble logs --emulator "$TARGET"
