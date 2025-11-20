#!/usr/bin/env bash
set -euo pipefail
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build/host_tests"
mkdir -p "$BUILD_DIR"

gcc -std=c11 -Wall -Wextra -pedantic -DNEKO_LOGIC_TESTING \
  -Isrc \
  "$PROJECT_ROOT/src/neko_logic.c" \
  "$PROJECT_ROOT/tests/neko_logic_test.c" \
  -o "$BUILD_DIR/neko_logic_test"

"$BUILD_DIR/neko_logic_test"
