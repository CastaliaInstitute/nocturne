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
    bool parental_controls_enabled;
    bool sleep_safe_mode;
    float max_novelty;
    float max_brightness;
    float max_event_activity;
    float max_harmonic_tension;
    float max_density_when_sleep_safe;
    bool block_conflicting_signals;
} nocturne_safety_policy_t;

typedef struct {
    char key[32];
    float before;
    float after;
    const char* constraint;
} nocturne_safety_adjustment_t;

typedef struct {
    nocturne_safety_adjustment_t adjustments[NOCTURNE_INTENT_DIMENSIONS];
    uint32_t count;
} nocturne_safety_result_t;

static inline void nocturne_safety_policy_init_default(nocturne_safety_policy_t* policy) {
    if (!policy) return;
    policy->parental_controls_enabled = false;
    policy->sleep_safe_mode = false;
    policy->max_novelty = 0.90f;
    policy->max_brightness = 1.0f;
    policy->max_event_activity = 0.85f;
    policy->max_harmonic_tension = 0.85f;
    policy->max_density_when_sleep_safe = 0.45f;
    policy->block_conflicting_signals = true;
}

static inline void nocturne_apply_cap(nocturne_safety_result_t* out, const nocturne_fused_frame_t* frame, size_t index, float* field, float limit, const char* reason) {
    (void)frame;
    if (!out || !field || !reason) {
        return;
    }
    if (out->count >= NOCTURNE_INTENT_DIMENSIONS) {
        return;
    }
    if (*field > limit) {
        out->adjustments[out->count].before = *field;
        out->adjustments[out->count].after = limit;
        out->adjustments[out->count].constraint = reason;
        snprintf(out->adjustments[out->count].key, sizeof(out->adjustments[out->count].key), "dim_%zu", index);
        out->count += 1;
        *field = limit;
    }
}

static inline nocturne_safety_result_t nocturne_apply_safety_policy(nocturne_fused_frame_t* frame, const nocturne_safety_policy_t* policy, const nocturne_fusion_conflict_report_t* conflict) {
    nocturne_safety_result_t out;
    memset(&out, 0, sizeof(out));
    if (!frame || !policy) {
        return out;
    }

    float* novelty = &frame->intent.novelty;
    float* brightness = &frame->intent.brightness;
    float* event_activity = &frame->intent.event_activity;
    float* harmonic_tension = &frame->intent.harmonic_tension;
    float* density = &frame->intent.density;

    nocturne_apply_cap(&out, frame, 9, novelty, policy->max_novelty, "novelty cap");
    nocturne_apply_cap(&out, frame, 1, brightness, policy->max_brightness, "brightness cap");
    nocturne_apply_cap(&out, frame, 7, event_activity, policy->max_event_activity, "event cap");
    nocturne_apply_cap(&out, frame, 8, harmonic_tension, policy->max_harmonic_tension, "harmonic tension cap");

    if (policy->sleep_safe_mode) {
        nocturne_apply_cap(&out, frame, 3, density, policy->max_density_when_sleep_safe, "sleep-safe density cap");
    }

    if (policy->parental_controls_enabled) {
        nocturne_apply_cap(&out, frame, 0, novelty, 0.45f, "parental novelty limit");
        nocturne_apply_cap(&out, frame, 7, event_activity, 0.30f, "parental event limit");
    }

    if (policy->block_conflicting_signals && conflict && conflict->conflict_detected) {
        if (out.count < NOCTURNE_INTENT_DIMENSIONS) {
            out.adjustments[out.count].before = conflict->dominant_signal;
            out.adjustments[out.count].after = conflict->runnerup_signal;
            out.adjustments[out.count].constraint = "conflict mitigation";
            snprintf(out.adjustments[out.count].key, sizeof(out.adjustments[out.count].key), "conflict_%zu", conflict->dimension);
            out.count += 1;
        }
    }

    return out;
}

#ifdef __cplusplus
}
#endif
