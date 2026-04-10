#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_NAME="ocrtoodt-dev:latest"
CONTAINER_WORKDIR="/workspace"
BINARY="${CONTAINER_WORKDIR}/build/podman-debug/OCRtoODT"

XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}"
HOME_DIR="${HOME}"

podman build -t "${IMAGE_NAME}" -f "${PROJECT_DIR}/Containerfile" "${PROJECT_DIR}"

podman run --rm -it \
  --userns keep-id \
  --security-opt label=disable \
  -e HOME="${HOME_DIR}" \
  -e XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR}" \
  -e WAYLAND_DISPLAY="${WAYLAND_DISPLAY}" \
  -e QT_QPA_PLATFORM=wayland \
  -v "${PROJECT_DIR}:${CONTAINER_WORKDIR}:Z" \
  -v "${HOME_DIR}:${HOME_DIR}:Z" \
  -v "${XDG_RUNTIME_DIR}:${XDG_RUNTIME_DIR}" \
  -w "${CONTAINER_WORKDIR}" \
  "${IMAGE_NAME}" \
  "${BINARY}"
