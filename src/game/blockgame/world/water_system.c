// Water flow simulation and audio system implementation.
// Roadmap: docs/WORLD_ROADMAP.md
#include <audio/underwater_filter.h>
#include <block/block.h>
#include <core/logger.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <world/water_system.h>

// Water flow constants
#define MAX_FLOW_DISTANCE 7
#define FLOW_SPEED_BASE 2.0f
#define WATERFALL_SPEED 8.0f
#define SPLASH_VELOCITY_THRESHOLD 5.0f
#define BREATH_CAPACITY 20.0f // seconds

static void water_update_flow_audio(WaterSystem *water);
static void water_update_underwater_audio(WaterSystem *water,
                                          AudioSystem *audio);
static void water_scan_nearby_flows(WaterSystem *water, ChunkManager *chunks,
                                    Vec3 player_pos);

void water_system_init(WaterSystem *water, AudioSystem *audio) {
  if (!water)
    return;

  memset(water, 0, sizeof(WaterSystem));
  water->audio_config = water_get_default_audio_config();
  water->breath_remaining = 1.0f;
  water->initialized = true;

  LOG_INFO("Water system initialized");
}

void water_system_free(WaterSystem *water) {
  if (!water)
    return;

  // Stop all audio sources
  // Note: Audio cleanup would happen via audio_system_free

  memset(water, 0, sizeof(WaterSystem));
  LOG_INFO("Water system freed");
}

void water_system_update(WaterSystem *water, ChunkManager *chunks,
                         Vec3 player_pos, f32 delta_time) {
  if (!water || !water->initialized || !chunks)
    return;

  // Check if player is underwater
  bool was_underwater = water->is_player_underwater;
  Vec3 head_pos = vec3_add(player_pos, vec3(0.0f, 1.6f, 0.0f)); // Eye level
  water->is_player_underwater = water_is_position_underwater(chunks, head_pos);

  if (water->is_player_underwater) {
    water->underwater_depth = water_get_depth_at_position(chunks, head_pos);

    // Update breath meter
    water->breath_remaining -= delta_time / BREATH_CAPACITY;
    if (water->breath_remaining < 0.0f) {
      water->breath_remaining = 0.0f;
      // Player is drowning - this would trigger damage in player system
    }
  } else {
    // Restore breath when above water
    water->breath_remaining += delta_time / (BREATH_CAPACITY * 0.5f);
    if (water->breath_remaining > 1.0f) {
      water->breath_remaining = 1.0f;
    }
    water->underwater_depth = 0.0f;
  }

  // Scan for nearby flowing water
  water_scan_nearby_flows(water, chunks, player_pos);

  // Update audio
  water_update_flow_audio(water);

  // Update splash cooldown
  if (water->splash_cooldown > 0.0f) {
    water->splash_cooldown -= delta_time;
  }

  // Log state changes
  if (water->is_player_underwater != was_underwater) {
    if (water->is_player_underwater) {
      LOG_DEBUG("Player entered water (depth: %.1f blocks)",
                water->underwater_depth);
    } else {
      LOG_DEBUG("Player exited water");
    }
  }

  // Update global underwater audio filtering each frame
  water_update_audio_filtering(water, NULL);
}

static void water_scan_nearby_flows(WaterSystem *water, ChunkManager *chunks,
                                    Vec3 player_pos) {
  if (!water || !chunks)
    return;

  water->active_flow_count = 0;

  // Scan 16x16x16 area around player
  i32 scan_radius = 8;
  i32 px = (i32)player_pos.x;
  i32 py = (i32)player_pos.y;
  i32 pz = (i32)player_pos.z;

  for (i32 dx = -scan_radius; dx <= scan_radius; dx++) {
    for (i32 dy = -scan_radius; dy <= scan_radius; dy++) {
      for (i32 dz = -scan_radius; dz <= scan_radius; dz++) {
        if (water->active_flow_count >= 256)
          return;

        Vec3 check_pos = vec3((f32)(px + dx), (f32)(py + dy), (f32)(pz + dz));
        WaterFlowState state = water_get_flow_state(chunks, check_pos);

        if (state != WATER_FLOW_NONE) {
          WaterFlowData *flow = &water->active_flows[water->active_flow_count];
          flow->position = check_pos;
          flow->state = state;
          flow->flow_direction =
              water_get_flow_direction(water, chunks, check_pos);
          flow->flow_speed = (state == WATER_FLOW_FALLING) ? 1.0f : 0.5f;
          water->active_flow_count++;
        }
      }
    }
  }
}

static void water_update_flow_audio(WaterSystem *water) {
  if (!water)
    return;

  // Calculate total flow intensity from nearby water
  f32 flow_intensity = 0.0f;
  f32 waterfall_intensity = 0.0f;

  for (u32 i = 0; i < water->active_flow_count; i++) {
    WaterFlowData *flow = &water->active_flows[i];

    if (flow->state == WATER_FLOW_FALLING) {
      waterfall_intensity += 0.2f;
    } else if (flow->state == WATER_FLOW_FLOWING) {
      flow_intensity += 0.1f;
    }
  }

  // Clamp intensities
  flow_intensity = (flow_intensity > 1.0f) ? 1.0f : flow_intensity;
  waterfall_intensity =
      (waterfall_intensity > 1.0f) ? 1.0f : waterfall_intensity;

  // Update audio volumes
  // Note: This would actually call audio_set_source_volume with the audio
  // system For now, we just store the values
  (void)flow_intensity;
  (void)waterfall_intensity;
}

bool water_check_flow(WaterSystem *water, ChunkManager *chunks, Vec3 position) {
  (void)water;
  if (!chunks)
    return false;

  WaterFlowState state = water_get_flow_state(chunks, position);
  return state == WATER_FLOW_FLOWING || state == WATER_FLOW_FALLING;
}

Vec3 water_get_flow_direction(WaterSystem *water, ChunkManager *chunks,
                              Vec3 position) {
  (void)water;
  if (!chunks)
    return vec3(0.0f, 0.0f, 0.0f);

  // Get water block
  ChunkPos cp =
      world_to_chunk_pos((i32)position.x, (i32)position.y, (i32)position.z);
  Chunk *chunk = chunk_manager_get(chunks, cp);
  if (!chunk)
    return vec3(0.0f, 0.0f, 0.0f);

  i32 lx = (i32)position.x - cp.x * CHUNK_SIZE;
  i32 ly = (i32)position.y - cp.y * CHUNK_SIZE;
  i32 lz = (i32)position.z - cp.z * CHUNK_SIZE;

  BlockID block = chunk_get_block(chunk, lx, ly, lz);
  if (block != BLOCK_WATER)
    return vec3(0.0f, 0.0f, 0.0f);

  // Calculate flow direction based on surrounding water levels
  Vec3 flow = vec3(0.0f, 0.0f, 0.0f);

  // Check cardinal directions
  BlockID east = chunk_get_block(chunk, lx + 1, ly, lz);
  BlockID west = chunk_get_block(chunk, lx - 1, ly, lz);
  BlockID north = chunk_get_block(chunk, lx, ly, lz + 1);
  BlockID south = chunk_get_block(chunk, lx, ly, lz - 1);
  BlockID below = chunk_get_block(chunk, lx, ly - 1, lz);

  // Flow downward if possible
  if (below == BLOCK_AIR) {
    flow.y = -1.0f;
  } else {
    // Flow horizontally toward air
    if (east == BLOCK_AIR)
      flow.x += 1.0f;
    if (west == BLOCK_AIR)
      flow.x -= 1.0f;
    if (north == BLOCK_AIR)
      flow.z += 1.0f;
    if (south == BLOCK_AIR)
      flow.z -= 1.0f;
  }

  // Normalize
  f32 length = sqrtf(flow.x * flow.x + flow.y * flow.y + flow.z * flow.z);
  if (length > 0.0f) {
    flow.x /= length;
    flow.y /= length;
    flow.z /= length;
  }

  return flow;
}

f32 water_get_flow_speed(WaterSystem *water, Vec3 position) {
  if (!water)
    return 0.0f;

  // Find matching flow in active flows
  for (u32 i = 0; i < water->active_flow_count; i++) {
    WaterFlowData *flow = &water->active_flows[i];
    if (fabsf(flow->position.x - position.x) < 0.5f &&
        fabsf(flow->position.y - position.y) < 0.5f &&
        fabsf(flow->position.z - position.z) < 0.5f) {
      return flow->flow_speed;
    }
  }

  return 0.0f;
}

WaterFlowState water_get_flow_state(ChunkManager *chunks, Vec3 position) {
  if (!chunks)
    return WATER_FLOW_NONE;

  ChunkPos cp =
      world_to_chunk_pos((i32)position.x, (i32)position.y, (i32)position.z);
  Chunk *chunk = chunk_manager_get(chunks, cp);
  if (!chunk)
    return WATER_FLOW_NONE;

  i32 lx = (i32)position.x - cp.x * CHUNK_SIZE;
  i32 ly = (i32)position.y - cp.y * CHUNK_SIZE;
  i32 lz = (i32)position.z - cp.z * CHUNK_SIZE;

  BlockID block = chunk_get_block(chunk, lx, ly, lz);
  if (block != BLOCK_WATER)
    return WATER_FLOW_NONE;

  // Check if water is falling
  BlockID below = chunk_get_block(chunk, lx, ly - 1, lz);
  if (below == BLOCK_AIR) {
    return WATER_FLOW_FALLING;
  }

  // Check if water is flowing (has air nearby)
  BlockID east = chunk_get_block(chunk, lx + 1, ly, lz);
  BlockID west = chunk_get_block(chunk, lx - 1, ly, lz);
  BlockID north = chunk_get_block(chunk, lx, ly, lz + 1);
  BlockID south = chunk_get_block(chunk, lx, ly, lz - 1);

  if (east == BLOCK_AIR || west == BLOCK_AIR || north == BLOCK_AIR ||
      south == BLOCK_AIR) {
    return WATER_FLOW_FLOWING;
  }

  return WATER_FLOW_SOURCE;
}

void water_play_splash(WaterSystem *water, Vec3 position, f32 velocity) {
  if (!water || !water->initialized)
    return;

  // Check cooldown to avoid spam
  if (water->splash_cooldown > 0.0f)
    return;

  // Only play splash if velocity is high enough
  if (velocity < SPLASH_VELOCITY_THRESHOLD)
    return;

  water->last_splash_position = position;
  water->splash_cooldown = 0.5f; // 500ms cooldown

  // Calculate splash intensity based on velocity
  f32 intensity = velocity / (SPLASH_VELOCITY_THRESHOLD * 3.0f);
  if (intensity > 1.0f)
    intensity = 1.0f;

  LOG_DEBUG("Water splash at (%.1f, %.1f, %.1f) with intensity %.2f",
            position.x, position.y, position.z, intensity);

  // Note: Would trigger audio_play_sound_at_position and particle spawn
}

void water_play_bubble_sound(WaterSystem *water, Vec3 position) {
  if (!water || !water->initialized)
    return;

  LOG_TRACE("Bubble sound at (%.1f, %.1f, %.1f)", position.x, position.y,
            position.z);

  // Note: Would trigger audio_play_sound_at_position
}

void water_set_underwater(WaterSystem *water, bool underwater, f32 depth) {
  if (!water)
    return;

  water->is_player_underwater = underwater;
  water->underwater_depth = depth;

  // Apply/clear global underwater audio filter immediately
  if (underwater) {
    underwater_filter_set_enabled(true);
    underwater_filter_update_depth(depth);
  } else {
    underwater_filter_set_enabled(false);
  }
}

WaterAudioConfig water_get_default_audio_config(void) {
  WaterAudioConfig config = {.flow_volume = 0.5f,
                             .splash_volume = 0.7f,
                             .underwater_volume = 0.3f,
                             .underwater_filter_freq = 1000.0f, // Hz
                             .enable_3d_audio = true,
                             .max_audio_distance = 32.0f};
  return config;
}

void water_set_audio_config(WaterSystem *water,
                            const WaterAudioConfig *config) {
  if (!water || !config)
    return;
  water->audio_config = *config;
}

void water_update_audio_filtering(WaterSystem *water, AudioSystem *audio) {
  if (!water)
    return;
  (void)audio; // Underwater filter is applied via global DSP module

  // Apply or clear underwater audio filtering based on state
  if (water->is_player_underwater) {
    underwater_filter_set_enabled(true);
    underwater_filter_update_depth(water->underwater_depth);
    LOG_TRACE("Underwater audio filtering active (%.0f Hz)",
              water->audio_config.underwater_filter_freq);
  } else {
    underwater_filter_set_enabled(false);
  }
}

f32 water_get_depth_at_position(ChunkManager *chunks, Vec3 position) {
  if (!chunks)
    return 0.0f;

  ChunkPos cp =
      world_to_chunk_pos((i32)position.x, (i32)position.y, (i32)position.z);
  Chunk *chunk = chunk_manager_get(chunks, cp);
  if (!chunk)
    return 0.0f;

  i32 lx = (i32)position.x - cp.x * CHUNK_SIZE;
  i32 ly = (i32)position.y - cp.y * CHUNK_SIZE;
  i32 lz = (i32)position.z - cp.z * CHUNK_SIZE;

  BlockID block = chunk_get_block(chunk, lx, ly, lz);
  if (block != BLOCK_WATER)
    return 0.0f;

  // Count water blocks above
  f32 depth = 1.0f;
  for (i32 dy = 1; dy < CHUNK_SIZE - ly; dy++) {
    BlockID above = chunk_get_block(chunk, lx, ly + dy, lz);
    if (above != BLOCK_WATER)
      break;
    depth += 1.0f;
  }

  return depth;
}

bool water_is_position_underwater(ChunkManager *chunks, Vec3 position) {
  if (!chunks)
    return false;

  ChunkPos cp =
      world_to_chunk_pos((i32)position.x, (i32)position.y, (i32)position.z);
  Chunk *chunk = chunk_manager_get(chunks, cp);
  if (!chunk)
    return false;

  i32 lx = (i32)position.x - cp.x * CHUNK_SIZE;
  i32 ly = (i32)position.y - cp.y * CHUNK_SIZE;
  i32 lz = (i32)position.z - cp.z * CHUNK_SIZE;

  BlockID block = chunk_get_block(chunk, lx, ly, lz);
  return block == BLOCK_WATER;
}

f32 water_get_temperature(ChunkManager *chunks, Vec3 position) {
  (void)chunks;
  (void)position;

  // Note: This would integrate with the biome system
  // For now, return a default temperature
  return 15.0f; // Celsius
}

bool water_is_frozen(ChunkManager *chunks, Vec3 position) {
  f32 temp = water_get_temperature(chunks, position);
  return temp < 0.0f;
}