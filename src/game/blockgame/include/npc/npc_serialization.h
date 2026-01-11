// include/npc/npc_serialization.h
//
// Purpose: Defines the public API for the serialization and deserialization of
// NPC (Non-Player Character) data. This header provides functions that
// integrate with the game's generic `SaveSystem` to enable persistent storage
// and retrieval of NPC states, crucial for maintaining dynamic game worlds
// across sessions.
//
// Public APIs:
// - `save_npcs`: Serializes the data of all NPCs managed by the `NPCSystem`
// into
//   the `SaveSystem`, associated with a given world name.
// - `load_npcs`: Deserializes NPC data from the `SaveSystem` for a specified
// world,
//   recreating NPCs and their states within the `NPCSystem` and `ECSWorld`.
//
// Ownership: These functions operate on data owned by the `SaveSystem` (for
// storage), the `NPCSystem` (for NPC management), and the `ECSWorld` (for
// entity recreation). They do not own any persistent data themselves but
// facilitate its transfer.
//
// Invariants:
// - A valid `SaveSystem` must be initialized before calling these functions.
// - The `NPCSystem` and `ECSWorld` instances must be valid and initialized.
// - `world_name` must be a non-null, valid identifier for the save data.
// - `id_map` is typically used during loading to remap `EntityID`s if they
// change
//   between save sessions, ensuring correct references.
// - Assumes `save.h` and `npc.h` structures are correctly defined.
//
#ifndef NPC_SERIALIZATION_H
#define NPC_SERIALIZATION_H

#include "../save/save.h"
#include "npc.h"

// Forward declarations
struct SaveSystem;
struct NPCSystem;
struct ECSWorld;

bool save_npcs(struct SaveSystem *save, const char *world_name,
               struct NPCSystem *npc_system);
bool load_npcs(struct SaveSystem *save, const char *world_name,
               struct NPCSystem *npc_system, struct World *ecs,
               EntityID *id_map);

#endif // NPC_SERIALIZATION_H
