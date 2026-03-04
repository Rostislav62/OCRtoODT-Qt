#!/usr/bin/env bash
set -euo pipefail

# ============================================================
#  OCRtoODT — One-command Release Builder (Local + GitHub)
#  File: build_appimage.sh
#
#  Options:
#    --auto-commit-version
#        If (and only if) the ONLY tracked change is CMakeLists.txt,
#        commits it with message: "Bump version to X.Y.Z"
#
#  Guarantees:
#    - Single source of truth: version is read from CMakeLists.txt
#    - Never runs git clean
#    - Never deletes anything
#    - Ignores untracked/ignored files (your dumps stay untouched)
# ============================================================

APP="OCRtoODT"
ARCH="x86_64"

PROJECT_FILE="CMakeLists.txt"
TAG_SCRIPT="./scripts/tag_from_cmake.sh"

BUILD_DIR="build-release"
APPDIR="AppDir"

LINUXDEPLOY="./linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="./linuxdeploy-plugin-qt-x86_64.AppImage"

DESKTOP_FILE_SRC="resources/OCRtoODT.desktop"
ICON_FILE_SRC="resources/icons/ocrtoodt.png"

AUTO_COMMIT_VERSION=false
NO_PUSH=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --auto-commit-version) AUTO_COMMIT_VERSION=true; shift ;;
    --no-push)             NO_PUSH=true; shift ;;
    *)
      echo "Usage: $0 [--auto-commit-version] [--no-push]" >&2
      exit 2
      ;;
  esac
done

die() { echo "ERROR: $*" >&2; exit 1; }
info() { echo "[INFO] $*"; }
warn() { echo "[WARN] $*" >&2; }

require_file() { [[ -f "$1" ]] || die "Missing file: $1"; }
require_exe() { [[ -x "$1" ]] || die "Missing or not executable: $1"; }

parse_cmake_version() {
  local line ver
  line="$(grep -E '^[[:space:]]*project\(OCRtoODT[[:space:]]+VERSION[[:space:]]+[0-9]+\.[0-9]+\.[0-9]+' -n "$PROJECT_FILE" || true)"
  [[ -n "$line" ]] || die "Cannot find: project(OCRtoODT VERSION X.Y.Z ...) in $PROJECT_FILE"
  ver="$(echo "$line" | sed -E 's/.*VERSION[[:space:]]+([0-9]+\.[0-9]+\.[0-9]+).*/\1/')"
  [[ -n "$ver" ]] || die "Failed to parse version from: $line"
  echo "$ver"
}

# ------------------------------------------------------------
# 0) Sanity checks
# ------------------------------------------------------------
require_file "$PROJECT_FILE"
require_file "$DESKTOP_FILE_SRC"
require_file "$ICON_FILE_SRC"
require_exe "$TAG_SCRIPT"

command -v git >/dev/null || die "git not found"
command -v cmake >/dev/null || die "cmake not found"

git rev-parse --is-inside-work-tree >/dev/null 2>&1 || die "Not a git repository"

# ------------------------------------------------------------
# 1) Optional auto-commit version bump
# ------------------------------------------------------------
if $AUTO_COMMIT_VERSION; then
  # Allow ONLY tracked modifications, and ONLY in CMakeLists.txt
  tracked_changed="$(git status --porcelain | awk '$1 ~ /^M|^A|^D|^R|^C/ {print $2}' || true)"
  # Note: git status --porcelain also prints ??, we ignore those by filtering statuses above.

  if [[ -z "$tracked_changed" ]]; then
    info "No tracked changes detected. Auto-commit not needed."
  else
    # Ensure exactly one tracked file changed and it's CMakeLists.txt
    count="$(echo "$tracked_changed" | wc -l | tr -d ' ')"
    if [[ "$count" != "1" || "$tracked_changed" != "$PROJECT_FILE" ]]; then
      echo "[ERROR] --auto-commit-version can commit ONLY when the ONLY tracked change is ${PROJECT_FILE}" >&2
      echo "Tracked changes detected:" >&2
      echo "$tracked_changed" >&2
      die "Refusing to auto-commit."
    fi

    # Ensure no staged changes (we'll stage exactly the file we want)
    if ! git diff --cached --quiet; then
      die "Index has staged changes. Commit/stash them first (auto-commit expects clean index)."
    fi

    # Parse version AFTER your edit
    VER_CMAKE="$(parse_cmake_version)"
    COMMIT_MSG="Bump version to ${VER_CMAKE}"

    info "Auto-committing ${PROJECT_FILE} with message: ${COMMIT_MSG}"
    git add "$PROJECT_FILE"
    git commit -m "$COMMIT_MSG"
  fi
fi

# ------------------------------------------------------------
# 2) Now enforce tracked-clean (untracked/ignored is OK)
# ------------------------------------------------------------
if ! git diff --quiet; then
  die "UNSTAGED tracked changes detected. Commit/stash them first."
fi
if ! git diff --cached --quiet; then
  die "STAGED changes detected. Commit them first."
fi

# ------------------------------------------------------------
# 3) Extract version from CMake (single source of truth)
# ------------------------------------------------------------
VER_CMAKE="$(parse_cmake_version)"
TAG_EXPECTED="v${VER_CMAKE}"
OUT_APPIMAGE="${APP}-v${VER_CMAKE}-${ARCH}.AppImage"

info "CMake version : ${VER_CMAKE}"
info "Expected tag  : ${TAG_EXPECTED}"
info "Output file   : ${OUT_APPIMAGE}"

# ------------------------------------------------------------
# 4) Ensure exact tag exists on HEAD and matches version
# ------------------------------------------------------------
HEAD_TAG="$(git describe --tags --exact-match 2>/dev/null || true)"
if [[ -z "$HEAD_TAG" ]]; then
  warn "HEAD has no exact tag. Creating tag from CMake: ${TAG_EXPECTED}"
  "$TAG_SCRIPT" >/dev/null
  HEAD_TAG="$(git describe --tags --exact-match 2>/dev/null || true)"
fi

[[ -n "$HEAD_TAG" ]] || die "Still no exact tag on HEAD after running tag script."
if [[ "$HEAD_TAG" != "$TAG_EXPECTED" ]]; then
  echo "[ERROR] Tag/CMake mismatch"
  echo "        CMakeLists.txt version: ${VER_CMAKE}"
  echo "        Expected tag          : ${TAG_EXPECTED}"
  echo "        HEAD exact tag        : ${HEAD_TAG}"
  die "Fix mismatch: retag HEAD or adjust CMake version."
fi

info "HEAD exact tag OK: ${HEAD_TAG}"

# ------------------------------------------------------------
# 5) Push (branch + exact tag) if origin exists
# ------------------------------------------------------------
if $NO_PUSH; then
  warn "--no-push specified. Skipping git push."
else
  if git remote get-url origin >/dev/null 2>&1; then
    current_branch="$(git rev-parse --abbrev-ref HEAD)"
    info "Remote origin detected. Pushing branch '${current_branch}' and tag '${TAG_EXPECTED}'..."

    # Push commits first
    git push origin "${current_branch}"

    # Push only the expected tag (safer than --tags)
    git push origin "refs/tags/${TAG_EXPECTED}"

    info "Push done."
  else
    warn "No remote 'origin'. Skipping git push."
  fi
fi

# ------------------------------------------------------------
# 6) Build Release
# ------------------------------------------------------------
info "Building Release..."
rm -rf "$BUILD_DIR" "$APPDIR"
rm -f "${APP}-${ARCH}.AppImage" "$OUT_APPIMAGE"

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j"$(nproc)"

# ------------------------------------------------------------
# 7) Install to AppDir via CMake install rules
# ------------------------------------------------------------
info "Installing into AppDir..."
cmake --install "$BUILD_DIR" --prefix "$APPDIR/usr"

# ------------------------------------------------------------
# 8) Optional: Wayland plugins (best effort)
# ------------------------------------------------------------
info "Optional: Wayland plugins (best effort)..."
mkdir -p "$APPDIR/usr/plugins/platforms"
for so in libqwayland-egl.so libqwayland-generic.so; do
  if [[ -f "/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms/${so}" ]]; then
    cp "/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms/${so}" "$APPDIR/usr/plugins/platforms/" || true
  fi
done

# ------------------------------------------------------------
# 9) linuxdeploy presence checks
# ------------------------------------------------------------
if [[ ! -x "$LINUXDEPLOY" || ! -x "$LINUXDEPLOY_QT" ]]; then
  cat >&2 <<EOF
ERROR: linuxdeploy tools are missing.
Expected:
  $LINUXDEPLOY
  $LINUXDEPLOY_QT

Fix (once) from repo root:
  wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage -O linuxdeploy-x86_64.AppImage
  wget -q https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage -O linuxdeploy-plugin-qt-x86_64.AppImage
  chmod +x linuxdeploy-x86_64.AppImage linuxdeploy-plugin-qt-x86_64.AppImage
EOF
  exit 1
fi

# ------------------------------------------------------------
# 10) Run linuxdeploy -> produces OCRtoODT-x86_64.AppImage
# ------------------------------------------------------------
info "Packaging AppImage via linuxdeploy..."
unset QML2_IMPORT_PATH QML_IMPORT_PATH QT_PLUGIN_PATH

"$LINUXDEPLOY" \
  --appdir "$APPDIR" \
  --executable "$APPDIR/usr/bin/$APP" \
  --desktop-file "$APPDIR/usr/share/applications/OCRtoODT.desktop" \
  --icon-file "$ICON_FILE_SRC" \
  --plugin qt \
  --output appimage

# ------------------------------------------------------------
# 11) Rename output to include version
# ------------------------------------------------------------
[[ -f "${APP}-${ARCH}.AppImage" ]] || die "Expected output not found: ${APP}-${ARCH}.AppImage"
mv "${APP}-${ARCH}.AppImage" "$OUT_APPIMAGE"

info "SUCCESS: Built ${OUT_APPIMAGE}"
ls -lah "$OUT_APPIMAGE"


# ============================================================
#  OCRtoODT — One-command Release Builder (Local + GitHub)
#  File: build_appimage.sh
#
#  PURPOSE
#  -------
#  This script builds a *Release* AppImage with a *versioned filename*
#  and enforces “single source of truth” versioning:
#      - Version is taken ONLY from CMakeLists.txt:
#            project(OCRtoODT VERSION X.Y.Z LANGUAGES CXX)
#      - Git tag MUST be exactly: vX.Y.Z and MUST point to current HEAD
#      - Output AppImage filename will be:
#            OCRtoODT-vX.Y.Z-x86_64.AppImage
#
#  IMPORTANT SAFETY RULES
#  ----------------------
#  - The script NEVER runs `git clean` and NEVER deletes your repo files.
#  - Untracked / ignored files (e.g. _project_dump/, translations/, dumps)
#    are NOT touched and do NOT block the release (only tracked changes do).
#  - Tracked changes ARE guarded, because they affect what you ship.
#
#  PREREQUISITES (ONCE)
#  -------------------
#  1) Make script executable:
#       chmod +x build_appimage.sh
#
#  2) Ensure linuxdeploy tools exist in repo root:
#       linuxdeploy-x86_64.AppImage
#       linuxdeploy-plugin-qt-x86_64.AppImage
#
#     If missing, download once:
#       wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \
#         -O linuxdeploy-x86_64.AppImage
#       wget -q https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage \
#         -O linuxdeploy-plugin-qt-x86_64.AppImage
#       chmod +x linuxdeploy-x86_64.AppImage linuxdeploy-plugin-qt-x86_64.AppImage
#
#  3) Ensure tag helper exists and is executable:
#       ./scripts/tag_from_cmake.sh
#
#  HOW IT WORKS (PIPELINE)
#  ----------------------
#  A) (Optional) Auto-commit version bump:
#     - If you pass --auto-commit-version:
#         • Script checks tracked changes.
#         • It will auto-commit ONLY if the ONLY tracked change is CMakeLists.txt.
#         • Commit message becomes: "Bump version to X.Y.Z"
#
#  B) Hard guards (always):
#     - No staged or unstaged tracked changes allowed (release must be deterministic).
#     - Script reads version X.Y.Z from CMakeLists.txt.
#     - Script ensures HEAD has exact tag vX.Y.Z.
#       If no exact tag exists, it runs ./scripts/tag_from_cmake.sh to create it.
#     - Script verifies: HEAD tag == vX.Y.Z == CMake version.
#       If mismatch -> FAIL (prevents desync).
#
#  C) (Optional) Push to GitHub:
#     - Default: pushes current branch + the exact tag vX.Y.Z to origin.
#     - If you pass --no-push:
#         • Script does NOT push anything (local-only build).
#         ./build_appimage.sh --no-push
#
#  D) Build and package:
#     - Configures and builds CMake Release into build-release/
#     - Installs into AppDir/ via CMake install rules
#     - Runs linuxdeploy + qt plugin to produce OCRtoODT-x86_64.AppImage
#     - Renames output to OCRtoODT-vX.Y.Z-x86_64.AppImage
#
#  HOW TO USE (TYPICAL)
#  -------------------
#  1) Change ONLY the version line in CMakeLists.txt:
#       project(OCRtoODT VERSION 3.0.3 LANGUAGES CXX)
#
#  2) Run release build with auto commit + auto tag + push:
#       chmod +x build_appimage.sh
#       ./build_appimage.sh --auto-commit-version
#
#     Result:
#       - Local file: OCRtoODT-v3.0.3-x86_64.AppImage
#       - GitHub: commit pushed, tag v3.0.3 pushed -> Actions builds release asset
#
#  LOCAL-ONLY BUILD (NO GITHUB PUSH)
#  --------------------------------
#  Use --no-push to build only locally (no git push, no tags pushed to GitHub):
#
#       ./build_appimage.sh --auto-commit-version --no-push
#
#     Result:
#       - Local file: OCRtoODT-vX.Y.Z-x86_64.AppImage
#       - Nothing pushed to GitHub (no workflow triggered)
#
#  STRICT MODE (NO AUTO-COMMIT)
#  ---------------------------
#  If you prefer manual commits/tags, do them yourself, then run without flags:
#
#       ./build_appimage.sh
#
#  EXIT CONDITIONS (WHY IT FAILS)
#  -----------------------------
#  - Tracked working tree is dirty (staged/unstaged)  -> FAIL
#  - Version in CMakeLists.txt cannot be parsed       -> FAIL
#  - HEAD tag exists but does not match CMake version -> FAIL
#  - linuxdeploy tools missing / not executable       -> FAIL
# ============================================================
