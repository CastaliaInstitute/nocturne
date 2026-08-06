#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core/include/nocturne/voice/voice_manager.h"

static nocturne_fused_frame_t make_frame(float novelty, float harmonic, float motion, float event_activity) {
    nocturne_fused_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.intent.novelty = novelty;
    frame.intent.harmonic_tension = harmonic;
    frame.intent.motion = motion;
    frame.intent.event_activity = event_activity;
    frame.intent.brightness = 0.5f;
    frame.intent.silence = 0.2f;
    frame.last_update_ms = 1000;
    for (size_t i = 0; i < NOCTURNE_INTENT_DIMENSIONS; ++i) {
        frame.influences[i].value = 0.5f;
        frame.influences[i].confidence = 0.9f;
        frame.influences[i].provenance_weight = 0.8f;
        frame.influences[i].min_bound = 0.0f;
        frame.influences[i].max_bound = 1.0f;
    }
    return frame;
}

static void test_lifecycle_spawn_fade_retire(void) {
    nocturne_voice_manager_t manager;
    nocturne_voice_manager_init(&manager, 42, 7);
    nocturne_fused_frame_t frame = make_frame(0.2f, 0.2f, 0.2f, 0.4f);

    const uint32_t id = nocturne_voice_spawn(&manager, NOCTURNE_VOICE_KIND_LAYER, "bed", &frame.intent, 1000);
    assert(id != 0);
    nocturne_voice_t* voice = nocturne_voice_find(&manager, id);
    assert(voice && voice->stage == NOCTURNE_VOICE_STAGE_FADING_IN);
    assert(voice->gain == 0.0f);

    for (int i = 0; i < 10; ++i) {
        nocturne_voice_update(&manager, 1000 + (uint64_t)i * 16);
    }
    voice = nocturne_voice_find(&manager, id);
    assert(voice && voice->stage == NOCTURNE_VOICE_STAGE_ACTIVE);
    assert(fabsf(voice->gain - 1.0f) < 1e-6f);

    assert(nocturne_voice_retire(&manager, id));
    for (int i = 0; i < 10; ++i) {
        nocturne_voice_update(&manager, 1200 + (uint64_t)i * 16);
    }
    assert(nocturne_voice_find(&manager, id) == NULL);
    printf("voice lifecycle spawn/fade/retire passed\n");
}

static void test_crossfade_links_partners(void) {
    nocturne_voice_manager_t manager;
    nocturne_voice_manager_init(&manager, 42, 7);
    nocturne_fused_frame_t frame = make_frame(0.2f, 0.2f, 0.2f, 0.4f);

    const uint32_t a = nocturne_voice_spawn(&manager, NOCTURNE_VOICE_KIND_STEM, "stem-a", &frame.intent, 0);
    for (int i = 0; i < 10; ++i) nocturne_voice_update(&manager, (uint64_t)i * 16);
    const uint32_t b = nocturne_voice_crossfade(&manager, a, NOCTURNE_VOICE_KIND_STEM, "stem-b", &frame.intent, 200);
    assert(b != 0 && b != a);

    nocturne_voice_t* old_voice = nocturne_voice_find(&manager, a);
    nocturne_voice_t* new_voice = nocturne_voice_find(&manager, b);
    assert(old_voice && old_voice->stage == NOCTURNE_VOICE_STAGE_FADING_OUT);
    assert(new_voice && new_voice->stage == NOCTURNE_VOICE_STAGE_FADING_IN);
    assert(old_voice->crossfade_partner_id == b);
    assert(new_voice->crossfade_partner_id == a);

    for (int i = 0; i < 12; ++i) nocturne_voice_update(&manager, 200 + (uint64_t)i * 16);
    assert(nocturne_voice_find(&manager, a) == NULL);
    new_voice = nocturne_voice_find(&manager, b);
    assert(new_voice && new_voice->stage == NOCTURNE_VOICE_STAGE_ACTIVE);
    printf("voice crossfade passed\n");
}

static void test_layering_constraints(void) {
    nocturne_voice_manager_t manager;
    nocturne_voice_manager_init(&manager, 42, 7);
    nocturne_fused_frame_t frame = make_frame(0.2f, 0.2f, 0.2f, 0.4f);

    assert(nocturne_voice_spawn(&manager, NOCTURNE_VOICE_KIND_MOTIF, "m1", &frame.intent, 0) != 0);
    assert(nocturne_voice_spawn(&manager, NOCTURNE_VOICE_KIND_MOTIF, "m2", &frame.intent, 0) != 0);
    assert(nocturne_voice_spawn(&manager, NOCTURNE_VOICE_KIND_MOTIF, "m3", &frame.intent, 0) == 0);

    for (int i = 0; i < NOCTURNE_VOICE_MAX; ++i) {
        char label[16];
        snprintf(label, sizeof(label), "l%d", i);
        (void)nocturne_voice_spawn(&manager, NOCTURNE_VOICE_KIND_LAYER, label, &frame.intent, 0);
    }
    assert(nocturne_voice_total_live(&manager) <= manager.constraints.max_total_active);
    printf("voice layering constraints passed (live=%u)\n", nocturne_voice_total_live(&manager));
}

static void test_deterministic_selection_contract(void) {
    nocturne_voice_manager_t manager_a;
    nocturne_voice_manager_t manager_b;
    nocturne_voice_manager_init(&manager_a, 1234, 99);
    nocturne_voice_manager_init(&manager_b, 1234, 99);

    nocturne_fused_frame_t frame = make_frame(0.8f, 0.3f, 0.2f, 0.7f);
    nocturne_scheduler_state_t sched = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_FOCUS, &frame);
    nocturne_scheduler_plan_t plan = nocturne_scheduler_schedule_once(&sched, &frame, false, 2000);

    for (int i = 0; i < 32; ++i) {
        nocturne_voice_event_t ea = nocturne_voice_select_event(&manager_a, &frame, &plan);
        nocturne_voice_event_t eb = nocturne_voice_select_event(&manager_b, &frame, &plan);
        assert(ea.selected == eb.selected);
        assert(ea.event_seed == eb.event_seed);
        assert(ea.kind == eb.kind);
        assert(fabsf(ea.energy - eb.energy) < 1e-9f);
        assert(fabsf(ea.pan - eb.pan) < 1e-9f);
        assert(strcmp(ea.explain, eb.explain) == 0);
    }
    printf("voice deterministic selection contract passed\n");
}

static void test_selection_explainability_labels(void) {
    nocturne_voice_manager_t manager;
    nocturne_voice_manager_init(&manager, 5, 5);

    nocturne_fused_frame_t frame = make_frame(0.9f, 0.2f, 0.1f, 0.9f);
    nocturne_scheduler_state_t sched = nocturne_scheduler_state_init(NOCTURNE_SCHEDULER_MODE_FOCUS, &frame);
    nocturne_scheduler_plan_t plan = nocturne_scheduler_schedule_once(&sched, &frame, false, 2000);
    plan.target_event_density = 1.0f;
    plan.target_novelty_gain = 0.5f;

    nocturne_voice_event_t event = nocturne_voice_select_event(&manager, &frame, &plan);
    assert(event.selected);
    assert(event.kind == NOCTURNE_VOICE_KIND_MOTIF);
    assert(strcmp(event.explain, "selected:motif_from_novelty") == 0);

    nocturne_voice_set_muted(&manager, true);
    event = nocturne_voice_select_event(&manager, &frame, &plan);
    assert(!event.selected);
    assert(strcmp(event.explain, "no_selection:muted") == 0);
    printf("voice explainability labels passed\n");
}

static void test_degraded_mode(void) {
    nocturne_voice_manager_t manager;
    nocturne_voice_manager_init(&manager, 42, 7);
    nocturne_voice_set_degraded(&manager, true);
    nocturne_fused_frame_t frame = make_frame(0.2f, 0.2f, 0.2f, 0.4f);

    uint32_t spawned = 0;
    for (int i = 0; i < 8; ++i) {
        char label[16];
        snprintf(label, sizeof(label), "d%d", i);
        if (nocturne_voice_spawn(&manager, NOCTURNE_VOICE_KIND_LAYER, label, &frame.intent, 0) != 0) {
            spawned += 1;
        }
    }
    assert(spawned <= manager.constraints.degraded_max_total_active);

    for (int i = 0; i < 20; ++i) nocturne_voice_update(&manager, (uint64_t)i * 16);
    for (size_t i = 0; i < NOCTURNE_VOICE_MAX; ++i) {
        const nocturne_voice_t* voice = &manager.voices[i];
        if (voice->stage == NOCTURNE_VOICE_STAGE_ACTIVE) {
            assert(voice->gain <= manager.constraints.degraded_gain_cap + 1e-6f);
        }
    }
    printf("voice degraded mode passed (spawned=%u)\n", spawned);
}

static void test_render_intent_and_diff(void) {
    nocturne_voice_manager_t manager;
    nocturne_voice_manager_init(&manager, 42, 7);
    nocturne_fused_frame_t frame = make_frame(0.2f, 0.2f, 0.6f, 0.4f);

    nocturne_voice_render_intent_t before = nocturne_voice_render_intent(&manager);
    assert(before.active_voices == 0);
    assert(strcmp(before.explain, "render:silent") == 0);

    (void)nocturne_voice_spawn(&manager, NOCTURNE_VOICE_KIND_LAYER, "bed", &frame.intent, 0);
    (void)nocturne_voice_spawn(&manager, NOCTURNE_VOICE_KIND_TEXTURE, "wind", &frame.intent, 0);
    for (int i = 0; i < 10; ++i) nocturne_voice_update(&manager, (uint64_t)i * 16);

    nocturne_voice_render_intent_t after = nocturne_voice_render_intent(&manager);
    assert(after.active_voices == 2);
    assert(after.voices_by_kind[NOCTURNE_VOICE_KIND_LAYER] == 1);
    assert(after.voices_by_kind[NOCTURNE_VOICE_KIND_TEXTURE] == 1);
    assert(after.total_gain > 1.9f);
    assert(strcmp(after.explain, "render:active") == 0);

    char labels[128];
    const uint32_t changes = nocturne_voice_render_diff(&before, &after, labels, sizeof(labels));
    assert(changes >= 2);
    assert(strstr(labels, "voices") != NULL);
    assert(strstr(labels, "gain") != NULL);

    nocturne_voice_set_muted(&manager, true);
    nocturne_voice_render_intent_t muted = nocturne_voice_render_intent(&manager);
    assert(muted.total_gain == 0.0f);
    assert(strcmp(muted.explain, "render:muted") == 0);
    const uint32_t mute_changes = nocturne_voice_render_diff(&after, &muted, labels, sizeof(labels));
    assert(mute_changes >= 2);
    assert(strstr(labels, "muted") != NULL);
    printf("voice render intent + diff passed\n");
}

static void test_spatial_mapping(void) {
    nocturne_acoustic_intent_t intent;
    memset(&intent, 0, sizeof(intent));
    intent.brightness = 1.0f;
    intent.motion = 0.8f;
    intent.event_activity = 0.5f;

    nocturne_voice_spatial_t spatial = nocturne_voice_spatial_from_intent(&intent, 0x123456789ABCDEFULL);
    assert(spatial.azimuth >= -1.0f && spatial.azimuth <= 1.0f);
    assert(fabsf(spatial.elevation - 0.5f) < 1e-6f);
    assert(fabsf(spatial.motion_rate - 0.8f) < 1e-6f);
    assert(spatial.distance >= 0.0f && spatial.distance <= 1.0f);

    nocturne_voice_spatial_t repeat = nocturne_voice_spatial_from_intent(&intent, 0x123456789ABCDEFULL);
    assert(fabsf(spatial.azimuth - repeat.azimuth) < 1e-9f);
    printf("voice spatial mapping passed\n");
}

int main(void) {
    test_lifecycle_spawn_fade_retire();
    test_crossfade_links_partners();
    test_layering_constraints();
    test_deterministic_selection_contract();
    test_selection_explainability_labels();
    test_degraded_mode();
    test_render_intent_and_diff();
    test_spatial_mapping();
    printf("voice_manager_test passed\n");
    return 0;
}
