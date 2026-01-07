// include/npc/villager_trading.h
//
// Purpose: Defines the villager trading system including professions, trades,
// and economy mechanics. Villagers have professions and offer trades based on
// their role (librarian, blacksmith, farmer, etc.).
//
// Public APIs:
// - `VillagerProfession`: Enumeration of 14 profession types
// - `TradeOffer`: Individual trade (input items -> output items + emerald cost)
// - `VillagerTrading`: Trading state for an individual villager
// - `TradeRegistry`: Global registry of all trade offers
// - `villager_trading_init`: Initialize trading for a villager
// - `villager_get_trades`: Get available trades for a villager
// - `villager_perform_trade`: Execute a trade transaction
// - `villager_unlock_trade`: Unlock a trade through progression
//
#ifndef VILLAGER_TRADING_H
#define VILLAGER_TRADING_H

#include "../game_common.h"
#include "../inventory/inventory.h"

// Villager professions
typedef enum {
    PROFESSION_UNEMPLOYED = 0,
    PROFESSION_ARMORER = 1,
    PROFESSION_CARTOGRAPHER = 2,
    PROFESSION_CLERIC = 3,
    PROFESSION_FARMER = 4,
    PROFESSION_FISHERMAN = 5,
    PROFESSION_FLETCHER = 6,
    PROFESSION_LIBRARIAN = 7,
    PROFESSION_MASONS = 8,
    PROFESSION_SHEPHERD = 9,
    PROFESSION_TOOLSMITH = 10,
    PROFESSION_WEAPONSMITH = 11,
    PROFESSION_CLOWN = 12,
    PROFESSION_WANDERING_TRADER = 13,
    PROFESSION_COUNT = 14
} VillagerProfession;

// Villager level (affects trades unlocked)
typedef enum {
    VILLAGE_LEVEL_NOVICE = 1,      // First trades only
    VILLAGE_LEVEL_APPRENTICE = 2,  // Additional trades unlocked
    VILLAGE_LEVEL_JOURNEYMAN = 3,  // More trades available
    VILLAGE_LEVEL_EXPERT = 4,      // Most trades available
    VILLAGE_LEVEL_MASTER = 5       // All trades available
} VillagerLevel;

// Single trade offer
typedef struct {
    u32 input_item_id;      // Item player gives
    u16 input_count;        // Quantity needed
    u32 output_item_id;     // Item player receives
    u16 output_count;       // Quantity received
    u32 emerald_cost;       // Emeralds villager wants (0 if input is currency)
    u32 experience_reward;  // XP player gets on trade
    bool is_enabled;        // Whether trade is currently available
    bool is_treasure;       // Rare trade (only from treasure categories)
    u32 max_uses;          // Max times this trade can be used (-1 for infinite)
    u32 uses;              // Current usage count
    f32 price_multiplier;  // Dynamic pricing based on demand
} TradeOffer;

// Villager trading state
typedef struct {
    VillagerProfession profession;
    VillagerLevel level;
    TradeOffer trades[16];      // Up to 16 trades per profession
    u32 trade_count;
    u32 experience;             // Towards next level
    bool is_willing_to_trade;   // Based on recent trades/mood
    f32 reputation;             // Player reputation with villager (-30 to 30)
    f32 demand_multiplier;      // Affects pricing
    u32 last_trade_time;        // Timestamp of last trade
    u32 trade_cooldown;         // Time before can trade again (in seconds)
} VillagerTrading;

// Global trade registry
typedef struct {
    TradeOffer *all_trades;
    u32 total_trades;
    u32 capacity;
} TradeRegistry;

// Initialize villager trading state
void villager_trading_init(VillagerTrading *trading, VillagerProfession profession);

// Initialize trade registry
void trade_registry_init(TradeRegistry *registry, u32 capacity);

// Free trade registry
void trade_registry_free(TradeRegistry *registry);

// Get available trades for a villager
const TradeOffer *villager_get_trades(const VillagerTrading *trading,
                                      u32 *out_trade_count);

// Perform a trade transaction
bool villager_perform_trade(VillagerTrading *trading, u32 trade_index,
                           Inventory *player_inventory, u32 *out_experience);

// Check if player can perform a trade
bool villager_can_perform_trade(const VillagerTrading *trading, u32 trade_index,
                               const Inventory *player_inventory);

// Unlock a trade through progression
void villager_unlock_trade(VillagerTrading *trading, u32 trade_index);

// Set villager reputation (affects prices and trading)
void villager_set_reputation(VillagerTrading *trading, f32 reputation_change);

// Increase demand for an item (affects pricing)
void villager_increase_demand(VillagerTrading *trading, u32 item_id, f32 amount);

// Level up villager
bool villager_level_up(VillagerTrading *trading);

// Get profession name
const char *villager_get_profession_name(VillagerProfession profession);

// Get trade price (with modifiers)
u32 villager_get_trade_cost(const VillagerTrading *trading, const TradeOffer *trade);

// Initialize default trades for all professions
void trade_registry_init_defaults(TradeRegistry *registry);

#endif // VILLAGER_TRADING_H
