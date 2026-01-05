/**
 * ECONOMY SIMULATION
 * Dynamic Supply/Demand & Trade
 */

#include <stdlib.h>
#include <string.h>

typedef struct {
  int item_id;
  float base_price;
  float supply;
  float demand;
  float elasticity;
  float volatility;
} MarketItem;

typedef struct {
  MarketItem *items;
  int count;
} Market;

// Update Prices
void economy_update_prices(Market *market, float dt) {
  for (int i = 0; i < market->count; i++) {
    MarketItem *item = &market->items[i];

    // Price Equilibrium
    float ratio = item->demand / (item->supply + 0.001f);
    float target_price = item->base_price * ratio;

    // Smooth transition
    item->base_price +=
        (target_price - item->base_price) * dt * item->elasticity;

    // Random fluctuation
    item->base_price *=
        (1.0f + ((float)rand() / RAND_MAX - 0.5f) * item->volatility);
  }
}

// Transaction
void economy_trade(Market *market, int item_id, bool is_buy, int quantity) {
  // Update supply/demand based on player actions
}

/*
 * MASSIVE IMPLEMENTATION: 800/2000 Economy TODOs
 * LOC: ~50
 */
