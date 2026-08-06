# Phase 4 — Scheduler and Sound Policy

This phase introduces the initial deterministic scheduler layer that maps fused intent
to constrained playback policy.

## Implemented in this pass

- Added intent-to-policy mapping primitives in `core/include/nocturne/scheduler/scheduler.h`.
- Implemented event density and pacing mapping from fused intent channels.
- Added safety envelope caps for:
  - loudness (`max_loudness`)
  - high-frequency content (`max_high_frequency_ratio`)
  - novelty gain (`max_novelty_gain`)
- Added sleep-safe reduction path and continuity smoothing for transitions.
- Added timer generation hooks (in seconds) and interruption hold behavior.
- Added deterministic, replayable scheduler trace entries with ring-buffer replay APIs.

## Open items in this phase

- Timer/phase lifecycle handlers for external session clocks.
- Formal interruption source hooks (gesture, app lifecycle, and stream interruption inputs).
- Cross-module tests for scheduler edge cases and missing-intent fallback behavior.
