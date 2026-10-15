/*
 * item_db_test.c
 * Item database registration and lookup by id and name.
 * Compiles item_db.c directly and does not link Engine.
 */

#include "core/logging/unified_logger.h"
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

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static Item make_item(u32 id, const char *name, ItemType type, ItemRarity rarity,
                      u32 max_stack, f32 weight, u32 value, u32 durability) {
    Item item;
    memset(&item, 0, sizeof(item));
    item.id = id;
    item.name = name;
    item.description = "";
    item.type = type;
    item.rarity = rarity;
    item.max_stack_size = max_stack;
    item.weight = weight;
    item.sell_value = value;
    item.max_durability = durability;
    return item;
}

static void expect_uninitialized(const char *label) {
    const Item *found = NULL;
    ItemStack stack;
    Item bogus;

    if (item_database_get_count() != 0) {
        fprintf(stderr, "FAIL: %s count\n", label);
        exit(1);
    }
    if (item_database_get(1) != NULL || item_database_exists(1)) {
        fprintf(stderr, "FAIL: %s lookup\n", label);
        exit(1);
    }
    if (item_database_get_by_name("Sword") != NULL) {
        fprintf(stderr, "FAIL: %s name lookup\n", label);
        exit(1);
    }
    if (item_database_get_max_stack(1) != 1 || item_database_get_weight(1) != 0.0f ||
        item_database_get_value(1) != 0) {
        fprintf(stderr, "FAIL: %s missing stats\n", label);
        exit(1);
    }
    if (item_database_get_items_by_type(ITEM_TYPE_WEAPON, &found, 1) != 0 ||
        item_database_get_items_by_rarity(ITEM_RARITY_COMMON, &found, 1) != 0 ||
        item_database_get_items_by_type(ITEM_TYPE_WEAPON, NULL, 1) != 0) {
        fprintf(stderr, "FAIL: %s query\n", label);
        exit(1);
    }
    stack = item_create_stack(1, 4);
    if (stack.item_id != 0 || stack.quantity != 0) {
        fprintf(stderr, "FAIL: %s unknown stack\n", label);
        exit(1);
    }
    stack = item_create_stack_by_name("Sword", 2);
    if (stack.item_id != 0 || stack.quantity != 0) {
        fprintf(stderr, "FAIL: %s unknown named stack\n", label);
        exit(1);
    }
    memset(&bogus, 0, sizeof(bogus));
    bogus.id = 1;
    bogus.name = "Sword";
    if (item_database_register(&bogus) || item_database_register(NULL)) {
        fprintf(stderr, "FAIL: %s register\n", label);
        exit(1);
    }
    if (item_database_load_from_json("items.json")) {
        fprintf(stderr, "FAIL: %s json\n", label);
        exit(1);
    }
}

int main(void) {
    Item sword;
    Item potion;
    Item gem;
    Item ore;
    Item extra;
    Item duplicate;
    const Item *by_id;
    const Item *by_name;
    const Item *typed[4];
    const Item *rare[4];
    ItemStack stack;
    ItemStack cloned;
    ItemStack empty;
    u32 type_count;
    u32 rarity_count;

    expect_uninitialized("before init");

    if (!item_database_init(4)) {
        fail("init");
    }
    if (item_database_init(4) || item_database_get_count() != 0) {
        fail("double init");
    }
    if (item_database_load_from_json(NULL) || !item_database_load_from_json("items.json")) {
        fail("json placeholder");
    }

    sword = make_item(10, "Sword", ITEM_TYPE_WEAPON, ITEM_RARITY_RARE, 1, 3.5f, 40, 80);
    potion = make_item(11, "Potion", ITEM_TYPE_CONSUMABLE, ITEM_RARITY_COMMON, 16, 0.2f, 5, 0);
    gem = make_item(12, "Gem", ITEM_TYPE_MATERIAL, ITEM_RARITY_RARE, 32, 0.1f, 100, 0);
    if (!item_database_register(&sword) || !item_database_register(&potion) ||
        !item_database_register(&gem)) {
        fail("register");
    }
    if (item_database_get_count() != 3) {
        fail("count after register");
    }

    duplicate = sword;
    if (item_database_register(&duplicate) || item_database_get_count() != 3) {
        fail("duplicate id");
    }

    by_id = item_database_get(10);
    by_name = item_database_get_by_name("Sword");
    if (!by_id || !by_name || by_id != by_name || by_id->id != 10 ||
        strcmp(by_id->name, "Sword") != 0) {
        fail("lookup sword");
    }
    if (!item_database_exists(11) || item_database_get(99) != NULL ||
        item_database_exists(99) || item_database_get_by_name("sword") != NULL ||
        item_database_get_by_name(NULL) != NULL ||
        item_database_get_by_name("Missing") != NULL) {
        fail("missing or case-sensitive lookup");
    }

    by_name = item_database_get_by_name("Potion");
    if (!by_name || by_name->id != 11 || by_name->type != ITEM_TYPE_CONSUMABLE) {
        fail("lookup potion");
    }
    if (item_database_get_max_stack(11) != 16 || item_database_get_weight(11) != 0.2f ||
        item_database_get_value(11) != 5) {
        fail("potion stats");
    }
    if (item_database_get_max_stack(99) != 1 || item_database_get_weight(99) != 0.0f ||
        item_database_get_value(99) != 0) {
        fail("missing stats");
    }

    type_count = item_database_get_items_by_type(ITEM_TYPE_WEAPON, typed, 4);
    if (type_count != 1 || typed[0]->id != 10) {
        fail("type query");
    }
    type_count = item_database_get_items_by_type(ITEM_TYPE_QUEST, typed, 4);
    if (type_count != 0) {
        fail("empty type query");
    }
    rarity_count = item_database_get_items_by_rarity(ITEM_RARITY_RARE, rare, 1);
    if (rarity_count != 1 || (rare[0]->id != 10 && rare[0]->id != 12)) {
        fail("rarity query is limited");
    }
    rarity_count = item_database_get_items_by_rarity(ITEM_RARITY_RARE, rare, 4);
    if (rarity_count != 2) {
        fail("rarity query");
    }
    if (item_database_get_items_by_type(ITEM_TYPE_WEAPON, NULL, 4) != 0) {
        fail("null type query");
    }

    stack = item_create_stack(10, 3);
    if (stack.item_id != 10 || stack.quantity != 3 || stack.max_quantity != 1 ||
        stack.durability != 80) {
        fail("create stack");
    }
    stack = item_create_stack(11, 0);
    if (stack.item_id != 11 || stack.quantity != 1 || stack.max_quantity != 16) {
        fail("zero quantity becomes one");
    }
    stack = item_create_stack_by_name("Gem", 4);
    if (stack.item_id != 12 || stack.quantity != 4 || stack.max_quantity != 32) {
        fail("create stack by name");
    }
    stack = item_create_stack(99, 2);
    if (stack.item_id != 0 || stack.quantity != 0) {
        fail("unknown stack");
    }
    cloned = item_stack_clone(&stack);
    if (cloned.item_id != stack.item_id || cloned.quantity != stack.quantity) {
        fail("clone empty unknown stack");
    }
    stack = item_create_stack_by_name("Potion", 7);
    cloned = item_stack_clone(&stack);
    if (cloned.item_id != 11 || cloned.quantity != 7 || cloned.max_quantity != 16) {
        fail("clone stack");
    }
    empty = item_stack_clone(NULL);
    if (empty.item_id != 0 || empty.quantity != 0) {
        fail("clone null");
    }

    ore = make_item(13, "Ore", ITEM_TYPE_MATERIAL, ITEM_RARITY_COMMON, 64, 1.0f, 2, 0);
    extra = make_item(14, "Shield", ITEM_TYPE_ARMOR, ITEM_RARITY_UNCOMMON, 1, 6.0f, 30, 50);
    if (!item_database_register(&ore) || item_database_get_count() != 4) {
        fail("fill capacity");
    }
    if (item_database_register(&extra) || item_database_get_count() != 4 ||
        item_database_get_by_name("Shield") != NULL) {
        fail("over capacity");
    }

    item_database_register_defaults();
    by_name = item_database_get_by_name("Ore");
    if (item_database_get_count() != 4 || !by_name || by_name->id != 13) {
        fail("defaults changed registry");
    }

    item_database_shutdown();
    expect_uninitialized("after shutdown");
    item_database_shutdown();

    if (!item_database_init(2)) {
        fail("reinit");
    }
    if (!item_database_register(&sword) || item_database_get_by_name("Sword") == NULL) {
        fail("lookup after reinit");
    }
    item_database_shutdown();

    printf("item database tests passed\n");
    return 0;
}
