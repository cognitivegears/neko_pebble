#!/usr/bin/env bash
set -euo pipefail
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="${1:-basalt}"
HEADLESS_FLAG="--headless"

"$PROJECT_ROOT/scripts/install_pebble_sdk.sh"
"$PROJECT_ROOT/scripts/run_emulator.sh" "$TARGET" "$HEADLESS_FLAG"
