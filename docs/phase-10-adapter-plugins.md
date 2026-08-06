# Nocturne SDK Plugin Interface

## Versioned API Surface

- Public API version: `NOCTURNE_SDK_VERSION_MAJOR.MINOR.PATCH` from `core/include/nocturne/sdk/nocturne_sdk_api.h`.
- Plugins are expected to expose a `nocturne_adapter_plugin_t` manifest in
  `core/include/nocturne/adapters/adapter_plugin.h`.

## Adapter Plugin Contract

- `api_version` must match host expected API major.
- `plugin_name` must be non-empty.
- `capability_mask` expresses optional feature support.
- Host checks support via:
  - `nocturne_adapter_plugin_is_supported`
  - `nocturne_adapter_plugin_release`

## Template and generation

- `templates/adapter/nocturne_adapter_template.c`: C adapter body skeleton.
- `templates/adapter/nocturne_adapter_template.txt`: template usage notes.
- `scripts/gen-adapter-template.mjs`: generate a per-plugin source file.

## Reference mini apps

- `examples/mini-apps/scheduler-intent-loop`
- `examples/mini-apps/seed-bridge`
- `examples/mini-apps/pwa-launcher`

## Onboarding path

- Use `core/include/nocturne/sdk/nocturne_sdk_api.h` for host compatibility checks.
- Use plugin manifests to register adapters safely.
- Keep plugin vtable naming stable and document any non-portable behavior in the
  adapter README.
