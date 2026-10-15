// Issue #269: inventory/item_registry.h must provide the item IDs that
// player_combat.c matches equipped items against. Each ID is checked against
// its registry value. A changed value moves an item to another weapon or tool
// class without any compiler diagnostic.
#include <inventory/item_registry.h>

#include <stdio.h>

static int g_failures = 0;

#define EXPECT_ID(id, expected)                                              \
  do {                                                                       \
    if ((id) != (expected)) {                                                \
      fprintf(stderr, "%s = %d, expected %d\n", #id, (int)(id),              \
              (int)(expected));                                              \
      g_failures++;                                                          \
    }                                                                        \
  } while (0)

int main(void) {
  // Empty hand.
  EXPECT_ID(ITEM_AIR, 0);

  // Pickaxes and axes (tools, 256-383).
  EXPECT_ID(ITEM_WOODEN_PICKAXE, 256);
  EXPECT_ID(ITEM_STONE_PICKAXE, 260);
  EXPECT_ID(ITEM_IRON_PICKAXE, 264);
  EXPECT_ID(ITEM_DIAMOND_PICKAXE, 268);
  EXPECT_ID(ITEM_GOLDEN_PICKAXE, 272);
  EXPECT_ID(ITEM_WOODEN_AXE, 257);
  EXPECT_ID(ITEM_STONE_AXE, 261);
  EXPECT_ID(ITEM_IRON_AXE, 265);
  EXPECT_ID(ITEM_DIAMOND_AXE, 269);
  EXPECT_ID(ITEM_GOLDEN_AXE, 273);

  // Swords, bow and crossbow (weapons, 384-511).
  EXPECT_ID(ITEM_WOODEN_SWORD, 384);
  EXPECT_ID(ITEM_STONE_SWORD, 385);
  EXPECT_ID(ITEM_IRON_SWORD, 386);
  EXPECT_ID(ITEM_DIAMOND_SWORD, 387);
  EXPECT_ID(ITEM_GOLDEN_SWORD, 388);
  EXPECT_ID(ITEM_BOW, 390);
  EXPECT_ID(ITEM_CROSSBOW, 392);

  if (g_failures != 0) {
    fprintf(stderr, "%d item id mismatch(es)\n", g_failures);
    return 1;
  }
  printf("item_registry ids: ok\n");
  return 0;
}
