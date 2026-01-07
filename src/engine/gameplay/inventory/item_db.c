#include <gameplay/inventory/item_database.h>
#include <common.h>
#include <core/hashmap.h>
#include <core/memory.h>
#include <core/logger.h>
#include <core/json.h>
#include <stdlib.h>
#include <string.h>

// Item database global state
static struct {
  Item *items; // Array of item templates
  u32 item_count;
  u32 capacity;
  HashMap *id_map;   // ID -> index mapping
  HashMap *name_map; // Name -> index mapping
  bool initialized;
  u32 next_instance_id;
} g_item_db = {0};

// Helper functions for u32 hashmap
static u32 hash_u32(const void *key) {
  return *(const u32 *)key;
}

static bool equals_u32(const void *a, const void *b) {
  return *(const u32 *)a == *(const u32 *)b;
}

// ============================================================================
// DATABASE MANAGEMENT
// ============================================================================

bool item_database_init(u32 max_items) {
  LOG_DEBUG("item_database_init: start");
  if (g_item_db.initialized)
    return true;

  LOG_DEBUG("item_database_init: allocating items array");
  g_item_db.capacity = max_items;
  g_item_db.items =
      (Item *)MALLOC_TAGGED(sizeof(Item) * max_items, MEMORY_TAG_GAMEPLAY);
  if (!g_item_db.items)
    return false;

  LOG_DEBUG("item_database_init: creating id_map");
  g_item_db.id_map = hashmap_create(1024, sizeof(u32), sizeof(u32), hash_u32, equals_u32);
  LOG_DEBUG("item_database_init: id_map created");
  
  LOG_DEBUG("item_database_init: creating name_map");
  g_item_db.name_map = hashmap_create(1024, sizeof(char *), sizeof(u32),
                                      hash_string, equals_string);
  LOG_DEBUG("item_database_init: name_map created");

  if (!g_item_db.id_map || !g_item_db.name_map) {
    LOG_ERROR("item_database_init: hashmap creation failed");
    item_database_shutdown();
    return false;
  }

  g_item_db.item_count = 0;
  g_item_db.next_instance_id = 1;
  g_item_db.initialized = true;

  LOG_INFO("item_database_init: complete");
  return true;
}

void item_database_shutdown(void) {
  if (!g_item_db.initialized)
    return;

  if (g_item_db.items) {
    for (u32 i = 0; i < g_item_db.item_count; i++) {
        FREE((void*)g_item_db.items[i].name);
        FREE((void*)g_item_db.items[i].description);
        FREE((void*)g_item_db.items[i].icon_path);
    }
    FREE(g_item_db.items);
  }
  if (g_item_db.id_map) {
    hashmap_destroy(g_item_db.id_map);
  }
  if (g_item_db.name_map) {
    hashmap_destroy(g_item_db.name_map);
  }

  memset(&g_item_db, 0, sizeof(g_item_db));
}

bool item_database_register(const Item *item) {
  if (!g_item_db.initialized || !item)
    return false;
  
  // LOG_DEBUG("Registering item: %d (%s)", item->id, item->name);
  
  if (g_item_db.item_count >= g_item_db.capacity)
    return false;
  if (!item_is_valid(item))
    return false;

  // Check if ID already exists
  if (item_database_exists(item->id))
    return false;

  // Add to array
  u32 index = g_item_db.item_count++;
  // Copy item to database
  g_item_db.items[index] = *item;

  // Database owns copies of strings
  if (item->name) g_item_db.items[index].name = strdup(item->name);
  if (item->description) g_item_db.items[index].description = strdup(item->description);
  if (item->icon_path) g_item_db.items[index].icon_path = strdup(item->icon_path);

  // Add to maps
  hashmap_set(g_item_db.id_map, &item->id, &index);
  hashmap_set_string(g_item_db.name_map, g_item_db.items[index].name, &index);

  return true;
}

bool item_database_load_from_json(const char *filepath) {
  if (!g_item_db.initialized || !filepath)
    return false;

  // Read JSON file
  FILE *file = fopen(filepath, "r");
  if (!file) {
    LOG_ERROR("Failed to open item database file: %s", filepath);
    return false;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Read entire file
  char *json_data = (char *)malloc(file_size + 1);
  if (!json_data) {
    fclose(file);
    return false;
  }

  size_t read_size = fread(json_data, 1, file_size, file);
  json_data[read_size] = '\0';
  fclose(file);

  // Parse JSON
  JsonValue *root = json_parse(json_data);
  free(json_data);

  if (!root || root->type != JSON_ARRAY) {
    LOG_ERROR("Invalid item database JSON format");
    if (root) json_free(root);
    return false;
  }

  // Iterate through items array
  uint32_t loaded_count = 0;
  for (uint32_t i = 0; i < root->array.count; i++) {
    JsonValue *item_obj = root->array.values[i];
    if (item_obj->type != JSON_OBJECT)
      continue;

    Item item = {0};

    // Parse required fields
    JsonValue *id = json_object_get(item_obj, "id");
    JsonValue *name = json_object_get(item_obj, "name");
    JsonValue *type = json_object_get(item_obj, "type");

    if (!id || !name || !type)
      continue;

    // Fix: Access anonymous union members directly and use strdup for const char*
    item.id = (u32)id->number;
    item.name = strdup(name->string);
    
    JsonValue *description = json_object_get(item_obj, "description");
    if (description) item.description = strdup(description->string);
    
    JsonValue *icon = json_object_get(item_obj, "icon");
    if (icon) item.icon_path = strdup(icon->string);

    // Parse type string to enum
    const char *type_str = type->string;
    if (strcmp(type_str, "weapon") == 0) item.type = ITEM_TYPE_WEAPON;
    else if (strcmp(type_str, "armor") == 0) item.type = ITEM_TYPE_ARMOR;
    else if (strcmp(type_str, "consumable") == 0) item.type = ITEM_TYPE_CONSUMABLE;
    else if (strcmp(type_str, "material") == 0) item.type = ITEM_TYPE_MATERIAL;
    else if (strcmp(type_str, "quest") == 0) item.type = ITEM_TYPE_QUEST;
    else continue;

    // Parse optional fields
    JsonValue *rarity = json_object_get(item_obj, "rarity");
    if (rarity) item.rarity = (ItemRarity)rarity->number;

    JsonValue *stack_size = json_object_get(item_obj, "max_stack_size");
    if (stack_size) item.max_stack_size = (u32)stack_size->number;
    else item.max_stack_size = 1;

    JsonValue *weight = json_object_get(item_obj, "weight");
    if (weight) item.weight = (float)weight->number;

    JsonValue *value = json_object_get(item_obj, "value");
    if (value) item.value = (u32)value->number;

    // Parse type-specific data
    if (item.type == ITEM_TYPE_WEAPON) {
      JsonValue *damage = json_object_get(item_obj, "damage");
      if (damage) item.data.weapon.damage = (float)damage->number;
      
      JsonValue *attack_speed = json_object_get(item_obj, "attack_speed");
      if (attack_speed) item.data.weapon.attack_speed = (float)attack_speed->number;
      else item.data.weapon.attack_speed = 1.0f;
      
      JsonValue *durability = json_object_get(item_obj, "durability");
      if (durability) item.data.weapon.durability = (u32)durability->number;
    }

    // Register the item
    if (item_database_register(&item)) {
      loaded_count++;
    }
  }

  json_free(root);
  LOG_INFO("Loaded %u items from JSON database: %s", loaded_count, filepath);
  return loaded_count > 0;
}

// ============================================================================
// ITEM LOOKUP
// ============================================================================

const Item *item_database_get(u32 item_id) {
  if (!g_item_db.initialized)
    return NULL;

  void *index_ptr = hashmap_get(g_item_db.id_map, &item_id);
  if (!index_ptr)
    return NULL;

  u32 index = *(u32 *)index_ptr;
  if (index >= g_item_db.item_count)
    return NULL;

  return &g_item_db.items[index];
}

const Item *item_database_get_by_name(const char *name) {
  if (!g_item_db.initialized || !name)
    return NULL;

  void *index_ptr = hashmap_get_string(g_item_db.name_map, name);
  if (!index_ptr)
    return NULL;

  u32 index = *(u32 *)index_ptr;
  if (index >= g_item_db.item_count)
    return NULL;

  return &g_item_db.items[index];
}

u32 item_database_get_items_by_type(ItemType type, const Item **out_items,
                                    u32 max_items) {
  if (!g_item_db.initialized || !out_items)
    return 0;

  u32 count = 0;
  for (u32 i = 0; i < g_item_db.item_count && count < max_items; i++) {
    if (g_item_db.items[i].type == type) {
      out_items[count++] = &g_item_db.items[i];
    }
  }

  return count;
}

u32 item_database_get_items_by_rarity(ItemRarity rarity, const Item **out_items,
                                      u32 max_items) {
  if (!g_item_db.initialized || !out_items)
    return 0;

  u32 count = 0;
  for (u32 i = 0; i < g_item_db.item_count && count < max_items; i++) {
    if (g_item_db.items[i].rarity == rarity) {
      out_items[count++] = &g_item_db.items[i];
    }
  }

  return count;
}

u32 item_database_get_count(void) {
  return g_item_db.initialized ? g_item_db.item_count : 0;
}

bool item_database_exists(u32 item_id) {
  return item_database_get(item_id) != NULL;
}

// ============================================================================
// ITEM CREATION
// ============================================================================

ItemStack item_create_stack(u32 item_id, u32 quantity) {
  ItemStack stack = {0};

  const Item *item = item_database_get(item_id);
  if (!item)
    return stack;

  stack.item = item;
  stack.quantity =
      (quantity <= item->max_stack_size) ? quantity : item->max_stack_size;
  stack.durability =
      (item->type == ITEM_TYPE_WEAPON)  ? item->data.weapon.durability
      : (item->type == ITEM_TYPE_ARMOR) ? item->data.armor.durability
                                        : 0;
  stack.instance_id = g_item_db.next_instance_id++;
  stack.is_equipped = false;

  return stack;
}

ItemStack item_create_stack_by_name(const char *name, u32 quantity) {
  const Item *item = item_database_get_by_name(name);
  if (!item) {
    ItemStack empty = {0};
    return empty;
  }

  return item_create_stack(item->id, quantity);
}

ItemStack item_stack_clone(const ItemStack *stack) {
  if (!stack || !stack->item) {
    ItemStack empty = {0};
    return empty;
  }

  ItemStack clone = *stack;
  clone.instance_id = g_item_db.next_instance_id++;
  return clone;
}

// ============================================================================
// DEFAULT ITEMS
// ============================================================================

void item_database_register_defaults(void) {
  if (!g_item_db.initialized)
    return;

  // Basic weapons
  Item iron_sword = {
      .id = 1000,
      .name = "Iron Sword",
      .description = "A sturdy iron blade.",
      .icon_path = "items/weapons/iron_sword.png",
      .type = ITEM_TYPE_WEAPON,
      .rarity = ITEM_RARITY_COMMON,
      .flags = ITEM_FLAG_EQUIPPABLE | ITEM_FLAG_TRADEABLE | ITEM_FLAG_DROPPABLE,
      .max_stack_size = 1,
      .weight = 2.5f,
      .value = 50,
      .data.weapon = {
          .damage = 10.0f, .attack_speed = 1.0f, .durability = 250}};
  item_database_register(&iron_sword);

  // Basic consumables
  Item health_potion = {.id = 2000,
                        .name = "Health Potion",
                        .description = "Restores 50 health.",
                        .icon_path = "items/consumables/health_potion.png",
                        .type = ITEM_TYPE_CONSUMABLE,
                        .rarity = ITEM_RARITY_COMMON,
                        .flags = ITEM_FLAG_STACKABLE | ITEM_FLAG_CONSUMABLE |
                                 ITEM_FLAG_TRADEABLE | ITEM_FLAG_DROPPABLE,
                        .max_stack_size = 10,
                        .weight = 0.1f,
                        .value = 25,
                        .data.consumable = {.healing = 50.0f,
                                            .mana_restore = 0.0f,
                                            .duration = 0.0f}};
  item_database_register(&health_potion);

  // Materials
  Item iron_ore = {.id = 3000,
                   .name = "Iron Ore",
                   .description = "Raw iron ore.",
                   .icon_path = "items/materials/iron_ore.png",
                   .type = ITEM_TYPE_MATERIAL,
                   .rarity = ITEM_RARITY_COMMON,
                   .flags = ITEM_FLAG_STACKABLE | ITEM_FLAG_TRADEABLE |
                            ITEM_FLAG_DROPPABLE,
                   .max_stack_size = 64,
                   .weight = 0.5f,
                   .value = 5};
  item_database_register(&iron_ore);

  Item wood = {.id = 3001,
               .name = "Wood",
               .description = "Building material.",
               .icon_path = "items/materials/wood.png",
               .type = ITEM_TYPE_MATERIAL,
               .rarity = ITEM_RARITY_COMMON,
               .flags = ITEM_FLAG_STACKABLE | ITEM_FLAG_TRADEABLE |
                        ITEM_FLAG_DROPPABLE,
               .max_stack_size = 64,
               .weight = 0.2f,
               .value = 1};
  item_database_register(&wood);

  Item planks = {.id = 3002,
                 .name = "Planks",
                 .description = "Processed wood.",
                 .icon_path = "items/materials/planks.png",
                 .type = ITEM_TYPE_MATERIAL,
                 .rarity = ITEM_RARITY_COMMON,
                 .flags = ITEM_FLAG_STACKABLE | ITEM_FLAG_TRADEABLE |
                          ITEM_FLAG_DROPPABLE,
                 .max_stack_size = 64,
                 .weight = 0.15f,
                 .value = 2};
  item_database_register(&planks);

  Item sticks = {.id = 3003,
                 .name = "Sticks",
                 .description = "Small wooden sticks.",
                 .icon_path = "items/materials/sticks.png",
                 .type = ITEM_TYPE_MATERIAL,
                 .rarity = ITEM_RARITY_COMMON,
                 .flags = ITEM_FLAG_STACKABLE | ITEM_FLAG_TRADEABLE |
                          ITEM_FLAG_DROPPABLE,
                 .max_stack_size = 64,
                 .weight = 0.05f,
                 .value = 1};
  item_database_register(&sticks);
}
