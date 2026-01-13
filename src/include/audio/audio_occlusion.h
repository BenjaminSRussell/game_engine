// include/audio/audio_occlusion.h
//
// Purpose: Audio occlusion system for realistic 3D sound propagation based on
// world geometry. Sounds are muffled/filtered when blocked by walls and
// terrain.
//
// Key Features:
// - Raycasting from listener to sound source
// - Material-based occlusion factors (stone blocks more than glass)
// - Smooth occlusion transitions
// - Low-pass filtering for occluded sounds
// - Indirect path finding for partial occlusion
//
// Public APIs:
// - audio_occlusion_init: Initialize occlusion system
// - audio_occlusion_update: Update occlusion for all active sounds
// - audio_occlusion_raycast: Perform occlusion test
//
// Invariants:
// - Requires access to world chunk data
// - Occlusion factor 0.0 = fully blocked, 1.0 = clear line of sight
// - Update per-frame for dynamic sound sources
//
#ifndef AUDIO_OCCLUSION_H
#define AUDIO_OCCLUSION_H

#include "engine/include/common.h"
#include "include/audio/audio_system.h"
#include <chunk/chunk.h>
#include <math/vec3.h>

#define MAX_OCCLUSION_RAYS 5
#define OCCLUSION_BLEND_SPEED 0.1f

typedef struct {
  u16 block_type;
  f32 occlusion_factor; // 0.0 = no occlusion, 1.0 = fully blocks
  f32 filter_cutoff;    // Low-pass filter frequency when occluded
} MaterialOcclusion;

typedef struct {
  Vec3 ray_origin;
  Vec3 ray_direction;
  f32 distance;
  f32 occlusion_amount;
  bool hit_solid;
} OcclusionRay;

typedef struct {
  ChunkManager *chunk_manager;
  AudioSystem *audio_system;

  // Material occlusion table
  MaterialOcclusion material_table[256];

  // Occlusion state per sound source
  f32 *source_occlusions;
  u32 max_sources;

  bool initialized;
} AudioOcclusionSystem;

// Lifecycle
void audio_occlusion_init(AudioOcclusionSystem *system, AudioSystem *audio,
                          ChunkManager *chunks, u32 max_sources);
void audio_occlusion_shutdown(AudioOcclusionSystem *system);

// Material setup
void audio_occlusion_set_material(AudioOcclusionSystem *system, u16 block_type,
                                  f32 occlusion_factor, f32 filter_cutoff);
void audio_occlusion_load_defaults(AudioOcclusionSystem *system);

// Occlusion calculation
f32 audio_occlusion_raycast(AudioOcclusionSystem *system, Vec3 listener_pos,
                            Vec3 sound_pos, OcclusionRay *rays, u32 ray_count);
void audio_occlusion_update(AudioOcclusionSystem *system, Vec3 listener_pos,
                            f32 delta_time);

// Apply occlusion to sound
void audio_occlusion_apply_to_source(AudioOcclusionSystem *system,
                                     u32 source_id, f32 occlusion);

#endif // AUDIO_OCCLUSION_H
