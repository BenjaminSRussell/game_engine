#ifndef PLANT_GENERATION_H
#define PLANT_GENERATION_H

#include "../chunk/chunk.h"
#include "../game_common.h"
#include "../world/generator.h"

// Plant types with biome affinities
typedef enum {
  PLANT_GRASS_SHORT,
  PLANT_GRASS_TALL,
  PLANT_FLOWER_MIXED,
  PLANT_MUSHROOM,
  PLANT_DEAD_BUSH,
  PLANT_FERN,
  PLANT_LILY_PAD,
  PLANT_CACTUS
} PlantType;

// Generate plants for a chunk after terrain/trees
void plant_generate_in_chunk(WorldGenerator *gen, Chunk *chunk);

// Place specific plant type at location (returns true if placed)
bool plant_try_place(Chunk *chunk, i32 x, i32 y, i32 z, PlantType type,
                     u32 seed);

// Check if block can support a plant
bool plant_can_place_on(BlockID block, PlantType type);

#endif // PLANT_GENERATION_H
