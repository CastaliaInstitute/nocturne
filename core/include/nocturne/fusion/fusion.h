#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    nocturne_influence_t weights[NOCTURNE_INTENT_DIMENSIONS];
    float anti_dominance_cap;
    float confidence_floor;
    float confidence_ceil;
    float trust_decay_per_cycle;
    float conflict_threshold;
} nocturne_fusion_profile_t;

typedef struct {
    size_t dimension;
    nocturne_context_domain_t winning_domain;
    nocturne_context_domain_t runnerup_domain;
    float dominant_signal;
    float runnerup_signal;
    float conflict_margin;
    char detail[64];
    uint8_t conflict_detected;
} nocturne_fusion_conflict_report_t;

typedef struct {
    char reason[64];
    float weighted_value;
    float confidence;
    nocturne_context_domain_t winning_domain;
} nocturne_explainability_entry_t;

typedef struct {
    nocturne_fused_frame_t frame;
    nocturne_explainability_entry_t entries[NOCTURNE_INTENT_DIMENSIONS];
    uint32_t explain_count;
    nocturne_fusion_conflict_report_t conflict;
} nocturne_fusion_result_t;

static inline const char* nocturne_intent_name(size_t index) {
    static const char* names[NOCTURNE_INTENT_DIMENSIONS] = {
        "grounding",
        "brightness",
        "warmth",
        "density",
        "motion",
        "spaciousness",
        "silence",
        "event_activity",
        "harmonic_tension",
        "novelty",
    };
    if (index >= NOCTURNE_INTENT_DIMENSIONS) {
        return "unknown";
    }
    return names[index];
}

static inline const nocturne_influence_t* nocturne_contrib_influence(const nocturne_context_contribution_t* sample, size_t index) {
    if (!sample || index >= NOCTURNE_INTENT_DIMENSIONS) {
        return NULL;
    }
    switch (index) {
        case 0:
            return &sample->grounding;
        case 1:
            return &sample->brightness;
        case 2:
            return &sample->warmth;
        case 3:
            return &sample->density;
        case 4:
            return &sample->motion;
        case 5:
            return &sample->spaciousness;
        case 6:
            return &sample->silence;
        case 7:
            return &sample->event_activity;
        case 8:
            return &sample->harmonic_tension;
        default:
            return &sample->novelty;
    }
}

static inline float* nocturne_fuse_intent_slot(nocturne_acoustic_intent_t* intent, size_t index) {
    if (!intent || index >= NOCTURNE_INTENT_DIMENSIONS) {
        return NULL;
    }
    switch (index) {
        case 0:
            return &intent->grounding;
        case 1:
            return &intent->brightness;
        case 2:
            return &intent->warmth;
        case 3:
            return &intent->density;
        case 4:
            return &intent->motion;
        case 5:
            return &intent->spaciousness;
        case 6:
            return &intent->silence;
        case 7:
            return &intent->event_activity;
        case 8:
            return &intent->harmonic_tension;
        default:
            return &intent->novelty;
    }
}

static inline float nocturne_clamp_zero_to_one(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static inline float nocturne_confidence_for(const nocturne_fusion_profile_t* profile, float confidence) {
    if (!profile) {
        return confidence;
    }
    return nocturne_clamp_float(confidence, 0.0f, 1.0f);
}

static inline float nocturne_weighted_value(
    const nocturne_context_contribution_t* sample,
    size_t dimension,
    const nocturne_fusion_profile_t* profile,
    size_t sample_index,
    float* source_confidence_accum
) {
    (void)sample_index;
    if (!sample || !profile || !source_confidence_accum) {
        if (source_confidence_accum) {
            *source_confidence_accum = 0.0f;
        }
        return 0.0f;
    }

    const nocturne_influence_t* influence = nocturne_contrib_influence(sample, dimension);
    if (!influence) {
        *source_confidence_accum = 0.0f;
        return 0.0f;
    }

    float clamped_confidence = nocturne_confidence_for(profile, influence->confidence);
    float weighted = influence->value * clamped_confidence * profile->weights[dimension].provenance_weight;
    weighted = nocturne_clamp_zero_to_one(weighted);
    if (weighted > profile->anti_dominance_cap) {
        weighted = profile->anti_dominance_cap;
    }

    *source_confidence_accum = clamped_confidence * (profile->weights[dimension].confidence);
    return weighted;
}

static inline void nocturne_detect_conflicts(
    const float* dominant,
    const float* runnerup,
    const nocturne_context_domain_t* dominant_domain,
    const nocturne_context_domain_t* runnerup_domain,
    size_t n,
    float threshold,
    nocturne_fusion_conflict_report_t* out
) {
    if (!out) {
        return;
    }
    out->dimension = 0;
    out->winning_domain = NOCTURNE_CONTEXT_OBSERVED;
    out->runnerup_domain = NOCTURNE_CONTEXT_OBSERVED;
    out->dominant_signal = 0.0f;
    out->runnerup_signal = 0.0f;
    out->conflict_margin = 0.0f;
    out->conflict_detected = 0;
    out->detail[0] = '\0';

    if (!dominant || !runnerup || !dominant_domain || !runnerup_domain || n == 0) {
        return;
    }
    if (threshold <= 0.0f) {
        threshold = 0.01f;
    }

    float smallest_margin = 1.0f;
    size_t best_dim = 0;
    for (size_t i = 0; i < n; ++i) {
        if (!nocturne_domain_ok(dominant_domain[i])) {
            continue;
        }
        if (!nocturne_domain_ok(runnerup_domain[i])) {
            continue;
        }
        if (dominant_domain[i] == runnerup_domain[i]) {
            continue;
        }
        if (runnerup[i] <= 0.0f) {
            continue;
        }
        float margin = dominant[i] - runnerup[i];
        if (margin < 0.0f) {
            margin = -margin;
        }
        if (margin < smallest_margin) {
            smallest_margin = margin;
            best_dim = i;
        }
    }

    if (smallest_margin > threshold || best_dim >= n) {
        return;
    }

    out->dimension = best_dim;
    out->winning_domain = dominant_domain[best_dim];
    out->runnerup_domain = runnerup_domain[best_dim];
    out->dominant_signal = dominant[best_dim];
    out->runnerup_signal = runnerup[best_dim];
    out->conflict_margin = smallest_margin;
    out->conflict_detected = 1;

    const char* winner = nocturne_domain_ok(dominant_domain[best_dim]) ? "observed" : "unknown";
    if (dominant_domain[best_dim] == NOCTURNE_CONTEXT_COMPUTED) winner = "computed";
    else if (dominant_domain[best_dim] == NOCTURNE_CONTEXT_SYMBOLIC) winner = "symbolic";
    else if (dominant_domain[best_dim] == NOCTURNE_CONTEXT_USER) winner = "user";

    const char* runnerup_name = nocturne_domain_ok(runnerup_domain[best_dim]) ? "observed" : "unknown";
    if (runnerup_domain[best_dim] == NOCTURNE_CONTEXT_COMPUTED) runnerup_name = "computed";
    else if (runnerup_domain[best_dim] == NOCTURNE_CONTEXT_SYMBOLIC) runnerup_name = "symbolic";
    else if (runnerup_domain[best_dim] == NOCTURNE_CONTEXT_USER) runnerup_name = "user";

    (void)snprintf(out->detail, sizeof(out->detail),
                   "close disagreement on %s between %s and %s (margin %.3f)",
                   nocturne_intent_name(best_dim),
                   winner,
                   runnerup_name,
                   smallest_margin);
}

static inline uint8_t nocturne_apply_conflict_mitigation(
    size_t index,
    float* destination,
    const nocturne_fusion_conflict_report_t* report,
    float anti_dominance_cap
) {
    if (!destination || !report) {
        return 0;
    }
    if (!report->conflict_detected || report->dimension != index) {
        return 0;
    }

    float blended = (report->dominant_signal + report->runnerup_signal) * 0.5f;
    if (blended < 0.0f) {
        blended = 0.0f;
    }
    if (blended > anti_dominance_cap) {
        blended = anti_dominance_cap;
    }
    *destination = blended;
    return 1;
}

static inline nocturne_fused_frame_t nocturne_fuse(
    const nocturne_fusion_profile_t* profile,
    const nocturne_context_contribution_t* samples,
    size_t sample_count,
    const nocturne_fused_frame_t* previous_frame
) {
    nocturne_fused_frame_t out;
    memset(&out, 0, sizeof(out));
    out.smoothing_alpha = profile ? 0.2f : 0.2f;
    out.confidence_cap = profile ? profile->confidence_ceil : 1.0f;
    out.valid_inputs = 0;
    out.domain_count = NOCTURNE_CONTEXT_USER + 1;

    if (!profile || !samples || sample_count == 0) {
        if (previous_frame) {
            out = *previous_frame;
        }
        return out;
    }

    float accum[NOCTURNE_INTENT_DIMENSIONS] = {0.0f};
    float source_confidence_sum[NOCTURNE_INTENT_DIMENSIONS] = {0.0f};

    for (size_t sample_index = 0; sample_index < sample_count; ++sample_index) {
        const nocturne_context_contribution_t* sample = &samples[sample_index];
        if (!sample || !nocturne_domain_ok(sample->source_domain)) {
            continue;
        }
        for (size_t d = 0; d < NOCTURNE_INTENT_DIMENSIONS; ++d) {
            float conf_weight = 0.0f;
            float weighted = nocturne_weighted_value(sample, d, profile, sample_index, &conf_weight);
            accum[d] += weighted;
            source_confidence_sum[d] += conf_weight;
        }
    }

    float denom = (float)sample_count;
    float baseline[NOCTURNE_INTENT_DIMENSIONS] = {0.0f};
    for (size_t d = 0; d < NOCTURNE_INTENT_DIMENSIONS; ++d) {
        if (denom <= 0.0f) {
            baseline[d] = 0.0f;
        } else {
            baseline[d] = nocturne_clamp_zero_to_one((accum[d] / denom) * profile->trust_decay_per_cycle);
        }
        float conf = source_confidence_sum[d] / denom;
        out.influences[d].value = baseline[d];
        out.influences[d].confidence = nocturne_clamp_float(conf, profile->confidence_floor, profile->confidence_ceil);
        out.influences[d].min_bound = profile->confidence_floor;
        out.influences[d].max_bound = profile->confidence_ceil;
        out.influences[d].provenance_weight = profile->weights[d].provenance_weight;
    }

    for (size_t d = 0; d < NOCTURNE_INTENT_DIMENSIONS; ++d) {
        float previous = previous_frame ? ((float*)&previous_frame->intent)[d] : 0.5f;
        float smoothed = nocturne_smooth(previous, baseline[d], out.smoothing_alpha);
        float* slot = nocturne_fuse_intent_slot(&out.intent, d);
        if (!slot) continue;
        *slot = smoothed;
    }

    if (out.smoothing_alpha <= 0.0f && previous_frame) {
        out.intent = previous_frame->intent;
    }
    out.last_update_ms = previous_frame ? previous_frame->last_update_ms : 0;
    out.valid_inputs = (uint32_t)sample_count;
    return out;
}

static inline nocturne_fusion_result_t nocturne_fuse_with_explainability(
    const nocturne_fusion_profile_t* profile,
    const nocturne_context_contribution_t* samples,
    size_t sample_count,
    const nocturne_fused_frame_t* previous_frame,
    bool conflict_mitigation_enabled
) {
    nocturne_fusion_result_t result;
    memset(&result, 0, sizeof(result));
    if (!profile) {
        static nocturne_fusion_profile_t fallback_profile;
        memset(&fallback_profile, 0, sizeof(fallback_profile));
        fallback_profile.anti_dominance_cap = 1.0f;
        fallback_profile.confidence_floor = 0.1f;
        fallback_profile.confidence_ceil = 1.0f;
        fallback_profile.trust_decay_per_cycle = 1.0f;
        fallback_profile.conflict_threshold = 0.10f;
        profile = &fallback_profile;
    }

    float accum[NOCTURNE_INTENT_DIMENSIONS] = {0.0f};
    float source_confidence_sum[NOCTURNE_INTENT_DIMENSIONS] = {0.0f};
    nocturne_context_domain_t top_domain[NOCTURNE_INTENT_DIMENSIONS];
    nocturne_context_domain_t runnerup_domain[NOCTURNE_INTENT_DIMENSIONS];
    float top_signal[NOCTURNE_INTENT_DIMENSIONS] = {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f};
    float runnerup_signal[NOCTURNE_INTENT_DIMENSIONS] = {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f};

    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        top_domain[i] = NOCTURNE_CONTEXT_OBSERVED;
        runnerup_domain[i] = NOCTURNE_CONTEXT_OBSERVED;
    }

    if (profile && samples && sample_count > 0) {
        for (size_t sample_index = 0; sample_index < sample_count; ++sample_index) {
            const nocturne_context_contribution_t* sample = &samples[sample_index];
            if (!sample || !nocturne_domain_ok(sample->source_domain)) {
                continue;
            }
            for (size_t d = 0; d < NOCTURNE_INTENT_DIMENSIONS; ++d) {
                float conf_weight = 0.0f;
                float weighted = nocturne_weighted_value(sample, d, profile, sample_index, &conf_weight);
                accum[d] += weighted;
                source_confidence_sum[d] += conf_weight;

                if (weighted > top_signal[d]) {
                    runnerup_signal[d] = top_signal[d];
                    runnerup_domain[d] = top_domain[d];
                    top_signal[d] = weighted;
                    top_domain[d] = sample->source_domain;
                } else if (weighted > runnerup_signal[d]) {
                    runnerup_signal[d] = weighted;
                    runnerup_domain[d] = sample->source_domain;
                }
            }
        }
        for (size_t d = 0; d < NOCTURNE_INTENT_DIMENSIONS; ++d) {
            if (runnerup_signal[d] < 0.0f) {
                runnerup_signal[d] = 0.0f;
                runnerup_domain[d] = top_domain[d];
            }
        }
    }

    result.frame = nocturne_fuse(profile, samples, sample_count, previous_frame);

    nocturne_detect_conflicts(
        top_signal,
        runnerup_signal,
        top_domain,
        runnerup_domain,
        NOCTURNE_INTENT_DIMENSIONS,
        profile->conflict_threshold,
        &result.conflict
    );

    result.explain_count = NOCTURNE_INTENT_DIMENSIONS;
    for (uint32_t i = 0; i < result.explain_count; ++i) {
        result.entries[i].weighted_value = ((const float*)&result.frame.intent)[i];
    result.entries[i].confidence = result.frame.influences[i].confidence;
    result.entries[i].winning_domain = top_domain[i];

        if (result.conflict.conflict_detected && result.conflict.dimension == i) {
            result.entries[i].winning_domain = result.conflict.winning_domain;
            (void)snprintf(result.entries[i].reason, sizeof(result.entries[i].reason),
                           "conflict softened: winner=%s run=%s margin=%.3f",
                           result.conflict.winning_domain == NOCTURNE_CONTEXT_COMPUTED ? "computed" :
                               (result.conflict.winning_domain == NOCTURNE_CONTEXT_SYMBOLIC ? "symbolic" :
                                (result.conflict.winning_domain == NOCTURNE_CONTEXT_USER ? "user" : "observed")),
                           result.conflict.runnerup_domain == NOCTURNE_CONTEXT_COMPUTED ? "computed" :
                               (result.conflict.runnerup_domain == NOCTURNE_CONTEXT_SYMBOLIC ? "symbolic" :
                                (result.conflict.runnerup_domain == NOCTURNE_CONTEXT_USER ? "user" : "observed")),
                           result.conflict.conflict_margin);
        } else {
            (void)snprintf(result.entries[i].reason, sizeof(result.entries[i].reason),
                           "blended from weighted contributions");
        }

        if (conflict_mitigation_enabled
            && nocturne_apply_conflict_mitigation(i, &((float*)&result.frame.intent)[i], &result.conflict, profile->anti_dominance_cap)) {
            result.entries[i].weighted_value = ((const float*)&result.frame.intent)[i];
            /* Prefix the existing reason via a copy: snprintf source and
               destination must not overlap, and precision keeps the
               combined string within the destination. */
            char prior_reason[sizeof(result.entries[i].reason)];
            (void)snprintf(prior_reason, sizeof(prior_reason), "%s", result.entries[i].reason);
            (void)snprintf(result.entries[i].reason, sizeof(result.entries[i].reason),
                           "conflict mitigation applied: %.*s",
                           (int)(sizeof(result.entries[i].reason) - 30), prior_reason);
        }
    }

    return result;
}

#ifdef __cplusplus
}
#endif
