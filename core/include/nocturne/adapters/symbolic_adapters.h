#pragma once

#include <string.h>
#include "../context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NOCTURNE_SYMBOLIC_NONE = 0,
    NOCTURNE_SYMBOLIC_CHAKRA = 1,
    NOCTURNE_SYMBOLIC_ASTROLOGY = 2,
    NOCTURNE_SYMBOLIC_RITUAL = 3,
    NOCTURNE_SYMBOLIC_MEDITATION_INTENT = 4,
} nocturne_symbolic_domain_t;

typedef struct {
    nocturne_symbolic_domain_t kind;
    const char* label;
    float weight;
    float confidence;
} nocturne_symbolic_bias_t;

static inline nocturne_symbolic_bias_t nocturne_symbolic_bias_for(const char* token, nocturne_symbolic_domain_t kind) {
    nocturne_symbolic_bias_t out = {kind, token, 0.3f, 0.75f};
    if (!token) {
        return (nocturne_symbolic_bias_t){NOCTURNE_SYMBOLIC_NONE, NULL, 0.0f, 0.0f};
    }

    if (kind == NOCTURNE_SYMBOLIC_CHAKRA && strncmp(token, "root", 4) == 0) {
        out.weight = 0.75f;
    } else if (kind == NOCTURNE_SYMBOLIC_ASTROLOGY && strncmp(token, "moon", 4) == 0) {
        out.weight = 0.55f;
    } else if (kind == NOCTURNE_SYMBOLIC_RITUAL && strncmp(token, "evening", 7) == 0) {
        out.weight = 0.60f;
    } else if (kind == NOCTURNE_SYMBOLIC_MEDITATION_INTENT && strncmp(token, "focus", 5) == 0) {
        out.weight = 0.70f;
    }
    return out;
}

static inline nocturne_context_contribution_t nocturne_build_symbolic_context(nocturne_symbolic_bias_t bias) {
    nocturne_context_contribution_t out = {0};
    out.schema_version = NOCTURNE_INTENT_SCHEMA_VERSION;
    out.source_domain = NOCTURNE_CONTEXT_SYMBOLIC;
    out.confidence_cap = bias.confidence;
    out.smoothing_alpha = 0.1f;
    out.grounding.value = bias.weight;
    return out;
}

#ifdef __cplusplus
}
#endif
