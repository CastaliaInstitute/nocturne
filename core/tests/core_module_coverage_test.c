#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../include/nocturne/context.h"
#include "../include/nocturne/compatibility.h"
#include "../include/nocturne/chronology.h"
#include "../include/nocturne/seed.h"
#include "../include/nocturne/fusion/fusion.h"
#include "../include/nocturne/fusion/fusion_profiles.h"
#include "../include/nocturne/fusion/fusion_log.h"
#include "../include/nocturne/fusion/policy.h"
#include "../include/nocturne/adapters/simulated_sensors.h"
#include "../include/nocturne/adapters/sensor_adapter.h"
#include "../include/nocturne/adapters/adapter_registry.h"
#include "../include/nocturne/adapters/computed_adapters.h"
#include "../include/nocturne/adapters/symbolic_adapters.h"
#include "../include/nocturne/adapters/user_controls.h"

static void assert_module_core_types(void) {
    assert(nocturne_compat_check_schema(NOCTURNE_SCHEMA_VERSION_CURRENT) == NOCTURNE_COMPATIBLE);
    assert(nocturne_default_chronology().year == 2026);

    const uint64_t seed = nocturne_seed_init(13, 17);
    const uint64_t next = nocturne_next_seed(seed);
    assert(next != seed);

    nocturne_seed_state_t state = {seed, 17, 0};
    nocturne_seed_advance(&state);
    assert(state.sequence == 1);
}

static void assert_module_fusion_pipeline(void) {
    const nocturne_context_contribution_t computed = nocturne_build_computed_context(1234, 7.0f, 8.0f, 3);
    const nocturne_context_contribution_t symbolic = nocturne_build_symbolic_context(nocturne_symbolic_bias_for("root", NOCTURNE_SYMBOLIC_CHAKRA));
    const nocturne_context_contribution_t samples[] = {computed, symbolic};

    const nocturne_fusion_profile_t profile = nocturne_fusion_profile_for(NOCTURNE_FUSION_PROFILE_FOCUS);
    const nocturne_fused_frame_t fused = nocturne_fuse(&profile, samples, 2, NULL);

    assert(fused.valid_inputs == 2);
    assert(fused.influences[0].confidence >= profile.confidence_floor);
    assert(fused.influences[0].confidence <= profile.confidence_ceil);

    nocturne_fusion_result_t result = nocturne_fuse_with_explainability(&profile, samples, 2, NULL, true);
    assert(result.explain_count == NOCTURNE_INTENT_DIMENSIONS);
    assert(result.entries[1].weighted_value >= 0.0f);

    nocturne_fusion_log_t log;
    nocturne_fusion_log_init(&log, true);
    assert(nocturne_fusion_log_append(&log, &result));
    nocturne_fused_frame_t replay = {0};
    assert(nocturne_fusion_log_replay(&log, 0, &replay));
}

static void assert_module_safety_and_sensors(void) {
    nocturne_fused_frame_t frame = {0};
    frame.intent.novelty = 0.99f;
    frame.intent.brightness = 1.0f;
    frame.intent.event_activity = 0.95f;
    frame.intent.harmonic_tension = 0.9f;

    nocturne_safety_policy_t policy;
    nocturne_safety_policy_init_default(&policy);
    policy.parental_controls_enabled = true;
    policy.sleep_safe_mode = true;
    nocturne_fusion_conflict_report_t conflict = {0};

    const nocturne_safety_result_t safety = nocturne_apply_safety_policy(&frame, &policy, &conflict);
    assert(safety.count >= 0);
    assert(frame.intent.novelty <= policy.max_novelty);

    nocturne_sensor_adapter_t sensor = nocturne_make_simulated_sensor(NOCTURNE_SENSOR_TEMPERATURE);
    assert(sensor.vtable.read != NULL);

    nocturne_sensor_sample_t sample;
    assert(nocturne_sensor_adapter_read(&sensor, &sample));
    assert(nocturne_sensor_sample_is_valid(&sample));
    assert(sensor.vtable.destroy != NULL);

    nocturne_sensor_registry_t registry;
    nocturne_sensor_registry_init(&registry);
    assert(nocturne_sensor_registry_add(&registry, sensor));
    assert(registry.count == 1);

    nocturne_sensor_sample_t samples[1];
    size_t got = 0;
    assert(nocturne_sensor_read_all(&registry, samples, 1, &got));
    assert(got == 1);
}

static void assert_module_controls_and_modes(void) {
    nocturne_user_controls_t controls = {
        .mode = NOCTURNE_MODE_FOCUS,
        .user_brightness_bias = 0.25f,
        .user_warmth_bias = 0.20f,
        .user_density_cap = 0.40f,
        .sleep_safe_mode = false,
        .parental_controls_enabled = false,
    };
    const nocturne_context_contribution_t user = nocturne_build_user_context(&controls);
    assert(user.source_domain == NOCTURNE_CONTEXT_USER);
    assert(user.brightness.value == 0.25f);
    assert(strcmp(nocturne_intent_name(1), "brightness") == 0);
}

int main(void) {
    assert_module_core_types();
    assert_module_fusion_pipeline();
    assert_module_safety_and_sensors();
    assert_module_controls_and_modes();

    printf("core_module_coverage_test passed\n");
    return 0;
}
