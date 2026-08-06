# Nocturne TODO

Top-level completion condition: treat this repository as "ready for production release" only when all TODO items in this file are completed.

## Phase 0 — Foundation

- [ ] Establish initial repository defaults
- [ ] Define and enforce language/tooling policy
- [ ] Add MIT license and contribution governance
- [ ] Create docs and architecture spec publication structure
- [ ] Add core design glossary (Acoustic Intent, Chronology, Context Domains)
- [ ] Define semantic versioning and release cadence
- [ ] Set up CI pipeline for lint/build/doc checks
- [ ] Create project-wide issue and PR templates
- [ ] Add CODEOWNERS and branch protections
- [ ] Add changelog and release notes policy

## Phase 1 — Core Platform Data Models

- [ ] Implement shared context domain models
- [ ] Implement canonical `AcousticIntent` structure and serde support
- [ ] Add confidence, bounds, and provenance metadata to intent updates
- [ ] Add deterministic random-seed and session-seed plumbing
- [ ] Implement context signal schemas for Observed, Computed, Symbolic, User domains
- [ ] Add input validation and saturation/smoothing utilities
- [ ] Add temporal model with hierarchical chronology utilities
- [ ] Add serialization format versioning for compatibility
- [ ] Create golden-test fixtures for intent determinism
- [ ] Document all core data types in API docs

## Phase 2 — Context Adapter Layer

- [ ] Add sensor adapter abstraction interfaces
- [ ] Implement sensor adapters:
  - [ ] Heart rate
  - [ ] HRV
  - [ ] Motion
  - [ ] Breathing/respiration
  - [ ] Ambient light
  - [ ] Temperature
- [ ] Implement computed context adapters:
  - [ ] Solar and civil-time phase
  - [ ] Circadian estimation
  - [ ] Sleep debt estimate
  - [ ] Sunrise/sunset
  - [ ] Season and lunar phase
- [ ] Implement symbolic adapters:
  - [ ] Chakra frameworks
  - [ ] Astrology module
  - [ ] Ritual calendar module
  - [ ] Meditation/intention profile module
- [ ] Add user controls adapter for mode and safety settings
- [ ] Add adapter registration and lifecycle management
- [ ] Add simulation/fake adapters for deterministic tests
- [ ] Add diagnostics for missing/invalid context channels

## Phase 3 — Context Fusion Engine

- [ ] Implement weighted fusion engine with bounded influence
- [ ] Add bounded blend weights and anti-dominance safeguards
- [ ] Add adaptive smoothing and inertia constants
- [ ] Implement conflict-resolution policy when channels disagree
- [ ] Add per-channel confidence gating and trust windows
- [ ] Add "explainability pack" output for each adaptation decision
- [ ] Add policy hooks for safety and parental controls
- [ ] Add replayable event logs for every fusion step
- [ ] Implement telemetry-free local logging mode
- [ ] Add parameterized fusion profiles and tests

## Phase 4 — Scheduler and Sound Policy

- [ ] Define intent-to-policy mapping rules
- [ ] Implement scheduler for event density and pacing
- [ ] Add safety envelopes for loudness, high-frequency content, and novelty
- [ ] Implement sleep-safe mode with reduced surprise behavior
- [ ] Implement transition management with continuity constraints
- [ ] Add timer, phase, and interruption handling
- [ ] Add fallback scheduling behavior for missing intent
- [ ] Add deterministic scheduler traces for reproducibility
- [ ] Add scheduler unit tests and scenario tests

## Phase 5 — Voice Manager and Acoustic Rendering Contract

- [ ] Define voice abstractions for layers, stems, motifs, and textures
- [ ] Implement voice lifecycle manager (spawn/retire/cross-fade)
- [ ] Implement event selection policy from intent and scheduling state
- [ ] Add spatialization and motion descriptors mapping
- [ ] Add layering constraints and conflict avoidance
- [ ] Implement deterministic sample/event RNG seed handling
- [ ] Add render intent diffing and explainability labels
- [ ] Add support for muted/low-resource degraded mode
- [ ] Add render contract tests with expected param outputs

## Phase 6 — DSP and Pack Runtime

- [ ] Define DSP preset model and parameter envelopes
- [ ] Implement core DSP blocks needed for ambience/tension/shimmer
- [ ] Add DSP state serialization for resume/replay
- [ ] Implement resource-aware scheduling for embedded devices
- [ ] Add denoise, soft-limiter, and safety filter chain
- [ ] Add output metering and clipping detection
- [ ] Add calibration workflow for target response and output gain
- [ ] Build cross-platform sample pack metadata compiler
- [ ] Add offline pack download/verification cache strategy

## Phase 7 — Platform Abstraction Layer

- [ ] Implement platform audio backend abstraction
- [ ] Add backend adapter for ESP32-S3/ESP-IDF
- [ ] Add backend adapter for ESP32-P4 placeholder build
- [ ] Add Android audio backend adapter
- [ ] Add desktop backend adapter (desktop reference target)
- [ ] Add optional cloud/analysis adapter (non-blocking fallback)
- [ ] Add platform capability probes (RAM, CPU budget, battery, permissions)
- [ ] Add error, backoff, and offline recovery behavior per backend
- [ ] Add cross-platform integration smoke tests

## Phase 8 — Learning and Personalization

- [ ] Define on-device feature extraction for adaptation learning
- [ ] Implement embedded inference interface (lightweight)
- [ ] Implement local personalization profile persistence
- [ ] Implement safe confidence-weighted policy tuning
- [ ] Add offline Android personalization flow
- [ ] Add optional federated update pipeline skeleton
- [ ] Add rollback and undo for policy drift
- [ ] Add privacy-preserving anonymization options
- [ ] Add privacy audit logging for policy updates

## Phase 9 — Privacy, Security, and Trust

- [ ] Implement local-first policy enforcement points
- [ ] Add explicit consent flow and permission state machine
- [ ] Add encrypted storage for sensitive context/state
- [ ] Add key rotation and secure wipe workflow
- [ ] Add right-to-delete and data export support
- [ ] Add model portability format and migration path
- [ ] Add threat-model documentation and security controls matrix
- [ ] Add data minimization checks and lint guards
- [ ] Add penetration test checklist and periodic review items

## Phase 10 — Developer SDK and Tooling

- [ ] Define public SDK API and versioned interfaces
- [ ] Add Rust/C++/Kotlin bindings scaffold as needed
- [ ] Add adapter plugin interface and sample implementations
- [ ] Add CLI/inspection tooling:
  - [ ] Validate pack metadata
  - [ ] Print fused intent timeline
  - [ ] Run deterministic playback dry-run
- [ ] Add code generation templates for new adapters
- [ ] Add onboarding docs for contributors and integrators
- [ ] Add examples and mini apps demonstrating each layer

## Phase 11 — Validation, Testing, and QA

- [ ] Add unit tests for every core module
- [ ] Add deterministic simulation harness
- [ ] Add replay-based integration tests for long sessions
- [ ] Add safety boundary tests for policy and output limits
- [ ] Add stress tests for missing data and sensor dropout
- [ ] Add battery and CPU budget profiling
- [ ] Add cross-device acoustic parity tests where feasible
- [ ] Add fuzz testing for malformed context streams
- [ ] Add acceptance test suite for each user mode
- [ ] Add benchmark suite for startup, frame time, and drift

## Phase 12 — Documentation and Reference Content

- [ ] Expand architecture docs to Volumes II-VII
- [ ] Add API reference docs and sequence diagrams
- [ ] Add symbol system adapter docs
- [ ] Add privacy model and deployment guide
- [ ] Add contributor onboarding guides per discipline
- [ ] Add release playbooks and rollback instructions
- [ ] Add user-facing explanation guide for adaptation behavior
- [ ] Add troubleshooting and known-limits documentation

## Phase 13 — Operational Readiness

- [ ] Set up package and artifact publishing
- [ ] Set up release automation and changelog generation
- [ ] Configure CI/CD for multi-platform build matrix
- [ ] Add monitoring dashboards for crashes and adaptation incidents
- [ ] Add canary deployment flow for mobile/embedded channels
- [ ] Add long-term support policy and support matrix
- [ ] Add project governance, maintainer process, and triage rotation
- [ ] Define issue labels and sprint cadence

## Phase 14 — Nocturne PWA Surface

- [ ] Reserve and configure production host `nocturne.castalia.institute`
- [ ] Stand up HTTPS static hosting and service account for Castalia deployment
- [ ] Implement secure Castalia authentication flow
- [ ] Implement token exchange and permission model for `castalia-{username}` repos
- [ ] Add repository access UI:
  - [ ] discover repo
  - [ ] list workspace files
  - [ ] show recent changes
- [ ] Add WebBLE pairing for ring and device peripherals
- [ ] Add consent dialogs for BLE and sensor operations
- [ ] Harden offline mode and install UX
- [ ] Add onboarding and troubleshooting docs for first-time setup
- [ ] Add app deployment pipeline and smoke tests

## Tracking

- [ ] Mark each item with status in project board
- [ ] Keep work in vertical slices to deliver end-to-end behavior at each milestone
- [ ] Prefer deterministic, explainable increments over broad rewrites
- [ ] Defer optional cloud features until local-first core is stable
