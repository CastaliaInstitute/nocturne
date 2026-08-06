#pragma once

#include <math.h>
#include <stdlib.h>
#include "sensor_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t sequence;
    float baseline;
    float amplitude;
    float noise_seed;
    nocturne_sensor_kind_t kind;
} nocturne_simulated_sensor_state_t;

static inline float nocturne_simulated_base_value(nocturne_simulated_sensor_state_t* state) {
    if (!state) {
        return 0.0f;
    }
    float wave = (float)sin(((double)state->sequence / 13.0f) + state->noise_seed);
    return state->baseline + (state->amplitude * wave);
}

static inline bool nocturne_simulated_sensor_read(void* adapter_state, nocturne_sensor_sample_t* out) {
    nocturne_simulated_sensor_state_t* s = (nocturne_simulated_sensor_state_t*)adapter_state;
    if (!s || !out) return false;

    out->kind = s->kind;
    out->value = nocturne_simulated_base_value(s);
    out->confidence = 0.85f;
    out->sample_ms = s->sequence * 100u;
    out->ttl_ms = 500u;
    out->stale = false;
    out->source_domain = NOCTURNE_CONTEXT_OBSERVED;

    s->sequence += 1;
    if (s->sequence > 1000000u) {
        s->sequence = 0;
    }
    return true;
}

static inline bool nocturne_simulated_sensor_reset(void* adapter_state) {
    nocturne_simulated_sensor_state_t* s = (nocturne_simulated_sensor_state_t*)adapter_state;
    if (!s) return false;
    s->sequence = 0;
    return true;
}

static inline void nocturne_simulated_sensor_destroy(void* adapter_state) {
    free(adapter_state);
}

static inline nocturne_sensor_adapter_t nocturne_make_simulated_sensor(nocturne_sensor_kind_t kind) {
    nocturne_simulated_sensor_state_t* state = (nocturne_simulated_sensor_state_t*)malloc(sizeof(nocturne_simulated_sensor_state_t));
    if (!state) {
        return (nocturne_sensor_adapter_t){0};
    }
    state->sequence = 0;
    state->kind = kind;
    state->noise_seed = (float)(kind * 0.47f);

    switch (kind) {
        case NOCTURNE_SENSOR_HEART_RATE:
            state->baseline = 70.0f;
            state->amplitude = 8.0f;
            break;
        case NOCTURNE_SENSOR_HRV:
            state->baseline = 40.0f;
            state->amplitude = 6.0f;
            break;
        case NOCTURNE_SENSOR_MOTION:
            state->baseline = 0.25f;
            state->amplitude = 0.5f;
            break;
        case NOCTURNE_SENSOR_BREATHING:
            state->baseline = 0.35f;
            state->amplitude = 0.3f;
            break;
        case NOCTURNE_SENSOR_AMBIENT_LIGHT:
            state->baseline = 150.0f;
            state->amplitude = 80.0f;
            break;
        case NOCTURNE_SENSOR_TEMPERATURE:
            state->baseline = 22.0f;
            state->amplitude = 3.0f;
            break;
        default:
            state->baseline = 0.0f;
            state->amplitude = 0.0f;
            break;
    }

    return (nocturne_sensor_adapter_t){
        .kind = kind,
        .vtable = {
            .name = "simulated-sensor",
            .read = nocturne_simulated_sensor_read,
            .reset = nocturne_simulated_sensor_reset,
            .destroy = nocturne_simulated_sensor_destroy,
        },
        .state = state
    };
}

#ifdef __cplusplus
}
#endif
