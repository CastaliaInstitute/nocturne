#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NOCTURNE_SDK_VERSION_MAJOR 1
#define NOCTURNE_SDK_VERSION_MINOR 0
#define NOCTURNE_SDK_VERSION_PATCH 0
#define NOCTURNE_SDK_VERSION_STRING "1.0.0"

typedef uint32_t nocturne_sdk_version_t;

typedef enum {
    NOCTURNE_SDK_OK = 0,
    NOCTURNE_SDK_ERR_INVALID_ARG = 1,
    NOCTURNE_SDK_ERR_UNSUPPORTED = 2,
    NOCTURNE_SDK_ERR_INTERNAL = 3,
} nocturne_sdk_status_t;

typedef struct {
    nocturne_sdk_version_t major;
    nocturne_sdk_version_t minor;
    nocturne_sdk_version_t patch;
    const char* semver;
} nocturne_sdk_version_info_t;

static inline nocturne_sdk_version_info_t nocturne_sdk_version_info(void) {
    return (nocturne_sdk_version_info_t){
        NOCTURNE_SDK_VERSION_MAJOR,
        NOCTURNE_SDK_VERSION_MINOR,
        NOCTURNE_SDK_VERSION_PATCH,
        NOCTURNE_SDK_VERSION_STRING,
    };
}

static inline bool nocturne_sdk_api_compatible(uint32_t major, uint32_t minor) {
    (void)minor;
    return major == NOCTURNE_SDK_VERSION_MAJOR;
}

#ifdef __cplusplus
}
#endif
