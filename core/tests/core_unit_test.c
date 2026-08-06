#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/nocturne/context.h"
#include "../include/nocturne/compatibility.h"
#include "../include/nocturne/chronology.h"
#include "../include/nocturne/seed.h"
#include "../include/nocturne/fusion/policy.h"
#include "../include/nocturne/fusion/fusion_profiles.h"
#include "../include/nocturne/fusion/fusion.h"
#include "../include/nocturne/fusion/fusion_log.h"
#include "../include/nocturne/adapters/simulated_sensors.h"
#include "../include/nocturne/adapters/adapter_registry.h"
#include "../include/nocturne/adapters/computed_adapters.h"
#include "../include/nocturne/adapters/symbolic_adapters.h"
#include "../include/nocturne/adapters/user_controls.h"

static void require_true(int cond, const char* message) {
    if (!cond) {
        fprintf(stderr, "ASSERT FAIL: %s\n", message);
        assert(cond);
    }
}

static void require_float_near(float a, float b, float eps, const char* message) {
    require_true(fabsf(a - b) <= eps, message);
}

static void test_context_utilities(void) {
    require_true(nocturne_clamp_float(-0.5f, 0.0f, 1.0f) == 0.0f, "context clamp lower bound");
    require_true(nocturne_clamp_float(1.2f, 0.0f, 1.0f) == 1.0f, "context clamp upper bound");

    require_float_near(nocturne_smooth(1.0f, 3.0f, 0.25f), 1.5f, 0.0001f, "smoothing alpha blend");

    nocturne_context_contribution_t contrib = {0};
    contrib.grounding.confidence = 0.3f;
    contrib.brightness.confidence = 0.7f;
    require_true(nocturne_domain_ok(NOCTURNE_CONTEXT_OBSERVED), "observed domain is valid");
    require_true(!nocturne_domain_ok((nocturne_context_domain_t)99), "invalid domain rejected");
}

static void test_compatibility_and_schema(void) {
    require_true(nocturne_compat_check_schema(NOCTURNE_SCHEMA_VERSION_CURRENT) == NOCTURNE_COMPATIBLE, "current schema compatible");
    require_true(nocturne_compat_check_schema(NOCTURNE_SCHEMA_VERSION_CURRENT - 1) == NOCTURNE_NEEDS_MIGRATION, "older schema migration needed");
    require_true(nocturne_compat_check_schema(NOCTURNE_SCHEMA_VERSION_CURRENT + 1) == NOCTURNE_INCOMPATIBLE, "future schema incompatible");
}

static void test_chronology_defaults(void) {
    nocturne_chronology_t chron = nocturne_default_chronology();
    require_true(chron.year == 2026, "default year initialized");
    require_true(chron.season == NOCTURNE_SEASON_SPRING, "default season is spring");
    require_true(chron.sleep_phase_ratio == 0.5f, "default sleep phase is centered");
}

static void test_seed_determinism_and_advance(void) {
    const uint64_t seed = nocturne_seed_init(42u, 7u);
    uint64_t advance = nocturne_next_seed(seed);
    require_true(seed != advance, "seed advance changes state");

    nocturne_seed_state_t state = {seed, 7u, 0u};
    nocturne_seed_advance(&state);
    require_true(state.sequence == 1u, "sequence increments on advance");
    require_true(state.seed == advance, "seed advances through helper");
}

static void test_profiles_and_fusion_cap(void) {
    nocturne_fusion_profile_t sleep_profile = nocturne_fusion_profile_for(NOCTURNE_FUSION_PROFILE_SLEEP);
    nocturne_fusion_profile_t focus_profile = nocturne_fusion_profile_for(NOCTURNE_FUSION_PROFILE_FOCUS);
    nocturne_fusion_profile_t ritual_profile = nocturne_fusion_profile_for(NOCTURNE_FUSION_PROFILE_RITUAL);

    require_true(focus_profile.anti_dominance_cap > sleep_profile.anti_dominance_cap, "focus profile less strict than sleep");
    require_true(ritual_profile.confidence_floor > 0.0f, "ritual confidence floor initialized");

    nocturne_fused_frame_t frame = {0};
    memset(&frame, 0, sizeof(frame));
    frame.valid_inputs = 1;
    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        frame.influences[i].confidence = 0.8f;
        frame.influences[i].provenance_weight = 0.4f;
        frame.influences[i].value = 0.12f;
    }

    frame.intent.novelty = 1.2f;
    frame.intent.event_activity = 2.1f;

    nocturne_safety_policy_t policy;
    nocturne_safety_policy_init_default(&policy);
    policy.sleep_safe_mode = true;

    nocturne_fusion_conflict_report_t conflict = {0};

    nocturne_apply_safety_policy(&frame, &policy, &conflict);
    require_true(frame.intent.novelty <= 0.90f, "novelty capped");
    require_true(frame.intent.event_activity <= policy.max_event_activity, "event activity capped");
    require_true(frame.intent.density <= policy.max_density_when_sleep_safe, "sleep safe density cap enabled");
}

static void test_adapters_and_registry(void) {
    nocturne_sensor_adapter_t sensor = nocturne_make_simulated_sensor(NOCTURNE_SENSOR_HEART_RATE);
    require_true(sensor.vtable.read != NULL, "simulated adapter has read");

    nocturne_sensor_sample_t sample;
    require_true(nocturne_sensor_adapter_read(&sensor, &sample), "simulated read succeeds");
    require_true(sample.value > 0.0f, "simulated sample has positive value");
    require_true(sample.ttl_ms == 500, "simulated ttl set");
    require_true(nocturne_sensor_sample_is_valid(&sample), "valid sample passes validity");

    nocturne_sensor_registry_t registry;
    nocturne_sensor_registry_init(&registry);
    require_true(nocturne_sensor_registry_add(&registry, sensor), "registry accepts adapter");
    require_true(registry.count == 1, "registry count increments");

    nocturne_sensor_sample_t samples[2] = {0};
    size_t got = 0;
    require_true(nocturne_sensor_read_all(&registry, samples, 2, &got), "read all executes");
    require_true(got == 1, "read_all returns one from single adapter");
}

static void test_computed_symbolic_user_builders(void) {
    nocturne_context_contribution_t computed = nocturne_build_computed_context(3600, 8.0f, 8.0f, 12);
    require_true(computed.source_domain == NOCTURNE_CONTEXT_COMPUTED, "computed context domain set");
    require_true(computed.silence.value >= 0.0f, "computed silence non-negative");
    require_true(nocturne_compute_solar_cycle_ratio(-1) == 0.0f, "negative solar ratio floors");

    nocturne_symbolic_bias_t bias = nocturne_symbolic_bias_for("root", NOCTURNE_SYMBOLIC_CHAKRA);
    nocturne_context_contribution_t symbolic = nocturne_build_symbolic_context(bias);
    require_true(symbolic.source_domain == NOCTURNE_CONTEXT_SYMBOLIC, "symbolic context domain set");
    require_true(strcmp(bias.label, "root") == 0, "symbolic label preserved");

    nocturne_user_controls_t controls = {
        .mode = NOCTURNE_MODE_FOCUS,
        .user_brightness_bias = 0.35f,
        .user_warmth_bias = 0.25f,
        .user_density_cap = 0.55f,
        .sleep_safe_mode = true,
        .parental_controls_enabled = true,
    };

    nocturne_context_contribution_t user = nocturne_build_user_context(&controls);
    require_true(user.source_domain == NOCTURNE_CONTEXT_USER, "user context domain set");
    require_true(user.silence.value >= 0.0f, "parental control sets user silence");
}

static void test_fusion_log_round_trip(void) {
    nocturne_fusion_log_t log;
    nocturne_fusion_log_init(&log, true);
    require_true(log.count == 0, "fusion log starts empty");

    nocturne_fusion_result_t result = {0};
    result.frame.intent.density = 0.4f;
    result.conflict.conflict_detected = 1;
    result.conflict.dimension = 3;
    result.conflict.winning_domain = NOCTURNE_CONTEXT_OBSERVED;
    result.conflict.runnerup_domain = NOCTURNE_CONTEXT_COMPUTED;
    result.conflict.conflict_margin = 0.05f;

    require_true(nocturne_fusion_log_append(&log, &result), "append writes when telemetry enabled");
    require_true(log.count == 1, "log count increments");

    nocturne_fused_frame_t replay = {0};
    require_true(nocturne_fusion_log_replay(&log, 0, &replay), "replay returns stored frame");
    require_true(replay.intent.density == 0.4f, "replayed payload preserved");
}

int main(void) {
    test_context_utilities();
    test_compatibility_and_schema();
    test_chronology_defaults();
    test_seed_determinism_and_advance();
    test_profiles_and_fusion_cap();
    test_adapters_and_registry();
    test_computed_symbolic_user_builders();
    test_fusion_log_round_trip();

    printf("core_unit_test passed\n");
    return 0;
}
