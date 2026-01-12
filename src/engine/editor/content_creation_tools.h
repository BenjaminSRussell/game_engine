#ifndef CONTENT_CREATION_TOOLS_H
#define CONTENT_CREATION_TOOLS_H

#include <math/vec3.h>
#include <ecs/ecs.h>

#include <core/types.h>
#include <ecs/ecs.h>
#include <math/vec3.h>
#include <stdbool.h>
#include <stddef.h>

// Content Types
typedef enum {
  CONTENT_TYPE_BLOCK,
  CONTENT_TYPE_ITEM,
  CONTENT_TYPE_MOB
} ContentType;

// Block Properties
typedef struct {
  char name[64];
  char description[256];

  // Visual properties
  char texture_path[256];
  char model_path[256];
  Vec3 color;
  bool transparent;
  bool emissive;
  f32 emission_strength;

  // Physical properties
  bool solid;
  bool collidable;
  f32 hardness;
  f32 resistance;
  bool gravity_affected;

  // Interaction properties
  bool breakable;
  bool placeable;
  bool stackable;
  u32 max_stack_size;

  // Custom behavior
  char on_break_script[256];
  char on_place_script[256];
  char on_interact_script[256];
} BlockDefinition;

// Item Properties
typedef struct {
  char name[64];
  char description[256];

  // Visual properties
  char texture_path[256];
  char model_path[256];
  Vec3 color;
  f32 scale;

  // Item properties
  u32 max_stack_size;
  bool consumable;
  bool equippable;
  bool placeable;

  // Usage properties
  char use_action[64];
  f32 cooldown;
  f32 durability;
  f32 max_durability;

  // Custom behavior
  char on_use_script[256];
  char on_equip_script[256];
  char on_unequip_script[256];
} ItemDefinition;

// Mob Properties
typedef struct {
  char name[64];
  char description[256];

  // Visual properties
  char model_path[256];
  char texture_path[256];
  Vec3 scale;
  Vec3 color;

  // Physical properties
  f32 health;
  f32 max_health;
  f32 speed;
  f32 jump_height;
  f32 size;

  // AI properties
  char ai_behavior_tree[256];
  bool hostile;
  bool passive;
  f32 detection_range;
  f32 attack_range;
  f32 attack_damage;
  f32 attack_cooldown;

  // Behavior properties
  bool can_fly;
  bool can_swim;
  bool can_climb;
  bool nocturnal;
  bool diurnal;

  // Drops
  char drop_items[16][64];
  u32 drop_counts[16];
  u32 drop_chances[16];
  u32 drop_count;

  // Custom behavior
  char on_spawn_script[256];
  char on_death_script[256];
  char on_attack_script[256];
} MobDefinition;

// Content Creation Context
typedef struct {
  ContentType current_type;
  bool is_editing;
  union {
    BlockDefinition *current_block;
    ItemDefinition *current_item;
    MobDefinition *current_mob;
  };

  // Preview state
  Entity preview_entity;
  bool preview_visible;
  Vec3 preview_position;

  // Tool state
  bool grid_snapping_enabled;
  f32 grid_size;
  bool collision_preview_enabled;
} ContentCreationContext;

// MARK: - Content Creation System Management

bool content_creation_init(void);
void content_creation_shutdown(void);
void content_creation_update(f32 delta_time);
void content_creation_render(void);

ContentCreationContext *content_creation_get_context(void);

// MARK: - Block Creation Tools

void content_creation_start_block_creation(void);
void content_creation_start_block_editing(BlockDefinition *block);
void content_creation_finish_block_creation(void);
void content_creation_cancel_block_creation(void);

void content_creation_set_block_name(const char *name);
void content_creation_set_block_texture(const char *texture_path);
void content_creation_set_block_model(const char *model_path);
void content_creation_set_block_color(Vec3 color);
void content_creation_set_block_solid(bool solid);
void content_creation_set_block_transparent(bool transparent);
void content_creation_set_block_hardness(f32 hardness);
void content_creation_set_block_breakable(bool breakable);

BlockDefinition *content_creation_create_block_definition(void);
void content_creation_save_block_definition(BlockDefinition *block,
                                            const char *file_path);
BlockDefinition *content_creation_load_block_definition(const char *file_path);

// MARK: - Item Creation Tools

void content_creation_start_item_creation(void);
void content_creation_start_item_editing(ItemDefinition *item);
void content_creation_finish_item_creation(void);
void content_creation_cancel_item_creation(void);

void content_creation_set_item_name(const char *name);
void content_creation_set_item_texture(const char *texture_path);
void content_creation_set_item_model(const char *model_path);
void content_creation_set_item_scale(f32 scale);
void content_creation_set_item_max_stack(u32 max_stack);
void content_creation_set_item_consumable(bool consumable);
void content_creation_set_item_equippable(bool equippable);
void content_creation_set_item_use_action(const char *action);

ItemDefinition *content_creation_create_item_definition(void);
void content_creation_save_item_definition(ItemDefinition *item,
                                           const char *file_path);
ItemDefinition *content_creation_load_item_definition(const char *file_path);

// MARK: - Mob Creation Tools

void content_creation_start_mob_creation(void);
void content_creation_start_mob_editing(MobDefinition *mob);
void content_creation_finish_mob_creation(void);
void content_creation_cancel_mob_creation(void);

void content_creation_set_mob_name(const char *name);
void content_creation_set_mob_model(const char *model_path);
void content_creation_set_mob_texture(const char *texture_path);
void content_creation_set_mob_scale(Vec3 scale);
void content_creation_set_mob_health(f32 health);
void content_creation_set_mob_speed(f32 speed);
void content_creation_set_mob_hostile(bool hostile);
void content_creation_set_mob_ai_tree(const char *ai_tree_path);
void content_creation_set_mob_drop_item(const char *item_name, u32 count,
                                        u32 chance);

MobDefinition *content_creation_create_mob_definition(void);
void content_creation_save_mob_definition(MobDefinition *mob,
                                          const char *file_path);
MobDefinition *content_creation_load_mob_definition(const char *file_path);

// MARK: - Preview and Placement

void content_creation_update_preview(Vec3 world_position);
void content_creation_place_content(void);
void content_creation_toggle_preview(void);
void content_creation_toggle_grid_snapping(void);
void content_creation_set_grid_size(f32 size);

// MARK: - Asset Management

void content_creation_refresh_asset_browser(void);
char **content_creation_get_available_textures(u32 *count);
char **content_creation_get_available_models(u32 *count);
char **content_creation_get_available_ai_trees(u32 *count);

// MARK: - Template System

BlockDefinition *
content_creation_create_block_from_template(const char *template_name);
ItemDefinition *
content_creation_create_item_from_template(const char *template_name);
MobDefinition *
content_creation_create_mob_from_template(const char *template_name);

// Fixed: Renamed functions to avoid C overloading
void content_creation_save_block_as_template(BlockDefinition *block,
                                             const char *template_name);
void content_creation_save_item_as_template(ItemDefinition *item,
                                            const char *template_name);
void content_creation_save_mob_as_template(MobDefinition *mob,
                                           const char *template_name);

// MARK: - Validation and Export

bool content_creation_validate_block_definition(BlockDefinition *block);
bool content_creation_validate_item_definition(ItemDefinition *item);
bool content_creation_validate_mob_definition(MobDefinition *mob);

void content_creation_export_block_to_game(BlockDefinition *block);
void content_creation_export_item_to_game(ItemDefinition *item);
void content_creation_export_mob_to_game(MobDefinition *mob);

// MARK: - Utility Functions

const char *content_creation_get_current_mode_string(void);
bool content_creation_is_in_creation_mode(void);
void content_creation_reset_context(void);

#endif // CONTENT_CREATION_TOOLS_H
