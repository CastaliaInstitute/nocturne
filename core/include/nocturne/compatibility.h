#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NOCTURNE_SCHEMA_VERSION_CURRENT 1

typedef enum {
    NOCTURNE_COMPATIBLE = 0,
    NOCTURNE_NEEDS_MIGRATION = 1,
    NOCTURNE_INCOMPATIBLE = 2
} nocturne_compat_result_t;

static inline nocturne_compat_result_t nocturne_compat_check_schema(uint32_t schema_version) {
    if (schema_version == NOCTURNE_SCHEMA_VERSION_CURRENT) {
        return NOCTURNE_COMPATIBLE;
    }
    if (schema_version < NOCTURNE_SCHEMA_VERSION_CURRENT) {
        return NOCTURNE_NEEDS_MIGRATION;
    }
    return NOCTURNE_INCOMPATIBLE;
}

#ifdef __cplusplus
}
#endif
