#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

// Currency denominations
typedef enum {
    CURRENCY_COPPER = 1,
    CURRENCY_SILVER = 100,
    CURRENCY_GOLD = 10000,
    CURRENCY_PLATINUM = 1000000
} currency_denomination_t;

// Market item types
typedef enum {
    MARKET_ITEM_RESOURCE,
    MARKET_ITEM_CRAFTED,
    MARKET_ITEM_WEAPON,
    MARKET_ITEM_ARMOR,
    MARKET_ITEM_FOOD,
    MARKET_ITEM_RARE,
    MARKET_ITEM_LEGENDARY
} market_item_type_t;

// Market order types
typedef enum {
    ORDER_BUY,
    ORDER_SELL
} market_order_type_t;

// Market item structure
typedef struct {
    char item_id[128];
    char name[256];
    market_item_type_t type;
    int base_price;
    int current_price;
    int supply;
    int demand;
    float price_volatility;
    time_t last_trade_time;
    char description[512];
} market_item_t;

// Market order structure
typedef struct {
    int order_id;
    char player_id[64];
    char item_id[128];
    market_order_type_t type;
    int quantity;
    int price_per_unit;
    time_t created_time;
    time_t expiry_time;
    bool is_active;
    int filled_quantity;
} market_order_t;

// Market transaction structure
typedef struct {
    int transaction_id;
    char buyer_id[64];
    char seller_id[64];
    char item_id[128];
    int quantity;
    int price_per_unit;
    time_t timestamp;
    int total_value;
} market_transaction_t;

// Market statistics structure
typedef struct {
    int total_transactions;
    int total_volume;
    int total_value;
    float average_price;
    int highest_price;
    int lowest_price;
    time_t period_start;
    time_t period_end;
} market_stats_t;

// Global market structure
typedef struct {
    market_item_t* items;
    int item_count;
    int item_capacity;
    market_order_t* orders;
    int order_count;
    int order_capacity;
    market_transaction_t* transactions;
    int transaction_count;
    int transaction_capacity;
    int next_order_id;
    int next_transaction_id;
    float market_fee_rate;
    int price_update_interval_seconds;
    time_t last_price_update;
    market_stats_t daily_stats;
    market_stats_t weekly_stats;
} global_market_t;

// Market manager functions
global_market_t* market_create(void);
void market_destroy(global_market_t* market);
int market_add_item(global_market_t* market, const market_item_t* item);
market_item_t* market_get_item(global_market_t* market, const char* item_id);
int market_place_order(global_market_t* market, const char* player_id, const char* item_id, 
                       market_order_type_t type, int quantity, int price_per_unit, int expiry_hours);
bool market_cancel_order(global_market_t* market, int order_id, const char* player_id);
market_order_t* market_get_active_orders(global_market_t* market, const char* item_id, int* count);
bool market_execute_order(global_market_t* market, int order_id, const char* buyer_id, int quantity);
void market_update_prices(global_market_t* market);
float market_calculate_price(const market_item_t* item, int supply_change, int demand_change);
market_stats_t* market_get_statistics(global_market_t* market, const char* item_id, bool daily);
void market_save_state(global_market_t* market, const char* filename);
void market_load_state(global_market_t* market, const char* filename);

// Price calculation functions
int market_get_buy_price(global_market_t* market, const char* item_id, int quantity);
int market_get_sell_price(global_market_t* market, const char* item_id, int quantity);
float market_get_price_trend(global_market_t* market, const char* item_id);
bool market_is_good_investment(global_market_t* market, const char* item_id);

// Market analysis functions
void market_generate_report(global_market_t* market, char* buffer, size_t buffer_size);
int market_get_top_movers(global_market_t* market, char** item_ids, int max_items, bool ascending);
int market_get_highest_volume(global_market_t* market, char** item_ids, int max_items);
float market_calculate_inflation_rate(global_market_t* market);

// Trading functions
bool market_can_afford(global_market_t* market, const char* player_id, int amount);
bool market_deduct_currency(global_market_t* market, const char* player_id, int amount);
bool market_add_currency(global_market_t* market, const char* player_id, int amount);
int market_get_player_balance(global_market_t* market, const char* player_id);

// Market event callbacks
typedef void (*market_order_filled_callback_t)(const market_transaction_t* transaction);
typedef void (*market_price_changed_callback_t)(const char* item_id, int old_price, int new_price);
typedef void (*market_item_added_callback_t)(const market_item_t* item);

void market_set_callbacks(global_market_t* market,
                        market_order_filled_callback_t order_filled,
                        market_price_changed_callback_t price_changed,
                        market_item_added_callback_t item_added);

// Utility functions
const char* market_item_type_to_string(market_item_type_t type);
const char* market_order_type_to_string(market_order_type_t type);
void market_format_price(char* buffer, size_t buffer_size, int price);
void market_debug_print_item(const market_item_t* item);
void market_debug_print_order(const market_order_t* order);
void market_debug_print_transaction(const market_transaction_t* transaction);

// Market implementation
global_market_t* market_create(void) {
    global_market_t* market = malloc(sizeof(global_market_t));
    if (!market) return NULL;
    
    market->items = malloc(sizeof(market_item_t) * 1000);
    market->item_count = 0;
    market->item_capacity = 1000;
    
    market->orders = malloc(sizeof(market_order_t) * 5000);
    market->order_count = 0;
    market->order_capacity = 5000;
    
    market->transactions = malloc(sizeof(market_transaction_t) * 10000);
    market->transaction_count = 0;
    market->transaction_capacity = 10000;
    
    market->next_order_id = 1;
    market->next_transaction_id = 1;
    market->market_fee_rate = 0.02f; // 2% fee
    market->price_update_interval_seconds = 300; // 5 minutes
    market->last_price_update = time(NULL);
    
    memset(&market->daily_stats, 0, sizeof(market_stats_t));
    memset(&market->weekly_stats, 0, sizeof(market_stats_t));
    
    return market;
}

void market_destroy(global_market_t* market) {
    if (!market) return;
    
    free(market->items);
    free(market->orders);
    free(market->transactions);
    free(market);
}

int market_add_item(global_market_t* market, const market_item_t* item) {
    if (!market || !item || market->item_count >= market->item_capacity) {
        return -1;
    }
    
    // Check if item already exists
    for (int i = 0; i < market->item_count; i++) {
        if (strcmp(market->items[i].item_id, item->item_id) == 0) {
            return -1; // Item already exists
        }
    }
    
    market->items[market->item_count] = *item;
    market->items[market->item_count].current_price = item->base_price;
    market->items[market->item_count].last_trade_time = time(NULL);
    
    return market->item_count++;
}

market_item_t* market_get_item(global_market_t* market, const char* item_id) {
    if (!market || !item_id) return NULL;
    
    for (int i = 0; i < market->item_count; i++) {
        if (strcmp(market->items[i].item_id, item_id) == 0) {
            return &market->items[i];
        }
    }
    return NULL;
}

int market_place_order(global_market_t* market, const char* player_id, const char* item_id, 
                       market_order_type_t type, int quantity, int price_per_unit, int expiry_hours) {
    if (!market || !player_id || !item_id || market->order_count >= market->order_capacity) {
        return -1;
    }
    
    market_item_t* item = market_get_item(market, item_id);
    if (!item) return -1;
    
    market_order_t order = {0};
    order.order_id = market->next_order_id++;
    strncpy(order.player_id, player_id, sizeof(order.player_id) - 1);
    strncpy(order.item_id, item_id, sizeof(order.item_id) - 1);
    order.type = type;
    order.quantity = quantity;
    order.price_per_unit = price_per_unit;
    order.created_time = time(NULL);
    order.expiry_time = order.created_time + (expiry_hours * 3600);
    order.is_active = true;
    order.filled_quantity = 0;
    
    market->orders[market->order_count++] = order;
    
    // Update supply/demand based on order type
    if (type == ORDER_SELL) {
        item->supply += quantity;
    } else {
        item->demand += quantity;
    }
    
    return order.order_id;
}

void market_update_prices(global_market_t* market) {
    if (!market) return;
    
    time_t current_time = time(NULL);
    if (current_time - market->last_price_update < market->price_update_interval_seconds) {
        return;
    }
    
    for (int i = 0; i < market->item_count; i++) {
        market_item_t* item = &market->items[i];
        
        // Calculate new price based on supply and demand
        float supply_demand_ratio = (float)item->demand / (float)(item->supply + 1);
        float price_multiplier = 1.0f + (supply_demand_ratio - 1.0f) * item->price_volatility;
        
        int old_price = item->current_price;
        item->current_price = (int)(item->base_price * price_multiplier);
        
        // Ensure price doesn't go below 1 or above 10x base price
        item->current_price = fmax(1, fmin(item->current_price, item->base_price * 10));
        
        // Apply some randomness
        float random_factor = 0.9f + ((float)rand() / RAND_MAX) * 0.2f; // 0.9 to 1.1
        item->current_price = (int)(item->current_price * random_factor);
        
        item->last_trade_time = current_time;
    }
    
    market->last_price_update = current_time;
}

float market_calculate_price(const market_item_t* item, int supply_change, int demand_change) {
    if (!item) return 0.0f;
    
    int new_supply = item->supply + supply_change;
    int new_demand = item->demand + demand_change;
    
    float supply_demand_ratio = (float)new_demand / (float)(new_supply + 1);
    float price_multiplier = 1.0f + (supply_demand_ratio - 1.0f) * item->price_volatility;
    
    return item->base_price * price_multiplier;
}

int market_get_buy_price(global_market_t* market, const char* item_id, int quantity) {
    market_item_t* item = market_get_item(market, item_id);
    if (!item) return -1;
    
    // Price increases with quantity bought
    float quantity_multiplier = 1.0f + (quantity - 1) * 0.01f; // 1% increase per additional unit
    int base_price = item->current_price * quantity;
    int final_price = (int)(base_price * quantity_multiplier);
    
    // Add market fee
    final_price = (int)(final_price * (1.0f + market->market_fee_rate));
    
    return final_price;
}

int market_get_sell_price(global_market_t* market, const char* item_id, int quantity) {
    market_item_t* item = market_get_item(market, item_id);
    if (!item) return -1;
    
    // Price decreases with quantity sold
    float quantity_multiplier = 1.0f - (quantity - 1) * 0.005f; // 0.5% decrease per additional unit
    int base_price = item->current_price * quantity;
    int final_price = (int)(base_price * quantity_multiplier);
    
    // Subtract market fee
    final_price = (int)(final_price * (1.0f - market->market_fee_rate));
    
    return final_price;
}

float market_get_price_trend(global_market_t* market, const char* item_id) {
    market_item_t* item = market_get_item(market, item_id);
    if (!item) return 0.0f;
    
    return (float)(item->current_price - item->base_price) / (float)item->base_price;
}

bool market_is_good_investment(global_market_t* market, const char* item_id) {
    market_item_t* item = market_get_item(market, item_id);
    if (!item) return false;
    
    // Consider it a good investment if:
    // 1. Current price is below base price (undervalued)
    // 2. Demand is higher than supply
    // 3. Price volatility is reasonable (not too risky)
    
    bool undervalued = item->current_price < item->base_price;
    bool high_demand = item->demand > item->supply;
    bool reasonable_volatility = item->price_volatility < 0.5f;
    
    return undervalued && high_demand && reasonable_volatility;
}

void market_format_price(char* buffer, size_t buffer_size, int price) {
    if (!buffer || buffer_size == 0) return;
    
    int platinum = price / CURRENCY_PLATINUM;
    price %= CURRENCY_PLATINUM;
    int gold = price / CURRENCY_GOLD;
    price %= CURRENCY_GOLD;
    int silver = price / CURRENCY_SILVER;
    int copper = price % CURRENCY_SILVER;
    
    buffer[0] = '\0';
    
    if (platinum > 0) {
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "%dp ", platinum);
    }
    if (gold > 0) {
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "%dg ", gold);
    }
    if (silver > 0) {
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "%ds ", silver);
    }
    if (copper > 0 || strlen(buffer) == 0) {
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "%dc", copper);
    }
}

const char* market_item_type_to_string(market_item_type_t type) {
    switch (type) {
        case MARKET_ITEM_RESOURCE: return "Resource";
        case MARKET_ITEM_CRAFTED: return "Crafted";
        case MARKET_ITEM_WEAPON: return "Weapon";
        case MARKET_ITEM_ARMOR: return "Armor";
        case MARKET_ITEM_FOOD: return "Food";
        case MARKET_ITEM_RARE: return "Rare";
        case MARKET_ITEM_LEGENDARY: return "Legendary";
        default: return "Unknown";
    }
}

void market_debug_print_item(const market_item_t* item) {
    if (!item) return;
    
    char price_str[128];
    market_format_price(price_str, sizeof(price_str), item->current_price);
    
    printf("Item: %s (%s)\n", item->name, item->item_id);
    printf("Type: %s\n", market_item_type_to_string(item->type));
    printf("Price: %s (Base: %d)\n", price_str, item->base_price);
    printf("Supply: %d, Demand: %d\n", item->supply, item->demand);
    printf("Volatility: %.2f\n", item->price_volatility);
}

// TODO: High - Implement a currency system with different denominations of coins.
// TODO: High - Implement a system for players to trade with each other and with NPCs.
// TODO: Medium - Implement a system for players to own and manage businesses, such as shops and farms.
// TODO: Low - Add a system for taxes and other economic regulations.
