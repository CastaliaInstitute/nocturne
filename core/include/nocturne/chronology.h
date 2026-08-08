#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NOCTURNE_SEASON_SPRING = 0,
    NOCTURNE_SEASON_SUMMER = 1,
    NOCTURNE_SEASON_AUTUMN = 2,
    NOCTURNE_SEASON_WINTER = 3
} nocturne_season_t;

typedef struct {
    int32_t year;
    nocturne_season_t season;
    int32_t week_of_year;
    int32_t day_of_year;
    int32_t day_of_week;
    int32_t hour_of_day;
    int32_t minute_of_hour;
    int32_t breath_cycle_index;
    int32_t heartbeat_cycle_index;
    float sleep_phase_ratio;
    float lunar_phase_ratio;
    float solar_cycle_ratio;
} nocturne_chronology_t;

static inline nocturne_chronology_t nocturne_default_chronology(void) {
    nocturne_chronology_t c = {0};
    c.year = 2026;
    c.season = NOCTURNE_SEASON_SPRING;
    c.week_of_year = 1;
    c.day_of_year = 1;
    c.day_of_week = 1;
    c.hour_of_day = 12;
    c.minute_of_hour = 0;
    c.sleep_phase_ratio = 0.5f;
    c.lunar_phase_ratio = 0.0f;
    c.solar_cycle_ratio = 0.5f;
    return c;
}

#ifdef __cplusplus
}
#endif
