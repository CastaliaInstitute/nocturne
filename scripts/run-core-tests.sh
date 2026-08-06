#!/usr/bin/env sh
set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR="${ROOT_DIR}/.build"
mkdir -p "$BUILD_DIR"

cc -std=c11 -Wall -Wextra -Werror -Wpedantic -I"${ROOT_DIR}" \
  -o "$BUILD_DIR/core-scheduler-test" \
  "$ROOT_DIR/core/tests/scheduler_unit_test.c"

cc -std=c11 -Wall -Wextra -Werror -Wpedantic -I"${ROOT_DIR}" \
  -o "$BUILD_DIR/core-module-test" \
  "$ROOT_DIR/core/tests/core_unit_test.c"

"$BUILD_DIR/core-scheduler-test"
"$BUILD_DIR/core-module-test"

rm -f "$BUILD_DIR/core-scheduler-test"
rm -f "$BUILD_DIR/core-module-test"
