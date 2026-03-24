#!/bin/bash
# Generate qmonstatek.icns from app_icon.png (run on macOS only)
# Requires: sips, iconutil (built into macOS)

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC="$SCRIPT_DIR/app_icon.png"
ICONSET="$SCRIPT_DIR/qmonstatek.iconset"

mkdir -p "$ICONSET"
sips -z 16 16     "$SRC" --out "$ICONSET/icon_16x16.png"
sips -z 32 32     "$SRC" --out "$ICONSET/icon_16x16@2x.png"
sips -z 32 32     "$SRC" --out "$ICONSET/icon_32x32.png"
sips -z 64 64     "$SRC" --out "$ICONSET/icon_32x32@2x.png"
sips -z 128 128   "$SRC" --out "$ICONSET/icon_128x128.png"
sips -z 256 256   "$SRC" --out "$ICONSET/icon_128x128@2x.png"
sips -z 256 256   "$SRC" --out "$ICONSET/icon_256x256.png"
sips -z 512 512   "$SRC" --out "$ICONSET/icon_256x256@2x.png"
sips -z 512 512   "$SRC" --out "$ICONSET/icon_512x512.png"
sips -z 1024 1024 "$SRC" --out "$ICONSET/icon_512x512@2x.png"

iconutil -c icns "$ICONSET" -o "$SCRIPT_DIR/qmonstatek.icns"
rm -rf "$ICONSET"
echo "Created qmonstatek.icns"
