#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_NAME="ocrtoodt-dev:latest"
CONTAINER_WORKDIR="/workspace"

podman build -t "${IMAGE_NAME}" -f "${PROJECT_DIR}/Containerfile" "${PROJECT_DIR}"

podman run --rm -it \
  --userns keep-id \
  --security-opt label=disable \
  -v "${PROJECT_DIR}:${CONTAINER_WORKDIR}:Z" \
  -w "${CONTAINER_WORKDIR}" \
  "${IMAGE_NAME}" \
  bash -lc '
    ./scripts/configure.sh &&
    ./scripts/build.sh
  '
