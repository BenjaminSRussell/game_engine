#include "../include/audio/audio_occlusion_raycast.h"
#include "../include/audio/audio_system.h"
#include "../include/core/common.h"
#include "../include/math/vec3.h"
#include "../include/physics/block_physics.h"
#include <math.h>
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

    if (hit.hit) {
      // Check if hit distance is significantly less than source distance
      // (allowing for small epsilon to avoid self-occlusion if source is inside
      // a block? but source is usually an entity)
      if (hit.distance < dist - 0.5f) { // 0.5f buffer
        // Occluded!
        src->target_occlusion = 0.3f;
      } else {
        src->target_occlusion = 1.0f;
      }
    } else {
      src->occlusion = 0.0f;
    }
  }
}

void audio_occlusion_raycast_init(void) {
  // No specific local init needed yet
}

void audio_occlusion_raycast_shutdown(void) {
  // Cleanup if needed
}
