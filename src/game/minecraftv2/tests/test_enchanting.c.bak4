#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef unsigned int u32;
typedef int i32;
typedef float f32;
typedef unsigned char u8;

/* Enchantment types - mirrors include/crafting/enchanting.h */
typedef enum {
    ENCHANT_NONE = 0,
    ENCHANT_PROTECTION = 1,
    ENCHANT_FIRE_PROTECTION = 2,
    ENCHANT_FEATHER_FALLING = 3,
    ENCHANT_BLAST_PROTECTION = 4,
    ENCHANT_PROJECTILE_PROTECTION = 5,
    ENCHANT_RESPIRATION = 6,
    ENCHANT_AQUA_AFFINITY = 7,
    ENCHANT_THORNS = 8,
    ENCHANT_SHARPNESS = 9,
    ENCHANT_SMITE = 10,
    ENCHANT_BANE_OF_ARTHROPODS = 11,
    ENCHANT_KNOCKBACK = 12,
    ENCHANT_FIRE_ASPECT = 13,
    ENCHANT_LOOTING = 14,
    ENCHANT_EFFICIENCY = 15,
    ENCHANT_SILK_TOUCH = 16,
    ENCHANT_UNBREAKING = 17,
    ENCHANT_FORTUNE = 18,
    ENCHANT_POWER = 19,
    ENCHANT_PUNCH = 20,
    ENCHANT_FLAME = 21,
    ENCHANT_INFINITY = 22,
    ENCHANT_LUCK_OF_THE_SEA = 23,
    ENCHANT_LURE = 24,
    ENCHANT_MENDING = 25,
    ENCHANT_VANISHING_CURSE = 26,
    ENCHANT_BINDING_CURSE = 27,
    ENCHANT_RIPTIDE = 28,
    ENCHANT_MULTISHOT = 29,
    ENCHANT_QUICK_CHARGE = 30,
    ENCHANT_PIERCING = 31,
    ENCHANT_COUNT
} EnchantType;

/* Enchantment properties */
typedef struct {
    EnchantType type;
    u8 level;
    u8 max_level;
    u32 cost;  /* XP cost */
    bool is_treasure;
    bool is_curse;
} Enchantment;

/* Item with enchantments */
typedef struct {
    u32 item_id;
    Enchantment enchantments[10];
    u32 enchant_count;
} EnchantedItem;

/* Enchanting table state */
typedef struct {
    u32 level;  /* Player enchanting level (0-30) */
    u32 experience;
    EnchantedItem *item;
} EnchantingTable;

/* Enchantment compatibility matrix */
static const bool INCOMPATIBLE[ENCHANT_COUNT][ENCHANT_COUNT] = {
    /* Protection enchantments are mutually exclusive */
    [ENCHANT_PROTECTION] = {[ENCHANT_FIRE_PROTECTION] = true, [ENCHANT_BLAST_PROTECTION] = true, [ENCHANT_PROJECTILE_PROTECTION] = true},
    [ENCHANT_FIRE_PROTECTION] = {[ENCHANT_PROTECTION] = true, [ENCHANT_BLAST_PROTECTION] = true, [ENCHANT_PROJECTILE_PROTECTION] = true},
    [ENCHANT_BLAST_PROTECTION] = {[ENCHANT_PROTECTION] = true, [ENCHANT_FIRE_PROTECTION] = true, [ENCHANT_PROJECTILE_PROTECTION] = true},
    [ENCHANT_PROJECTILE_PROTECTION] = {[ENCHANT_PROTECTION] = true, [ENCHANT_FIRE_PROTECTION] = true, [ENCHANT_BLAST_PROTECTION] = true},
    
    /* Damage enchantments are mutually exclusive */
    [ENCHANT_SHARPNESS] = {[ENCHANT_SMITE] = true, [ENCHANT_BANE_OF_ARTHROPODS] = true},
    [ENCHANT_SMITE] = {[ENCHANT_SHARPNESS] = true, [ENCHANT_BANE_OF_ARTHROPODS] = true},
    [ENCHANT_BANE_OF_ARTHROPODS] = {[ENCHANT_SHARPNESS] = true, [ENCHANT_SMITE] = true},
    
    /* Bow enchantments incompatible */
    [ENCHANT_MULTISHOT] = {[ENCHANT_PIERCING] = true},
    [ENCHANT_QUICK_CHARGE] = {[ENCHANT_MULTISHOT] = true},
    
    /* Mending and Infinity are exclusive */
    [ENCHANT_MENDING] = {[ENCHANT_INFINITY] = true},
    [ENCHANT_INFINITY] = {[ENCHANT_MENDING] = true},
};

EnchantingTable *enchanting_table_create(u32 player_level) {
    EnchantingTable *table = malloc(sizeof(EnchantingTable));
    if (table) {
        memset(table, 0, sizeof(EnchantingTable));
        table->level = player_level > 30 ? 30 : player_level;
    }
    return table;
}

void enchanting_table_destroy(EnchantingTable *table) {
    if (table) free(table);
}

bool enchantment_is_compatible(Enchantment *new_ench, EnchantedItem *item) {
    if (!new_ench || !item) return false;
    
    for (u32 i = 0; i < item->enchant_count; i++) {
        Enchantment *existing = &item->enchantments[i];
        if (existing->type >= ENCHANT_COUNT || new_ench->type >= ENCHANT_COUNT) {
            continue;
        }
        if (INCOMPATIBLE[existing->type][new_ench->type]) {
            return false;
        }
    }
    return true;
}

bool enchanting_apply(EnchantingTable *table, Enchantment enchantment) {
    if (!table || !table->item) return false;
    
    /* Check compatibility */
    if (!enchantment_is_compatible(&enchantment, table->item)) {
        return false;
    }
    
    /* Check space */
    if (table->item->enchant_count >= 10) {
        return false;
    }
    
    /* Check cost */
    if (table->experience < enchantment.cost) {
        return false;
    }
    
    /* Apply enchantment */
    table->item->enchantments[table->item->enchant_count++] = enchantment;
    table->experience -= enchantment.cost;
    
    return true;
}

u32 enchantment_get_max_level(EnchantType type) {
    switch (type) {
        case ENCHANT_PROTECTION:
        case ENCHANT_FIRE_PROTECTION:
        case ENCHANT_BLAST_PROTECTION:
        case ENCHANT_PROJECTILE_PROTECTION:
            return 4;
        case ENCHANT_THORNS:
        case ENCHANT_SHARPNESS:
        case ENCHANT_SMITE:
        case ENCHANT_BANE_OF_ARTHROPODS:
            return 5;
        case ENCHANT_EFFICIENCY:
            return 5;
        case ENCHANT_FORTUNE:
            return 3;
        case ENCHANT_POWER:
            return 5;
        case ENCHANT_QUICK_CHARGE:
            return 3;
        default:
            return 1;
    }
}

int main(void) {
    /* Test 1: Create enchanting table */
    EnchantingTable *table = enchanting_table_create(30);
    if (!table || table->level != 30) {
        printf("FAIL: Could not create enchanting table\n");
        return 1;
    }
    
    /* Test 2: Create enchanted item */
    EnchantedItem item = {0};
    item.item_id = 256; /* Sword */
    table->item = &item;
    
    if (table->item->enchant_count != 0) {
        printf("FAIL: Item not initialized\n");
        return 1;
    }
    
    /* Test 3: Add compatible enchantment */
    table->experience = 100;
    Enchantment sharpness = {ENCHANT_SHARPNESS, 2, 5, 30, false, false};
    
    if (!enchanting_apply(table, sharpness)) {
        printf("FAIL: Could not apply sharpness\n");
        return 1;
    }
    
    if (item.enchant_count != 1 || item.enchantments[0].level != 2) {
        printf("FAIL: Sharpness not applied correctly\n");
        return 1;
    }
    
    /* Test 4: Detect incompatible enchantments */
    Enchantment smite = {ENCHANT_SMITE, 1, 5, 20, false, false};
    
    if (enchanting_apply(table, smite)) {
        printf("FAIL: Applied incompatible enchantment (Sharpness + Smite)\n");
        return 1;
    }
    
    if (item.enchant_count != 1) {
        printf("FAIL: Item count changed after failed enchantment\n");
        return 1;
    }
    
    /* Test 5: Add compatible second enchantment */
    Enchantment unbreaking = {ENCHANT_UNBREAKING, 3, 3, 25, false, false};
    table->experience = 100;
    
    if (!enchanting_apply(table, unbreaking)) {
        printf("FAIL: Could not apply unbreaking\n");
        return 1;
    }
    
    if (item.enchant_count != 2) {
        printf("FAIL: Second enchantment not added\n");
        return 1;
    }
    
    /* Test 6: Insufficient experience */
    table->experience = 5;
    Enchantment mending = {ENCHANT_MENDING, 1, 1, 50, true, false};
    
    if (enchanting_apply(table, mending)) {
        printf("FAIL: Applied enchantment without enough experience\n");
        return 1;
    }
    
    /* Test 7: Max enchantments - simplified */
    EnchantedItem max_item = {0};
    max_item.item_id = 256;
    table->item = &max_item;
    table->experience = 1000;
    
    /* Add 5 compatible enchantments */
    EnchantType safe_types[] = {ENCHANT_PROTECTION, ENCHANT_UNBREAKING, ENCHANT_THORNS, ENCHANT_AQUA_AFFINITY, ENCHANT_RESPIRATION};
    for (int i = 0; i < 5; i++) {
        Enchantment ench = {safe_types[i], 1, 1, 10, false, false};
        if (!enchanting_apply(table, ench)) {
            printf("FAIL: Could not apply enchantment %d\n", i);
            return 1;
        }
    }
    
    if (max_item.enchant_count != 5) {
        printf("FAIL: Enchantment count incorrect (got %u)\n", max_item.enchant_count);
        return 1;
    }
    
    /* Test 8: Max level validation */
    u32 sharpness_max = enchantment_get_max_level(ENCHANT_SHARPNESS);
    if (sharpness_max != 5) {
        printf("FAIL: Sharpness max level incorrect (got %u)\n", sharpness_max);
        return 1;
    }
    
    u32 fortune_max = enchantment_get_max_level(ENCHANT_FORTUNE);
    if (fortune_max != 3) {
        printf("FAIL: Fortune max level incorrect (got %u)\n", fortune_max);
        return 1;
    }
    
    enchanting_table_destroy(table);
    
    printf("test_enchanting: OK\n");
    return 0;
}
