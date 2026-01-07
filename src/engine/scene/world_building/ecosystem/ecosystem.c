#include "scene/world_building/ecosystem/ecosystem.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <include/math/math.h>
#include <stdio.h>

#ifndef ITEMID_DEFINED
typedef uint32_t ItemID;
#define ITEMID_DEFINED
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

static float random_float() {
    return (float)rand() / (float)RAND_MAX;
}

// Forward declarations
void creature_update_ai(WildlifeCreature *creature, EcosystemManager *mgr, float dt);
void pack_update_center(CreaturePack *pack, EcosystemManager *mgr);
void ecosystem_update_chunk(EcosystemChunk *chunk, EcosystemManager *mgr, float dt);

/* =================================================================================================
 *                                    WILDLIFE SPECIES
 * =================================================================================================
 */

static WildlifeSpecies *g_species_db = NULL;
static uint32_t g_species_count = 0;
static uint32_t g_species_capacity = 0;

void species_database_init(uint32_t capacity) {
    g_species_db = (WildlifeSpecies *)calloc(capacity, sizeof(WildlifeSpecies));
    g_species_capacity = capacity;
    g_species_count = 0;
}

void species_register(WildlifeSpecies species) {
    if (g_species_count >= g_species_capacity) return;
    g_species_db[g_species_count++] = species;
}

WildlifeSpecies *species_get(uint32_t id) {
    for (uint32_t i = 0; i < g_species_count; i++) {
        if (g_species_db[i].id == id) return &g_species_db[i];
    }
    return NULL;
}

WildlifeSpecies **species_find_by_biome(uint32_t biome_id, uint32_t *out_count) {
    if (!out_count) return NULL;
    uint32_t count = 0;
    for (uint32_t i = 0; i < g_species_count; i++) {
        for (uint32_t b = 0; b < g_species_db[i].biome_count; b++) {
            if (g_species_db[i].preferred_biomes[b] == biome_id) {
                count++;
                break;
            }
        }
    }
    
    if (count == 0) {
        *out_count = 0;
        return NULL;
    }
    
    WildlifeSpecies **results = (WildlifeSpecies **)malloc(count * sizeof(WildlifeSpecies *));
    uint32_t idx = 0;
    for (uint32_t i = 0; i < g_species_count; i++) {
        for (uint32_t b = 0; b < g_species_db[i].biome_count; b++) {
            if (g_species_db[i].preferred_biomes[b] == biome_id) {
                results[idx++] = &g_species_db[i];
                break;
            }
        }
    }
    *out_count = count;
    return results;
}

bool species_load_json(const char *path) {
    // Stub: Simulated JSON loading
    return true;
}

/* =================================================================================================
 *                                    CREATURE INSTANCES
 * =================================================================================================
 */

WildlifeCreature creature_spawn(uint32_t species_id, float pos[3]) {
    WildlifeCreature creature;
    memset(&creature, 0, sizeof(WildlifeCreature));
    creature.id = rand();
    creature.species_id = species_id;
    memcpy(creature.position, pos, sizeof(float) * 3);
    memcpy(creature.spawn_position, pos, sizeof(float) * 3);
    
    WildlifeSpecies *species = species_get(species_id);
    if (species) {
        creature.health = species->health;
        creature.hunger = 0.5f;
        creature.thirst = 0.5f;
        creature.energy = 1.0f;
        creature.wander_radius = 50.0f;
    }
    creature.activity = ACTIVITY_IDLE;
    return creature;
}

void creature_despawn(WildlifeCreature *creature) {
    if (creature) memset(creature, 0, sizeof(WildlifeCreature));
}

void creature_update_ai(WildlifeCreature *creature, EcosystemManager *mgr, float dt) {
    if (!creature || !mgr) return;
    
    creature->age += dt;
    creature->activity_timer -= dt;
    
    if (creature->hunger > 0.8f) {
        creature->activity = ACTIVITY_EATING;
    } else if (creature->thirst > 0.8f) {
        creature->activity = ACTIVITY_DRINKING;
    } else if (creature->energy < 0.3f) {
        creature->activity = ACTIVITY_SLEEPING;
        creature->energy += dt * 0.1f;
    } else {
        if (creature->activity_timer <= 0) {
            creature->activity = ACTIVITY_ROAMING;
            creature->activity_timer = 5.0f + random_float() * 10.0f;
        }
    }
    
    // Basic movement logic
    if (creature->activity == ACTIVITY_ROAMING) {
        creature->position[0] += (random_float() - 0.5f) * dt * 2.0f;
        creature->position[2] += (random_float() - 0.5f) * dt * 2.0f;
    } else if (creature->activity == ACTIVITY_FLEEING) {
        creature->position[0] += creature->velocity[0] * dt;
        creature->position[2] += creature->velocity[2] * dt;
        creature->velocity[0] *= 0.9f; 
        creature->velocity[2] *= 0.9f;
    }
}

void creature_update_needs(WildlifeCreature *creature, float dt) {
    if (!creature) return;
    WildlifeSpecies *species = species_get(creature->species_id);
    float h_rate = species ? species->hunger_rate : 0.01f;
    float t_rate = species ? species->thirst_rate : 0.01f;
    
    creature->hunger += h_rate * dt;
    creature->thirst += t_rate * dt;
    creature->energy -= dt * 0.005f;
    
    if (creature->hunger > 1.0f) creature->hunger = 1.0f;
    if (creature->thirst > 1.0f) creature->thirst = 1.0f;
    if (creature->energy < 0.0f) creature->energy = 0.0f;
}

bool creature_find_food(WildlifeCreature *creature, EcosystemManager *mgr) {
    // Search grid for interactables (stub)
    return true;
}

bool creature_find_water(WildlifeCreature *creature, EcosystemManager *mgr) {
    return true;
}

bool creature_find_shelter(WildlifeCreature *creature, EcosystemManager *mgr) {
    return true;
}

void creature_flee_from(WildlifeCreature *creature, float threat_pos[3]) {
    if (!creature) return;
    float dx = creature->position[0] - threat_pos[0];
    float dz = creature->position[2] - threat_pos[2];
    float dist = sqrtf(dx*dx + dz*dz);
    if (dist < 0.01f) dist = 0.01f;
    
    creature->velocity[0] = (dx / dist) * 5.0f;
    creature->velocity[2] = (dz / dist) * 5.0f;
    creature->activity = ACTIVITY_FLEEING;
    creature->activity_timer = 5.0f;
}

void creature_chase_target(WildlifeCreature *creature, uint32_t target_id) {
    creature->current_target = target_id;
    creature->activity = ACTIVITY_HUNTING;
}

void creature_attack(WildlifeCreature *creature, WildlifeCreature *target) {
    if (!creature || !target) return;
    target->health -= 15.0f;
    if (target->health < 0) target->health = 0;
}

void creature_die(WildlifeCreature *creature) {
    if (creature) {
        creature->health = 0;
        // Mark for deletion or spawn loot
    }
}

WildlifeCreature *creature_reproduce(WildlifeCreature *parent1, WildlifeCreature *parent2) {
    // Return null, handled by manager spawn usually
    return NULL; 
}

void creature_age(WildlifeCreature *creature, float dt) {
    if (creature) creature->age += dt;
}

bool creature_path_to(WildlifeCreature *creature, float dest[3]) {
    // Set velocity towards dest
    return true;
}

void creature_animation_update(WildlifeCreature *creature, float dt) {
    // Animation state machine update
}

/* =================================================================================================
 *                                    PACK/HERD SYSTEM
 * =================================================================================================
 */

CreaturePack pack_create(uint32_t species_id, uint32_t leader_id) {
    CreaturePack pack;
    memset(&pack, 0, sizeof(CreaturePack));
    pack.id = rand();
    pack.species_id = species_id;
    pack.leader_id = leader_id;
    pack.territory_radius = 100.0f;
    return pack;
}

void pack_add_member(CreaturePack *pack, uint32_t creature_id) {
    if (!pack || pack->member_count >= 16) return;
    pack->members[pack->member_count++] = creature_id;
}

void pack_remove_member(CreaturePack *pack, uint32_t creature_id) {
    if (!pack) return;
    for (uint32_t i = 0; i < pack->member_count; i++) {
        if (pack->members[i] == creature_id) {
            for (uint32_t j = i; j < pack->member_count - 1; j++) {
                pack->members[j] = pack->members[j + 1];
            }
            pack->member_count--;
            return;
        }
    }
}

void pack_elect_leader(CreaturePack *pack, EcosystemManager *mgr) {
    if (!pack || pack->member_count == 0) return;
    pack->leader_id = pack->members[0];
}

void pack_update_center(CreaturePack *pack, EcosystemManager *mgr) {
    if (!pack || !mgr || pack->member_count == 0) return;
    // Calculation stub
}

void pack_coordinated_hunt(CreaturePack *pack, uint32_t prey_id, EcosystemManager *mgr) {
    // Group AI signal
}

void pack_defend_territory(CreaturePack *pack, EcosystemManager *mgr) {}
void herd_flock_behavior(CreaturePack *herd, EcosystemManager *mgr) {}
void herd_flee_together(CreaturePack *herd, float threat_pos[3], EcosystemManager *mgr) {}

/* =================================================================================================
 *                                    VEGETATION SYSTEM
 * =================================================================================================
 */

static VegetationType *g_vegetation_types = NULL;
static uint32_t g_vegetation_type_count = 0;
static uint32_t g_vegetation_type_capacity = 0;

void vegetation_type_register(VegetationType type) {
    if (!g_vegetation_types) {
        g_vegetation_type_capacity = 64;
        g_vegetation_types = (VegetationType *)calloc(g_vegetation_type_capacity, sizeof(VegetationType));
    }
    if (g_vegetation_type_count >= g_vegetation_type_capacity) return;
    g_vegetation_types[g_vegetation_type_count++] = type;
}

VegetationInstance vegetation_spawn(uint32_t type_id, float pos[3]) {
    VegetationInstance veg;
    memset(&veg, 0, sizeof(VegetationInstance));
    veg.id = rand();
    veg.type_id = type_id;
    memcpy(veg.position, pos, sizeof(float) * 3);
    veg.scale = 0.8f + random_float() * 0.4f;
    veg.rotation = random_float() * 6.28f;
    veg.health = 100.0f;
    return veg;
}

bool vegetation_harvest(VegetationInstance *veg) {
    if (!veg || veg->is_harvested) return false;
    veg->is_harvested = true;
    veg->regrow_timer = 60.0f;
    return true;
}

void vegetation_grow(VegetationInstance *veg, float dt) {
    if (!veg) return;
    if (veg->is_harvested) {
        veg->regrow_timer -= dt;
        if (veg->regrow_timer <= 0) {
            veg->is_harvested = false;
            veg->health = 100.0f;
        }
    } else {
        veg->age += dt;
    }
}

void vegetation_spread(VegetationInstance *veg, EcosystemChunk *chunk) {}
void vegetation_die(VegetationInstance *veg) { if (veg) veg->health = 0; }
void vegetation_seasonal_change(VegetationInstance *veg, uint32_t season) {}
void vegetation_fire_spread(VegetationInstance *veg, EcosystemChunk *chunk) {}
float vegetation_density_map(EcosystemChunk *chunk, float pos[2]) { return 0.5f; }

/* =================================================================================================
 *                                    ECOSYSTEM MANAGER
 * =================================================================================================
 */

void ecosystem_manager_init(EcosystemManager *mgr) {
    if (!mgr) return;
    memset(mgr, 0, sizeof(EcosystemManager));
    species_database_init(128);
    mgr->simulation_speed = 1.0f;
    mgr->spawn_rate = 1.0f;
    mgr->max_creatures_per_chunk = 20.0f;
}

void ecosystem_manager_shutdown(EcosystemManager *mgr) {
    if (!mgr) return;
    if (mgr->chunks) {
        for (uint32_t i = 0; i < mgr->chunk_count; i++) {
            if (mgr->chunks[i].creatures) free(mgr->chunks[i].creatures);
            if (mgr->chunks[i].vegetation) free(mgr->chunks[i].vegetation);
        }
        free(mgr->chunks);
    }
    if (mgr->packs) free(mgr->packs);
    if (g_species_db) free(g_species_db);
    if (g_vegetation_types) free(g_vegetation_types);
}

void ecosystem_manager_update(EcosystemManager *mgr, float dt) {
    if (!mgr) return;
    dt *= mgr->simulation_speed;
    
    for (uint32_t i = 0; i < mgr->chunk_count; i++) {
        ecosystem_update_chunk(&mgr->chunks[i], mgr, dt);
    }
    
    for (uint32_t i = 0; i < mgr->pack_count; i++) {
        pack_update_center(&mgr->packs[i], mgr);
    }
}

void ecosystem_update_chunk(EcosystemChunk *chunk, EcosystemManager *mgr, float dt) {
    if (!chunk || !mgr) return;
    
    for (uint32_t i = 0; i < chunk->creature_count; i++) {
        creature_update_needs(&chunk->creatures[i], dt);
        creature_update_ai(&chunk->creatures[i], mgr, dt);
    }
    
    for (uint32_t i = 0; i < chunk->vegetation_count; i++) {
        vegetation_grow(&chunk->vegetation[i], dt);
    }
}

void ecosystem_spawn_initial(EcosystemManager *mgr, EcosystemChunk *chunk) {
    if (!mgr || !chunk) return;
    uint32_t spawn_count = (uint32_t)(mgr->spawn_rate * 5);
    // Logic for initial population
    // ...
}

void ecosystem_balance_populations(EcosystemManager *mgr) {}
void ecosystem_handle_extinction(EcosystemManager *mgr, uint32_t species_id) {}
void ecosystem_seasonal_migration(EcosystemManager *mgr) {}
void ecosystem_day_night_behavior(EcosystemManager *mgr, float time_of_day) {}
void ecosystem_weather_effects(EcosystemManager *mgr, int weather_type) {}
bool ecosystem_serialize(EcosystemManager *mgr, void *buffer, size_t size) { return false; }
bool ecosystem_deserialize(EcosystemManager *mgr, const void *buffer, size_t size) { return false; }
void ecosystem_statistics(EcosystemManager *mgr) {}
