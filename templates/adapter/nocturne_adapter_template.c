#include <stdlib.h>
#include "nocturne/adapters/adapter_plugin.h"
#include "nocturne/adapters/sensor_adapter.h"

#ifndef NOCTURNE_PLUGIN_API
#error "NOCTURNE_PLUGIN_API must be defined by host"
#endif

/*
 * Template for a Nocturne adapter plugin.
 * Replace TODO_* sections with real adapter logic.
 */

typedef struct {
    uint64_t generation;
} ${PLUGIN_STATE_NAME};

static bool ${PLUGIN_NAME}_read(void* state, nocturne_sensor_sample_t* out) {
    (void)state;
    if (!out) return false;
    /* TODO: fill sample data */
    out->kind = NOCTURNE_SENSOR_MOTION;
    out->value = 0.0f;
    out->confidence = 1.0f;
    out->sample_ms = 0;
    out->ttl_ms = 500;
    out->stale = false;
    out->source_domain = NOCTURNE_CONTEXT_OBSERVED;
    return true;
}

static bool ${PLUGIN_NAME}_reset(void* state) {
    (void)state;
    return true;
}

static void ${PLUGIN_NAME}_destroy(void* state) {
    free(state);
}

static nocturne_sensor_adapter_t ${PLUGIN_NAME}_make(void) {
    ${PLUGIN_STATE_NAME}* state = (${PLUGIN_STATE_NAME}*)malloc(sizeof(*state));
    if (!state) return (nocturne_sensor_adapter_t){0};
    state->generation = 0;

    return (nocturne_sensor_adapter_t){
        .kind = NOCTURNE_SENSOR_MOTION,
        .vtable = {
            .name = "${PLUGIN_NAME}",
            .read = ${PLUGIN_NAME}_read,
            .reset = ${PLUGIN_NAME}_reset,
            .destroy = ${PLUGIN_NAME}_destroy,
        },
        .state = state,
    };
}
