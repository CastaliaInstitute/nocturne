# API Reference: Core Context Types

## AcousticIntent

`nocturne_acoustic_intent_t`

- `grounding`
- `brightness`
- `warmth`
- `density`
- `motion`
- `spaciousness`
- `silence`
- `event_activity`
- `harmonic_tension`
- `novelty`

Each field is normalized into `[0.0, 1.0]`.

## Influence entry

`nocturne_influence_t`

- `value`: raw contribution value before fusion
- `confidence`: quality score for that channel
- `min_bound`, `max_bound`: per-channel clamp boundaries
- `provenance_weight`: bounded contribution weight source preference

## Domain

`nocturne_context_domain_t`

- `observed`
- `computed`
- `symbolic`
- `user`

## Envelope

`nocturne_context_contribution_t` and `nocturne_fused_frame_t` carry deterministic
intent updates and fused state for replay.

## Deterministic seed state

`nocturne_seed_state_t`

- `seed`
- `session_id`
- `sequence`
