#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../scheduler/scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NOCTURNE_PLUGIN_KIND_SENSOR = 0,
    NOCTURNE_PLUGIN_KIND_COMPUTED = 1,
    NOCTURNE_PLUGIN_KIND_SYMBOLIC = 2,
    NOCTURNE_PLUGIN_KIND_USER = 3,
} nocturne_plugin_kind_t;

typedef struct {
    uint32_t api_version;
    const char* plugin_name;
    const char* plugin_vendor;
    nocturne_plugin_kind_t kind;
    uint32_t capability_mask;
} nocturne_adapter_plugin_manifest_t;

typedef void (*nocturne_plugin_destroy_f)(void* plugin_state);

typedef struct {
    const nocturne_adapter_plugin_manifest_t* manifest;
    void* state;
    void (*destroy)(void* plugin_state);
} nocturne_adapter_plugin_t;

static inline bool nocturne_adapter_plugin_is_supported(
    const nocturne_adapter_plugin_t* plugin,
    uint32_t expected_api_version
) {
    if (!plugin || !plugin->manifest) {
        return false;
    }
    if (plugin->manifest->api_version != expected_api_version) {
        return false;
    }
    return plugin->manifest->plugin_name != NULL && plugin->manifest->plugin_name[0] != '\0';
}

static inline void nocturne_adapter_plugin_release(nocturne_adapter_plugin_t* plugin) {
    if (!plugin || !plugin->destroy || !plugin->state) {
        return;
    }
    plugin->destroy(plugin->state);
    plugin->state = NULL;
    plugin->destroy = NULL;
    plugin->manifest = NULL;
}

#ifdef __cplusplus
}
#endif
