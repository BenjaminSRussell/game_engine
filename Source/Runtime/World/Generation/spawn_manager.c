#include "spawn_manager.h"
#include "../../Core/Game/game_context.h"
#include <common.h>
#include <math.h>
#include <stdlib.h>

typedef struct {
  bool active;
  bool completed;
  i32 chunks_generated;
  i32 total_chunks;
  f32 progress;
  Vec3 spawn_point;
  ThreadPool *worker_threads;
} AsyncSpawnGenerator;

static AsyncSpawnGenerator g_async_spawn = {0};

typedef struct {
  Vec3 position;
  bool active;
  f32 animation_time;
  f32 beam_height;
  f32 beam_intensity;
  Vec3 beacon_color;
} SpawnPointMarker;

static SpawnPointMarker g_spawn_marker = {0};

// Forward declarations of helpers
static i32 find_surface_level(i32 x, i32 z);
static bool is_spawn_location_valid(Vec3 pos, i32 min_flat_area, f32 max_slope);
static bool is_area_flat(i32 center_x, i32 center_z, i32 radius, f32 max_slope);
static f32 evaluate_spawn_quality(Vec3 pos);

// -----------------------------------------------------------------------------
// Async Spawn Implementation
// -----------------------------------------------------------------------------

static void async_spawn_generate_chunk(void *data) {
  ChunkPos *pos = (ChunkPos *)data;
  Chunk *chunk = chunk_manager_get_or_create(&g_game.chunk_manager, *pos);
  if (chunk && chunk->state != CHUNK_STATE_GENERATED) {
    world_generator_generate_chunk(&g_game.world_generator, chunk);
    chunk->state = CHUNK_STATE_GENERATED;
    g_async_spawn.chunks_generated++;
    g_async_spawn.progress = (f32)g_async_spawn.chunks_generated /
                             (f32)g_async_spawn.total_chunks * 100.0f;
  }
  free(pos);
}

void async_spawn_init(Vec3 spawn_point) {
  g_async_spawn.active = true;
  g_async_spawn.completed = false;
  g_async_spawn.chunks_generated = 0;
  g_async_spawn.total_chunks = 25; // 5x5 area around spawn
  g_async_spawn.progress = 0.0f;
  g_async_spawn.spawn_point = spawn_point;
  g_async_spawn.worker_threads = &g_game.thread_pool;

  LOG_INFO("Starting async spawn chunk generation around (%.1f, %.1f, %.1f)",
           spawn_point.x, spawn_point.y, spawn_point.z);
}

void async_spawn_update(void) {
  if (!g_async_spawn.active || g_async_spawn.completed)
    return;

  if (g_async_spawn.chunks_generated >= g_async_spawn.total_chunks) {
    g_async_spawn.completed = true;
    g_async_spawn.active = false;
    LOG_INFO("Async spawn chunk generation completed: %d chunks",
             g_async_spawn.chunks_generated);
    return;
  }

  i32 spawn_chunk_x = (i32)floorf(g_async_spawn.spawn_point.x / 16.0f);
  i32 spawn_chunk_z = (i32)floorf(g_async_spawn.spawn_point.z / 16.0f);

  for (i32 cz = -2;
       cz <= 2 && g_async_spawn.chunks_generated < g_async_spawn.total_chunks;
       cz++) {
    for (i32 cx = -2;
         cx <= 2 && g_async_spawn.chunks_generated < g_async_spawn.total_chunks;
         cx++) {
      ChunkPos pos = {spawn_chunk_x + cx, 0, spawn_chunk_z + cz};
      Chunk *chunk = chunk_manager_get_or_create(&g_game.chunk_manager, pos);

      if (chunk && chunk->state != CHUNK_STATE_GENERATED) {
        ChunkPos *job_data = malloc(sizeof(ChunkPos));
        *job_data = pos;
        thread_pool_submit(g_async_spawn.worker_threads,
                           async_spawn_generate_chunk, job_data, 1);
        // Break to avoid submitting too many at once - simple throttling
        break;
      }
    }
    if (g_async_spawn.chunks_generated >= g_async_spawn.total_chunks)
      break;
  }
}

bool async_spawn_is_complete(void) { return g_async_spawn.completed; }

// -----------------------------------------------------------------------------
// Spawn Point Calculation
// -----------------------------------------------------------------------------

Vec3 find_suitable_spawn_point(void) {
  const i32 search_radius = 100;
  const i32 min_flat_area = 5;
  const f32 acceptable_slope = 0.3f;

  Vec3 best_spawn = vec3(0.0f, 70.0f, 0.0f);
  f32 best_score = -1.0f;

  LOG_INFO("Searching for suitable spawn point within %d blocks...",
           search_radius);

  for (i32 attempts = 0; attempts < 50; attempts++) {
    i32 candidate_x = (rand() % (search_radius * 2)) - search_radius;
    i32 candidate_z = (rand() % (search_radius * 2)) - search_radius;

    i32 surface_y = find_surface_level(candidate_x, candidate_z);
    if (surface_y < 10)
      continue;

    Vec3 candidate =
        vec3((f32)candidate_x, (f32)surface_y + 2, (f32)candidate_z);

    if (is_spawn_location_valid(candidate, min_flat_area, acceptable_slope)) {
      f32 score = evaluate_spawn_quality(candidate);
      if (score > best_score) {
        best_score = score;
        best_spawn = candidate;
      }
    }
  }

  LOG_INFO("Selected spawn point: (%.1f, %.1f, %.1f) with score %.2f",
           best_spawn.x, best_spawn.y, best_spawn.z, best_score);
  return best_spawn;
}

static i32 find_surface_level(i32 x, i32 z) {
  const i32 max_height = 120;
  const i32 min_height = 50;
  for (i32 y = max_height; y >= min_height; y--) {
    BlockID block = chunk_manager_get_block(&g_game.chunk_manager, x, y, z);
    if (block != BLOCK_AIR && block != BLOCK_WATER && block != BLOCK_LAVA) {
      return y;
    }
  }
  return 70;
}

static bool is_spawn_location_valid(Vec3 pos, i32 min_flat_area,
                                    f32 max_slope) {
  i32 x = (i32)pos.x;
  i32 y = (i32)pos.y;
  i32 z = (i32)pos.z;

  BlockID head_block =
      chunk_manager_get_block(&g_game.chunk_manager, x, y + 1, z);
  BlockID feet_block = chunk_manager_get_block(&g_game.chunk_manager, x, y, z);
  BlockID ground_block =
      chunk_manager_get_block(&g_game.chunk_manager, x, y - 1, z);

  if (head_block != BLOCK_AIR)
    return false;
  if (feet_block != BLOCK_AIR && feet_block != BLOCK_WATER &&
      feet_block != BLOCK_TALL_GRASS) {
    return false;
  }
  if (ground_block == BLOCK_AIR || ground_block == BLOCK_LAVA ||
      ground_block == BLOCK_WATER) {
    return false;
  }

  return is_area_flat(x, z, min_flat_area, max_slope);
}

static bool is_area_flat(i32 center_x, i32 center_z, i32 radius,
                         f32 max_slope) {
  i32 base_height = find_surface_level(center_x, center_z);
  for (i32 dx = -radius; dx <= radius; dx++) {
    for (i32 dz = -radius; dz <= radius; dz++) {
      i32 height = find_surface_level(center_x + dx, center_z + dz);
      f32 slope = fabsf((f32)(height - base_height));
      if (slope > max_slope) {
        return false;
      }
    }
  }
  return true;
}

static f32 evaluate_spawn_quality(Vec3 pos) {
  f32 score = 0.0f;
  i32 x = (i32)pos.x;
  i32 z = (i32)pos.z;

  for (i32 dx = -20; dx <= 20; dx++) {
    for (i32 dz = -20; dz <= 20; dz++) {
      BlockID block = chunk_manager_get_block(&g_game.chunk_manager, x + dx,
                                              (i32)pos.y, z + dz);
      if (block == BLOCK_WATER) {
        score += 0.1f;
      }
    }
  }

  // Bonus points for trees, etc. (Abbreviated as in monolithic)
  if (pos.y >= 60 && pos.y <= 80) {
    score += 0.5f;
  }
  return score;
}

// -----------------------------------------------------------------------------
// Spawn Marker Implementation
// -----------------------------------------------------------------------------

void spawn_marker_init(Vec3 position) {
  g_spawn_marker.position = position;
  g_spawn_marker.active = true;
  g_spawn_marker.animation_time = 0.0f;
  g_spawn_marker.beam_height = 50.0f;
  g_spawn_marker.beam_intensity = 1.0f;
  g_spawn_marker.beacon_color = vec3(0.0f, 1.0f, 0.0f);
  LOG_INFO("Spawn point marker initialized at (%.1f, %.1f, %.1f)", position.x,
           position.y, position.z);
}

void spawn_marker_update(f32 delta_time) {
  if (!g_spawn_marker.active)
    return;
  g_spawn_marker.animation_time += delta_time;
  g_spawn_marker.beam_intensity =
      0.7f + 0.3f * sinf(g_spawn_marker.animation_time * 2.0f);
  f32 color_shift = sinf(g_spawn_marker.animation_time * 0.5f) * 0.1f;
  g_spawn_marker.beacon_color =
      vec3(0.0f + color_shift, 1.0f, 0.0f - color_shift);
}

void spawn_marker_render(void) {
  // Placeholder logic as in monolithic
  if (!g_spawn_marker.active)
    return;
}
