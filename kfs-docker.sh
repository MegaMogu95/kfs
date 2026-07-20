#!/usr/bin/env bash
# Build/run helper for the KFS docker toolchain.
#
#   ./kfs-docker.sh build      -> build the image (first time only, slow)
#   ./kfs-docker.sh make       -> run `make` inside the container
#   ./kfs-docker.sh make re    -> run any make target
#   ./kfs-docker.sh shell      -> drop into a shell in the container
#
# Your current directory is mounted at /kfs, so build outputs
# (kfs, kfs.iso) appear right here on the host.
set -euo pipefail

IMAGE=kfs-toolchain

case "${1:-make}" in
  build)
    docker build -t "$IMAGE" .
    ;;
  shell)
    docker run --rm -it -v "$PWD":/kfs "$IMAGE" /bin/bash
    ;;
  *)
    docker run --rm -v "$PWD":/kfs "$IMAGE" "$@"
    ;;
esac