#pragma once

#include <stdbool.h>
#include "../context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NOCTURNE_MODE_SLEEP = 0,
    NOCTURNE_MODE_MEDITATION = 1,
    NOCTURNE_MODE_FOCUS = 2,
    NOCTURNE_MODE_RITUAL = 3,
    NOCTURNE_MODE_UNKNOWN = 255,
} nocturne_mode_t;

typedef struct {
    nocturne_mode_t mode;
    float user_brightness_bias;
    float user_warmth_bias;
    float user_density_cap;
    bool sleep_safe_mode;
    bool parental_controls_enabled;
} nocturne_user_controls_t;

static inline nocturne_context_contribution_t nocturne_build_user_context(const nocturne_user_controls_t* controls) {
    nocturne_context_contribution_t out = {0};
    out.schema_version = NOCTURNE_INTENT_SCHEMA_VERSION;
    out.source_domain = NOCTURNE_CONTEXT_USER;
    if (!controls) {
        return out;
    }

    out.brightness.value = controls->user_brightness_bias;
    out.density.value = controls->user_density_cap;
    out.warmth.value = controls->user_warmth_bias;
    if (controls->parental_controls_enabled) {
        out.silence.value = 0.2f;
    }
    return out;
}

#ifdef __cplusplus
}
#endif
