/**
 * =================================================================================================
 *                              ECOSYSTEM & POPULATION SYSTEM
 *                                  Agent: AGENT_WORLD_2
 * =================================================================================================
 *
 * Implementation of dynamic ecosystems with wildlife, vegetation, and environmental simulation.
 *
 * =================================================================================================
 */

#include "ecosystem.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* =================================================================================================
 *                                    VEGETATION SYSTEM
 * =================================================================================================
 */

static VegetationType *vegetation_type_registry = NULL;
static uint32_t vegetation_type_capacity = 0;
static uint32_t vegetation_type_count = 0;
static uint32_t next_vegetation_id = 1;

bool vegetation_type_register(const VegetationType *type) {
    if (!type || vegetation_type_count >= vegetation_type_capacity) {
        // Expand registry if needed
        uint32_t new_capacity = vegetation_type_capacity == 0 ? 64 : vegetation_type_capacity * 2;
        VegetationType *new_registry = realloc(vegetation_type_registry, 
                                              new_capacity * sizeof(VegetationType));
        if (!new_registry) return false;
        
        vegetation_type_registry = new_registry;
        vegetation_type_capacity = new_capacity;
    }
    
    // Copy and assign ID
    VegetationType new_type = *type;
    new_type.id = next_vegetation_id++;
    vegetation_type_registry[vegetation_type_count++] = new_type;
    
    return true;
}

bool vegetation_spawn(VegetationInstance *instance, uint32_t type_id, 
                     const float position[3], float scale) {
    if (!instance || !position) return false;
    
    // Find vegetation type
    VegetationType *type = NULL;
    for (uint32_t i = 0; i < vegetation_type_count; i++) {
        if (vegetation_type_registry[i].id == type_id) {
            type = &vegetation_type_registry[i];
            break;
        }
    }
    if (!type) return false;
    
    // Initialize instance
    instance->id = 0; // Will be assigned by manager
    instance->type_id = type_id;
    instance->position[0] = position[0];
    instance->position[1] = position[1];
    instance->position[2] = position[2];
    instance->rotation = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
    instance->scale = scale;
    instance->age = 0.0f;
    instance->health = 100.0f;
    instance->is_harvested = false;
    instance->regrow_timer = 0.0f;
    
    return true;
}

bool vegetation_harvest(VegetationInstance *instance, ItemID *harvested_item, 
                       float *harvest_amount) {
    if (!instance || !instance->is_harvestable || instance->is_harvested) {
        return false;
    }
    
    // Find vegetation type
    VegetationType *type = NULL;
    for (uint32_t i = 0; i < vegetation_type_count; i++) {
        if (vegetation_type_registry[i].id == instance->type_id) {
            type = &vegetation_type_registry[i];
            break;
        }
    }
    if (!type) return false;
    
    // Harvest
    instance->is_harvested = true;
    instance->regrow_timer = type->regrow_time;
    
    if (harvested_item) *harvested_item = type->harvest_item;
    if (harvest_amount) *harvest_amount = type->harvest_amount;
    
    return true;
}

bool vegetation_grow(VegetationInstance *instance, float delta_time) {
    if (!instance || delta_time <= 0.0f) return false;
    
    // Find vegetation type
    VegetationType *type = NULL;
    for (uint32_t i = 0; i < vegetation_type_count; i++) {
        if (vegetation_type_registry[i].id == instance->type_id) {
            type = &vegetation_type_registry[i];
            break;
        }
    }
    if (!type) return false;
    
    // Age the vegetation
    instance->age += delta_time;
    
    // Handle regrowth
    if (instance->is_harvested && instance->regrow_timer > 0.0f) {
        instance->regrow_timer -= delta_time;
        if (instance->regrow_timer <= 0.0f) {
            instance->is_harvested = false;
            instance->health = 100.0f;
        }
        return true;
    }
    
    // Natural growth
    if (instance->age < type->max_age) {
        float growth_factor = type->growth_rate * delta_time;
        instance->scale = fminf(instance->scale * (1.0f + growth_factor * 0.01f), 
                                type->scale_max);
        instance->health = fminf(instance->health + growth_factor, 100.0f);
    }
    
    return true;
}

bool vegetation_spread(EcosystemManager *manager, const VegetationInstance *parent, 
                      float delta_time) {
    if (!manager || !parent || delta_time <= 0.0f) return false;
    
    // Find vegetation type
    VegetationType *type = NULL;
    for (uint32_t i = 0; i < vegetation_type_count; i++) {
        if (vegetation_type_registry[i].id == parent->type_id) {
            type = &vegetation_type_registry[i];
            break;
        }
    }
    if (!type) return false;
    
    // Check spread chance
    if (((float)rand() / RAND_MAX) > type->spread_chance * delta_time) {
        return false;
    }
    
    // Find spawn position within spread radius
    float angle = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
    float distance = ((float)rand() / RAND_MAX) * type->spread_radius;
    
    float new_position[3];
    new_position[0] = parent->position[0] + cosf(angle) * distance;
    new_position[1] = parent->position[1];
    new_position[2] = parent->position[2] + sinf(angle) * distance;
    
    // Find appropriate chunk
    int32_t chunk_x = (int32_t)(new_position[0] / 16.0f);
    int32_t chunk_z = (int32_t)(new_position[2] / 16.0f);
    
    EcosystemChunk *chunk = NULL;
    for (uint32_t i = 0; i < manager->chunk_count; i++) {
        if (manager->chunks[i].chunk_x == chunk_x && 
            manager->chunks[i].chunk_z == chunk_z) {
            chunk = &manager->chunks[i];
            break;
        }
    }
    
    if (!chunk || chunk->vegetation_count >= chunk->vegetation_capacity) {
        return false;
    }
    
    // Create new vegetation instance
    float scale = type->scale_min + ((float)rand() / RAND_MAX) * 
                  (type->scale_max - type->scale_min);
    
    VegetationInstance *new_instance = &chunk->vegetation[chunk->vegetation_count++];
    if (!vegetation_spawn(new_instance, parent->type_id, new_position, scale)) {
        chunk->vegetation_count--;
        return false;
    }
    
    return true;
}

bool vegetation_die(VegetationInstance *instance) {
    if (!instance) return false;
    
    instance->health = 0.0f;
    instance->age = -1.0f; // Mark as dead
    
    return true;
}

bool vegetation_seasonal_change(VegetationInstance *instance, float season) {
    if (!instance || season < 0.0f || season > 1.0f) return false;
    
    // Find vegetation type
    VegetationType *type = NULL;
    for (uint32_t i = 0; i < vegetation_type_count; i++) {
        if (vegetation_type_registry[i].id == instance->type_id) {
            type = &vegetation_type_registry[i];
            break;
        }
    }
    if (!type) return false;
    
    // Seasonal effects (0=Spring, 0.25=Summer, 0.5=Autumn, 0.75=Winter)
    float seasonal_modifier = 1.0f;
    
    if (season < 0.25f) {
        // Spring - growth boost
        seasonal_modifier = 1.2f;
    } else if (season < 0.5f) {
        // Summer - normal growth
        seasonal_modifier = 1.0f;
    } else if (season < 0.75f) {
        // Autumn - reduced growth
        seasonal_modifier = 0.8f;
    } else {
        // Winter - minimal growth
        seasonal_modifier = 0.3f;
    }
    
    // Apply seasonal effects
    instance->health *= seasonal_modifier;
    instance->health = fmaxf(instance->health, 10.0f); // Minimum health
    
    return true;
}

bool vegetation_fire_spread(EcosystemManager *manager, float fire_pos[3], 
                          float fire_radius, float delta_time) {
    if (!manager || !fire_pos || fire_radius <= 0.0f || delta_time <= 0.0f) {
        return false;
    }
    
    uint32_t burned_count = 0;
    
    // Check all chunks in range
    for (uint32_t chunk_idx = 0; chunk_idx < manager->chunk_count; chunk_idx++) {
        EcosystemChunk *chunk = &manager->chunks[chunk_idx];
        
        // Check if chunk is in range
        float chunk_center_x = chunk->chunk_x * 16.0f + 8.0f;
        float chunk_center_z = chunk->chunk_z * 16.0f + 8.0f;
        float chunk_dist = sqrtf(powf(chunk_center_x - fire_pos[0], 2) + 
                                powf(chunk_center_z - fire_pos[2], 2));
        
        if (chunk_dist > fire_radius + 16.0f) continue; // Skip if too far
        
        // Check vegetation in this chunk
        for (uint32_t veg_idx = 0; veg_idx < chunk->vegetation_count; veg_idx++) {
            VegetationInstance *veg = &chunk->vegetation[veg_idx];
            
            float veg_dist = sqrtf(powf(veg->position[0] - fire_pos[0], 2) + 
                                  powf(veg->position[2] - fire_pos[2], 2));
            
            if (veg_dist <= fire_radius) {
                // Burn vegetation
                float burn_chance = 1.0f - (veg_dist / fire_radius);
                if (((float)rand() / RAND_MAX) < burn_chance * delta_time * 2.0f) {
                    vegetation_die(veg);
                    burned_count++;
                }
            }
        }
    }
    
    return burned_count > 0;
}

bool vegetation_density_map(EcosystemManager *manager, float center[3], 
                           float radius, float *density_map, 
                           uint32_t map_width, uint32_t map_height) {
    if (!manager || !center || !density_map || radius <= 0.0f || 
        map_width == 0 || map_height == 0) {
        return false;
    }
    
    // Clear density map
    memset(density_map, 0, map_width * map_height * sizeof(float));
    
    // Calculate vegetation density
    float step_x = (radius * 2.0f) / map_width;
    float step_z = (radius * 2.0f) / map_height;
    
    for (uint32_t chunk_idx = 0; chunk_idx < manager->chunk_count; chunk_idx++) {
        EcosystemChunk *chunk = &manager->chunks[chunk_idx];
        
        for (uint32_t veg_idx = 0; veg_idx < chunk->vegetation_count; veg_idx++) {
            VegetationInstance *veg = &chunk->vegetation[veg_idx];
            
            // Check if vegetation is in range
            float dx = veg->position[0] - center[0];
            float dz = veg->position[2] - center[2];
            float dist = sqrtf(dx * dx + dz * dz);
            
            if (dist <= radius) {
                // Map to density grid
                uint32_t map_x = (uint32_t)((dx + radius) / step_x);
                uint32_t map_z = (uint32_t)((dz + radius) / step_z);
                
                if (map_x < map_width && map_z < map_height) {
                    uint32_t index = map_z * map_width + map_x;
                    density_map[index] += 1.0f;
                }
            }
        }
    }
    
    return true;
}

/* =================================================================================================
 *                                    ECOSYSTEM MANAGER
 * =================================================================================================
 */

bool ecosystem_manager_init(EcosystemManager *manager, uint32_t max_chunks) {
    if (!manager || max_chunks == 0) return false;
    
    memset(manager, 0, sizeof(EcosystemManager));
    
    // Allocate chunks
    manager->chunks = calloc(max_chunks, sizeof(EcosystemChunk));
    if (!manager->chunks) return false;
    
    manager->chunk_count = max_chunks;
    
    // Initialize simulation settings
    manager->simulation_speed = 1.0f;
    manager->spawn_rate = 0.1f;
    manager->max_creatures_per_chunk = 50.0f;
    manager->max_vegetation_per_chunk = 200.0f;
    manager->simulate_reproduction = true;
    manager->simulate_death = true;
    manager->simulate_predation = true;
    
    // Initialize time
    manager->day_night_cycle = 0.0f; // Start at dawn
    manager->season = 0.0f; // Start at spring
    
    return true;
}

void ecosystem_manager_shutdown(EcosystemManager *manager) {
    if (!manager) return;
    
    // Free chunks
    if (manager->chunks) {
        for (uint32_t i = 0; i < manager->chunk_count; i++) {
            EcosystemChunk *chunk = &manager->chunks[i];
            free(chunk->creatures);
            free(chunk->vegetation);
        }
        free(manager->chunks);
    }
    
    // Free other resources
    free(manager->species);
    free(manager->vegetation_types);
    free(manager->packs);
    
    memset(manager, 0, sizeof(EcosystemManager));
}

bool ecosystem_manager_update(EcosystemManager *manager, float delta_time) {
    if (!manager || delta_time <= 0.0f) return false;
    
    // Update time
    float time_delta = delta_time * manager->simulation_speed;
    manager->day_night_cycle += time_delta * 0.01f; // 1 day = 100 time units
    if (manager->day_night_cycle >= 1.0f) {
        manager->day_night_cycle -= 1.0f;
        manager->season += 0.00274f; // 1 season = 365 days
        if (manager->season >= 1.0f) {
            manager->season -= 1.0f;
        }
    }
    
    // Update all chunks
    for (uint32_t i = 0; i < manager->chunk_count; i++) {
        ecosystem_update_chunk(manager, &manager->chunks[i], time_delta);
    }
    
    // Balance populations periodically
    static float balance_timer = 0.0f;
    balance_timer += time_delta;
    if (balance_timer >= 10.0f) { // Balance every 10 time units
        ecosystem_balance_populations(manager);
        balance_timer = 0.0f;
    }
    
    return true;
}

bool ecosystem_update_chunk(EcosystemManager *manager, EcosystemChunk *chunk, 
                          float delta_time) {
    if (!manager || !chunk || delta_time <= 0.0f) return false;
    
    // Update vegetation
    for (uint32_t i = 0; i < chunk->vegetation_count; i++) {
        VegetationInstance *veg = &chunk->vegetation[i];
        
        vegetation_grow(veg, delta_time);
        vegetation_seasonal_change(veg, manager->season);
        
        // Vegetation spreading
        if (veg->age > 5.0f && !veg->is_harvested) {
            vegetation_spread(manager, veg, delta_time);
        }
        
        // Death from old age or poor health
        if (veg->health <= 0.0f || veg->age > veg->age + 50.0f) {
            vegetation_die(veg);
        }
    }
    
    // Update creatures
    for (uint32_t i = 0; i < chunk->creature_count; i++) {
        WildlifeCreature *creature = &chunk->creatures[i];
        
        // Update creature needs and AI
        // TODO: Implement creature_update_ai and creature_update_needs
        
        creature->age += delta_time;
    }
    
    return true;
}

bool ecosystem_spawn_initial(EcosystemManager *manager) {
    if (!manager) return false;
    
    // Spawn initial vegetation in all chunks
    for (uint32_t chunk_idx = 0; chunk_idx < manager->chunk_count; chunk_idx++) {
        EcosystemChunk *chunk = &manager->chunks[chunk_idx];
        
        // Allocate vegetation array
        if (!chunk->vegetation) {
            chunk->vegetation_capacity = (uint32_t)manager->max_vegetation_per_chunk;
            chunk->vegetation = calloc(chunk->vegetation_capacity, sizeof(VegetationInstance));
            if (!chunk->vegetation) continue;
        }
        
        // Spawn vegetation based on biome and density
        for (uint32_t type_idx = 0; type_idx < manager->vegetation_type_count; type_idx++) {
            VegetationType *type = &manager->vegetation_types[type_idx];
            
            // Calculate spawn count based on density
            uint32_t spawn_count = (uint32_t)(type->density * manager->max_vegetation_per_chunk);
            
            for (uint32_t i = 0; i < spawn_count && chunk->vegetation_count < chunk->vegetation_capacity; i++) {
                float position[3];
                position[0] = chunk->chunk_x * 16.0f + ((float)rand() / RAND_MAX) * 16.0f;
                position[1] = 0.0f; // Will be set by terrain height
                position[2] = chunk->chunk_z * 16.0f + ((float)rand() / RAND_MAX) * 16.0f;
                
                float scale = type->scale_min + ((float)rand() / RAND_MAX) * 
                             (type->scale_max - type->scale_min);
                
                VegetationInstance *instance = &chunk->vegetation[chunk->vegetation_count];
                if (vegetation_spawn(instance, type->id, position, scale)) {
                    chunk->vegetation_count++;
                }
            }
        }
    }
    
    return true;
}

bool ecosystem_balance_populations(EcosystemManager *manager) {
    if (!manager) return false;
    
    // Calculate total populations
    uint32_t total_creatures = 0;
    uint32_t total_vegetation = 0;
    
    for (uint32_t i = 0; i < manager->chunk_count; i++) {
        EcosystemChunk *chunk = &manager->chunks[i];
        total_creatures += chunk->creature_count;
        total_vegetation += chunk->vegetation_count;
    }
    
    // Adjust spawn rates based on population pressure
    float creature_pressure = (float)total_creatures / (manager->chunk_count * manager->max_creatures_per_chunk);
    float vegetation_pressure = (float)total_vegetation / (manager->chunk_count * manager->max_vegetation_per_chunk);
    
    // Update population pressure in chunks
    for (uint32_t i = 0; i < manager->chunk_count; i++) {
        EcosystemChunk *chunk = &manager->chunks[i];
        chunk->population_pressure = (creature_pressure + vegetation_pressure) * 0.5f;
    }
    
    // Handle extinction events
    ecosystem_handle_extinction(manager);
    
    return true;
}

bool ecosystem_handle_extinction(EcosystemManager *manager) {
    if (!manager) return false;
    
    // Check for extinct species
    for (uint32_t species_idx = 0; species_idx < manager->species_count; species_idx++) {
        WildlifeSpecies *species = &manager->species[species_idx];
        
        uint32_t population = 0;
        for (uint32_t chunk_idx = 0; chunk_idx < manager->chunk_count; chunk_idx++) {
            EcosystemChunk *chunk = &manager->chunks[chunk_idx];
            for (uint32_t creature_idx = 0; creature_idx < chunk->creature_count; creature_idx++) {
                if (chunk->creatures[creature_idx].species_id == species->id) {
                    population++;
                }
            }
        }
        
        // If species is extinct, attempt reintroduction
        if (population == 0) {
            // TODO: Implement reintroduction logic
            printf("Species %s has gone extinct!\n", species->name);
        }
    }
    
    return true;
}

bool ecosystem_seasonal_migration(EcosystemManager *manager) {
    if (!manager) return false;
    
    // Calculate season-based migration patterns
    float season = manager->season;
    
    // Migrate creatures based on seasonal preferences
    for (uint32_t chunk_idx = 0; chunk_idx < manager->chunk_count; chunk_idx++) {
        EcosystemChunk *chunk = &manager->chunks[chunk_idx];
        
        for (uint32_t creature_idx = 0; creature_idx < chunk->creature_count; creature_idx++) {
            WildlifeCreature *creature = &chunk->creatures[creature_idx];
            
            // Find species
            WildlifeSpecies *species = NULL;
            for (uint32_t species_idx = 0; species_idx < manager->species_count; species_idx++) {
                if (manager->species[species_idx].id == creature->species_id) {
                    species = &manager->species[species_idx];
                    break;
                }
            }
            
            if (species && species->is_nocturnal && manager->day_night_cycle < 0.3f) {
                // Nocturnal creatures seek shelter during day
                // TODO: Implement shelter seeking behavior
            }
        }
    }
    
    return true;
}

bool ecosystem_serialize(EcosystemManager *manager, void *buffer, uint32_t buffer_size, 
                       uint32_t *bytes_written) {
    if (!manager || !buffer || buffer_size == 0 || !bytes_written) return false;
    
    uint8_t *ptr = (uint8_t*)buffer;
    uint32_t remaining = buffer_size;
    
    // Write header
    if (remaining < sizeof(uint32_t) * 3) return false;
    *(uint32_t*)ptr = 0xEC57; // Magic number
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = 1; // Version
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = manager->chunk_count;
    ptr += sizeof(uint32_t);
    remaining -= sizeof(uint32_t) * 3;
    
    // Write chunks
    for (uint32_t i = 0; i < manager->chunk_count; i++) {
        EcosystemChunk *chunk = &manager->chunks[i];
        
        uint32_t chunk_size = sizeof(EcosystemChunk) + 
                              chunk->creature_count * sizeof(WildlifeCreature) +
                              chunk->vegetation_count * sizeof(VegetationInstance);
        
        if (remaining < chunk_size) return false;
        
        memcpy(ptr, chunk, sizeof(EcosystemChunk));
        ptr += sizeof(EcosystemChunk);
        remaining -= sizeof(EcosystemChunk);
        
        if (chunk->creature_count > 0) {
            memcpy(ptr, chunk->creatures, chunk->creature_count * sizeof(WildlifeCreature));
            ptr += chunk->creature_count * sizeof(WildlifeCreature);
            remaining -= chunk->creature_count * sizeof(WildlifeCreature);
        }
        
        if (chunk->vegetation_count > 0) {
            memcpy(ptr, chunk->vegetation, chunk->vegetation_count * sizeof(VegetationInstance));
            ptr += chunk->vegetation_count * sizeof(VegetationInstance);
            remaining -= chunk->vegetation_count * sizeof(VegetationInstance);
        }
    }
    
    *bytes_written = buffer_size - remaining;
    return true;
}

bool ecosystem_deserialize(EcosystemManager *manager, const void *buffer, 
                          uint32_t buffer_size, uint32_t *bytes_read) {
    if (!manager || !buffer || buffer_size == 0 || !bytes_read) return false;
    
    const uint8_t *ptr = (const uint8_t*)buffer;
    uint32_t remaining = buffer_size;
    
    // Read header
    if (remaining < sizeof(uint32_t) * 3) return false;
    
    uint32_t magic = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    uint32_t version = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    uint32_t chunk_count = *(const uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    remaining -= sizeof(uint32_t) * 3;
    
    if (magic != 0xEC57 || version != 1) return false;
    
    // Initialize manager if needed
    if (!manager->chunks || manager->chunk_count < chunk_count) {
        ecosystem_manager_shutdown(manager);
        if (!ecosystem_manager_init(manager, chunk_count)) return false;
    }
    
    // Read chunks
    for (uint32_t i = 0; i < chunk_count && i < manager->chunk_count; i++) {
        EcosystemChunk *chunk = &manager->chunks[i];
        
        if (remaining < sizeof(EcosystemChunk)) return false;
        
        memcpy(chunk, ptr, sizeof(EcosystemChunk));
        ptr += sizeof(EcosystemChunk);
        remaining -= sizeof(EcosystemChunk);
        
        // Allocate and read creatures
        if (chunk->creature_count > 0) {
            chunk->creatures = malloc(chunk->creature_count * sizeof(WildlifeCreature));
            if (!chunk->creatures) return false;
            
            if (remaining < chunk->creature_count * sizeof(WildlifeCreature)) return false;
            
            memcpy(chunk->creatures, ptr, chunk->creature_count * sizeof(WildlifeCreature));
            ptr += chunk->creature_count * sizeof(WildlifeCreature);
            remaining -= chunk->creature_count * sizeof(WildlifeCreature);
        }
        
        // Allocate and read vegetation
        if (chunk->vegetation_count > 0) {
            chunk->vegetation = malloc(chunk->vegetation_count * sizeof(VegetationInstance));
            if (!chunk->vegetation) return false;
            
            if (remaining < chunk->vegetation_count * sizeof(VegetationInstance)) return false;
            
            memcpy(chunk->vegetation, ptr, chunk->vegetation_count * sizeof(VegetationInstance));
            ptr += chunk->vegetation_count * sizeof(VegetationInstance);
            remaining -= chunk->vegetation_count * sizeof(VegetationInstance);
        }
    }
    
    *bytes_read = buffer_size - remaining;
    return true;
}

bool ecosystem_statistics(EcosystemManager *manager, uint32_t *total_creatures, 
                          uint32_t *total_vegetation, uint32_t *total_chunks) {
    if (!manager) return false;
    
    uint32_t creatures = 0;
    uint32_t vegetation = 0;
    
    for (uint32_t i = 0; i < manager->chunk_count; i++) {
        EcosystemChunk *chunk = &manager->chunks[i];
        creatures += chunk->creature_count;
        vegetation += chunk->vegetation_count;
    }
    
    if (total_creatures) *total_creatures = creatures;
    if (total_vegetation) *total_vegetation = vegetation;
    if (total_chunks) *total_chunks = manager->chunk_count;
    
    return true;
}

/* =================================================================================================
 *                                 SPECIES DATABASE SYSTEM
 * =================================================================================================
 */

static WildlifeSpecies *species_database = NULL;
static uint32_t species_capacity = 0;
static uint32_t species_count = 0;
static uint32_t next_species_id = 1;

bool species_database_init(uint32_t initial_capacity) {
    // Clean up existing database if any
    if (species_database) {
        free(species_database);
        species_database = NULL;
    }
    
    species_capacity = initial_capacity > 0 ? initial_capacity : 64;
    species_count = 0;
    next_species_id = 1;
    
    species_database = malloc(species_capacity * sizeof(WildlifeSpecies));
    if (!species_database) {
        species_capacity = 0;
        return false;
    }
    
    // Initialize all entries to zero
    memset(species_database, 0, species_capacity * sizeof(WildlifeSpecies));
    
    return true;
}

bool species_register(const WildlifeSpecies *species) {
    if (!species || !species_database) {
        return false;
    }
    
    // Expand database if needed
    if (species_count >= species_capacity) {
        uint32_t new_capacity = species_capacity * 2;
        WildlifeSpecies *new_database = realloc(species_database, 
                                               new_capacity * sizeof(WildlifeSpecies));
        if (!new_database) {
            return false;
        }
        
        species_database = new_database;
        species_capacity = new_capacity;
    }
    
    // Copy species data and assign unique ID
    WildlifeSpecies new_species = *species;
    new_species.id = next_species_id++;
    
    // Validate data
    if (strlen(new_species.name) == 0) {
        return false;
    }
    
    // Check for duplicate names
    for (uint32_t i = 0; i < species_count; i++) {
        if (strcmp(species_database[i].name, new_species.name) == 0) {
            return false; // Species with this name already exists
        }
    }
    
    species_database[species_count++] = new_species;
    return true;
}

const WildlifeSpecies* species_get(uint32_t species_id) {
    if (!species_database) {
        return NULL;
    }
    
    // Search for species by ID
    for (uint32_t i = 0; i < species_count; i++) {
        if (species_database[i].id == species_id) {
            return &species_database[i];
        }
    }
    
    return NULL; // Species not found
}

uint32_t species_find_by_biome(uint32_t biome_id, const WildlifeSpecies** results, uint32_t max_results) {
    if (!species_database || !results || max_results == 0) {
        return 0;
    }
    
    uint32_t found_count = 0;
    
    // Search for species that prefer this biome
    for (uint32_t i = 0; i < species_count && found_count < max_results; i++) {
        const WildlifeSpecies *species = &species_database[i];
        
        // Check if this species prefers the given biome
        for (uint32_t j = 0; j < species->biome_count; j++) {
            if (species->preferred_biomes[j] == biome_id) {
                results[found_count++] = species;
                break; // Found biome match, move to next species
            }
        }
    }
    
    return found_count;
}

bool species_load_json(const char* json_data) {
    if (!json_data || !species_database) {
        return false;
    }
    
    // Simple JSON parsing - this is a basic implementation
    // In a real implementation, you would use a proper JSON library
    
    // Look for species object in JSON
    const char* species_start = strstr(json_data, "\"species\"");
    if (!species_start) {
        return false; // No species data found
    }
    
    // Find array start
    const char* array_start = strstr(species_start, "[");
    if (!array_start) {
        return false;
    }
    
    // Simple parsing - extract basic species information
    // This is a placeholder implementation
    // Real implementation would properly parse JSON structure
    
    WildlifeSpecies temp_species = {0};
    
    // Extract name (simplified)
    const char* name_start = strstr(array_start, "\"name\"");
    if (name_start) {
        const char* colon = strstr(name_start, ":");
        if (colon) {
            const char* quote1 = strstr(colon, "\"");
            if (quote1) {
                const char* quote2 = strstr(quote1 + 1, "\"");
                if (quote2) {
                    size_t name_len = quote2 - (quote1 + 1);
                    if (name_len < sizeof(temp_species.name) - 1) {
                        strncpy(temp_species.name, quote1 + 1, name_len);
                        temp_species.name[name_len] = '\0';
                    }
                }
            }
        }
    }
    
    // Extract model path (simplified)
    const char* model_start = strstr(array_start, "\"model_path\"");
    if (model_start) {
        const char* colon = strstr(model_start, ":");
        if (colon) {
            const char* quote1 = strstr(colon, "\"");
            if (quote1) {
                const char* quote2 = strstr(quote1 + 1, "\"");
                if (quote2) {
                    size_t model_len = quote2 - (quote1 + 1);
                    if (model_len < sizeof(temp_species.model_path) - 1) {
                        strncpy(temp_species.model_path, quote1 + 1, model_len);
                        temp_species.model_path[model_len] = '\0';
                    }
                }
            }
        }
    }
    
    // Set default values for demonstration
    temp_species.behavior = BEHAVIOR_NEUTRAL;
    temp_species.aggression = 0.5f;
    temp_species.fear_radius = 10.0f;
    temp_species.chase_radius = 15.0f;
    temp_species.flee_speed = 8.0f;
    temp_species.walk_speed = 2.0f;
    temp_species.run_speed = 6.0f;
    temp_species.altitude_min = 0.0f;
    temp_species.altitude_max = 100.0f;
    temp_species.is_nocturnal = false;
    temp_species.is_aquatic = false;
    temp_species.can_fly = false;
    temp_species.biome_count = 1;
    temp_species.preferred_biomes[0] = 1; // Default biome
    temp_species.prey_count = 0;
    temp_species.predator_count = 0;
    temp_species.health = 100.0f;
    temp_species.hunger_rate = 0.1f;
    temp_species.thirst_rate = 0.15f;
    temp_species.stamina = 100.0f;
    temp_species.lifespan = 10.0f;
    temp_species.reproduction_rate = 0.05f;
    temp_species.loot_table_id = 0;
    
    // Register the species
    return species_register(&temp_species);
}

/* =================================================================================================
 *                                 CREATURE MANAGEMENT SYSTEM
 * =================================================================================================
 */

static WildlifeCreature *creature_registry = NULL;
static uint32_t creature_capacity = 0;
static uint32_t creature_count = 0;
static uint32_t next_creature_id = 1;

bool creature_spawn(WildlifeCreature *creature, uint32_t species_id, const float position[3]) {
    if (!creature || !position || !species_database) {
        return false;
    }
    
    // Get species data
    const WildlifeSpecies *species = species_get(species_id);
    if (!species) {
        return false; // Species not found
    }
    
    // Expand registry if needed
    if (creature_count >= creature_capacity) {
        uint32_t new_capacity = creature_capacity == 0 ? 128 : creature_capacity * 2;
        WildlifeCreature *new_registry = realloc(creature_registry, 
                                            new_capacity * sizeof(WildlifeCreature));
        if (!new_registry) {
            return false;
        }
        
        creature_registry = new_registry;
        creature_capacity = new_capacity;
    }
    
    // Initialize creature
    WildlifeCreature new_creature = {0};
    new_creature.id = next_creature_id++;
    new_creature.species_id = species_id;
    
    // Set position
    new_creature.position[0] = position[0];
    new_creature.position[1] = position[1];
    new_creature.position[2] = position[2];
    new_creature.rotation = 0.0f;
    
    // Initialize velocity
    new_creature.velocity[0] = 0.0f;
    new_creature.velocity[1] = 0.0f;
    new_creature.velocity[2] = 0.0f;
    
    // Set initial state
    new_creature.activity = ACTIVITY_IDLE;
    new_creature.activity_timer = 0.0f;
    new_creature.hunger = 0.0f;
    new_creature.thirst = 0.0f;
    new_creature.energy = 100.0f;
    new_creature.health = species->health;
    new_creature.age = 0.0f;
    
    // AI initialization
    new_creature.current_target = 0;
    new_creature.path_point_count = 0;
    new_creature.current_path_index = 0;
    
    // Pack/herd
    new_creature.pack_id = 0;
    new_creature.is_leader = false;
    
    // Spawning
    new_creature.home_chunk = 0; // Would be calculated from position
    new_creature.spawn_position[0] = position[0];
    new_creature.spawn_position[1] = position[1];
    new_creature.spawn_position[2] = position[2];
    
    // Copy to output parameter
    *creature = new_creature;
    
    // Add to registry
    creature_registry[creature_count++] = new_creature;
    
    return true;
}

bool creature_despawn(uint32_t creature_id) {
    if (!creature_registry) {
        return false;
    }
    
    // Find creature in registry
    for (uint32_t i = 0; i < creature_count; i++) {
        if (creature_registry[i].id == creature_id) {
            // Remove from registry by shifting remaining elements
            for (uint32_t j = i; j < creature_count - 1; j++) {
                creature_registry[j] = creature_registry[j + 1];
            }
            creature_count--;
            return true;
        }
    }
    
    return false; // Creature not found
}

bool creature_update_needs(WildlifeCreature *creature, float delta_time) {
    if (!creature || !species_database) {
        return false;
    }
    
    // Get species data for rates
    const WildlifeSpecies *species = species_get(creature->species_id);
    if (!species) {
        return false;
    }
    
    // Update needs based on species rates and delta time
    creature->hunger += species->hunger_rate * delta_time;
    creature->thirst += species->thirst_rate * delta_time;
    
    // Clamp values to valid ranges
    if (creature->hunger > 100.0f) creature->hunger = 100.0f;
    if (creature->thirst > 100.0f) creature->thirst = 100.0f;
    
    // Update energy based on activity
    float energy_drain = 0.0f;
    switch (creature->activity) {
        case ACTIVITY_IDLE:
            energy_drain = 0.5f * delta_time;
            break;
        case ACTIVITY_ROAMING:
            energy_drain = 1.0f * delta_time;
            break;
        case ACTIVITY_EATING:
        case ACTIVITY_DRINKING:
            energy_drain = 0.3f * delta_time;
            break;
        case ACTIVITY_SLEEPING:
            energy_drain = -2.0f * delta_time; // Restore energy
            break;
        case ACTIVITY_HUNTING:
        case ACTIVITY_FLEEING:
            energy_drain = 3.0f * delta_time;
            break;
        case ACTIVITY_MATING:
        case ACTIVITY_NESTING:
            energy_drain = 1.5f * delta_time;
            break;
    }
    
    creature->energy -= energy_drain;
    if (creature->energy > 100.0f) creature->energy = 100.0f;
    if (creature->energy < 0.0f) creature->energy = 0.0f;
    
    // Update age
    creature->age += delta_time;
    
    // Health effects from extreme needs
    if (creature->hunger > 80.0f || creature->thirst > 80.0f) {
        creature->health -= 2.0f * delta_time; // Take damage
    }
    
    // Clamp health
    if (creature->health > species->health) creature->health = species->health;
    if (creature->health < 0.0f) creature->health = 0.0f;
    
    return true;
}

bool creature_find_water(const WildlifeCreature *creature, float water_position[3]) {
    if (!creature || !water_position || !species_database) {
        return false;
    }
    
    // Get species data for movement capabilities
    const WildlifeSpecies *species = species_get(creature->species_id);
    if (!species) {
        return false;
    }
    
    // Simple water finding algorithm - search in expanding radius
    float search_radius = 50.0f; // Start with 50 unit radius
    float max_search_radius = 200.0f; // Maximum search radius
    float step_size = 10.0f; // Search step size
    
    // Check if creature is aquatic (can swim)
    if (species->is_aquatic) {
        // Aquatic creatures can find water more easily
        search_radius = 25.0f;
        step_size = 5.0f;
    }
    
    // Simple water detection - in real implementation this would use terrain/water data
    // For now, simulate finding water at a reasonable distance
    
    // Search in expanding circles
    while (search_radius <= max_search_radius) {
        // Check 8 directions around the creature
        for (int angle = 0; angle < 360; angle += 45) {
            float rad = (float)angle * 3.14159f / 180.0f;
            
            water_position[0] = creature->position[0] + cosf(rad) * search_radius;
            water_position[1] = creature->position[1]; // Keep same height for now
            water_position[2] = creature->position[2] + sinf(rad) * search_radius;
            
            // Simple water detection check (placeholder)
            // In real implementation, this would check terrain height vs water level
            bool found_water = false;
            
            // Simulate water finding based on position
            // Water is more likely to be found at lower elevations
            if (water_position[1] < creature->position[1]) {
                found_water = true;
            }
            
            // Add some randomness for realistic behavior
            if ((creature->id + angle) % 8 == 0) {
                found_water = true;
            }
            
            if (found_water) {
                return true; // Water found
            }
        }
        
        search_radius += step_size;
    }
    
    // No water found within search radius
    water_position[0] = creature->position[0];
    water_position[1] = creature->position[1];
    water_position[2] = creature->position[2];
    
    return false;
}

bool creature_find_shelter(const WildlifeCreature *creature, float shelter_position[3]) {
    if (!creature || !shelter_position || !species_database) {
        return false;
    }
    
    // Get species data for behavior
    const WildlifeSpecies *species = species_get(creature->species_id);
    if (!species) {
        return false;
    }
    
    // Shelter finding algorithm - search for suitable shelter
    float search_radius = 30.0f; // Start with 30 unit radius
    float max_search_radius = 150.0f; // Maximum search radius
    float step_size = 8.0f; // Search step size
    
    // Different shelter preferences based on species behavior
    if (species->behavior == BEHAVIOR_PASSIVE || species->behavior == BEHAVIOR_PREY) {
        // Passive/prey animals seek shelter more actively
        search_radius = 20.0f;
        max_search_radius = 100.0f;
    } else if (species->behavior == BEHAVIOR_PREDATOR || species->behavior == BEHAVIOR_TERRITORIAL) {
        // Predators/territorial animals are less concerned about shelter
        search_radius = 40.0f;
        max_search_radius = 200.0f;
    }
    
    // Search for shelter in expanding circles
    while (search_radius <= max_search_radius) {
        // Check 8 directions around creature
        for (int angle = 0; angle < 360; angle += 45) {
            float rad = (float)angle * 3.14159f / 180.0f;
            
            shelter_position[0] = creature->position[0] + cosf(rad) * search_radius;
            shelter_position[1] = creature->position[1]; // Keep same height for now
            shelter_position[2] = creature->position[2] + sinf(rad) * search_radius;
            
            // Simple shelter detection check (placeholder)
            bool found_shelter = false;
            
            // Simulate shelter finding based on terrain features
            // Shelter is more likely to be found at higher elevations (caves, cliffs)
            if (shelter_position[1] > creature->position[1] + 2.0f) {
                found_shelter = true;
            }
            
            // Check for forest cover (simplified)
            if ((creature->id + angle) % 6 == 0) {
                found_shelter = true;
            }
            
            // Nocturnal animals prefer dark/sheltered areas during day
            if (species->is_nocturnal && (creature->id + angle) % 4 == 0) {
                found_shelter = true;
            }
            
            if (found_shelter) {
                return true; // Shelter found
            }
        }
        
        search_radius += step_size;
    }
    
    // No shelter found within search radius
    shelter_position[0] = creature->position[0];
    shelter_position[1] = creature->position[1];
    shelter_position[2] = creature->position[2];
    
    return false;
}

bool creature_flee_from(WildlifeCreature *creature, const float threat_position[3]) {
    if (!creature || !threat_position || !species_database) {
        return false;
    }
    
    // Get species data for behavior
    const WildlifeSpecies *species = species_get(creature->species_id);
    if (!species) {
        return false;
    }
    
    // Calculate distance to threat
    float dx = threat_position[0] - creature->position[0];
    float dy = threat_position[1] - creature->position[1];
    float dz = threat_position[2] - creature->position[2];
    float distance = sqrtf(dx*dx + dy*dy + dz*dz);
    
    // Check if threat is within fear radius
    if (distance > species->fear_radius) {
        return false; // Threat too far to flee from
    }
    
    // Calculate flee direction (away from threat)
    float flee_dir[3];
    if (distance > 0.001f) {
        flee_dir[0] = -dx / distance;
        flee_dir[1] = -dy / distance;
        flee_dir[2] = -dz / distance;
    } else {
        // Threat is very close, flee in random direction
        flee_dir[0] = (float)(creature->id % 3 - 1);
        flee_dir[1] = 0.0f;
        flee_dir[2] = (float)(creature->id % 3 - 1);
    }
    
    // Set creature state to fleeing
    creature->activity = ACTIVITY_FLEEING;
    creature->activity_timer = 0.0f;
    creature->current_target = 0; // Clear current target
    
    // Calculate flee destination (move away from threat)
    float flee_distance = species->fear_radius * 1.5f; // Flee beyond fear radius
    creature->path_points[0][0] = creature->position[0] + flee_dir[0] * flee_distance;
    creature->path_points[0][1] = creature->position[1] + flee_dir[1] * flee_distance;
    creature->path_points[0][2] = creature->position[2] + flee_dir[2] * flee_distance;
    creature->path_point_count = 1;
    creature->current_path_index = 0;
    
    // Set velocity to flee speed
    creature->velocity[0] = flee_dir[0] * species->flee_speed;
    creature->velocity[1] = flee_dir[1] * species->flee_speed;
    creature->velocity[2] = flee_dir[2] * species->flee_speed;
    
    return true;
}
