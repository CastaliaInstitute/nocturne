# Mini App: Scheduler Intent Loop

A small reference exercise showing how to run a deterministic scheduler loop from
fused intents and inspect the resulting policy outputs.

## What this demonstrates

- Deterministic startup with fixed seed policy
- Repeated scheduling updates through a single mode
- Logging `target_event_density`, `target_loudness`, `target_high_frequency_content`

## Inputs

- `intent` values are generated in-band from a small synthetic sweep.
- Output logs are intended for debugging, not production sound output.

This mini-app is currently documented as a scaffold for adapter and SDK
integration testing.
