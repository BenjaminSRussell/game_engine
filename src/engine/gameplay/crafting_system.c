#include "crafting_system.h"
#include "core/common/memory/allocator.h"
#include "engine/include/core/logger.h"
#include <string.h>
#include <stdlib.h>

#define MAX_RECIPES 1024
#define MAX_INGREDIENTS_PER_RECIPE 8
#define MAX_RESULTS_PER_RECIPE 4
#define MAX_CRAFTING_STATIONS 64
#define RECIPE_NAME_LENGTH 64
#define ITEM_NAME_LENGTH 32

typedef struct ingredient {
    uint32_t item_id;
    uint32_t quantity;
    bool required;  // false = optional
} ingredient_t;

typedef struct crafting_result {
    uint32_t item_id;
    uint32_t quantity;
    float probability;  // 0.0 to 1.0
} crafting_result_t;

typedef struct recipe {
    uint32_t recipe_id;
    char name[RECIPE_NAME_LENGTH];
    
    ingredient_t ingredients[MAX_INGREDIENTS_PER_RECIPE];
    uint32_t ingredient_count;
    
    crafting_result_t results[MAX_RESULTS_PER_RECIPE];
    uint32_t result_count;
    
    uint32_t crafting_time_ms;
    uint32_t required_station_type;
    uint32_t skill_level_required;
    float skill_xp_reward;
    
    bool enabled;
    bool discovered;
} recipe_t;

typedef struct crafting_station {
    uint32_t station_id;
    uint32_t station_type;
    uint32_t entity_id;
    uint32_t grid_width;
    uint32_t grid_height;
    
    uint32_t* grid_items;  // item_id per grid slot
    uint32_t* grid_quantities;  // quantity per grid slot
    
    bool is_active;
    uint32_t current_recipe_id;
    uint32_t crafting_progress;
    uint32_t crafting_start_time;
} crafting_station_t;

typedef struct crafting_system {
    recipe_t recipes[MAX_RECIPES];
    uint32_t recipe_count;
    
    crafting_station_t stations[MAX_CRAFTING_STATIONS];
    uint32_t station_count;
    
    uint32_t next_recipe_id;
    uint32_t next_station_id;
    
    bool initialized;
} crafting_system_t;

static crafting_system_t g_crafting = {0};

// Forward declarations
static bool recipe_can_be_crafted(const recipe_t* recipe, uint32_t player_id);
static void consume_ingredients(const recipe_t* recipe, uint32_t player_id);
static void give_results(const recipe_t* recipe, uint32_t player_id);
static uint32_t get_current_time_ms(void);

bool crafting_init(void) {
    if (g_crafting.initialized) {
        return true;
    }
    
    memset(&g_crafting, 0, sizeof(g_crafting));
    g_crafting.next_recipe_id = 1;
    g_crafting.next_station_id = 1;
    
    g_crafting.initialized = true;
    log_info("Crafting system initialized");
    return true;
}

void crafting_shutdown(void) {
    if (!g_crafting.initialized) {
        return;
    }
    
    // Clean up station grids
    for (uint32_t i = 0; i < g_crafting.station_count; i++) {
        if (g_crafting.stations[i].grid_items) {
            memory_free(g_crafting.stations[i].grid_items);
        }
        if (g_crafting.stations[i].grid_quantities) {
            memory_free(g_crafting.stations[i].grid_quantities);
        }
    }
    
    memset(&g_crafting, 0, sizeof(g_crafting));
    log_info("Crafting system shutdown");
}

uint32_t crafting_add_recipe(const char* name, uint32_t crafting_time_ms,
                            uint32_t required_station_type, uint32_t skill_level_required,
                            float skill_xp_reward) {
    if (!g_crafting.initialized || !name || g_crafting.recipe_count >= MAX_RECIPES) {
        return 0;
    }
    
    recipe_t* recipe = &g_crafting.recipes[g_crafting.recipe_count];
    memset(recipe, 0, sizeof(recipe_t));
    
    recipe->recipe_id = g_crafting.next_recipe_id++;
    strncpy(recipe->name, name, RECIPE_NAME_LENGTH - 1);
    recipe->crafting_time_ms = crafting_time_ms;
    recipe->required_station_type = required_station_type;
    recipe->skill_level_required = skill_level_required;
    recipe->skill_xp_reward = skill_xp_reward;
    recipe->enabled = true;
    recipe->discovered = false;
    
    g_crafting.recipe_count++;
    log_debug("Added recipe: %s (ID: %u)", name, recipe->recipe_id);
    
    return recipe->recipe_id;
}

bool crafting_add_recipe_ingredient(uint32_t recipe_id, uint32_t item_id,
                                   uint32_t quantity, bool required) {
    if (!g_crafting.initialized) {
        return false;
    }
    
    for (uint32_t i = 0; i < g_crafting.recipe_count; i++) {
        recipe_t* recipe = &g_crafting.recipes[i];
        if (recipe->recipe_id == recipe_id) {
            if (recipe->ingredient_count >= MAX_INGREDIENTS_PER_RECIPE) {
                log_error("Recipe %u already has maximum ingredients", recipe_id);
                return false;
            }
            
            ingredient_t* ingredient = &recipe->ingredients[recipe->ingredient_count];
            ingredient->item_id = item_id;
            ingredient->quantity = quantity;
            ingredient->required = required;
            
            recipe->ingredient_count++;
            log_debug("Added ingredient to recipe %u: item %u x%u", recipe_id, item_id, quantity);
            return true;
        }
    }
    
    log_error("Recipe %u not found", recipe_id);
    return false;
}

bool crafting_add_recipe_result(uint32_t recipe_id, uint32_t item_id,
                              uint32_t quantity, float probability) {
    if (!g_crafting.initialized) {
        return false;
    }
    
    for (uint32_t i = 0; i < g_crafting.recipe_count; i++) {
        recipe_t* recipe = &g_crafting.recipes[i];
        if (recipe->recipe_id == recipe_id) {
            if (recipe->result_count >= MAX_RESULTS_PER_RECIPE) {
                log_error("Recipe %u already has maximum results", recipe_id);
                return false;
            }
            
            crafting_result_t* result = &recipe->results[recipe->result_count];
            result->item_id = item_id;
            result->quantity = quantity;
            result->probability = probability;
            
            recipe->result_count++;
            log_debug("Added result to recipe %u: item %u x%u (%.1f%% chance)",
                     recipe_id, item_id, quantity, probability * 100.0f);
            return true;
        }
    }
    
    log_error("Recipe %u not found", recipe_id);
    return false;
}

uint32_t crafting_add_station(uint32_t station_type, uint32_t entity_id,
                             uint32_t grid_width, uint32_t grid_height) {
    if (!g_crafting.initialized || g_crafting.station_count >= MAX_CRAFTING_STATIONS) {
        return 0;
    }
    
    crafting_station_t* station = &g_crafting.stations[g_crafting.station_count];
    memset(station, 0, sizeof(crafting_station_t));
    
    station->station_id = g_crafting.next_station_id++;
    station->station_type = station_type;
    station->entity_id = entity_id;
    station->grid_width = grid_width;
    station->grid_height = grid_height;
    
    uint32_t grid_size = grid_width * grid_height;
    station->grid_items = memory_calloc(grid_size, sizeof(uint32_t));
    station->grid_quantities = memory_calloc(grid_size, sizeof(uint32_t));
    
    if (!station->grid_items || !station->grid_quantities) {
        log_error("Failed to allocate memory for station grid");
        if (station->grid_items) memory_free(station->grid_items);
        if (station->grid_quantities) memory_free(station->grid_quantities);
        return 0;
    }
    
    g_crafting.station_count++;
    log_debug("Added crafting station %u (type %u) for entity %u",
             station->station_id, station_type, entity_id);
    
    return station->station_id;
}

bool crafting_set_station_item(uint32_t station_id, uint32_t grid_x, uint32_t grid_y,
                              uint32_t item_id, uint32_t quantity) {
    if (!g_crafting.initialized) {
        return false;
    }
    
    for (uint32_t i = 0; i < g_crafting.station_count; i++) {
        crafting_station_t* station = &g_crafting.stations[i];
        if (station->station_id == station_id) {
            if (grid_x >= station->grid_width || grid_y >= station->grid_height) {
                log_error("Grid position (%u,%u) out of bounds for station %u",
                         grid_x, grid_y, station_id);
                return false;
            }
            
            uint32_t index = grid_y * station->grid_width + grid_x;
            station->grid_items[index] = item_id;
            station->grid_quantities[index] = quantity;
            
            log_debug("Set item %u x%u at station %u grid (%u,%u)",
                     item_id, quantity, station_id, grid_x, grid_y);
            return true;
        }
    }
    
    log_error("Station %u not found", station_id);
    return false;
}

static bool check_grid_recipe(const crafting_station_t* station, const recipe_t* recipe) {
    // Create a temporary copy of grid items to track used items
    uint32_t grid_size = station->grid_width * station->grid_height;
    uint32_t* temp_quantities = memory_alloc(grid_size * sizeof(uint32_t));
    if (!temp_quantities) {
        return false;
    }
    
    memcpy(temp_quantities, station->grid_quantities, grid_size * sizeof(uint32_t));
    
    // Check all required ingredients
    for (uint32_t i = 0; i < recipe->ingredient_count; i++) {
        const ingredient_t* ingredient = &recipe->ingredients[i];
        if (!ingredient->required) {
            continue;  // Skip optional ingredients for now
        }
        
        uint32_t remaining_quantity = ingredient->quantity;
        
        // Search for this ingredient in the grid
        for (uint32_t j = 0; j < grid_size && remaining_quantity > 0; j++) {
            if (station->grid_items[j] == ingredient->item_id && temp_quantities[j] > 0) {
                uint32_t use_quantity = (remaining_quantity < temp_quantities[j]) ?
                                       remaining_quantity : temp_quantities[j];
                temp_quantities[j] -= use_quantity;
                remaining_quantity -= use_quantity;
            }
        }
        
        if (remaining_quantity > 0) {
            memory_free(temp_quantities);
            return false;  // Not enough of this ingredient
        }
    }
    
    memory_free(temp_quantities);
    return true;
}

uint32_t crafting_find_matching_recipe(uint32_t station_id) {
    if (!g_crafting.initialized) {
        return 0;
    }
    
    // Find the station
    crafting_station_t* station = NULL;
    for (uint32_t i = 0; i < g_crafting.station_count; i++) {
        if (g_crafting.stations[i].station_id == station_id) {
            station = &g_crafting.stations[i];
            break;
        }
    }
    
    if (!station) {
        log_error("Station %u not found", station_id);
        return 0;
    }
    
    // Check each recipe to see if it matches the grid layout
    for (uint32_t i = 0; i < g_crafting.recipe_count; i++) {
        const recipe_t* recipe = &g_crafting.recipes[i];
        
        if (!recipe->enabled || recipe->required_station_type != station->station_type) {
            continue;
        }
        
        if (check_grid_recipe(station, recipe)) {
            log_debug("Found matching recipe %u for station %u", recipe->recipe_id, station_id);
            return recipe->recipe_id;
        }
    }
    
    return 0;  // No matching recipe found
}

bool crafting_start_crafting(uint32_t station_id, uint32_t recipe_id, uint32_t player_id) {
    if (!g_crafting.initialized) {
        return false;
    }
    
    // Find station and recipe
    crafting_station_t* station = NULL;
    const recipe_t* recipe = NULL;
    
    for (uint32_t i = 0; i < g_crafting.station_count; i++) {
        if (g_crafting.stations[i].station_id == station_id) {
            station = &g_crafting.stations[i];
            break;
        }
    }
    
    for (uint32_t i = 0; i < g_crafting.recipe_count; i++) {
        if (g_crafting.recipes[i].recipe_id == recipe_id) {
            recipe = &g_crafting.recipes[i];
            break;
        }
    }
    
    if (!station || !recipe) {
        log_error("Station %u or recipe %u not found", station_id, recipe_id);
        return false;
    }
    
    if (station->is_active) {
        log_error("Station %u is already crafting", station_id);
        return false;
    }
    
    if (!recipe_can_be_crafted(recipe, player_id)) {
        log_error("Player %u cannot craft recipe %u", player_id, recipe_id);
        return false;
    }
    
    // Start crafting
    station->is_active = true;
    station->current_recipe_id = recipe_id;
    station->crafting_progress = 0;
    station->crafting_start_time = get_current_time_ms();  // Assuming this function exists
    
    log_debug("Started crafting recipe %u at station %u for player %u",
             recipe_id, station_id, player_id);
    return true;
}

bool crafting_update(uint32_t current_time_ms) {
    if (!g_crafting.initialized) {
        return false;
    }
    
    for (uint32_t i = 0; i < g_crafting.station_count; i++) {
        crafting_station_t* station = &g_crafting.stations[i];
        
        if (!station->is_active) {
            continue;
        }
        
        // Find the recipe
        const recipe_t* recipe = NULL;
        for (uint32_t j = 0; j < g_crafting.recipe_count; j++) {
            if (g_crafting.recipes[j].recipe_id == station->current_recipe_id) {
                recipe = &g_crafting.recipes[j];
                break;
            }
        }
        
        if (!recipe) {
            log_error("Recipe %u not found for active station %u", station->current_recipe_id, station->station_id);
            station->is_active = false;
            continue;
        }
        
        // Update progress
        uint32_t elapsed_time = current_time_ms - station->crafting_start_time;
        station->crafting_progress = elapsed_time;
        
        // Check if crafting is complete
        if (elapsed_time >= recipe->crafting_time_ms) {
            // Get player ID (this would need to be stored in the station)
            uint32_t player_id = 0;  // Placeholder
            
            // Consume ingredients and give results
            consume_ingredients(recipe, player_id);
            give_results(recipe, player_id);
            
            // Clear the station grid
            uint32_t grid_size = station->grid_width * station->grid_height;
            memset(station->grid_items, 0, grid_size * sizeof(uint32_t));
            memset(station->grid_quantities, 0, grid_size * sizeof(uint32_t));
            
            // Reset station
            station->is_active = false;
            station->current_recipe_id = 0;
            station->crafting_progress = 0;
            
            log_debug("Completed crafting recipe %u at station %u", recipe->recipe_id, station->station_id);
        }
    }
    
    return true;
}

bool crafting_cancel_crafting(uint32_t station_id) {
    if (!g_crafting.initialized) {
        return false;
    }
    
    for (uint32_t i = 0; i < g_crafting.station_count; i++) {
        crafting_station_t* station = &g_crafting.stations[i];
        if (station->station_id == station_id && station->is_active) {
            station->is_active = false;
            station->current_recipe_id = 0;
            station->crafting_progress = 0;
            
            log_debug("Cancelled crafting at station %u", station_id);
            return true;
        }
    }
    
    return false;
}

const recipe_t* crafting_get_recipe(uint32_t recipe_id) {
    if (!g_crafting.initialized) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < g_crafting.recipe_count; i++) {
        if (g_crafting.recipes[i].recipe_id == recipe_id) {
            return &g_crafting.recipes[i];
        }
    }
    
    return NULL;
}

const crafting_station_t* crafting_get_station(uint32_t station_id) {
    if (!g_crafting.initialized) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < g_crafting.station_count; i++) {
        if (g_crafting.stations[i].station_id == station_id) {
            return &g_crafting.stations[i];
        }
    }
    
    return NULL;
}

void crafting_get_all_recipes(const recipe_t** recipes, uint32_t* recipe_count) {
    if (!g_crafting.initialized || !recipes || !recipe_count) {
        return;
    }
    
    *recipes = g_crafting.recipes;
    *recipe_count = g_crafting.recipe_count;
}

void crafting_get_recipes_by_station(uint32_t station_type, const recipe_t** recipes,
                                     uint32_t* recipe_count) {
    if (!g_crafting.initialized || !recipes || !recipe_count) {
        return;
    }
    
    // This is a simplified implementation - in practice you might want to
    // return a filtered list rather than making the caller filter
    *recipes = g_crafting.recipes;
    *recipe_count = 0;
    
    for (uint32_t i = 0; i < g_crafting.recipe_count; i++) {
        if (g_crafting.recipes[i].required_station_type == station_type &&
            g_crafting.recipes[i].enabled) {
            (*recipe_count)++;
        }
    }
}

bool crafting_discover_recipe(uint32_t recipe_id) {
    if (!g_crafting.initialized) {
        return false;
    }
    
    for (uint32_t i = 0; i < g_crafting.recipe_count; i++) {
        if (g_crafting.recipes[i].recipe_id == recipe_id) {
            g_crafting.recipes[i].discovered = true;
            log_debug("Discovered recipe %u", recipe_id);
            return true;
        }
    }
    
    return false;
}

bool crafting_station_is_active(uint32_t station_id) {
    const crafting_station_t* station = crafting_get_station(station_id);
    return station ? station->is_active : false;
}

bool crafting_recipe_get_name(uint32_t recipe_id, char* buffer, uint32_t buffer_size) {
    const recipe_t* recipe = crafting_get_recipe(recipe_id);
    if (!recipe || !buffer || buffer_size == 0) {
        return false;
    }

    strncpy(buffer, recipe->name, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    return true;
}

// Helper functions (these would typically interact with inventory/system)
static bool recipe_can_be_crafted(const recipe_t* recipe, uint32_t player_id) {
    // This would check player's inventory for required ingredients
    // and verify skill level requirements
    // For now: always return true since inventory integration would be game-specific
    log_debug("Checking if recipe %u can be crafted by player %u", recipe->recipe_id, player_id);
    return true;
}

static void consume_ingredients(const recipe_t* recipe, uint32_t player_id) {
    // This would remove ingredients from player's inventory
    // Log the ingredients being consumed for integration with inventory system
    for (uint32_t i = 0; i < recipe->ingredient_count; i++) {
        const ingredient_t* ingredient = &recipe->ingredients[i];
        log_debug("Consuming item %u x%u from player %u inventory",
                 ingredient->item_id, ingredient->quantity, player_id);
        // TODO: Call inventory_remove_item(player_id, ingredient->item_id, ingredient->quantity)
        // when inventory system is integrated
    }
}

static void give_results(const recipe_t* recipe, uint32_t player_id) {
    // This would add results to player's inventory
    // Handle probability for random results
    for (uint32_t i = 0; i < recipe->result_count; i++) {
        const crafting_result_t* result = &recipe->results[i];

        // Check if result should be given based on probability
        float roll = (float)rand() / RAND_MAX;
        if (roll < result->probability) {
            log_debug("Giving item %u x%u to player %u from recipe %u (probability: %.1f%%)",
                     result->item_id, result->quantity, player_id, recipe->recipe_id,
                     result->probability * 100.0f);
            // TODO: Call inventory_add_item(player_id, result->item_id, result->quantity)
            // when inventory system is integrated
        } else {
            log_debug("Item %u skipped for player %u (failed probability check: %.1f%%)",
                     result->item_id, player_id, result->probability * 100.0f);
        }
    }

    // Award XP to player for crafting this recipe
    if (recipe->skill_xp_reward > 0.0f) {
        log_debug("Awarding %.1f crafting XP to player %u from recipe %u",
                 recipe->skill_xp_reward, player_id, recipe->recipe_id);
        // TODO: Call leveling_add_xp(player_id, SKILL_CRAFTING, recipe->skill_xp_reward)
        // when leveling system is integrated
    }
}

static uint32_t get_current_time_ms(void) {
    // Placeholder - would typically use platform-specific time function
    static uint32_t counter = 0;
    return counter += 16;  // Simulate 60 FPS
}
