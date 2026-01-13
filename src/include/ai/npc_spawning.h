// include/ai/npc_spawning.h
//
// Purpose: Defines the NPC spawning system interface for procedural generation
// of NPCs within the game world. This system handles population control,
// biome-appropriate spawning, light-based rules, and distance-based despawning.
//
// Public APIs:
// - `npc_spawn_in_chunk`: Spawns NPCs within a specific chunk based on
//   conditions and population limits.
// - `npc_despawn_distant`: Removes NPCs that are too far from players to
//   optimize performance.
// - `npc_spawn_initial_npcs`: Creates the initial NPC population around
//   the player when the game starts.
//
// Ownership: Functions operate on the provided NPCSystem but do not own it.
//
// Invariants:
// - NPCSystem must be initialized before calling spawning functions.
// - Chunk data must be valid when calling `npc_spawn_in_chunk`.
// - Player position should be current for optimal initial spawning.
//
#ifndef AI_NPC_SPAWNING_H
#define AI_NPC_SPAWNING_H

#include <ai/npc.h>
#include <game/blockgame/include/chunk/chunk.h>
#include <game/blockgame/include/world/generator.h>
#include <math/vec3.h>

// Spawn NPCs in a specific chunk
void npc_spawn_in_chunk(NPCSystem *system, Chunk *chunk,
                        struct WorldGenerator *generator);

// Despawn NPCs that are too far from any player
void npc_despawn_distant(NPCSystem *system);

// Spawn initial NPCs around the player
void npc_spawn_initial_npcs(NPCSystem *system, Vec3 player_pos, u32 radius_chunks);

#endif // AI_NPC_SPAWNING_H
