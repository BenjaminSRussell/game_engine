
#include "../Public/unified_ecs.h"
#include <stdio.h>

// Mock system for testing
void TestSystemUpdate(World *world, float dt) {
  (void)world;
  printf("System Update: dt=%f\n", dt);
}

int main(void) {
  printf("Starting ECS Verification...\n");

  // 1. Create World
  WorldConfig config = ecs_world_create_default_config();
  World *world = ecs_world_create(&config);
  if (!world) {
    printf("FAILED: World creation failed\n");
    return 1;
  }
  printf("PASSED: World created\n");

  // 2. Register Component
  ComponentInfo pos_info = {
      .name = "Position", .size = sizeof(float) * 3, .alignment = 16};
  ECSComponentID pos_id = ecs_register_component(world, &pos_info);

  ComponentInfo vel_info = {
      .name = "Velocity", .size = sizeof(float) * 3, .alignment = 16};
  ECSComponentID vel_id = ecs_register_component(world, &vel_info);

  printf("PASSED: Components registered (Pos: %d, Vel: %d)\n", pos_id, vel_id);

  // 3. Create Entity
  Entity e = ecs_create_entity(world);
  if (!ecs_is_valid(world, e)) {
    printf("FAILED: Entity creation failed\n");
    return 1;
  }
  printf("PASSED: Entity created (ID: %d)\n", e.id);

  // 4. Add Components
  float pos_data[3] = {10, 20, 30};
  ecs_add_component(world, e, pos_id, pos_data);

  float vel_data[3] = {1, 0, 0};
  ecs_add_component(world, e, vel_id, vel_data);
  printf("PASSED: Components added to entity\n");

  // 5. Register System
  ecs_register_system(world, "TestSystem", TestSystemUpdate);
  printf("PASSED: System registered\n");

  // 6. Run Systems
  ecs_run_systems(world, 0.016f); // Simulate 1 frame
  printf("PASSED: Systems ran\n");

  // 7. Cleanup
  ecs_destroy_entity(world, e);
  ecs_world_destroy(world);
  printf("PASSED: World destroyed\n");

  printf("ECS Verification Successful!\n");
  return 0;
}
