#ifndef ADVANCED_TERRAIN_H
#include "../../../procedural/terrain/advanced_terrain.h"
#endif

#include "mega_terrain.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for job system (simplified for now)
typedef void (*JobFunc)(void *data);
typedef struct Job Job;
Job *job_create(JobFunc func, void *data);
void job_system_submit(void *sys,
                       Job *job); // Assuming a global or passed job system

// Internal helpers
static void terrain_chunk_load_async(TerrainSystem *system,
                                     TerrainChunk *chunk);
static void terrain_chunk_unload(TerrainSystem *system, TerrainChunk *chunk);
static float calculate_distance_sq(float p1[3], float p2[3]);

bool terrain_system_init(TerrainSystem *system, const TerrainConfig *config) {
  if (!system || !config)
    return false;

  system->config = *config;
  system->chunk_count = 0;
  system->chunks =
      (TerrainChunk *)calloc(TERRAIN_MAX_CHUNKS, sizeof(TerrainChunk));

  if (!system->chunks)
    return false;

  memset(&system->streaming, 0, sizeof(TerrainStreamingState));
  system->streaming.is_streaming = true;
  system->streaming.load_radius = 1000.0f; // Default
  system->streaming.unload_radius = 1200.0f;
  system->streaming.max_chunks_per_frame = 4;

  // Initialize biome database (TODO-26605)
  // PLAINS
  system->biomes[BIOME_PLAINS].temperature_min = 0.4f;
  system->biomes[BIOME_PLAINS].temperature_max = 0.7f;
  system->biomes[BIOME_PLAINS].moisture_min = 0.3f;
  system->biomes[BIOME_PLAINS].moisture_max = 0.6f;
  snprintf(system->biomes[BIOME_PLAINS].name, 32, "Plains");

  // DESERT
  system->biomes[BIOME_DESERT].temperature_min = 0.7f;
  system->biomes[BIOME_DESERT].temperature_max = 1.0f;
  system->biomes[BIOME_DESERT].moisture_min = 0.0f;
  system->biomes[BIOME_DESERT].moisture_max = 0.2f;
  snprintf(system->biomes[BIOME_DESERT].name, 32, "Desert");

  // MOUNTAIN
  system->biomes[BIOME_MOUNTAIN].altitude_min = 0.7f;
  system->biomes[BIOME_MOUNTAIN].altitude_max = 1.0f;
  system->biomes[BIOME_MOUNTAIN].temperature_min = 0.0f;
  system->biomes[BIOME_MOUNTAIN].temperature_max = 0.3f;
  snprintf(system->biomes[BIOME_MOUNTAIN].name, 32, "Mountain");

  // SNOW
  system->biomes[BIOME_SNOW].temperature_min = 0.0f;
  system->biomes[BIOME_SNOW].temperature_max = 0.2f;
  system->biomes[BIOME_SNOW].moisture_min = 0.4f;
  system->biomes[BIOME_SNOW].moisture_max = 0.8f;
  snprintf(system->biomes[BIOME_SNOW].name, 32, "Snow");

  // Rest can be initialized to defaults
  for (int i = 0; i < BIOME_COUNT; i++) {
    if (strlen(system->biomes[i].name) == 0) {
      system->biomes[i].type = (BiomeType)i;
      snprintf(system->biomes[i].name, 32, "Biome %d", i);
    }
  }

  return true;
}

void terrain_system_shutdown(TerrainSystem *system) {
  if (!system)
    return;

  if (system->chunks) {
    for (uint32_t i = 0; i < TERRAIN_MAX_CHUNKS; i++) {
      if (system->chunks[i].is_loaded) {
        terrain_chunk_unload(system, &system->chunks[i]);
      }
    }
    free(system->chunks);
  }
}

void terrain_system_update(TerrainSystem *system, float dt) {
  if (!system || !system->streaming.is_streaming)
    return;

  // 1. Calculate predicted camera position for prefetching
  float prefetch_time = 2.0f; // Predict 2 seconds ahead
  float predicted_pos[3] = {
      system->streaming.camera_position[0] +
          system->streaming.camera_velocity[0] * prefetch_time,
      system->streaming.camera_position[1] +
          system->streaming.camera_velocity[1] * prefetch_time,
      system->streaming.camera_position[2] +
          system->streaming.camera_velocity[2] * prefetch_time};

  // Calculate current camera chunk
  int32_t cam_chunk_x = (int32_t)floorf(predicted_pos[0] / TERRAIN_CHUNK_SIZE);
  int32_t cam_chunk_z = (int32_t)floorf(predicted_pos[2] / TERRAIN_CHUNK_SIZE);

  int32_t radius =
      (int32_t)ceilf(system->streaming.load_radius / TERRAIN_CHUNK_SIZE);
  float load_radius_sq =
      system->streaming.load_radius * system->streaming.load_radius;
  float unload_radius_sq =
      system->streaming.unload_radius * system->streaming.unload_radius;

  // 2. Identify chunks that need to be loaded
  uint32_t load_candidates_count = 0;
  TerrainChunk *load_candidates[256];

  for (int32_t z = cam_chunk_z - radius; z <= cam_chunk_z + radius; z++) {
    for (int32_t x = cam_chunk_x - radius; x <= cam_chunk_x + radius; x++) {
      float chunk_center[3] = {(x + 0.5f) * TERRAIN_CHUNK_SIZE, 0.0f,
                               (z + 0.5f) * TERRAIN_CHUNK_SIZE};
      float dist_sq = calculate_distance_sq(predicted_pos, chunk_center);

      if (dist_sq <= load_radius_sq) {
        bool found = false;
        for (uint32_t i = 0; i < system->chunk_count; i++) {
          if (system->chunks[i].chunk_x == x &&
              system->chunks[i].chunk_z == z) {
            found = true;
            break;
          }
        }

        if (!found && load_candidates_count < 256) {
          if (system->chunk_count < TERRAIN_MAX_CHUNKS) {
            TerrainChunk *new_chunk = &system->chunks[system->chunk_count++];
            new_chunk->chunk_x = x;
            new_chunk->chunk_z = z;
            new_chunk->is_loaded = false;
            load_candidates[load_candidates_count++] = new_chunk;
          }
        }
      }
    }
  }

  // 3. Sort candidates by distance to predicted position (nearest first)
  for (uint32_t i = 0; i < load_candidates_count; i++) {
    for (uint32_t j = i + 1; j < load_candidates_count; j++) {
      float center_i[3] = {
          (load_candidates[i]->chunk_x + 0.5f) * TERRAIN_CHUNK_SIZE, 0.0f,
          (load_candidates[i]->chunk_z + 0.5f) * TERRAIN_CHUNK_SIZE};
      float center_j[3] = {
          (load_candidates[j]->chunk_x + 0.5f) * TERRAIN_CHUNK_SIZE, 0.0f,
          (load_candidates[j]->chunk_z + 0.5f) * TERRAIN_CHUNK_SIZE};

      if (calculate_distance_sq(predicted_pos, center_j) <
          calculate_distance_sq(predicted_pos, center_i)) {
        TerrainChunk *temp = load_candidates[i];
        load_candidates[i] = load_candidates[j];
        load_candidates[j] = temp;
      }
    }
  }

  // 4. Dispatch load jobs for high priority chunks
  uint32_t dispatched = 0;
  for (uint32_t i = 0; i < load_candidates_count &&
                       dispatched < system->streaming.max_chunks_per_frame;
       i++) {
    system->streaming.chunks_loading++;
    terrain_chunk_load_async(system, load_candidates[i]);
    dispatched++;
  }

  // 5. Identify chunks that need to be unloaded (using ACTUAL camera position)
  uint32_t loaded_count = 0;
  for (uint32_t i = 0; i < system->chunk_count; i++) {
    TerrainChunk *chunk = &system->chunks[i];
    if (!chunk->is_loaded)
      continue;

    loaded_count++;
    float chunk_center[3] = {(chunk->chunk_x + 0.5f) * TERRAIN_CHUNK_SIZE, 0.0f,
                             (chunk->chunk_z + 0.5f) * TERRAIN_CHUNK_SIZE};
    float dist_sq =
        calculate_distance_sq(system->streaming.camera_position, chunk_center);

    if (dist_sq > unload_radius_sq) {
      terrain_chunk_unload(system, chunk);

      // Compact the array by swapping with the last element
      if (i < system->chunk_count - 1) {
        system->chunks[i] = system->chunks[system->chunk_count - 1];
        memset(&system->chunks[system->chunk_count - 1], 0,
               sizeof(TerrainChunk));
      }
      system->chunk_count--;
      i--; // Re-check this index as it now contains a new chunk
      loaded_count--;
    }
  }

  system->streaming.chunks_loaded = loaded_count;

  // 6. Call progress callback
  if (system->streaming.progress_callback) {
    system->streaming.progress_callback(system->streaming.chunks_loading,
                                        system->streaming.chunks_loaded);
  }
}

static void terrain_chunk_load_job(void *data) {
  TerrainChunk *chunk = (TerrainChunk *)data;

  // Allocate heightmap if not already present
  if (!chunk->heightmap) {
    chunk->heightmap_size = TERRAIN_CHUNK_SIZE; // Simplified
    chunk->heightmap = (float *)malloc(chunk->heightmap_size *
                                       chunk->heightmap_size * sizeof(float));
  }

  // TODO: Call procedural generation (advanced_terrain.h)
  // terrain_generate_heightmap(...)

  chunk->is_loaded = true;
  chunk->needs_update = true;
}

// Chunk normal map generation (TODO-26597)
void terrain_chunk_generate_normals(TerrainChunk *chunk) {
  if (!chunk || !chunk->heightmap)
    return;

  uint32_t size = chunk->heightmap_size;
  for (uint32_t z = 1; z < size - 1; z++) {
    for (uint32_t x = 1; x < size - 1; x++) {
      float h_l = chunk->heightmap[z * size + (x - 1)];
      float h_r = chunk->heightmap[z * size + (x + 1)];
      float h_u = chunk->heightmap[(z - 1) * size + x];
      float h_d = chunk->heightmap[(z + 1) * size + x];

      float dx = (h_l - h_r);
      float dz = (h_u - h_d);
      float dy = 2.0f;

      float length = sqrtf(dx * dx + dy * dy + dz * dz);
      // Normalized: dx/length, dy/length, dz/length
    }
  }
}

// Chunk stitching at LOD boundaries (TODO-26595)
void terrain_chunk_stitch_edges(TerrainChunk *chunk,
                                TerrainChunk *neighbors[4]) {
  if (!chunk || !chunk->heightmap)
    return;

  uint32_t size = chunk->heightmap_size;

  // Stitch North edge (z = 0)
  if (neighbors[0] && neighbors[0]->is_loaded &&
      neighbors[0]->current_lod > chunk->current_lod) {
    for (uint32_t x = 0; x < size; x += 2) {
      float avg = (chunk->heightmap[x] + chunk->heightmap[x + 1]) * 0.5f;
      chunk->heightmap[x] = chunk->heightmap[x + 1] = avg;
    }
  }

  // Stitch South edge (z = size - 1)
  if (neighbors[2] && neighbors[2]->is_loaded &&
      neighbors[2]->current_lod > chunk->current_lod) {
    for (uint32_t x = 0; x < size; x += 2) {
      float avg = (chunk->heightmap[(size - 1) * size + x] +
                   chunk->heightmap[(size - 1) * size + x + 1]) *
                  0.5f;
      chunk->heightmap[(size - 1) * size + x] =
          chunk->heightmap[(size - 1) * size + x + 1] = avg;
    }
  }

  // Similar for East and West...
}

static void terrain_chunk_load_async(TerrainSystem *system,
                                     TerrainChunk *chunk) {
  if (chunk->is_loaded)
    return;

  // For now, call it synchronously until we have a stable job system hook
  terrain_chunk_load_job(chunk);
}

static void terrain_chunk_unload(TerrainSystem *system, TerrainChunk *chunk) {
  if (!chunk->is_loaded)
    return;

  if (chunk->heightmap) {
    free(chunk->heightmap);
    chunk->heightmap = NULL;
  }

  chunk->is_loaded = false;
  chunk->mesh_id = 0;
}

static float calculate_distance_sq(float p1[3], float p2[3]) {
  float dx = p1[0] - p2[0];
  float dy = p1[1] - p2[1];
  float dz = p1[2] - p2[2];
  return dx * dx + dy * dy + dz * dz;
}

// Biome sampling implementation (TODO-26606)
BiomeType terrain_system_get_biome_at(TerrainSystem *system, float x, float z) {
  if (!system)
    return BIOME_PLAINS;

  // Simplified moisture/temperature sampling using Perlin noise
  // In a real system, these would come from the heightmap/biome map of the
  // chunk
  float temperature = 0.5f + 0.5f * sinf(x * 0.001f) * cosf(z * 0.001f);
  float moisture = 0.5f + 0.5f * cosf(x * 0.001f) * sinf(z * 0.001f);
  float altitude = 0.0f; // Could sample height here

  // Simple heuristic lookup
  if (altitude > 0.8f)
    return BIOME_MOUNTAIN;
  if (temperature > 0.7f && moisture < 0.2f)
    return BIOME_DESERT;
  if (temperature < 0.2f)
    return BIOME_SNOW;
  if (moisture > 0.7f)
    return BIOME_SWAMP;

  return BIOME_PLAINS;
}

// Biome-specific hooks (TODO-26608, 26609, 26610)
void terrain_update_biome_effects(TerrainSystem *system, float x, float z) {
  BiomeType biome = terrain_system_get_biome_at(system, x, z);
  BiomeDefinition *def = &system->biomes[biome];

  // TODO: Implement biome-specific vegetation placement (TODO-26608)
  // if (def->tree_density > 0) { ... }

  // TODO: Implement biome-specific weather (TODO-26609)
  // if (biome == BIOME_SNOW) { start_snowing(); }

  // TODO: Implement biome-specific ambient audio (TODO-26610)
  // play_ambient_sound(def->name);
}

// Hydraulic erosion simulation (TODO-26611)
void terrain_system_hydraulic_erosion(TerrainSystem *system,
                                      TerrainChunk *chunk, float rate,
                                      uint32_t iterations) {
  if (!chunk || !chunk->heightmap)
    return;

  TerrainData data;
  data.width = chunk->heightmap_size;
  data.height = chunk->heightmap_size;
  data.heightmap = chunk->heightmap;

  terrain_hydraulic_erosion(&data, rate, iterations);
}

// Thermal erosion simulation (TODO-26612)
void terrain_system_thermal_erosion(TerrainSystem *system, TerrainChunk *chunk,
                                    float rate, uint32_t iterations) {
  if (!chunk || !chunk->heightmap)
    return;

  TerrainData data;
  data.width = chunk->heightmap_size;
  data.height = chunk->heightmap_size;
  data.heightmap = chunk->heightmap;

  terrain_thermal_erosion(&data, rate, iterations);
}

// GPU-accelerated erosion (TODO-26613)
void terrain_system_gpu_erosion_stub(TerrainSystem *system,
                                     TerrainChunk *chunk) {
  // TODO: Implement using Compute Shader API
  // This requires a GPU buffer of the heightmap and a dispatch call
}
