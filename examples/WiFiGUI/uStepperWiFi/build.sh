#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

SKETCH="uStepperWiFi.ino"
DATA_DIR="data"
BUILD_DIR="build"
FQBN="esp32:esp32:esp32c3"
FLASH_SIZE="4MB"
FLASH_MODE="dio"
FLASH_FREQ="80m"
FS_OFFSET="0x290000"

COMBINED_BIN="$BUILD_DIR/uStepperWiFi_combined.bin"

ARDUINO_LIBS=( "ModbusMaster" "ArduinoJson" "WebSockets" )
VENV_DIR="$SCRIPT_DIR/flashtool/.venv"

# --- Dependency check & install ---

PYFLAGS=""

ensure_venv() {
  local pip="$VENV_DIR/bin/pip"
  [ -f "$pip" ] && return 0

  if ! python3 -m venv --help &>/dev/null; then
    echo "  -> Installing python3-venv (needed for isolated builds)..."
    sudo apt-get install -y python3-venv 2>/dev/null || true
  fi

  echo "  -> Creating Python virtual environment..."
  rm -rf "$VENV_DIR"
  python3 -m venv "$VENV_DIR" && [ -f "$pip" ] && return 0

  echo "  -> venv unavailable. Using --break-system-packages instead."
  PYFLAGS="--break-system-packages"
}

install_python_pkg() {
  local pip="$VENV_DIR/bin/pip"
  if [ -f "$pip" ]; then
    "$pip" install --quiet "$1" 2>/dev/null && return 0
    echo "  -> Installing Python package: $1"
    "$pip" install "$1"
  else
    python3 -m pip install --quiet $PYFLAGS "$1" 2>/dev/null && return 0
    echo "  -> Installing Python package: $1"
    python3 -m pip install $PYFLAGS "$1"
  fi
}

install_arduino_cli() {
  if command -v arduino-cli &>/dev/null; then
    return 0
  fi
  echo "arduino-cli not found. Downloading..."
  OS=$(uname -s | tr '[:upper:]' '[:lower:]')
  ARCH=$(uname -m)
  [ "$ARCH" = "x86_64" ] && ARCH="64bit" || ARCH="ARM64"
  URL="https://github.com/arduino/arduino-cli/releases/latest/download/arduino-cli_${OS}_${ARCH}.tar.gz"
  curl -fsSL "$URL" | tar xz -C /tmp
  chmod +x /tmp/arduino-cli
  echo "  -> Installing to /usr/local/bin/ (may ask for sudo)..."
  sudo mv /tmp/arduino-cli /usr/local/bin/
}

install_esp32_core() {
  echo "  -> Installing ESP32 board core..."
  arduino-cli core update-index
  arduino-cli core install esp32:esp32
}

install_arduino_libs() {
  for lib in "${ARDUINO_LIBS[@]}"; do
    if arduino-cli lib list 2>/dev/null | grep -qi "$lib"; then
      echo "  [ok] $lib"
    else
      echo "  -> Installing Arduino library: $lib"
      arduino-cli lib install "$lib"
    fi
  done
}

install_deps() {
  echo "=== Checking dependencies ==="

  echo "[Python]"
  command -v python3 &>/dev/null || { echo "ERROR: python3 not found"; exit 1; }
  ensure_venv
  install_python_pkg esptool
  install_python_pkg pyserial
  install_python_pkg littlefs-python

  echo ""
  echo "[arduino-cli]"
  install_arduino_cli

  echo ""
  echo "[ESP32 core & libraries]"
  install_esp32_core
  install_arduino_libs
  echo ""
}

if [ "${1:-}" != "--skip-install" ]; then
  install_deps
fi

PYTHON="python3"
if [ -f "$VENV_DIR/bin/pip" ]; then
  PYTHON="$VENV_DIR/bin/python3"
fi

echo "=== 1/3: Compiling $SKETCH for $FQBN ==="
arduino-cli compile --fqbn "$FQBN" "$SKETCH" --build-path "$BUILD_DIR"

echo ""
echo "=== 2/3: Building LittleFS image from $DATA_DIR/ ==="
"$PYTHON" /dev/stdin <<PYEOF
import os
from littlefs import LittleFS

data_dir = "$DATA_DIR"
block_size = 4096
block_count = 352  # 0x160000 / 4096

lfs = LittleFS(
    block_size=block_size,
    block_count=block_count,
    read_size=256,
    prog_size=256,
)

for root, dirs, files in os.walk(data_dir):
    for fname in files:
        rel_dir = os.path.relpath(root, data_dir)
        lpath = os.path.join("/", rel_dir, fname) if rel_dir != "." else "/" + fname
        with open(os.path.join(root, fname), "rb") as f:
            content = f.read()
        parent = os.path.dirname(lpath)
        if parent and parent != "/":
            try:
                lfs.makedirs(parent)
            except FileExistsError:
                pass
        with lfs.open(lpath, "wb") as f:
            f.write(content)
        print(f"  Added: {lpath} ({len(content)} bytes)")

with open("$BUILD_DIR/littlefs.bin", "wb") as f:
    f.write(lfs.context.buffer)

fsize = os.path.getsize("$BUILD_DIR/littlefs.bin")
expect = block_size * block_count
assert fsize == expect, f"Size mismatch: {fsize} != {expect}"
print(f"  LittleFS image: {fsize} bytes ({fsize/1024/1024:.2f} MB)")
PYEOF

echo ""
echo "=== 3/3: Merging into single binary ==="
BOOTLOADER="$BUILD_DIR/$SKETCH.bootloader.bin"
PARTITIONS="$BUILD_DIR/$SKETCH.partitions.bin"
APP="$BUILD_DIR/$SKETCH.bin"
LITTLEFS="$BUILD_DIR/littlefs.bin"

ESP32_CORE=$(ls -d ~/.arduino15/packages/esp32/hardware/esp32/*/ 2>/dev/null | sort -V | tail -1)
BOOTAPP0="${ESP32_CORE}tools/partitions/boot_app0.bin"
if [ ! -f "$BOOTAPP0" ]; then
    echo "ERROR: boot_app0.bin not found at $BOOTAPP0"
    exit 1
fi

"$PYTHON" -m esptool --chip esp32c3 merge-bin \
    --flash-mode "$FLASH_MODE" \
    --flash-freq "$FLASH_FREQ" \
    --flash-size "$FLASH_SIZE" \
    --output "$COMBINED_BIN" \
    0x00000 "$BOOTLOADER" \
    0x08000 "$PARTITIONS" \
    0x0e000 "$BOOTAPP0" \
    0x10000 "$APP" \
    $FS_OFFSET "$LITTLEFS"

echo ""
echo "=== Done! ==="
echo "Combined binary: $COMBINED_BIN"
ls -lh "$COMBINED_BIN"

echo ""
echo "Flash it with:"
echo "  python3 \"$SCRIPT_DIR/flashtool/flash_esp.py\" \"$COMBINED_BIN\""
echo "  (or use the GUI flasher)"
