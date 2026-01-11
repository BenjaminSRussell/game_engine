#include <assert.h>
#include <audio/audio_system.h>
#include <core/engine.h>
#include <physics/physics.h>
#include <physics/physics_engine_core.h>
#include <stdio.h>
#include <stdlib.h>

void audio_test() {
  printf("  [AUDIO] Initializing... ");
  AudioSystem *audio = (AudioSystem *)calloc(1, sizeof(AudioSystem));
  assert(audio != NULL);

  // Test Initialization
  audio_system_init(audio, 32);
  assert(audio->initialized == true);

  // Test Volume Control
  audio_set_master_volume(audio, 0.5f);
  assert(audio->master_volume == 0.5f);

  // Test Cleanup
  audio_system_free(audio);
  free(audio);
  printf("PASS\n");
}

void physics_test() {
  printf("  [PHYSICS] Testing Rigid Body Dynamics... ");
  PhysicsConfig config = {.gravity = {0.0f, -9.81f, 0.0f},
                          .fixed_timestep = 1.0f / 60.0f,
                          .velocity_iterations = 6,
                          .position_iterations = 2};

  PhysicsWorld *world = physics_world_create(config);
  assert(world != NULL);

  // Create a dynamic body
  RigidBody *body = rigid_body_create(BODY_TYPE_DYNAMIC, (Vec3){0, 10, 0});
  assert(body != NULL);

  physics_world_add_body(world, body);

  // Step simulation
  for (int i = 0; i < 60; i++) {
    physics_world_step(world, 1.0f / 60.0f);
  }

  // Verify gravity applied (y should be < 10)
  Vec3 pos = rigid_body_get_position(body);
  // x = x0 + v0*t + 0.5*a*t^2 = 10 + 0 + 0.5*(-9.81)*1^2 = 10 - 4.905 = 5.095
  // With damping it might be slightly different but definitely less than 10
  assert(pos.y < 10.0f);
  assert(pos.y >
         0.0f); // Should not have fallen through floor (if we had floor)
                // Wait, my implementation had a floor check at y=0!

  printf("Position after 1s: %.2f (Expected ~5.1)\n", pos.y);

  rigid_body_destroy(body);
  physics_world_destroy(world);
  printf("PASS\n");
}

void full_integration_test() {
  printf("STARTING ENGINE VERIFICATION...\n");

  audio_test();
  physics_test();

  printf("SUCCESS: Engine Verification Passed.\n");
}
