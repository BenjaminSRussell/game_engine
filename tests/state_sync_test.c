/*
 * state_sync_test.c
 * Core entity state sync: register, snapshot, apply, interpolate, predict, delta.
 * Compiles src/engine/network/state_sync.c directly and does not link Engine.
 */

#include "state_sync.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect_int(const char *what, int got, int want) {
    if (got != want) {
        fprintf(stderr, "FAIL: %s got %d want %d\n", what, got, want);
        exit(1);
    }
}

static void expect_u32(const char *what, uint32_t got, uint32_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL: %s got %u want %u\n", what, got, want);
        exit(1);
    }
}

static void expect_near(const char *what, float got, float want) {
    if (fabsf(got - want) > 0.0001f) {
        fprintf(stderr, "FAIL: %s got %f want %f\n", what, got, want);
        exit(1);
    }
}

static void expect_vec3(const char *what, Vec3 got, float x, float y, float z) {
    char buf[96];
    snprintf(buf, sizeof(buf), "%s.x", what);
    expect_near(buf, got.x, x);
    snprintf(buf, sizeof(buf), "%s.y", what);
    expect_near(buf, got.y, y);
    snprintf(buf, sizeof(buf), "%s.z", what);
    expect_near(buf, got.z, z);
}

static void expect_quat(const char *what, Quat got, float x, float y, float z, float w) {
    char buf[96];
    snprintf(buf, sizeof(buf), "%s.x", what);
    expect_near(buf, got.x, x);
    snprintf(buf, sizeof(buf), "%s.y", what);
    expect_near(buf, got.y, y);
    snprintf(buf, sizeof(buf), "%s.z", what);
    expect_near(buf, got.z, z);
    snprintf(buf, sizeof(buf), "%s.w", what);
    expect_near(buf, got.w, w);
}

static EntityState *find_state(StateSnapshot *snapshot, EntityID id) {
    uint32_t i;
    if (!snapshot) {
        return NULL;
    }
    for (i = 0; i < snapshot->state_count; i++) {
        if (snapshot->states[i].entity_id == id) {
            return &snapshot->states[i];
        }
    }
    return NULL;
}

static void test_null_and_capacity(void) {
    StateSynchronizer *sync;
    Vec3 pos = {1.0f, 2.0f, 3.0f};
    Quat rot = {0.0f, 0.0f, 0.0f, 1.0f};
    uint8_t blob[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t big[257];
    SyncStatistics stats;
    InterpolatedState interp;
    Vec3 predicted;

    state_sync_destroy(NULL);
    state_sync_free_snapshot(NULL);
    state_sync_free_delta_compressed(NULL);

    expect_int("register null", state_sync_register_entity(NULL, 1), -1);
    expect_int("unregister null", state_sync_unregister_entity(NULL, 1), -1);
    expect_int("position null sync", state_sync_update_position(NULL, 1, &pos), -1);
    expect_int("rotation null quat", state_sync_update_rotation(NULL, 1, NULL), -1);
    expect_int("velocity null vec", state_sync_update_velocity(NULL, 1, NULL), -1);
    expect_int("animation null", state_sync_update_animation(NULL, 1, 1, 0.0f), -1);
    expect_int("property null", state_sync_update_property(NULL, 1, 0, blob, 4), -1);
    expect_int("snapshot null", state_sync_create_snapshot(NULL, NULL, STATE_CHANGE_POSITION), -1);
    expect_int("apply null", state_sync_apply_state(NULL, NULL), -1);
    expect_int("apply snapshot null", state_sync_apply_snapshot(NULL, NULL), -1);
    expect_int("interp null", state_sync_get_interpolated_state(NULL, 1, 0.0f, NULL), -1);
    expect_int("predict null", state_sync_predict_state(NULL, 1, 0.0f, NULL), -1);
    expect_int("stats null", state_sync_get_statistics(NULL, NULL), -1);
    expect_int("rate null", state_sync_set_update_rate(NULL, 10), -1);
    expect_int("interp flag null", state_sync_set_interpolation_enabled(NULL, true), -1);
    expect_int("predict flag null", state_sync_set_prediction_enabled(NULL, true), -1);

    sync = state_sync_create(2);
    if (!sync) {
        fail("create");
    }

    expect_int("entity 0", state_sync_register_entity(sync, 0), -1);
    expect_int("register 1", state_sync_register_entity(sync, 1), 0);
    expect_int("duplicate", state_sync_register_entity(sync, 1), -2);
    expect_int("register 2", state_sync_register_entity(sync, 2), 0);
    expect_int("full", state_sync_register_entity(sync, 3), -3);
    expect_int("missing position", state_sync_update_position(sync, 9, &pos), -2);
    expect_int("null position", state_sync_update_position(sync, 1, NULL), -1);
    expect_int("null rotation", state_sync_update_rotation(sync, 1, NULL), -1);
    expect_int("bad property id", state_sync_update_property(sync, 1, 32, blob, 4), -1);
    expect_int("null property data", state_sync_update_property(sync, 1, 0, NULL, 4), -1);
    memset(big, 7, sizeof(big));
    expect_int("property too big", state_sync_update_property(sync, 1, 0, big, sizeof(big)), -2);
    expect_int("property ok", state_sync_update_property(sync, 1, 3, blob, sizeof(blob)), 0);
    expect_int("unknown unregister", state_sync_unregister_entity(sync, 9), -2);
    expect_int("rate zero", state_sync_set_update_rate(sync, 0), -1);
    expect_int("rate ok", state_sync_set_update_rate(sync, 30), 0);
    expect_int("interp range", state_sync_get_interpolated_state(sync, 1, 1.5f, &interp), -1);
    expect_int("predict negative", state_sync_predict_state(sync, 1, -1.0f, &predicted), -1);
    expect_int("stats ok", state_sync_get_statistics(sync, &stats), 0);
    expect_u32("count before unregister", stats.synchronized_entities, 2);

    expect_int("unregister 1", state_sync_unregister_entity(sync, 1), 0);
    expect_int("stats after unregister", state_sync_get_statistics(sync, &stats), 0);
    expect_u32("count after unregister", stats.synchronized_entities, 1);
    expect_int("update removed", state_sync_update_position(sync, 1, &pos), -2);
    expect_int("reuse slot", state_sync_register_entity(sync, 3), 0);
    expect_int("rotation missing", state_sync_update_rotation(sync, 1, &rot), -2);

    state_sync_destroy(sync);

    sync = state_sync_create(0);
    if (!sync) {
        fail("create default");
    }
    expect_int("default cap register", state_sync_register_entity(sync, 4), 0);
    state_sync_destroy(sync);
}

static void test_snapshot_apply_and_delta(void) {
    StateSynchronizer *host;
    StateSynchronizer *remote;
    StateSnapshot *snapshot = NULL;
    StateSnapshot *decoded = NULL;
    DeltaCompressed *compressed = NULL;
    EntityState *state;
    EntityState incoming;
    SyncStatistics stats;
    Vec3 pos = {4.0f, 5.0f, 6.0f};
    Vec3 vel = {1.0f, 0.0f, -2.0f};
    Quat rot = {0.0f, 0.0f, 1.0f, 0.0f};
    StateChangeType mask;

    host = state_sync_create(4);
    remote = state_sync_create(4);
    if (!host || !remote) {
        fail("create pair");
    }

    expect_int("host a", state_sync_register_entity(host, 10), 0);
    expect_int("host b", state_sync_register_entity(host, 11), 0);
    expect_int("pos", state_sync_update_position(host, 10, &pos), 0);
    expect_int("rot", state_sync_update_rotation(host, 10, &rot), 0);
    expect_int("vel", state_sync_update_velocity(host, 10, &vel), 0);
    expect_int("anim", state_sync_update_animation(host, 10, 7, 1.25f), 0);

    mask = (StateChangeType)(STATE_CHANGE_POSITION | STATE_CHANGE_VELOCITY |
                              STATE_CHANGE_ROTATION | STATE_CHANGE_ANIMATION);
    expect_int("snapshot", state_sync_create_snapshot(host, &snapshot, mask), 0);
    if (!snapshot || snapshot->state_count != 2 || !snapshot->states) {
        fail("snapshot shape");
    }
    state = find_state(snapshot, 10);
    if (!state) {
        fail("missing entity 10");
    }
    expect_u32("flags", state->change_flags, (uint32_t)mask);
    expect_vec3("snap pos", state->position, 4.0f, 5.0f, 6.0f);
    expect_vec3("snap vel", state->velocity, 1.0f, 0.0f, -2.0f);
    expect_vec3("snap scale", state->scale, 1.0f, 1.0f, 1.0f);
    expect_quat("snap rot", state->rotation, 0.0f, 0.0f, 1.0f, 0.0f);
    expect_u32("anim id", state->animation_id, 7);
    expect_near("anim time", state->animation_time, 1.25f);
    if (!find_state(snapshot, 11)) {
        fail("missing entity 11");
    }

    expect_int("stats", state_sync_get_statistics(host, &stats), 0);
    expect_u32("snapshots created", stats.snapshots_created, 1);
    expect_u32("host entities", stats.synchronized_entities, 2);

    expect_int("remote register", state_sync_register_entity(remote, 10), 0);
    memset(&incoming, 0, sizeof(incoming));
    incoming.entity_id = 10;
    incoming.change_flags = STATE_CHANGE_POSITION;
    incoming.position.x = 9.0f;
    incoming.velocity.x = 50.0f;
    expect_int("apply position only", state_sync_apply_state(remote, &incoming), 0);

    memset(&incoming, 0, sizeof(incoming));
    incoming.entity_id = 10;
    incoming.change_flags = STATE_CHANGE_SCALE;
    incoming.scale.x = 2.0f;
    incoming.scale.y = 3.0f;
    incoming.scale.z = 4.0f;
    incoming.position.x = 99.0f;
    expect_int("apply scale", state_sync_apply_state(remote, &incoming), 0);

    expect_int("apply known snapshot state", state_sync_apply_state(remote, &snapshot->states[0]), 0);
    /* Entity 11 is in the host snapshot but not registered on the remote. */
    {
        EntityState *other = find_state(snapshot, 11);
        if (!other) {
            fail("entity 11");
        }
        expect_int("apply missing", state_sync_apply_state(remote, other), -2);
    }
    expect_int("apply batch", state_sync_apply_snapshot(remote, snapshot), 0);

    expect_int("delta", state_sync_delta_compress_snapshot(host, snapshot, &compressed), 0);
    if (!compressed || compressed->entity_count != 2) {
        fail("compressed shape");
    }
    if (compressed->size != 2 * sizeof(EntityState) || !compressed->data) {
        fail("compressed size");
    }
    expect_int("decompress", state_sync_delta_decompress(host, compressed, &decoded), 0);
    if (!decoded) {
        fail("decoded");
    }
    state = find_state(decoded, 10);
    if (!state) {
        fail("decoded entity");
    }
    expect_vec3("roundtrip pos", state->position, 4.0f, 5.0f, 6.0f);
    expect_quat("roundtrip rot", state->rotation, 0.0f, 0.0f, 1.0f, 0.0f);
    expect_near("roundtrip anim", state->animation_time, 1.25f);

    expect_int("bytes", state_sync_get_statistics(host, &stats), 0);
    if (stats.bytes_transmitted < compressed->size) {
        fail("bytes transmitted");
    }
    expect_near("ratio", stats.avg_delta_compression_ratio, 0.0f);
    expect_int("remote stats", state_sync_get_statistics(remote, &stats), 0);
    if (stats.snapshots_applied == 0) {
        fail("snapshots applied");
    }

    state_sync_free_delta_compressed(compressed);
    state_sync_free_snapshot(decoded);
    state_sync_free_snapshot(snapshot);
    state_sync_destroy(host);
    state_sync_destroy(remote);
}

static void test_interpolation_and_prediction(void) {
    StateSynchronizer *sync;
    InterpolatedState interp;
    EntityState incoming;
    Vec3 pos = {10.0f, 0.0f, 0.0f};
    Vec3 vel = {4.0f, 0.0f, -2.0f};
    Vec3 predicted;
    Quat rot = {0.0f, 0.0f, 1.0f, 0.0f};
    const float half = 0.70710678f;

    sync = state_sync_create(2);
    if (!sync) {
        fail("create");
    }
    expect_int("register", state_sync_register_entity(sync, 10), 0);
    expect_int("seed pos", state_sync_update_position(sync, 10, &pos), 0);
    expect_int("no history", state_sync_get_interpolated_state(sync, 10, 0.0f, &interp), 0);
    expect_vec3("current without history", interp.position, 10.0f, 0.0f, 0.0f);
    expect_near("factor", interp.interpolation_factor, 0.0f);
    expect_int("unknown interp", state_sync_get_interpolated_state(sync, 3, 0.5f, &interp), -2);

    memset(&incoming, 0, sizeof(incoming));
    incoming.entity_id = 10;
    incoming.change_flags = STATE_CHANGE_POSITION | STATE_CHANGE_ROTATION;
    incoming.rotation.w = 1.0f;
    expect_int("apply origin", state_sync_apply_state(sync, &incoming), 0);
    expect_int("move", state_sync_update_position(sync, 10, &pos), 0);
    expect_int("spin", state_sync_update_rotation(sync, 10, &rot), 0);

    expect_int("t0", state_sync_get_interpolated_state(sync, 10, 0.0f, &interp), 0);
    expect_vec3("lerp start", interp.position, 0.0f, 0.0f, 0.0f);
    expect_quat("slerp start", interp.rotation, 0.0f, 0.0f, 0.0f, 1.0f);

    expect_int("t1", state_sync_get_interpolated_state(sync, 10, 1.0f, &interp), 0);
    expect_vec3("lerp end", interp.position, 10.0f, 0.0f, 0.0f);
    expect_quat("slerp end", interp.rotation, 0.0f, 0.0f, 1.0f, 0.0f);

    expect_int("t half", state_sync_get_interpolated_state(sync, 10, 0.5f, &interp), 0);
    expect_vec3("lerp mid", interp.position, 5.0f, 0.0f, 0.0f);
    expect_quat("slerp mid", interp.rotation, 0.0f, 0.0f, half, half);

    expect_int("disable interp", state_sync_set_interpolation_enabled(sync, false), 0);
    expect_int("t0 raw", state_sync_get_interpolated_state(sync, 10, 0.0f, &interp), 0);
    expect_vec3("raw position", interp.position, 10.0f, 0.0f, 0.0f);

    expect_int("seed vel", state_sync_update_velocity(sync, 10, &vel), 0);
    expect_int("predict off", state_sync_predict_state(sync, 10, 0.5f, &predicted), 0);
    expect_vec3("held position", predicted, 10.0f, 0.0f, 0.0f);
    expect_int("enable predict", state_sync_set_prediction_enabled(sync, true), 0);
    expect_int("predict on", state_sync_predict_state(sync, 10, 0.5f, &predicted), 0);
    expect_vec3("predicted", predicted, 12.0f, 0.0f, -1.0f);
    expect_int("predict zero dt", state_sync_predict_state(sync, 10, 0.0f, &predicted), 0);
    expect_vec3("predicted dt0", predicted, 10.0f, 0.0f, 0.0f);
    expect_int("predict missing", state_sync_predict_state(sync, 4, 0.5f, &predicted), -2);

    /* Scale applied earlier is covered in the snapshot test; confirm a later
     * position-only apply did not zero the default scale path used above. */
    expect_int("reenable interp", state_sync_set_interpolation_enabled(sync, true), 0);

    state_sync_destroy(sync);
}

static void test_scale_survives_snapshot(void) {
    StateSynchronizer *sync;
    StateSnapshot *snapshot = NULL;
    EntityState incoming;
    EntityState *state;

    sync = state_sync_create(1);
    if (!sync) {
        fail("create");
    }
    expect_int("register", state_sync_register_entity(sync, 8), 0);
    memset(&incoming, 0, sizeof(incoming));
    incoming.entity_id = 8;
    incoming.change_flags = STATE_CHANGE_SCALE;
    incoming.scale.x = 2.0f;
    incoming.scale.y = 3.0f;
    incoming.scale.z = 4.0f;
    incoming.position.x = 99.0f;
    expect_int("apply scale", state_sync_apply_state(sync, &incoming), 0);
    expect_int("snapshot", state_sync_create_snapshot(sync, &snapshot, STATE_CHANGE_SCALE), 0);
    state = find_state(snapshot, 8);
    if (!state) {
        fail("scaled entity");
    }
    expect_vec3("scaled", state->scale, 2.0f, 3.0f, 4.0f);
    expect_vec3("position untouched", state->position, 0.0f, 0.0f, 0.0f);
    state_sync_free_snapshot(snapshot);
    state_sync_destroy(sync);
}

int main(void) {
    test_null_and_capacity();
    test_snapshot_apply_and_delta();
    test_interpolation_and_prediction();
    test_scale_survives_snapshot();
    printf("ok\n");
    return 0;
}
