/*
 * food_recipe_test.c
 * Cooking recipes register with the crafting system and the item database.
 * This target compiles those sources directly and does not link Engine.
 */

#include "core/logging/unified_logger.h"
#include "core/memory/unified_memory_allocator.h"
#include "core/utils.h"
#include "gameplay/crafting_system.h"
#include "gameplay/food_system.h"
#include "gameplay/inventory/item_database.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
    (void)level;
    (void)category;
    (void)file;
    (void)line;
    (void)function;
    (void)format;
}

void *unified_memory_alloc(size_t size, MemoryStrategy strategy, MemoryFlags flags,
                           const char *file, int line, const char *function) {
    (void)strategy;
    (void)flags;
    (void)file;
    (void)line;
    (void)function;
    return calloc(1, size > 0 ? size : 1);
}

void unified_memory_free(void *ptr, const char *file, int line, const char *function) {
    (void)file;
    (void)line;
    (void)function;
    free(ptr);
}

u64 get_current_time_ms(void) { return 1000; }

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect_station_count(uint32_t station_type, uint32_t expected, const char *msg) {
    const recipe_t *recipes = NULL;
    uint32_t count = 0;
    crafting_get_recipes_by_station(station_type, &recipes, &count);
    if (count != expected) {
        fprintf(stderr, "FAIL: %s (got %u expected %u)\n", msg, count, expected);
        exit(1);
    }
}

int main(void) {
    const recipe_t *all = NULL;
    uint32_t all_count = 0;
    char name[64];
    uint32_t ingredient_id = 0;
    uint32_t ingredient_qty = 0;
    uint32_t result_id = 0;
    uint32_t result_qty = 0;
    uint32_t station = 0;
    uint32_t time_ms = 0;
    bool discovered = false;

    if (!item_database_init(32) || !food_system_init() || !crafting_init()) {
        fail("init");
    }

    if (food_set_cooking_recipe(1, 2, STATION_TYPE_CAMPFIRE, 1000)) {
        fail("missing foods created a recipe");
    }
    crafting_get_all_recipes(&all, &all_count);
    if (all_count != 0) {
        fail("failed recipe call still registered");
    }

    uint32_t raw = food_add_item("Meat", "Raw meat", 0.0f, 0.0f, 2.0f, 0.0f);
    uint32_t cooked = food_add_item("Steak", "Cooked steak", 8.0f, 1.0f, 6.0f, 0.0f);
    if (raw == 0 || cooked == 0) {
        fail("food_add_item");
    }

    const Item *raw_item = item_database_get(raw);
    if (!raw_item || strcmp(raw_item->name, "Meat") != 0 || raw_item->id != raw) {
        fail("raw item was not registered");
    }
    if (raw_item->type != ITEM_TYPE_CONSUMABLE) {
        fail("food type");
    }
    if ((raw_item->flags & ITEM_FLAG_CONSUMABLE) == 0 ||
        (raw_item->flags & ITEM_FLAG_STACKABLE) == 0) {
        fail("food flags");
    }
    if (raw_item->max_stack_size != 64 || raw_item->sell_value != 10) {
        fail("stack or value");
    }

    const Item *cooked_item = item_database_get(cooked);
    if (!cooked_item || cooked_item->data.consumable.healing != 8.0f) {
        fail("cooked healing");
    }

    if (!food_set_cooking_recipe(raw, cooked, STATION_TYPE_CAMPFIRE, 2500)) {
        fail("food_set_cooking_recipe");
    }
    if (!crafting_recipe_get_name(1, name, sizeof(name)) || strcmp(name, "Cook Meat") != 0) {
        fail("recipe name");
    }
    if (!crafting_recipe_get_primary(1, &ingredient_id, &ingredient_qty, &result_id, &result_qty,
                                     &station, &time_ms, &discovered)) {
        fail("recipe primary");
    }
    if (ingredient_id != raw || ingredient_qty != 1 || result_id != cooked || result_qty != 1) {
        fail("recipe inputs");
    }
    if (station != STATION_TYPE_CAMPFIRE || time_ms != 2500 || !discovered) {
        fail("recipe station, time, or discovered");
    }
    expect_station_count(STATION_TYPE_CAMPFIRE, 1, "campfire recipes");
    expect_station_count(STATION_TYPE_FURNACE, 0, "furnace recipes before batch");

    uint32_t potato = food_add_item("Potato", "Raw potato", 0.0f, 0.0f, 1.0f, 0.0f);
    uint32_t baked = food_add_item("Baked Potato", "Baked", 3.0f, 0.0f, 4.0f, 0.0f);
    if (!food_set_preparation(potato, true, 4000, false, 0)) {
        fail("preparation");
    }
    if (!food_set_cooking_result(potato, baked)) {
        fail("cooking result");
    }
    expect_station_count(STATION_TYPE_FURNACE, 1, "furnace recipe");
    if (!crafting_recipe_get_name(2, name, sizeof(name)) || strcmp(name, "Cook Potato") != 0) {
        fail("furnace recipe name");
    }
    if (!crafting_recipe_get_primary(2, &ingredient_id, &ingredient_qty, &result_id, &result_qty,
                                     &station, &time_ms, &discovered)) {
        fail("furnace primary");
    }
    if (ingredient_id != potato || result_id != baked || station != STATION_TYPE_FURNACE ||
        time_ms != 4000 || discovered) {
        fail("furnace recipe fields");
    }

    food_register_furnace_recipes();
    expect_station_count(STATION_TYPE_FURNACE, 1, "furnace duplicate");
    expect_station_count(STATION_TYPE_CAMPFIRE, 1, "campfire after furnace batch");

    uint32_t fish = food_add_item("Fish", "Raw fish", 0.0f, 0.0f, 1.0f, 0.0f);
    uint32_t grilled = food_add_item("Grilled Fish", "Grilled", 4.0f, 0.0f, 3.0f, 0.0f);
    if (!food_set_cooking_recipe(fish, grilled, STATION_TYPE_CAMPFIRE, 1800)) {
        fail("fish recipe");
    }
    food_register_furnace_recipes();
    expect_station_count(STATION_TYPE_CAMPFIRE, 2, "campfire after fish");
    expect_station_count(STATION_TYPE_FURNACE, 1, "explicit recipe suppressed furnace duplicate");

    food_system_shutdown();
    crafting_shutdown();
    item_database_shutdown();
    printf("food recipe tests passed\n");
    return 0;
}
