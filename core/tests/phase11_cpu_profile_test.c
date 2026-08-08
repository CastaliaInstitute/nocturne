/* Expose clock_gettime/CLOCK_MONOTONIC under -std=c11. */
#define _POSIX_C_SOURCE 199309L

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "../include/nocturne/scheduler/scheduler.h"

static nocturne_fused_frame_t make_frame(float base, uint64_t now_ms) {
    nocturne_fused_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    frame.intent.grounding = base;
    frame.intent.brightness = base + 0.03f;
    frame.intent.warmth = base + 0.02f;
    frame.intent.density = base + 0.01f;
    frame.intent.motion = base + 0.04f;
    frame.intent.spaciousness = base + 0.05f;
    frame.intent.silence = 0.15f;
    frame.intent.event_activity = base + 0.06f;
    frame.intent.harmonic_tension = base + 0.07f;
    frame.intent.novelty = base + 0.08f;

    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        frame.influences[i].confidence = 0.80f;
        frame.influences[i].provenance_weight = 0.60f;
        frame.influences[i].value = 0.2f + 0.005f * (float)i;
        frame.influences[i].min_bound = 0.0f;
        frame.influences[i].max_bound = 1.0f;
    }

    frame.last_update_ms = (float)now_ms;
    return frame;
}

static inline double ns_delta(const struct timespec* start, const struct timespec* end) {
    return (double)(end->tv_sec - start->tv_sec) * 1e9 + (double)(end->tv_nsec - start->tv_nsec);
}

int main(void) {
    const int iterations = 30000;
    struct timespec start;
    struct timespec end;

    nocturne_scheduler_state_t state = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_FOCUS, NULL);

    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        perror("clock_gettime");
        return 1;
    }

    double total_ns = 0.0;
    for (int i = 0; i < iterations; ++i) {
        const float base = (float)(i % 100) / 140.0f;
        const nocturne_fused_frame_t frame = make_frame(base, (uint64_t)i * 11u);
        struct timespec step_start;
        struct timespec step_end;
        if (clock_gettime(CLOCK_MONOTONIC, &step_start) != 0) {
            perror("clock_gettime");
            return 1;
        }
        const nocturne_scheduler_plan_t plan = nocturne_scheduler_schedule_once(&state, &frame, false, frame.last_update_ms);
        (void)plan;
        if (clock_gettime(CLOCK_MONOTONIC, &step_end) != 0) {
            perror("clock_gettime");
            return 1;
        }
        total_ns += ns_delta(&step_start, &step_end);
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        perror("clock_gettime");
        return 1;
    }

    const double elapsed_ns = ns_delta(&start, &end);
    const double avg_ns_per_frame = total_ns / (double)iterations;
    const double cpu_ms_per_1000_frames = (avg_ns_per_frame * 1000.0) / 1e6;

    printf("phase11_cpu_profile_test iterations=%d elapsed_ms=%.3f avg_ns_per_frame=%.1f cpu_ms_per_1000_frames=%.3f\n",
           iterations,
           elapsed_ns / 1.0e6,
           avg_ns_per_frame,
           cpu_ms_per_1000_frames);

    if (cpu_ms_per_1000_frames < 0.0 || cpu_ms_per_1000_frames > 20.0) {
        fprintf(stderr, "CPU budget outlier: %0.3f ms per 1000 frames\n", cpu_ms_per_1000_frames);
    }

    return 0;
}
