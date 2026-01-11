// Tree generation entry points.
// Roadmap: docs/TREE_GENERATION_ROADMAP.md.
#include <block/block.h>
#include <chunk/chunk.h>
#include <math.h>
#include <stdlib.h>
#include <world/generator.h>
#include <world/tree_varieties.h>

// Simple hash for deterministic tree placement
static u32 tree_hash(i32 x, i32 z, u32 seed) {
  return ((u32)x * 73856093) ^ ((u32)z * 19349663) ^ seed;
}

// Generate tree at position
void world_generator_generate_tree(WorldGenerator *gen, Chunk *chunk, i32 x,
                                   i32 y, i32 z) {
  if (!gen || !chunk)
    return;

  // Determine tree type based on biome
  BiomeType biome = world_generator_get_biome(
      gen, chunk->pos.x * CHUNK_SIZE + x, y, chunk->pos.z * CHUNK_SIZE + z);

  TreeType tree_type = TREE_OAK;
  u32 hash = tree_hash(x, z, gen->seed);

  switch (biome) {
  case BIOME_FOREST:
    tree_type = (hash % 2 == 0) ? TREE_OAK : TREE_BIRCH;
    break;
  case BIOME_TAIGA:
    tree_type = TREE_SPRUCE;
    break;
  case BIOME_JUNGLE:
    tree_type = TREE_JUNGLE;
    break;
  case BIOME_PLAINS:
    // Small chance for rubber trees in plains
    if (hash % 100 < 5) {
      tree_type = TREE_RUBBER;
    } else {
      tree_type = TREE_OAK;
    }
    break;
  default:
    tree_type = TREE_OAK;
    break;
  }

// VFX: Leaf particle effects and tree growth sounds
// Note: Actual particle/audio systems would be called here when fully
// integrated For now, we provide hooks that can be wired up later
#if 0 // Enable when particle/audio systems are ready
    {
        // Calculate world position for effects
        i32 world_x = chunk->pos.x * CHUNK_SIZE + x;
        i32 world_z = chunk->pos.z * CHUNK_SIZE + z;
        
        // Play growth sound based on tree type
        const char *growth_sound = NULL;
        switch (tree_type) {
            case TREE_OAK: growth_sound = "tree/oak_grow"; break;
            case TREE_BIRCH: growth_sound = "tree/birch_grow"; break;
            case TREE_SPRUCE: growth_sound = "tree/spruce_grow"; break;
            case TREE_JUNGLE: growth_sound = "tree/jungle_grow"; break;
            default: growth_sound = "tree/generic_grow"; break;
        }
        // audio_play_3d_sound(growth_sound, vec3(world_x, y, world_z), 1.0f, 1.0f);
        
        // Spawn leaf particle effects (green sparkles)
        // particle_spawn("leaf_sparkle", vec3(world_x, y + 3, world_z), 10);
        
        // Subtle screen shake for large jungle trees near player
        if (tree_type == TREE_JUNGLE) {
            // Vec3 player_pos = player_get_position();
            // f32 distance = vec3_distance(player_pos, vec3(world_x, y, world_z));
            // if (distance < 20.0f) {
            //     camera_shake(0.1f, 0.3f);  // amplitude, duration
            // }
        }
    }
#endif

  // Generate tree using tree varieties system
  tree_generate(chunk, x, y, z, tree_type, gen->seed);
}

// Generate biome decorations (trees, grass, etc.)
void world_generator_generate_biome_decorations(WorldGenerator *gen,
                                                Chunk *chunk) {
  if (!gen || !chunk)
    return;

  for (i32 x = 2; x < CHUNK_SIZE - 2; x++) {
    for (i32 z = 2; z < CHUNK_SIZE - 2; z++) {
      i32 world_x = chunk->pos.x * CHUNK_SIZE + x;
      i32 world_z = chunk->pos.z * CHUNK_SIZE + z;

      // Get biome
      BiomeType biome = world_generator_get_biome(gen, world_x, 0, world_z);

      // Find surface height
      i32 surface_y = -1;
      for (i32 y = CHUNK_SIZE - 1; y >= 0; y--) {
        BlockID block = chunk_get_block(chunk, x, y, z);
        if (block != BLOCK_AIR && block != BLOCK_WATER) {
          surface_y = y;
          break;
        }
      }

      if (surface_y < 0)
        continue;

      i32 world_y = chunk->pos.y * CHUNK_SIZE + surface_y;

      // Generate decorations based on biome
      u32 hash = tree_hash(world_x, world_z, gen->seed);
      f32 chance = (hash % 1000) / 1000.0f;

      switch (biome) {
      case BIOME_FOREST:
      case BIOME_JUNGLE:
        // Trees in forests
        if (chance < 0.05f) { // 5% chance
          world_generator_generate_tree(gen, chunk, x, surface_y + 1, z);
        }
        break;

      case BIOME_PLAINS:
      case BIOME_SAVANNA:
        // Occasional trees
        if (chance < 0.01f) { // 1% chance
          world_generator_generate_tree(gen, chunk, x, surface_y + 1, z);
        }
        break;

      default:
        break;
      }
    }
  }
}
