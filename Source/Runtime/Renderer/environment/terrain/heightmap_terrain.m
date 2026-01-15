// environment/terrain/heightmap_terrain.c
// Complete terrain system implementation
#include "include/environment/terrain/heightmap_terrain.h"
#include "include/core/logger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

TerrainSystem *terrain_create(id<MTLDevice> device, u32 heightmap_width,
                              u32 heightmap_height, f32 world_scale,
                              f32 height_scale) {
  TerrainSystem *terrain = (TerrainSystem *)calloc(1, sizeof(TerrainSystem));

  terrain->heightmap_width = heightmap_width;
  terrain->heightmap_height = heightmap_height;
  terrain->world_scale = world_scale;
  terrain->height_scale = height_scale;

  // Allocate heightmap
  u32 total_pixels = heightmap_width * heightmap_height;
  terrain->heightmap = (f32 *)calloc(total_pixels, sizeof(f32));

  // Create GPU buffers
  u32 vertex_count = heightmap_width * heightmap_height;
  terrain->vertex_buffer = [device
      newBufferWithLength:vertex_count * sizeof(Vec3) * 2 // pos + normal
                  options:MTLResourceStorageModeShared];

  // Initialize LOD distances
  for (u32 i = 0; i < TERRAIN_MAX_LOD_LEVELS; i++) {
    terrain->lod_distances[i] = powf(2.0f, (f32)i) * 100.0f;
  }

  LOG_INFO("Created terrain %ux%u, world scale %.2f, height scale %.2f",
           heightmap_width, heightmap_height, world_scale, height_scale);

  return terrain;
}

void terrain_generate_perlin(TerrainSystem *terrain, u32 seed, u32 octaves,
                             f32 persistence) {
  // Simplified Perlin noise implementation
  for (u32 y = 0; y < terrain->heightmap_height; y++) {
    for (u32 x = 0; x < terrain->heightmap_width; x++) {
      f32 amplitude = 1.0f;
      f32 frequency = 1.0f;
      f32 height = 0.0f;

      for (u32 octave = 0; octave < octaves; octave++) {
        f32 sample_x = (f32)x / terrain->heightmap_width * frequency;
        f32 sample_y = (f32)y / terrain->heightmap_height * frequency;

        // Simple noise function (replace with real Perlin)
        f32 noise_value = sinf(sample_x * 3.14159f) * cosf(sample_y * 3.14159f);
        height += noise_value * amplitude;

        amplitude *= persistence;
        frequency *= 2.0f;
      }

      u32 idx = y * terrain->heightmap_width + x;
      terrain->heightmap[idx] = height * 0.5f + 0.5f; // Normalize to [0,1]
    }
  }

  LOG_INFO("Generated Perlin terrain with %u octaves", octaves);
}

void terrain_apply_erosion(TerrainSystem *terrain, u32 iterations,
                           f32 strength) {
  // Hydraulic erosion simulation
  for (u32 iter = 0; iter < iterations; iter++) {
    for (u32 y = 1; y < terrain->heightmap_height - 1; y++) {
      for (u32 x = 1; x < terrain->heightmap_width - 1; x++) {
        u32 idx = y * terrain->heightmap_width + x;
        f32 center = terrain->heightmap[idx];

        // Find lowest neighbor
        f32 min_height = center;
        for (i32 dy = -1; dy <= 1; dy++) {
          for (i32 dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0)
              continue;
            u32 neighbor_idx = (y + dy) * terrain->heightmap_width + (x + dx);
            if (terrain->heightmap[neighbor_idx] < min_height) {
              min_height = terrain->heightmap[neighbor_idx];
            }
          }
        }

        // Apply erosion
        if (min_height < center) {
          f32 diff = (center - min_height) * strength;
          terrain->heightmap[idx] -= diff * 0.5f;
        }
      }
    }
  }

  LOG_INFO("Applied erosion (%u iterations)", iterations);
}

f32 terrain_get_height(const TerrainSystem *terrain, f32 world_x, f32 world_z) {
  if (!terrain || !terrain->heightmap)
    return 0.0f;

  // Convert world coords to heightmap coords
  f32 hm_x = (world_x / terrain->world_scale) * terrain->heightmap_width;
  f32 hm_z = (world_z / terrain->world_scale) * terrain->heightmap_height;

  // Clamp to bounds
  if (hm_x < 0 || hm_x >= terrain->heightmap_width - 1 || hm_z < 0 ||
      hm_z >= terrain->heightmap_height - 1) {
    return 0.0f;
  }

  // Bilinear interpolation
  u32 x0 = (u32)hm_x;
  u32 z0 = (u32)hm_z;
  u32 x1 = x0 + 1;
  u32 z1 = z0 + 1;

  f32 fx = hm_x - x0;
  f32 fz = hm_z - z0;

  f32 h00 = terrain->heightmap[z0 * terrain->heightmap_width + x0];
  f32 h10 = terrain->heightmap[z0 * terrain->heightmap_width + x1];
  f32 h01 = terrain->heightmap[z1 * terrain->heightmap_width + x0];
  f32 h11 = terrain->heightmap[z1 * terrain->heightmap_width + x1];

  f32 h0 = h00 * (1.0f - fx) + h10 * fx;
  f32 h1 = h01 * (1.0f - fx) + h11 * fx;

  return (h0 * (1.0f - fz) + h1 * fz) * terrain->height_scale;
}

void terrain_destroy(TerrainSystem *terrain) {
  if (!terrain)
    return;

  free(terrain->heightmap);
  terrain->vertex_buffer = nil;
  for (u32 i = 0; i < TERRAIN_MAX_LOD_LEVELS; i++) {
    terrain->index_buffers[i] = nil;
  }
  free(terrain->patches);
  free(terrain);
}

void terrain_build_patches(TerrainSystem *terrain) {
  // Quadtree subdivision
  u32 patch_size = TERRAIN_PATCH_SIZE;
  u32 patches_x = (terrain->heightmap_width + patch_size - 1) / patch_size;
  u32 patches_z = (terrain->heightmap_height + patch_size - 1) / patch_size;

  terrain->patch_count = patches_x * patches_z;
  terrain->patches =
      (TerrainPatch *)calloc(terrain->patch_count, sizeof(TerrainPatch));

  for (u32 pz = 0; pz < patches_z; pz++) {
    for (u32 px = 0; px < patches_x; px++) {
      u32 patch_idx = pz * patches_x + px;
      TerrainPatch *patch = &terrain->patches[patch_idx];

      patch->position = vec2(px * patch_size * terrain->world_scale,
                             pz * patch_size * terrain->world_scale);
      patch->size = patch_size * terrain->world_scale;
      patch->lod_level = 0;

      // Calculate min/max height for this patch
      patch->min_height = 1e30f;
      patch->max_height = -1e30f;

      for (u32 z = 0;
           z < patch_size && (pz * patch_size + z) < terrain->heightmap_height;
           z++) {
        for (u32 x = 0;
             x < patch_size && (px * patch_size + x) < terrain->heightmap_width;
             x++) {
          u32 hm_idx = (pz * patch_size + z) * terrain->heightmap_width +
                       (px * patch_size + x);
          f32 height = terrain->heightmap[hm_idx] * terrain->height_scale;

          if (height < patch->min_height)
            patch->min_height = height;
          if (height > patch->max_height)
            patch->max_height = height;
        }
      }
    }
  }

  LOG_INFO("Built %u terrain patches (%ux%u)", terrain->patch_count, patches_x,
           patches_z);
}

void terrain_add_layer(TerrainSystem *terrain, const char *name,
                       id<MTLTexture> albedo, id<MTLTexture> normal,
                       f32 tiling_scale) {
  if (terrain->layer_count >= TERRAIN_MAX_LAYERS) {
    LOG_WARN("Cannot add terrain layer '%s': max layers reached", name);
    return;
  }

  TerrainLayer *layer = &terrain->layers[terrain->layer_count++];
  strncpy(layer->name, name, sizeof(layer->name) - 1);
  layer->albedo = albedo;
  layer->normal = normal;
  layer->tiling_scale = tiling_scale;
  layer->blend_sharpness = 0.5f;

  LOG_INFO("Added terrain layer '%s' (%.2f tiling)", name, tiling_scale);
}
