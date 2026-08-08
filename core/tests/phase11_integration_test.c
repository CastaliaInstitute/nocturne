#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../include/nocturne/scheduler/scheduler.h"
#include "../include/nocturne/adapters/adapter_registry.h"
#include "../include/nocturne/adapters/sensor_adapter.h"

#define SESSION_LEN 128

static void require_true(int cond, const char* message) {
    if (!cond) {
        fprintf(stderr, "ASSERT FAIL: %s\n", message);
        assert(cond);
    }
}

static nocturne_fused_frame_t make_frame(float base, uint64_t now_ms) {
    nocturne_fused_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    frame.intent.grounding = base;
    frame.intent.brightness = base + 0.02f;
    frame.intent.warmth = base + 0.04f;
    frame.intent.density = base + 0.06f;
    frame.intent.motion = base + 0.08f;
    frame.intent.spaciousness = base + 0.10f;
    frame.intent.silence = 0.05f;
    frame.intent.event_activity = base + 0.12f;
    frame.intent.harmonic_tension = base + 0.14f;
    frame.intent.novelty = base + 0.16f;

    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        frame.influences[i].confidence = 0.81f;
        frame.influences[i].value = 0.2f + 0.01f * (float)i;
    }
    frame.last_update_ms = now_ms;
    return frame;
}

static nocturne_sensor_sample_t zero_sample;

static bool read_zero(void* state, nocturne_sensor_sample_t* out) {
    (void)state;
    if (!out) return false;
    *out = zero_sample;
    return false;
}

static bool read_ok(void* state, nocturne_sensor_sample_t* out) {
    (void)state;
    if (!out) return false;
    out->kind = NOCTURNE_SENSOR_MOTION;
    out->value = 0.12f;
    out->confidence = 0.8f;
    out->sample_ms = 123;
    out->ttl_ms = 500;
    out->stale = false;
    out->source_domain = NOCTURNE_CONTEXT_OBSERVED;
    return true;
}

static bool no_reset(void* state) {
    (void)state;
    return true;
}

static void test_deterministic_simulation_harness(void) {
    nocturne_fused_frame_t frame = {0};
    nocturne_scheduler_state_t state = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_SLEEP, NULL);
    nocturne_scheduler_set_phase(&state, 1000, 11, 1000);

    float loud_sum = 0.0f;
    float total_density = 0.0f;
    for (int i = 0; i < SESSION_LEN; ++i) {
        frame = make_frame((float)(i % 10) / 40.0f, 1000u + (uint64_t)i * 125u);
        nocturne_scheduler_plan_t plan = nocturne_scheduler_schedule_once(&state, &frame, (i % 2) == 0, frame.last_update_ms);
        require_true(plan.target_event_density <= 1.0f && plan.target_event_density >= 0.0f, "deterministic session plan density bounded");
        require_true(plan.target_loudness <= 1.0f && plan.target_loudness >= 0.0f, "deterministic session plan loudness bounded");
        require_true(plan.target_high_frequency_content <= state.profile.max_high_frequency_ratio + 1e-6f, "hf respects profile cap");
        loud_sum += plan.target_loudness;
        total_density += plan.target_event_density;
    }

    require_true(loud_sum > 0.0f, "deterministic session produced nonzero loudness");
    require_true(total_density > 0.0f, "deterministic session produced nonzero density");
}

static void test_replay_integration_long_session(void) {
    nocturne_scheduler_state_t state = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_FOCUS, NULL);
    nocturne_scheduler_trace_t trace;
    nocturne_scheduler_trace_init(&trace);

    nocturne_fused_frame_t frame = {0};
    for (int i = 0; i < 30; ++i) {
        frame = make_frame(0.2f + (float)i / 200.0f, 2000u + (uint64_t)i * 64u);
        nocturne_scheduler_plan_t plan = nocturne_scheduler_schedule_once(&state, &frame, false, frame.last_update_ms);
        nocturne_scheduler_trace_append(&trace, &plan, frame.last_update_ms, state.mode);
    }

    nocturne_scheduler_trace_entry_t out_first;
    nocturne_scheduler_trace_entry_t out_mid;
    nocturne_scheduler_trace_entry_t out_last;

    require_true(nocturne_scheduler_trace_replay(&trace, 0, &out_first), "long-session replay first frame");
    require_true(nocturne_scheduler_trace_replay(&trace, 15, &out_mid), "long-session replay mid frame");
    require_true(nocturne_scheduler_trace_replay(&trace, 29, &out_last), "long-session replay last frame");
    require_true(out_last.entry_id > out_first.entry_id, "trace entry ids increase");
}

static void test_safety_boundaries_for_scheduler_policy(void) {
    nocturne_scheduler_state_t state = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_RITUAL, NULL);
    nocturne_fused_frame_t frame = make_frame(0.95f, 5000);

    frame.intent.novelty = 1.9f;
    frame.intent.harmonic_tension = 1.4f;
    frame.intent.event_activity = 2.0f;
    frame.intent.brightness = 2.0f;

    nocturne_scheduler_plan_t plan = nocturne_scheduler_schedule_once(&state, &frame, true, 5000);

    require_true(plan.target_loudness <= state.profile.max_loudness + 1e-6f, "loudness respects cap");
    require_true(plan.target_novelty_gain <= state.profile.max_novelty_gain + 1e-6f, "novelty respects cap");
    require_true(plan.target_high_frequency_content <= state.profile.max_high_frequency_ratio + 1e-6f, "hf respects cap");
    require_true(plan.timer_seconds <= state.profile.timer_max_ms / 1000.0f + 1e-6f, "timer respects max");
}

static void test_stress_missing_data_and_sensor_dropout(void) {
    nocturne_fused_frame_t malformed = {0};
    malformed.intent.grounding = 0.4f;
    malformed.intent.brightness = 0.6f;
    malformed.intent.warmth = 0.1f;
    malformed.intent.density = 0.3f;
    malformed.intent.motion = 0.5f;
    malformed.intent.spaciousness = 0.2f;
    malformed.intent.silence = -0.1f;
    malformed.intent.event_activity = 0.4f;
    malformed.intent.harmonic_tension = 0.1f;
    malformed.intent.novelty = 0.2f;
    malformed.last_update_ms = 7000;

    nocturne_scheduler_plan_t fallback = nocturne_scheduler_schedule_once(NULL, &malformed, false, 7000);
    require_true(fallback.fallback_used, "null state is handled as fallback even with partial frame");

    nocturne_sensor_registry_t registry;
    nocturne_sensor_registry_init(&registry);

    nocturne_sensor_adapter_t bad = {
        .kind = NOCTURNE_SENSOR_TEMPERATURE,
        .vtable = {
            .name = "noop",
            .read = read_zero,
            .reset = no_reset,
            .destroy = NULL,
        },
        .state = (void*)1,
    };

    require_true(nocturne_sensor_registry_add(&registry, bad), "registry add for dropped-sensor adapter");
    nocturne_sensor_sample_t out[4] = {0};
    size_t got = 0;
    require_true(nocturne_sensor_read_all(&registry, out, 4, &got), "read_all succeeds on dropped samples");
    require_true(got == 0, "no samples from dropped sensor read");

    nocturne_sensor_adapter_t ok = {
        .kind = NOCTURNE_SENSOR_MOTION,
        .vtable = {
            .name = "ok",
            .read = read_ok,
            .reset = no_reset,
            .destroy = NULL,
        },
        .state = (void*)1,
    };
    nocturne_sensor_registry_init(&registry);
    require_true(nocturne_sensor_registry_add(&registry, bad), "registry add dropped adapter");
    require_true(nocturne_sensor_registry_add(&registry, ok), "registry add operational adapter");
    require_true(nocturne_sensor_read_all(&registry, out, 4, &got), "read_all executes on mixed adapters");
    require_true(got == 1, "only operational sensor sample recorded");
}

int main(void) {
    test_deterministic_simulation_harness();
    test_replay_integration_long_session();
    test_safety_boundaries_for_scheduler_policy();
    test_stress_missing_data_and_sensor_dropout();

    printf("phase11_integration_test passed\n");
    return 0;
}
