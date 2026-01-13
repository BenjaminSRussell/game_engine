#include "food_system.h"
#include "gameplay/crafting_system.h"
#include "core/common/memory/allocator.h"
#include "engine/include/core/logger.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_PLAYERS 1024
#define MAX_FOOD_ITEMS 512
#define MAX_FOOD_TYPES 64
#define FOOD_NAME_LENGTH 32
#define FOOD_DESCRIPTION_LENGTH 128
#define MAX_NUTRIENTS 8

typedef struct nutrient_value {
    nutrient_type_t type;
    float amount;
    float quality;  // 0.0 to 1.0, affects effectiveness
} nutrient_value_t;

typedef struct food_item {
    uint32_t item_id;
    char name[FOOD_NAME_LENGTH];
    char description[FOOD_DESCRIPTION_LENGTH];
    
    nutrient_value_t nutrients[MAX_NUTRIENTS];
    uint32_t nutrient_count;
    
    float health_restore;
    float stamina_restore;
    float hunger_reduction;
    float thirst_reduction;
    
    uint32_t preparation_time_ms;
    bool requires_cooking;
    uint32_t cooked_item_id;
    bool is_perishable;
    uint32_t spoil_time_ms;
    
    uint32_t buff_id;
    float buff_duration;
    
    float weight;
    uint32_t stack_size;
    
    bool enabled;
} food_item_t;

typedef struct character_nutrition {
    uint32_t player_id;
    float nutrient_levels[MAX_NUTRIENTS];
    float last_meal_time;
    float total_calories_consumed;
    
    float metabolism_rate;
    float activity_multiplier;
    
    bool is_hungry;
    bool is_thirsty;
    bool is_overfed;
    
    float health_bonus;
    float stamina_bonus;
} character_nutrition_t;

typedef struct food_system {
    food_item_t food_items[MAX_FOOD_ITEMS];
    uint32_t food_count;
    
    character_nutrition_t player_nutrition[MAX_PLAYERS];
    uint32_t player_count;
    
    uint32_t next_food_id;
    
    bool initialized;
} food_system_t;

static food_system_t g_food_system = {0};

// Forward declarations
static food_item_t* get_food_item(uint32_t item_id);
static character_nutrition_t* get_player_nutrition(uint32_t player_id);
static void apply_nutrients(uint32_t player_id, const food_item_t* food);
static void update_nutrition_status(uint32_t player_id, float delta_time);
static float calculate_nutrition_bonus(const character_nutrition_t* nutrition);
static float calculate_calories(const food_item_t* food);
static uint32_t get_current_time_ms(void);
static void register_furnace_recipe(food_item_t* food);

bool food_system_init(void) {
    if (g_food_system.initialized) {
        return true;
    }
    
    memset(&g_food_system, 0, sizeof(g_food_system));
    g_food_system.next_food_id = 1;
    
    // Initialize default metabolism rates
    for (uint32_t i = 0; i < MAX_PLAYERS; i++) {
        g_food_system.player_nutrition[i].metabolism_rate = 1.0f;
        g_food_system.player_nutrition[i].activity_multiplier = 1.0f;
    }
    
    g_food_system.initialized = true;
    log_info("Food system initialized");
    return true;
}

void food_system_shutdown(void) {
    if (!g_food_system.initialized) {
        return;
    }
    
    memset(&g_food_system, 0, sizeof(g_food_system));
    log_info("Food system shutdown");
}

uint32_t food_add_item(const char* name, const char* description,
                      float health_restore, float stamina_restore,
                      float hunger_reduction, float thirst_reduction) {
    if (!g_food_system.initialized || !name || !description ||
        g_food_system.food_count >= MAX_FOOD_ITEMS) {
        return 0;
    }
    
    food_item_t* food = &g_food_system.food_items[g_food_system.food_count];
    memset(food, 0, sizeof(food_item_t));
    
    food->item_id = g_food_system.next_food_id++;
    strncpy(food->name, name, FOOD_NAME_LENGTH - 1);
    strncpy(food->description, description, FOOD_DESCRIPTION_LENGTH - 1);
    food->health_restore = health_restore;
    food->stamina_restore = stamina_restore;
    food->hunger_reduction = hunger_reduction;
    food->thirst_reduction = thirst_reduction;
    food->enabled = true;
    food->stack_size = 64;
    food->weight = 0.1f;
    
    g_food_system.food_count++;
    log_debug("Added food item: %s (ID: %u)", name, food->item_id);
    
    return food->item_id;
}

bool food_add_nutrient(uint32_t item_id, nutrient_type_t type, float amount, float quality) {
    if (!g_food_system.initialized) {
        return false;
    }
    
    food_item_t* food = get_food_item(item_id);
    if (!food || food->nutrient_count >= MAX_NUTRIENTS) {
        return false;
    }
    
    nutrient_value_t* nutrient = &food->nutrients[food->nutrient_count];
    nutrient->type = type;
    nutrient->amount = amount;
    nutrient->quality = quality;
    
    food->nutrient_count++;
    log_debug("Added nutrient to food %u: type %u, amount %.2f, quality %.2f",
             item_id, type, amount, quality);
    
    return true;
}

bool food_set_preparation(uint32_t item_id, bool requires_cooking, uint32_t prep_time_ms,
                         bool is_perishable, uint32_t spoil_time_ms) {
    if (!g_food_system.initialized) {
        return false;
    }
    
    food_item_t* food = get_food_item(item_id);
    if (!food) {
        return false;
    }
    
    food->requires_cooking = requires_cooking;
    food->preparation_time_ms = prep_time_ms;
    food->is_perishable = is_perishable;
    food->spoil_time_ms = spoil_time_ms;
    
    log_debug("Set preparation for food %u: cooking=%s, prep_time=%u, perishable=%s, spoil_time=%u",
             item_id, requires_cooking ? "yes" : "no", prep_time_ms,
             is_perishable ? "yes" : "no", spoil_time_ms);
    
    return true;
}

bool food_set_buff(uint32_t item_id, uint32_t buff_id, float duration) {
    if (!g_food_system.initialized) {
        return false;
    }
    
    food_item_t* food = get_food_item(item_id);
    if (!food) {
        return false;
    }
    
    food->buff_id = buff_id;
    food->buff_duration = duration;
    
    log_debug("Set buff for food %u: buff_id=%u, duration=%.1f", item_id, buff_id, duration);
    
    return true;
}

bool food_consume_item(uint32_t player_id, uint32_t item_id, uint32_t quantity) {
    if (!g_food_system.initialized || quantity == 0) {
        return false;
    }
    
    food_item_t* food = get_food_item(item_id);
    if (!food || !food->enabled) {
        return false;
    }
    
    character_nutrition_t* nutrition = get_player_nutrition(player_id);
    if (!nutrition) {
        return false;
    }
    
    // Apply effects for each consumed item
    for (uint32_t i = 0; i < quantity; i++) {
        // Restore health and stamina
        nutrition->health_bonus += food->health_restore;
        nutrition->stamina_bonus += food->stamina_restore;
        
        // Reduce hunger and thirst
        nutrition->is_hungry = false;
        nutrition->is_thirsty = false;
        
        // Apply nutrients
        apply_nutrients(player_id, food);
        
        // Update meal timing
        nutrition->last_meal_time = get_current_time_ms() / 1000.0f;
        nutrition->total_calories_consumed += calculate_calories(food);
        
        // Apply buff if any
        if (food->buff_id != 0) {
            // This would interface with the buff system
            log_debug("Applied buff %u to player %u for %.1f seconds",
                     food->buff_id, player_id, food->buff_duration);
        }
    }
    
    log_info("Player %u consumed %u x %s", player_id, quantity, food->name);
    return true;
}

bool food_cook_item(uint32_t player_id, uint32_t raw_item_id, uint32_t cooked_item_id) {
    if (!g_food_system.initialized) {
        return false;
    }
    
    food_item_t* raw_food = get_food_item(raw_item_id);
    food_item_t* cooked_food = get_food_item(cooked_item_id);
    
    if (!raw_food || !cooked_food || !raw_food->requires_cooking) {
        return false;
    }
    
    // Cooking improves nutritional value and removes harmful effects
    log_info("Player %u cooked %s into %s", player_id, raw_food->name, cooked_food->name);
    
    // This would typically remove raw item from inventory and add cooked item
    return true;
}

bool food_set_cooking_result(uint32_t raw_item_id, uint32_t cooked_item_id) {
    if (!g_food_system.initialized) {
        return false;
    }

    food_item_t* raw_food = get_food_item(raw_item_id);
    if (!raw_food) {
        return false;
    }

    raw_food->cooked_item_id = cooked_item_id;
    log_debug("Set cooking result for %s (ID: %u) to ID: %u",
             raw_food->name, raw_item_id, cooked_item_id);

    // Automatically register furnace recipe if crafting system is available
    if (raw_food->requires_cooking) {
        register_furnace_recipe(raw_food);
    }

    return true;
}

void food_register_furnace_recipes(void) {
    if (!g_food_system.initialized) {
        return;
    }

    uint32_t count = 0;
    for (uint32_t i = 0; i < g_food_system.food_count; i++) {
        food_item_t* food = &g_food_system.food_items[i];

        // If food requires cooking and has a defined cooked version
        if (food->requires_cooking && food->cooked_item_id != 0) {
            register_furnace_recipe(food);
            count++;
        }
    }

    log_info("Registered %u furnace recipes from food system", count);
}

static void register_furnace_recipe(food_item_t* food) {
    if (!food || !food->requires_cooking || food->cooked_item_id == 0) {
        return;
    }

    char recipe_name[64];
    snprintf(recipe_name, sizeof(recipe_name), "Cook %s", food->name);

    // Register recipe in crafting system
    // Use preparation_time_ms as crafting time
    uint32_t recipe_id = crafting_add_recipe(
        recipe_name,
        food->preparation_time_ms > 0 ? food->preparation_time_ms : 5000,
        STATION_TYPE_FURNACE,
        0, // No skill requirement for now
        10.0f // Some XP reward
    );

    if (recipe_id != 0) {
        // Add ingredient (1x Raw Food)
        crafting_add_recipe_ingredient(recipe_id, food->item_id, 1, true);

        // Add result (1x Cooked Food)
        crafting_add_recipe_result(recipe_id, food->cooked_item_id, 1, 1.0f);

        log_debug("Registered furnace recipe: %s", recipe_name);
    }
}

void food_update_player(uint32_t player_id, float delta_time) {
    if (!g_food_system.initialized) {
        return;
    }
    
    character_nutrition_t* nutrition = get_player_nutrition(player_id);
    if (!nutrition) {
        return;
    }
    
    // Update nutrition status
    update_nutrition_status(player_id, delta_time);
    
    // Calculate bonuses
    nutrition->health_bonus = calculate_nutrition_bonus(nutrition);
    nutrition->stamina_bonus = nutrition->health_bonus * 0.8f;  // Stamina is 80% of health bonus
}

void food_set_activity_level(uint32_t player_id, float activity_multiplier) {
    if (!g_food_system.initialized) {
        return;
    }
    
    character_nutrition_t* nutrition = get_player_nutrition(player_id);
    if (!nutrition) {
        return;
    }
    
    nutrition->activity_multiplier = activity_multiplier;
    log_debug("Set activity level for player %u: %.2f", player_id, activity_multiplier);
}

void food_set_metabolism(uint32_t player_id, float metabolism_rate) {
    if (!g_food_system.initialized) {
        return;
    }
    
    character_nutrition_t* nutrition = get_player_nutrition(player_id);
    if (!nutrition) {
        return;
    }
    
    nutrition->metabolism_rate = metabolism_rate;
    log_debug("Set metabolism rate for player %u: %.2f", player_id, metabolism_rate);
}

bool food_is_spoiled(uint32_t item_id, uint32_t creation_time_ms) {
    if (!g_food_system.initialized) {
        return false;
    }
    
    food_item_t* food = get_food_item(item_id);
    if (!food || !food->is_perishable) {
        return false;
    }
    
    uint32_t current_time = get_current_time_ms();
    uint32_t age = current_time - creation_time_ms;
    
    return age >= food->spoil_time_ms;
}

float food_get_nutrition_level(uint32_t player_id, nutrient_type_t nutrient_type) {
    if (!g_food_system.initialized) {
        return 0.0f;
    }
    
    character_nutrition_t* nutrition = get_player_nutrition(player_id);
    if (!nutrition || nutrient_type >= MAX_NUTRIENTS) {
        return 0.0f;
    }
    
    return nutrition->nutrient_levels[nutrient_type];
}

void food_get_nutrition_status(uint32_t player_id, bool* is_hungry, bool* is_thirsty,
                              bool* is_overfed, float* health_bonus, float* stamina_bonus) {
    if (!g_food_system.initialized) {
        return;
    }
    
    character_nutrition_t* nutrition = get_player_nutrition(player_id);
    if (!nutrition) {
        return;
    }
    
    if (is_hungry) *is_hungry = nutrition->is_hungry;
    if (is_thirsty) *is_thirsty = nutrition->is_thirsty;
    if (is_overfed) *is_overfed = nutrition->is_overfed;
    if (health_bonus) *health_bonus = nutrition->health_bonus;
    if (stamina_bonus) *stamina_bonus = nutrition->stamina_bonus;
}

const food_item_t* food_get_item(uint32_t item_id) {
    return get_food_item(item_id);
}

void food_get_all_items(const food_item_t** items, uint32_t* item_count) {
    if (!g_food_system.initialized || !items || !item_count) {
        return;
    }
    
    *items = g_food_system.food_items;
    *item_count = g_food_system.food_count;
}

// Static helper functions
static food_item_t* get_food_item(uint32_t item_id) {
    for (uint32_t i = 0; i < g_food_system.food_count; i++) {
        if (g_food_system.food_items[i].item_id == item_id) {
            return &g_food_system.food_items[i];
        }
    }
    return NULL;
}

static character_nutrition_t* get_player_nutrition(uint32_t player_id) {
    // Find existing player nutrition data
    for (uint32_t i = 0; i < g_food_system.player_count; i++) {
        if (g_food_system.player_nutrition[i].player_id == player_id) {
            return &g_food_system.player_nutrition[i];
        }
    }
    
    // Create new player nutrition data if space allows
    if (g_food_system.player_count < MAX_PLAYERS) {
        character_nutrition_t* nutrition = &g_food_system.player_nutrition[g_food_system.player_count];
        memset(nutrition, 0, sizeof(character_nutrition_t));
        nutrition->player_id = player_id;
        nutrition->metabolism_rate = 1.0f;
        nutrition->activity_multiplier = 1.0f;
        g_food_system.player_count++;
        return nutrition;
    }
    
    return NULL;
}

static void apply_nutrients(uint32_t player_id, const food_item_t* food) {
    character_nutrition_t* nutrition = get_player_nutrition(player_id);
    if (!nutrition) {
        return;
    }
    
    for (uint32_t i = 0; i < food->nutrient_count; i++) {
        const nutrient_value_t* nutrient = &food->nutrients[i];
        if (nutrient->type < MAX_NUTRIENTS) {
            nutrition->nutrient_levels[nutrient->type] += nutrient->amount * nutrient->quality;
            
            // Clamp nutrient levels to reasonable ranges
            if (nutrition->nutrient_levels[nutrient->type] > 100.0f) {
                nutrition->nutrient_levels[nutrient->type] = 100.0f;
            }
        }
    }
}

static void update_nutrition_status(uint32_t player_id, float delta_time) {
    character_nutrition_t* nutrition = get_player_nutrition(player_id);
    if (!nutrition) {
        return;
    }
    
    float metabolism_factor = nutrition->metabolism_rate * nutrition->activity_multiplier;
    
    // Deplete nutrients over time
    for (uint32_t i = 0; i < MAX_NUTRIENTS; i++) {
        float depletion_rate = 0.1f * metabolism_factor * delta_time;
        nutrition->nutrient_levels[i] -= depletion_rate;
        
        if (nutrition->nutrient_levels[i] < 0.0f) {
            nutrition->nutrient_levels[i] = 0.0f;
        }
    }
    
    // Check hunger and thirst status
    float total_nutrients = 0.0f;
    for (uint32_t i = 0; i < MAX_NUTRIENTS; i++) {
        total_nutrients += nutrition->nutrient_levels[i];
    }
    
    nutrition->is_hungry = total_nutrients < 30.0f;
    nutrition->is_thirsty = nutrition->nutrient_levels[NUTRIENT_WATER] < 20.0f;
    nutrition->is_overfed = total_nutrients > 90.0f;
}

static float calculate_nutrition_bonus(const character_nutrition_t* nutrition) {
    float total_score = 0.0f;
    
    // Calculate balanced nutrition bonus
    for (uint32_t i = 0; i < MAX_NUTRIENTS; i++) {
        float level = nutrition->nutrient_levels[i];
        
        // Optimal range is 40-60 for most nutrients
        if (level >= 40.0f && level <= 60.0f) {
            total_score += 1.0f;
        } else if (level >= 20.0f && level < 40.0f) {
            total_score += 0.5f;
        } else if (level > 60.0f && level <= 80.0f) {
            total_score += 0.7f;
        }
        // Too low or too high gives no bonus
    }
    
    // Convert to percentage bonus (0-25% max)
    return (total_score / MAX_NUTRIENTS) * 0.25f;
}

static float calculate_calories(const food_item_t* food) {
    float calories = 0.0f;
    
    for (uint32_t i = 0; i < food->nutrient_count; i++) {
        const nutrient_value_t* nutrient = &food->nutrients[i];
        
        switch (nutrient->type) {
            case NUTRIENT_PROTEIN:
                calories += nutrient->amount * 4.0f;  // 4 cal per gram
                break;
            case NUTRIENT_CARBS:
                calories += nutrient->amount * 4.0f;  // 4 cal per gram
                break;
            case NUTRIENT_FAT:
                calories += nutrient->amount * 9.0f;  // 9 cal per gram
                break;
            default:
                break;
        }
    }
    
    return calories;
}

static uint32_t get_current_time_ms(void) {
    // Placeholder - would typically use platform-specific time function
    static uint32_t counter = 0;
    return counter += 16;  // Simulate 60 FPS
}
