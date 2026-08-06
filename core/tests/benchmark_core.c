#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#include <time.h>

#include "../include/nocturne/scheduler/scheduler.h"

static nocturne_fused_frame_t make_frame(float base, uint64_t now_ms) {
    nocturne_fused_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    frame.intent.grounding = base;
    frame.intent.brightness = base + 0.02f;
    frame.intent.warmth = base + 0.01f;
    frame.intent.density = base + 0.03f;
    frame.intent.motion = base + 0.04f;
    frame.intent.spaciousness = base + 0.05f;
    frame.intent.silence = 0.05f;
    frame.intent.event_activity = base + 0.06f;
    frame.intent.harmonic_tension = base + 0.07f;
    frame.intent.novelty = base + 0.08f;

    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        frame.influences[i].confidence = 0.75f;
        frame.influences[i].provenance_weight = 0.6f;
        frame.influences[i].value = 0.2f + 0.005f * (float)i;
    }

    frame.last_update_ms = now_ms;
    return frame;
}

static double seconds_since(const struct timespec* start, const struct timespec* end) {
    return (double)(end->tv_sec - start->tv_sec) + ((double)(end->tv_nsec - start->tv_nsec) / 1e9);
}

int main(void) {
    const int iterations = 5000;
    struct timespec t0;
    struct timespec t1;
    struct timespec t_startup_0;
    struct timespec t_startup_1;
    struct timespec frame_start;
    struct timespec frame_end;

    if (clock_gettime(CLOCK_MONOTONIC, &t_startup_0) != 0) {
        perror("clock_gettime");
        return 1;
    }

    nocturne_scheduler_state_t state = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_FOCUS, NULL);
    const uint32_t phase_length_ms = 1000;
    nocturne_scheduler_set_phase(&state, 1, 1, phase_length_ms);

    if (clock_gettime(CLOCK_MONOTONIC, &t_startup_1) != 0) {
        perror("clock_gettime");
        return 1;
    }

    nocturne_fused_frame_t frame;
    uint64_t last_plan_id = 0;
    uint64_t monotonic_min = UINT64_MAX;
    uint64_t monotonic_max = 0;
    double total_frame_ns = 0.0;
    double max_frame_ns = 0.0;
    double min_frame_ns = DBL_MAX;

    if (clock_gettime(CLOCK_MONOTONIC, &t0) != 0) {
        perror("clock_gettime");
        return 1;
    }

    for (int i = 0; i < iterations; ++i) {
        float base = (float)((i % 100) + 1) / 110.0f;
        frame = make_frame(base, (uint64_t)(i * 17 + 1));
        if (clock_gettime(CLOCK_MONOTONIC, &frame_start) != 0) {
            perror("clock_gettime");
            return 1;
        }
        nocturne_scheduler_plan_t plan = nocturne_scheduler_schedule_once(&state, &frame, (i & 1) == 0, frame.last_update_ms);
        (void)plan;
        if (state.monotonic_ms < monotonic_min) monotonic_min = state.monotonic_ms;
        if (state.monotonic_ms > monotonic_max) monotonic_max = state.monotonic_ms;
        last_plan_id++;

        if (clock_gettime(CLOCK_MONOTONIC, &frame_end) != 0) {
            perror("clock_gettime");
            return 1;
        }
        const double frame_ns = (double)(frame_end.tv_sec - frame_start.tv_sec) * 1.0e9
            + (double)(frame_end.tv_nsec - frame_start.tv_nsec);
        if (frame_ns < min_frame_ns) {
            min_frame_ns = frame_ns;
        }
        if (frame_ns > max_frame_ns) {
            max_frame_ns = frame_ns;
        }
        total_frame_ns += frame_ns;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0) {
        perror("clock_gettime");
        return 1;
    }

    const double secs = seconds_since(&t0, &t1);
    const double fps = (double)iterations / (secs > 0.0 ? secs : 1.0);
    const double drift_ms = (double)(monotonic_max - monotonic_min) - ((double)(iterations - 1) * 17.0);
    const double startup_ms = seconds_since(&t_startup_0, &t_startup_1) * 1000.0;
    const double avg_frame_us = (total_frame_ns / (double)iterations) / 1000.0;
    const double min_frame_us = min_frame_ns / 1000.0;
    const double max_frame_us = max_frame_ns / 1000.0;

    assert(last_plan_id == (uint64_t)iterations);
    printf("benchmark iterations=%d startup_ms=%.6f average_frame_time_us=%.6f min_frame_time_us=%.6f max_frame_time_us=%.6f fps=%.2f drift_ms=%.6f\n",
           iterations,
           startup_ms,
           avg_frame_us,
           min_frame_us,
           max_frame_us,
           fps,
           drift_ms);

    return 0;
}
