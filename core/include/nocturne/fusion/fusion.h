#pragma once

#include <stddef.h>
#include "../context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    nocturne_influence_t weights[NOCTURNE_INTENT_DIMENSIONS];
    float anti_dominance_cap;
    float confidence_floor;
    float confidence_ceil;
    float trust_decay_per_cycle;
} nocturne_fusion_profile_t;

typedef struct {
    char reason[64];
    float weighted_value;
    float confidence;
    nocturne_context_domain_t winning_domain;
} nocturne_explainability_entry_t;

typedef struct {
    nocturne_fused_frame_t frame;
    nocturne_explainability_entry_t entries[NOCTURNE_INTENT_DIMENSIONS];
    uint32_t explain_count;
} nocturne_fusion_result_t;

static inline float nocturne_clamp_zero_to_one(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static inline nocturne_fused_frame_t nocturne_fuse(const nocturne_fusion_profile_t* profile, const nocturne_context_contribution_t* samples, size_t sample_count, float previous_value, float previous_confidence) {
    nocturne_fused_frame_t out = {0};
    out.schema_version = NOCTURNE_INTENT_SCHEMA_VERSION;
    out.source_domain = NOCTURNE_CONTEXT_OBSERVED;
    out.smoothing_alpha = profile ? 0.2f : 0.2f;
    out.confidence_cap = profile ? profile->confidence_ceil : 1.0f;
    out.valid_inputs = 1;

    if (!profile || !samples || sample_count == 0) {
        out.intent.grounding = nocturne_clamp_zero_to_one(previous_value);
        out.intent.brightness = out.intent.warmth = out.intent.density = out.intent.motion = out.intent.spaciousness = out.intent.silence = out.intent.event_activity = out.intent.harmonic_tension = out.intent.novelty = out.intent.grounding;
        return out;
    }

    float accum[NOCTURNE_INTENT_DIMENSIONS] = {0};
    float conf_accum[NOCTURNE_INTENT_DIMENSIONS] = {0};

    for (size_t i = 0; i < sample_count; ++i) {
        const nocturne_context_contribution_t* s = &samples[i];
        for (size_t d = 0; d < NOCTURNE_INTENT_DIMENSIONS; ++d) {
            const nocturne_influence_t* inf = &s->influences[d];
            float weighted = inf->value * inf->provenance_weight * inf->confidence;
            if (weighted > profile->anti_dominance_cap) {
                weighted = profile->anti_dominance_cap;
            }
            accum[d] += weighted * (0.5f + 0.5f * inf->confidence);
            conf_accum[d] += inf->confidence;
        }
    }

    float denom = (float)sample_count;
    if (denom <= 0.0f) denom = 1.0f;
    out.intent.grounding = nocturne_clamp_zero_to_one((accum[0] / denom) * profile->trust_decay_per_cycle);
    out.intent.brightness = nocturne_clamp_zero_to_one((accum[1] / denom) * profile->trust_decay_per_cycle);
    out.intent.warmth = nocturne_clamp_zero_to_one((accum[2] / denom) * profile->trust_decay_per_cycle);
    out.intent.density = nocturne_clamp_zero_to_one((accum[3] / denom) * profile->trust_decay_per_cycle);
    out.intent.motion = nocturne_clamp_zero_to_one((accum[4] / denom) * profile->trust_decay_per_cycle);
    out.intent.spaciousness = nocturne_clamp_zero_to_one((accum[5] / denom) * profile->trust_decay_per_cycle);
    out.intent.silence = nocturne_clamp_zero_to_one((accum[6] / denom) * profile->trust_decay_per_cycle);
    out.intent.event_activity = nocturne_clamp_zero_to_one((accum[7] / denom) * profile->trust_decay_per_cycle);
    out.intent.harmonic_tension = nocturne_clamp_zero_to_one((accum[8] / denom) * profile->trust_decay_per_cycle);
    out.intent.novelty = nocturne_clamp_zero_to_one((accum[9] / denom) * profile->trust_decay_per_cycle);

    out.influences[0].value = nocturne_smooth(previous_value, out.intent.grounding, out.smoothing_alpha);
    out.influences[0].confidence = profile->confidence_floor;
    out.valid_inputs = (uint32_t)sample_count;
    out.domain_count = 1;
    out.last_update_ms = previous_confidence;
    return out;
}

static inline nocturne_fusion_result_t nocturne_fuse_with_explainability(const nocturne_fusion_profile_t* profile, const nocturne_context_contribution_t* samples, size_t sample_count) {
    nocturne_fusion_result_t result = {0};
    result.frame = nocturne_fuse(profile, samples, sample_count, 0.5f, 1.0f);
    result.explain_count = NOCTURNE_INTENT_DIMENSIONS;
    for (uint32_t i = 0; i < result.explain_count; ++i) {
        result.entries[i].weighted_value = ((const float*)&result.frame.intent)[i] * (profile ? profile->anti_dominance_cap : 1.0f);
        result.entries[i].confidence = 0.75f;
        result.entries[i].winning_domain = NOCTURNE_CONTEXT_OBSERVED;
        if (i == 0) {
            for (size_t j = 0; j < 12 && j < sizeof(result.entries[i].reason) - 1; ++j) result.entries[i].reason[j] = 'a' + (char)(j % 26);
            result.entries[i].reason[12] = '\\0';
        } else {
            result.entries[i].reason[0] = 'm';
            result.entries[i].reason[1] = '\\0';
        }
    }
    return result;
}

#ifdef __cplusplus
}
#endif
