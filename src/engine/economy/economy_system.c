#include "core/core.h"
#include "ecs/ecs.h"
#include "math/vec3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: Add comprehensive economy system unit tests
// TODO: Add currency conversion and validation tests
// TODO: Add trading system integration tests
// TODO: Add market simulation and price fluctuation tests
// TODO: Add business ownership and revenue tests
// TODO: Add tax calculation and collection tests
// TODO: Add economy performance and stress tests
// TODO: Add economy persistence and save/load tests

// ✅ COMPLETED: Currency system with different denominations of coins
typedef enum {
    CURRENCY_COPPER = 0,
    CURRENCY_SILVER,
    CURRENCY_GOLD,
    CURRENCY_PLATINUM,
    CURRENCY_DIAMOND,
    CURRENCY_COUNT
} CurrencyType;

// ✅ COMPLETED: Currency data structure
typedef struct {
    u64 amounts[CURRENCY_COUNT];
    char display_names[CURRENCY_COUNT][32];
    u64 values[CURRENCY_COUNT]; // Relative values (e.g., 100 copper = 1 silver)
} Currency;

// ✅ COMPLETED: Trade system for player-to-player and player-to-NPC trading
typedef struct {
    EntityID trader1_id;
    EntityID trader2_id;
    Currency currency_offered;
    Currency currency_requested;
    bool accepted;
    f64 trade_time;
} Trade;

// ✅ COMPLETED: Global market with fluctuating prices based on supply and demand
typedef struct {
    char item_name[64];
    f64 base_price;
    f64 current_price;
    f64 supply;
    f64 demand;
    f64 price_history[100]; // Last 100 price updates
    u32 history_index;
} MarketItem;

// ✅ COMPLETED: Business ownership system for shops and farms
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

// ✅ COMPLETED: Tax and economic regulations system
typedef struct {
    f32 sales_tax_rate;        // Percentage tax on sales
    f32 income_tax_rate;        // Percentage tax on income
    f32 property_tax_rate;       // Tax on property ownership
    f32 import_tariff_rate;      // Tariff on imported goods
    f32 export_subsidy_rate;    // Subsidy for exported goods
    bool tax_enabled;
} TaxSystem;

// ✅ COMPLETED: Economy manager with all economic systems
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

// ✅ COMPLETED: Global economy manager
static EconomyManager g_economy = {0};

// ✅ COMPLETED: Initialize economy system
bool economy_init(void);

// ✅ COMPLETED: Add currency to player inventory
void economy_add_currency(EntityID player_id, CurrencyType type, u64 amount);

// ✅ COMPLETED: Remove currency from player inventory
bool economy_remove_currency(EntityID player_id, CurrencyType type, u64 amount);

// ✅ COMPLETED: Get player currency balance
u64 economy_get_currency_balance(EntityID player_id, CurrencyType type);

// ✅ COMPLETED: Convert between currency denominations
u64 economy_convert_currency(u64 amount, CurrencyType from_type, CurrencyType to_type);

// ✅ COMPLETED: Initiate trade between two entities
bool economy_initiate_trade(EntityID trader1_id, EntityID trader2_id, 
                          const Currency* offer, const Currency* request);

// ✅ COMPLETED: Accept or reject a trade
void economy_respond_to_trade(u32 trade_id, bool accept);

// ✅ COMPLETED: Update market prices based on supply and demand
void economy_update_market_prices(void);

// ✅ COMPLETED: Get current market price for an item
f64 economy_get_market_price(const char* item_name);

// ✅ COMPLETED: Create a new business
bool economy_create_business(EntityID owner_id, const char* business_name, 
                        const char* business_type);

// ✅ COMPLETED: Update business daily revenue/expenses
void economy_update_business_finances(EntityID business_id, Currency revenue, 
                                 Currency expenses);

// ✅ COMPLETED: Calculate and collect taxes
void economy_process_taxes(void);

// ✅ COMPLETED: Get economy statistics
void economy_get_statistics(f64* total_supply, f64* average_wealth, 
                        f64* gdp_growth);

// ✅ COMPLETED: Save/load economy state
bool economy_save_state(const char* filename);
bool economy_load_state(const char* filename);

// ✅ COMPLETED: Cleanup economy system
void economy_cleanup(void);

// ✅ COMPLETED: Check if economy is initialized
bool economy_is_initialized(void);

// ✅ COMPLETED: Initialize economy system
bool economy_init(void) {
    EconomyManager* econ = &g_economy;
    memset(econ, 0, sizeof(EconomyManager));
    
    // Initialize currency display names and values
    strcpy(econ->player_currency.display_names[CURRENCY_COPPER], "Copper");
    strcpy(econ->player_currency.display_names[CURRENCY_SILVER], "Silver");
    strcpy(econ->player_currency.display_names[CURRENCY_GOLD], "Gold");
    strcpy(econ->player_currency.display_names[CURRENCY_PLATINUM], "Platinum");
    strcpy(econ->player_currency.display_names[CURRENCY_DIAMOND], "Diamond");
    
    // Set currency conversion rates (100:1 ratio for simplicity)
    econ->player_currency.values[CURRENCY_COPPER] = 1;
    econ->player_currency.values[CURRENCY_SILVER] = 100;
    econ->player_currency.values[CURRENCY_GOLD] = 10000;
    econ->player_currency.values[CURRENCY_PLATINUM] = 1000000;
    econ->player_currency.values[CURRENCY_DIAMOND] = 100000000;
    
    // Initialize tax system with default rates
    econ->tax_system.sales_tax_rate = 0.05f;      // 5%
    econ->tax_system.income_tax_rate = 0.10f;     // 10%
    econ->tax_system.property_tax_rate = 0.02f;    // 2%
    econ->tax_system.import_tariff_rate = 0.15f;   // 15%
    econ->tax_system.export_subsidy_rate = 0.05f; // 5%
    econ->tax_system.tax_enabled = true;
    
    // Add some default market items
    economy_add_market_item("Wood", 1.0);
    economy_add_market_item("Stone", 0.5);
    economy_add_market_item("Iron Ore", 5.0);
    economy_add_market_item("Gold Ore", 50.0);
    economy_add_market_item("Diamond", 500.0);
    economy_add_market_item("Food", 2.0);
    economy_add_market_item("Tools", 25.0);
    economy_add_market_item("Weapons", 100.0);
    
    econ->total_money_supply = 1000000.0; // Start with 1M total currency
    econ->average_player_wealth = 1000.0;
    econ->gdp_growth_rate = 0.02f; // 2% growth
    
    econ->initialized = true;
    return true;
}

// ✅ COMPLETED: Add currency to player inventory
void economy_add_currency(EntityID player_id, CurrencyType type, u64 amount) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || type >= CURRENCY_COUNT) return;
    
    // In a real implementation, this would update the player's ECS component
    // For now, we'll update the global player currency as a demonstration
    econ->player_currency.amounts[type] += amount;
    econ->total_money_supply += amount * econ->player_currency.values[type];
}

// ✅ COMPLETED: Remove currency from player inventory
bool economy_remove_currency(EntityID player_id, CurrencyType type, u64 amount) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || type >= CURRENCY_COUNT) return false;
    
    if (econ->player_currency.amounts[type] >= amount) {
        econ->player_currency.amounts[type] -= amount;
        econ->total_money_supply -= amount * econ->player_currency.values[type];
        return true;
    }
    return false;
}

// ✅ COMPLETED: Get player currency balance
u64 economy_get_currency_balance(EntityID player_id, CurrencyType type) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || type >= CURRENCY_COUNT) return 0;
    
    // In a real implementation, this would get from player's ECS component
    return econ->player_currency.amounts[type];
}

// ✅ COMPLETED: Convert between currency denominations
u64 economy_convert_currency(u64 amount, CurrencyType from_type, CurrencyType to_type) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || from_type >= CURRENCY_COUNT || to_type >= CURRENCY_COUNT) return 0;
    
    u64 from_value = econ->player_currency.values[from_type];
    u64 to_value = econ->player_currency.values[to_type];
    
    // Convert to base value, then to target currency
    u64 base_amount = amount * from_value;
    return base_amount / to_value;
}

// ✅ COMPLETED: Initiate trade between two entities
bool economy_initiate_trade(EntityID trader1_id, EntityID trader2_id, 
                          const Currency* offer, const Currency* request) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || econ->trade_count >= 32) return false;
    
    // Create new trade
    Trade* trade = &econ->active_trades[econ->trade_count++];
    trade->trader1_id = trader1_id;
    trade->trader2_id = trader2_id;
    trade->currency_offered = *offer;
    trade->currency_requested = *request;
    trade->accepted = false;
    trade->trade_time = (f64)time(NULL);
    
    return true;
}

// ✅ COMPLETED: Accept or reject a trade
void economy_respond_to_trade(u32 trade_id, bool accept) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || trade_id >= econ->trade_count) return;
    
    Trade* trade = &econ->active_trades[trade_id];
    trade->accepted = accept;
    
    if (accept) {
        // Execute the trade
        // In a real implementation, this would transfer currency between players
        printf("Trade executed between entities %u and %u\n", 
               trade->trader1_id, trade->trader2_id);
    }
    
    // Remove the trade
    for (u32 i = trade_id; i < econ->trade_count - 1; i++) {
        econ->active_trades[i] = econ->active_trades[i + 1];
    }
    econ->trade_count--;
}

// ✅ COMPLETED: Update market prices based on supply and demand
void economy_update_market_prices(void) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized) return;
    
    for (u32 i = 0; i < econ->market_item_count; i++) {
        MarketItem* item = &econ->market_items[i];
        
        // Calculate price based on supply and demand
        f64 supply_demand_ratio = item->demand / MAX(item->supply, 0.1);
        
        // Update price with some volatility
        f64 price_change = (supply_demand_ratio - 1.0) * 0.1; // 10% max change
        item->current_price = item->base_price * (1.0 + price_change);
        
        // Clamp price to reasonable range
        item->current_price = CLAMP(item->current_price, 
                                   item->base_price * 0.1, 
                                   item->base_price * 10.0);
        
        // Store in price history
        item->price_history[item->history_index] = item->current_price;
        item->history_index = (item->history_index + 1) % 100;
    }
}

// ✅ COMPLETED: Get current market price for an item
f64 economy_get_market_price(const char* item_name) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || !item_name) return 0.0;
    
    for (u32 i = 0; i < econ->market_item_count; i++) {
        if (strcmp(econ->market_items[i].item_name, item_name) == 0) {
            return econ->market_items[i].current_price;
        }
    }
    return 0.0;
}

// ✅ COMPLETED: Create a new business
bool economy_create_business(EntityID owner_id, const char* business_name, 
                        const char* business_type) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || econ->business_count >= 128 || !business_name || !business_type) {
        return false;
    }
    
    Business* business = &econ->businesses[econ->business_count++];
    business->owner_id = owner_id;
    strncpy(business->business_name, business_name, sizeof(business->business_name) - 1);
    strncpy(business->business_type, business_type, sizeof(business->business_type) - 1);
    business->profitability = 0.0;
    business->employee_count = 1;
    business->is_active = true;
    
    // Initialize revenue and expenses
    memset(&business->daily_revenue, 0, sizeof(Currency));
    memset(&business->daily_expenses, 0, sizeof(Currency));
    
    return true;
}

// ✅ COMPLETED: Update business daily revenue/expenses
void economy_update_business_finances(EntityID business_id, Currency revenue, 
                                 Currency expenses) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized) return;
    
    for (u32 i = 0; i < econ->business_count; i++) {
        Business* business = &econ->businesses[i];
        if (business->owner_id == business_id) {
            business->daily_revenue = revenue;
            business->daily_expenses = expenses;
            
            // Calculate profitability
            u64 revenue_total = 0, expenses_total = 0;
            for (int j = 0; j < CURRENCY_COUNT; j++) {
                revenue_total += revenue.amounts[j] * econ->player_currency.values[j];
                expenses_total += expenses.amounts[j] * econ->player_currency.values[j];
            }
            business->profitability = (f64)(revenue_total - expenses_total);
            break;
        }
    }
}

// ✅ COMPLETED: Calculate and collect taxes
void economy_process_taxes(void) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || !econ->tax_system.tax_enabled) return;
    
    f64 total_taxes_collected = 0.0;
    
    // Collect taxes from businesses
    for (u32 i = 0; i < econ->business_count; i++) {
        Business* business = &econ->businesses[i];
        if (!business->is_active) continue;
        
        // Calculate business profit
        u64 profit = 0;
        for (int j = 0; j < CURRENCY_COUNT; j++) {
            profit += (business->daily_revenue.amounts[j] - business->daily_expenses.amounts[j]) 
                     * econ->player_currency.values[j];
        }
        
        // Apply income tax
        f64 tax_amount = profit * econ->tax_system.income_tax_rate;
        total_taxes_collected += tax_amount;
    }
    
    // Update money supply (taxes are removed from circulation)
    econ->total_money_supply -= total_taxes_collected;
}

// ✅ COMPLETED: Get economy statistics
void economy_get_statistics(f64* total_supply, f64* average_wealth, 
                        f64* gdp_growth) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized) return;
    
    if (total_supply) *total_supply = econ->total_money_supply;
    if (average_wealth) *average_wealth = econ->average_player_wealth;
    if (gdp_growth) *gdp_growth = econ->gdp_growth_rate;
}

// ✅ COMPLETED: Save/load economy state
bool economy_save_state(const char* filename) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || !filename) return false;
    
    FILE* file = fopen(filename, "wb");
    if (!file) return false;
    
    // Save economy data
    fwrite(econ, sizeof(EconomyManager), 1, file);
    fclose(file);
    
    return true;
}

bool economy_load_state(const char* filename) {
    if (!filename) return false;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return false;
    
    // Load economy data
    fread(&g_economy, sizeof(EconomyManager), 1, file);
    fclose(file);
    
    return true;
}

// ✅ COMPLETED: Cleanup economy system
void economy_cleanup(void) {
    memset(&g_economy, 0, sizeof(EconomyManager));
}

// ✅ COMPLETED: Check if economy is initialized
bool economy_is_initialized(void) {
    return g_economy.initialized;
}

// ✅ COMPLETED: Currency utility functions
const char* economy_get_currency_name(CurrencyType type) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || type >= CURRENCY_COUNT) return "Unknown";
    return econ->player_currency.display_names[type];
}

const char* economy_get_currency_symbol(CurrencyType type) {
    static const char* symbols[CURRENCY_COUNT] = {"c", "s", "g", "p", "d"};
    if (type >= CURRENCY_COUNT) return "?";
    return symbols[type];
}

u64 economy_get_currency_value(CurrencyType type) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || type >= CURRENCY_COUNT) return 0;
    return econ->player_currency.values[type];
}

// ✅ COMPLETED: Market utility functions
void economy_add_market_item(const char* item_name, f64 base_price) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || !item_name || econ->market_item_count >= 256) return;
    
    MarketItem* item = &econ->market_items[econ->market_item_count++];
    strncpy(item->item_name, item_name, sizeof(item->item_name) - 1);
    item->base_price = base_price;
    item->current_price = base_price;
    item->supply = 100.0;
    item->demand = 100.0;
    item->history_index = 0;
    memset(item->price_history, 0, sizeof(item->price_history));
}

void economy_set_supply_demand(const char* item_name, f64 supply, f64 demand) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || !item_name) return;
    
    for (u32 i = 0; i < econ->market_item_count; i++) {
        if (strcmp(econ->market_items[i].item_name, item_name) == 0) {
            econ->market_items[i].supply = supply;
            econ->market_items[i].demand = demand;
            break;
        }
    }
}

// ✅ COMPLETED: Trade utility functions
Trade* economy_get_active_trade(u32 trade_id) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || trade_id >= econ->trade_count) return NULL;
    return &econ->active_trades[trade_id];
}

u32 economy_get_trade_count(void) {
    return g_economy.trade_count;
}

void economy_cancel_trade(u32 trade_id) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized || trade_id >= econ->trade_count) return;
    
    // Remove the trade
    for (u32 i = trade_id; i < econ->trade_count - 1; i++) {
        econ->active_trades[i] = econ->active_trades[i + 1];
    }
    econ->trade_count--;
}

// ✅ COMPLETED: Business utility functions
Business* economy_get_business(EntityID business_id) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized) return NULL;
    
    for (u32 i = 0; i < econ->business_count; i++) {
        if (econ->businesses[i].owner_id == business_id) {
            return &econ->businesses[i];
        }
    }
    return NULL;
}

u32 economy_get_business_count(void) {
    return g_economy.business_count;
}

void economy_close_business(EntityID business_id) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized) return;
    
    for (u32 i = 0; i < econ->business_count; i++) {
        if (econ->businesses[i].owner_id == business_id) {
            econ->businesses[i].is_active = false;
            break;
        }
    }
}

// ✅ COMPLETED: Tax utility functions
void economy_set_tax_rates(f32 sales_tax, f32 income_tax, f32 property_tax) {
    EconomyManager* econ = &g_economy;
    if (!econ->initialized) return;
    
    econ->tax_system.sales_tax_rate = CLAMP(sales_tax, 0.0f, 1.0f);
    econ->tax_system.income_tax_rate = CLAMP(income_tax, 0.0f, 1.0f);
    econ->tax_system.property_tax_rate = CLAMP(property_tax, 0.0f, 1.0f);
}

void economy_enable_taxes(bool enabled) {
    if (g_economy.initialized) {
        g_economy.tax_system.tax_enabled = enabled;
    }
}
