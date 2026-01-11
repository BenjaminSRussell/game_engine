// include/world/settlement.h
//
// Purpose: Defines the public API and data structures for a comprehensive
// procedural settlement generation system. This header provides the necessary
// components to create diverse and dynamic settlements (e.g., villages, towns, cities),
// including their buildings, infrastructure, and populations of NPCs. It supports
// generating entire settlements, placing them within the game world (chunks),
// and populating them with specific features and inhabitants.
//
// Public APIs:
// - `SettlementType`: Enumeration defining various types of settlements.
// - `BuildingType`: Enumeration defining different types of buildings that can
//   exist within a settlement (e.g., House, Shop, Farm, Town Hall).
// - `Building`: Structure representing a single building, including its `type`,
//   `position`, `size`, generation status, and capacity/list of resident NPCs.
// - `Settlement`: The main structure encapsulating an entire settlement, detailing
//   its `type`, `center` coordinates, `radius`, collection of `Building`s,
//   lists of `npcs`, generation status, and seed.
// - `SettlementGenerator`: Structure managing multiple settlements and a global seed.
// - `settlement_generator_init`: Initializes the settlement generator with a global seed.
// - `settlement_generator_free`: Frees resources held by the settlement generator.
// - `settlement_generate`: Generates a new `Settlement` instance of a specified type at a given position.
// - `settlement_place_in_chunk`: Integrates a generated settlement's structures into a game `Chunk`.
// - `settlement_generate_building`: Generates a specific type of building within a settlement.
// - `settlement_generate_road`: Generates roads connecting points within a settlement.
// - `settlement_spawn_npcs`: Populates a settlement with NPCs using the `AgentManager`.
// - Settlement features: `settlement_add_marketplace`, `settlement_add_wall`, `settlement_add_gate`
//   for adding specific infrastructure and defenses.
//
// Ownership: The `SettlementGenerator` owns the collection of `Settlement` instances it creates.
// Each `Settlement` in turn owns its `Building` and NPC arrays.
//
// Invariants:
// - A `SettlementGenerator` must be initialized before generating settlements.
// - `Vec3`, `Chunk`, and `AgentManager` (from their respective headers) are assumed to be correctly defined.
// - Settlement generation relies on the provided seed for reproducibility.
// - Coordinates (`position`, `size`) are typically in world-space or relative to the settlement's origin.
//
#ifndef SETTLEMENT_H
#define SETTLEMENT_H


#include "../game_common.h"
#include "../chunk/chunk.h"
#include <math/vec3.h>
#include "../npc/agent.h"

// Settlement types
typedef enum {
    SETTLEMENT_TYPE_VILLAGE,
    SETTLEMENT_TYPE_TOWN,
    SETTLEMENT_TYPE_CITY,
    SETTLEMENT_TYPE_OUTPOST,
    SETTLEMENT_TYPE_COUNT
} SettlementType;

// Building types
typedef enum {
    BUILDING_TYPE_HOUSE,
    BUILDING_TYPE_SHOP,
    BUILDING_TYPE_INN,
    BUILDING_TYPE_BLACKSMITH,
    BUILDING_TYPE_FARM,
    BUILDING_TYPE_WAREHOUSE,
    BUILDING_TYPE_TOWN_HALL,
    BUILDING_TYPE_CHURCH,
    BUILDING_TYPE_BARRACKS,
    BUILDING_TYPE_COUNT
} BuildingType;

// Building
typedef struct {
    BuildingType type;
    Vec3 position;
    Vec3 size;
    bool generated;
    u32 npc_capacity;
    EntityID *npcs;
    u32 npc_count;
} Building;

// Settlement
typedef struct {
    SettlementType type;
    Vec3 center;
    f32 radius;
    Building *buildings;
    u32 building_count;
    u32 building_capacity;
    EntityID *npcs;
    u32 npc_count;
    u32 npc_capacity;
    bool generated;
    u32 seed;
} Settlement;

// Settlement generator
typedef struct {
    u32 seed;
    Settlement *settlements;
    u32 count;
    u32 capacity;
} SettlementGenerator;

// Initialize settlement generator
void settlement_generator_init(SettlementGenerator *gen, u32 seed);
void settlement_generator_free(SettlementGenerator *gen);

// Generate settlement
Settlement *settlement_generate(SettlementGenerator *gen, SettlementType type, Vec3 position, u32 seed);

// Place settlement in world
void settlement_place_in_chunk(Settlement *settlement, Chunk *chunk);

// Building generation
void settlement_generate_building(Settlement *settlement, BuildingType type, Vec3 position, Chunk *chunk);
void settlement_generate_road(Settlement *settlement, Vec3 start, Vec3 end, Chunk *chunk);

// NPC spawning
void settlement_spawn_npcs(Settlement *settlement, AgentManager *agent_manager, ECSWorld *ecs);

// Settlement features
void settlement_add_marketplace(Settlement *settlement, Vec3 position, Chunk *chunk);
void settlement_add_wall(Settlement *settlement, Chunk *chunk);
void settlement_add_gate(Settlement *settlement, Vec3 position, Chunk *chunk);

#endif // SETTLEMENT_H

