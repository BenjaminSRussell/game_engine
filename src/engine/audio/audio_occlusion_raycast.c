#include <audio/audio_occlusion_raycast.h>
#include <audio/audio_system.h>
#include "engine/include/common.h"
#include <math.h>
#include <math/vec3.h>
#include <physics/block_physics.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * =================================================================================================
 *                                   AUDIO - OCCLUSION
 * =================================================================================================
 *
 * GOAL: Muffle sounds behind walls.
 */

void Audio_UpdateOcclusion(AudioSystem *sys, BlockPhysicsSystem *bp) {
  if (!sys || !bp)
    return;

  for (uint32_t i = 0; i < sys->max_channels; i++) {
    SoundSource *src = &sys->sources[i];
    if (!src->active) {
      src->occlusion_factor = 1.0f; // No occlusion
      continue;
    }

    // Calculate vector from listener to source
    Vec3 listener_pos = sys->listener_position;
    Vec3 source_pos = src->position;

    Vec3 to_source = vec3_sub(source_pos, listener_pos);
    f32 dist = vec3_length(to_source);

    if (dist < 0.1f) {
      src->occlusion_factor = 1.0f;
      continue;
    }

    Vec3 dir = vec3_normalize(to_source);

    // Raycast
    BlockRaycast hit = block_raycast(bp, listener_pos, dir, dist);

    if (hit.hit && hit.distance < dist - 0.5f) {
      // Direct path is blocked. Check for diffraction (obstructed vs muffled).
      // Find a vector perpendicular to the direction.
      Vec3 up = {0, 1, 0};
      if (fabsf(dir.y) > 0.9f) {
        up = (Vec3){1, 0, 0};
      }
      Vec3 side = vec3_cross(dir, up);
      side = vec3_normalize(side);

      // Check slightly to the sides to see if we're near an edge
      Vec3 left_p = vec3_add(listener_pos, vec3_mul(side, 0.5f));
      Vec3 right_p = vec3_sub(listener_pos, vec3_mul(side, 0.5f));

      BlockRaycast hit_l = block_raycast(bp, left_p, dir, dist);
      BlockRaycast hit_r = block_raycast(bp, right_p, dir, dist);

      if (!hit_l.hit || !hit_r.hit) {
        // We're near an edge: Obstructed (Hear diffraction)
        src->occlusion_state = OCCLUSION_OBSTRUCTED;
        src->target_occlusion = 0.6f;
      } else {
        // Fully behind geometry: Muffled (Sound passes through/around more
        // blocks)
        src->occlusion_state = OCCLUSION_MUFFLED;
        src->target_occlusion = 0.2f;
      }
    } else {
      // No hit or hit is behind source
      src->occlusion_state = OCCLUSION_NONE;
      src->target_occlusion = 1.0f;
    }

    // Initial value for factor if it hasn't been set
    if (src->occlusion_factor < 0.001f &&
        src->occlusion_state == OCCLUSION_NONE) {
      src->occlusion_factor = 1.0f;
    }
  }
}

void audio_occlusion_raycast_init(void) {
  // No specific local init needed yet
}

void audio_occlusion_raycast_shutdown(void) {
  // Cleanup if needed
}
