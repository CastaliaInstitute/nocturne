#!/usr/bin/env sh
set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR="${ROOT_DIR}/.build"
mkdir -p "$BUILD_DIR"

report_battery_snapshot() {
  local battery_root
  battery_root="$(find /sys/class/power_supply -maxdepth 1 -type d -name 'BAT*' 2>/dev/null | head -n 1 || true)"
  if [ -z "$battery_root" ]; then
    echo "battery_snapshot: unavailable_in_ci"
    return
  fi

  local status_path="$battery_root/status"
  local cap_path="$battery_root/capacity"
  local energy_now_path="$battery_root/energy_now"
  local energy_full_path="$battery_root/energy_full"
  local now=""
  local full=""

  if [ -f "$status_path" ]; then
    echo "battery_snapshot: status=$(cat "$status_path")"
  fi
  if [ -f "$cap_path" ]; then
    echo "battery_snapshot: capacity=$(cat "$cap_path")%"
  fi
  if [ -f "$energy_now_path" ] && [ -f "$energy_full_path" ]; then
    now="$(cat "$energy_now_path")"
    full="$(cat "$energy_full_path")"
    if [ -n "$now" ] && [ -n "$full" ] && [ "$full" -gt 0 ] 2>/dev/null; then
      printf 'battery_snapshot: energy_ratio=%.4f\n' "$(awk "BEGIN{print $now / $full}")"
    fi
  fi
}

report_battery_snapshot

cc -std=c11 -Wall -Wextra -Werror -Wpedantic -I"$ROOT_DIR" \
  -o "$BUILD_DIR/core-phase11-cpuprofile" \
  "$ROOT_DIR/core/tests/phase11_cpu_profile_test.c"

"$BUILD_DIR/core-phase11-cpuprofile"
