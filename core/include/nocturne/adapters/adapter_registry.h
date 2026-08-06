#pragma once

#include <stdint.h>
#include <stddef.h>
#include "sensor_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NOCTURNE_SENSOR_REGISTRY_CAP 16

typedef struct {
    nocturne_sensor_adapter_t adapters[NOCTURNE_SENSOR_REGISTRY_CAP];
    size_t count;
} nocturne_sensor_registry_t;

static inline void nocturne_sensor_registry_init(nocturne_sensor_registry_t* registry) {
    if (!registry) return;
    registry->count = 0;
}

static inline bool nocturne_sensor_registry_add(nocturne_sensor_registry_t* registry, nocturne_sensor_adapter_t adapter) {
    if (!registry || registry->count >= NOCTURNE_SENSOR_REGISTRY_CAP) {
        return false;
    }
    registry->adapters[registry->count++] = adapter;
    return true;
}

static inline bool nocturne_sensor_read_all(nocturne_sensor_registry_t* registry, nocturne_sensor_sample_t* samples, size_t max_samples, size_t* out_count) {
    if (!registry || !samples || !out_count) {
        return false;
    }

    size_t wrote = 0;
    for (size_t i = 0; i < registry->count && wrote < max_samples; ++i) {
        if (nocturne_sensor_adapter_read(&registry->adapters[i], &samples[wrote])) {
            wrote += 1;
        }
    }
    *out_count = wrote;
    return true;
}

static inline bool nocturne_sensor_sample_is_valid(const nocturne_sensor_sample_t* sample) {
    if (!sample) return false;
    if (sample->value < 0.0f) return false;
    if (sample->confidence < 0.0f || sample->confidence > 1.0f) return false;
    if (sample->ttl_ms == 0) return false;
    if (!nocturne_domain_ok(sample->source_domain)) return false;
    return true;
}

#ifdef __cplusplus
}
#endif
