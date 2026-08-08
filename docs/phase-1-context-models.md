# Phase 1 — Core Platform Data Models

This document defines the initial shared schema and data contract for deterministic
context and acoustic-intent modeling.

## Implemented artifacts

- `core/include/nocturne/context.h`
  - `nocturne_context_domain_t`
  - `nocturne_acoustic_intent_t`
  - `nocturne_context_contribution_t`
  - Clamp and smoothing helpers
- `core/include/nocturne/seed.h`
  - deterministic seed state and progression
- `core/schemas/acoustic_intent.schema.json`
  - JSON envelope for `AcousticIntent`

## Boundaries

- This release includes schema definitions only.
- Fusion and weighted aggregation are defined in Phase 3.
- Domain-specific adapters and serialization tooling are defined in later work items.
