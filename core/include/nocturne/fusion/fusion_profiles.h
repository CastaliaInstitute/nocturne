#pragma once

#include <string.h>
#include "fusion.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NOCTURNE_FUSION_PROFILE_SLEEP = 0,
    NOCTURNE_FUSION_PROFILE_FOCUS = 1,
    NOCTURNE_FUSION_PROFILE_RITUAL = 2,
} nocturne_fusion_profile_preset_t;

static inline nocturne_fusion_profile_t nocturne_fusion_profile_for(nocturne_fusion_profile_preset_t preset) {
    nocturne_fusion_profile_t out = {
        .anti_dominance_cap = 0.35f,
        .confidence_floor = 0.2f,
        .confidence_ceil = 0.95f,
        .trust_decay_per_cycle = 0.95f,
    };

    memset(out.weights, 0, sizeof(out.weights));

    if (preset == NOCTURNE_FUSION_PROFILE_SLEEP) {
        out.anti_dominance_cap = 0.22f;
        out.confidence_floor = 0.15f;
        out.trust_decay_per_cycle = 0.9f;
    } else if (preset == NOCTURNE_FUSION_PROFILE_FOCUS) {
        out.anti_dominance_cap = 0.45f;
        out.confidence_floor = 0.4f;
        out.trust_decay_per_cycle = 0.98f;
    } else if (preset == NOCTURNE_FUSION_PROFILE_RITUAL) {
        out.anti_dominance_cap = 0.30f;
        out.confidence_floor = 0.3f;
        out.trust_decay_per_cycle = 0.93f;
    }

    for (int i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        out.weights[i].value = 1.0f / (float)NOCTURNE_INTENT_DIMENSIONS;
        out.weights[i].confidence = 0.9f;
        out.weights[i].min_bound = 0.0f;
        out.weights[i].max_bound = 1.0f;
        out.weights[i].provenance_weight = 1.0f;
    }

    return out;
}

#ifdef __cplusplus
}
#endif
