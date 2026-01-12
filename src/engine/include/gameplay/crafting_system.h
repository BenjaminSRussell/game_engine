#ifndef CRAFTING_SYSTEM_H
#define CRAFTING_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations for opaque types
typedef struct recipe recipe_t;
typedef struct crafting_station crafting_station_t;

// Initialize the crafting system
bool crafting_init(void);

// Shutdown the crafting system
void crafting_shutdown(void);

// Add a new recipe to the system
// name: Recipe name
// crafting_time_ms: Time required to craft in milliseconds
// required_station_type: Type of crafting station required
// skill_level_required: Minimum skill level required
// skill_xp_reward: XP awarded for crafting this recipe
// Returns: Recipe ID or 0 on failure
uint32_t crafting_add_recipe(const char* name, uint32_t crafting_time_ms,
                            uint32_t required_station_type, uint32_t skill_level_required,
                            float skill_xp_reward);

// Add an ingredient to a recipe
// recipe_id: Recipe to modify
// item_id: Item ID required
// quantity: Quantity required
// required: Whether this ingredient is required (true) or optional (false)
bool crafting_add_recipe_ingredient(uint32_t recipe_id, uint32_t item_id,
                                   uint32_t quantity, bool required);

// Add a result to a recipe
// recipe_id: Recipe to modify
// item_id: Result item ID
// quantity: Quantity produced
// probability: Probability of receiving this result (0.0 to 1.0)
bool crafting_add_recipe_result(uint32_t recipe_id, uint32_t item_id,
                              uint32_t quantity, float probability);

// Add a crafting station
// station_type: Type of station
// entity_id: Entity ID the station is attached to
// grid_width: Width of crafting grid
// grid_height: Height of crafting grid
// Returns: Station ID or 0 on failure
uint32_t crafting_add_station(uint32_t station_type, uint32_t entity_id,
                             uint32_t grid_width, uint32_t grid_height);

// Set an item in a station's crafting grid
// station_id: Station to modify
// grid_x, grid_y: Grid position
// item_id: Item ID to place
// quantity: Quantity of item
bool crafting_set_station_item(uint32_t station_id, uint32_t grid_x, uint32_t grid_y,
                              uint32_t item_id, uint32_t quantity);

// Find a recipe that matches the current grid layout
// station_id: Station to check
// Returns: Recipe ID or 0 if no match found
uint32_t crafting_find_matching_recipe(uint32_t station_id);

// Start crafting a recipe at a station
// station_id: Station to use
// recipe_id: Recipe to craft
// player_id: Player doing the crafting
bool crafting_start_crafting(uint32_t station_id, uint32_t recipe_id, uint32_t player_id);

// Update all active crafting stations
// current_time_ms: Current time in milliseconds
bool crafting_update(uint32_t current_time_ms);

// Cancel crafting at a station
// station_id: Station to cancel
bool crafting_cancel_crafting(uint32_t station_id);

// Get recipe information
// recipe_id: Recipe to retrieve
// Returns: Recipe pointer or NULL if not found
const recipe_t* crafting_get_recipe(uint32_t recipe_id);

// Get station information
// station_id: Station to retrieve
// Returns: Station pointer or NULL if not found
const crafting_station_t* crafting_get_station(uint32_t station_id);

// Get all recipes
// recipes: Output array of recipes
// recipe_count: Output number of recipes
void crafting_get_all_recipes(const recipe_t** recipes, uint32_t* recipe_count);

// Get recipes for a specific station type
// station_type: Station type to filter by
// recipes: Output array of recipes
// recipe_count: Output number of matching recipes
void crafting_get_recipes_by_station(uint32_t station_type, const recipe_t** recipes,
                                     uint32_t* recipe_count);

// Mark a recipe as discovered by a player
// recipe_id: Recipe to discover
bool crafting_discover_recipe(uint32_t recipe_id);

// Utility function to get current time (placeholder)
uint32_t get_current_time_ms(void);

#endif // CRAFTING_SYSTEM_H
