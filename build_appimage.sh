#!/usr/bin/env bash
set -euo pipefail

# ============================================================
#  OCRtoODT — Local AppImage Builder
#  File: build_appimage.sh
#
#  Responsibility:
#      - Build Release binary locally
#      - Create AppDir via CMake install rules
#      - Package AppImage via linuxdeploy + qt plugin
#
#  Policy:
#      - Build is allowed ONLY on an exact git tag vX.Y.Z
#      - Tag version MUST match CMakeLists.txt project VERSION
#      - AppImage output name: OCRtoODT-vX.Y.Z-x86_64.AppImage
# ============================================================

APP="OCRtoODT"
BUILD_DIR="build-release"
APPDIR="AppDir"
ARCH="x86_64"

LINUXDEPLOY="./linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="./linuxdeploy-plugin-qt-x86_64.AppImage"

DESKTOP_FILE="resources/OCRtoODT.desktop"
ICON_FILE="resources/icons/ocrtoodt.png"

# ------------------------------------------------------------
# 1) Hard guard: require exact tag on current commit
# ------------------------------------------------------------
TAG="$(git describe --tags --exact-match 2>/dev/null || true)"
if [[ -z "${TAG}" ]]; then
  echo "ERROR: Current commit has no exact git tag."
  echo "       Create it from CMake: ./scripts/tag_from_cmake.sh"
  exit 1
fi

if [[ ! "${TAG}" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "ERROR: Tag '${TAG}' is not in vX.Y.Z format."
  exit 1
fi

VER_TAG="${TAG#v}"

# ------------------------------------------------------------
# 2) Guard: tag version must match CMake project VERSION
# ------------------------------------------------------------
VER_CMAKE="$(perl -ne 'print "$1\n" if /project\s*\(\s*OCRtoODT\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)/i' CMakeLists.txt | head -n1)"
if [[ -z "${VER_CMAKE}" ]]; then
  echo "ERROR: Cannot parse version from CMakeLists.txt"
  exit 1
fi

if [[ "${VER_TAG}" != "${VER_CMAKE}" ]]; then
  echo "ERROR: Tag version (${VER_TAG}) does not match CMake version (${VER_CMAKE})."
  exit 1
fi

# ------------------------------------------------------------
# 3) Guard: working tree must be clean
# ------------------------------------------------------------
if ! git diff --quiet || ! git diff --cached --quiet; then
  echo "ERROR: Working tree is dirty. Commit changes before building a release AppImage."
  exit 1
fi

OUT_APPIMAGE="${APP}-v${VER_TAG}-${ARCH}.AppImage"

echo "======================================"
echo " OCRtoODT AppImage local build"
echo " Tag      : ${TAG}"
echo " Version  : ${VER_TAG}"
echo " Output   : ${OUT_APPIMAGE}"
echo "======================================"

# ------------------------------------------------------------
# 4) Clean
# ------------------------------------------------------------
echo "=== Cleaning ==="
rm -rf "${BUILD_DIR}" "${APPDIR}"
rm -f "${APP}-${ARCH}.AppImage" "${OUT_APPIMAGE}"

# ------------------------------------------------------------
# 5) Configure + Build (Release)
# ------------------------------------------------------------
echo "=== Configure (Release) ==="
cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release

echo "=== Build ==="
cmake --build "${BUILD_DIR}" -j"$(nproc)"

# ------------------------------------------------------------
# 6) Install into AppDir (single source of truth: CMake install rules)
# ------------------------------------------------------------
echo "=== Install into AppDir ==="
cmake --install "${BUILD_DIR}" --prefix "${APPDIR}/usr"

# ------------------------------------------------------------
# 7) Optional: Wayland plugins (best effort)
#    (kept from your script, harmless, sometimes helps)
# ------------------------------------------------------------
echo "=== Optional: Wayland plugins (best effort) ==="
mkdir -p "${APPDIR}/usr/plugins/platforms"
for so in libqwayland-egl.so libqwayland-generic.so; do
  if [[ -f "/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms/${so}" ]]; then
    cp "/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms/${so}" "${APPDIR}/usr/plugins/platforms/"
  fi
done

# ------------------------------------------------------------
# 8) linuxdeploy present check
# ------------------------------------------------------------
if [[ ! -x "${LINUXDEPLOY}" ]]; then
  echo "ERROR: ${LINUXDEPLOY} not found or not executable."
  echo "       Download it (once) or place it near this script."
  exit 1
fi

if [[ ! -x "${LINUXDEPLOY_QT}" ]]; then
  echo "ERROR: ${LINUXDEPLOY_QT} not found or not executable."
  echo "       Download it (once) or place it near this script."
  exit 1
fi

# ------------------------------------------------------------
# 9) Run linuxdeploy
# ------------------------------------------------------------
echo "=== Running linuxdeploy ==="
unset QML2_IMPORT_PATH QML_IMPORT_PATH QT_PLUGIN_PATH

"${LINUXDEPLOY}" \
  --appdir "${APPDIR}" \
  --desktop-file "${APPDIR}/usr/share/applications/OCRtoODT.desktop" \
  --icon-file "${ICON_FILE}" \
  --plugin qt \
  --output appimage

# ------------------------------------------------------------
# 10) Rename output (versioned filename)
# ------------------------------------------------------------
if [[ ! -f "${APP}-${ARCH}.AppImage" ]]; then
  echo "ERROR: Expected output '${APP}-${ARCH}.AppImage' not found."
  exit 1
fi

mv "${APP}-${ARCH}.AppImage" "${OUT_APPIMAGE}"

echo ""
echo "======================================"
echo " AppImage build finished successfully"
echo " ${OUT_APPIMAGE}"
echo "======================================"
