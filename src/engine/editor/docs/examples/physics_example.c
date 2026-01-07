// examples/physics_example.c
//
// Purpose: Example usage of the physics system
//
#include <core/logger.h>
#include <math/vec3.h>
#include <physics/physics.h>
#include <stdio.h>
#include <stdlib.h>

// Example: Basic physics setup and simulation
void example_basic_physics(void) {
  printf("=== Basic Physics Example ===\n");
  
  // Initialize physics configuration
  physics_config_init();
  
  // Load Minecraft preset for game-like physics
  physics_config_load_preset_by_name("minecraft");
  
  // Create physics world
  PhysicsConfig config = physics_config_get_current();
  PhysicsWorld *world = physics_world_create(config);
  
  // Create ground (static box)
  Vec3 ground_pos = vec3(0, -5, 0);
  Vec3 ground_size = vec3(20, 1, 20);
  PhysicsMaterial ground_material = physics_material_get_by_name("stone");
  RigidBody *ground = physics_create_box(world, ground_pos, ground_size, 0.0f, &ground_material);
  
  // Create falling blocks (dynamic boxes)
  RigidBody *blocks[5];
  for (int i = 0; i < 5; i++) {
    Vec3 block_pos = vec3(i * 2.0f - 4.0f, 10.0f + i * 2.0f, 0);
    Vec3 block_size = vec3(0.98f, 0.98f, 0.98f); // Slightly smaller than 1x1x1
    PhysicsMaterial block_material = physics_material_get_by_name("stone");
    
    blocks[i] = physics_create_box(world, block_pos, block_size, 1.0f, &block_material);
  }
  
  // Simulate for a few seconds
  printf("Simulating falling blocks...\n");
  for (int frame = 0; frame < 300; frame++) { // 5 seconds at 60 FPS
    physics_world_step(world, 1.0f / 60.0f);
    
    // Print positions every 60 frames (1 second)
    if (frame % 60 == 0) {
      printf("Frame %d:\n", frame);
      for (int i = 0; i < 5; i++) {
        Vec3 pos = rigid_body_get_position(blocks[i]);
        printf("  Block %d: y=%.2f\n", i, pos.y);
      }
    }
  }
  
  // Cleanup
  for (int i = 0; i < 5; i++) {
    rigid_body_destroy(blocks[i]);
  }
  rigid_body_destroy(ground);
  physics_world_destroy(world);
  
  printf("Basic physics example completed.\n\n");
}

// Example: Character controller usage
void example_character_controller(void) {
  printf("=== Character Controller Example ===\n");
  
  // Initialize systems
  physics_config_init();
  character_controller_init_system();
  
  // Create physics world with Minecraft settings
  PhysicsConfig config = physics_config_get_minecraft();
  PhysicsWorld *world = physics_world_create(config);
  
  // Create ground
  Vec3 ground_pos = vec3(0, -5, 0);
  Vec3 ground_size = vec3(20, 1, 20);
  RigidBody *ground = physics_create_box(world, ground_pos, ground_size, 0.0f, NULL);
  
  // Create character controller
  Vec3 player_start = vec3(0, 2, 0);
  CharacterController *player = character_controller_create(world, player_start);
  
  // Configure character
  character_controller_set_move_speed(player, 5.0f);
  character_controller_set_jump_height(player, 1.2f);
  character_controller_set_slope_limit(player, 45.0f);
  
  printf("Character controller created at y=%.2f\n", player_start.y);
  
  // Simulate character falling and jumping
  for (int frame = 0; frame < 180; frame++) { // 3 seconds
    f32 delta_time = 1.0f / 60.0f;
    
    // Let character fall initially
    if (frame < 60) {
      character_controller_update(player, world, delta_time);
    }
    // Make character jump after landing
    else if (frame == 60) {
      if (character_controller_is_grounded(player)) {
        character_controller_jump(player, world);
        printf("Jump! (grounded: %s)\n", 
               character_controller_is_grounded(player) ? "true" : "false");
      }
      character_controller_update(player, world, delta_time);
    }
    // Continue simulation
    else {
      character_controller_update(player, world, delta_time);
    }
    
    // Print status every 30 frames
    if (frame % 30 == 0) {
      Vec3 pos = character_controller_get_position(player);
      Vec3 vel = character_controller_get_velocity(player);
      printf("Frame %d: pos(%.2f, %.2f, %.2f) vel(%.2f, %.2f, %.2f) grounded=%s\n",
             frame, pos.x, pos.y, pos.z, vel.x, vel.y, vel.z,
             character_controller_is_grounded(player) ? "true" : "false");
    }
  }
  
  // Test movement
  printf("\nTesting movement...\n");
  Vec3 move_direction = vec3(1, 0, 0); // Move right
  
  for (int frame = 0; frame < 60; frame++) {
    character_controller_move(player, world, move_direction, 1.0f / 60.0f);
  }
  
  Vec3 final_pos = character_controller_get_position(player);
  printf("Final position after moving right: x=%.2f\n", final_pos.x);
  
  // Cleanup
  character_controller_destroy(player);
  rigid_body_destroy(ground);
  physics_world_destroy(world);
  character_controller_cleanup_system();
  
  printf("Character controller example completed.\n\n");
}

// Example: Raycasting for line of sight
void example_raycasting(void) {
  printf("=== Raycasting Example ===\n");
  
  physics_config_init();
  
  PhysicsConfig config = physics_config_get_default();
  PhysicsWorld *world = physics_world_create(config);
  
  // Create some obstacles
  Vec3 obstacle_positions[] = {
    vec3(0, 0, 5),
    vec3(2, 1, 8),
    vec3(-1, 0, 12)
  };
  
  RigidBody *obstacles[3];
  for (int i = 0; i < 3; i++) {
    obstacles[i] = physics_create_box(world, obstacle_positions[i], 
                                     vec3(1, 2, 1), 1.0f, NULL);
  }
  
  // Cast rays from player position
  Vec3 player_pos = vec3(0, 1, 0);
  
  printf("Casting rays from player position (%.1f, %.1f, %.1f):\n", 
         player_pos.x, player_pos.y, player_pos.z);
  
  // Cast rays in different directions
  Vec3 directions[] = {
    vec3(0, 0, 1),   // Forward
    vec3(1, 0, 1),   // Forward-right
    vec3(-1, 0, 1),  // Forward-left
    vec3(0, 0.2f, 1) // Slightly upward
  };
  
  const char *direction_names[] = {
    "Forward", "Forward-right", "Forward-left", "Upward"
  };
  
  for (int i = 0; i < 4; i++) {
    RaycastResult result = physics_raycast(world, player_pos, directions[i], 20.0f);
    
    if (result.hit) {
      printf("  %s: HIT at distance %.2f, point(%.2f, %.2f, %.2f)\n",
             direction_names[i], result.hit_distance,
             result.hit_point.x, result.hit_point.y, result.hit_point.z);
    } else {
      printf("  %s: No hit (max distance 20.0)\n", direction_names[i]);
    }
  }
  
  // Cleanup
  for (int i = 0; i < 3; i++) {
    rigid_body_destroy(obstacles[i]);
  }
  physics_world_destroy(world);
  
  printf("Raycasting example completed.\n\n");
}

// Example: Material system usage
void example_materials(void) {
  printf("=== Materials Example ===\n");
  
  physics_config_init();
  
  // List all available materials
  printf("Available material presets:\n");
  physics_material_list_presets();
  
  // Create different objects with different materials
  PhysicsConfig config = physics_config_get_default();
  PhysicsWorld *world = physics_world_create(config);
  
  // Create ground
  Vec3 ground_pos = vec3(0, -5, 0);
  RigidBody *ground = physics_create_box(world, ground_pos, vec3(10, 1, 10), 0.0f, NULL);
  
  // Create objects with different materials
  struct {
    const char *name;
    Vec3 position;
    const char *material;
  } objects[] = {
    {"Stone Block", vec3(-2, 5, 0), "stone"},
    {"Ice Block", vec3(0, 5, 0), "ice"},
    {"Rubber Ball", vec3(2, 5, 0), "rubber"},
    {"Wood Block", vec3(4, 5, 0), "wood"}
  };
  
  RigidBody *bodies[4];
  
  for (int i = 0; i < 4; i++) {
    PhysicsMaterial material = physics_material_get_by_name(objects[i].material);
    bodies[i] = physics_create_box(world, objects[i].position, vec3(0.8f, 0.8f, 0.8f), 
                                   1.0f, &material);
    
    printf("Created %s with %s material (friction=%.2f, restitution=%.2f)\n",
           objects[i].name, objects[i].material, material.friction, material.restitution);
  }
  
  // Simulate and observe different behaviors
  printf("\nSimulating material behaviors...\n");
  
  for (int frame = 0; frame < 120; frame++) { // 2 seconds
    physics_world_step(world, 1.0f / 60.0f);
    
    // Print positions every 60 frames
    if (frame % 60 == 0) {
      printf("Frame %d:\n", frame);
      for (int i = 0; i < 4; i++) {
        Vec3 pos = rigid_body_get_position(bodies[i]);
        Vec3 vel = rigid_body_get_velocity(bodies[i]);
        printf("  %s: y=%.2f, bounce=%.2f\n", 
               objects[i].name, pos.y, fabsf(vel.y));
      }
    }
  }
  
  // Cleanup
  for (int i = 0; i < 4; i++) {
    rigid_body_destroy(bodies[i]);
  }
  rigid_body_destroy(ground);
  physics_world_destroy(world);
  
  printf("Materials example completed.\n\n");
}

int main(void) {
  printf("Physics System Examples\n");
  printf("======================\n\n");
  
  // Run all examples
  example_basic_physics();
  example_character_controller();
  example_raycasting();
  example_materials();
  
  printf("All examples completed successfully!\n");
  
  return 0;
}
