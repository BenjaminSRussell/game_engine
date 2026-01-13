#include "scene/world/terrain_mega/mega_terrain.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Helper to get a pointer to a chunk at given world coordinates.
 */
static TerrainChunk *get_chunk_at(TerrainSystem *system, float x, float z) {
  int32_t cx = (int32_t)floorf(x / TERRAIN_CHUNK_SIZE);
  int32_t cz = (int32_t)floorf(z / TERRAIN_CHUNK_SIZE);

  for (uint32_t i = 0; i < system->chunk_count; ++i) {
    if (system->chunks[i].chunk_x == cx && system->chunks[i].chunk_z == cz) {
      return &system->chunks[i];
    }
  }
  return NULL;
}

float terrain_get_height_at(TerrainSystem *system, float x, float z) {
  TerrainChunk *chunk = get_chunk_at(system, x, z);
  if (!chunk || !chunk->heightmap)
    return 0.0f;

  // Relative coordinates within chunk
  float rx = x - (chunk->chunk_x * TERRAIN_CHUNK_SIZE);
  float rz = z - (chunk->chunk_z * TERRAIN_CHUNK_SIZE);

  // Grid coordinates
  float gx = (rx / TERRAIN_CHUNK_SIZE) * (chunk->heightmap_size - 1);
  float gz = (rz / TERRAIN_CHUNK_SIZE) * (chunk->heightmap_size - 1);

  int32_t x0 = (int32_t)floorf(gx);
  int32_t z0 = (int32_t)floorf(gz);
  int32_t x1 = x0 + 1;
  int32_t z1 = z0 + 1;

  if (x1 >= chunk->heightmap_size)
    x1 = chunk->heightmap_size - 1;
  if (z1 >= chunk->heightmap_size)
    z1 = chunk->heightmap_size - 1;

  float dx = gx - x0;
  float dz = gz - z0;

  float h00 = chunk->heightmap[z0 * chunk->heightmap_size + x0];
  float h10 = chunk->heightmap[z0 * chunk->heightmap_size + x1];
  float h01 = chunk->heightmap[z1 * chunk->heightmap_size + x0];
  float h11 = chunk->heightmap[z1 * chunk->heightmap_size + x1];

  // Bilinear interpolation
  float h0 = h00 * (1.0f - dx) + h10 * dx;
  float h1 = h01 * (1.0f - dx) + h11 * dx;

  return h0 * (1.0f - dz) + h1 * dz;
}

void terrain_get_normal_at(TerrainSystem *system, float x, float z,
                           float normal[3]) {
  float eps = 0.1f;
  float hL = terrain_get_height_at(system, x - eps, z);
  float hR = terrain_get_height_at(system, x + eps, z);
  float hD = terrain_get_height_at(system, x, z - eps);
  float hU = terrain_get_height_at(system, x, z + eps);

  normal[0] = hL - hR;
  normal[1] = 2.0f * eps;
  normal[2] = hD - hU;

  float len = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] +
                    normal[2] * normal[2]);
  if (len > 0.0f) {
    normal[0] /= len;
    normal[1] /= len;
    normal[2] /= len;
  } else {
    normal[0] = 0.0f;
    normal[1] = 1.0f;
    normal[2] = 0.0f;
  }
}

BiomeType terrain_get_biome_at(TerrainSystem *system, float x, float z) {
  // Placeholder logic for biome selection
  // In a real system, this would sample noise for temperature/moisture
  return BIOME_PLAINS;
}

bool terrain_raycast(TerrainSystem *system, float origin[3], float direction[3],
                     float hit_pos[3]) {
  float step_size = 1.0f;
  float max_dist = 1000.0f;
  float current_dist = 0.0f;

  while (current_dist < max_dist) {
    float p[3] = {origin[0] + direction[0] * current_dist,
                  origin[1] + direction[1] * current_dist,
                  origin[2] + direction[2] * current_dist};

    float terrain_h = terrain_get_height_at(system, p[0], p[2]);
    if (p[1] <= terrain_h) {
      if (hit_pos) {
        hit_pos[0] = p[0];
        hit_pos[1] = p[1];
        hit_pos[2] = p[2];
      }
      return true;
    }

    current_dist += step_size;
  }

  return false;
}

void terrain_sculpt(TerrainSystem *system, float x, float z, float radius,
                    float strength) {
  int32_t min_cx = (int32_t)floorf((x - radius) / TERRAIN_CHUNK_SIZE);
  int32_t max_cx = (int32_t)floorf((x + radius) / TERRAIN_CHUNK_SIZE);
  int32_t min_cz = (int32_t)floorf((z - radius) / TERRAIN_CHUNK_SIZE);
  int32_t max_cz = (int32_t)floorf((z + radius) / TERRAIN_CHUNK_SIZE);

  for (uint32_t i = 0; i < system->chunk_count; ++i) {
    TerrainChunk *chunk = &system->chunks[i];
    if (chunk->chunk_x >= min_cx && chunk->chunk_x <= max_cx &&
        chunk->chunk_z >= min_cz && chunk->chunk_z <= max_cz) {

      for (uint32_t gz = 0; gz < chunk->heightmap_size; ++gz) {
        for (uint32_t gx = 0; gx < chunk->heightmap_size; ++gx) {
          float world_x =
              (chunk->chunk_x * TERRAIN_CHUNK_SIZE) +
              ((float)gx / (chunk->heightmap_size - 1)) * TERRAIN_CHUNK_SIZE;
          float world_z =
              (chunk->chunk_z * TERRAIN_CHUNK_SIZE) +
              ((float)gz / (chunk->heightmap_size - 1)) * TERRAIN_CHUNK_SIZE;

          float dx = world_x - x;
          float dz = world_z - z;
          float dist_sq = dx * dx + dz * dz;

          if (dist_sq < radius * radius) {
            float dist = sqrtf(dist_sq);
            float falloff = 1.0f - (dist / radius);
            chunk->heightmap[gz * chunk->heightmap_size + gx] +=
                strength * falloff;
            chunk->needs_update = true;
          }
        }
      }
    }
  }
}

void terrain_paint_material(TerrainSystem *system, float x, float z,
                            float radius, uint32_t material_id) {
  // Placeholder for material painting
}

bool terrain_save(TerrainSystem *system, const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f)
    return false;

  fwrite(&system->config, sizeof(TerrainConfig), 1, f);
  fwrite(&system->chunk_count, sizeof(uint32_t), 1, f);

  for (uint32_t i = 0; i < system->chunk_count; ++i) {
    TerrainChunk *chunk = &system->chunks[i];
    fwrite(chunk, sizeof(TerrainChunk), 1, f);
    if (chunk->heightmap) {
      fwrite(chunk->heightmap, sizeof(float),
             chunk->heightmap_size * chunk->heightmap_size, f);
    }
  }

  fclose(f);
  return true;
}

bool terrain_load(TerrainSystem *system, const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;

  fread(&system->config, sizeof(TerrainConfig), 1, f);
  fread(&system->chunk_count, sizeof(uint32_t), 1, f);

  // Assuming system->chunks is already allocated or we allocate here
  for (uint32_t i = 0; i < system->chunk_count; ++i) {
    TerrainChunk *chunk = &system->chunks[i];
    fread(chunk, sizeof(TerrainChunk), 1, f);
    if (chunk->heightmap_size > 0) {
      chunk->heightmap = (float *)malloc(sizeof(float) * chunk->heightmap_size *
                                         chunk->heightmap_size);
      fread(chunk->heightmap, sizeof(float),
            chunk->heightmap_size * chunk->heightmap_size, f);
    }
  }

  fclose(f);
  return true;
}

void terrain_export_heightmap(TerrainSystem *system, const char *path) {
  // Basic export: combine loaded chunks into a single large heightmap if
  // possible For now, just a placeholder
}

void terrain_import_heightmap(TerrainSystem *system, const char *path) {
  // Basic import logic
}

void terrain_erosion_preview(TerrainSystem *system, ErosionSettings *settings) {
  // For preview, we apply a very fast, single-iteration hydraulic erosion
  // to give the user an idea of the effect.
  for (uint32_t i = 0; i < system->chunk_count; ++i) {
    TerrainChunk *chunk = &system->chunks[i];
    if (!chunk->heightmap)
      continue;

    // Backup current state for undo (in a real system this would go to a stack)
    // For now, we'll just simulate a preview effect by smoothing
    for (uint32_t y = 1; y < chunk->heightmap_size - 1; ++y) {
      for (uint32_t x = 1; x < chunk->heightmap_size - 1; ++x) {
        float avg = (chunk->heightmap[y * chunk->heightmap_size + x] +
                     chunk->heightmap[(y - 1) * chunk->heightmap_size + x] +
                     chunk->heightmap[(y + 1) * chunk->heightmap_size + x] +
                     chunk->heightmap[y * chunk->heightmap_size + (x - 1)] +
                     chunk->heightmap[y * chunk->heightmap_size + (x + 1)]) /
                    5.0f;
        chunk->heightmap[y * chunk->heightmap_size + x] = avg;
      }
    }
    chunk->needs_update = true;
  }
}

void terrain_erosion_undo(TerrainSystem *system) {
  // In this simplified implementation, undo would reload from disk
  // or from a memory-cached backup.
  printf("Erosion undo logic triggered.\n");
}
