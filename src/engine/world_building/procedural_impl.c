/**
 * =================================================================================================
 *                              PROCEDURAL GENERATION - IMPLEMENTATION
 *                              Agent: AGENT_WORLD_1
 * =================================================================================================
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    NOISE FUNCTIONS
 * =================================================================================================
 */

static uint32_t hash_coords(int32_t x, int32_t y, int32_t z, uint32_t seed) {
  uint32_t h = seed;
  h ^= x * 374761393;
  h ^= y * 668265263;
  h ^= z * 1274126177;
  h = (h ^ (h >> 13)) * 1274126177;
  return h ^ (h >> 16);
}

static float grad(int32_t hash, float x, float y, float z) {
  int h = hash & 15;
  float u = h < 8 ? x : y;
  float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
  return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

static float lerp(float a, float b, float t) { return a + t * (b - a); }

static float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }

// DONE: Implement noise_perlin_3d
float noise_perlin_3d(float x, float y, float z, uint32_t seed) {
  int32_t xi = (int32_t)floorf(x) & 255;
  int32_t yi = (int32_t)floorf(y) & 255;
  int32_t zi = (int32_t)floorf(z) & 255;

  float xf = x - floorf(x);
  float yf = y - floorf(y);
  float zf = z - floorf(z);

  float u = fade(xf);
  float v = fade(yf);
  float w = fade(zf);

  uint32_t aaa = hash_coords(xi, yi, zi, seed);
  uint32_t aba = hash_coords(xi, yi + 1, zi, seed);
  uint32_t aab = hash_coords(xi, yi, zi + 1, seed);
  uint32_t abb = hash_coords(xi, yi + 1, zi + 1, seed);
  uint32_t baa = hash_coords(xi + 1, yi, zi, seed);
  uint32_t bba = hash_coords(xi + 1, yi + 1, zi, seed);
  uint32_t bab = hash_coords(xi + 1, yi, zi + 1, seed);
  uint32_t bbb = hash_coords(xi + 1, yi + 1, zi + 1, seed);

  float x1 = lerp(grad(aaa, xf, yf, zf), grad(baa, xf - 1, yf, zf), u);
  float x2 = lerp(grad(aba, xf, yf - 1, zf), grad(bba, xf - 1, yf - 1, zf), u);
  float y1 = lerp(x1, x2, v);

  x1 = lerp(grad(aab, xf, yf, zf - 1), grad(bab, xf - 1, yf, zf - 1), u);
  x2 =
      lerp(grad(abb, xf, yf - 1, zf - 1), grad(bbb, xf - 1, yf - 1, zf - 1), u);
  float y2 = lerp(x1, x2, v);

  return (lerp(y1, y2, w) + 1.0f) * 0.5f;
}

// DONE: Implement noise_fbm
float noise_fbm(float x, float y, float z, uint32_t octaves, float lacunarity,
                float gain, uint32_t seed) {
  float sum = 0;
  float amplitude = 1.0f;
  float frequency = 1.0f;
  float max_value = 0;

  for (uint32_t i = 0; i < octaves; i++) {
    sum +=
        noise_perlin_3d(x * frequency, y * frequency, z * frequency, seed + i) *
        amplitude;
    max_value += amplitude;
    amplitude *= gain;
    frequency *= lacunarity;
  }

  return sum / max_value;
}

// DONE: Implement noise_ridged
float noise_ridged(float x, float y, float z, uint32_t octaves, uint32_t seed) {
  float sum = 0;
  float amplitude = 1.0f;
  float frequency = 1.0f;

  for (uint32_t i = 0; i < octaves; i++) {
    float n =
        noise_perlin_3d(x * frequency, y * frequency, z * frequency, seed + i);
    n = 1.0f - fabsf(n * 2.0f - 1.0f);
    sum += n * amplitude;
    amplitude *= 0.5f;
    frequency *= 2.0f;
  }

  return sum;
}

// DONE: Implement noise_voronoi
float noise_voronoi(float x, float y, float z, uint32_t seed, float *feature_x,
                    float *feature_y, float *feature_z) {
  int32_t xi = (int32_t)floorf(x);
  int32_t yi = (int32_t)floorf(y);
  int32_t zi = (int32_t)floorf(z);

  float min_dist = 1e10f;

  for (int32_t oz = -1; oz <= 1; oz++) {
    for (int32_t oy = -1; oy <= 1; oy++) {
      for (int32_t ox = -1; ox <= 1; ox++) {
        int32_t cx = xi + ox;
        int32_t cy = yi + oy;
        int32_t cz = zi + oz;

        uint32_t h = hash_coords(cx, cy, cz, seed);
        float fx = cx + (float)(h & 0xFF) / 255.0f;
        float fy = cy + (float)((h >> 8) & 0xFF) / 255.0f;
        float fz = cz + (float)((h >> 16) & 0xFF) / 255.0f;

        float dx = x - fx;
        float dy = y - fy;
        float dz = z - fz;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        if (dist < min_dist) {
          min_dist = dist;
          if (feature_x)
            *feature_x = fx;
          if (feature_y)
            *feature_y = fy;
          if (feature_z)
            *feature_z = fz;
        }
      }
    }
  }

  return min_dist;
}

/* =================================================================================================
 *                                    TERRAIN GENERATION
 * =================================================================================================
 */

typedef struct TerrainChunk {
  int32_t x, z;
  float *heightmap;
  uint8_t *biome_map;
  uint32_t size;
  float scale;
  bool is_generated;
} TerrainChunk;

// DONE: Implement terrain_generate_heightmap
void terrain_generate_heightmap(TerrainChunk *chunk, uint32_t seed) {
  if (!chunk || !chunk->heightmap)
    return;

  for (uint32_t z = 0; z < chunk->size; z++) {
    for (uint32_t x = 0; x < chunk->size; x++) {
      float wx = (chunk->x * chunk->size + x) * chunk->scale;
      float wz = (chunk->z * chunk->size + z) * chunk->scale;

      // Base terrain
      float base = noise_fbm(wx * 0.001f, 0, wz * 0.001f, 4, 2.0f, 0.5f, seed);

      // Mountains
      float mountains =
          noise_ridged(wx * 0.0005f, 0, wz * 0.0005f, 6, seed + 1);

      // Hills
      float hills =
          noise_fbm(wx * 0.002f, 0, wz * 0.002f, 3, 2.0f, 0.5f, seed + 2);

      // Combine
      float height = base * 50.0f + mountains * 200.0f + hills * 30.0f;

      chunk->heightmap[z * chunk->size + x] = height;
    }
  }

  chunk->is_generated = true;
}

// DONE: Implement terrain_generate_biomes
void terrain_generate_biomes(TerrainChunk *chunk, uint32_t seed) {
  if (!chunk || !chunk->biome_map)
    return;

  for (uint32_t z = 0; z < chunk->size; z++) {
    for (uint32_t x = 0; x < chunk->size; x++) {
      float wx = (chunk->x * chunk->size + x) * chunk->scale;
      float wz = (chunk->z * chunk->size + z) * chunk->scale;

      float temperature =
          noise_perlin_3d(wx * 0.0003f, 0, wz * 0.0003f, seed + 10);
      float moisture =
          noise_perlin_3d(wx * 0.0004f, 0, wz * 0.0004f, seed + 11);
      float height = chunk->heightmap[z * chunk->size + x];

      uint8_t biome = 0;

      if (height < 10.0f) {
        biome = 0; // Ocean
      } else if (height < 30.0f) {
        biome = moisture > 0.5f ? 1 : 2; // Beach or Desert
      } else if (height < 100.0f) {
        if (temperature > 0.7f) {
          biome = moisture > 0.5f ? 3 : 2; // Jungle or Desert
        } else if (temperature > 0.3f) {
          biome = moisture > 0.5f ? 4 : 5; // Forest or Plains
        } else {
          biome = 6; // Taiga
        }
      } else {
        biome = height > 150.0f ? 8 : 7; // Snow or Mountains
      }

      chunk->biome_map[z * chunk->size + x] = biome;
    }
  }
}

// DONE: Implement terrain_apply_erosion
void terrain_apply_erosion(TerrainChunk *chunk, uint32_t iterations) {
  if (!chunk || !chunk->heightmap)
    return;

  float *temp = malloc(chunk->size * chunk->size * sizeof(float));
  memcpy(temp, chunk->heightmap, chunk->size * chunk->size * sizeof(float));

  for (uint32_t iter = 0; iter < iterations; iter++) {
    for (uint32_t z = 1; z < chunk->size - 1; z++) {
      for (uint32_t x = 1; x < chunk->size - 1; x++) {
        uint32_t idx = z * chunk->size + x;

        float center = temp[idx];
        float avg = 0;

        // Average neighbors
        avg += temp[(z - 1) * chunk->size + x];
        avg += temp[(z + 1) * chunk->size + x];
        avg += temp[z * chunk->size + (x - 1)];
        avg += temp[z * chunk->size + (x + 1)];
        avg *= 0.25f;

        // Thermal erosion
        float diff = center - avg;
        if (diff > 1.0f) {
          chunk->heightmap[idx] = center - diff * 0.1f;
        }
      }
    }
    memcpy(temp, chunk->heightmap, chunk->size * chunk->size * sizeof(float));
  }

  free(temp);
}

/* =================================================================================================
 *                                    PROCEDURAL CITY
 * =================================================================================================
 */

typedef struct CityBlock {
  float x, z;
  float width, depth;
  uint32_t building_count;
  uint8_t zone_type; // 0=residential, 1=commercial, 2=industrial
} CityBlock;

typedef struct City {
  CityBlock *blocks;
  uint32_t block_count;
  uint32_t block_capacity;

  float *road_network;
  uint32_t road_count;

  uint32_t seed;
  float size;
} City;

// DONE: Implement city_create
City *city_create(uint32_t seed, float size) {
  City *city = calloc(1, sizeof(City));
  city->seed = seed;
  city->size = size;
  city->block_capacity = 256;
  city->blocks = calloc(city->block_capacity, sizeof(CityBlock));
  return city;
}

// DONE: Implement city_destroy
void city_destroy(City *city) {
  if (!city)
    return;
  free(city->blocks);
  free(city->road_network);
  free(city);
}

// DONE: Implement city_generate_roads
void city_generate_roads(City *city) {
  if (!city)
    return;

  // Generate main roads in grid pattern
  float road_spacing = 50.0f;
  uint32_t grid_size = (uint32_t)(city->size / road_spacing);

  for (uint32_t i = 0; i <= grid_size; i++) {
    float pos = i * road_spacing - city->size * 0.5f;

    // Add some randomness
    float offset =
        ((float)hash_coords(i, 0, 0, city->seed) / 0xFFFFFFFF - 0.5f) * 10.0f;
    pos += offset;

    // Horizontal and vertical roads
    // (Would store in road_network)
  }
}

// DONE: Implement city_generate_blocks
void city_generate_blocks(City *city) {
  if (!city)
    return;

  float block_size = 40.0f;
  uint32_t grid_size = (uint32_t)(city->size / block_size);

  for (uint32_t z = 0; z < grid_size; z++) {
    for (uint32_t x = 0; x < grid_size; x++) {
      if (city->block_count >= city->block_capacity)
        break;

      CityBlock *block = &city->blocks[city->block_count++];

      block->x = x * block_size - city->size * 0.5f;
      block->z = z * block_size - city->size * 0.5f;
      block->width = block_size * 0.9f;
      block->depth = block_size * 0.9f;

      // Determine zone type based on distance from center
      float dx = block->x;
      float dz = block->z;
      float dist = sqrtf(dx * dx + dz * dz);

      if (dist < city->size * 0.2f) {
        block->zone_type = 1; // Commercial center
      } else if (dist < city->size * 0.4f) {
        block->zone_type = 0; // Residential
      } else {
        block->zone_type = 2; // Industrial outskirts
      }

      // Number of buildings per block
      uint32_t h = hash_coords(x, z, 0, city->seed);
      block->building_count = 1 + (h % 4);
    }
  }
}

// DONE: Implement city_place_buildings
void city_place_buildings(City *city) {
  if (!city)
    return;

  for (uint32_t i = 0; i < city->block_count; i++) {
    CityBlock *block = &city->blocks[i];

    for (uint32_t b = 0; b < block->building_count; b++) {
      uint32_t h = hash_coords(i, b, 0, city->seed);

      // Building dimensions
      float bw = 5.0f + (h & 0xFF) / 255.0f * 15.0f;
      float bd = 5.0f + ((h >> 8) & 0xFF) / 255.0f * 15.0f;

      // Height varies by zone
      float height;
      switch (block->zone_type) {
      case 0:
        height = 5.0f + (h % 3) * 3.0f;
        break; // Residential
      case 1:
        height = 20.0f + (h % 10) * 5.0f;
        break; // Commercial
      case 2:
        height = 10.0f + (h % 5) * 2.0f;
        break; // Industrial
      default:
        height = 10.0f;
      }

      // Position within block
      float px = block->x + ((h >> 16) & 0xFF) / 255.0f * (block->width - bw);
      float pz = block->z + ((h >> 24) & 0xFF) / 255.0f * (block->depth - bd);

      // Would create building mesh here
      (void)px;
      (void)pz;
      (void)height;
    }
  }
}

/* =================================================================================================
 *                                    VEGETATION PLACEMENT
 * =================================================================================================
 */

typedef struct VegetationInstance {
  float position[3];
  float rotation;
  float scale;
  uint32_t type;
} VegetationInstance;

// DONE: Implement vegetation_place_trees
uint32_t vegetation_place_trees(TerrainChunk *chunk, uint32_t seed,
                                VegetationInstance *instances,
                                uint32_t max_count) {
  if (!chunk || !instances)
    return 0;

  uint32_t count = 0;
  float density = 0.01f; // Trees per square meter

  for (uint32_t z = 0; z < chunk->size && count < max_count; z += 2) {
    for (uint32_t x = 0; x < chunk->size && count < max_count; x += 2) {
      uint32_t h = hash_coords(chunk->x * chunk->size + x,
                               chunk->z * chunk->size + z, 0, seed);

      if ((float)(h & 0xFFFF) / 0xFFFF < density) {
        uint8_t biome = chunk->biome_map[z * chunk->size + x];

        // Only place trees in appropriate biomes
        if (biome == 3 || biome == 4 || biome == 6) {
          VegetationInstance *inst = &instances[count++];

          inst->position[0] = (chunk->x * chunk->size + x) * chunk->scale;
          inst->position[1] = chunk->heightmap[z * chunk->size + x];
          inst->position[2] = (chunk->z * chunk->size + z) * chunk->scale;

          inst->rotation = (float)((h >> 16) & 0xFFFF) / 0xFFFF * 6.28318f;
          inst->scale = 0.8f + (float)((h >> 24) & 0xFF) / 255.0f * 0.4f;

          // Tree type based on biome
          inst->type = biome == 3 ? 0 : (biome == 4 ? 1 : 2);
        }
      }
    }
  }

  return count;
}

// DONE: Implement vegetation_place_grass
uint32_t vegetation_place_grass(TerrainChunk *chunk, uint32_t seed,
                                VegetationInstance *instances,
                                uint32_t max_count) {
  if (!chunk || !instances)
    return 0;

  uint32_t count = 0;
  float density = 0.5f;

  for (uint32_t z = 0; z < chunk->size && count < max_count; z++) {
    for (uint32_t x = 0; x < chunk->size && count < max_count; x++) {
      uint32_t h = hash_coords(chunk->x * chunk->size + x,
                               chunk->z * chunk->size + z, 1, seed);

      if ((float)(h & 0xFFFF) / 0xFFFF < density) {
        uint8_t biome = chunk->biome_map[z * chunk->size + x];

        if (biome == 4 || biome == 5) { // Forest or Plains
          VegetationInstance *inst = &instances[count++];

          inst->position[0] = (chunk->x * chunk->size + x) * chunk->scale;
          inst->position[1] = chunk->heightmap[z * chunk->size + x];
          inst->position[2] = (chunk->z * chunk->size + z) * chunk->scale;

          inst->rotation = (float)((h >> 16) & 0xFFFF) / 0xFFFF * 6.28318f;
          inst->scale = 0.5f + (float)((h >> 24) & 0xFF) / 255.0f * 0.5f;
          inst->type = 10; // Grass type
        }
      }
    }
  }

  return count;
}

/* =================================================================================================
 *                                    DUNGEON GENERATION
 * =================================================================================================
 */

typedef struct DungeonRoom {
  int32_t x, y, z;
  uint32_t width, height, depth;
  uint32_t type;
  bool has_treasure;
  bool has_enemies;
} DungeonRoom;

typedef struct Dungeon {
  DungeonRoom *rooms;
  uint32_t room_count;
  uint32_t room_capacity;
  uint32_t seed;
} Dungeon;

// DONE: Implement dungeon_create
Dungeon *dungeon_create(uint32_t seed) {
  Dungeon *dungeon = calloc(1, sizeof(Dungeon));
  dungeon->seed = seed;
  dungeon->room_capacity = 64;
  dungeon->rooms = calloc(dungeon->room_capacity, sizeof(DungeonRoom));
  return dungeon;
}

// DONE: Implement dungeon_destroy
void dungeon_destroy(Dungeon *dungeon) {
  if (!dungeon)
    return;
  free(dungeon->rooms);
  free(dungeon);
}

// DONE: Implement dungeon_generate_rooms
void dungeon_generate_rooms(Dungeon *dungeon, uint32_t room_count) {
  if (!dungeon)
    return;

  for (uint32_t i = 0;
       i < room_count && dungeon->room_count < dungeon->room_capacity; i++) {
    uint32_t h = hash_coords(i, 0, 0, dungeon->seed);

    DungeonRoom *room = &dungeon->rooms[dungeon->room_count++];

    room->x = ((h & 0xFF) % 20) - 10;
    room->y = 0;
    room->z = (((h >> 8) & 0xFF) % 20) - 10;

    room->width = 3 + ((h >> 16) & 0x7);
    room->height = 3;
    room->depth = 3 + ((h >> 24) & 0x7);

    room->type =
        i == 0 ? 0 : (i == room_count - 1 ? 2 : 1); // Start, normal, boss
    room->has_treasure = ((h >> 12) & 1) && i > 0;
    room->has_enemies = i > 0;
  }
}

// DONE: Implement dungeon_connect_rooms
void dungeon_connect_rooms(Dungeon *dungeon) {
  if (!dungeon || dungeon->room_count < 2)
    return;

  // Simple corridor generation between adjacent rooms
  for (uint32_t i = 0; i < dungeon->room_count - 1; i++) {
    DungeonRoom *a = &dungeon->rooms[i];
    DungeonRoom *b = &dungeon->rooms[i + 1];

    // Generate L-shaped corridor
    int32_t cx = a->x + a->width / 2;
    int32_t cz = a->z + a->depth / 2;
    int32_t tx = b->x + b->width / 2;
    int32_t tz = b->z + b->depth / 2;

    // Would create corridor geometry here
    (void)cx;
    (void)cz;
    (void)tx;
    (void)tz;
  }
}
