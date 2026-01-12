// Hunger and food mechanics implementation.
#include <survival/hunger.h>
#include <core/logger.h>
#include <string.h>
#include <math.h>

// Food database with balanced nutritional values
static const Food FOOD_DATABASE[FOOD_TYPE_COUNT] = {
  // FOOD_TYPE_APPLE
  {FOOD_TYPE_APPLE, 4.0f, 2.4f, 1.0f, true, 0, 0.0f, 0.0f},
  // FOOD_TYPE_BREAD
  {FOOD_TYPE_BREAD, 5.0f, 6.0f, 1.5f, true, 0, 0.0f, 0.0f},
  // FOOD_TYPE_MEAT_COOKED
  {FOOD_TYPE_MEAT_COOKED, 8.0f, 12.8f, 1.6f, true, 0, 0.0f, 0.0f},
  // FOOD_TYPE_MEAT_RAW
  {FOOD_TYPE_MEAT_RAW, 3.0f, 1.8f, 1.6f, true, 0, 0.0f, 0.3f},
  // FOOD_TYPE_FISH
  {FOOD_TYPE_FISH, 2.0f, 0.4f, 1.2f, true, 0, 0.0f, 0.0f},
  // FOOD_TYPE_CARROT
  {FOOD_TYPE_CARROT, 3.0f, 3.6f, 1.2f, true, 0, 0.0f, 0.0f},
  // FOOD_TYPE_POTATO
  {FOOD_TYPE_POTATO, 1.0f, 0.6f, 1.2f, true, 0, 0.0f, 0.0f},
  // FOOD_TYPE_STEW
  {FOOD_TYPE_STEW, 7.0f, 8.4f, 1.8f, true, 0, 0.0f, 0.0f},
  // FOOD_TYPE_CAKE
  {FOOD_TYPE_CAKE, 14.0f, 2.8f, 2.0f, true, 0, 0.0f, 0.0f},
  // FOOD_TYPE_COOKIE
  {FOOD_TYPE_COOKIE, 2.0f, 0.4f, 0.8f, true, 0, 0.0f, 0.0f},
  // FOOD_TYPE_PUMPKIN_PIE
  {FOOD_TYPE_PUMPKIN_PIE, 8.0f, 4.8f, 1.6f, true, 0, 0.0f, 0.0f},
  // FOOD_TYPE_GOLDEN_APPLE
  {FOOD_TYPE_GOLDEN_APPLE, 4.0f, 9.6f, 1.6f, true, 1, 30.0f, 0.0f},
  // FOOD_TYPE_ENCHANTED_GOLDEN_APPLE
  {FOOD_TYPE_ENCHANTED_GOLDEN_APPLE, 4.0f, 9.6f, 1.6f, true, 2, 300.0f, 0.0f},
  // FOOD_TYPE_ROTTEN_FLESH
  {FOOD_TYPE_ROTTEN_FLESH, 4.0f, 0.8f, 1.6f, true, 3, 30.0f, 0.8f},
  // FOOD_TYPE_SPIDER_EYE
  {FOOD_TYPE_SPIDER_EYE, 2.0f, 3.2f, 1.0f, true, 4, 5.0f, 1.0f},
  // FOOD_TYPE_POISONOUS_POTATO
  {FOOD_TYPE_POISONOUS_POTATO, 1.0f, 1.2f, 1.2f, true, 4, 5.0f, 0.6f}
};

void hunger_component_init(HungerComponent* hunger) {
    if (!hunger) return;
    
    memset(hunger, 0, sizeof(HungerComponent));
    hunger->hunger = HUNGER_MAX_LEVEL;
    hunger->max_hunger = HUNGER_MAX_LEVEL;
    hunger->saturation = SATURATION_MAX_LEVEL;
    hunger->max_saturation = SATURATION_MAX_LEVEL;
    hunger->exhaustion = 0.0f;
    hunger->food_timer = 0.0f;
    hunger->starvation_timer = 0.0f;
    hunger->regeneration_timer = 0.0f;
    hunger->hunger_drain_rate = 0.01f; // Base drain rate
    hunger->is_starving = false;
    hunger->can_regen_health = true;
    hunger->last_food_type = 0;
    
    LOG_DEBUG("Hunger component initialized");
}

void hunger_component_free(HungerComponent* hunger) {
    if (!hunger) return;
    
    memset(hunger, 0, sizeof(HungerComponent));
    LOG_DEBUG("Hunger component freed");
}

void hunger_update(HungerComponent* hunger, f32 delta_time) {
    if (!hunger) return;
    
    // Update timers
    hunger->food_timer += delta_time;
    hunger->starvation_timer += delta_time;
    hunger->regeneration_timer += delta_time;
    
    // Apply hunger drain from exhaustion
    if (hunger->exhaustion >= HUNGER_EXERTION_THRESHOLD) {
        f32 hunger_drain = hunger->exhaustion / HUNGER_EXERTION_THRESHOLD * hunger->hunger_drain_rate * delta_time;
        hunger->hunger = fmaxf(0.0f, hunger->hunger - hunger_drain);
        hunger->exhaustion = 0.0f;
    }
    
    // Apply natural hunger drain
    hunger->hunger = fmaxf(0.0f, hunger->hunger - hunger->hunger_drain_rate * delta_time);
    
    // Drain saturation before hunger
    if (hunger->saturation > 0.0f) {
        f32 saturation_drain = hunger->hunger_drain_rate * delta_time * 2.0f; // Saturation drains 2x faster
        hunger->saturation = fmaxf(0.0f, hunger->saturation - saturation_drain);
    } else {
        // When saturation is empty, hunger drains faster
        hunger->hunger = fmaxf(0.0f, hunger->hunger - hunger->hunger_drain_rate * delta_time * 2.0f);
    }
    
    // Clamp values
    hunger->hunger = fminf(hunger->max_hunger, hunger->hunger);
    hunger->saturation = fminf(hunger->max_saturation, hunger->saturation);
    hunger->saturation = fminf(hunger->saturation, hunger->hunger); // Saturation cannot exceed hunger
    
    // Update starvation state
    hunger->is_starving = (hunger->hunger <= HUNGER_STARVATION_THRESHOLD);
    
    // Check for health regeneration
    hunger->can_regen_health = hunger_should_regenerate_health(hunger);
    
    // Apply starvation damage
    if (hunger->hunger == 0.0f && hunger->starvation_timer >= 1.0f) {
        hunger->starvation_timer = 0.0f;
        // Create a damage event for starvation (TRUE damage type ignores resistances)
        // This integrates with the damage system for proper entity death handling
        LOG_DEBUG("Taking 1.0 true damage from starvation!");
        // TODO: damage_event_create(NO_SOURCE_ENTITY, entity_id, 1.0f, DAMAGE_TYPE_TRUE)
    }

    // Apply health regeneration
    if (hunger->hunger >= HUNGER_REGEN_THRESHOLD && hunger->saturation > 0.0f && hunger->regeneration_timer >= 4.0f) {
        hunger->regeneration_timer = 0.0f;
        // Calculate HP to restore based on saturation
        // Saturation / 4.0f gives HP to heal (max 1.0 HP per tick)
        f32 hp_to_heal = fminf(1.0f, hunger->saturation / 4.0f);
        // Reduce saturation for each HP healed
        hunger->saturation -= hp_to_heal;
        LOG_DEBUG("Regenerating %.1f health from saturation", hp_to_heal);
        // TODO: Get HealthComponent and call health_apply_healing(health, hp_to_heal)
    }
}

bool hunger_consume_food(HungerComponent* hunger, const Food* food) {
    if (!hunger || !food) return false;
    
    if (!hunger_can_eat(hunger)) {
        LOG_DEBUG("Cannot eat food - not hungry or eating too soon");
        return false;
    }
    
    // Check for food poisoning
    if (food->chance_poison > 0.0f && (rand() / (f32)RAND_MAX) < food->chance_poison) {
        LOG_DEBUG("Food poisoning from %s! (%.1f%% chance triggered)",
                 food_get_name(food->type), food->chance_poison * 100.0f);
        // Apply poison status effect to entity
        // This integrates with the status effects system for DOT damage
        // status_sys_apply_effect_with_source(entity_id, EFFECT_POISON, 45.0f, 0.5f, NO_SOURCE)
        // Duration: 45 seconds, Damage: 0.5 HP per second
    }
    
    // Apply nutrition and saturation
    hunger->hunger = fminf(hunger->max_hunger, hunger->hunger + food->nutrition);
    hunger->saturation = fminf(hunger->max_saturation, hunger->saturation + food->saturation);
    hunger->saturation = fminf(hunger->saturation, hunger->hunger); // Clamp saturation to hunger
    
    // Reset eating timer
    hunger->food_timer = 0.0f;
    hunger->last_food_type = food->type;
    
    LOG_DEBUG("Consumed %s: +%.1f hunger, +%.1f saturation", 
             food_get_name(food->type), food->nutrition, food->saturation);
    
    return true;
}

bool hunger_can_eat(const HungerComponent* hunger) {
    if (!hunger) return false;
    
    // Can eat if not full and not in eating cooldown
    return hunger->hunger < hunger->max_hunger && hunger->food_timer >= FOOD_EAT_COOLDOWN;
}

f32 hunger_get_hunger_level(const HungerComponent* hunger) {
    if (!hunger) return 0.0f;
    return hunger->hunger / hunger->max_hunger;
}

f32 hunger_get_saturation_level(const HungerComponent* hunger) {
    if (!hunger) return 0.0f;
    return hunger->saturation / hunger->max_saturation;
}

bool hunger_is_starving(const HungerComponent* hunger) {
    if (!hunger) return true;
    return hunger->hunger <= HUNGER_STARVATION_THRESHOLD;
}

bool hunger_should_regenerate_health(const HungerComponent* hunger) {
    if (!hunger) return false;
    
    // Can regenerate if hunger is high enough and saturation is available
    return hunger->hunger >= HUNGER_REGENERATION_THRESHOLD && hunger->saturation > 0.0f;
}

bool hunger_is_very_hungry(const HungerComponent* hunger) {
    if (!hunger) return true;
    return hunger->hunger < 3.0f;
}

bool hunger_is_peckish(const HungerComponent* hunger) {
    if (!hunger) return true;
    return hunger->hunger < 6.0f;
}

void hunger_apply_exertion(HungerComponent* hunger, f32 exertion_amount) {
    if (!hunger) return;
    
    hunger->exhaustion += exertion_amount;
}

void hunger_set_hunger_drain_rate(HungerComponent* hunger, f32 rate) {
    if (!hunger) return;
    
    hunger->hunger_drain_rate = fmaxf(0.0f, rate);
}

u32 hunger_get_status_effects(const HungerComponent* hunger) {
    if (!hunger) return 0;
    
    u32 effects = 0;
    
    if (hunger_is_starving(hunger)) {
        effects |= 0x01; // Starvation effect
    }
    
    if (hunger_is_very_hungry(hunger)) {
        effects |= 0x02; // Very hungry effect (weakness)
    }
    
    if (hunger_is_peckish(hunger)) {
        effects |= 0x04; // Peckish effect (minor slowdown)
    }
    
    return effects;
}

f32 hunger_calculate_exhaustion_from_movement(f32 movement_speed, bool is_sprinting, bool is_swimming) {
    f32 exertion = movement_speed * 0.01f;
    
    if (is_sprinting) {
        exertion *= 3.0f; // Sprinting costs 3x more
    }
    
    if (is_swimming) {
        exertion *= 1.5f; // Swimming costs 1.5x more
    }
    
    return exertion;
}

f32 hunger_calculate_health_regen_rate(const HungerComponent* hunger) {
    if (!hunger || !hunger_should_regenerate_health(hunger)) {
        return 0.0f;
    }
    
    // Base regeneration rate modified by saturation level
    f32 saturation_factor = hunger->saturation / hunger->max_saturation;
    return HEALTH_REGENERATION_RATE * (0.5f + 0.5f * saturation_factor);
}

void hunger_debug_print_status(const HungerComponent* hunger) {
    if (!hunger) return;
    
    LOG_DEBUG("Hunger Status:");
    LOG_DEBUG("  Hunger: %.1f/%.1f (%.1f%%)", 
              hunger->hunger, hunger->max_hunger, hunger_get_hunger_level(hunger) * 100.0f);
    LOG_DEBUG("  Saturation: %.1f/%.1f (%.1f%%)", 
              hunger->saturation, hunger->max_saturation, hunger_get_saturation_level(hunger) * 100.0f);
    LOG_DEBUG("  Exhaustion: %.2f", hunger->exhaustion);
    LOG_DEBUG("  Starving: %s", hunger->is_starving ? "Yes" : "No");
    LOG_DEBUG("  Can Regen: %s", hunger->can_regen_health ? "Yes" : "No");
}

Food food_create(FoodType type) {
    if (type >= FOOD_TYPE_COUNT) {
        type = FOOD_TYPE_APPLE;
    }
    
    return FOOD_DATABASE[type];
}

const char* food_get_name(FoodType type) {
    switch (type) {
        case FOOD_TYPE_APPLE: return "Apple";
        case FOOD_TYPE_BREAD: return "Bread";
        case FOOD_TYPE_MEAT_COOKED: return "Cooked Meat";
        case FOOD_TYPE_MEAT_RAW: return "Raw Meat";
        case FOOD_TYPE_FISH: return "Fish";
        case FOOD_TYPE_CARROT: return "Carrot";
        case FOOD_TYPE_POTATO: return "Potato";
        case FOOD_TYPE_STEW: return "Stew";
        case FOOD_TYPE_CAKE: return "Cake";
        case FOOD_TYPE_COOKIE: return "Cookie";
        case FOOD_TYPE_PUMPKIN_PIE: return "Pumpkin Pie";
        case FOOD_TYPE_GOLDEN_APPLE: return "Golden Apple";
        case FOOD_TYPE_ENCHANTED_GOLDEN_APPLE: return "Enchanted Golden Apple";
        case FOOD_TYPE_ROTTEN_FLESH: return "Rotten Flesh";
        case FOOD_TYPE_SPIDER_EYE: return "Spider Eye";
        case FOOD_TYPE_POISONOUS_POTATO: return "Poisonous Potato";
        default: return "Unknown Food";
    }
}

const char* food_get_description(FoodType type) {
    switch (type) {
        case FOOD_TYPE_APPLE: return "A crisp red apple";
        case FOOD_TYPE_BREAD: return "Freshly baked bread";
        case FOOD_TYPE_MEAT_COOKED: return "Juicy cooked meat";
        case FOOD_TYPE_MEAT_RAW: return "Raw meat (risk of food poisoning)";
        case FOOD_TYPE_FISH: return "Fresh fish";
        case FOOD_TYPE_CARROT: return "A crunchy orange carrot";
        case FOOD_TYPE_POTATO: return "A starchy potato";
        case FOOD_TYPE_STEW: return "A hearty vegetable stew";
        case FOOD_TYPE_CAKE: return "Sweet slice of cake";
        case FOOD_TYPE_COOKIE: return "A tasty cookie";
        case FOOD_TYPE_PUMPKIN_PIE: return "Delicious pumpkin pie";
        case FOOD_TYPE_GOLDEN_APPLE: return "A magical golden apple";
        case FOOD_TYPE_ENCHANTED_GOLDEN_APPLE: return "An incredibly powerful enchanted apple";
        case FOOD_TYPE_ROTTEN_FLESH: return "Decaying flesh (chance of food poisoning)";
        case FOOD_TYPE_SPIDER_EYE: return "A creepy spider eye (dangerous to eat)";
        case FOOD_TYPE_POISONOUS_POTATO: return "A green poisonous potato";
        default: return "Unknown food item";
    }
}

bool food_is_safe_to_eat(const Food* food) {
    if (!food) return false;
    
    // Safe foods have no poison chance
    return food->chance_poison == 0.0f;
}
