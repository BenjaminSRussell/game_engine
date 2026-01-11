// Player collision/ground checks against the block grid with simple AABB tests.
// Roadmap: docs/PLAYER_WATER_ROADMAP.md.
// TODO: Implement player movement smoothing system.
// TODO: Add player movement prediction system for multiplayer.
// TODO: Implement player movement validation system.
// TODO: Add player movement statistics tracking.
// TODO: Implement player movement optimization system.
// TODO: Add player movement debugging visualization.
// TODO: Implement player movement unit testing framework.
// TODO: Add player movement documentation system.
// TODO: Implement player movement profiling system.
// TODO: Add player movement accessibility features.
#include <block/block.h>
#include <chunk/chunk.h>
#include <ecs/component_ids.h>
#include <ecs/components/transform.h>
#include <math.h>
#include <math/vec3.h>
#include <physics/physics.h>
#include <player/player.h>

// Check if player collides with blocks
bool player_check_block_collision(PlayerSystem *system, Vec3 position,
                                  Vec3 size, Vec3 *out_normal) {
  if (!system || !system->chunk_manager || !system->block_registry)
    return false;

  // Simple AABB collision check; missing chunks are treated as empty during
  // streaming.
  i32 min_x = (i32)floorf(position.x - size.x);
  i32 max_x = (i32)ceilf(position.x + size.x);
  i32 min_y = (i32)floorf(position.y - size.y);
  i32 max_y = (i32)ceilf(position.y + size.y);
  i32 min_z = (i32)floorf(position.z - size.z);
  i32 max_z = (i32)ceilf(position.z + size.z);

  for (i32 x = min_x; x <= max_x; x++) {
    for (i32 y = min_y; y <= max_y; y++) {
      for (i32 z = min_z; z <= max_z; z++) {
        ChunkPos cp = world_to_chunk_pos(x, y, z);
        Chunk *chunk = chunk_manager_get(system->chunk_manager, cp);
        if (chunk) {
          i32 local_x = x - cp.x * CHUNK_SIZE;
          i32 local_y = y - cp.y * CHUNK_SIZE;
          i32 local_z = z - cp.z * CHUNK_SIZE;

          BlockID block_id = chunk_get_block(chunk, local_x, local_y, local_z);
          const BlockType *block_type =
              block_registry_get(system->block_registry, block_id);

          if (block_is_solid(block_type)) {
            // Collision detected
            if (out_normal) {
              // Calculate collision normal (simplified)
              Vec3 block_center =
                  vec3((f32)x + 0.5f, (f32)y + 0.5f, (f32)z + 0.5f);
              Vec3 diff = vec3_sub(position, block_center);

              // Find axis with largest difference
              f32 abs_x = fabsf(diff.x);
              f32 abs_y = fabsf(diff.y);
              f32 abs_z = fabsf(diff.z);

              if (abs_x > abs_y && abs_x > abs_z) {
                *out_normal = vec3(diff.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
              } else if (abs_y > abs_z) {
                *out_normal = vec3(0.0f, diff.y > 0 ? 1.0f : -1.0f, 0.0f);
              } else {
                *out_normal = vec3(0.0f, 0.0f, diff.z > 0 ? 1.0f : -1.0f);
              }
            }
            return true;
          }
        }
      }
    }
  }

  return false;
}

// Update player physics with block collision
void player_update_physics_with_blocks(PlayerSystem *system, f32 delta_time) {
  if (!system || !system->player)
    return;

  PlayerComponent *p = system->player;

  TransformComponent *transform = (TransformComponent *)ecs_get_component(
      (World *)system->ecs_world, (Entity){p->entity_id, 0},
      TRANSFORM_COMPONENT_ID);
  if (!transform)
    return;

  // Get physics body position
  if (p->physics_body) {
    transform->position = rigid_body_get_position(p->physics_body);
  }

  // Check ground collision and sample block material below the player.
  Vec3 feet_pos = vec3(transform->position.x, transform->position.y - 0.9f,
                       transform->position.z);
  Vec3 size = vec3(0.3f, 0.1f, 0.3f);
  Vec3 normal;

  p->on_ground = player_check_block_collision(system, feet_pos, size, &normal);
  bool in_liquid = false;
  {
    i32 block_x = (i32)floorf(feet_pos.x);
    i32 block_y = (i32)floorf(feet_pos.y);
    i32 block_z = (i32)floorf(feet_pos.z);
    ChunkPos cp = world_to_chunk_pos(block_x, block_y, block_z);
    Chunk *chunk = chunk_manager_get(system->chunk_manager, cp);
    if (chunk) {
      i32 local_x = block_x - cp.x * CHUNK_SIZE;
      i32 local_y = block_y - cp.y * CHUNK_SIZE;
      i32 local_z = block_z - cp.z * CHUNK_SIZE;
      BlockID block_id = chunk_get_block(chunk, local_x, local_y, local_z);
      const BlockType *block_type =
          block_registry_get(system->block_registry, block_id);
      in_liquid = block_is_liquid(block_type);
    }
  }
  p->is_swimming = in_liquid;

  // Check body collision
  Vec3 body_pos = transform->position;
  Vec3 body_size = vec3(0.3f, 1.6f, 0.3f);

  if (player_check_block_collision(system, body_pos, body_size, &normal)) {
    Vec3 push = vec3_mul(normal, 0.02f);
    transform->position = vec3_add(transform->position, push);
    if (p->physics_body) {
      Vec3 vel = rigid_body_get_velocity(p->physics_body);
      f32 into_surface = vec3_dot(vel, normal);
      if (into_surface < 0.0f) {
        vel = vec3_sub(vel, vec3_mul(normal, into_surface));
        rigid_body_set_velocity(p->physics_body, vel);
      }
      rigid_body_set_position(p->physics_body, transform->position);
    }
  }

  // Apply gravity with simple drag and terminal velocity.
  if (!p->on_ground && !p->is_flying && p->physics_body) {
    Vec3 vel = rigid_body_get_velocity(p->physics_body);
    f32 gravity_scale = in_liquid ? 0.4f : 1.0f;
    vel.y += p->gravity * gravity_scale * delta_time;
    f32 drag = in_liquid ? 2.0f : 0.0f;
    vel.x *= (1.0f - drag * delta_time);
    vel.z *= (1.0f - drag * delta_time);
    if (vel.y < -50.0f) {
      vel.y = -50.0f;
    }
    rigid_body_set_velocity(p->physics_body, vel);
  }
}
