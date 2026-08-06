#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "../include/nocturne/scheduler/scheduler.h"

typedef struct {
    float loudness;
    float density;
    float hf;
    float novelty;
    float surprise;
    float safety;
} mode_snapshot_t;

static nocturne_fused_frame_t make_input(float base, uint64_t now_ms) {
    nocturne_fused_frame_t frame;
    frame.intent.grounding = base;
    frame.intent.brightness = base + 0.01f;
    frame.intent.warmth = base + 0.02f;
    frame.intent.density = base + 0.03f;
    frame.intent.motion = base + 0.04f;
    frame.intent.spaciousness = base + 0.05f;
    frame.intent.silence = 0.1f;
    frame.intent.event_activity = base + 0.06f;
    frame.intent.harmonic_tension = base + 0.07f;
    frame.intent.novelty = base + 0.08f;

    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        frame.influences[i].value = 0.2f;
        frame.influences[i].confidence = 0.8f;
        frame.influences[i].provenance_weight = 0.6f;
        frame.influences[i].min_bound = 0.0f;
        frame.influences[i].max_bound = 1.0f;
    }

    frame.last_update_ms = (float)now_ms;
    return frame;
}

static mode_snapshot_t run_profile_sequence(
    const nocturne_scheduler_profile_t* override_profile,
    bool sleep_safe
) {
    nocturne_scheduler_state_t state = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_FOCUS, NULL);
    if (override_profile) {
        state.profile = *override_profile;
    }

    mode_snapshot_t acc = {0};
    uint32_t count = 0;

    for (uint32_t i = 0; i < 64; ++i) {
        const float base = 0.15f + (float)i * 0.012f;
        const nocturne_fused_frame_t frame = make_input(base, (uint64_t)i * 31u + 11u);
        const nocturne_scheduler_plan_t plan = nocturne_scheduler_schedule_once(&state, &frame, sleep_safe, frame.last_update_ms);
        assert(!plan.fallback_used);
        assert(isfinite(plan.target_loudness));
        assert(isfinite(plan.target_event_density));
        assert(isfinite(plan.target_high_frequency_content));
        assert(isfinite(plan.target_novelty_gain));

        acc.loudness += plan.target_loudness;
        acc.density += plan.target_event_density;
        acc.hf += plan.target_high_frequency_content;
        acc.novelty += plan.target_novelty_gain;
        acc.surprise += plan.target_surprise;
        acc.safety += plan.safety_margin;
        ++count;
    }

    assert(count > 0);
    acc.loudness /= (float)count;
    acc.density /= (float)count;
    acc.hf /= (float)count;
    acc.novelty /= (float)count;
    acc.surprise /= (float)count;
    acc.safety /= (float)count;

    return acc;
}

int main(void) {
    const mode_snapshot_t desktop = run_profile_sequence(NULL, false);

    nocturne_scheduler_profile_t embedded = nocturne_scheduler_profile_for(NOCTURNE_SCHEDULER_MODE_FOCUS);
    embedded.max_event_density = embedded.max_event_density * 0.85f;
    embedded.max_loudness = embedded.max_loudness * 0.80f;
    embedded.max_high_frequency_ratio = embedded.max_high_frequency_ratio * 0.75f;
    embedded.max_novelty_gain = embedded.max_novelty_gain * 0.80f;

    const mode_snapshot_t embedded_snapshot = run_profile_sequence(&embedded, false);

    assert(embedded_snapshot.loudness <= desktop.loudness + 1e-6f);
    assert(embedded_snapshot.density <= desktop.density + 1e-6f);
    assert(embedded_snapshot.hf <= desktop.hf + 1e-6f);
    assert(embedded_snapshot.novelty <= desktop.novelty + 1e-6f);
    assert(embedded_snapshot.safety >= desktop.safety - 1e-6f);

    printf("phase11_parity_test passed\n");
    return 0;
}
