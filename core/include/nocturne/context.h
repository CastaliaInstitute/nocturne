#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NOCTURNE_INTENT_DIMENSIONS 10
#define NOCTURNE_INTENT_SCHEMA_VERSION 1

typedef enum {
    NOCTURNE_CONTEXT_OBSERVED = 0,
    NOCTURNE_CONTEXT_COMPUTED = 1,
    NOCTURNE_CONTEXT_SYMBOLIC = 2,
    NOCTURNE_CONTEXT_USER = 3
} nocturne_context_domain_t;

typedef struct {
    float value;
    float confidence;
    float min_bound;
    float max_bound;
    float provenance_weight;
} nocturne_influence_t;

typedef struct {
    float grounding;
    float brightness;
    float warmth;
    float density;
    float motion;
    float spaciousness;
    float silence;
    float event_activity;
    float harmonic_tension;
    float novelty;
} nocturne_acoustic_intent_t;

typedef struct {
    uint32_t schema_version;
    uint64_t seed;
    uint64_t session_id;
    uint64_t monotonic_ms;
    nocturne_context_domain_t source_domain;
    nocturne_influence_t grounding;
    nocturne_influence_t brightness;
    nocturne_influence_t warmth;
    nocturne_influence_t density;
    nocturne_influence_t motion;
    nocturne_influence_t spaciousness;
    nocturne_influence_t silence;
    nocturne_influence_t event_activity;
    nocturne_influence_t harmonic_tension;
    nocturne_influence_t novelty;
} nocturne_context_contribution_t;

typedef struct {
    nocturne_acoustic_intent_t intent;
    nocturne_influence_t influences[NOCTURNE_INTENT_DIMENSIONS];
    uint32_t valid_inputs;
    uint32_t domain_count;
    float confidence_cap;
    float smoothing_alpha;
    float last_update_ms;
} nocturne_fused_frame_t;

static inline float nocturne_clamp_float(float value, float min_value, float max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static inline float nocturne_smooth(float previous, float current, float alpha) {
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    return (1.0f - alpha) * previous + alpha * current;
}

static inline bool nocturne_domain_ok(nocturne_context_domain_t domain) {
    return domain == NOCTURNE_CONTEXT_OBSERVED
        || domain == NOCTURNE_CONTEXT_COMPUTED
        || domain == NOCTURNE_CONTEXT_SYMBOLIC
        || domain == NOCTURNE_CONTEXT_USER;
}

#ifdef __cplusplus
}
#endif
