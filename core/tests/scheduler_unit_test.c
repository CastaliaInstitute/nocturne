#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../include/nocturne/scheduler/scheduler.h"

static nocturne_fused_frame_t make_frame(float value_base, uint64_t now_ms) {
    nocturne_fused_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    frame.intent.grounding = value_base;
    frame.intent.brightness = value_base + 0.01f;
    frame.intent.warmth = value_base + 0.02f;
    frame.intent.density = value_base + 0.03f;
    frame.intent.motion = value_base + 0.04f;
    frame.intent.spaciousness = value_base + 0.05f;
    frame.intent.silence = 0.10f;
    frame.intent.event_activity = value_base + 0.06f;
    frame.intent.harmonic_tension = value_base + 0.07f;
    frame.intent.novelty = value_base + 0.08f;

    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        frame.influences[i].confidence = 0.8f;
        frame.influences[i].provenance_weight = 0.6f;
        frame.influences[i].value = ((float)(i + 1)) / 10.0f;
        frame.influences[i].min_bound = 0.0f;
        frame.influences[i].max_bound = 1.0f;
    }
    frame.last_update_ms = now_ms;
    return frame;
}

static void require_true(bool cond, const char* message) {
    if (!cond) {
        fprintf(stderr, "ASSERT FAIL: %s\n", message);
        assert(cond);
    }
}

static void test_fallback_when_invalid_state(void) {
    nocturne_fused_frame_t frame = make_frame(0.2f, 100);
    frame.intent.event_activity = -0.2f;

    nocturne_scheduler_plan_t plan = nocturne_scheduler_schedule_once(NULL, &frame, false, 200);

    require_true(plan.fallback_used, "fallback_used expected for null state");
    require_true(strcmp(plan.reason, "missing_or_invalid_input") == 0, "fallback reason text");
    require_true(plan.timer_seconds == 0.0f, "fallback timer reset");
    require_true(plan.target_loudness == 0.2f, "fallback target loudness");
}

static void test_deterministic_output_for_same_inputs(void) {
    nocturne_fused_frame_t frame = make_frame(0.4f, 500);
    nocturne_scheduler_state_t s1 = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_FOCUS, NULL);
    nocturne_scheduler_state_t s2 = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_FOCUS, NULL);

    nocturne_scheduler_plan_t p1 = nocturne_scheduler_schedule_once(&s1, &frame, false, 600);
    nocturne_scheduler_plan_t p2 = nocturne_scheduler_schedule_once(&s2, &frame, false, 600);

    require_true(memcmp(&p1, &p2, sizeof(p1)) == 0, "deterministic plans are identical");
}

static void test_phase_progress_advance_and_wrap(void) {
    nocturne_fused_frame_t frame = make_frame(0.45f, 1000);
    nocturne_scheduler_state_t state = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_SLEEP, NULL);

    bool phase_started = nocturne_scheduler_set_phase(&state, 1000, 7, 1000);
    require_true(phase_started, "phase_started should indicate change on first set");

    nocturne_scheduler_plan_t plan1 = nocturne_scheduler_schedule_once(&state, &frame, false, 2005);
    require_true(state.phase_id == 8, "phase id should advance by loop count");
    require_true(state.phase_progress > 0.0f, "phase progressed into new cycle");
    require_true(plan1.target_event_density > 0.0f, "plan density generated");
}

static void test_interrupt_hold_reduces_activity(void) {
    nocturne_fused_frame_t frame = make_frame(0.5f, 1000);
    nocturne_scheduler_state_t state = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_RITUAL, NULL);

    nocturne_scheduler_plan_t normal = nocturne_scheduler_schedule_once(&state, &frame, false, 1000);

    state = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_RITUAL, NULL);
    nocturne_scheduler_signal_interrupt(&state, 1200, 500);
    nocturne_scheduler_plan_t held = nocturne_scheduler_schedule_once(&state, &frame, false, 1300);

    require_true(held.in_interruption_hold, "interruption hold active");
    require_true(held.target_event_density < normal.target_event_density, "event density reduced during hold");
    require_true(held.target_novelty_gain < normal.target_novelty_gain, "novelty reduced during hold");
    require_true(held.target_high_frequency_content < normal.target_high_frequency_content, "HF reduced during hold");
}

static void test_trace_append_and_replay(void) {
    nocturne_fused_frame_t frame = make_frame(0.3f, 1100);
    nocturne_scheduler_state_t state = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_UNKNOWN, NULL);
    nocturne_scheduler_trace_t trace;
    nocturne_scheduler_trace_init(&trace);

    for (int i = 0; i < 3; ++i) {
        frame.last_update_ms += 100;
        state.monotonic_ms = frame.last_update_ms;
        nocturne_scheduler_plan_t plan = nocturne_scheduler_schedule_once(&state, &frame, false, frame.last_update_ms);
        nocturne_scheduler_trace_append(&trace, &plan, state.monotonic_ms, state.mode);
    }

    require_true(trace.count == 3, "trace count after appends");
    nocturne_scheduler_trace_entry_t out;
    const bool got = nocturne_scheduler_trace_replay(&trace, 1, &out);
    require_true(got, "trace replay returns second entry");
    require_true(out.entry_id == 1, "trace entry id retained");
}

int main(void) {
    test_fallback_when_invalid_state();
    test_deterministic_output_for_same_inputs();
    test_phase_progress_advance_and_wrap();
    test_interrupt_hold_reduces_activity();
    test_trace_append_and_replay();

    printf("scheduler_unit_test passed\n");
    return 0;
}
