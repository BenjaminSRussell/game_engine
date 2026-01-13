/**
 * @file physics_replay_test.c
 * @brief Test for deterministic physics replay
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "../src/engine/physics/world/physics_world_manager.h"
#include "../src/engine/physics/networking/physics_replay.h"

// Helper to compare floats
bool float_equals(float a, float b, float epsilon) {
    return fabsf(a - b) < epsilon;
}

// Helper to compare vectors
bool vec3_equals(const float *a, const float *b, float epsilon) {
    return float_equals(a[0], b[0], epsilon) &&
           float_equals(a[1], b[1], epsilon) &&
           float_equals(a[2], b[2], epsilon);
}

int main() {
    printf("Starting Physics Replay Test...\n");

    // 1. Setup World
    PhysicsWorldConfig config = physics_world_get_default_config();
    PhysicsWorld *world = physics_world_create(&config);
    if (!world) {
        printf("FAILED: Could not create physics world\n");
        return 1;
    }

    // Create a material
    uint32_t mat_id = physics_world_create_material(world, "TestMat", 1.0f, 0.5f, 0.5f);
    PhysicsMaterial *mat = physics_world_get_material(world, mat_id);

    // Create a dynamic object (Cube)
    uint64_t cube_id = physics_world_create_object(world, PHYSICS_OBJECT_RIGID_BODY, mat);
    float start_pos[3] = {0.0f, 10.0f, 0.0f};
    physics_world_set_object_transform(world, cube_id,
        (float[]){1,0,0,0, 0,1,0,0, 0,0,1,0, start_pos[0], start_pos[1], start_pos[2], 1});

    // 2. Setup Replay
    PhysicsReplaySystem *replay = physics_replay_create(world);
    if (!replay) {
        printf("FAILED: Could not create replay system\n");
        return 1;
    }

    // 3. Record Session
    printf("Recording session...\n");
    if (!physics_replay_start_recording(replay)) {
        printf("FAILED: Could not start recording\n");
        return 1;
    }

    // Run for 60 frames
    // Apply a force at frame 10
    for (int i = 0; i < 60; i++) {
        if (i == 10) {
            float force[3] = {100.0f, 0.0f, 0.0f};
            float point[3] = {0.0f, 10.0f, 0.0f}; // Center of mass approx
            physics_replay_apply_force(replay, cube_id, force, point);
            printf("  Frame %d: Applied force {%.1f, %.1f, %.1f}\n", i, force[0], force[1], force[2]);
        }

        // Use replay update which also steps the world
        physics_replay_update(replay);
    }

    physics_replay_stop_recording(replay);

    // Capture final state
    float recorded_final_pos[3];
    float recorded_final_rot[4]; // quaternion is not stored in transform in our simple API wrapper but lets inspect transform
    float recorded_transform[16];
    physics_world_get_object_transform(world, cube_id, recorded_transform);
    recorded_final_pos[0] = recorded_transform[12];
    recorded_final_pos[1] = recorded_transform[13];
    recorded_final_pos[2] = recorded_transform[14];

    printf("Recorded Final Position: {%.4f, %.4f, %.4f}\n",
           recorded_final_pos[0], recorded_final_pos[1], recorded_final_pos[2]);

    // 4. Replay Session
    printf("Replaying session...\n");
    if (!physics_replay_start_replay(replay)) {
        printf("FAILED: Could not start replay\n");
        return 1;
    }

    // Verify reset happened
    float reset_transform[16];
    physics_world_get_object_transform(world, cube_id, reset_transform);
    float reset_pos[3] = {reset_transform[12], reset_transform[13], reset_transform[14]};
    printf("Reset Position: {%.4f, %.4f, %.4f}\n", reset_pos[0], reset_pos[1], reset_pos[2]);

    if (!vec3_equals(reset_pos, start_pos, 0.001f)) {
        printf("FAILED: World did not reset to initial state correctly\n");
        return 1;
    }

    // Run for 60 frames
    for (int i = 0; i < 60; i++) {
        physics_replay_update(replay);
    }

    physics_replay_stop_replay(replay);

    // Capture replay final state
    float replay_final_pos[3];
    float replay_transform[16];
    physics_world_get_object_transform(world, cube_id, replay_transform);
    replay_final_pos[0] = replay_transform[12];
    replay_final_pos[1] = replay_transform[13];
    replay_final_pos[2] = replay_transform[14];

    printf("Replay Final Position:   {%.4f, %.4f, %.4f}\n",
           replay_final_pos[0], replay_final_pos[1], replay_final_pos[2]);

    // 5. Verification
    if (vec3_equals(recorded_final_pos, replay_final_pos, 0.0001f)) {
        printf("SUCCESS: Replay is deterministic!\n");
    } else {
        printf("FAILED: Replay diverged!\n");
        printf("  Diff: {%.6f, %.6f, %.6f}\n",
               fabsf(recorded_final_pos[0] - replay_final_pos[0]),
               fabsf(recorded_final_pos[1] - replay_final_pos[1]),
               fabsf(recorded_final_pos[2] - replay_final_pos[2]));
        return 1;
    }

    physics_replay_destroy(replay);
    physics_world_destroy(world);

    return 0;
}
