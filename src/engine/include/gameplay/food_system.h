#ifndef FOOD_SYSTEM_H
#define FOOD_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

// Nutrient types for food items
typedef enum nutrient_type {
    NUTRIENT_PROTEIN = 0,
    NUTRIENT_CARBS,
    NUTRIENT_FAT,
    NUTRIENT_VITAMINS,
    NUTRIENT_MINERALS,
    NUTRIENT_WATER,
    NUTRIENT_FIBER,
    NUTRIENT_ANTIOXIDANTS
} nutrient_type_t;

// Forward declarations for opaque types
typedef struct food_item food_item_t;
typedef struct character_nutrition character_nutrition_t;

// Initialize the food system
bool food_system_init(void);

// Shutdown the food system
void food_system_shutdown(void);

// Add a new food item to the system
// name: Food item name
// description: Food description
// health_restore: Health points restored
// stamina_restore: Stamina points restored
// hunger_reduction: Hunger reduction amount
// thirst_reduction: Thirst reduction amount
// Returns: Food item ID or 0 on failure
uint32_t food_add_item(const char* name, const char* description,
                      float health_restore, float stamina_restore,
                      float hunger_reduction, float thirst_reduction);

// Add nutritional value to a food item
// item_id: Food item to modify
// type: Type of nutrient
// amount: Amount of nutrient
// quality: Quality factor (0.0 to 1.0)
bool food_add_nutrient(uint32_t item_id, nutrient_type_t type, float amount, float quality);

// Set preparation requirements for food
// item_id: Food item to modify
// requires_cooking: Whether cooking is required
// prep_time_ms: Preparation time in milliseconds
// is_perishable: Whether food spoils over time
// spoil_time_ms: Time until food spoils
bool food_set_preparation(uint32_t item_id, bool requires_cooking, uint32_t prep_time_ms,
                         bool is_perishable, uint32_t spoil_time_ms);

// Set buff effects for food
// item_id: Food item to modify
// buff_id: Buff to apply when consumed
// duration: Buff duration in seconds
bool food_set_buff(uint32_t item_id, uint32_t buff_id, float duration);

// Consume a food item
// player_id: Player consuming the food
// item_id: Food item being consumed
// quantity: Number of items consumed
bool food_consume_item(uint32_t player_id, uint32_t item_id, uint32_t quantity);

// Cook a raw food item
// player_id: Player doing the cooking
// raw_item_id: Raw food item
// cooked_item_id: Resulting cooked food item
bool food_cook_item(uint32_t player_id, uint32_t raw_item_id, uint32_t cooked_item_id);

// Update nutrition status for a player
// player_id: Player to update
// delta_time: Time since last update in seconds
void food_update_player(uint32_t player_id, float delta_time);

// Set activity level for metabolism calculations
// player_id: Player to modify
// activity_multiplier: Activity level multiplier
void food_set_activity_level(uint32_t player_id, float activity_multiplier);

// Set metabolism rate for a player
// player_id: Player to modify
// metabolism_rate: Metabolism rate multiplier
void food_set_metabolism(uint32_t player_id, float metabolism_rate);

// Check if a food item has spoiled
// item_id: Food item to check
// creation_time_ms: When the item was created
bool food_is_spoiled(uint32_t item_id, uint32_t creation_time_ms);

// Get current nutrient level for a player
// player_id: Player to check
// nutrient_type: Type of nutrient
// Returns: Current nutrient level
float food_get_nutrition_level(uint32_t player_id, nutrient_type_t nutrient_type);

// Get comprehensive nutrition status for a player
// player_id: Player to check
// is_hungry: Output hunger status
// is_thirsty: Output thirst status
// is_overfed: Output overfed status
// health_bonus: Output health bonus
// stamina_bonus: Output stamina bonus
void food_get_nutrition_status(uint32_t player_id, bool* is_hungry, bool* is_thirsty,
                              bool* is_overfed, float* health_bonus, float* stamina_bonus);

// Get food item information
// item_id: Food item to retrieve
// Returns: Food item pointer or NULL if not found
const food_item_t* food_get_item(uint32_t item_id);

// Get all food items
// items: Output array of food items
// item_count: Output number of food items
void food_get_all_items(const food_item_t** items, uint32_t* item_count);

// Utility function to get current time (placeholder)
uint32_t get_current_time_ms(void);

#endif // FOOD_SYSTEM_H
