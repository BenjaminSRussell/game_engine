// src/inventory/item_registry.c
//
// Purpose: Implementation of the item registry system with comprehensive
// default item definitions for all game items.
//
#include <containers/hashmap.h>
#include "engine/include/core/logger.h"
#include <core/memory.h>
#include <ctype.h>
#include <inventory/item_registry.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  u32 magic;
  u32 version;
  u32 count;
  u32 capacity;
  u32 dependency_count;
} ItemRegistryFileHeader;

#define ITEM_REGISTRY_MAGIC 0x49545247u /* "ITRG" */

static u32 item_registry_hash_cstr_ptr(const void *key) {
  const char *s = *(const char *const *)key;
  if (!s)
    return 0;
  u32 h = 5381u;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    h = ((h << 5) + h) + (u32)(*p);
  }
  return h;
}

static bool item_registry_equals_cstr_ptr(const void *a, const void *b) {
  const char *sa = *(const char *const *)a;
  const char *sb = *(const char *const *)b;
  if (!sa || !sb)
    return sa == sb;
  return strcmp(sa, sb) == 0;
}

static bool item_registry_definition_valid(const ExtendedItemDefinition *def,
                                           char *reason, size_t reason_size) {
  if (!def) {
    if (reason && reason_size) {
      snprintf(reason, reason_size, "definition is NULL");
    }
    return false;
  }
  if (def->base.name[0] == '\0') {
    if (reason && reason_size) {
      snprintf(reason, reason_size, "missing name");
    }
    return false;
  }
  if (def->base.max_stack_size == 0) {
    if (reason && reason_size) {
      snprintf(reason, reason_size, "max_stack_size is zero");
    }
    return false;
  }
  if (def->base.item_type > ITEM_TYPE_MISC) {
    if (reason && reason_size) {
      snprintf(reason, reason_size, "invalid item type");
    }
    return false;
  }
  return true;
}

static bool item_registry_name_contains(const char *name, const char *needle) {
  if (!name || !needle || needle[0] == '\0')
    return false;

  size_t needle_len = strlen(needle);
  for (const char *p = name; *p; p++) {
    size_t i = 0;
    while (p[i] && i < needle_len) {
      if ((char)tolower((unsigned char)p[i]) !=
          (char)tolower((unsigned char)needle[i])) {
        break;
      }
      i++;
    }
    if (i == needle_len) {
      return true;
    }
  }
  return false;
}

static const char *item_registry_item_type_name(ItemType type) {
  switch (type) {
  case ITEM_TYPE_BLOCK:
    return "Block";
  case ITEM_TYPE_TOOL:
    return "Tool";
  case ITEM_TYPE_WEAPON:
    return "Weapon";
  case ITEM_TYPE_FOOD:
    return "Food";
  case ITEM_TYPE_MATERIAL:
    return "Material";
  case ITEM_TYPE_ARMOR:
    return "Armor";
  case ITEM_TYPE_MISC:
    return "Misc";
  default:
    return "Unknown";
  }
}

// Initialize item registry
void item_registry_init(ItemRegistry *registry) {
  if (!registry) {
    LOG_ERROR("item_registry_init: registry is NULL");
    return;
  }

  registry->capacity = 2048;
  registry->count = 0;
  registry->version = ITEM_REGISTRY_VERSION;
  memset(&registry->stats, 0, sizeof(registry->stats));
  registry->items = CALLOC(registry->capacity, sizeof(ExtendedItemDefinition));
  registry->name_lookup = hashmap_create(512, sizeof(const char *), sizeof(u32),
                                         item_registry_hash_cstr_ptr,
                                         item_registry_equals_cstr_ptr);
  registry->dependencies = NULL;
  registry->dependency_count = 0;
  registry->dependency_capacity = 0;

  LOG_INFO("Item registry initialized with %u items", registry->count);
}

// Free item registry
void item_registry_free(ItemRegistry *registry) {
  if (!registry)
    return;

  if (registry->items) {
    FREE(registry->items);
    registry->items = NULL;
  }

  if (registry->name_lookup) {
    hashmap_destroy((HashMap *)registry->name_lookup);
    registry->name_lookup = NULL;
  }

  if (registry->dependencies) {
    FREE(registry->dependencies);
    registry->dependencies = NULL;
  }

  registry->count = 0;
  registry->capacity = 0;
  registry->dependency_count = 0;
  registry->dependency_capacity = 0;
}

// Register a new item
bool item_registry_register(ItemRegistry *registry, u32 item_id,
                            const ExtendedItemDefinition *def) {
  char reason[128];
  if (!registry || !def)
    return false;
  if (item_id >= registry->capacity)
    return false;

  if (!item_registry_definition_valid(def, reason, sizeof(reason))) {
    registry->stats.invalid_items++;
    LOG_WARN("Item %u rejected: %s", item_id, reason);
    return false;
  }

  if (item_id < registry->count &&
      registry->items[item_id].base.name[0] != '\0') {
    registry->stats.duplicate_id_attempts++;
    LOG_WARN("Duplicate item id %u rejected (%s)", item_id, def->base.name);
    return false;
  }

  // Add to name lookup
  if (registry->name_lookup) {
    const char *key = def->base.name;
    u32 value = item_id;
    u32 *existing = (u32 *)hashmap_get((HashMap *)registry->name_lookup, &key);
    if (existing && *existing != item_id) {
      registry->stats.duplicate_name_attempts++;
      LOG_WARN("Duplicate item name rejected: %s", def->base.name);
      return false;
    }
  }

  // Copy definition
  registry->items[item_id] = *def;

  if (registry->name_lookup) {
    const char *key = def->base.name;
    u32 value = item_id;
    hashmap_insert((HashMap *)registry->name_lookup, &key, &value);
  }

  if (item_id >= registry->count) {
    registry->count = item_id + 1;
  }

  registry->stats.total_registered++;
  return true;
}

// Get item by ID
const ExtendedItemDefinition *item_registry_get(const ItemRegistry *registry,
                                                u32 item_id) {
  if (!registry || item_id >= registry->count)
    return NULL;
  return &registry->items[item_id];
}

// Get item by name
const ExtendedItemDefinition *
item_registry_get_by_name(const ItemRegistry *registry, const char *name) {
  if (!registry || !name || !registry->name_lookup)
    return NULL;

  const char *key = name;
  u32 *result = (u32 *)hashmap_get((HashMap *)registry->name_lookup, &key);
  if (!result)
    return NULL;

  u32 item_id = *result;
  return item_registry_get(registry, item_id);
}

bool item_registry_validate(const ItemRegistry *registry) {
  if (!registry)
    return false;

  bool ok = true;
  for (u32 i = 0; i < registry->count; i++) {
    const ExtendedItemDefinition *def = &registry->items[i];
    if (def->base.name[0] == '\0') {
      continue;
    }
    if (!item_registry_definition_valid(def, NULL, 0)) {
      ok = false;
    }
  }

  if (!item_registry_validate_dependencies(registry)) {
    ok = false;
  }

  if (!ok) {
    ((ItemRegistry *)registry)->stats.validation_failures++;
  }

  return ok;
}

bool item_registry_validate_dependencies(const ItemRegistry *registry) {
  if (!registry)
    return false;

  bool ok = true;
  for (u32 i = 0; i < registry->dependency_count; i++) {
    const ItemDependency *dep = &registry->dependencies[i];
    if (dep->item_id >= registry->count ||
        dep->requires_item_id >= registry->count) {
      ok = false;
      continue;
    }
    if (registry->items[dep->item_id].base.name[0] == '\0' ||
        registry->items[dep->requires_item_id].base.name[0] == '\0') {
      ok = false;
    }
  }

  if (!ok) {
    ((ItemRegistry *)registry)->stats.dependency_violations++;
  }

  return ok;
}

bool item_registry_add_dependency(ItemRegistry *registry, u32 item_id,
                                  u32 requires_item_id) {
  if (!registry)
    return false;

  if (registry->dependency_count >= registry->dependency_capacity) {
    u32 new_capacity = registry->dependency_capacity == 0
                           ? 32
                           : registry->dependency_capacity * 2;
    ItemDependency *deps =
        REALLOC(registry->dependencies, new_capacity * sizeof(ItemDependency));
    if (!deps)
      return false;
    registry->dependencies = deps;
    registry->dependency_capacity = new_capacity;
  }

  registry->dependencies[registry->dependency_count++] = (ItemDependency){
      .item_id = item_id, .requires_item_id = requires_item_id};
  return true;
}

const ItemRegistryStats *item_registry_get_stats(const ItemRegistry *registry) {
  if (!registry)
    return NULL;
  return &registry->stats;
}

u32 item_registry_find_by_type(const ItemRegistry *registry, ItemType type,
                               u32 *out_ids, u32 max_out) {
  if (!registry || !out_ids || max_out == 0)
    return 0;

  u32 count = 0;
  for (u32 i = 0; i < registry->count && count < max_out; i++) {
    const ExtendedItemDefinition *def = &registry->items[i];
    if (def->base.name[0] == '\0')
      continue;
    if (def->base.item_type == type) {
      out_ids[count++] = i;
    }
  }
  return count;
}

u32 item_registry_find_by_name_contains(const ItemRegistry *registry,
                                        const char *needle, u32 *out_ids,
                                        u32 max_out) {
  if (!registry || !needle || !out_ids || max_out == 0)
    return 0;

  u32 count = 0;
  for (u32 i = 0; i < registry->count && count < max_out; i++) {
    const ExtendedItemDefinition *def = &registry->items[i];
    if (def->base.name[0] == '\0')
      continue;
    if (item_registry_name_contains(def->base.name, needle)) {
      out_ids[count++] = i;
    }
  }
  return count;
}

bool item_registry_save(const ItemRegistry *registry, const char *path) {
  if (!registry || !path)
    return false;

  FILE *fp = fopen(path, "wb");
  if (!fp)
    return false;

  ItemRegistryFileHeader header = {.magic = ITEM_REGISTRY_MAGIC,
                                   .version = registry->version,
                                   .count = registry->count,
                                   .capacity = registry->capacity,
                                   .dependency_count =
                                       registry->dependency_count};

  bool ok = fwrite(&header, sizeof(header), 1, fp) == 1;
  if (ok && registry->count > 0) {
    ok = fwrite(registry->items, sizeof(ExtendedItemDefinition),
                registry->count, fp) == registry->count;
  }
  if (ok && registry->dependency_count > 0) {
    ok = fwrite(registry->dependencies, sizeof(ItemDependency),
                registry->dependency_count, fp) == registry->dependency_count;
  }

  fclose(fp);
  return ok;
}

bool item_registry_load(ItemRegistry *registry, const char *path) {
  if (!registry || !path)
    return false;

  FILE *fp = fopen(path, "rb");
  if (!fp)
    return false;

  ItemRegistryFileHeader header = {0};
  if (fread(&header, sizeof(header), 1, fp) != 1) {
    fclose(fp);
    return false;
  }

  if (header.magic != ITEM_REGISTRY_MAGIC) {
    fclose(fp);
    return false;
  }

  item_registry_free(registry);

  registry->capacity = header.capacity > 0 ? header.capacity : header.count;
  registry->count = header.count;
  registry->version = header.version;
  memset(&registry->stats, 0, sizeof(registry->stats));
  registry->items = CALLOC(registry->capacity, sizeof(ExtendedItemDefinition));
  registry->name_lookup = hashmap_create(512, sizeof(const char *), sizeof(u32),
                                         item_registry_hash_cstr_ptr,
                                         item_registry_equals_cstr_ptr);
  registry->dependencies = NULL;
  registry->dependency_count = 0;
  registry->dependency_capacity = 0;

  bool ok = true;
  if (registry->count > 0) {
    ok = fread(registry->items, sizeof(ExtendedItemDefinition), registry->count,
               fp) == registry->count;
  }

  if (ok && header.dependency_count > 0) {
    registry->dependencies =
        CALLOC(header.dependency_count, sizeof(ItemDependency));
    registry->dependency_capacity = header.dependency_count;
    registry->dependency_count = header.dependency_count;
    ok = fread(registry->dependencies, sizeof(ItemDependency),
               header.dependency_count, fp) == header.dependency_count;
  }

  fclose(fp);

  if (!ok) {
    item_registry_free(registry);
    return false;
  }

  for (u32 i = 0; i < registry->count; i++) {
    if (registry->items[i].base.name[0] == '\0')
      continue;
    const char *key = registry->items[i].base.name;
    u32 value = i;
    hashmap_insert((HashMap *)registry->name_lookup, &key, &value);
  }

  if (registry->version != ITEM_REGISTRY_VERSION) {
    item_registry_migrate(registry, registry->version, ITEM_REGISTRY_VERSION);
  }

  item_registry_validate(registry);
  return true;
}

bool item_registry_migrate(ItemRegistry *registry, u32 from_version,
                           u32 to_version) {
  if (!registry)
    return false;
  if (from_version == to_version)
    return true;

  registry->version = to_version;
  registry->stats.migrations_applied++;
  LOG_INFO("Item registry migrated from v%u to v%u", from_version, to_version);
  return true;
}

bool item_registry_hot_reload(ItemRegistry *registry) {
  if (!registry)
    return false;

  u32 hot_reload_count = registry->stats.hot_reload_count + 1;
  item_registry_free(registry);
  item_registry_init(registry);
  registry->stats.hot_reload_count = hot_reload_count;
  return true;
}

bool item_registry_write_docs(const ItemRegistry *registry, const char *path) {
  if (!registry || !path)
    return false;

  FILE *fp = fopen(path, "w");
  if (!fp)
    return false;

  fprintf(fp, "# Item Registry\n\n");
  fprintf(fp, "Version: %u\n\n", registry->version);
  fprintf(fp, "| ID | Name | Type | Stack | Durability |\n");
  fprintf(fp, "|---:|:-----|:-----|------:|-----------:|\n");

  for (u32 i = 0; i < registry->count; i++) {
    const ExtendedItemDefinition *def = &registry->items[i];
    if (def->base.name[0] == '\0')
      continue;
    fprintf(fp, "| %u | %s | %s | %u | %.0f |\n", i, def->base.name,
            item_registry_item_type_name(def->base.item_type),
            def->base.max_stack_size, def->base.max_durability);
  }

  fclose(fp);
  return true;
}

// Helper: Check if item is food
bool item_is_food(const ExtendedItemDefinition *item) {
  return item && item->base.item_type == ITEM_TYPE_FOOD;
}

// Helper: Check if item is tool
bool item_is_tool(const ExtendedItemDefinition *item) {
  return item && item->base.item_type == ITEM_TYPE_TOOL;
}

// Helper: Check if item is weapon
bool item_is_weapon(const ExtendedItemDefinition *item) {
  return item && item->base.item_type == ITEM_TYPE_WEAPON;
}

// Helper: Check if item is armor
bool item_is_armor(const ExtendedItemDefinition *item) {
  return item && item->base.item_type == ITEM_TYPE_ARMOR;
}

// Helper: Check if item is fuel
bool item_is_fuel(const ExtendedItemDefinition *item) {
  return item && item->fuel_value > 0.0f;
}

// Macro for registering items
#define REGISTER_ITEM(id, def) item_registry_register(registry, id, &def)

// Initialize all default items
void item_registry_init_defaults(ItemRegistry *registry) {
  if (!registry)
    return;

  // === TOOLS ===

  // Wooden tools
  ExtendedItemDefinition wooden_pickaxe = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_TOOL,
               .name = "Wooden Pickaxe",
               .description = "Basic mining tool",
               .max_stack_size = 1,
               .durability = 59,
               .max_durability = 59},
      .properties = {.tool = {.mining_speed = 2.0f,
                              .harvest_level = 1,
                              .attack_damage = 2.0f,
                              .attack_speed = 1.2f}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_PLANKS,
      .fuel_value = 10.0f,
      .rarity = 0};
  REGISTER_ITEM(ITEM_WOODEN_PICKAXE, wooden_pickaxe);

  ExtendedItemDefinition wooden_axe = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_TOOL,
               .name = "Wooden Axe",
               .description = "Chops wood faster",
               .max_stack_size = 1,
               .durability = 59,
               .max_durability = 59},
      .properties = {.tool = {.mining_speed = 2.0f,
                              .harvest_level = 1,
                              .attack_damage = 7.0f,
                              .attack_speed = 0.8f}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_PLANKS,
      .fuel_value = 10.0f,
      .rarity = 0};
  REGISTER_ITEM(ITEM_WOODEN_AXE, wooden_axe);

  ExtendedItemDefinition wooden_shovel = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_TOOL,
               .name = "Wooden Shovel",
               .description = "Digs dirt and sand faster",
               .max_stack_size = 1,
               .durability = 59,
               .max_durability = 59},
      .properties = {.tool = {.mining_speed = 2.0f,
                              .harvest_level = 1,
                              .attack_damage = 2.5f,
                              .attack_speed = 1.0f}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_PLANKS,
      .fuel_value = 10.0f,
      .rarity = 0};
  REGISTER_ITEM(ITEM_WOODEN_SHOVEL, wooden_shovel);

  // Stone tools
  ExtendedItemDefinition stone_pickaxe = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_TOOL,
               .name = "Stone Pickaxe",
               .description = "Improved mining tool",
               .max_stack_size = 1,
               .durability = 131,
               .max_durability = 131},
      .properties = {.tool = {.mining_speed = 4.0f,
                              .harvest_level = 2,
                              .attack_damage = 3.0f,
                              .attack_speed = 1.2f}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_COBBLESTONE,
      .fuel_value = 0.0f,
      .rarity = 0};
  REGISTER_ITEM(ITEM_STONE_PICKAXE, stone_pickaxe);

  ExtendedItemDefinition stone_axe = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_TOOL,
               .name = "Stone Axe",
               .description = "Efficient wood chopping",
               .max_stack_size = 1,
               .durability = 131,
               .max_durability = 131},
      .properties = {.tool = {.mining_speed = 4.0f,
                              .harvest_level = 2,
                              .attack_damage = 9.0f,
                              .attack_speed = 0.8f}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_COBBLESTONE,
      .fuel_value = 0.0f,
      .rarity = 0};
  REGISTER_ITEM(ITEM_STONE_AXE, stone_axe);

  ExtendedItemDefinition stone_shovel = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_TOOL,
               .name = "Stone Shovel",
               .description = "Efficient digging tool",
               .max_stack_size = 1,
               .durability = 131,
               .max_durability = 131},
      .properties = {.tool = {.mining_speed = 4.0f,
                              .harvest_level = 2,
                              .attack_damage = 3.5f,
                              .attack_speed = 1.0f}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_COBBLESTONE,
      .fuel_value = 0.0f,
      .rarity = 0};
  REGISTER_ITEM(ITEM_STONE_SHOVEL, stone_shovel);

  // Iron tools
  ExtendedItemDefinition iron_pickaxe = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_TOOL,
               .name = "Iron Pickaxe",
               .description = "Fast and durable mining tool",
               .max_stack_size = 1,
               .durability = 250,
               .max_durability = 250},
      .properties = {.tool = {.mining_speed = 6.0f,
                              .harvest_level = 3,
                              .attack_damage = 4.0f,
                              .attack_speed = 1.2f}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_IRON_INGOT,
      .fuel_value = 0.0f,
      .rarity = 1};
  REGISTER_ITEM(ITEM_IRON_PICKAXE, iron_pickaxe);

  ExtendedItemDefinition iron_axe = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_TOOL,
               .name = "Iron Axe",
               .description = "Rapid wood chopping",
               .max_stack_size = 1,
               .durability = 250,
               .max_durability = 250},
      .properties = {.tool = {.mining_speed = 6.0f,
                              .harvest_level = 3,
                              .attack_damage = 9.0f,
                              .attack_speed = 0.9f}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_IRON_INGOT,
      .fuel_value = 0.0f,
      .rarity = 1};
  REGISTER_ITEM(ITEM_IRON_AXE, iron_axe);

  ExtendedItemDefinition iron_shovel = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_TOOL,
               .name = "Iron Shovel",
               .description = "Professional digging tool",
               .max_stack_size = 1,
               .durability = 250,
               .max_durability = 250},
      .properties = {.tool = {.mining_speed = 6.0f,
                              .harvest_level = 3,
                              .attack_damage = 4.5f,
                              .attack_speed = 1.0f}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_IRON_INGOT,
      .fuel_value = 0.0f,
      .rarity = 1};
  REGISTER_ITEM(ITEM_IRON_SHOVEL, iron_shovel);

  // Diamond tools
  ExtendedItemDefinition diamond_pickaxe = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_TOOL,
               .name = "Diamond Pickaxe",
               .description = "The finest mining tool",
               .max_stack_size = 1,
               .durability = 1561,
               .max_durability = 1561},
      .properties = {.tool = {.mining_speed = 8.0f,
                              .harvest_level = 4,
                              .attack_damage = 5.0f,
                              .attack_speed = 1.2f}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_DIAMOND,
      .fuel_value = 0.0f,
      .rarity = 3};
  REGISTER_ITEM(ITEM_DIAMOND_PICKAXE, diamond_pickaxe);

  ExtendedItemDefinition diamond_axe = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_TOOL,
               .name = "Diamond Axe",
               .description = "Ultimate wood chopping tool",
               .max_stack_size = 1,
               .durability = 1561,
               .max_durability = 1561},
      .properties = {.tool = {.mining_speed = 8.0f,
                              .harvest_level = 4,
                              .attack_damage = 9.0f,
                              .attack_speed = 1.0f}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_DIAMOND,
      .fuel_value = 0.0f,
      .rarity = 3};
  REGISTER_ITEM(ITEM_DIAMOND_AXE, diamond_axe);

  ExtendedItemDefinition diamond_shovel = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_TOOL,
               .name = "Diamond Shovel",
               .description = "Unmatched digging speed",
               .max_stack_size = 1,
               .durability = 1561,
               .max_durability = 1561},
      .properties = {.tool = {.mining_speed = 8.0f,
                              .harvest_level = 4,
                              .attack_damage = 5.5f,
                              .attack_speed = 1.0f}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_DIAMOND,
      .fuel_value = 0.0f,
      .rarity = 3};
  REGISTER_ITEM(ITEM_DIAMOND_SHOVEL, diamond_shovel);

  // === WEAPONS ===

  ExtendedItemDefinition wooden_sword = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_WEAPON,
               .name = "Wooden Sword",
               .description = "Basic melee weapon",
               .max_stack_size = 1,
               .durability = 59,
               .max_durability = 59},
      .properties = {.weapon = {.attack_damage = 4.0f,
                                .attack_speed = 1.6f,
                                .knockback = 0.0f,
                                .reach = 3.0f,
                                .is_ranged = false}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_PLANKS,
      .fuel_value = 10.0f,
      .rarity = 0};
  REGISTER_ITEM(ITEM_WOODEN_SWORD, wooden_sword);

  ExtendedItemDefinition stone_sword = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_WEAPON,
               .name = "Stone Sword",
               .description = "Sturdy combat weapon",
               .max_stack_size = 1,
               .durability = 131,
               .max_durability = 131},
      .properties = {.weapon = {.attack_damage = 5.0f,
                                .attack_speed = 1.6f,
                                .knockback = 0.0f,
                                .reach = 3.0f,
                                .is_ranged = false}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_COBBLESTONE,
      .fuel_value = 0.0f,
      .rarity = 0};
  REGISTER_ITEM(ITEM_STONE_SWORD, stone_sword);

  ExtendedItemDefinition iron_sword = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_WEAPON,
               .name = "Iron Sword",
               .description = "Reliable combat weapon",
               .max_stack_size = 1,
               .durability = 250,
               .max_durability = 250},
      .properties = {.weapon = {.attack_damage = 6.0f,
                                .attack_speed = 1.6f,
                                .knockback = 0.0f,
                                .reach = 3.0f,
                                .is_ranged = false}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_IRON_INGOT,
      .fuel_value = 0.0f,
      .rarity = 1};
  REGISTER_ITEM(ITEM_IRON_SWORD, iron_sword);

  ExtendedItemDefinition diamond_sword = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_WEAPON,
               .name = "Diamond Sword",
               .description = "Elite combat weapon",
               .max_stack_size = 1,
               .durability = 1561,
               .max_durability = 1561},
      .properties = {.weapon = {.attack_damage = 7.0f,
                                .attack_speed = 1.6f,
                                .knockback = 0.0f,
                                .reach = 3.0f,
                                .is_ranged = false}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_DIAMOND,
      .fuel_value = 0.0f,
      .rarity = 3};
  REGISTER_ITEM(ITEM_DIAMOND_SWORD, diamond_sword);

  ExtendedItemDefinition bow = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_WEAPON,
               .name = "Bow",
               .description = "Ranged weapon",
               .max_stack_size = 1,
               .durability = 384,
               .max_durability = 384},
      .properties = {.weapon = {.attack_damage = 9.0f,
                                .attack_speed = 1.0f,
                                .knockback = 0.0f,
                                .reach = 50.0f,
                                .is_ranged = true,
                                .projectile_speed = 40.0f}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_STRING,
      .fuel_value = 0.0f,
      .rarity = 1};
  REGISTER_ITEM(ITEM_BOW, bow);

  ExtendedItemDefinition arrow = {.base = {.block_id = 0,
                                           .item_type = ITEM_TYPE_MATERIAL,
                                           .name = "Arrow",
                                           .description = "Ammunition for bows",
                                           .max_stack_size = 64,
                                           .durability = 0,
                                           .max_durability = 0},
                                  .is_stackable = true,
                                  .is_repairable = false,
                                  .rarity = 0};
  REGISTER_ITEM(ITEM_ARROW, arrow);

  ExtendedItemDefinition shield = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_WEAPON,
               .name = "Shield",
               .description = "Blocks incoming attacks",
               .max_stack_size = 1,
               .durability = 336,
               .max_durability = 336},
      .properties = {.weapon = {.attack_damage = 0.0f,
                                .attack_speed = 0.0f,
                                .knockback = 1.0f,
                                .reach = 0.0f,
                                .is_ranged = false}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_PLANKS,
      .fuel_value = 0.0f,
      .rarity = 1};
  REGISTER_ITEM(ITEM_SHIELD, shield);

  // === ARMOR ===

  // Iron armor
  ExtendedItemDefinition iron_helmet = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_ARMOR,
               .name = "Iron Helmet",
               .description = "Protects your head",
               .max_stack_size = 1,
               .durability = 165,
               .max_durability = 165},
      .properties = {.armor = {.defense = 2.0f,
                               .toughness = 0.0f,
                               .knockback_resistance = 0.0f,
                               .armor_slot = 0}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_IRON_INGOT,
      .rarity = 1};
  REGISTER_ITEM(ITEM_IRON_HELMET, iron_helmet);

  ExtendedItemDefinition iron_chestplate = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_ARMOR,
               .name = "Iron Chestplate",
               .description = "Protects your torso",
               .max_stack_size = 1,
               .durability = 240,
               .max_durability = 240},
      .properties = {.armor = {.defense = 6.0f,
                               .toughness = 0.0f,
                               .knockback_resistance = 0.0f,
                               .armor_slot = 1}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_IRON_INGOT,
      .rarity = 1};
  REGISTER_ITEM(ITEM_IRON_CHESTPLATE, iron_chestplate);

  ExtendedItemDefinition iron_leggings = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_ARMOR,
               .name = "Iron Leggings",
               .description = "Protects your legs",
               .max_stack_size = 1,
               .durability = 225,
               .max_durability = 225},
      .properties = {.armor = {.defense = 5.0f,
                               .toughness = 0.0f,
                               .knockback_resistance = 0.0f,
                               .armor_slot = 2}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_IRON_INGOT,
      .rarity = 1};
  REGISTER_ITEM(ITEM_IRON_LEGGINGS, iron_leggings);

  ExtendedItemDefinition iron_boots = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_ARMOR,
               .name = "Iron Boots",
               .description = "Protects your feet",
               .max_stack_size = 1,
               .durability = 195,
               .max_durability = 195},
      .properties = {.armor = {.defense = 2.0f,
                               .toughness = 0.0f,
                               .knockback_resistance = 0.0f,
                               .armor_slot = 3}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_IRON_INGOT,
      .rarity = 1};
  REGISTER_ITEM(ITEM_IRON_BOOTS, iron_boots);

  // Diamond armor
  ExtendedItemDefinition diamond_helmet = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_ARMOR,
               .name = "Diamond Helmet",
               .description = "Maximum head protection",
               .max_stack_size = 1,
               .durability = 363,
               .max_durability = 363},
      .properties = {.armor = {.defense = 3.0f,
                               .toughness = 2.0f,
                               .knockback_resistance = 0.0f,
                               .armor_slot = 0}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_DIAMOND,
      .rarity = 3};
  REGISTER_ITEM(ITEM_DIAMOND_HELMET, diamond_helmet);

  ExtendedItemDefinition diamond_chestplate = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_ARMOR,
               .name = "Diamond Chestplate",
               .description = "Maximum torso protection",
               .max_stack_size = 1,
               .durability = 528,
               .max_durability = 528},
      .properties = {.armor = {.defense = 8.0f,
                               .toughness = 2.0f,
                               .knockback_resistance = 0.0f,
                               .armor_slot = 1}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_DIAMOND,
      .rarity = 3};
  REGISTER_ITEM(ITEM_DIAMOND_CHESTPLATE, diamond_chestplate);

  ExtendedItemDefinition diamond_leggings = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_ARMOR,
               .name = "Diamond Leggings",
               .description = "Maximum leg protection",
               .max_stack_size = 1,
               .durability = 495,
               .max_durability = 495},
      .properties = {.armor = {.defense = 6.0f,
                               .toughness = 2.0f,
                               .knockback_resistance = 0.0f,
                               .armor_slot = 2}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_DIAMOND,
      .rarity = 3};
  REGISTER_ITEM(ITEM_DIAMOND_LEGGINGS, diamond_leggings);

  ExtendedItemDefinition diamond_boots = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_ARMOR,
               .name = "Diamond Boots",
               .description = "Maximum feet protection",
               .max_stack_size = 1,
               .durability = 429,
               .max_durability = 429},
      .properties = {.armor = {.defense = 3.0f,
                               .toughness = 2.0f,
                               .knockback_resistance = 0.0f,
                               .armor_slot = 3}},
      .is_stackable = false,
      .is_repairable = true,
      .repair_material_id = ITEM_DIAMOND,
      .rarity = 3};
  REGISTER_ITEM(ITEM_DIAMOND_BOOTS, diamond_boots);

  // === FOOD ===

  ExtendedItemDefinition apple = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Apple",
               .description = "Crisp and nutritious",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 4.0f,
                              .saturation_modifier = 0.3f,
                              .eat_duration = 1.6f,
                              .has_effects = false,
                              .quality = 1.0f,
                              .spoil_time = 1200.0f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_APPLE, apple);

  ExtendedItemDefinition golden_apple = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Golden Apple",
               .description = "Grants regeneration and absorption",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 4.0f,
                              .saturation_modifier = 1.2f,
                              .eat_duration = 1.6f,
                              .has_effects = true,
                              .effect_id = 1,
                              .effect_chance = 1.0f,
                              .quality = 1.5f,
                              .spoil_time = 0.0f}},
      .is_stackable = true,
      .rarity = 3};
  REGISTER_ITEM(ITEM_GOLDEN_APPLE, golden_apple);

  ExtendedItemDefinition bread = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Bread",
               .description = "Filling and satisfying",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 5.0f,
                              .saturation_modifier = 0.6f,
                              .eat_duration = 1.6f,
                              .has_effects = false,
                              .quality = 1.0f,
                              .spoil_time = 900.0f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_BREAD, bread);

  ExtendedItemDefinition cooked_porkchop = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Cooked Porkchop",
               .description = "Delicious and nutritious",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 8.0f,
                              .saturation_modifier = 0.8f,
                              .eat_duration = 1.6f,
                              .has_effects = false,
                              .quality = 1.2f,
                              .spoil_time = 600.0f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_COOKED_PORKCHOP, cooked_porkchop);

  ExtendedItemDefinition raw_porkchop = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Raw Porkchop",
               .description = "Best cooked before eating",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 3.0f,
                              .saturation_modifier = 0.3f,
                              .eat_duration = 1.6f,
                              .has_effects = false,
                              .quality = 0.8f,
                              .spoil_time = 300.0f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_RAW_PORKCHOP, raw_porkchop);

  ExtendedItemDefinition cooked_beef = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Steak",
               .description = "Highly nutritious meat",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 8.0f,
                              .saturation_modifier = 0.8f,
                              .eat_duration = 1.6f,
                              .has_effects = false,
                              .quality = 1.2f,
                              .spoil_time = 600.0f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_COOKED_BEEF, cooked_beef);

  ExtendedItemDefinition raw_beef = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Raw Beef",
               .description = "Requires cooking",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 3.0f,
                              .saturation_modifier = 0.3f,
                              .eat_duration = 1.6f,
                              .has_effects = false,
                              .quality = 0.8f,
                              .spoil_time = 300.0f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_RAW_BEEF, raw_beef);

  ExtendedItemDefinition cooked_chicken = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Cooked Chicken",
               .description = "Light and tasty",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 6.0f,
                              .saturation_modifier = 0.6f,
                              .eat_duration = 1.6f,
                              .has_effects = false,
                              .quality = 1.1f,
                              .spoil_time = 600.0f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_COOKED_CHICKEN, cooked_chicken);

  ExtendedItemDefinition raw_chicken = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Raw Chicken",
               .description = "May cause food poisoning",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 2.0f,
                              .saturation_modifier = 0.3f,
                              .eat_duration = 1.6f,
                              .has_effects = true,
                              .effect_id = 0,
                              .effect_chance = 0.3f,
                              .quality = 0.5f,
                              .spoil_time = 300.0f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_RAW_CHICKEN, raw_chicken);

  ExtendedItemDefinition carrot = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Carrot",
               .description = "Crunchy vegetable",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 3.0f,
                              .saturation_modifier = 0.6f,
                              .eat_duration = 1.6f,
                              .has_effects = false,
                              .quality = 1.0f,
                              .spoil_time = 1200.0f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_CARROT, carrot);

  ExtendedItemDefinition potato = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Potato",
               .description = "Plain but filling",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 1.0f,
                              .saturation_modifier = 0.3f,
                              .eat_duration = 1.6f,
                              .has_effects = false,
                              .quality = 1.0f,
                              .spoil_time = 1200.0f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_POTATO, potato);

  ExtendedItemDefinition red_mushroom = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Red Mushroom",
               .description = "A spotted red fungus",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_RED_MUSHROOM, red_mushroom);

  ExtendedItemDefinition brown_mushroom = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Brown Mushroom",
               .description = "A simple brown fungus",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_BROWN_MUSHROOM, brown_mushroom);

  ExtendedItemDefinition baked_potato = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Baked Potato",
               .description = "Warm and filling",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 5.0f,
                              .saturation_modifier = 0.6f,
                              .eat_duration = 1.6f,
                              .has_effects = false,
                              .quality = 1.1f,
                              .spoil_time = 900.0f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_BAKED_POTATO, baked_potato);

  ExtendedItemDefinition poisonous_potato = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Poisonous Potato",
               .description = "Dangerous to eat",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 2.0f,
                              .saturation_modifier = 0.3f,
                              .eat_duration = 1.6f,
                              .has_effects = true,
                              .effect_id = 0,
                              .effect_chance = 0.6f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_POISONOUS_POTATO, poisonous_potato);

  ExtendedItemDefinition spoiled_food = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Spoiled Food",
               .description =
                   "Smells funky. Eating this is probably a bad idea.",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 1.0f,
                              .saturation_modifier = 0.1f,
                              .eat_duration = 1.6f,
                              .has_effects = true,
                              .effect_id = 0,
                              .effect_chance = 0.8f,
                              .quality = 0.1f,
                              .spoil_time = 0.0f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_SPOILED_FOOD, spoiled_food);

  ExtendedItemDefinition pumpkin_pie = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Pumpkin Pie",
               .description = "A holiday favorite",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 8.0f,
                              .saturation_modifier = 0.6f,
                              .eat_duration = 1.6f,
                              .has_effects = false,
                              .quality = 1.2f,
                              .spoil_time = 1200.0f}},
      .is_stackable = true,
      .rarity = 1};
  REGISTER_ITEM(ITEM_PUMPKIN_PIE, pumpkin_pie);

  ExtendedItemDefinition cake = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Cake",
               .description = "The cake is a lie",
               .max_stack_size = 1,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 14.0f,
                              .saturation_modifier = 0.4f,
                              .eat_duration = 1.0f,
                              .has_effects = false,
                              .quality = 1.5f,
                              .spoil_time = 1800.0f}},
      .is_stackable = false,
      .rarity = 2};
  REGISTER_ITEM(ITEM_CAKE, cake);

  ExtendedItemDefinition mushroom_stew = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Mushroom Stew",
               .description = "Earthy and filling",
               .max_stack_size = 1,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 6.0f,
                              .saturation_modifier = 0.6f,
                              .eat_duration = 2.0f,
                              .has_effects = false,
                              .quality = 1.3f,
                              .spoil_time = 600.0f}},
      .is_stackable = false,
      .rarity = 1};
  REGISTER_ITEM(ITEM_MUSHROOM_STEW, mushroom_stew);

  ExtendedItemDefinition bowl = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Bowl",
               .description = "A simple wooden bowl.",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_BOWL, bowl);

  ExtendedItemDefinition hearty_stew = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Hearty Stew",
               .description = "A thick, nutritious stew.",
               .max_stack_size = 1,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 10.0f,
                              .saturation_modifier = 1.2f,
                              .eat_duration = 2.4f,
                              .has_effects = false,
                              .quality = 1.8f,
                              .spoil_time = 900.0f}},
      .is_stackable = false,
      .rarity = 1};
  REGISTER_ITEM(ITEM_HEARTY_STEW, hearty_stew);

  // === MATERIALS (768-1023) ===

  ExtendedItemDefinition stick = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Stick",
               .description = "Basic crafting material",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .fuel_value = 5.0f,
      .rarity = 0};
  REGISTER_ITEM(ITEM_STICK, stick);

  ExtendedItemDefinition coal = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Coal",
               .description = "Fuel and crafting material",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .fuel_value = 80.0f,
      .rarity = 0};
  REGISTER_ITEM(ITEM_COAL, coal);

  ExtendedItemDefinition iron_ingot = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Iron Ingot",
               .description = "Refined iron metal",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 1};
  REGISTER_ITEM(ITEM_IRON_INGOT, iron_ingot);

  ExtendedItemDefinition gold_ingot = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Gold Ingot",
               .description = "Valuable precious metal",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 2};
  REGISTER_ITEM(ITEM_GOLD_INGOT, gold_ingot);

  ExtendedItemDefinition diamond = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Diamond",
               .description = "Rare and precious gem",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 3};
  REGISTER_ITEM(ITEM_DIAMOND, diamond);

  ExtendedItemDefinition emerald = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Emerald",
               .description = "Valuable trading currency",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 3};
  REGISTER_ITEM(ITEM_EMERALD, emerald);

  ExtendedItemDefinition string_item = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "String",
               .description = "Spider silk thread",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_STRING, string_item);

  ExtendedItemDefinition leather = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Leather",
               .description = "Animal hide material",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_LEATHER, leather);

  ExtendedItemDefinition feather = {.base = {.block_id = 0,
                                             .item_type = ITEM_TYPE_MATERIAL,
                                             .name = "Feather",
                                             .description = "Light and fluffy",
                                             .max_stack_size = 64,
                                             .durability = 0,
                                             .max_durability = 0},
                                    .is_stackable = true,
                                    .rarity = 0};
  REGISTER_ITEM(ITEM_FEATHER, feather);

  // Tech materials
  ExtendedItemDefinition rubber = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Rubber",
               .description = "Flexible industrial material",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 1};
  REGISTER_ITEM(ITEM_RUBBER, rubber);

  ExtendedItemDefinition circuit = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Electronic Circuit",
               .description = "Advanced tech component",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 2};
  REGISTER_ITEM(ITEM_CIRCUIT, circuit);

  ExtendedItemDefinition battery = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Battery",
               .description = "Stores electrical energy",
               .max_stack_size = 16,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 2};
  REGISTER_ITEM(ITEM_BATTERY, battery);

  // === MISC ===

  ExtendedItemDefinition bucket = {.base = {.block_id = 0,
                                            .item_type = ITEM_TYPE_MISC,
                                            .name = "Bucket",
                                            .description = "Holds liquids",
                                            .max_stack_size = 16,
                                            .durability = 0,
                                            .max_durability = 0},
                                   .is_stackable = true,
                                   .rarity = 0};
  REGISTER_ITEM(ITEM_BUCKET, bucket);

  ExtendedItemDefinition milk_bucket = {
      .base =
          {.block_id = 0,
           .item_type = ITEM_TYPE_FOOD,
           .name = "Milk Bucket",
           .description =
               "Fresh milk. Clears all status effects (not yet implemented).",
           .max_stack_size = 1,
           .durability = 0,
           .max_durability = 0},
      .properties = {.food = {.hunger_restored = 0.0f,
                              .saturation_modifier = 0.0f,
                              .eat_duration = 1.6f,
                              .has_effects = false}},
      .is_stackable = false,
      .rarity = 0};
  REGISTER_ITEM(ITEM_MILK_BUCKET, milk_bucket);

  ExtendedItemDefinition sugar = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MATERIAL,
               .name = "Sugar",
               .description = "Sweet and white powder",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_SUGAR, sugar);

  ExtendedItemDefinition egg = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_FOOD,
               .name = "Egg",
               .description = "Can be used in cooking",
               .max_stack_size = 16,
               .durability = 0,
               .max_durability = 0},
      .properties = {.food = {.hunger_restored = 0.0f,
                              .saturation_modifier = 0.0f,
                              .eat_duration = 1.6f,
                              .has_effects = false,
                              .quality = 0.5f,
                              .spoil_time = 1200.0f}},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_EGG, egg);

  ExtendedItemDefinition book = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MISC,
               .name = "Book",
               .description = "For writing and enchanting",
               .max_stack_size = 64,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = true,
      .rarity = 0};
  REGISTER_ITEM(ITEM_BOOK, book);

  ExtendedItemDefinition minecart = {
      .base = {.block_id = 0,
               .item_type = ITEM_TYPE_MISC,
               .name = "Minecart",
               .description = "Rail transportation",
               .max_stack_size = 1,
               .durability = 0,
               .max_durability = 0},
      .is_stackable = false,
      .rarity = 1};
  REGISTER_ITEM(ITEM_MINECART, minecart);

  ExtendedItemDefinition boat = {.base = {.block_id = 0,
                                          .item_type = ITEM_TYPE_MISC,
                                          .name = "Boat",
                                          .description = "Water transportation",
                                          .max_stack_size = 1,
                                          .durability = 0,
                                          .max_durability = 0},
                                 .is_stackable = false,
                                 .fuel_value = 60.0f,
                                 .rarity = 0};
  REGISTER_ITEM(ITEM_BOAT, boat);

  LOG_INFO("Initialized %u default items", registry->count);
}
