# Nocturne

An adaptive, context-aware acoustic intelligence platform for sleep, meditation,
ritual, and immersive listening experiences.

Nocturne is an evolving sound engine that responds to context, physiology,
chronology, and user intention without relying on a music library or single fixed
playlists.

## Mission

Create an open, modular acoustic intelligence platform capable of producing
personalized, context-aware soundscapes that improve user experience through
adaptive composition rather than fixed recordings.

## Core principles

- Local-first operation with offline playback
- Explainable adaptations with deterministic behavior
- Privacy-preserving data handling and policy controls
- Embedded-first design for ESP32-S3 and future expansion to ESP32-P4, Android,
  desktop, and other platforms
- Safe adaptation bounded by user preferences and acoustic guardrails

## Repository overview

- `docs/architecture/` contains the architecture specification and design notes.
- `core/` is reserved for the platform-agnostic engine implementation.
- `platform/` contains platform adapter projects (ESP-IDF, Android, desktop).
- `samples/` stores acoustic assets, packs, and fixture test materials.
- `tools/` provides build and validation helpers.
- `tests/` holds simulation and policy tests.

This first commit is foundational documentation to establish the project structure
and system intent.

## High-level architecture

Inputs
-> Context Adapters
-> Context Fusion Engine
-> Acoustic Intent Vector
-> Scheduler
-> Voice Manager
-> DSP Engine
-> Platform Audio Backend

The acoustic intent model is the canonical contract between contextual systems and
the renderer.

```c
struct AcousticIntent {
    float grounding;
    float brightness;
    float warmth;
    float density;
    float motion;
    float spaciousness;
    float silence;
    float eventActivity;
    float harmonicTension;
    float novelty;
};
```

## Why this exists

Nocturne is designed to move beyond playlists and fixed compositions by keeping
sound generation decoupled from sensor, symbolic, and user-intent subsystems.
All adaptation is policy-driven and explainable.

## Roadmap snapshot

- DSP engine specification
- Sample pack compiler and tooling
- ESP-IDF reference implementation
- Android SDK
- Cloud optional APIs and federated learning flow
- Security and privacy model
- Developer SDK and reference applications

## Contributing

Contributions are expected to follow:

- Minimal, explainable changes
- Deterministic behavior for equivalent seeds and context
- Respect for safety limits and privacy policy

See `CONTRIBUTING.md` for details.
