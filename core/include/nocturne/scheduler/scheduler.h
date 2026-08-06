#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "../context.h"
#include "../fusion/fusion.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NOCTURNE_SCHEDULER_MODE_UNKNOWN = 0,
    NOCTURNE_SCHEDULER_MODE_SLEEP = 1,
    NOCTURNE_SCHEDULER_MODE_FOCUS = 2,
    NOCTURNE_SCHEDULER_MODE_RITUAL = 3,
} nocturne_scheduler_mode_t;

typedef struct {
    float max_event_density;
    float max_loudness;
    float max_high_frequency_ratio;
    float max_novelty_gain;
    float continuity_rate;
    float continuity_hold_ms;
    float surprise_reduction_sleep;
    float safety_surprise_cap;
    float interruption_decay;
    float timer_default_ms;
    float timer_max_ms;
} nocturne_scheduler_profile_t;

typedef struct {
    nocturne_scheduler_mode_t mode;
    float target_event_density;
    float target_loudness;
    float target_high_frequency_content;
    float target_novelty_gain;
    float target_surprise;
    float continuity;
    float timer_seconds;
    float safety_margin;
    bool fallback_used;
    bool in_interruption_hold;
    uint8_t reason_code;
    char reason[64];
} nocturne_scheduler_plan_t;

typedef struct {
    nocturne_scheduler_profile_t profile;
    nocturne_scheduler_mode_t mode;
    uint32_t phase_id;
    uint64_t phase_started_ms;
    uint64_t phase_length_ms;
    float smoothed_event_density;
    float smoothed_loudness;
    float smoothed_high_frequency_content;
    float smoothed_novelty_gain;
    float smoothed_surprise;
    uint64_t monotonic_ms;
    uint64_t cycle;
    uint64_t interruption_until_ms;
    uint32_t interrupts;
    float phase_progress;
} nocturne_scheduler_state_t;

typedef struct {
    uint64_t monotonic_ms;
    nocturne_scheduler_plan_t plan;
    nocturne_scheduler_mode_t source_mode;
    float trace_signature;
    uint32_t entry_id;
} nocturne_scheduler_trace_entry_t;

typedef struct {
    nocturne_scheduler_trace_entry_t entries[48];
    uint32_t count;
    bool enabled;
} nocturne_scheduler_trace_t;

static inline float nocturne_scheduler_clamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static inline nocturne_scheduler_profile_t nocturne_scheduler_profile_for(nocturne_scheduler_mode_t mode) {
    nocturne_scheduler_profile_t out;
    memset(&out, 0, sizeof(out));
    out.max_event_density = 0.60f;
    out.max_loudness = 0.55f;
    out.max_high_frequency_ratio = 0.30f;
    out.max_novelty_gain = 0.45f;
    out.continuity_rate = 0.85f;
    out.continuity_hold_ms = 450.0f;
    out.surprise_reduction_sleep = 0.45f;
    out.safety_surprise_cap = 0.35f;
    out.interruption_decay = 0.75f;
    out.timer_default_ms = 900.0f;
    out.timer_max_ms = 1800.0f;

    if (mode == NOCTURNE_SCHEDULER_MODE_FOCUS) {
        out.max_event_density = 0.75f;
        out.max_loudness = 0.70f;
        out.max_high_frequency_ratio = 0.24f;
        out.max_novelty_gain = 0.30f;
    } else if (mode == NOCTURNE_SCHEDULER_MODE_RITUAL) {
        out.max_event_density = 0.45f;
        out.max_loudness = 0.40f;
        out.max_high_frequency_ratio = 0.20f;
        out.max_novelty_gain = 0.20f;
        out.timer_default_ms = 1200.0f;
    } else if (mode == NOCTURNE_SCHEDULER_MODE_SLEEP) {
        out.max_event_density = 0.30f;
        out.max_loudness = 0.30f;
        out.max_high_frequency_ratio = 0.12f;
        out.max_novelty_gain = 0.12f;
        out.surprise_reduction_sleep = 0.25f;
        out.safety_surprise_cap = 0.15f;
        out.timer_default_ms = 600.0f;
    }

    return out;
}

static inline bool nocturne_scheduler_frame_is_valid(const nocturne_fused_frame_t* frame) {
    if (!frame) {
        return false;
    }
    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        const float value = ((const float*)&frame->intent)[i];
        if (!isfinite(value)) {
            return false;
        }
        if (value < 0.0f || value > 1.0f) {
            return false;
        }
    }

    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        const nocturne_influence_t* influence = &frame->influences[i];
        if (!isfinite(influence->value)
            || !isfinite(influence->confidence)
            || !isfinite(influence->provenance_weight)
            || !isfinite(influence->min_bound)
            || !isfinite(influence->max_bound)) {
            return false;
        }
        if (influence->value < 0.0f || influence->value > 1.0f) {
            return false;
        }
        if (influence->confidence < 0.0f || influence->confidence > 1.0f) {
            return false;
        }
        if (influence->provenance_weight < 0.0f || influence->provenance_weight > 1.0f) {
            return false;
        }
        if (influence->min_bound > influence->max_bound
            || influence->min_bound < 0.0f
            || influence->max_bound > 1.0f) {
            return false;
        }
    }

    return true;
}

static inline nocturne_scheduler_state_t nocturne_scheduler_state_init(nocturne_scheduler_mode_t mode, const nocturne_fused_frame_t* seed_frame) {
    nocturne_scheduler_state_t state;
    memset(&state, 0, sizeof(state));
    state.mode = mode;
    state.profile = nocturne_scheduler_profile_for(mode);
    state.phase_id = 0;
    state.phase_started_ms = 0;
    state.phase_length_ms = 0;
    state.phase_progress = 0.0f;
    state.interruption_until_ms = 0;
    if (seed_frame) {
        state.smoothed_event_density = seed_frame->intent.event_activity;
        state.smoothed_loudness = 1.0f - seed_frame->intent.silence;
        state.smoothed_high_frequency_content = seed_frame->intent.harmonic_tension;
        state.smoothed_novelty_gain = seed_frame->intent.novelty;
        state.smoothed_surprise = (seed_frame->intent.event_activity + seed_frame->intent.harmonic_tension) * 0.5f;
        state.monotonic_ms = seed_frame->last_update_ms;
    }
    return state;
}

static inline float nocturne_scheduler_signature(const nocturne_scheduler_plan_t* plan) {
    if (!plan) return 0.0f;
    return nocturne_scheduler_clamp01(
        (plan->target_event_density
         + plan->target_loudness
         + plan->target_novelty_gain
         + plan->target_high_frequency_content
         + plan->target_surprise
         + plan->safety_margin) * 0.2f
    );
}

static inline bool nocturne_scheduler_set_phase(
    nocturne_scheduler_state_t* state,
    uint64_t now_ms,
    uint32_t phase_id,
    uint64_t phase_length_ms
) {
    if (!state) {
        return false;
    }
    if (phase_length_ms == 0) {
        state->phase_id = phase_id;
        state->phase_length_ms = 0;
        state->phase_started_ms = now_ms;
        state->phase_progress = 0.0f;
        return true;
    }

    if (state->phase_length_ms == 0 || state->phase_id != phase_id) {
        state->phase_id = phase_id;
        state->phase_length_ms = phase_length_ms;
        state->phase_started_ms = now_ms;
        state->phase_progress = 0.0f;
        return true;
    }

    if (state->phase_length_ms != phase_length_ms) {
        state->phase_length_ms = phase_length_ms;
        if (now_ms < state->phase_started_ms) {
            state->phase_started_ms = now_ms;
            state->phase_progress = 0.0f;
        } else {
            const uint64_t elapsed_ms = now_ms - state->phase_started_ms;
            if (elapsed_ms >= phase_length_ms) {
                const uint64_t remainder = elapsed_ms % phase_length_ms;
                state->phase_started_ms = now_ms - remainder;
                state->phase_progress = (float)(remainder) / (float)phase_length_ms;
            } else {
                state->phase_progress = (float)elapsed_ms / (float)phase_length_ms;
            }
        }
        return false;
    }

    if (now_ms >= state->phase_started_ms) {
        const uint64_t elapsed_ms = now_ms - state->phase_started_ms;
        if (elapsed_ms >= state->phase_length_ms) {
            const uint64_t loops = elapsed_ms / state->phase_length_ms;
            state->phase_id = state->phase_id + (uint32_t)loops;
            const uint64_t remainder_ms = elapsed_ms % state->phase_length_ms;
            state->phase_started_ms += loops * state->phase_length_ms;
            state->phase_progress = (float)remainder_ms / (float)state->phase_length_ms;
            return true;
        }
        state->phase_progress = (float)elapsed_ms / (float)state->phase_length_ms;
    }
    return false;
}

static inline nocturne_scheduler_plan_t nocturne_scheduler_schedule_once(
    nocturne_scheduler_state_t* state,
    const nocturne_fused_frame_t* frame,
    bool sleep_safe,
    uint64_t now_ms
) {
    nocturne_scheduler_plan_t plan;
    memset(&plan, 0, sizeof(plan));
    plan.mode = state ? state->mode : NOCTURNE_SCHEDULER_MODE_UNKNOWN;
    if (!state || !nocturne_scheduler_frame_is_valid(frame)) {
        plan.target_event_density = 0.25f;
        plan.target_loudness = 0.20f;
        plan.target_high_frequency_content = 0.05f;
        plan.target_novelty_gain = 0.05f;
        plan.target_surprise = 0.05f;
        plan.continuity = state ? state->smoothed_event_density : 0.0f;
        plan.timer_seconds = 0.0f;
        plan.safety_margin = 0.0f;
        plan.fallback_used = true;
        plan.in_interruption_hold = false;
        plan.reason_code = 1;
        (void)strncpy(plan.reason, "missing_or_invalid_input", sizeof(plan.reason) - 1);
        return plan;
    }

    if (frame->last_update_ms > state->monotonic_ms) {
        state->monotonic_ms = frame->last_update_ms;
    }
    if (now_ms > 0) {
        state->monotonic_ms = now_ms;
    }
    state->cycle += 1;
    if (state->phase_length_ms > 0) {
        (void)nocturne_scheduler_set_phase(state, state->monotonic_ms, state->phase_id, state->phase_length_ms);
    }

    const float motion = frame->intent.motion;
    const float novelty = frame->intent.novelty;
    const float harmonic = frame->intent.harmonic_tension;
    const float event_activity = frame->intent.event_activity;
    const float brightness = frame->intent.brightness;
    const float silence = frame->intent.silence;
    const float confidence = frame->influences[0].confidence;

    float mapped_density = (event_activity * 0.60f) + (motion * 0.25f) + (brightness * 0.15f);
    float mapped_loudness = nocturne_scheduler_clamp01(1.0f - (silence * 0.75f) + (brightness * 0.12f));
    float mapped_novelty = nocturne_scheduler_clamp01((novelty * 0.55f) + (frame->influences[1].confidence * 0.20f));
    float mapped_hf = nocturne_scheduler_clamp01((harmonic * 0.50f) + (novelty * 0.25f));
    float mapped_surprise = nocturne_scheduler_clamp01((mapped_novelty + mapped_hf) * 0.5f);

    if (sleep_safe) {
        mapped_density *= state->profile.surprise_reduction_sleep;
        mapped_surprise *= state->profile.safety_surprise_cap;
        (void)strncpy(plan.reason, "sleep-safe", sizeof(plan.reason) - 1);
        plan.reason_code = 2;
    }

    const float cap_density = state->profile.max_event_density * confidence;
    const float cap_loud = state->profile.max_loudness;
    const float cap_hf = state->profile.max_high_frequency_ratio;
    const float cap_novelty = state->profile.max_novelty_gain;
    const float cap_surprise = state->profile.safety_surprise_cap;

    plan.target_event_density = nocturne_scheduler_clamp01(mapped_density > cap_density ? cap_density : mapped_density);
    plan.target_loudness = nocturne_scheduler_clamp01(mapped_loudness > cap_loud ? cap_loud : mapped_loudness);
    plan.target_high_frequency_content = nocturne_scheduler_clamp01(mapped_hf > cap_hf ? cap_hf : mapped_hf);
    plan.target_novelty_gain = nocturne_scheduler_clamp01(mapped_novelty > cap_novelty ? cap_novelty : mapped_novelty);
    plan.target_surprise = nocturne_scheduler_clamp01(mapped_surprise > cap_surprise ? cap_surprise : mapped_surprise);

    if (state->phase_id > 0 && state->phase_progress >= 0.95f) {
        plan.target_event_density *= 0.85f;
        plan.target_high_frequency_content *= 0.90f;
        if (plan.reason[0] != '\0') {
            (void)strncat(plan.reason, ", phase_tail_softening", sizeof(plan.reason) - strlen(plan.reason) - 1);
        }
    }

    const float blend_alpha = state->profile.continuity_rate;
    state->smoothed_event_density = (state->smoothed_event_density * blend_alpha) + (plan.target_event_density * (1.0f - blend_alpha));
    state->smoothed_loudness = (state->smoothed_loudness * blend_alpha) + (plan.target_loudness * (1.0f - blend_alpha));
    state->smoothed_high_frequency_content = (state->smoothed_high_frequency_content * blend_alpha) + (plan.target_high_frequency_content * (1.0f - blend_alpha));
    state->smoothed_novelty_gain = (state->smoothed_novelty_gain * blend_alpha) + (plan.target_novelty_gain * (1.0f - blend_alpha));
    state->smoothed_surprise = (state->smoothed_surprise * blend_alpha) + (plan.target_surprise * (1.0f - blend_alpha));

    plan.target_event_density = state->smoothed_event_density;
    plan.target_loudness = state->smoothed_loudness;
    plan.target_high_frequency_content = state->smoothed_high_frequency_content;
    plan.target_novelty_gain = state->smoothed_novelty_gain;
    plan.target_surprise = state->smoothed_surprise;

    if (plan.reason[0] == '\0') {
        (void)strncpy(plan.reason, "intent_to_policy_map", sizeof(plan.reason) - 1);
        plan.reason_code = 3;
    }

    if (state->interruption_until_ms > state->monotonic_ms) {
        plan.in_interruption_hold = true;
        plan.target_event_density *= 0.2f;
        plan.target_novelty_gain *= 0.4f;
        plan.target_high_frequency_content *= 0.3f;
        if (plan.reason[0] != '\0') {
            (void)strncat(plan.reason, ", interruption_hold", sizeof(plan.reason) - strlen(plan.reason) - 1);
        }
    }

    plan.timer_seconds = (plan.target_event_density * 0.75f + 0.25f) * state->profile.timer_max_ms / 1000.0f;
    if (plan.timer_seconds < 0.0f) plan.timer_seconds = 0.0f;
    if (plan.timer_seconds > state->profile.timer_max_ms / 1000.0f) {
        plan.timer_seconds = state->profile.timer_max_ms / 1000.0f;
    }

    plan.mode = state->mode;
    plan.continuity = 1.0f - (1.0f - state->smoothed_event_density) * 0.25f;
    plan.safety_margin = 1.0f - plan.target_high_frequency_content;
    plan.fallback_used = false;
    plan.timer_seconds = plan.timer_seconds + (state->phase_progress * 0.10f);
    if (plan.timer_seconds > state->profile.timer_max_ms / 1000.0f) {
        plan.timer_seconds = state->profile.timer_max_ms / 1000.0f;
    }

    if (!plan.reason[0]) {
        (void)strncpy(plan.reason, "scheduler_ok", sizeof(plan.reason) - 1);
    }
    return plan;
}

static inline void nocturne_scheduler_signal_interrupt(nocturne_scheduler_state_t* state, uint64_t now_ms, uint32_t hold_ms) {
    if (!state) return;
    state->interruption_until_ms = now_ms + (uint64_t)hold_ms;
    state->interrupts += 1;
}

static inline void nocturne_scheduler_trace_init(nocturne_scheduler_trace_t* trace) {
    if (!trace) return;
    memset(trace, 0, sizeof(*trace));
    trace->enabled = true;
}

static inline void nocturne_scheduler_trace_append(nocturne_scheduler_trace_t* trace, const nocturne_scheduler_plan_t* plan, uint64_t now_ms, nocturne_scheduler_mode_t mode) {
    if (!trace || !plan || !trace->enabled) {
        return;
    }
    uint32_t idx = trace->count % (uint32_t)(sizeof(trace->entries) / sizeof(trace->entries[0]));
    trace->entries[idx].monotonic_ms = now_ms;
    trace->entries[idx].plan = *plan;
    trace->entries[idx].source_mode = mode;
    trace->entries[idx].trace_signature = nocturne_scheduler_signature(plan);
    trace->entries[idx].entry_id = trace->count;
    trace->count += 1;
}

static inline bool nocturne_scheduler_trace_replay(const nocturne_scheduler_trace_t* trace, uint32_t index, nocturne_scheduler_trace_entry_t* out) {
    if (!trace || !out || trace->count == 0) {
        return false;
    }
    if (index >= trace->count) {
        return false;
    }
    uint32_t max_entries = (uint32_t)(sizeof(trace->entries) / sizeof(trace->entries[0]));
    uint32_t idx = index % max_entries;
    *out = trace->entries[idx];
    return true;
}

#ifdef __cplusplus
}
#endif
