/**
 * =================================================================================================
 *                              ECOSYSTEM & POPULATION SYSTEM
 *                                  Agent: AGENT_WORLD_2
 * =================================================================================================
 *
 * Dynamic ecosystems with wildlife, vegetation, and environmental simulation.
 *
 * =================================================================================================
 */

#ifndef ECOSYSTEM_H
#define ECOSYSTEM_H

#include <stdbool.h>
#include <stdint.h>

// Mock ItemID if not defined elsewhere
#ifndef ITEMID_DEFINED
typedef uint32_t ItemID;
#define ITEMID_DEFINED
#endif

/* =================================================================================================
 *                                    WILDLIFE SPECIES
 * =================================================================================================
 */

typedef enum CreatureBehavior {
  BEHAVIOR_PASSIVE,
  BEHAVIOR_NEUTRAL,
  BEHAVIOR_AGGRESSIVE,
  BEHAVIOR_TERRITORIAL,
  BEHAVIOR_PREDATOR,
  BEHAVIOR_PREY,
  BEHAVIOR_PACK,
  BEHAVIOR_HERD,
} CreatureBehavior;

typedef enum CreatureActivity {
  ACTIVITY_IDLE,
  ACTIVITY_ROAMING,
  ACTIVITY_EATING,
  ACTIVITY_DRINKING,
  ACTIVITY_SLEEPING,
  ACTIVITY_HUNTING,
  ACTIVITY_FLEEING,
  ACTIVITY_MATING,
  ACTIVITY_NESTING,
} CreatureActivity;

typedef struct WildlifeSpecies {
  uint32_t id;
  char name[32];
  char model_path[128];

  // Behavior
  CreatureBehavior behavior;
  float aggression;
  float fear_radius;
  float chase_radius;
  float flee_speed;
  float walk_speed;
  float run_speed;

  // Habitat
  uint32_t preferred_biomes[8];
  uint32_t biome_count;
  float altitude_min;
  float altitude_max;
  bool is_nocturnal;
  bool is_aquatic;
  bool can_fly;

  // Ecology
  uint32_t prey_species[4];
  uint32_t prey_count;
  uint32_t predator_species[4];
  uint32_t predator_count;

  // Stats
  float health;
  float hunger_rate;
  float thirst_rate;
  float stamina;
  float lifespan;
  float reproduction_rate;

  // Drops
  ItemID loot_table_id;
} WildlifeSpecies;

/* Species Database Functions */
bool species_database_init(uint32_t initial_capacity);
bool species_register(const WildlifeSpecies *species);
const WildlifeSpecies* species_get(uint32_t species_id);
uint32_t species_find_by_biome(uint32_t biome_id, const WildlifeSpecies** results, uint32_t max_results);
bool species_load_json(const char* json_data);

/* =================================================================================================
 *                                    CREATURE INSTANCES
 * =================================================================================================
 */

typedef struct WildlifeCreature {
  uint32_t id;
  uint32_t species_id;

  // Position
  float position[3];
  float rotation;
  float velocity[3];

  // State
  CreatureActivity activity;
  float activity_timer;
  float hunger;
  float thirst;
  float energy;
  float health;
  float age;

  // AI
  uint32_t current_target;
  float path_points[16][3];
  uint32_t path_point_count;
  uint32_t current_path_index;

  // Pack/Herd
  uint32_t pack_id;
  bool is_leader;

  // Spawning
  uint32_t home_chunk;
  float spawn_position[3];
  float wander_radius;
} WildlifeCreature;

// TODO(AGENT_WORLD_2): Implement creature_spawn [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement creature_despawn [Difficulty: 3]
// TODO(AGENT_WORLD_2): Implement creature_update_ai [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement creature_update_needs [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement creature_find_food [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement creature_find_water [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement creature_find_shelter [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement creature_flee_from [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement creature_chase_target [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement creature_attack [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement creature_die [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement creature_reproduce [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement creature_age [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement creature_path_to [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement creature_animation_update [Difficulty: 5]

/* =================================================================================================
 *                                    PACK/HERD SYSTEM
 * =================================================================================================
 */

typedef struct CreaturePack {
  uint32_t id;
  uint32_t species_id;
  uint32_t members[16];
  uint32_t member_count;
  uint32_t leader_id;

  float center[3];
  float territory_center[3];
  float territory_radius;

  bool is_hunting;
  uint32_t hunt_target;
} CreaturePack;

// TODO(AGENT_WORLD_2): Implement pack_create [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement pack_add_member [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement pack_remove_member [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement pack_elect_leader [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement pack_update_center [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement pack_coordinated_hunt [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement pack_defend_territory [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement herd_flock_behavior [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement herd_flee_together [Difficulty: 6]

/* =================================================================================================
 *                                    VEGETATION SYSTEM
 * =================================================================================================
 */

typedef struct VegetationType {
  uint32_t id;
  char name[32];
  char model_path[128];

  uint32_t preferred_biomes[8];
  uint32_t biome_count;
  float altitude_min;
  float altitude_max;
  float slope_max;
  float moisture_min;

  float growth_rate;
  float max_age;
  float spread_chance;
  float spread_radius;

  bool is_harvestable;
  ItemID harvest_item;
  float harvest_amount;
  float regrow_time;

  float density;
  float scale_min;
  float scale_max;
} VegetationType;

typedef struct VegetationInstance {
  uint32_t id;
  uint32_t type_id;
  float position[3];
  float rotation;
  float scale;
  float age;
  float health;
  bool is_harvested;
  float regrow_timer;
} VegetationInstance;

// TODO(AGENT_WORLD_2): Implement vegetation_type_register [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement vegetation_spawn [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement vegetation_harvest [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement vegetation_grow [Difficulty: 4]
// TODO(AGENT_WORLD_2): Implement vegetation_spread [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement vegetation_die [Difficulty: 3]
// TODO(AGENT_WORLD_2): Implement vegetation_seasonal_change [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement vegetation_fire_spread [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement vegetation_density_map [Difficulty: 6]

/* =================================================================================================
 *                                    ECOSYSTEM MANAGER
 * =================================================================================================
 */

typedef struct EcosystemChunk {
  int32_t chunk_x;
  int32_t chunk_z;

  WildlifeCreature *creatures;
  uint32_t creature_count;
  uint32_t creature_capacity;

  VegetationInstance *vegetation;
  uint32_t vegetation_count;
  uint32_t vegetation_capacity;

  // Environmental factors
  float temperature;
  float moisture;
  float pollution;
  float population_pressure;
} EcosystemChunk;

typedef struct EcosystemManager {
  WildlifeSpecies *species;
  uint32_t species_count;

  VegetationType *vegetation_types;
  uint32_t vegetation_type_count;

  EcosystemChunk *chunks;
  uint32_t chunk_count;

  CreaturePack *packs;
  uint32_t pack_count;

  // Simulation settings
  float simulation_speed;
  float spawn_rate;
  float max_creatures_per_chunk;
  float max_vegetation_per_chunk;
  bool simulate_reproduction;
  bool simulate_death;
  bool simulate_predation;

  // Time
  float day_night_cycle;
  float season;
} EcosystemManager;

// TODO(AGENT_WORLD_2): Implement ecosystem_manager_init [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement ecosystem_manager_shutdown [Difficulty: 3]
// TODO(AGENT_WORLD_2): Implement ecosystem_manager_update [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement ecosystem_update_chunk [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement ecosystem_spawn_initial [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement ecosystem_balance_populations [Difficulty: 7]
// TODO(AGENT_WORLD_2): Implement ecosystem_handle_extinction [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement ecosystem_seasonal_migration [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement ecosystem_day_night_behavior [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement ecosystem_weather_effects [Difficulty: 5]
// TODO(AGENT_WORLD_2): Implement ecosystem_serialize [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement ecosystem_deserialize [Difficulty: 6]
// TODO(AGENT_WORLD_2): Implement ecosystem_statistics [Difficulty: 4]

#endif // ECOSYSTEM_H
