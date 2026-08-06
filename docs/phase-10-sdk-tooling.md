# Phase 10 — Developer SDK and Tooling

## Implemented in this pass

- Added lightweight inspection CLI at `scripts/nocturne-cli.mjs` with commands:
  - `validate-pack-metadata <file>`
  - `print-fused-intent [file]`
  - `deterministic-dryrun [mode] [iterations]`
- Added deterministic dry-run example output path for scripted QA workflows.
- Added sample pack metadata fixture: `core/tests/fixtures/nocturne-pack-metadata.json`.
- Added `core/tests/core_unit_test.c` and `core/tests/scheduler_unit_test.c` as inspection/test primitives.

## Open items in this phase

- Add public SDK API/versioned interfaces.
- Add Rust/C++/Kotlin bindings scaffold.
- Add onboarding docs for contributors and integrators.
- Add adapter examples and mini-app reference layer demonstrations.
