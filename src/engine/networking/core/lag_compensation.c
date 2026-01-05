/**
 * @file lag_compensation.c
 * @brief Server-side Hit Verification.
 *
 * Implements "Rewind Time" functionality to validate shots against
 * past entity positions.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <networking/core/lag_compensation.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef struct HitboxSnapshot {
  uint32_t tick;
  AABB bounds;
  vec3 position;
  quat rotation;
} HitboxSnapshot;

typedef struct HistoryBuffer {
  uint32_t entity_id;
  HitboxSnapshot history[64]; // ~1 second at 60Hz
  int head_idx;
} HistoryBuffer;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

/**
 * @brief Records current state of hitboxes for an entity.
 */
void lag_record_frame(HistoryBuffer *buff, Entity *e, uint32_t current_tick) {
  int idx = (buff->head_idx + 1) % 64;
  buff->head_idx = idx;

  HitboxSnapshot *snap = &buff->history[idx];
  snap->tick = current_tick;
  // ... Copy collider data ...
  snap->bounds = e->collider.bounds;
  snap->position = e->transform.position;
}

/**
 * @brief Rewinds entity to state at `target_time`.
 *
 * Interpolates between two closest snapshots.
 */
void lag_rewind_entity(Entity *e, HistoryBuffer *buff, float target_time) {
  // Find snapshots A and B around target_time
  // ... search logic ...

  // Lerp positions
  // e->transform.position = vec3_lerp(A.pos, B.pos, alpha);

  // Update collider world bounds based on rewound position
  physics_update_collider(e);
}

/**
 * @brief Verify a hitscan shot.
 *
 * 1. Rewind all potential targets to `client_timestamp`.
 * 2. Perform raycast.
 * 3. Restore all targets to current time.
 */
bool lag_verify_hit(uint32_t shooter_id, vec3 ray_origin, vec3 ray_dir,
                    float timestamp) {
  // 1. Rewind
  for (int i = 0; i < world_entity_count; i++) {
    lag_rewind_entity(world_entities[i], &buffers[i], timestamp);
  }

  // 2. Cast
  RaycastResult hit;
  bool did_hit = physics_raycast(ray_origin, ray_dir, 1000.0f, &hit);

  // 3. Restore (Logic often uses a "backup" struct to restore quickly)
  // ... restore logic ...

  return did_hit;
}
