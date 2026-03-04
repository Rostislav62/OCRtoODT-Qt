#!/usr/bin/env bash
set -euo pipefail

# 1) вытащить версию из CMakeLists.txt (строка project(... VERSION X.Y.Z ...)
VER="$(perl -ne 'print "$1\n" if /project\s*\(\s*OCRtoODT\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)/i' CMakeLists.txt | head -n1)"

if [[ -z "${VER}" ]]; then
  echo "ERROR: Cannot parse version from CMakeLists.txt"
  exit 1
fi

TAG="v${VER}"

# 2) убедиться что рабочее дерево чистое
if ! git diff --quiet || ! git diff --cached --quiet; then
  echo "ERROR: Working tree is dirty. Commit changes before tagging."
  exit 1
fi

# 3) создать (или проверить) тег
if git rev-parse -q --verify "refs/tags/${TAG}" >/dev/null; then
  echo "Tag ${TAG} already exists."
else
  git tag -a "${TAG}" -m "Release ${TAG}"
  echo "Created tag ${TAG}"
fi

echo "${TAG}"
