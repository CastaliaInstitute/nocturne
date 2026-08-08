#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t seed;
    uint64_t session_id;
    uint64_t sequence;
} nocturne_seed_state_t;

static inline uint64_t nocturne_seed_init(uint64_t base_seed, uint64_t session_id) {
    return (base_seed ^ (session_id << 1u)) + 0x9E3779B97F4A7C15ULL;
}

static inline uint64_t nocturne_next_seed(uint64_t current) {
    uint64_t x = current;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x * 2685821657736338717ULL;
}

static inline void nocturne_seed_advance(nocturne_seed_state_t* state) {
    if (!state) {
        return;
    }
    state->seed = nocturne_next_seed(state->seed);
    state->sequence += 1;
}

#ifdef __cplusplus
}
#endif
