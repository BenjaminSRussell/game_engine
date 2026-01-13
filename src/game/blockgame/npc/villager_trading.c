// src/npc/villager_trading.c
//
// Purpose: Implementation of the villager trading system with professions,
// trades, and dynamic pricing based on reputation and demand.
//
#include "engine/include/core/logger.h"
#include <core/memory.h>
#include <inventory/inventory.h>
#include <inventory/item_registry.h>
#include <npc/villager_trading.h>
#include <stdlib.h>
#include <string.h>

// Initialize villager trading state
void villager_trading_init(VillagerTrading *trading,
                           VillagerProfession profession) {
  if (!trading)
    return;

  memset(trading, 0, sizeof(VillagerTrading));

  trading->profession = profession;
  trading->level = VILLAGE_LEVEL_NOVICE;
  trading->is_willing_to_trade = true;
  trading->reputation = 0.0f;
  trading->demand_multiplier = 1.0f;
  trading->trade_cooldown = 0;

  LOG_INFO("Villager trading initialized with profession: %d", profession);
}

// Initialize trade registry
void trade_registry_init(TradeRegistry *registry, u32 capacity) {
  if (!registry)
    return;

  registry->capacity = capacity;
  registry->total_trades = 0;
  registry->all_trades = CALLOC(capacity, sizeof(TradeOffer));

  LOG_INFO("Trade registry initialized with capacity: %u", capacity);
}

// Free trade registry
void trade_registry_free(TradeRegistry *registry) {
  if (!registry)
    return;

  if (registry->all_trades) {
    FREE(registry->all_trades);
    registry->all_trades = NULL;
  }

  registry->total_trades = 0;
  registry->capacity = 0;
}

// Get available trades for a villager
const TradeOffer *villager_get_trades(const VillagerTrading *trading,
                                      u32 *out_trade_count) {
  if (!trading || !out_trade_count)
    return NULL;

  *out_trade_count = trading->trade_count;
  return trading->trades;
}

// Check if player can perform a trade
bool villager_can_perform_trade(const VillagerTrading *trading, u32 trade_index,
                                const Inventory *player_inventory) {
  if (!trading || !player_inventory || trade_index >= trading->trade_count) {
    return false;
  }

  const TradeOffer *trade = &trading->trades[trade_index];

  // Check if trade is enabled
  if (!trade->is_enabled)
    return false;

  // Check max uses
  if (trade->max_uses != UINT32_MAX && trade->uses >= trade->max_uses) {
    return false;
  }

  // Check if player has required items
  u16 input_count = inventory_get_item_count((Inventory *)player_inventory,
                                             trade->input_item_id);
  if (input_count < trade->input_count) {
    return false;
  }

  // If trade costs emeralds, check for those too
  if (trade->emerald_cost > 0) {
    u16 emerald_count =
        inventory_get_item_count((Inventory *)player_inventory, ITEM_EMERALD);
    if (emerald_count < trade->emerald_cost) {
      return false;
    }
  }

  return true;
}

// Perform a trade transaction
bool villager_perform_trade(VillagerTrading *trading, u32 trade_index,
                            Inventory *player_inventory, u32 *out_experience) {
  if (!trading || !player_inventory || trade_index >= trading->trade_count) {
    return false;
  }

  if (!villager_can_perform_trade(trading, trade_index, player_inventory)) {
    LOG_WARN("Cannot perform trade %u", trade_index);
    return false;
  }

  TradeOffer *trade = &trading->trades[trade_index];

  // Remove input items from player
  inventory_remove_item(player_inventory, trade->input_item_id,
                        trade->input_count);

  // Remove emerald cost if applicable
  if (trade->emerald_cost > 0) {
    inventory_remove_item(player_inventory, ITEM_EMERALD, trade->emerald_cost);
  }

  // Add output items to player
  inventory_add_item(player_inventory, trade->output_item_id,
                     trade->output_count);

  // Award experience
  if (out_experience) {
    *out_experience = trade->experience_reward;
  }

  // Update trade state
  trade->uses++;
  trading->experience += trade->experience_reward;

  // Reset willingness if too many trades in a row
  if (trade->uses % 5 == 0) {
    trading->is_willing_to_trade = true; // Refresh willingness
  }

  // Update reputation (positive for completing trades)
  trading->reputation += 1.0f;

  LOG_INFO("Villager trade %u completed. Reputation: %.1f", trade_index,
           trading->reputation);

  return true;
}

// Unlock a trade through progression
void villager_unlock_trade(VillagerTrading *trading, u32 trade_index) {
  if (!trading || trade_index >= trading->trade_count)
    return;

  trading->trades[trade_index].is_enabled = true;

  LOG_DEBUG("Trade %u unlocked for profession %d", trade_index,
            trading->profession);
}

// Set villager reputation
void villager_set_reputation(VillagerTrading *trading, f32 reputation_change) {
  if (!trading)
    return;

  trading->reputation += reputation_change;

  // Clamp reputation to -30 to 30
  if (trading->reputation > 30.0f)
    trading->reputation = 30.0f;
  if (trading->reputation < -30.0f)
    trading->reputation = -30.0f;

  // Update pricing based on reputation
  if (trading->reputation > 0.0f) {
    // Positive reputation = lower prices (multiplier < 1.0)
    trading->demand_multiplier = 1.0f - (trading->reputation / 30.0f) * 0.25f;
  } else if (trading->reputation < 0.0f) {
    // Negative reputation = higher prices (multiplier > 1.0)
    trading->demand_multiplier = 1.0f + (-trading->reputation / 30.0f) * 0.25f;
  } else {
    trading->demand_multiplier = 1.0f;
  }

  LOG_DEBUG("Villager reputation changed. New reputation: %.1f, demand "
            "multiplier: %.2f",
            trading->reputation, trading->demand_multiplier);
}

// Increase demand for an item
void villager_increase_demand(VillagerTrading *trading, u32 item_id,
                              f32 amount) {
  if (!trading)
    return;

  // Find the first trade that outputs this item and increase its price
  for (u32 i = 0; i < trading->trade_count; i++) {
    if (trading->trades[i].output_item_id == item_id) {
      trading->trades[i].price_multiplier += amount;
      if (trading->trades[i].price_multiplier > 2.0f) {
        trading->trades[i].price_multiplier = 2.0f; // Cap at 2x
      }
      LOG_DEBUG("Item %u demand increased. Price multiplier: %.2f", item_id,
                trading->trades[i].price_multiplier);
      break;
    }
  }
}

// Level up villager
bool villager_level_up(VillagerTrading *trading) {
  if (!trading)
    return false;

  // Check if enough experience
  u32 xp_needed = trading->level * 100; // 100 XP per level
  if (trading->experience < xp_needed) {
    return false;
  }

  // Level up
  trading->experience -= xp_needed;
  trading->level++;

  // Unlock new trades at each level
  for (u32 i = 0; i < trading->trade_count; i++) {
    u32 required_level = (i / 4) + 1; // Every 4 trades requires 1 level
    if (trading->level >= required_level) {
      villager_unlock_trade(trading, i);
    }
  }

  LOG_INFO("Villager leveled up to level %d", trading->level);

  return true;
}

// Get profession name
const char *villager_get_profession_name(VillagerProfession profession) {
  const char *names[] = {"Unemployed", "Armorer",         "Cartographer",
                         "Cleric",     "Farmer",          "Fisherman",
                         "Fletcher",   "Librarian",       "Mason",
                         "Shepherd",   "Toolsmith",       "Weaponsmith",
                         "Clown",      "Wandering Trader"};

  if (profession < PROFESSION_COUNT) {
    return names[profession];
  }

  return "Unknown";
}

// Get trade price with modifiers
u32 villager_get_trade_cost(const VillagerTrading *trading,
                            const TradeOffer *trade) {
  if (!trading || !trade)
    return 0;

  f32 base_cost = (f32)trade->emerald_cost;

  // Apply reputation multiplier
  f32 cost = base_cost * trading->demand_multiplier;

  // Apply demand multiplier
  cost *= trade->price_multiplier;

  return (u32)cost;
}

// Initialize default trades for all professions
void trade_registry_init_defaults(TradeRegistry *registry) {
  if (!registry)
    return;

  // This would be a large function containing all default trades
  // For brevity, showing the structure for a few professions

  // === FARMER ===
  TradeOffer farmer_trades[] = {// Buy wheat, sell emeralds
                                {.input_item_id = ITEM_WHEAT,
                                 .input_count = 20,
                                 .output_item_id = ITEM_EMERALD,
                                 .output_count = 1,
                                 .emerald_cost = 0,
                                 .experience_reward = 10,
                                 .is_enabled = true,
                                 .is_treasure = false,
                                 .max_uses = UINT32_MAX,
                                 .uses = 0,
                                 .price_multiplier = 1.0f},
                                // Buy carrot, sell emeralds
                                {.input_item_id = ITEM_CARROT,
                                 .input_count = 22,
                                 .output_item_id = ITEM_EMERALD,
                                 .output_count = 1,
                                 .emerald_cost = 0,
                                 .experience_reward = 10,
                                 .is_enabled = true,
                                 .is_treasure = false,
                                 .max_uses = UINT32_MAX,
                                 .uses = 0,
                                 .price_multiplier = 1.0f},
                                // Buy potato, sell emeralds
                                {.input_item_id = ITEM_POTATO,
                                 .input_count = 26,
                                 .output_item_id = ITEM_EMERALD,
                                 .output_count = 1,
                                 .emerald_cost = 0,
                                 .experience_reward = 10,
                                 .is_enabled = true,
                                 .is_treasure = false,
                                 .max_uses = UINT32_MAX,
                                 .uses = 0,
                                 .price_multiplier = 1.0f},
                                // Sell wheat
                                {.input_item_id = 0, // No input
                                 .input_count = 0,
                                 .output_item_id = ITEM_WHEAT,
                                 .output_count = 1,
                                 .emerald_cost = 1,
                                 .experience_reward = 10,
                                 .is_enabled = false, // Unlocks at higher level
                                 .is_treasure = false,
                                 .max_uses = UINT32_MAX,
                                 .uses = 0,
                                 .price_multiplier = 1.0f}};

  // === LIBRARIAN (most valuable - sells enchanted books) ===
  TradeOffer librarian_trades[] = {
      // Buys paper
      {.input_item_id = ITEM_BOOK,
       .input_count = 4,
       .output_item_id = ITEM_EMERALD,
       .output_count = 1,
       .emerald_cost = 0,
       .experience_reward = 20,
       .is_enabled = true,
       .is_treasure = false,
       .max_uses = UINT32_MAX,
       .uses = 0,
       .price_multiplier = 1.0f},
      // Sells enchanted books (treasure - high cost)
      {.input_item_id = 0,
       .input_count = 0,
       .output_item_id = ITEM_ENCHANTED_BOOK,
       .output_count = 1,
       .emerald_cost = 5,
       .experience_reward = 30,
       .is_enabled = false, // Treasure trade
       .is_treasure = true,
       .max_uses = 1, // Limited use
       .uses = 0,
       .price_multiplier = 1.0f}};

  // === CLERIC ===
  TradeOffer cleric_trades[] = {// Buy redstone
                                {.input_item_id = ITEM_REDSTONE,
                                 .input_count = 15,
                                 .output_item_id = ITEM_EMERALD,
                                 .output_count = 1,
                                 .emerald_cost = 0,
                                 .experience_reward = 10,
                                 .is_enabled = true,
                                 .is_treasure = false,
                                 .max_uses = UINT32_MAX,
                                 .uses = 0,
                                 .price_multiplier = 1.0f},
                                // Sell glowstone dust
                                {.input_item_id = 0,
                                 .input_count = 0,
                                 .output_item_id = ITEM_GLOWSTONE_DUST,
                                 .output_count = 1,
                                 .emerald_cost = 1,
                                 .experience_reward = 20,
                                 .is_enabled = true,
                                 .is_treasure = false,
                                 .max_uses = UINT32_MAX,
                                 .uses = 0,
                                 .price_multiplier = 1.0f}};

  LOG_INFO("Trade registry defaults initialized");
}
