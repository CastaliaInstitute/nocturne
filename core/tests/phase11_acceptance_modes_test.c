#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/nocturne/scheduler/scheduler.h"

typedef struct {
    float loudness_acc;
    float density_acc;
    float hf_acc;
    float novelty_acc;
    uint32_t plans;
} mode_summary_t;

static nocturne_fused_frame_t make_session_frame(float base, uint64_t now_ms) {
    nocturne_fused_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    frame.intent.grounding = base;
    frame.intent.brightness = base + 0.08f;
    frame.intent.warmth = 0.60f;
    frame.intent.density = base + 0.04f;
    frame.intent.motion = 0.24f;
    frame.intent.spaciousness = 0.45f;
    frame.intent.silence = 0.08f + (base * 0.06f);
    frame.intent.event_activity = 0.18f + (base * 0.20f);
    frame.intent.harmonic_tension = 0.30f;
    frame.intent.novelty = 0.22f;

    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        frame.influences[i].confidence = 0.9f - (0.05f * (float)i / 10.0f);
        frame.influences[i].provenance_weight = 0.6f;
        frame.influences[i].value = 0.20f + 0.01f * (float)i;
        frame.influences[i].min_bound = 0.0f;
        frame.influences[i].max_bound = 1.0f;
    }

    frame.last_update_ms = (float)now_ms;
    return frame;
}

static mode_summary_t summarize_mode(nocturne_scheduler_mode_t mode, bool sleep_safe) {
    mode_summary_t summary = {0};
    nocturne_scheduler_state_t state = nocturne_scheduler_state_init(mode, NULL);
    nocturne_scheduler_set_phase(&state, 111u, 11u, 1000u);

    for (uint32_t i = 0; i < 32; ++i) {
        const float base = 0.30f + (float)i / 120.0f;
        const nocturne_fused_frame_t frame = make_session_frame(base, (uint64_t)i * 100u + 11u);
        const nocturne_scheduler_plan_t plan = nocturne_scheduler_schedule_once(&state, &frame, sleep_safe, frame.last_update_ms);

        assert(!plan.fallback_used);
        assert(isfinite(plan.target_loudness));
        assert(isfinite(plan.target_event_density));
        assert(isfinite(plan.target_high_frequency_content));
        assert(isfinite(plan.target_novelty_gain));
        assert(plan.timer_seconds >= 0.0f);
        assert(plan.timer_seconds <= state.profile.timer_max_ms / 1000.0f + 1e-6f);

        summary.loudness_acc += plan.target_loudness;
        summary.density_acc += plan.target_event_density;
        summary.hf_acc += plan.target_high_frequency_content;
        summary.novelty_acc += plan.target_novelty_gain;
        summary.plans++;
    }

    assert(summary.plans > 0);
    summary.loudness_acc /= (float)summary.plans;
    summary.density_acc /= (float)summary.plans;
    summary.hf_acc /= (float)summary.plans;
    summary.novelty_acc /= (float)summary.plans;

    return summary;
}

int main(void) {
    const mode_summary_t sleep_mode = summarize_mode(NOCTURNE_SCHEDULER_MODE_SLEEP, true);
    const mode_summary_t focus_mode = summarize_mode(NOCTURNE_SCHEDULER_MODE_FOCUS, false);
    const mode_summary_t ritual_mode = summarize_mode(NOCTURNE_SCHEDULER_MODE_RITUAL, false);

    /* Acceptance expectations by user mode profile ordering */
    assert(sleep_mode.loudness_acc < ritual_mode.loudness_acc + 1e-6f);
    assert(ritual_mode.loudness_acc < focus_mode.loudness_acc);
    assert(sleep_mode.hf_acc < ritual_mode.hf_acc + 1e-6f);
    assert(ritual_mode.hf_acc <= focus_mode.hf_acc + 0.02f);
    assert(sleep_mode.density_acc < focus_mode.density_acc);
    assert(focus_mode.novelty_acc <= 0.5f);

    printf("phase11_acceptance_modes_test passed\n");
    return 0;
}
