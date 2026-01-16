#include "ecs/systems/audio_systems.h"
#include "audio/audio_allocator.h"
#include "ecs/components/audio_components.h"
#include "engine/include/core/logger.h"
#include "engine/include/math/math_all.h"
#include <math.h>

// Mock Transform (since we rely on component IDs from headers we might not have
// linked fully yet in this partial view) In production, include
// "ecs/components/transform_component.h"
typedef struct {
  Vec3 position;
  Vec3 rotation;
  Vec3 scale;
} TransformComponent; // Placeholder

// Helper: Distance squared
static f32 dist_sq(Vec3 a, Vec3 b) {
  f32 dx = a.x - b.x;
  f32 dy = a.y - b.y;
  f32 dz = a.z - b.z;
  return dx * dx + dy * dy + dz * dz;
}

// Global listener state (cached for system access)
static Vec3 g_listener_pos = {0, 0, 0};
static bool g_listener_active = false;

void spatial_audio_system(SystemContext *ctx) {
  if (!ctx || !ctx->world)
    return;

  // 1. Find Listener
  // Query: AudioListener + Transform
  // For simplicity here, we assume listener is at origin or static if we can't
  // query transform easily without IDs In real implementation: Query for
  // Listener entity and get its Transform
  g_listener_active = true;
  g_listener_pos = (Vec3){0, 0, 0}; // Default to origin

  // 2. Update Sources
  ComponentType types[] = {g_audio_source_component_id};
  QueryDesc desc = {.all_components = types, .all_count = 1};

  Query *query = ecs_query_create(ctx->world, &desc);
  if (!query)
    return;

  Entity entity;
  void *comps[1];

  while (ecs_query_next(query, &entity, comps)) {
    AudioSourceComponent *source = (AudioSourceComponent *)comps[0];

    if (!source->is_3d || !source->is_playing)
      continue;

    // Mock source position (would get from Transform)
    Vec3 source_pos = {10, 0, 0};

    f32 d2 = dist_sq(source_pos, g_listener_pos);
    f32 dist = sqrtf(d2);

    // Calculate attenuation (linear rolloff for now)
    f32 range = source->max_distance - source->min_distance;
    if (range <= 0.001f)
      range = 0.001f;

    f32 t = (dist - source->min_distance) / range;
    if (t < 0)
      t = 0;
    if (t > 1)
      t = 1;

    f32 attenuation = 1.0f - t;

    // Attenuate volume
    // source->effective_volume = source->volume * attenuation; (Internal logic)
  }

  ecs_query_destroy(ctx->world, query);
}

void audio_submission_system(SystemContext *ctx) {
  if (!ctx || !ctx->world)
    return;

  // Reset command arena for this frame
  audio_allocator_reset_commands();

  // Iterate sources and submit commands
  // ... commands would be allocated via audio_alloc_command()
}

void register_audio_systems(World *world) {
  if (!world) {
    LOG_ERROR("[Audio] Cannot register systems: null world");
    return;
  }

  // Spatial System (Priority 80)
  QueryDesc spatial_query = {0};
  System *spatial_sys = ecs_system_create(world, "SpatialAudio",
                                          spatial_audio_system, &spatial_query);
  ecs_system_set_priority(world, spatial_sys, 80);

  // Submission System (Priority 90)
  QueryDesc sub_query = {0};
  System *sub_sys = ecs_system_create(world, "AudioSubmission",
                                      audio_submission_system, &sub_query);
  ecs_system_set_priority(world, sub_sys, 90);

  LOG_INFO("[Audio] Registered systems:");
  LOG_INFO("  SpatialAudio (priority 80)");
  LOG_INFO("  AudioSubmission (priority 90)");
}
