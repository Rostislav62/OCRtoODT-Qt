#!/usr/bin/env bash
set -euo pipefail

APP=OCRtoODT
BUILD_DIR=build-release
APPDIR=AppDir

echo "=== Cleaning ==="
rm -rf "$BUILD_DIR" "$APPDIR"

echo "=== Building Release ==="
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "=== Creating AppDir ==="
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"

cp "$BUILD_DIR/$APP" "$APPDIR/usr/bin/"
cp config.yaml "$APPDIR/usr/bin/"

# ВАЖНО: выбери один путь (ниже оставил как у тебя сейчас)
DESKTOP_FILE="resources/OCRtoODT.desktop"
ICON_FILE="resources/icons/ocrtoodt.png"

cp "$DESKTOP_FILE" "$APPDIR/usr/share/applications/"
cp "$ICON_FILE" "$APPDIR/usr/share/icons/hicolor/256x256/apps/"

echo "=== Optional: Wayland plugins (best effort) ==="
mkdir -p "$APPDIR/usr/plugins/platforms"
for so in libqwayland-egl.so libqwayland-generic.so; do
  if [ -f "/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms/$so" ]; then
    cp "/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms/$so" "$APPDIR/usr/plugins/platforms/"
  fi
done

echo "=== Running linuxdeploy ==="
# Не задаём QT_PLUGIN_PATH: пусть linuxdeploy-plugin-qt сам решит
unset QML2_IMPORT_PATH QML_IMPORT_PATH QT_PLUGIN_PATH

./linuxdeploy-x86_64.AppImage \
  --appdir "$APPDIR" \
  -d "$DESKTOP_FILE" \
  -i "$ICON_FILE" \
  --plugin qt \
  --output appimage

echo ""
echo "======================================"
echo " AppImage build finished successfully"
echo "======================================"
