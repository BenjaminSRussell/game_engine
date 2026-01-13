/**
 * @file mass_system.c
 * @brief ECS Implementation
 * @description Entity Component System implementation
 * @date 2026-01-13
 */

#include "../include/ecs/mass_system.h"
#include "../include/core/memory.h"
#include "../include/math/vec3.h"
#include <stdlib.h>
#include <string.h>

// ----------------------------------------------------------------------------
// Internal Storage
// ----------------------------------------------------------------------------
// Structure of Arrays (SoA) for cache efficiency
static struct {
  MassFragment_Transform *transforms;
  MassFragment_Movement *movements;
  u8 *active_flags;

  u32 capacity;
  u32 count;
} g_mass_context;

// ----------------------------------------------------------------------------
// Implementation
// ----------------------------------------------------------------------------

void mass_system_init(u32 max_entities) {
  g_mass_context.capacity = max_entities;
  g_mass_context.count = 0;

  // Allocate SoA buffers
  g_mass_context.transforms = (MassFragment_Transform *)malloc(
      sizeof(MassFragment_Transform) * max_entities);
  g_mass_context.movements = (MassFragment_Movement *)malloc(
      sizeof(MassFragment_Movement) * max_entities);
  g_mass_context.active_flags = (u8 *)calloc(max_entities, sizeof(u8));
}

void mass_system_shutdown(void) {
  if (g_mass_context.transforms)
    free(g_mass_context.transforms);
  if (g_mass_context.movements)
    free(g_mass_context.movements);
  if (g_mass_context.active_flags)
    free(g_mass_context.active_flags);
  g_mass_context.capacity = 0;
  g_mass_context.count = 0;
}

MassEntity mass_entity_spawn(Vec3 position, Vec3 velocity) {
  if (g_mass_context.count >= g_mass_context.capacity)
    return 0xFFFFFFFF; // Full

  // Simple bump allocation for V1 (reusing slots requires a free list)
  // Here we just append.
  u32 id = g_mass_context.count++;

  g_mass_context.active_flags[id] = 1;
  g_mass_context.transforms[id].position = position;
  g_mass_context.transforms[id].velocity = velocity;
  g_mass_context.transforms[id].scale = 1.0f;

  // Defaults
  g_mass_context.movements[id].move_speed = 1.0f; // m/s
  g_mass_context.movements[id].avoidance_radius = 0.5f;
  g_mass_context.movements[id].target_location = position; // Stay put initially

  return id;
}

void mass_entity_set_target(MassEntity entity, Vec3 target) {
  if (entity < g_mass_context.count) {
    g_mass_context.movements[entity].target_location = target;
  }
}

const MassFragment_Transform *mass_entity_get_transform(MassEntity entity) {
  if (entity < g_mass_context.count) {
    return &g_mass_context.transforms[entity];
  }
  return NULL;
}

// Parallel-friendly update (though written linearly here for simplicity)
void mass_system_update(float dt) {
  // Phase 1: AI / Steering Calculation
  for (u32 i = 0; i < g_mass_context.count; i++) {
    if (!g_mass_context.active_flags[i])
      continue;

    MassFragment_Transform *tf = &g_mass_context.transforms[i];
    MassFragment_Movement *mv = &g_mass_context.movements[i];

    // 1. Seek Target
    Vec3 to_target = vec3_sub(mv->target_location, tf->position);
    float dist = vec3_length(to_target);

    Vec3 desired_velocity = {0, 0, 0};
    if (dist > 0.1f) {
      desired_velocity = vec3_mul(vec3_normalize(to_target), mv->move_speed);
    }

    // 2. Simple Avoidance (Brute force O(N^2) - would use grid/hash in prod)
    // For V1 (small N test), we'll skip or do very limited check
    // ... (Skipping O(N^2) loop to avoid freezing implementation)

    // 3. Integrate
    // Blend current velocity towards desired
    float steering_speed = 5.0f;
    tf->velocity =
        vec3_lerp(tf->velocity, desired_velocity, dt * steering_speed);

    // euler integration
    tf->position = vec3_add(tf->position, vec3_mul(tf->velocity, dt));

    // 4. Update Rotation (Face velocity)
    if (vec3_length_sq(tf->velocity) > 0.01f) {
      tf->rotation =
          quat_look_rotation(vec3_normalize(tf->velocity), vec3_up());
    }
  }
}
