// include/ai/npc.h
//
// Purpose: Defines the core API and structures for the overall NPC (Non-Player
// Character) system using the current ECS architecture. This header serves as a 
// high-level manager for NPC entities within the game world, integrating with the 
// modern Entity-Component-System (ECS), physics engine, and world generation modules.
// It provides functions for the lifecycle management of NPCs, including their creation,
// updates, and interactions with the environment.
//
// Public APIs:
// - `NPCSystem`: The main structure representing the NPC management system,
//   holding references to the `World` (ECS) and `PhysicsWorld` instances.
// - `npc_system_init`: Initializes the NPC system, linking it to the ECS and
//   physics engines.
// - `npc_system_free`: Frees resources associated with the NPC system.
// - `npc_create`: Creates a new NPC entity of a specified type at a given
//   position, registering it with the ECS.
// - `npc_update`: The primary update function for the entire NPC system,
//   typically called once per frame to process all active NPCs.
// - `npc_ai_update`: Updates the artificial intelligence and behavior of a
//   specific NPC entity.
// - `npc_spawn_in_chunk`: Handles the procedural spawning of NPCs within a
//   given chunk.
// - `npc_despawn_distant`: Manages the despawning of NPCs that are too far from
//   the player or active game areas to optimize performance.
//
// Ownership: The `NPCSystem` holds references to the `World` (ECS) and
// `PhysicsWorld` but does not own them. It manages the creation and destruction
// of NPC entities within the ECS.
//
// Invariants:
// - An `NPCSystem` must be initialized with `npc_system_init` before creating
//   or updating NPCs.
// - `World` (ECS) and `PhysicsWorld` instances must be valid and initialized when
//   passed to `npc_system_init`.
// - `NPCType` (from `npc_types.h`) provides the blueprint for NPC creation.
// - `delta_time` should be consistently passed to `npc_update` for accurate
//   time progression.
//
#ifndef AI_NPC_H
#define AI_NPC_H

#include <math/vec3.h>
#include <physics/physics.h>
#include <ai/npc_types.h>
#include <ecs/ecs.h>

// Forward declarations
struct WorldGenerator;

// NPC system using modern ECS
typedef struct NPCSystem {
  struct World *ecs;
  struct PhysicsWorld *physics;
} NPCSystem;

// Initialize/free NPC system
void npc_system_init(NPCSystem *system, struct World *ecs,
                     struct PhysicsWorld *physics);
void npc_system_free(NPCSystem *system);

// Create/update NPC
Entity npc_create(NPCSystem *system, Vec3 position, NPCType type);
void npc_update(NPCSystem *system, f32 delta_time);
void npc_ai_update(NPCSystem *system, Entity entity, NPCComponent *npc,
                   f32 delta_time);
void npc_spawn_in_chunk(NPCSystem *system, struct Chunk *chunk,
                        struct WorldGenerator *generator);
void npc_despawn_distant(NPCSystem *system);

// NPC interaction systems
void npc_begin_trading(NPCSystem *system, Entity npc, Entity player);
void npc_end_trading(NPCSystem *system, Entity npc);
void npc_begin_dialogue(NPCSystem *system, Entity npc, Entity player);
void npc_end_dialogue(NPCSystem *system, Entity npc);

// NPC relationship system
i16 npc_get_relation(NPCSystem *system, Entity npc, Entity other);
void npc_set_relation(NPCSystem *system, Entity npc, Entity other,
                      i16 value);
void npc_adjust_relation(NPCSystem *system, Entity npc, Entity other,
                         i16 delta);

// NPC reputation system
i16 npc_get_reputation(NPCSystem *system, Entity npc);
void npc_set_reputation(NPCSystem *system, Entity npc, i16 value);
void npc_adjust_reputation(NPCSystem *system, Entity npc, i16 delta);

#endif // AI_NPC_H
