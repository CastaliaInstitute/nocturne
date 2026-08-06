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

## Open items in this phase

- Deterministic simulation harness for long sessions.
- Replay-based integration tests for long sessions.
- Safety-boundary fuzz and performance benchmark coverage.
- Cross-device parity tests and broader acceptance suite.
