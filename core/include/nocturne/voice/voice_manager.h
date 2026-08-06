#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "../context.h"
#include "../seed.h"
#include "../fusion/fusion.h"
#include "../scheduler/scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NOCTURNE_VOICE_MAX 16
#define NOCTURNE_VOICE_KIND_COUNT 4
#define NOCTURNE_VOICE_LABEL_MAX 32
#define NOCTURNE_VOICE_EXPLAIN_MAX 96

typedef enum {
    NOCTURNE_VOICE_KIND_LAYER = 0,
    NOCTURNE_VOICE_KIND_STEM = 1,
    NOCTURNE_VOICE_KIND_MOTIF = 2,
    NOCTURNE_VOICE_KIND_TEXTURE = 3,
} nocturne_voice_kind_t;

typedef enum {
    NOCTURNE_VOICE_STAGE_IDLE = 0,
    NOCTURNE_VOICE_STAGE_FADING_IN = 1,
    NOCTURNE_VOICE_STAGE_ACTIVE = 2,
    NOCTURNE_VOICE_STAGE_FADING_OUT = 3,
    NOCTURNE_VOICE_STAGE_RETIRED = 4,
} nocturne_voice_stage_t;

typedef struct {
    float azimuth;
    float elevation;
    float distance;
    float motion_rate;
} nocturne_voice_spatial_t;

typedef struct {
    uint32_t voice_id;
    nocturne_voice_kind_t kind;
    nocturne_voice_stage_t stage;
    float gain;
    float target_gain;
    float fade_rate;
    uint64_t spawned_ms;
    uint64_t event_seed;
    uint32_t crossfade_partner_id;
    nocturne_voice_spatial_t spatial;
    char label[NOCTURNE_VOICE_LABEL_MAX];
} nocturne_voice_t;

typedef struct {
    uint8_t max_active_per_kind[NOCTURNE_VOICE_KIND_COUNT];
    uint8_t max_total_active;
    float default_fade_rate;
    float degraded_gain_cap;
    uint8_t degraded_max_total_active;
} nocturne_voice_constraints_t;

typedef struct {
    nocturne_voice_t voices[NOCTURNE_VOICE_MAX];
    nocturne_voice_constraints_t constraints;
    nocturne_seed_state_t rng;
    uint32_t next_voice_id;
    uint64_t monotonic_ms;
    bool degraded_mode;
    bool muted;
} nocturne_voice_manager_t;

typedef struct {
    nocturne_voice_kind_t kind;
    float energy;
    float brightness;
    float pitch_center;
    float pan;
    uint64_t event_seed;
    bool selected;
    char explain[NOCTURNE_VOICE_EXPLAIN_MAX];
} nocturne_voice_event_t;

typedef struct {
    uint32_t active_voices;
    uint32_t voices_by_kind[NOCTURNE_VOICE_KIND_COUNT];
    float total_gain;
    float spatial_spread;
    float mean_motion_rate;
    bool degraded;
    bool muted;
    char explain[NOCTURNE_VOICE_EXPLAIN_MAX];
} nocturne_voice_render_intent_t;

static inline float nocturne_voice_clamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static inline float nocturne_voice_seed_unit(uint64_t seed) {
    return (float)((double)(seed >> 11) / (double)(1ULL << 53));
}

static inline nocturne_voice_constraints_t nocturne_voice_default_constraints(void) {
    nocturne_voice_constraints_t out;
    memset(&out, 0, sizeof(out));
    out.max_active_per_kind[NOCTURNE_VOICE_KIND_LAYER] = 4;
    out.max_active_per_kind[NOCTURNE_VOICE_KIND_STEM] = 4;
    out.max_active_per_kind[NOCTURNE_VOICE_KIND_MOTIF] = 2;
    out.max_active_per_kind[NOCTURNE_VOICE_KIND_TEXTURE] = 4;
    out.max_total_active = 10;
    out.default_fade_rate = 0.15f;
    out.degraded_gain_cap = 0.5f;
    out.degraded_max_total_active = 4;
    return out;
}

static inline void nocturne_voice_manager_init(
    nocturne_voice_manager_t* manager,
    uint64_t base_seed,
    uint64_t session_id
) {
    if (!manager) return;
    memset(manager, 0, sizeof(*manager));
    manager->constraints = nocturne_voice_default_constraints();
    manager->rng.seed = nocturne_seed_init(base_seed, session_id);
    manager->rng.session_id = session_id;
    manager->rng.sequence = 0;
    manager->next_voice_id = 1;
}

static inline void nocturne_voice_set_degraded(nocturne_voice_manager_t* manager, bool degraded) {
    if (!manager) return;
    manager->degraded_mode = degraded;
}

static inline void nocturne_voice_set_muted(nocturne_voice_manager_t* manager, bool muted) {
    if (!manager) return;
    manager->muted = muted;
}

static inline uint32_t nocturne_voice_count_stage(
    const nocturne_voice_manager_t* manager,
    nocturne_voice_kind_t kind,
    bool live_only
) {
    if (!manager) return 0;
    uint32_t count = 0;
    for (size_t i = 0; i < NOCTURNE_VOICE_MAX; ++i) {
        const nocturne_voice_t* voice = &manager->voices[i];
        if (voice->stage == NOCTURNE_VOICE_STAGE_IDLE || voice->stage == NOCTURNE_VOICE_STAGE_RETIRED) {
            continue;
        }
        if (live_only && voice->stage == NOCTURNE_VOICE_STAGE_FADING_OUT) {
            continue;
        }
        if (voice->kind == kind) {
            count += 1;
        }
    }
    return count;
}

static inline uint32_t nocturne_voice_total_live(const nocturne_voice_manager_t* manager) {
    if (!manager) return 0;
    uint32_t count = 0;
    for (size_t i = 0; i < NOCTURNE_VOICE_MAX; ++i) {
        const nocturne_voice_stage_t stage = manager->voices[i].stage;
        if (stage == NOCTURNE_VOICE_STAGE_FADING_IN || stage == NOCTURNE_VOICE_STAGE_ACTIVE) {
            count += 1;
        }
    }
    return count;
}

static inline nocturne_voice_t* nocturne_voice_find(nocturne_voice_manager_t* manager, uint32_t voice_id) {
    if (!manager || voice_id == 0) return NULL;
    for (size_t i = 0; i < NOCTURNE_VOICE_MAX; ++i) {
        nocturne_voice_t* voice = &manager->voices[i];
        if (voice->voice_id == voice_id
            && voice->stage != NOCTURNE_VOICE_STAGE_IDLE
            && voice->stage != NOCTURNE_VOICE_STAGE_RETIRED) {
            return voice;
        }
    }
    return NULL;
}

static inline nocturne_voice_spatial_t nocturne_voice_spatial_from_intent(
    const nocturne_acoustic_intent_t* intent,
    uint64_t event_seed
) {
    nocturne_voice_spatial_t out;
    memset(&out, 0, sizeof(out));
    if (!intent) {
        return out;
    }
    const float jitter = nocturne_voice_seed_unit(event_seed);
    out.azimuth = (jitter * 2.0f) - 1.0f;
    out.elevation = nocturne_voice_clamp01(intent->brightness) * 0.5f;
    out.distance = nocturne_voice_clamp01(1.0f - intent->event_activity * 0.6f);
    out.motion_rate = nocturne_voice_clamp01(intent->motion);
    return out;
}

static inline uint32_t nocturne_voice_spawn(
    nocturne_voice_manager_t* manager,
    nocturne_voice_kind_t kind,
    const char* label,
    const nocturne_acoustic_intent_t* intent,
    uint64_t now_ms
) {
    if (!manager || (int)kind < 0 || (int)kind >= NOCTURNE_VOICE_KIND_COUNT) {
        return 0;
    }
    const uint8_t kind_cap = manager->constraints.max_active_per_kind[kind];
    const uint8_t total_cap = manager->degraded_mode
        ? manager->constraints.degraded_max_total_active
        : manager->constraints.max_total_active;
    if (nocturne_voice_count_stage(manager, kind, true) >= kind_cap) {
        return 0;
    }
    if (nocturne_voice_total_live(manager) >= total_cap) {
        return 0;
    }

    for (size_t i = 0; i < NOCTURNE_VOICE_MAX; ++i) {
        nocturne_voice_t* voice = &manager->voices[i];
        if (voice->stage != NOCTURNE_VOICE_STAGE_IDLE && voice->stage != NOCTURNE_VOICE_STAGE_RETIRED) {
            continue;
        }
        memset(voice, 0, sizeof(*voice));
        voice->voice_id = manager->next_voice_id++;
        voice->kind = kind;
        voice->stage = NOCTURNE_VOICE_STAGE_FADING_IN;
        voice->gain = 0.0f;
        voice->target_gain = manager->degraded_mode ? manager->constraints.degraded_gain_cap : 1.0f;
        voice->fade_rate = manager->constraints.default_fade_rate;
        voice->spawned_ms = now_ms;
        nocturne_seed_advance(&manager->rng);
        voice->event_seed = manager->rng.seed;
        voice->spatial = nocturne_voice_spatial_from_intent(intent, voice->event_seed);
        if (label) {
            (void)strncpy(voice->label, label, sizeof(voice->label) - 1);
        }
        if (now_ms > manager->monotonic_ms) {
            manager->monotonic_ms = now_ms;
        }
        return voice->voice_id;
    }
    return 0;
}

static inline bool nocturne_voice_retire(nocturne_voice_manager_t* manager, uint32_t voice_id) {
    nocturne_voice_t* voice = nocturne_voice_find(manager, voice_id);
    if (!voice) {
        return false;
    }
    voice->stage = NOCTURNE_VOICE_STAGE_FADING_OUT;
    voice->target_gain = 0.0f;
    return true;
}

static inline uint32_t nocturne_voice_crossfade(
    nocturne_voice_manager_t* manager,
    uint32_t retiring_id,
    nocturne_voice_kind_t kind,
    const char* label,
    const nocturne_acoustic_intent_t* intent,
    uint64_t now_ms
) {
    nocturne_voice_t* old_voice = nocturne_voice_find(manager, retiring_id);
    if (!manager || !old_voice) {
        return 0;
    }
    (void)nocturne_voice_retire(manager, retiring_id);
    const uint32_t new_id = nocturne_voice_spawn(manager, kind, label, intent, now_ms);
    if (new_id != 0) {
        nocturne_voice_t* new_voice = nocturne_voice_find(manager, new_id);
        if (new_voice) {
            new_voice->crossfade_partner_id = retiring_id;
            old_voice->crossfade_partner_id = new_id;
        }
    }
    return new_id;
}

static inline void nocturne_voice_update(nocturne_voice_manager_t* manager, uint64_t now_ms) {
    if (!manager) return;
    if (now_ms > manager->monotonic_ms) {
        manager->monotonic_ms = now_ms;
    }
    for (size_t i = 0; i < NOCTURNE_VOICE_MAX; ++i) {
        nocturne_voice_t* voice = &manager->voices[i];
        if (voice->stage == NOCTURNE_VOICE_STAGE_IDLE || voice->stage == NOCTURNE_VOICE_STAGE_RETIRED) {
            continue;
        }
        const float cap = manager->degraded_mode ? manager->constraints.degraded_gain_cap : 1.0f;
        if (voice->target_gain > cap) {
            voice->target_gain = cap;
        }
        if (voice->gain < voice->target_gain) {
            voice->gain += voice->fade_rate;
            if (voice->gain >= voice->target_gain) {
                voice->gain = voice->target_gain;
                if (voice->stage == NOCTURNE_VOICE_STAGE_FADING_IN) {
                    voice->stage = NOCTURNE_VOICE_STAGE_ACTIVE;
                }
            }
        } else if (voice->gain > voice->target_gain) {
            voice->gain -= voice->fade_rate;
            if (voice->gain <= voice->target_gain) {
                voice->gain = voice->target_gain;
                if (voice->stage == NOCTURNE_VOICE_STAGE_FADING_OUT && voice->gain <= 0.0f) {
                    voice->stage = NOCTURNE_VOICE_STAGE_RETIRED;
                }
            }
        } else if (voice->stage == NOCTURNE_VOICE_STAGE_FADING_IN) {
            voice->stage = NOCTURNE_VOICE_STAGE_ACTIVE;
        }
    }
}

static inline nocturne_voice_event_t nocturne_voice_select_event(
    nocturne_voice_manager_t* manager,
    const nocturne_fused_frame_t* frame,
    const nocturne_scheduler_plan_t* plan
) {
    nocturne_voice_event_t event;
    memset(&event, 0, sizeof(event));
    if (!manager || !frame || !plan) {
        (void)strncpy(event.explain, "no_selection:missing_input", sizeof(event.explain) - 1);
        return event;
    }
    if (manager->muted) {
        (void)strncpy(event.explain, "no_selection:muted", sizeof(event.explain) - 1);
        return event;
    }

    nocturne_seed_advance(&manager->rng);
    const uint64_t draw_seed = manager->rng.seed;
    const float draw = nocturne_voice_seed_unit(draw_seed);
    if (draw > plan->target_event_density) {
        (void)strncpy(event.explain, "no_selection:density_gate", sizeof(event.explain) - 1);
        return event;
    }

    const float novelty = frame->intent.novelty;
    const float harmonic = frame->intent.harmonic_tension;
    if (plan->target_novelty_gain > 0.30f && novelty > 0.5f) {
        event.kind = NOCTURNE_VOICE_KIND_MOTIF;
        (void)strncpy(event.explain, "selected:motif_from_novelty", sizeof(event.explain) - 1);
    } else if (harmonic > 0.5f) {
        event.kind = NOCTURNE_VOICE_KIND_STEM;
        (void)strncpy(event.explain, "selected:stem_from_harmonic_tension", sizeof(event.explain) - 1);
    } else if (frame->intent.motion > 0.5f) {
        event.kind = NOCTURNE_VOICE_KIND_TEXTURE;
        (void)strncpy(event.explain, "selected:texture_from_motion", sizeof(event.explain) - 1);
    } else {
        event.kind = NOCTURNE_VOICE_KIND_LAYER;
        (void)strncpy(event.explain, "selected:layer_baseline", sizeof(event.explain) - 1);
    }

    event.selected = true;
    event.event_seed = draw_seed;
    event.energy = nocturne_voice_clamp01(plan->target_loudness);
    event.brightness = nocturne_voice_clamp01(
        frame->intent.brightness * (1.0f - plan->target_high_frequency_content * 0.5f)
    );
    event.pitch_center = nocturne_voice_clamp01(0.5f + (harmonic - 0.5f) * 0.4f);
    event.pan = (nocturne_voice_seed_unit(draw_seed ^ 0xA5A5A5A5A5A5A5A5ULL) * 2.0f) - 1.0f;
    return event;
}

static inline nocturne_voice_render_intent_t nocturne_voice_render_intent(
    const nocturne_voice_manager_t* manager
) {
    nocturne_voice_render_intent_t out;
    memset(&out, 0, sizeof(out));
    if (!manager) {
        (void)strncpy(out.explain, "empty:no_manager", sizeof(out.explain) - 1);
        return out;
    }
    out.degraded = manager->degraded_mode;
    out.muted = manager->muted;
    float spread_min = 1.0f;
    float spread_max = -1.0f;
    float motion_sum = 0.0f;
    for (size_t i = 0; i < NOCTURNE_VOICE_MAX; ++i) {
        const nocturne_voice_t* voice = &manager->voices[i];
        if (voice->stage == NOCTURNE_VOICE_STAGE_IDLE || voice->stage == NOCTURNE_VOICE_STAGE_RETIRED) {
            continue;
        }
        out.active_voices += 1;
        out.voices_by_kind[voice->kind] += 1;
        out.total_gain += manager->muted ? 0.0f : voice->gain;
        motion_sum += voice->spatial.motion_rate;
        if (voice->spatial.azimuth < spread_min) spread_min = voice->spatial.azimuth;
        if (voice->spatial.azimuth > spread_max) spread_max = voice->spatial.azimuth;
    }
    if (out.active_voices > 0) {
        out.mean_motion_rate = motion_sum / (float)out.active_voices;
        out.spatial_spread = (spread_max > spread_min) ? (spread_max - spread_min) * 0.5f : 0.0f;
        (void)strncpy(out.explain, manager->muted ? "render:muted" : "render:active", sizeof(out.explain) - 1);
    } else {
        (void)strncpy(out.explain, "render:silent", sizeof(out.explain) - 1);
    }
    return out;
}

static inline uint32_t nocturne_voice_render_diff(
    const nocturne_voice_render_intent_t* before,
    const nocturne_voice_render_intent_t* after,
    char* out_labels,
    size_t out_labels_len
) {
    if (!before || !after || !out_labels || out_labels_len == 0) {
        return 0;
    }
    out_labels[0] = '\0';
    uint32_t changes = 0;
    const struct { const char* label; bool changed; } checks[] = {
        { "voices", before->active_voices != after->active_voices },
        { "gain", fabsf(before->total_gain - after->total_gain) > 0.001f },
        { "spread", fabsf(before->spatial_spread - after->spatial_spread) > 0.001f },
        { "motion", fabsf(before->mean_motion_rate - after->mean_motion_rate) > 0.001f },
        { "degraded", before->degraded != after->degraded },
        { "muted", before->muted != after->muted },
    };
    for (size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); ++i) {
        if (!checks[i].changed) {
            continue;
        }
        if (changes > 0) {
            (void)strncat(out_labels, ",", out_labels_len - strlen(out_labels) - 1);
        }
        (void)strncat(out_labels, checks[i].label, out_labels_len - strlen(out_labels) - 1);
        changes += 1;
    }
    return changes;
}

#ifdef __cplusplus
}
#endif
