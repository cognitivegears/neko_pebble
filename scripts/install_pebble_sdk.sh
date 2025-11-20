#!/usr/bin/env bash
set -euo pipefail
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INSTALL_LOG="$PROJECT_ROOT/build/install_pebble_sdk.log"
mkdir -p "$(dirname "$INSTALL_LOG")"

log() {
  echo "[install-pebble-sdk] $*"
}

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    log "Missing dependency '$1'. Please install it manually and re-run."
    exit 1;
  fi
}

# Ensure sudo exists for package installs
if ! command -v sudo >/dev/null 2>&1; then
  log "sudo is required to install system packages."
  exit 1
fi

log "Installing OS dependencies (this may take a while)..."
sudo apt-get update >>"$INSTALL_LOG" 2>&1
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
  build-essential python3 python3-pip python3-venv git wget curl \
  tar bzip2 unzip libsdl1.2debian libsdl-image1.2 libsdl-ttf2.0-0 \
  libfreeimage3 libudev-dev libfreetype6 libfreetype6-dev \
  libpng16-16 libpng-dev libgl1 libxi6 libxrandr2 libxinerama1 \
  libxcursor1 libpulse0 libdbus-1-3 xvfb >>"$INSTALL_LOG" 2>&1

log "Upgrading pip and installing pebble-tool..."
python3 -m pip install --user --upgrade pip >>"$INSTALL_LOG" 2>&1
python3 -m pip install --user --upgrade pebble-tool >>"$INSTALL_LOG" 2>&1

export PATH="$HOME/.local/bin:$PATH"
require_cmd pebble

SDK_DIR="$HOME/.pebble-sdk"
if [ ! -d "$SDK_DIR" ] || [ -z "$(ls -A "$SDK_DIR" 2>/dev/null)" ]; then
  log "Installing latest Pebble SDK via pebble-tool..."
  yes | pebble sdk install latest >>"$INSTALL_LOG" 2>&1
else
  log "Pebble SDK already present at $SDK_DIR"
fi

log "Ensuring basalt emulator assets are installed..."
if ! pebble sdk list --installed 2>/dev/null | grep -qi "basalt"; then
  yes | pebble sdk install latest >>"$INSTALL_LOG" 2>&1
fi

log "Pebble SDK installed. Version: $(pebble --version)"
log "Installation complete. See $INSTALL_LOG for details."
