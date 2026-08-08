#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "../context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NOCTURNE_SENSOR_HEART_RATE = 0,
    NOCTURNE_SENSOR_HRV = 1,
    NOCTURNE_SENSOR_MOTION = 2,
    NOCTURNE_SENSOR_BREATHING = 3,
    NOCTURNE_SENSOR_AMBIENT_LIGHT = 4,
    NOCTURNE_SENSOR_TEMPERATURE = 5,
} nocturne_sensor_kind_t;

typedef struct {
    nocturne_sensor_kind_t kind;
    float value;
    float confidence;
    uint64_t sample_ms;
    uint64_t ttl_ms;
    bool stale;
    nocturne_context_domain_t source_domain;
} nocturne_sensor_sample_t;

typedef bool (*nocturne_sensor_read_fn)(void* adapter_state, nocturne_sensor_sample_t* out);
typedef bool (*nocturne_sensor_reset_fn)(void* adapter_state);
typedef void (*nocturne_sensor_destroy_fn)(void* adapter_state);

typedef struct {
    const char* name;
    nocturne_sensor_read_fn read;
    nocturne_sensor_reset_fn reset;
    nocturne_sensor_destroy_fn destroy;
} nocturne_sensor_adapter_vtable_t;

typedef struct {
    nocturne_sensor_kind_t kind;
    nocturne_sensor_adapter_vtable_t vtable;
    void* state;
} nocturne_sensor_adapter_t;

static inline bool nocturne_sensor_adapter_read(nocturne_sensor_adapter_t* adapter, nocturne_sensor_sample_t* out) {
    if (!adapter || !adapter->state || !out || !adapter->vtable.read) {
        return false;
    }
    return adapter->vtable.read(adapter->state, out);
}

static inline bool nocturne_sensor_adapter_reset(nocturne_sensor_adapter_t* adapter) {
    if (!adapter || !adapter->state || !adapter->vtable.reset) {
        return false;
    }
    return adapter->vtable.reset(adapter->state);
}

static inline void nocturne_sensor_adapter_destroy(nocturne_sensor_adapter_t* adapter) {
    if (!adapter || !adapter->state || !adapter->vtable.destroy) {
        return;
    }
    adapter->vtable.destroy(adapter->state);
    adapter->state = NULL;
}

#ifdef __cplusplus
}
#endif
