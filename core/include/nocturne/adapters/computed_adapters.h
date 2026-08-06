#pragma once

#include <math.h>
#include <time.h>
#include "../context.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline float nocturne_compute_solar_cycle_ratio(int64_t unix_seconds) {
    if (unix_seconds < 0) return 0.0f;
    double seconds_per_day = 24.0 * 60.0 * 60.0;
    double local = fmod((double)unix_seconds, seconds_per_day);
    if (local < 0) {
        local += seconds_per_day;
    }
    return (float)(local / seconds_per_day);
}

static inline float nocturne_compute_circadian_ratio(int64_t unix_seconds) {
    return nocturne_compute_solar_cycle_ratio(unix_seconds);
}

static inline float nocturne_compute_sleep_debt_ratio(float time_awake_hours, float sleep_target_hours) {
    if (sleep_target_hours <= 0.0f) return 0.0f;
    float debt = (sleep_target_hours - time_awake_hours) / sleep_target_hours;
    if (debt < 0.0f) debt = 0.0f;
    if (debt > 1.0f) debt = 1.0f;
    return debt;
}

static inline float nocturne_compute_sunrise_ratio(float solar_ratio) {
    if (solar_ratio < 0.0f) return 0.0f;
    if (solar_ratio > 1.0f) return 1.0f;
    return 1.0f - fabsf(0.5f - solar_ratio) * 2.0f;
}

static inline nocturne_season_t nocturne_compute_season_from_month(int month_one_based) {
    if (month_one_based >= 3 && month_one_based <= 5) return NOCTURNE_SEASON_SPRING;
    if (month_one_based >= 6 && month_one_based <= 8) return NOCTURNE_SEASON_SUMMER;
    if (month_one_based >= 9 && month_one_based <= 11) return NOCTURNE_SEASON_AUTUMN;
    return NOCTURNE_SEASON_WINTER;
}

static inline float nocturne_compute_lunar_ratio(int64_t unix_seconds) {
    if (unix_seconds < 0) return 0.0f;
    double lunar_period = 29.530588 * 24.0 * 60.0 * 60.0;
    double ratio = fmod((double)unix_seconds, lunar_period) / lunar_period;
    if (ratio < 0.0) ratio = 0.0;
    return (float)ratio;
}

static inline nocturne_context_contribution_t nocturne_build_computed_context(int64_t unix_seconds, float time_awake_hours, float sleep_target_hours, int month) {
    nocturne_context_contribution_t out = {0};
    out.schema_version = NOCTURNE_INTENT_SCHEMA_VERSION;
    out.seed = (uint64_t)unix_seconds;
    out.session_id = 1;
    out.monotonic_ms = (uint64_t)unix_seconds * 1000ull;
    out.source_domain = NOCTURNE_CONTEXT_COMPUTED;

    float solar = nocturne_compute_solar_cycle_ratio(unix_seconds);
    float circadian = nocturne_compute_circadian_ratio(unix_seconds);
    float sleep_debt = nocturne_compute_sleep_debt_ratio(time_awake_hours, sleep_target_hours);
    float sunrise = nocturne_compute_sunrise_ratio(solar);
    float lunar = nocturne_compute_lunar_ratio(unix_seconds);

    out.grounding.value = solar;
    out.brightness.value = circadian;
    out.warmth.value = lunar;
    out.density.value = sleep_debt;
    out.motion.value = sunrise;
    out.spaciousness.value = (float)nocturne_compute_season_from_month(month) / 3.0f;
    out.silence.value = 1.0f - lunar;
    out.event_activity.value = 0.2f;
    out.harmonic_tension.value = 0.4f;
    out.novelty.value = 0.5f;

    return out;
}

#ifdef __cplusplus
}
#endif
