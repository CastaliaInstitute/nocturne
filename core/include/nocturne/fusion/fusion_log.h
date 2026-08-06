#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "fusion.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    nocturne_fused_frame_t frame;
    uint32_t frame_id;
    char explanation[64];
} nocturne_fusion_log_entry_t;

typedef struct {
    nocturne_fusion_log_entry_t entries[32];
    uint32_t count;
    bool telemetry_enabled;
} nocturne_fusion_log_t;

static inline void nocturne_fusion_log_init(nocturne_fusion_log_t* log, bool telemetry_enabled) {
    if (!log) return;
    log->count = 0;
    log->telemetry_enabled = telemetry_enabled;
}

static inline bool nocturne_fusion_log_append(nocturne_fusion_log_t* log, const nocturne_fusion_result_t* result) {
    if (!log || !result || !log->telemetry_enabled) {
        return false;
    }
    uint32_t idx = log->count % 32;
    log->entries[idx].frame = result->frame;
    log->entries[idx].frame_id = log->count;
    if (result->conflict.conflict_detected) {
        (void)snprintf(
            log->entries[idx].explanation,
            sizeof(log->entries[idx].explanation),
            "conflict dim=%zu winner=%u runnerup=%u margin=%.3f",
            result->conflict.dimension,
            (unsigned)result->conflict.winning_domain,
            (unsigned)result->conflict.runnerup_domain,
            result->conflict.conflict_margin
        );
    } else {
        log->entries[idx].explanation[0] = '\0';
        (void)strncpy(log->entries[idx].explanation, "no conflict", sizeof(log->entries[idx].explanation) - 1);
        log->entries[idx].explanation[sizeof(log->entries[idx].explanation) - 1] = '\0';
    }
    log->count += 1;
    return true;
}

static inline bool nocturne_fusion_log_replay(const nocturne_fusion_log_t* log, uint32_t index, nocturne_fused_frame_t* out_frame) {
    if (!log || !out_frame || log->count == 0 || index >= log->count) {
        return false;
    }
    uint32_t idx = index % 32;
    *out_frame = log->entries[idx].frame;
    return true;
}

#ifdef __cplusplus
}
#endif
