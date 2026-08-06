#!/usr/bin/env sh
set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR="${ROOT_DIR}/.build"
mkdir -p "$BUILD_DIR"

cc -std=c11 -Wall -Wextra -Werror -Wpedantic -I"$ROOT_DIR" \
  -o "$BUILD_DIR/core-benchmark" \
  "$ROOT_DIR/core/tests/benchmark_core.c"

"$BUILD_DIR/core-benchmark"
