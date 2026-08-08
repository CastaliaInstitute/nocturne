# Phase 11 — Validation, Testing, and QA

## Implemented in this pass

- Added C-based core unit tests in `core/tests/core_unit_test.c` covering:
  - context/compatibility/chronology/seed helpers
  - safety policy application
  - fusion profile configuration
  - simulated sensor + registry path
  - computed/symbolic/user context builders
- Added scheduler scenario/unit tests in `core/tests/scheduler_unit_test.c` covering:
  - fallback behavior
  - deterministic output for identical inputs
  - phase progression/wrap
  - interruption hold reductions
  - trace append + replay
- Added CI-backed execution entrypoint in `scripts/run-core-tests.sh`.
- Added `core-tests` job in `.github/workflows/ci.yml`.
- Added long-session validation in `core/tests/phase11_integration_test.c` covering:
  - deterministic simulation harness and long-session replay checks
  - scheduler safety-boundary assertions under extreme input
  - missing-data fallback and sensor-dropout stress checks
- Added benchmark harness in `core/tests/benchmark_core.c` plus CI test entrypoint `scripts/run-core-benchmark.sh` for startup timing and per-frame timing checks (avg/min/max frame time, drift).
- Added phase 11 hardening and coverage pass:
  - fuzz coverage for malformed context streams (`core/tests/phase11_fuzz_context_test.c`)
  - acceptance suite by user mode (`core/tests/phase11_acceptance_modes_test.c`)
  - deterministic cross-profile parity checks (`core/tests/phase11_parity_test.c`)
  - CPU profile pass (`core/tests/phase11_cpu_profile_test.c`) and orchestration script `scripts/run-core-profiles.sh`
  - module coverage validation for core layers (`core/tests/core_module_coverage_test.c`)

## CI status

- CI core job now runs both test and benchmark suites via `./scripts/run-core-tests.sh` and `./scripts/run-core-benchmark.sh`.

## Open items in this phase

- Expand cross-device acoustic parity checks across actual platform backends.
