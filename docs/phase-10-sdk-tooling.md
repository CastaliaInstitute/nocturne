# Phase 10 — Developer SDK and Tooling

## Implemented in this pass

- Added lightweight inspection CLI at `scripts/nocturne-cli.mjs` with commands:
  - `validate-pack-metadata <file>`
  - `print-fused-intent [file]`
  - `deterministic-dryrun [mode] [iterations]`
- Added deterministic dry-run example output path for scripted QA workflows.
- Added sample pack metadata fixture: `core/tests/fixtures/nocturne-pack-metadata.json`.
- Added `core/tests/core_unit_test.c` and `core/tests/scheduler_unit_test.c` as inspection/test primitives.
- Added public SDK API header with version contract: `core/include/nocturne/sdk/nocturne_sdk_api.h`.
- Added adapter plugin contract and manifest contract in `core/include/nocturne/adapters/adapter_plugin.h`.
- Added adapter template and generation helper:
  - `templates/adapter/nocturne_adapter_template.c`
  - `templates/adapter/nocturne_adapter_template.txt`
  - `scripts/gen-adapter-template.mjs`
- Added mini-app reference documents:
  - `examples/mini-apps/scheduler-intent-loop`
  - `examples/mini-apps/seed-bridge`
  - `examples/mini-apps/pwa-launcher`
- Added contributor onboarding docs: `docs/contributors/onboarding.md`.
- Added developer-facing Phase 10 plugin guide: `docs/phase-10-adapter-plugins.md`.
- Added language binding scaffolds:
  - `bindings/README.md`
  - `bindings/cpp/nocturne_cpp_bindings.hpp`
  - `bindings/rust/src/lib.rs`
  - `bindings/kotlin/NocturneSdk.kt`

## Open items in this phase

- Publish/version and package bindings artifacts for external-language clients.
