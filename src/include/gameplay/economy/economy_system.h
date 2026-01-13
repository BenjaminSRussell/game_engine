#pragma once

#include "core/core.h"
#include "ecs/ecs.h"
#include "math/vec3.h"

//  COMPLETED: Currency system with different denominations of coins
typedef enum {
    CURRENCY_COPPER = 0,
    CURRENCY_SILVER,
    CURRENCY_GOLD,
    CURRENCY_PLATINUM,
    CURRENCY_DIAMOND,
    CURRENCY_COUNT
} CurrencyType;

//  COMPLETED: Currency data structure
typedef struct {
    u64 amounts[CURRENCY_COUNT];
    char display_names[CURRENCY_COUNT][32];
    u64 values[CURRENCY_COUNT]; // Relative values (e.g., 100 copper = 1 silver)
} Currency;

//  COMPLETED: Trade system for player-to-player and player-to-NPC trading
typedef struct {
    EntityID trader1_id;
    EntityID trader2_id;
    Currency currency_offered;
    Currency currency_requested;
    bool accepted;
    f64 trade_time;
} Trade;

//  COMPLETED: Global market with fluctuating prices based on supply and demand
typedef struct {
    char item_name[64];
    f64 base_price;
    f64 current_price;
    f64 supply;
    f64 demand;
    f64 price_history[100]; // Last 100 price updates
    u32 history_index;
} MarketItem;

//  COMPLETED: Business ownership system for shops and farms
typedef struct {
    EntityID owner_id;
    char business_name[64];
    char business_type[32]; // "shop", "farm", "mine", etc.
    Currency daily_revenue;
    Currency daily_expenses;
    f64 profitability;
    u32 employee_count;
    bool is_active;
} Business;

//  COMPLETED: Tax and economic regulations system
typedef struct {
    f32 sales_tax_rate;        // Percentage tax on sales
    f32 income_tax_rate;        // Percentage tax on income
    f32 property_tax_rate;       // Tax on property ownership
    f32 import_tariff_rate;      // Tariff on imported goods
    f32 export_subsidy_rate;    // Subsidy for exported goods
    bool tax_enabled;
} TaxSystem;

//  COMPLETED: Economy manager with all economic systems
typedef struct {
    // Currency system
    Currency player_currency;
    Currency npc_average_currency;
    
    // Trading system
    Trade active_trades[32];
    u32 trade_count;
    
    // Market system
    MarketItem market_items[256];
    u32 market_item_count;
    
    // Business system
    Business businesses[128];
    u32 business_count;
    
    // Tax system
    TaxSystem tax_system;
    
    // Economic statistics
    f64 total_money_supply;
    f64 average_player_wealth;
    f64 gdp_growth_rate;
    
    bool initialized;
} EconomyManager;

//  COMPLETED: Initialize economy system
bool economy_init(void);

//  COMPLETED: Add currency to player inventory
void economy_add_currency(EntityID player_id, CurrencyType type, u64 amount);

//  COMPLETED: Remove currency from player inventory
bool economy_remove_currency(EntityID player_id, CurrencyType type, u64 amount);

//  COMPLETED: Get player currency balance
u64 economy_get_currency_balance(EntityID player_id, CurrencyType type);

//  COMPLETED: Convert between currency denominations
u64 economy_convert_currency(u64 amount, CurrencyType from_type, CurrencyType to_type);

//  COMPLETED: Initiate trade between two entities
bool economy_initiate_trade(EntityID trader1_id, EntityID trader2_id, 
                          const Currency* offer, const Currency* request);

//  COMPLETED: Accept or reject a trade
void economy_respond_to_trade(u32 trade_id, bool accept);

//  COMPLETED: Update market prices based on supply and demand
void economy_update_market_prices(void);

//  COMPLETED: Get current market price for an item
f64 economy_get_market_price(const char* item_name);

//  COMPLETED: Create a new business
bool economy_create_business(EntityID owner_id, const char* business_name, 
                        const char* business_type);

//  COMPLETED: Update business daily revenue/expenses
void economy_update_business_finances(EntityID business_id, Currency revenue, 
                                 Currency expenses);

//  COMPLETED: Calculate and collect taxes
void economy_process_taxes(void);

//  COMPLETED: Get economy statistics
void economy_get_statistics(f64* total_supply, f64* average_wealth, 
                        f64* gdp_growth);

//  COMPLETED: Save/load economy state
bool economy_save_state(const char* filename);
bool economy_load_state(const char* filename);

//  COMPLETED: Cleanup economy system
void economy_cleanup(void);

//  COMPLETED: Check if economy is initialized
bool economy_is_initialized(void);

//  COMPLETED: Currency utility functions
const char* economy_get_currency_name(CurrencyType type);
const char* economy_get_currency_symbol(CurrencyType type);
u64 economy_get_currency_value(CurrencyType type);

//  COMPLETED: Market utility functions
void economy_add_market_item(const char* item_name, f64 base_price);
void economy_set_supply_demand(const char* item_name, f64 supply, f64 demand);

//  COMPLETED: Trade utility functions
Trade* economy_get_active_trade(u32 trade_id);
u32 economy_get_trade_count(void);
void economy_cancel_trade(u32 trade_id);

//  COMPLETED: Business utility functions
Business* economy_get_business(EntityID business_id);
u32 economy_get_business_count(void);
void economy_close_business(EntityID business_id);

//  COMPLETED: Tax utility functions
void economy_set_tax_rates(f32 sales_tax, f32 income_tax, f32 property_tax);
void economy_enable_taxes(bool enabled);
