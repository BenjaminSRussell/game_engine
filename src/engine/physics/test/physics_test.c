// src/engine/physics/test/physics_test.c
//
// Purpose: Basic physics system tests
//
#include <core/logger.h>
#include <math/vec3.h>
#include <physics/physics.h>
#include <stdio.h>
#include <stdlib.h>

static bool test_basic_collision(void) {
  printf("Testing basic collision detection...\n");
  
  // Create physics world
  PhysicsConfig config = physics_config_get_default();
  PhysicsWorld *world = physics_world_create(config);
  if (!world) {
    printf("FAILED: Could not create physics world\n");
    return false;
  }
  
  // Create two boxes
  Vec3 pos1 = vec3(0, 0, 0);
  Vec3 pos2 = vec3(0.5f, 0, 0); // Overlapping
  Vec3 half_extents = vec3(0.5f, 0.5f, 0.5f);
  
  RigidBody *box1 = physics_create_box(world, pos1, vec3_mul(half_extents, 2.0f), 1.0f, NULL);
  RigidBody *box2 = physics_create_box(world, pos2, vec3_mul(half_extents, 2.0f), 1.0f, NULL);
  
  if (!box1 || !box2) {
    printf("FAILED: Could not create rigid bodies\n");
    physics_world_destroy(world);
    return false;
  }
  
  // Step simulation
  physics_world_step(world, 1.0f / 60.0f);
  
  // Check if bodies are still valid
  Vec3 final_pos1 = rigid_body_get_position(box1);
  Vec3 final_pos2 = rigid_body_get_position(box2);
  
  // Bodies should have been pushed apart by collision resolution
  f32 distance = vec3_length(vec3_sub(final_pos1, final_pos2));
  if (distance > 1.1f) { // Should be greater than original size due to separation
    printf("PASSED: Collision resolution working (distance: %.3f)\n", distance);
  } else {
    printf("FAILED: Bodies still overlapping (distance: %.3f)\n", distance);
  }
  
  // Cleanup
  rigid_body_destroy(box1);
  rigid_body_destroy(box2);
  physics_world_destroy(world);
  
  return distance > 1.1f;
}

static bool test_gravity(void) {
  printf("Testing gravity...\n");
  
  PhysicsConfig config = physics_config_get_default();
  PhysicsWorld *world = physics_world_create(config);
  if (!world) {
    printf("FAILED: Could not create physics world\n");
    return false;
  }
  
  // Create a dynamic body in the air
  Vec3 start_pos = vec3(0, 10, 0);
  RigidBody *body = physics_create_sphere(world, start_pos, 0.5f, 1.0f, NULL);
  if (!body) {
    printf("FAILED: Could not create rigid body\n");
    physics_world_destroy(world);
    return false;
  }
  
  // Step simulation for 1 second
  for (int i = 0; i < 60; i++) {
    physics_world_step(world, 1.0f / 60.0f);
  }
  
  Vec3 final_pos = rigid_body_get_position(body);
  Vec3 velocity = rigid_body_get_velocity(body);
  
  // After 1 second, body should have fallen due to gravity
  f32 expected_y = start_pos.y + (config.gravity.y * 0.5f); // s = ut + 0.5at^2
  f32 tolerance = 0.5f;
  
  bool passed = fabsf(final_pos.y - expected_y) < tolerance;
  
  if (passed) {
    printf("PASSED: Gravity working (final y: %.3f, expected: %.3f)\n", 
           final_pos.y, expected_y);
  } else {
    printf("FAILED: Gravity not working (final y: %.3f, expected: %.3f)\n", 
           final_pos.y, expected_y);
  }
  
  // Cleanup
  rigid_body_destroy(body);
  physics_world_destroy(world);
  
  return passed;
}

static bool test_character_controller(void) {
  printf("Testing character controller...\n");
  
  PhysicsConfig config = physics_config_get_minecraft();
  PhysicsWorld *world = physics_world_create(config);
  if (!world) {
    printf("FAILED: Could not create physics world\n");
    return false;
  }
  
  // Create character controller
  Vec3 start_pos = vec3(0, 2, 0);
  CharacterController *controller = character_controller_create(world, start_pos);
  if (!controller) {
    printf("FAILED: Could not create character controller\n");
    physics_world_destroy(world);
    return false;
  }
  
  // Test initial state
  if (!character_controller_is_grounded(controller)) {
    printf("PASSED: Character starts in air\n");
  } else {
    printf("FAILED: Character should start in air\n");
  }
  
  // Let character fall
  for (int i = 0; i < 30; i++) {
    character_controller_update(controller, world, 1.0f / 60.0f);
  }
  
  // Check if grounded
  Vec3 final_pos = character_controller_get_position(controller);
  bool grounded = character_controller_is_grounded(controller);
  
  if (grounded && final_pos.y <= 1.0f) {
    printf("PASSED: Character landed on ground (y: %.3f)\n", final_pos.y);
  } else {
    printf("FAILED: Character did not land properly (grounded: %s, y: %.3f)\n", 
           grounded ? "true" : "false", final_pos.y);
  }
  
  // Test jump
  Vec3 jump_pos = character_controller_get_position(controller);
  character_controller_jump(controller, world);
  
  // Update for a few frames
  for (int i = 0; i < 10; i++) {
    character_controller_update(controller, world, 1.0f / 60.0f);
  }
  
  Vec3 after_jump_pos = character_controller_get_position(controller);
  bool jumped = after_jump_pos.y > jump_pos.y;
  
  if (jumped) {
    printf("PASSED: Character jump working\n");
  } else {
    printf("FAILED: Character jump not working\n");
  }
  
  // Cleanup
  character_controller_destroy(controller);
  physics_world_destroy(world);
  
  return grounded && jumped;
}

static bool test_raycast(void) {
  printf("Testing raycast...\n");
  
  PhysicsConfig config = physics_config_get_default();
  PhysicsWorld *world = physics_world_create(config);
  if (!world) {
    printf("FAILED: Could not create physics world\n");
    return false;
  }
  
  // Create a box
  Vec3 box_pos = vec3(0, 0, 0);
  RigidBody *box = physics_create_box(world, box_pos, vec3(1, 1, 1), 1.0f, NULL);
  if (!box) {
    printf("FAILED: Could not create box\n");
    physics_world_destroy(world);
    return false;
  }
  
  // Cast ray from above
  Vec3 ray_origin = vec3(0, 5, 0);
  Vec3 ray_direction = vec3(0, -1, 0);
  
  RaycastResult result = physics_raycast(world, ray_origin, ray_direction, 10.0f);
  
  if (result.hit && result.hit_distance > 4.0f && result.hit_distance < 6.0f) {
    printf("PASSED: Raycast hit box at distance %.3f\n", result.hit_distance);
  } else {
    printf("FAILED: Raycast did not work properly (hit: %s, distance: %.3f)\n", 
           result.hit ? "true" : "false", result.hit_distance);
  }
  
  // Cleanup
  rigid_body_destroy(box);
  physics_world_destroy(world);
  
  return result.hit;
}

static bool test_material_presets(void) {
  printf("Testing material presets...\n");
  
  // Test getting materials by name
  PhysicsMaterial stone = physics_material_get_by_name("stone");
  PhysicsMaterial ice = physics_material_get_by_name("ice");
  PhysicsMaterial rubber = physics_material_get_by_name("rubber");
  
  bool passed = true;
  
  // Check stone properties
  if (fabsf(stone.friction - 0.8f) < 0.01f && 
      fabsf(stone.restitution - 0.1f) < 0.01f) {
    printf("PASSED: Stone material preset correct\n");
  } else {
    printf("FAILED: Stone material preset incorrect\n");
    passed = false;
  }
  
  // Check ice properties (should be slippery)
  if (fabsf(ice.friction - 0.05f) < 0.01f) {
    printf("PASSED: Ice material preset correct\n");
  } else {
    printf("FAILED: Ice material preset incorrect\n");
    passed = false;
  }
  
  // Check rubber properties (should be bouncy)
  if (fabsf(rubber.restitution - 0.9f) < 0.01f) {
    printf("PASSED: Rubber material preset correct\n");
  } else {
    printf("FAILED: Rubber material preset incorrect\n");
    passed = false;
  }
  
  return passed;
}

int main(void) {
  printf("=== Physics System Tests ===\n\n");
  
  // Initialize systems
  physics_config_init();
  character_controller_init_system();
  
  int passed = 0;
  int total = 0;
  
  // Run tests
  total++;
  if (test_basic_collision()) passed++;
  
  total++;
  if (test_gravity()) passed++;
  
  total++;
  if (test_character_controller()) passed++;
  
  total++;
  if (test_raycast()) passed++;
  
  total++;
  if (test_material_presets()) passed++;
  
  // Cleanup
  character_controller_cleanup_system();
  
  printf("\n=== Test Results ===\n");
  printf("Passed: %d/%d tests\n", passed, total);
  
  if (passed == total) {
    printf("All tests PASSED!\n");
    return 0;
  } else {
    printf("Some tests FAILED!\n");
    return 1;
  }
}
