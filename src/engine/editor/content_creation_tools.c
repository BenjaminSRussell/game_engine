// Content Creation Tools implementation
#include "editor/content_creation_tools.h"
#include <core/logger.h>
#include <ecs/ecs.h>
#include <ecs/components/transform.h>
#include <renderer/renderer.h>
#include <math/vec3.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Global content creation context
static ContentCreationContext g_context = {0};
static bool g_initialized = false;

// MARK: - Content Creation System Management

bool content_creation_init(void) {
    if (g_initialized) {
        return true;
    }
    
    memset(&g_context, 0, sizeof(ContentCreationContext));
    g_context.grid_size = 1.0f;
    g_context.grid_snapping_enabled = true;
    g_context.collision_preview_enabled = true;
    
    g_initialized = true;
    LOG_INFO("Content creation tools initialized");
    return true;
}

void content_creation_shutdown(void) {
    if (!g_initialized) {
        return;
    }
    
    content_creation_reset_context();
    g_initialized = false;
    LOG_INFO("Content creation tools shutdown");
}

void content_creation_update(f32 delta_time) {
    if (!g_initialized || !g_context.is_editing) {
        return;
    }
    
    // Update preview entity position and appearance
    if (g_context.preview_visible && g_context.preview_entity.id != 0) {
        TransformComponent* transform = (TransformComponent*)ecs_get_component(
            NULL, g_context.preview_entity, TRANSFORM_COMPONENT_ID);
        
        if (transform) {
            transform->position = g_context.preview_position;
            
            // Apply grid snapping if enabled
            if (g_context.grid_snapping_enabled) {
                transform->position.x = floorf(transform->position.x / g_context.grid_size) * g_context.grid_size + g_context.grid_size * 0.5f;
                transform->position.y = floorf(transform->position.y / g_context.grid_size) * g_context.grid_size + g_context.grid_size * 0.5f;
                transform->position.z = floorf(transform->position.z / g_context.grid_size) * g_context.grid_size + g_context.grid_size * 0.5f;
            }
        }
    }
}

void content_creation_render(void) {
    if (!g_initialized || !g_context.is_editing) {
        return;
    }
    
    // Render preview entity
    if (g_context.preview_visible && g_context.preview_entity.id != 0) {
        // Preview entity will be rendered by the main renderer
    }
    
    // Render grid if enabled
    if (g_context.grid_snapping_enabled) {
        // TODO: Render grid visualization
    }
    
    // Render collision preview if enabled
    if (g_context.collision_preview_enabled) {
        // TODO: Render collision bounds preview
    }
}

ContentCreationContext* content_creation_get_context(void) {
    return &g_context;
}

// MARK: - Block Creation Tools

void content_creation_start_block_creation(void) {
    content_creation_reset_context();
    
    g_context.current_type = CONTENT_TYPE_BLOCK;
    g_context.is_editing = true;
    g_context.current_block = content_creation_create_block_definition();
    
    // Create preview entity
    // TODO: Create preview entity with default block mesh
    
    g_context.preview_visible = true;
    LOG_INFO("Started block creation");
}

void content_creation_start_block_editing(BlockDefinition* block) {
    if (!block) {
        LOG_ERROR("Cannot edit NULL block");
        return;
    }
    
    content_creation_reset_context();
    
    g_context.current_type = CONTENT_TYPE_BLOCK;
    g_context.is_editing = true;
    g_context.current_block = block;
    
    // Create preview entity with block properties
    // TODO: Create preview entity with block's mesh and texture
    
    g_context.preview_visible = true;
    LOG_INFO("Started editing block: %s", block->name);
}

void content_creation_finish_block_creation(void) {
    if (!g_context.is_editing || g_context.current_type != CONTENT_TYPE_BLOCK || !g_context.current_block) {
        LOG_ERROR("No block creation in progress");
        return;
    }
    
    if (content_creation_validate_block_definition(g_context.current_block)) {
        // Export block to game
        content_creation_export_block_to_game(g_context.current_block);
        
        // Save to file
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "assets/user_blocks/%s.block", g_context.current_block->name);
        content_creation_save_block_definition(g_context.current_block, file_path);
        
        LOG_INFO("Finished block creation: %s", g_context.current_block->name);
        content_creation_reset_context();
    } else {
        LOG_ERROR("Block definition validation failed");
    }
}

void content_creation_cancel_block_creation(void) {
    if (g_context.current_type == CONTENT_TYPE_BLOCK && g_context.current_block) {
        free(g_context.current_block);
    }
    content_creation_reset_context();
    LOG_INFO("Cancelled block creation");
}

void content_creation_set_block_name(const char* name) {
    if (g_context.current_block && name) {
        strncpy(g_context.current_block->name, name, sizeof(g_context.current_block->name) - 1);
    }
}

void content_creation_set_block_texture(const char* texture_path) {
    if (g_context.current_block && texture_path) {
        strncpy(g_context.current_block->texture_path, texture_path, sizeof(g_context.current_block->texture_path) - 1);
        
        // Update preview texture
        // TODO: Update preview entity texture
    }
}

void content_creation_set_block_model(const char* model_path) {
    if (g_context.current_block && model_path) {
        strncpy(g_context.current_block->model_path, model_path, sizeof(g_context.current_block->model_path) - 1);
        
        // Update preview model
        // TODO: Update preview entity model
    }
}

void content_creation_set_block_color(Vec3 color) {
    if (g_context.current_block) {
        g_context.current_block->color = color;
        
        // Update preview color
        // TODO: Update preview entity color
    }
}

void content_creation_set_block_solid(bool solid) {
    if (g_context.current_block) {
        g_context.current_block->solid = solid;
        
        // Update collision preview
        if (g_context.collision_preview_enabled) {
            // TODO: Update collision preview
        }
    }
}

void content_creation_set_block_transparent(bool transparent) {
    if (g_context.current_block) {
        g_context.current_block->transparent = transparent;
        
        // Update preview transparency
        // TODO: Update preview entity transparency
    }
}

void content_creation_set_block_hardness(f32 hardness) {
    if (g_context.current_block) {
        g_context.current_block->hardness = hardness;
    }
}

void content_creation_set_block_breakable(bool breakable) {
    if (g_context.current_block) {
        g_context.current_block->breakable = breakable;
    }
}

BlockDefinition* content_creation_create_block_definition(void) {
    BlockDefinition* block = malloc(sizeof(BlockDefinition));
    if (!block) {
        LOG_ERROR("Failed to allocate block definition");
        return NULL;
    }
    
    memset(block, 0, sizeof(BlockDefinition));
    
    // Set default values
    strcpy(block->name, "New Block");
    strcpy(block->description, "A custom block created with the content creation tools");
    strcpy(block->texture_path, "assets/textures/default.png");
    strcpy(block->model_path, "assets/models/block.obj");
    
    block->color = (Vec3){1.0f, 1.0f, 1.0f};
    block->solid = true;
    block->collidable = true;
    block->hardness = 1.0f;
    block->resistance = 1.0f;
    block->breakable = true;
    block->placeable = true;
    block->stackable = true;
    block->max_stack_size = 64;
    
    return block;
}

void content_creation_save_block_definition(BlockDefinition* block, const char* file_path) {
    if (!block || !file_path) {
        return;
    }
    
    FILE* file = fopen(file_path, "w");
    if (!file) {
        LOG_ERROR("Failed to open file for writing: %s", file_path);
        return;
    }
    
    // Simple JSON-like format for now
    fprintf(file, "{\n");
    fprintf(file, "  \"name\": \"%s\",\n", block->name);
    fprintf(file, "  \"description\": \"%s\",\n", block->description);
    fprintf(file, "  \"texture_path\": \"%s\",\n", block->texture_path);
    fprintf(file, "  \"model_path\": \"%s\",\n", block->model_path);
    fprintf(file, "  \"color\": [%.3f, %.3f, %.3f],\n", block->color.x, block->color.y, block->color.z);
    fprintf(file, "  \"transparent\": %s,\n", block->transparent ? "true" : "false");
    fprintf(file, "  \"solid\": %s,\n", block->solid ? "true" : "false");
    fprintf(file, "  \"hardness\": %.3f,\n", block->hardness);
    fprintf(file, "  \"breakable\": %s\n", block->breakable ? "true" : "false");
    fprintf(file, "}\n");
    
    fclose(file);
    LOG_INFO("Saved block definition to: %s", file_path);
}

BlockDefinition* content_creation_load_block_definition(const char* file_path) {
    // TODO: Implement proper JSON parsing
    // For now, return a new block with default values
    LOG_INFO("Loading block definition from: %s", file_path);
    return content_creation_create_block_definition();
}

// MARK: - Item Creation Tools

void content_creation_start_item_creation(void) {
    content_creation_reset_context();
    
    g_context.current_type = CONTENT_TYPE_ITEM;
    g_context.is_editing = true;
    g_context.current_item = content_creation_create_item_definition();
    
    // Create preview entity
    // TODO: Create preview entity with default item mesh
    
    g_context.preview_visible = true;
    LOG_INFO("Started item creation");
}

void content_creation_start_item_editing(ItemDefinition* item) {
    if (!item) {
        LOG_ERROR("Cannot edit NULL item");
        return;
    }
    
    content_creation_reset_context();
    
    g_context.current_type = CONTENT_TYPE_ITEM;
    g_context.is_editing = true;
    g_context.current_item = item;
    
    // Create preview entity with item properties
    // TODO: Create preview entity with item's mesh and texture
    
    g_context.preview_visible = true;
    LOG_INFO("Started editing item: %s", item->name);
}

void content_creation_finish_item_creation(void) {
    if (!g_context.is_editing || g_context.current_type != CONTENT_TYPE_ITEM || !g_context.current_item) {
        LOG_ERROR("No item creation in progress");
        return;
    }
    
    if (content_creation_validate_item_definition(g_context.current_item)) {
        // Export item to game
        content_creation_export_item_to_game(g_context.current_item);
        
        // Save to file
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "assets/user_items/%s.item", g_context.current_item->name);
        content_creation_save_item_definition(g_context.current_item, file_path);
        
        LOG_INFO("Finished item creation: %s", g_context.current_item->name);
        content_creation_reset_context();
    } else {
        LOG_ERROR("Item definition validation failed");
    }
}

void content_creation_cancel_item_creation(void) {
    if (g_context.current_type == CONTENT_TYPE_ITEM && g_context.current_item) {
        free(g_context.current_item);
    }
    content_creation_reset_context();
    LOG_INFO("Cancelled item creation");
}

void content_creation_set_item_name(const char* name) {
    if (g_context.current_item && name) {
        strncpy(g_context.current_item->name, name, sizeof(g_context.current_item->name) - 1);
    }
}

void content_creation_set_item_texture(const char* texture_path) {
    if (g_context.current_item && texture_path) {
        strncpy(g_context.current_item->texture_path, texture_path, sizeof(g_context.current_item->texture_path) - 1);
        
        // Update preview texture
        // TODO: Update preview entity texture
    }
}

void content_creation_set_item_model(const char* model_path) {
    if (g_context.current_item && model_path) {
        strncpy(g_context.current_item->model_path, model_path, sizeof(g_context.current_item->model_path) - 1);
        
        // Update preview model
        // TODO: Update preview entity model
    }
}

void content_creation_set_item_scale(f32 scale) {
    if (g_context.current_item) {
        g_context.current_item->scale = scale;
        
        // Update preview scale
        // TODO: Update preview entity scale
    }
}

void content_creation_set_item_max_stack(u32 max_stack) {
    if (g_context.current_item) {
        g_context.current_item->max_stack_size = max_stack;
    }
}

void content_creation_set_item_consumable(bool consumable) {
    if (g_context.current_item) {
        g_context.current_item->consumable = consumable;
    }
}

void content_creation_set_item_equippable(bool equippable) {
    if (g_context.current_item) {
        g_context.current_item->equippable = equippable;
    }
}

void content_creation_set_item_use_action(const char* action) {
    if (g_context.current_item && action) {
        strncpy(g_context.current_item->use_action, action, sizeof(g_context.current_item->use_action) - 1);
    }
}

ItemDefinition* content_creation_create_item_definition(void) {
    ItemDefinition* item = malloc(sizeof(ItemDefinition));
    if (!item) {
        LOG_ERROR("Failed to allocate item definition");
        return NULL;
    }
    
    memset(item, 0, sizeof(ItemDefinition));
    
    // Set default values
    strcpy(item->name, "New Item");
    strcpy(item->description, "A custom item created with the content creation tools");
    strcpy(item->texture_path, "assets/textures/item_default.png");
    strcpy(item->model_path, "assets/models/item.obj");
    
    item->color = (Vec3){1.0f, 1.0f, 1.0f};
    item->scale = 1.0f;
    item->max_stack_size = 64;
    item->consumable = false;
    item->equippable = false;
    item->placeable = false;
    item->cooldown = 0.0f;
    item->durability = 100.0f;
    item->max_durability = 100.0f;
    
    return item;
}

void content_creation_save_item_definition(ItemDefinition* item, const char* file_path) {
    if (!item || !file_path) {
        return;
    }
    
    FILE* file = fopen(file_path, "w");
    if (!file) {
        LOG_ERROR("Failed to open file for writing: %s", file_path);
        return;
    }
    
    // Simple JSON-like format for now
    fprintf(file, "{\n");
    fprintf(file, "  \"name\": \"%s\",\n", item->name);
    fprintf(file, "  \"description\": \"%s\",\n", item->description);
    fprintf(file, "  \"texture_path\": \"%s\",\n", item->texture_path);
    fprintf(file, "  \"model_path\": \"%s\",\n", item->model_path);
    fprintf(file, "  \"scale\": %.3f,\n", item->scale);
    fprintf(file, "  \"max_stack_size\": %u,\n", item->max_stack_size);
    fprintf(file, "  \"consumable\": %s,\n", item->consumable ? "true" : "false");
    fprintf(file, "  \"equippable\": %s\n", item->equippable ? "true" : "false");
    fprintf(file, "}\n");
    
    fclose(file);
    LOG_INFO("Saved item definition to: %s", file_path);
}

ItemDefinition* content_creation_load_item_definition(const char* file_path) {
    // TODO: Implement proper JSON parsing
    // For now, return a new item with default values
    LOG_INFO("Loading item definition from: %s", file_path);
    return content_creation_create_item_definition();
}

// MARK: - Mob Creation Tools

void content_creation_start_mob_creation(void) {
    content_creation_reset_context();
    
    g_context.current_type = CONTENT_TYPE_MOB;
    g_context.is_editing = true;
    g_context.current_mob = content_creation_create_mob_definition();
    
    // Create preview entity
    // TODO: Create preview entity with default mob mesh
    
    g_context.preview_visible = true;
    LOG_INFO("Started mob creation");
}

void content_creation_start_mob_editing(MobDefinition* mob) {
    if (!mob) {
        LOG_ERROR("Cannot edit NULL mob");
        return;
    }
    
    content_creation_reset_context();
    
    g_context.current_type = CONTENT_TYPE_MOB;
    g_context.is_editing = true;
    g_context.current_mob = mob;
    
    // Create preview entity with mob properties
    // TODO: Create preview entity with mob's mesh and texture
    
    g_context.preview_visible = true;
    LOG_INFO("Started editing mob: %s", mob->name);
}

void content_creation_finish_mob_creation(void) {
    if (!g_context.is_editing || g_context.current_type != CONTENT_TYPE_MOB || !g_context.current_mob) {
        LOG_ERROR("No mob creation in progress");
        return;
    }
    
    if (content_creation_validate_mob_definition(g_context.current_mob)) {
        // Export mob to game
        content_creation_export_mob_to_game(g_context.current_mob);
        
        // Save to file
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "assets/user_mobs/%s.mob", g_context.current_mob->name);
        content_creation_save_mob_definition(g_context.current_mob, file_path);
        
        LOG_INFO("Finished mob creation: %s", g_context.current_mob->name);
        content_creation_reset_context();
    } else {
        LOG_ERROR("Mob definition validation failed");
    }
}

void content_creation_cancel_mob_creation(void) {
    if (g_context.current_type == CONTENT_TYPE_MOB && g_context.current_mob) {
        free(g_context.current_mob);
    }
    content_creation_reset_context();
    LOG_INFO("Cancelled mob creation");
}

void content_creation_set_mob_name(const char* name) {
    if (g_context.current_mob && name) {
        strncpy(g_context.current_mob->name, name, sizeof(g_context.current_mob->name) - 1);
    }
}

void content_creation_set_mob_model(const char* model_path) {
    if (g_context.current_mob && model_path) {
        strncpy(g_context.current_mob->model_path, model_path, sizeof(g_context.current_mob->model_path) - 1);
        
        // Update preview model
        // TODO: Update preview entity model
    }
}

void content_creation_set_mob_texture(const char* texture_path) {
    if (g_context.current_mob && texture_path) {
        strncpy(g_context.current_mob->texture_path, texture_path, sizeof(g_context.current_mob->texture_path) - 1);
        
        // Update preview texture
        // TODO: Update preview entity texture
    }
}

void content_creation_set_mob_scale(Vec3 scale) {
    if (g_context.current_mob) {
        g_context.current_mob->scale = scale;
        
        // Update preview scale
        // TODO: Update preview entity scale
    }
}

void content_creation_set_mob_health(f32 health) {
    if (g_context.current_mob) {
        g_context.current_mob->health = health;
        g_context.current_mob->max_health = health;
    }
}

void content_creation_set_mob_speed(f32 speed) {
    if (g_context.current_mob) {
        g_context.current_mob->speed = speed;
    }
}

void content_creation_set_mob_hostile(bool hostile) {
    if (g_context.current_mob) {
        g_context.current_mob->hostile = hostile;
        g_context.current_mob->passive = !hostile;
    }
}

void content_creation_set_mob_ai_tree(const char* ai_tree_path) {
    if (g_context.current_mob && ai_tree_path) {
        strncpy(g_context.current_mob->ai_behavior_tree, ai_tree_path, sizeof(g_context.current_mob->ai_behavior_tree) - 1);
    }
}

void content_creation_set_mob_drop_item(const char* item_name, u32 count, u32 chance) {
    if (!g_context.current_mob || !item_name || g_context.current_mob->drop_count >= 16) {
        return;
    }
    
    u32 index = g_context.current_mob->drop_count;
    strncpy(g_context.current_mob->drop_items[index], item_name, sizeof(g_context.current_mob->drop_items[index]) - 1);
    g_context.current_mob->drop_counts[index] = count;
    g_context.current_mob->drop_chances[index] = chance;
    g_context.current_mob->drop_count++;
}

MobDefinition* content_creation_create_mob_definition(void) {
    MobDefinition* mob = malloc(sizeof(MobDefinition));
    if (!mob) {
        LOG_ERROR("Failed to allocate mob definition");
        return NULL;
    }
    
    memset(mob, 0, sizeof(MobDefinition));
    
    // Set default values
    strcpy(mob->name, "New Mob");
    strcpy(mob->description, "A custom mob created with the content creation tools");
    strcpy(mob->model_path, "assets/models/mob_default.obj");
    strcpy(mob->texture_path, "assets/textures/mob_default.png");
    
    mob->scale = (Vec3){1.0f, 1.0f, 1.0f};
    mob->color = (Vec3){1.0f, 1.0f, 1.0f};
    mob->health = 100.0f;
    mob->max_health = 100.0f;
    mob->speed = 1.0f;
    mob->jump_height = 1.0f;
    mob->size = 1.0f;
    
    mob->hostile = false;
    mob->passive = true;
    mob->detection_range = 10.0f;
    mob->attack_range = 2.0f;
    mob->attack_damage = 10.0f;
    mob->attack_cooldown = 1.0f;
    
    mob->can_fly = false;
    mob->can_swim = false;
    mob->can_climb = false;
    mob->nocturnal = false;
    mob->diurnal = true;
    
    return mob;
}

void content_creation_save_mob_definition(MobDefinition* mob, const char* file_path) {
    if (!mob || !file_path) {
        return;
    }
    
    FILE* file = fopen(file_path, "w");
    if (!file) {
        LOG_ERROR("Failed to open file for writing: %s", file_path);
        return;
    }
    
    // Simple JSON-like format for now
    fprintf(file, "{\n");
    fprintf(file, "  \"name\": \"%s\",\n", mob->name);
    fprintf(file, "  \"description\": \"%s\",\n", mob->description);
    fprintf(file, "  \"model_path\": \"%s\",\n", mob->model_path);
    fprintf(file, "  \"texture_path\": \"%s\",\n", mob->texture_path);
    fprintf(file, "  \"health\": %.1f,\n", mob->health);
    fprintf(file, "  \"speed\": %.3f,\n", mob->speed);
    fprintf(file, "  \"hostile\": %s,\n", mob->hostile ? "true" : "false");
    fprintf(file, "  \"ai_behavior_tree\": \"%s\"\n", mob->ai_behavior_tree);
    fprintf(file, "}\n");
    
    fclose(file);
    LOG_INFO("Saved mob definition to: %s", file_path);
}

MobDefinition* content_creation_load_mob_definition(const char* file_path) {
    // TODO: Implement proper JSON parsing
    // For now, return a new mob with default values
    LOG_INFO("Loading mob definition from: %s", file_path);
    return content_creation_create_mob_definition();
}

// MARK: - Preview and Placement

void content_creation_update_preview(Vec3 world_position) {
    g_context.preview_position = world_position;
}

void content_creation_place_content(void) {
    if (!g_context.is_editing) {
        return;
    }
    
    // Place the content at the current preview position
    Vec3 place_position = g_context.preview_position;
    
    if (g_context.grid_snapping_enabled) {
        place_position.x = floorf(place_position.x / g_context.grid_size) * g_context.grid_size + g_context.grid_size * 0.5f;
        place_position.y = floorf(place_position.y / g_context.grid_size) * g_context.grid_size + g_context.grid_size * 0.5f;
        place_position.z = floorf(place_position.z / g_context.grid_size) * g_context.grid_size + g_context.grid_size * 0.5f;
    }
    
    // TODO: Create actual entity in the world at place_position
    LOG_INFO("Placed content at position: %.2f, %.2f, %.2f", place_position.x, place_position.y, place_position.z);
}

void content_creation_toggle_preview(void) {
    g_context.preview_visible = !g_context.preview_visible;
    LOG_INFO("Preview %s", g_context.preview_visible ? "enabled" : "disabled");
}

void content_creation_toggle_grid_snapping(void) {
    g_context.grid_snapping_enabled = !g_context.grid_snapping_enabled;
    LOG_INFO("Grid snapping %s", g_context.grid_snapping_enabled ? "enabled" : "disabled");
}

void content_creation_set_grid_size(f32 size) {
    g_context.grid_size = size;
    LOG_INFO("Grid size set to: %.2f", size);
}

// MARK: - Asset Management

void content_creation_refresh_asset_browser(void) {
    // TODO: Scan asset directories and update available assets
    LOG_INFO("Refreshed asset browser");
}

char** content_creation_get_available_textures(u32* count) {
    // TODO: Scan textures directory and return list
    *count = 0;
    return NULL;
}

char** content_creation_get_available_models(u32* count) {
    // TODO: Scan models directory and return list
    *count = 0;
    return NULL;
}

char** content_creation_get_available_ai_trees(u32* count) {
    // TODO: Scan AI trees directory and return list
    *count = 0;
    return NULL;
}

// MARK: - Template System

BlockDefinition* content_creation_create_block_from_template(const char* template_name) {
    // TODO: Load block template and create new block from it
    LOG_INFO("Creating block from template: %s", template_name);
    return content_creation_create_block_definition();
}

ItemDefinition* content_creation_create_item_from_template(const char* template_name) {
    // TODO: Load item template and create new item from it
    LOG_INFO("Creating item from template: %s", template_name);
    return content_creation_create_item_definition();
}

MobDefinition* content_creation_create_mob_from_template(const char* template_name) {
    // TODO: Load mob template and create new mob from it
    LOG_INFO("Creating mob from template: %s", template_name);
    return content_creation_create_mob_definition();
}

// MARK: - Validation and Export

bool content_creation_validate_block_definition(BlockDefinition* block) {
    if (!block) {
        return false;
    }
    
    // Check required fields
    if (strlen(block->name) == 0) {
        LOG_ERROR("Block name is required");
        return false;
    }
    
    if (strlen(block->texture_path) == 0) {
        LOG_ERROR("Block texture path is required");
        return false;
    }
    
    // Check file existence
    // TODO: Verify texture and model files exist
    
    return true;
}

bool content_creation_validate_item_definition(ItemDefinition* item) {
    if (!item) {
        return false;
    }
    
    // Check required fields
    if (strlen(item->name) == 0) {
        LOG_ERROR("Item name is required");
        return false;
    }
    
    if (strlen(item->texture_path) == 0) {
        LOG_ERROR("Item texture path is required");
        return false;
    }
    
    return true;
}

bool content_creation_validate_mob_definition(MobDefinition* mob) {
    if (!mob) {
        return false;
    }
    
    // Check required fields
    if (strlen(mob->name) == 0) {
        LOG_ERROR("Mob name is required");
        return false;
    }
    
    if (strlen(mob->model_path) == 0) {
        LOG_ERROR("Mob model path is required");
        return false;
    }
    
    if (mob->health <= 0) {
        LOG_ERROR("Mob health must be positive");
        return false;
    }
    
    return true;
}

void content_creation_export_block_to_game(BlockDefinition* block) {
    // TODO: Register block with game's block registry
    LOG_INFO("Exported block to game: %s", block->name);
}

void content_creation_export_item_to_game(ItemDefinition* item) {
    // TODO: Register item with game's item registry
    LOG_INFO("Exported item to game: %s", item->name);
}

void content_creation_export_mob_to_game(MobDefinition* mob) {
    // TODO: Register mob with game's mob registry
    LOG_INFO("Exported mob to game: %s", mob->name);
}

// MARK: - Utility Functions

const char* content_creation_get_current_mode_string(void) {
    if (!g_context.is_editing) {
        return "None";
    }
    
    switch (g_context.current_type) {
        case CONTENT_TYPE_BLOCK: return "Block Creation";
        case CONTENT_TYPE_ITEM: return "Item Creation";
        case CONTENT_TYPE_MOB: return "Mob Creation";
        default: return "Unknown";
    }
}

bool content_creation_is_in_creation_mode(void) {
    return g_context.is_editing;
}

void content_creation_reset_context(void) {
    // Clean up current editing content
    if (g_context.is_editing) {
        switch (g_context.current_type) {
            case CONTENT_TYPE_BLOCK:
                if (g_context.current_block) {
                    free(g_context.current_block);
                }
                break;
            case CONTENT_TYPE_ITEM:
                if (g_context.current_item) {
                    free(g_context.current_item);
                }
                break;
            case CONTENT_TYPE_MOB:
                if (g_context.current_mob) {
                    free(g_context.current_mob);
                }
                break;
        }
    }
    
    // Clean up preview entity
    if (g_context.preview_entity.id != 0) {
        // TODO: Destroy preview entity
        g_context.preview_entity.id = 0;
    }
    
    memset(&g_context, 0, sizeof(ContentCreationContext));
    g_context.grid_size = 1.0f;
    g_context.grid_snapping_enabled = true;
    g_context.collision_preview_enabled = true;
}
