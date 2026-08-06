#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../include/nocturne/scheduler/scheduler.h"

typedef struct {
    uint32_t state;
} xorshift32_t;

static uint32_t xorshift32_next(xorshift32_t* x) {
    uint32_t y = x->state;
    y ^= y << 13;
    y ^= y >> 17;
    y ^= y << 5;
    x->state = y;
    return y;
}

static float float_from_u32(uint32_t x) {
    return (float)x / (float)UINT32_MAX;
}

static nocturne_fused_frame_t make_base_frame(float base, uint64_t now_ms) {
    nocturne_fused_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    frame.intent.grounding = base;
    frame.intent.brightness = 0.2f + 0.01f * base;
    frame.intent.warmth = 0.15f + 0.005f * base;
    frame.intent.density = 0.35f;
    frame.intent.motion = 0.40f;
    frame.intent.spaciousness = 0.25f;
    frame.intent.silence = 0.10f;
    frame.intent.event_activity = 0.30f;
    frame.intent.harmonic_tension = 0.22f;
    frame.intent.novelty = 0.28f;
    frame.valid_inputs = 1;

    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        frame.influences[i].confidence = 0.8f;
        frame.influences[i].provenance_weight = 0.6f;
        frame.influences[i].value = 0.5f;
        frame.influences[i].min_bound = 0.0f;
        frame.influences[i].max_bound = 1.0f;
    }

    frame.last_update_ms = (float)now_ms;
    return frame;
}

static bool frame_strict_valid(const nocturne_fused_frame_t* frame) {
    if (!frame) {
        return false;
    }
    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        const float* p = ((const float*)&frame->intent) + i;
        if (!isfinite(*p) || *p < 0.0f || *p > 1.0f) {
            return false;
        }
    }
    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        const nocturne_influence_t* inf = &frame->influences[i];
        if (!isfinite(inf->value) || inf->value < 0.0f || inf->value > 1.0f) return false;
        if (!isfinite(inf->confidence) || inf->confidence < 0.0f || inf->confidence > 1.0f) return false;
        if (!isfinite(inf->provenance_weight) || inf->provenance_weight < 0.0f || inf->provenance_weight > 1.0f) return false;
        if (!isfinite(inf->min_bound) || !isfinite(inf->max_bound)) return false;
        if (inf->min_bound > inf->max_bound) return false;
    }
    return true;
}

static void assert_plan_sane(const nocturne_scheduler_plan_t* plan, const nocturne_scheduler_state_t* state) {
    assert(plan);
    assert(isfinite(plan->target_event_density));
    assert(isfinite(plan->target_loudness));
    assert(isfinite(plan->target_high_frequency_content));
    assert(isfinite(plan->target_novelty_gain));
    assert(isfinite(plan->target_surprise));
    assert(plan->target_event_density >= 0.0f && plan->target_event_density <= 1.0f);
    assert(plan->target_loudness >= 0.0f && plan->target_loudness <= state->profile.max_loudness + 1e-6f);
    assert(plan->target_high_frequency_content >= 0.0f && plan->target_high_frequency_content <= state->profile.max_high_frequency_ratio + 1e-6f);
    assert(plan->target_novelty_gain >= 0.0f && plan->target_novelty_gain <= state->profile.max_novelty_gain + 1e-6f);
}

int main(void) {
    xorshift32_t rng = { .state = 0x12345678u };
    nocturne_scheduler_state_t state = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_FOCUS, NULL);
    state.phase_length_ms = 1200;
    state.phase_id = 3;

    for (uint32_t i = 0; i < 1200; ++i) {
        float base = float_from_u32(xorshift32_next(&rng));
        nocturne_fused_frame_t frame = make_base_frame(base, (uint64_t)i * 17u);
        const uint32_t mutation = xorshift32_next(&rng) % 10u;

        switch (mutation) {
            case 0:
                frame.intent.grounding = -0.1f;
                break;
            case 1:
                frame.intent.brightness = 1.8f;
                break;
            case 2:
                frame.intent.novelty = (float)NAN;
                break;
            case 3:
                frame.intent.motion = (float)INFINITY;
                break;
            case 4:
                frame.influences[0].confidence = -0.3f;
                break;
            case 5:
                frame.influences[1].value = 1.3f;
                break;
            case 6:
                frame.influences[2].provenance_weight = (float)-INFINITY;
                break;
            case 7:
                frame.last_update_ms = -1.0f;
                break;
            case 8: {
                frame.valid_inputs = 0;
                break;
            }
            default:
                break;
        }

        const bool expected_fallback = !frame_strict_valid(&frame);
        const nocturne_scheduler_plan_t plan = nocturne_scheduler_schedule_once(&state, &frame, (i & 1u) == 0u, (uint64_t)i * 17u + 1u);

        assert(plan.fallback_used == expected_fallback);
        assert_plan_sane(&plan, &state);

        if (expected_fallback) {
            assert(strcmp(plan.reason, "missing_or_invalid_input") == 0);
        } else {
            assert(strcmp(plan.reason, "missing_or_invalid_input") != 0);
        }
    }

    printf("phase11_fuzz_context_test passed\n");
    return 0;
}
