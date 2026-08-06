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

cc -std=c11 -Wall -Wextra -Werror -Wpedantic -I"${ROOT_DIR}" \
  -o "$BUILD_DIR/core-phase11-test" \
  "$ROOT_DIR/core/tests/phase11_integration_test.c"

cc -std=c11 -Wall -Wextra -Werror -Wpedantic -I"${ROOT_DIR}" \
  -o "$BUILD_DIR/core-phase11-fuzz" \
  "$ROOT_DIR/core/tests/phase11_fuzz_context_test.c"

cc -std=c11 -Wall -Wextra -Werror -Wpedantic -I"${ROOT_DIR}" \
  -o "$BUILD_DIR/core-phase11-acceptance" \
  "$ROOT_DIR/core/tests/phase11_acceptance_modes_test.c"

cc -std=c11 -Wall -Wextra -Werror -Wpedantic -I"${ROOT_DIR}" \
  -o "$BUILD_DIR/core-phase11-parity" \
  "$ROOT_DIR/core/tests/phase11_parity_test.c"

cc -std=c11 -Wall -Wextra -Werror -Wpedantic -I"${ROOT_DIR}" \
  -o "$BUILD_DIR/core-phase11-cpuprofile" \
  "$ROOT_DIR/core/tests/phase11_cpu_profile_test.c"

cc -std=c11 -Wall -Wextra -Werror -Wpedantic -I"${ROOT_DIR}" \
  -o "$BUILD_DIR/core-module-coverage" \
  "$ROOT_DIR/core/tests/core_module_coverage_test.c"

cc -std=c11 -Wall -Wextra -Werror -Wpedantic -I"${ROOT_DIR}" \
  -o "$BUILD_DIR/core-voice-manager-test" \
  "$ROOT_DIR/core/tests/voice_manager_test.c"

"$BUILD_DIR/core-scheduler-test"
"$BUILD_DIR/core-module-test"
"$BUILD_DIR/core-phase11-test"
"$BUILD_DIR/core-phase11-fuzz"
"$BUILD_DIR/core-phase11-acceptance"
"$BUILD_DIR/core-phase11-parity"
"$BUILD_DIR/core-phase11-cpuprofile"
"$BUILD_DIR/core-module-coverage"
"$BUILD_DIR/core-voice-manager-test"

rm -f "$BUILD_DIR/core-scheduler-test"
rm -f "$BUILD_DIR/core-module-test"
rm -f "$BUILD_DIR/core-phase11-test"
rm -f "$BUILD_DIR/core-phase11-fuzz"
rm -f "$BUILD_DIR/core-phase11-acceptance"
rm -f "$BUILD_DIR/core-phase11-parity"
rm -f "$BUILD_DIR/core-phase11-cpuprofile"
rm -f "$BUILD_DIR/core-module-coverage"
rm -f "$BUILD_DIR/core-voice-manager-test"
