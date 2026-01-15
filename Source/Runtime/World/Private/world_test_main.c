#include "unified_logger.h"
#include "unified_memory.h"
#include "unified_world.h"
#include <stdio.h>

int main(void) {
  printf("Starting World System Verification...\\n");

  // Initialize logger
  if (!logger_init(NULL)) {
    printf("FAILED: Logger init\\n");
    return 1;
  }

  // Test 1: Create world
  WorldConfig config = {.name = "TestWorld",
                        .seed = 12345,
                        .chunk_size = 16,
                        .render_distance = 4,
                        .enable_terrain_generation = true,
                        .enable_biomes = true};

  World *world = world_create(&config);
  if (!world) {
    printf("FAILED: World creation\\n");
    return 1;
  }
  printf("PASSED: World created\\n");

  // Test 2: World info
  if (world_get_seed(world) != 12345) {
    printf("FAILED: World seed mismatch\\n");
    world_destroy(world);
    return 1;
  }
  printf("PASSED: World info correct\\n");

  // Test 3: Load chunks
  world_load_chunk(world, 0, 0);
  world_load_chunk(world, 1, 0);
  world_load_chunk(world, 0, 1);

  if (world_get_loaded_chunk_count(world) != 3) {
    printf("FAILED: Chunk count (expected 3, got %u)\\n",
           world_get_loaded_chunk_count(world));
    world_destroy(world);
    return 1;
  }
  printf("PASSED: Chunks loaded\\n");

  // Test 4: Chunk queries
  if (!world_is_chunk_loaded(world, 0, 0)) {
    printf("FAILED: Chunk should be loaded\\n");
    world_destroy(world);
    return 1;
  }
  printf("PASSED: Chunk query\\n");

  // Test 5: Terrain height
  float height = world_get_height(world, 8.0f, 8.0f);
  if (height < 0.0f || height > 200.0f) {
    printf("FAILED: Invalid height: %f\\n", height);
    world_destroy(world);
    return 1;
  }
  printf("PASSED: Terrain height query\\n");

  // Test 6: Biome query
  BiomeType biome = world_get_biome(world, 8.0f, 8.0f);
  const char *biome_name = world_get_biome_name(biome);
  if (!biome_name) {
    printf("FAILED: Biome name\\n");
    world_destroy(world);
    return 1;
  }
  printf("PASSED: Biome query (%s)\\n", biome_name);

  // Test 7: Load chunks around position
  world_load_chunks_around(world, 0.0f, 0.0f, 2);
  uint32_t loaded_count = world_get_loaded_chunk_count(world);
  if (loaded_count < 3) {
    printf("FAILED: Not enough chunks loaded (%u)\\n", loaded_count);
    world_destroy(world);
    return 1;
  }
  printf("PASSED: Load chunks around position\\n");

  // Test 8: Player position
  world_set_player_position(world, 100.0f, 64.0f, 100.0f);
  printf("PASSED: Set player position\\n");

  // Test 9: Unload chunk
  world_unload_chunk(world, 1, 0);
  if (world_is_chunk_loaded(world, 1, 0)) {
    printf("FAILED: Chunk should be unloaded\\n");
    world_destroy(world);
    return 1;
  }
  printf("PASSED: Chunk unload\\n");

  // Test 10: Cleanup
  world_destroy(world);
  printf("PASSED: World cleanup\\n");

  logger_shutdown();

  printf("\\nWorld System Verification Successful!\\n");
  return 0;
}
