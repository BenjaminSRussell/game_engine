#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef unsigned int u32;
typedef int i32;
typedef float f32;

/* Inventory slot */
typedef struct {
    u32 item_id;
    u32 count;
    u32 durability;
} InventorySlot;

/* Inventory */
typedef struct {
    InventorySlot slots[36];
    u32 selected_slot;
    u32 total_items;
} Inventory;

Inventory *inventory_create(void) {
    Inventory *inv = malloc(sizeof(Inventory));
    if (inv) {
        memset(inv, 0, sizeof(Inventory));
    }
    return inv;
}

void inventory_free(Inventory *inv) {
    if (inv) free(inv);
}

bool inventory_add_item(Inventory *inv, u32 item_id, u32 count) {
    if (!inv || count == 0) return false;
    
    /* Try to stack on existing items */
    for (u32 i = 0; i < 36; i++) {
        if (inv->slots[i].item_id == item_id && inv->slots[i].count > 0) {
            u32 space = 64 - inv->slots[i].count;
            if (space > 0) {
                u32 to_add = (count < space) ? count : space;
                inv->slots[i].count += to_add;
                inv->total_items += to_add;
                count -= to_add;
                if (count == 0) return true;
            }
        }
    }
    
    /* Find empty slot for remaining items */
    for (u32 i = 0; i < 36; i++) {
        if (inv->slots[i].item_id == 0) {
            u32 to_add = (count > 64) ? 64 : count;
            inv->slots[i].item_id = item_id;
            inv->slots[i].count = to_add;
            inv->total_items += to_add;
            count -= to_add;
            if (count == 0) return true;
        }
    }
    
    return false; /* Inventory full */
}

bool inventory_remove_item(Inventory *inv, u32 item_id, u32 count) {
    if (!inv || count == 0) return false;
    
    for (u32 i = 0; i < 36; i++) {
        if (inv->slots[i].item_id == item_id) {
            if (inv->slots[i].count >= count) {
                inv->slots[i].count -= count;
                inv->total_items -= count;
                if (inv->slots[i].count == 0) {
                    inv->slots[i].item_id = 0;
                }
                return true;
            } else {
                count -= inv->slots[i].count;
                inv->total_items -= inv->slots[i].count;
                inv->slots[i].item_id = 0;
                inv->slots[i].count = 0;
            }
        }
    }
    
    return false;
}

u32 inventory_count_item(Inventory *inv, u32 item_id) {
    if (!inv) return 0;
    u32 count = 0;
    for (u32 i = 0; i < 36; i++) {
        if (inv->slots[i].item_id == item_id) {
            count += inv->slots[i].count;
        }
    }
    return count;
}

bool inventory_has_space(Inventory *inv, u32 item_count) {
    if (!inv) return false;
    u32 empty_slots = 0;
    for (u32 i = 0; i < 36; i++) {
        if (inv->slots[i].item_id == 0) empty_slots++;
    }
    return empty_slots > 0 || inv->total_items + item_count <= 36 * 64;
}

int main(void) {
    /* Test 1: Create inventory */
    Inventory *inv = inventory_create();
    if (!inv || inv->total_items != 0) {
        printf("FAIL: Could not create inventory\n");
        return 1;
    }
    
    /* Test 2: Add items */
    if (!inventory_add_item(inv, 1, 32)) {
        printf("FAIL: Could not add items\n");
        return 1;
    }
    
    if (inv->total_items != 32) {
        printf("FAIL: Total items incorrect (got %u, expected 32)\n", inv->total_items);
        return 1;
    }
    
    /* Test 3: Count items */
    u32 count = inventory_count_item(inv, 1);
    if (count != 32) {
        printf("FAIL: Item count incorrect (got %u, expected 32)\n", count);
        return 1;
    }
    
    /* Test 4: Stack items in same slot */
    if (!inventory_add_item(inv, 1, 20)) {
        printf("FAIL: Could not add more items to stack\n");
        return 1;
    }
    
    if (inventory_count_item(inv, 1) != 52) {
        printf("FAIL: Stacking failed\n");
        return 1;
    }
    
    /* Test 5: Stack overflow creates new slot */
    if (!inventory_add_item(inv, 1, 50)) {
        printf("FAIL: Could not create new slot\n");
        return 1;
    }
    
    u32 total_count = inventory_count_item(inv, 1);
    if (total_count != 102) {
        printf("FAIL: Total count incorrect after overflow (got %u, expected 102)\n", total_count);
        return 1;
    }
    
    /* Test 6: Remove items */
    if (!inventory_remove_item(inv, 1, 30)) {
        printf("FAIL: Could not remove items\n");
        return 1;
    }
    
    if (inventory_count_item(inv, 1) != 72) {
        printf("FAIL: Remove failed (got %u, expected 72)\n", inventory_count_item(inv, 1));
        return 1;
    }
    
    /* Test 7: Remove non-existent item fails */
    if (inventory_remove_item(inv, 999, 10)) {
        printf("FAIL: Removed non-existent item\n");
        return 1;
    }
    
    /* Test 8: Multiple item types */
    inventory_add_item(inv, 2, 16);
    inventory_add_item(inv, 3, 64);
    
    if (inventory_count_item(inv, 2) != 16 || inventory_count_item(inv, 3) != 64) {
        printf("FAIL: Multiple item types failed\n");
        return 1;
    }
    
    /* Test 9: Fill inventory */
    Inventory *inv2 = inventory_create();
    
    /* Add items until full */
    for (int i = 0; i < 36; i++) {
        if (!inventory_add_item(inv2, i + 10, 64)) {
            break;
        }
    }
    
    u32 total = inv2->total_items;
    if (total != 36 * 64) {
        printf("FAIL: Inventory not full (got %u items, expected %u)\n", total, 36 * 64);
        return 1;
    }
    
    /* Test 10: Cannot add to full inventory */
    if (inventory_add_item(inv2, 999, 1)) {
        printf("FAIL: Added item to full inventory\n");
        return 1;
    }
    
    inventory_free(inv);
    inventory_free(inv2);
    
    printf("test_inventory: OK\n");
    return 0;
}
