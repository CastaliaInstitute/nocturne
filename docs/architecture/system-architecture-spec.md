# Nocturne

## System Architecture Specification

**Version 1.0 (Draft)**

**Authors**
- Daniel C. McShan
- OpenAI ChatGPT

## Part I - Vision

### 1. Mission

To create an open, modular acoustic intelligence platform capable of producing
personalized, context-aware soundscapes that improve user experience through
adaptive composition rather than fixed recordings.

The system should:
- operate offline,
- learn from experience,
- respect user privacy,
- remain explainable,
- scale from embedded devices to cloud infrastructure, and
- remain artistically expressive.

### 2. Core Principles

#### 2.1 Separation of Concerns

The system separates:
- Context
- Interpretation
- Acoustic Intent
- Scheduling
- Rendering
- Platform

Each layer has one responsibility.

#### 2.2 Local First

Everything required for playback should function without cloud connectivity.
Cloud services improve personalization and research but are never required for
operation.

#### 2.3 Explainability

Every meaningful adaptation should be explainable.
The system should answer: "Why did the soundscape change?"
rather than behaving as an opaque black box.

#### 2.4 Safety

No learning algorithm may bypass:
- user preferences
- acoustic safety limits
- sleep-safe adaptation bounds
- privacy policies
- parental controls

#### 2.5 Determinism

Given identical seed, identical sound pack, identical context, and identical
policy, the engine should produce identical output.

## Part II - System Overview

### 3. High-Level Architecture

External Inputs
-> Context Adapters
-> Context Fusion Engine
-> Acoustic Intent Vector
-> Scheduler
-> Voice Manager
-> DSP Engine
-> Platform Audio Backend

This architecture intentionally isolates context from rendering.

### 4. External Inputs

Inputs are grouped into four categories.

#### Observed
Measured by sensors.
- Heart rate
- HRV
- Motion
- Breathing
- Ambient light
- Temperature

#### Computed
Derived mathematically.
- Circadian phase
- Sleep debt
- Sunrise
- Sunset
- Season
- Lunar phase

#### Symbolic
User-selected interpretive frameworks.
- Chakra systems
- Astrology
- Family synastry
- Ritual calendars
- Intentions
- Meditation traditions

These provide personalization and narrative context rather than physiological claims.

#### User Inputs
Explicit controls.
- Sleep
- Meditation
- Focus
- Ritual
- Volume
- Timer
- Sound pack
- Privacy settings

## Part III - Context Fusion

### 5. Philosophy

Every subsystem speaks a common language. Rather than allowing individual
modules to manipulate audio directly, each produces normalized semantic influence
vectors.

This creates one canonical interface between the outside world and the acoustic
engine.

### 6. Acoustic Intent Vector

The scheduler consumes only this structure.

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

Future dimensions may be added while maintaining backward compatibility.

### 7. Context Fusion

Context Fusion combines:
- Experience profile
- Current time
- Chronology
- Biometrics
- Symbolic context
- User settings

using bounded weighting and smoothing. No single subsystem can dominate the
output.

## Part IV - Chronology

Time is hierarchical:
- Year
- Season
- Solar cycle
- Lunar cycle
- Week
- Day
- Sleep phase
- Breath
- Heartbeat

Chronology is continuous. No abrupt transitions occur at midnight or on calendar
boundaries.

## Part V - Biometrics

Biometrics provide adaptive feedback.

Primary inputs:
- Heart rate
- HRV
- Movement
- Respiratory rate
- Stress estimate
- Recovery estimate

These values are normalized relative to each participant's personal baseline.
The engine adapts cautiously over minutes rather than reacting to individual
samples.

## Part VI - Symbolic Context

Nocturne supports optional symbolic systems:
- Chakra frameworks
- Astrology
- Seasonal rituals
- Family synastry

These are interpreted through adapters that translate symbolic information into
semantic biases. The renderer remains agnostic to any specific tradition.

## Part VII - Learning

The platform contains three learning layers:
1. Embedded inference
2. Offline Android personalization
3. Optional cloud and federated learning

Learning adjusts acoustic policies rather than directly generating audio.

## Part VIII - Privacy

Design principles include:
- Local-first processing
- Explicit consent
- Fine-grained data sharing
- Encryption
- Explainability
- Model portability
- Right to delete
- Offline functionality

Sensitive personal information remains on trusted devices whenever possible.

## Part IX - Roadmap

Volume I establishes the architecture. Subsequent volumes specify:
- DSP engine
- Sample pack compiler
- ESP-IDF implementation
- Android SDK
- Cloud APIs
- Machine-learning pipeline
- Security model
- Developer SDK
- Reference implementation
- Validation methodology

## Closing Principle

Observed data informs adaptation. Computed data provides temporal context.
Symbolic data provides personal meaning. The Context Fusion Engine transforms them
into a coherent Acoustic Intent. The renderer transforms Acoustic Intent into
sound.
