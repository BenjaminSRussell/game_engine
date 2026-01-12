// Content Creation Tools - STUBBED
#include "editor/content_creation_tools.h"
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>

// Global content creation context
static ContentCreationContext g_context = {0};
static bool g_initialized = false;

// MARK: - Content Creation System Management

bool content_creation_init(void) {
  if (g_initialized)
    return true;
  memset(&g_context, 0, sizeof(ContentCreationContext));
  g_initialized = true;
  return true;
}

void content_creation_shutdown(void) { g_initialized = false; }

void content_creation_update(f32 delta_time) {}
void content_creation_render(void) {}

ContentCreationContext *content_creation_get_context(void) {
  return &g_context;
}

// MARK: - Block Creation Tools

void content_creation_start_block_creation(void) {}
void content_creation_start_block_editing(BlockDefinition *block) {}
void content_creation_finish_block_creation(void) {}
void content_creation_cancel_block_creation(void) {}

void content_creation_set_block_name(const char *name) {}
void content_creation_set_block_texture(const char *texture_path) {}
void content_creation_set_block_model(const char *model_path) {}
void content_creation_set_block_color(Vec3 color) {}
void content_creation_set_block_solid(bool solid) {}
void content_creation_set_block_transparent(bool transparent) {}
void content_creation_set_block_hardness(f32 hardness) {}
void content_creation_set_block_breakable(bool breakable) {}

BlockDefinition *content_creation_create_block_definition(void) {
  return malloc(sizeof(BlockDefinition));
}
void content_creation_save_block_definition(BlockDefinition *block,
                                            const char *file_path) {}
BlockDefinition *content_creation_load_block_definition(const char *file_path) {
  return NULL;
}

// MARK: - Item Creation Tools

void content_creation_start_item_creation(void) {}
void content_creation_start_item_editing(ItemDefinition *item) {}
void content_creation_finish_item_creation(void) {}
void content_creation_cancel_item_creation(void) {}

void content_creation_set_item_name(const char *name) {}
void content_creation_set_item_texture(const char *texture_path) {}
void content_creation_set_item_model(const char *model_path) {}
void content_creation_set_item_scale(f32 scale) {}
void content_creation_set_item_max_stack(u32 max_stack) {}
void content_creation_set_item_consumable(bool consumable) {}
void content_creation_set_item_equippable(bool equippable) {}
void content_creation_set_item_use_action(const char *action) {}

ItemDefinition *content_creation_create_item_definition(void) {
  return malloc(sizeof(ItemDefinition));
}
void content_creation_save_item_definition(ItemDefinition *item,
                                           const char *file_path) {}
ItemDefinition *content_creation_load_item_definition(const char *file_path) {
  return NULL;
}

// MARK: - Mob Creation Tools

void content_creation_start_mob_creation(void) {}
void content_creation_start_mob_editing(MobDefinition *mob) {}
void content_creation_finish_mob_creation(void) {}
void content_creation_cancel_mob_creation(void) {}

void content_creation_set_mob_name(const char *name) {}
void content_creation_set_mob_model(const char *model_path) {}
void content_creation_set_mob_texture(const char *texture_path) {}
void content_creation_set_mob_scale(Vec3 scale) {}
void content_creation_set_mob_health(f32 health) {}
void content_creation_set_mob_speed(f32 speed) {}
void content_creation_set_mob_hostile(bool hostile) {}
void content_creation_set_mob_ai_tree(const char *ai_tree_path) {}
void content_creation_set_mob_drop_item(const char *item_name, u32 count,
                                        u32 chance) {}

MobDefinition *content_creation_create_mob_definition(void) {
  return malloc(sizeof(MobDefinition));
}
void content_creation_save_mob_definition(MobDefinition *mob,
                                          const char *file_path) {}
MobDefinition *content_creation_load_mob_definition(const char *file_path) {
  return NULL;
}

// MARK: - Preview and Placement

void content_creation_update_preview(Vec3 world_position) {}
void content_creation_place_content(void) {}
void content_creation_toggle_preview(void) {}
void content_creation_toggle_grid_snapping(void) {}
void content_creation_set_grid_size(f32 size) {}

// MARK: - Asset Management

void content_creation_refresh_asset_browser(void) {}
char **content_creation_get_available_textures(u32 *count) {
  *count = 0;
  return NULL;
}
char **content_creation_get_available_models(u32 *count) {
  *count = 0;
  return NULL;
}
char **content_creation_get_available_ai_trees(u32 *count) {
  *count = 0;
  return NULL;
}

// MARK: - Template System

BlockDefinition *
content_creation_create_block_from_template(const char *template_name) {
  return NULL;
}
ItemDefinition *
content_creation_create_item_from_template(const char *template_name) {
  return NULL;
}
MobDefinition *
content_creation_create_mob_from_template(const char *template_name) {
  return NULL;
}

void content_creation_save_block_as_template(BlockDefinition *block,
                                             const char *template_name) {}
void content_creation_save_item_as_template(ItemDefinition *item,
                                            const char *template_name) {}
void content_creation_save_mob_as_template(MobDefinition *mob,
                                           const char *template_name) {}

// MARK: - Validation and Export

bool content_creation_validate_block_definition(BlockDefinition *block) {
  return true;
}
bool content_creation_validate_item_definition(ItemDefinition *item) {
  return true;
}
bool content_creation_validate_mob_definition(MobDefinition *mob) {
  return true;
}

void content_creation_export_block_to_game(BlockDefinition *block) {}
void content_creation_export_item_to_game(ItemDefinition *item) {}
void content_creation_export_mob_to_game(MobDefinition *mob) {}

// MARK: - Utility Functions

const char *content_creation_get_current_mode_string(void) { return "None"; }
bool content_creation_is_in_creation_mode(void) { return false; }
void content_creation_reset_context(void) {}
