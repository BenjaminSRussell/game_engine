// include/npc/npc.h
//
// Purpose: Defines the core API and structures for the overall NPC (Non-Player
// Character) system. This header serves as a high-level manager for NPC
// entities within the game world, integrating with the Entity-Component-System
// (ECS), physics engine, and world generation modules. It provides functions
// for the lifecycle management of NPCs, including their creation, updates, and
// interactions with the environment.
//
// Public APIs:
// - `NPCSystem`: The main structure representing the NPC management system,
// holding
//   references to the `ECSWorld` and `PhysicsWorld` instances.
// - `npc_system_init`: Initializes the NPC system, linking it to the ECS and
// physics engines.
// - `npc_system_free`: Frees resources associated with the NPC system.
// - `npc_create`: Creates a new NPC entity of a specified type at a given
// position,
//   registering it with the ECS.
// - `npc_update`: The primary update function for the entire NPC system,
// typically
//   called once per frame to process all active NPCs.
// - `npc_ai_update`: Updates the artificial intelligence and behavior of a
// specific NPC entity.
// - `npc_spawn_in_chunk`: Handles the procedural spawning of NPCs within a
// given chunk.
// - `npc_despawn_distant`: Manages the despawning of NPCs that are too far from
// the player
//   or active game areas to optimize performance.
//
// Ownership: The `NPCSystem` holds references to the `ECSWorld` and
// `PhysicsWorld` but does not own them. It manages the creation and destruction
// of NPC entities within the ECS.
//
// Invariants:
// - An `NPCSystem` must be initialized with `npc_system_init` before creating
// or updating NPCs.
// - `ECSWorld` and `PhysicsWorld` instances must be valid and initialized when
// passed to `npc_system_init`.
// - `NPCType` (from `npc_types.h`) provides the blueprint for NPC creation.
// - `delta_time` should be consistently passed to `npc_update` for accurate
// time progression.
//
#ifndef NPC_H
#define NPC_H

#include <chunk/chunk.h>
#include <ecs/ecs.h>
#include <game_common.h>
#include <math/vec3.h>
#include <npc/npc_types.h>
#include <physics/physics.h>

// Forward declarations (types defined in their respective headers)
struct ECSWorld;
struct PhysicsWorld;
struct WorldGenerator;
struct NPCComponent;

// NPC system
typedef struct NPCSystem {
  struct World *ecs;
  struct PhysicsWorld *physics;
} NPCSystem;

// Initialize/free NPC system
void npc_system_init(NPCSystem *system, struct World *ecs,
                     struct PhysicsWorld *physics);
void npc_system_free(NPCSystem *system);

#include <ecs/components/npc.h>

// Create/update NPC
EntityID npc_create(NPCSystem *system, Vec3 position, NPCType type);
void npc_update(NPCSystem *system, f32 delta_time);
void npc_ai_update(NPCSystem *system, EntityID entity, NPCComponent *npc,
                   f32 delta_time);
void npc_spawn_in_chunk(NPCSystem *system, Chunk *chunk,
                        struct WorldGenerator *generator);
void npc_despawn_distant(NPCSystem *system);
void npc_begin_trading(NPCSystem *system, EntityID npc, EntityID player);
void npc_end_trading(NPCSystem *system, EntityID npc);
void npc_begin_dialogue(NPCSystem *system, EntityID npc, EntityID player);
void npc_end_dialogue(NPCSystem *system, EntityID npc);
i16 npc_get_relation(NPCSystem *system, EntityID npc, EntityID other);
void npc_set_relation(NPCSystem *system, EntityID npc, EntityID other,
                      i16 value);
void npc_adjust_relation(NPCSystem *system, EntityID npc, EntityID other,
                         i16 delta);

#endif // NPC_H
