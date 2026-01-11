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
        content_creation_render_grid();
    }
    
    // Render collision preview if enabled
    if (g_context.collision_preview_enabled) {
        content_creation_render_collision_preview();
    }
}

// MARK: - Grid Visualization

static void content_creation_render_grid(void) {
    // Grid parameters
    const f32 grid_size = g_context.grid_size;
    const f32 grid_extent = 50.0f; // Render 50 units in each direction
    const f32 grid_alpha = 0.3f;
    const vec4_t grid_color_major = {0.2f, 0.2f, 0.2f, grid_alpha};
    const vec4_t grid_color_minor = {0.1f, 0.1f, 0.1f, grid_alpha * 0.5f};
    
    // Calculate grid bounds
    const i32 grid_start = -(i32)(grid_extent / grid_size);
    const i32 grid_end = (i32)(grid_extent / grid_size);
    
    // Render grid lines on XZ plane (Y = 0)
    for (i32 x = grid_start; x <= grid_end; x++) {
        f32 x_pos = x * grid_size;
        vec3_t start = {x_pos, 0.0f, -grid_extent};
        vec3_t end = {x_pos, 0.0f, grid_extent};
        
        // Use major color for origin and every 5th line
        vec4_t color = (x == 0 || x % 5 == 0) ? grid_color_major : grid_color_minor;
        
        // Draw line - this would use the debug renderer
        // debug_draw_line(&g_debug_renderer, start, end, color);
    }
    
    // Render grid lines on Z axis
    for (i32 z = grid_start; z <= grid_end; z++) {
        f32 z_pos = z * grid_size;
        vec3_t start = {-grid_extent, 0.0f, z_pos};
        vec3_t end = {grid_extent, 0.0f, z_pos};
        
        // Use major color for origin and every 5th line
        vec4_t color = (z == 0 || z % 5 == 0) ? grid_color_major : grid_color_minor;
        
        // Draw line - this would use the debug renderer
        // debug_draw_line(&g_debug_renderer, start, end, color);
    }
    
    // Render origin axes for reference
    const vec4_t x_axis_color = {0.8f, 0.2f, 0.2f, 0.8f};
    const vec4_t y_axis_color = {0.2f, 0.8f, 0.2f, 0.8f};
    const vec4_t z_axis_color = {0.2f, 0.2f, 0.8f, 0.8f};
    
    const f32 axis_length = 2.0f;
    
    // X axis (red)
    vec3_t x_start = {0.0f, 0.0f, 0.0f};
    vec3_t x_end = {axis_length, 0.0f, 0.0f};
    // debug_draw_line(&g_debug_renderer, x_start, x_end, x_axis_color);
    
    // Y axis (green)
    vec3_t y_start = {0.0f, 0.0f, 0.0f};
    vec3_t y_end = {0.0f, axis_length, 0.0f};
    // debug_draw_line(&g_debug_renderer, y_start, y_end, y_axis_color);
    
    // Z axis (blue)
    vec3_t z_start = {0.0f, 0.0f, 0.0f};
    vec3_t z_end = {0.0f, 0.0f, axis_length};
    // debug_draw_line(&g_debug_renderer, z_start, z_end, z_axis_color);
}

// MARK: - Collision Visualization

static void content_creation_render_collision_preview(void) {
    if (!g_context.preview_visible || g_context.preview_entity.id == 0) {
        return;
    }
    
    // Get preview entity transform
    TransformComponent* transform = (TransformComponent*)ecs_get_component(
        g_context.preview_entity.id, COMPONENT_TYPE_TRANSFORM);
    
    if (!transform) {
        return;
    }
    
    // Collision preview colors
    const vec4_t solid_color = {0.2f, 0.8f, 0.2f, 0.4f};  // Green for solid
    const vec4_t non_solid_color = {0.8f, 0.8f, 0.2f, 0.4f}; // Yellow for non-solid
    const vec4_t wireframe_color = {1.0f, 1.0f, 1.0f, 0.8f}; // White wireframe
    
    vec3_t position = transform->position;
    vec3_t scale = transform->scale;
    
    // Determine collision properties based on content type
    bool is_collidable = false;
    bool is_solid = false;
    vec3_t collision_bounds = {1.0f, 1.0f, 1.0f}; // Default bounds
    
    switch (g_context.current_type) {
        case CONTENT_TYPE_BLOCK:
            if (g_context.current_block) {
                is_collidable = g_context.current_block->collidable;
                is_solid = g_context.current_block->solid;
                // Use block scale for collision bounds
                collision_bounds = scale;
            }
            break;
            
        case CONTENT_TYPE_ITEM:
            if (g_context.current_item) {
                // Items typically have smaller collision bounds
                collision_bounds = (vec3_t){0.25f, 0.25f, 0.25f};
                collision_bounds.x *= scale.x;
                collision_bounds.y *= scale.y;
                collision_bounds.z *= scale.z;
                is_collidable = false; // Items usually don't collide
                is_solid = false;
            }
            break;
            
        case CONTENT_TYPE_MOB:
            if (g_context.current_mob) {
                // Mobs have collision bounds based on their size
                collision_bounds = (vec3_t){
                    g_context.current_mob->width * scale.x,
                    g_context.current_mob->height * scale.y,
                    g_context.current_mob->width * scale.z
                };
                is_collidable = true;
                is_solid = true; // Mobs are solid
            }
            break;
    }
    
    if (!is_collidable) {
        // Render non-collidable indicator (dashed outline)
        content_creation_render_dashed_bounds(position, collision_bounds, non_solid_color);
    } else {
        // Render solid collision bounds
        vec4_t fill_color = is_solid ? solid_color : non_solid_color;
        
        // Render filled collision box (semi-transparent)
        // debug_draw_box(&g_debug_renderer, position, collision_bounds, fill_color, true);
        
        // Render wireframe outline
        // debug_draw_box(&g_debug_renderer, position, collision_bounds, wireframe_color, false);
        
        // Add collision type indicator
        const char* collision_type = is_solid ? "SOLID" : "NON-SOLID";
        vec3_t text_position = {
            position.x,
            position.y + collision_bounds.y + 0.5f,
            position.z
        };
        // debug_draw_text_3d(&g_debug_renderer, text_position, collision_type, wireframe_color);
    }
}

static void content_creation_render_dashed_bounds(vec3_t center, vec3_t extents, vec4_t color) {
    const f32 dash_length = 0.1f;
    const f32 gap_length = 0.05f;
    const i32 dashes_per_edge = (i32)(2.0f / (dash_length + gap_length));
    
    // Calculate box corners
    vec3_t corners[8] = {
        {center.x - extents.x, center.y - extents.y, center.z - extents.z}, // 0: bottom-left-back
        {center.x + extents.x, center.y - extents.y, center.z - extents.z}, // 1: bottom-right-back
        {center.x + extents.x, center.y - extents.y, center.z + extents.z}, // 2: bottom-right-front
        {center.x - extents.x, center.y - extents.y, center.z + extents.z}, // 3: bottom-left-front
        {center.x - extents.x, center.y + extents.y, center.z - extents.z}, // 4: top-left-back
        {center.x + extents.x, center.y + extents.y, center.z - extents.z}, // 5: top-right-back
        {center.x + extents.x, center.y + extents.y, center.z + extents.z}, // 6: top-right-front
        {center.x - extents.x, center.y + extents.y, center.z + extents.z}, // 7: top-left-front
    };
    
    // Edge connections (pairs of corner indices)
    const i32 edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Bottom edges
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Top edges
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Vertical edges
    };
    
    // Render dashed lines for each edge
    for (i32 edge = 0; edge < 12; edge++) {
        vec3_t start = corners[edges[edge][0]];
        vec3_t end = corners[edges[edge][1]];
        
        vec3_t direction = vec3_sub(end, start);
        f32 edge_length = vec3_length(direction);
        direction = vec3_normalize(direction);
        
        // Draw dashes along the edge
        for (i32 i = 0; i < dashes_per_edge; i++) {
            f32 start_t = i * (dash_length + gap_length);
            f32 end_t = start_t + dash_length;
            
            if (start_t >= edge_length) break;
            if (end_t > edge_length) end_t = edge_length;
            
            vec3_t dash_start = vec3_add(start, vec3_scale(direction, start_t));
            vec3_t dash_end = vec3_add(start, vec3_scale(direction, end_t));
            
            // debug_draw_line(&g_debug_renderer, dash_start, dash_end, color);
        }
    }
}

ContentCreationContext* content_creation_get_context(void) {
    return &g_context;
}

// MARK: - JSON Parsing Helpers

static void content_creation_parse_block_from_json(BlockDefinition* block, JsonValue* json_root) {
    if (!block || !json_root) return;
    
    // Parse basic properties
    JsonValue* name_val = json_object_get(json_root, "name");
    if (name_val && json_get_type(name_val) == JSON_TYPE_STRING) {
        strncpy(block->name, json_get_string(name_val), sizeof(block->name) - 1);
    }
    
    JsonValue* texture_val = json_object_get(json_root, "texture_path");
    if (texture_val && json_get_type(texture_val) == JSON_TYPE_STRING) {
        strncpy(block->texture_path, json_get_string(texture_val), sizeof(block->texture_path) - 1);
    }
    
    JsonValue* model_val = json_object_get(json_root, "model_path");
    if (model_val && json_get_type(model_val) == JSON_TYPE_STRING) {
        strncpy(block->model_path, json_get_string(model_val), sizeof(block->model_path) - 1);
    }
    
    // Parse color (RGB array)
    JsonValue* color_val = json_object_get(json_root, "color");
    if (color_val && json_get_type(color_val) == JSON_TYPE_ARRAY) {
        JsonArray* color_array = json_get_array(color_val);
        if (json_array_size(color_array) >= 3) {
            block->color.x = (f32)json_get_number(json_array_get(color_array, 0));
            block->color.y = (f32)json_get_number(json_array_get(color_array, 1));
            block->color.z = (f32)json_get_number(json_array_get(color_array, 2));
        }
    }
    
    // Parse boolean properties
    JsonValue* collidable_val = json_object_get(json_root, "collidable");
    if (collidable_val && json_get_type(collidable_val) == JSON_TYPE_BOOLEAN) {
        block->collidable = json_get_boolean(collidable_val);
    }
    
    JsonValue* solid_val = json_object_get(json_root, "solid");
    if (solid_val && json_get_type(solid_val) == JSON_TYPE_BOOLEAN) {
        block->solid = json_get_boolean(solid_val);
    }
    
    JsonValue* transparent_val = json_object_get(json_root, "transparent");
    if (transparent_val && json_get_type(transparent_val) == JSON_TYPE_BOOLEAN) {
        block->transparent = json_get_boolean(transparent_val);
    }
    
    JsonValue* emissive_val = json_object_get(json_root, "emissive");
    if (emissive_val && json_get_type(emissive_val) == JSON_TYPE_BOOLEAN) {
        block->emissive = json_get_boolean(emissive_val);
    }
    
    JsonValue* breakable_val = json_object_get(json_root, "breakable");
    if (breakable_val && json_get_type(breakable_val) == JSON_TYPE_BOOLEAN) {
        block->breakable = json_get_boolean(breakable_val);
    }
    
    // Parse numeric properties
    JsonValue* hardness_val = json_object_get(json_root, "hardness");
    if (hardness_val && json_get_type(hardness_val) == JSON_TYPE_NUMBER) {
        block->hardness = (f32)json_get_number(hardness_val);
    }
}

static void content_creation_parse_item_from_json(ItemDefinition* item, JsonValue* json_root) {
    if (!item || !json_root) return;
    
    // Parse basic properties
    JsonValue* name_val = json_object_get(json_root, "name");
    if (name_val && json_get_type(name_val) == JSON_TYPE_STRING) {
        strncpy(item->name, json_get_string(name_val), sizeof(item->name) - 1);
    }
    
    JsonValue* texture_val = json_object_get(json_root, "texture_path");
    if (texture_val && json_get_type(texture_val) == JSON_TYPE_STRING) {
        strncpy(item->texture_path, json_get_string(texture_val), sizeof(item->texture_path) - 1);
    }
    
    JsonValue* model_val = json_object_get(json_root, "model_path");
    if (model_val && json_get_type(model_val) == JSON_TYPE_STRING) {
        strncpy(item->model_path, json_get_string(model_val), sizeof(item->model_path) - 1);
    }
    
    // Parse numeric properties
    JsonValue* scale_val = json_object_get(json_root, "scale");
    if (scale_val && json_get_type(scale_val) == JSON_TYPE_NUMBER) {
        item->scale = (f32)json_get_number(scale_val);
    }
    
    JsonValue* max_stack_val = json_object_get(json_root, "max_stack_size");
    if (max_stack_val && json_get_type(max_stack_val) == JSON_TYPE_NUMBER) {
        item->max_stack_size = (u32)json_get_number(max_stack_val);
    }
    
    // Parse boolean properties
    JsonValue* consumable_val = json_object_get(json_root, "consumable");
    if (consumable_val && json_get_type(consumable_val) == JSON_TYPE_BOOLEAN) {
        item->consumable = json_get_boolean(consumable_val);
    }
    
    JsonValue* equippable_val = json_object_get(json_root, "equippable");
    if (equippable_val && json_get_type(equippable_val) == JSON_TYPE_BOOLEAN) {
        item->equippable = json_get_boolean(equippable_val);
    }
}

static void content_creation_parse_mob_from_json(MobDefinition* mob, JsonValue* json_root) {
    if (!mob || !json_root) return;
    
    // Parse basic properties
    JsonValue* name_val = json_object_get(json_root, "name");
    if (name_val && json_get_type(name_val) == JSON_TYPE_STRING) {
        strncpy(mob->name, json_get_string(name_val), sizeof(mob->name) - 1);
    }
    
    JsonValue* texture_val = json_object_get(json_root, "texture_path");
    if (texture_val && json_get_type(texture_val) == JSON_TYPE_STRING) {
        strncpy(mob->texture_path, json_get_string(texture_val), sizeof(mob->texture_path) - 1);
    }
    
    JsonValue* model_val = json_object_get(json_root, "model_path");
    if (model_val && json_get_type(model_val) == JSON_TYPE_STRING) {
        strncpy(mob->model_path, json_get_string(model_val), sizeof(mob->model_path) - 1);
    }
    
    JsonValue* ai_val = json_object_get(json_root, "ai_behavior_tree");
    if (ai_val && json_get_type(ai_val) == JSON_TYPE_STRING) {
        strncpy(mob->ai_behavior_tree, json_get_string(ai_val), sizeof(mob->ai_behavior_tree) - 1);
    }
    
    // Parse numeric properties
    JsonValue* width_val = json_object_get(json_root, "width");
    if (width_val && json_get_type(width_val) == JSON_TYPE_NUMBER) {
        mob->width = (f32)json_get_number(width_val);
    }
    
    JsonValue* height_val = json_object_get(json_root, "height");
    if (height_val && json_get_type(height_val) == JSON_TYPE_NUMBER) {
        mob->height = (f32)json_get_number(height_val);
    }
    
    JsonValue* health_val = json_object_get(json_root, "health");
    if (health_val && json_get_type(health_val) == JSON_TYPE_NUMBER) {
        mob->health = (f32)json_get_number(health_val);
        mob->max_health = mob->health;
    }
    
    JsonValue* speed_val = json_object_get(json_root, "speed");
    if (speed_val && json_get_type(speed_val) == JSON_TYPE_NUMBER) {
        mob->speed = (f32)json_get_number(speed_val);
    }
    
    // Parse scale (Vec3 array)
    JsonValue* scale_val = json_object_get(json_root, "scale");
    if (scale_val && json_get_type(scale_val) == JSON_TYPE_ARRAY) {
        JsonArray* scale_array = json_get_array(scale_val);
        if (json_array_size(scale_array) >= 3) {
            mob->scale.x = (f32)json_get_number(json_array_get(scale_array, 0));
            mob->scale.y = (f32)json_get_number(json_array_get(scale_array, 1));
            mob->scale.z = (f32)json_get_number(json_array_get(scale_array, 2));
        }
    }
    
    // Parse boolean properties
    JsonValue* hostile_val = json_object_get(json_root, "hostile");
    if (hostile_val && json_get_type(hostile_val) == JSON_TYPE_BOOLEAN) {
        mob->hostile = json_get_boolean(hostile_val);
    }
}

// MARK: - Preview Entity System

static Entity content_creation_create_preview_entity(ContentType type) {
    // Create a preview entity based on content type
    Entity preview_entity = {0};
    
    // Create entity through ECS system
    preview_entity = ecs_create_entity();
    if (preview_entity.id == 0) {
        LOG_ERROR("Failed to create preview entity");
        return preview_entity;
    }
    
    // Add transform component
    TransformComponent* transform = (TransformComponent*)ecs_add_component(preview_entity.id, COMPONENT_TYPE_TRANSFORM);
    if (!transform) {
        LOG_ERROR("Failed to add transform to preview entity");
        return preview_entity;
    }
    
    // Set initial transform
    transform->position = (vec3_t){0.0f, 0.0f, 0.0f};
    transform->rotation = (vec4_t){0.0f, 0.0f, 0.0f, 1.0f};
    transform->scale = (vec3_t){1.0f, 1.0f, 1.0f};
    
    // Add render component
    RenderComponent* render = (RenderComponent*)ecs_add_component(preview_entity.id, COMPONENT_TYPE_RENDER);
    if (!render) {
        LOG_ERROR("Failed to add render component to preview entity");
        return preview_entity;
    }
    
    // Set default render properties based on content type
    switch (type) {
        case CONTENT_TYPE_BLOCK:
            content_creation_setup_block_preview(preview_entity, render);
            break;
        case CONTENT_TYPE_ITEM:
            content_creation_setup_item_preview(preview_entity, render);
            break;
        case CONTENT_TYPE_MOB:
            content_creation_setup_mob_preview(preview_entity, render);
            break;
        default:
            LOG_ERROR("Unknown content type for preview: %d", type);
            break;
    }
    
    // Add physics component for collision preview
    PhysicsComponent* physics = (PhysicsComponent*)ecs_add_component(preview_entity.id, COMPONENT_TYPE_PHYSICS);
    if (physics) {
        physics->collidable = true;
        physics->solid = true;
        physics->bounds = (vec3_t){0.5f, 0.5f, 0.5f}; // Default bounds
    }
    
    LOG_INFO("Created preview entity for type %d", type);
    return preview_entity;
}

static void content_creation_setup_block_preview(Entity entity, RenderComponent* render) {
    if (!render) return;
    
    // Set default block render properties
    render->mesh_id = 0; // Default cube mesh
    render->material_id = 0; // Default material
    render->visible = true;
    render->cast_shadows = true;
    render->receive_shadows = true;
    
    // Apply current block properties if available
    if (g_context.current_block) {
        content_creation_update_preview_from_block(entity, g_context.current_block);
    }
}

static void content_creation_setup_item_preview(Entity entity, RenderComponent* render) {
    if (!render) return;
    
    // Set default item render properties (smaller scale)
    render->mesh_id = 1; // Default item mesh
    render->material_id = 1; // Default item material
    render->visible = true;
    render->cast_shadows = false; // Items typically don't cast shadows
    render->receive_shadows = true;
    
    // Apply current item properties if available
    if (g_context.current_item) {
        content_creation_update_preview_from_item(entity, g_context.current_item);
    }
}

static void content_creation_setup_mob_preview(Entity entity, RenderComponent* render) {
    if (!render) return;
    
    // Set default mob render properties
    render->mesh_id = 2; // Default humanoid mesh
    render->material_id = 2; // Default mob material
    render->visible = true;
    render->cast_shadows = true;
    render->receive_shadows = true;
    
    // Apply current mob properties if available
    if (g_context.current_mob) {
        content_creation_update_preview_from_mob(entity, g_context.current_mob);
    }
}

static void content_creation_update_preview_from_block(Entity entity, BlockDefinition* block) {
    if (!block) return;
    
    // Update render component
    RenderComponent* render = (RenderComponent*)ecs_get_component(entity.id, COMPONENT_TYPE_RENDER);
    if (render) {
        // Load block texture if specified
        if (strlen(block->texture_path) > 0) {
            render->texture_id = content_creation_load_texture(block->texture_path);
        }
        
        // Load block model if specified
        if (strlen(block->model_path) > 0) {
            render->mesh_id = content_creation_load_mesh(block->model_path);
        }
        
        // Apply color tint
        render->color_tint = block->color;
        render->transparent = block->transparent;
        render->emissive = block->emissive;
        
        // Update physics based on block properties
        PhysicsComponent* physics = (PhysicsComponent*)ecs_get_component(entity.id, COMPONENT_TYPE_PHYSICS);
        if (physics) {
            physics->collidable = block->collidable;
            physics->solid = block->solid;
            physics->bounds = (vec3_t){0.5f, 0.5f, 0.5f}; // Standard block size
        }
    }
    
    // Update transform
    TransformComponent* transform = (TransformComponent*)ecs_get_component(entity.id, COMPONENT_TYPE_TRANSFORM);
    if (transform) {
        transform->position = g_context.preview_position;
    }
}

static void content_creation_update_preview_from_item(Entity entity, ItemDefinition* item) {
    if (!item) return;
    
    // Update render component
    RenderComponent* render = (RenderComponent*)ecs_get_component(entity.id, COMPONENT_TYPE_RENDER);
    if (render) {
        // Load item texture if specified
        if (strlen(item->texture_path) > 0) {
            render->texture_id = content_creation_load_texture(item->texture_path);
        }
        
        // Load item model if specified
        if (strlen(item->model_path) > 0) {
            render->mesh_id = content_creation_load_mesh(item->model_path);
        }
        
        // Apply item scale (items are typically smaller)
        TransformComponent* transform = (TransformComponent*)ecs_get_component(entity.id, COMPONENT_TYPE_TRANSFORM);
        if (transform) {
            transform->scale = (vec3_t){item->scale, item->scale, item->scale};
            transform->position = g_context.preview_position;
        }
        
        // Update physics (items typically don't collide)
        PhysicsComponent* physics = (PhysicsComponent*)ecs_get_component(entity.id, COMPONENT_TYPE_PHYSICS);
        if (physics) {
            physics->collidable = false;
            physics->solid = false;
            physics->bounds = (vec3_t){0.25f, 0.25f, 0.25f}; // Smaller bounds for items
        }
    }
}

static void content_creation_update_preview_from_mob(Entity entity, MobDefinition* mob) {
    if (!mob) return;
    
    // Update render component
    RenderComponent* render = (RenderComponent*)ecs_get_component(entity.id, COMPONENT_TYPE_RENDER);
    if (render) {
        // Load mob texture if specified
        if (strlen(mob->texture_path) > 0) {
            render->texture_id = content_creation_load_texture(mob->texture_path);
        }
        
        // Load mob model if specified
        if (strlen(mob->model_path) > 0) {
            render->mesh_id = content_creation_load_mesh(mob->model_path);
        }
        
        // Update transform with mob dimensions
        TransformComponent* transform = (TransformComponent*)ecs_get_component(entity.id, COMPONENT_TYPE_TRANSFORM);
        if (transform) {
            transform->scale = (vec3_t){mob->width, mob->height, mob->width}; // Use mob dimensions
            transform->position = g_context.preview_position;
        }
        
        // Update physics with mob dimensions
        PhysicsComponent* physics = (PhysicsComponent*)ecs_get_component(entity.id, COMPONENT_TYPE_PHYSICS);
        if (physics) {
            physics->collidable = true;
            physics->solid = true;
            physics->bounds = (vec3_t){mob->width * 0.5f, mob->height * 0.5f, mob->width * 0.5f};
        }
    }
}

static u32 content_creation_load_texture(const char* texture_path) {
    // Load texture through asset manager
    if (g_context.asset_manager) {
        Asset* texture_asset = asset_manager_load(g_context.asset_manager, 
                                                  texture_path, 
                                                  ASSET_TYPE_TEXTURE, 
                                                  texture_path);
        if (texture_asset) {
            return texture_asset->id;
        }
    }
    return 0; // Default texture ID
}

static u32 content_creation_load_mesh(const char* mesh_path) {
    // Load mesh through asset manager
    if (g_context.asset_manager) {
        Asset* mesh_asset = asset_manager_load(g_context.asset_manager, 
                                                mesh_path, 
                                                ASSET_TYPE_MODEL, 
                                                mesh_path);
        if (mesh_asset) {
            return mesh_asset->id;
        }
    }
    return 0; // Default mesh ID
}

static void content_creation_destroy_preview_entity(void) {
    if (g_context.preview_entity.id != 0) {
        // Destroy preview entity through ECS system
        ecs_destroy_entity(g_context.preview_entity.id);
        g_context.preview_entity.id = 0;
        LOG_INFO("Destroyed preview entity");
    }
}

// MARK: - Block Creation Tools

void content_creation_start_block_creation(void) {
    content_creation_reset_context();
    
    g_context.current_type = CONTENT_TYPE_BLOCK;
    g_context.is_editing = true;
    g_context.current_block = content_creation_create_block_definition();
    
    // Create preview entity with default block mesh
    g_context.preview_entity = content_creation_create_preview_entity(CONTENT_TYPE_BLOCK);
    
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
    g_context.preview_entity = content_creation_create_preview_entity(CONTENT_TYPE_BLOCK);
    content_creation_update_preview_from_block(g_context.preview_entity, block);
    
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
        content_creation_update_preview_from_block(g_context.preview_entity, g_context.current_block);
    }
}

void content_creation_set_block_model(const char* model_path) {
    if (g_context.current_block && model_path) {
        strncpy(g_context.current_block->model_path, model_path, sizeof(g_context.current_block->model_path) - 1);
        
        // Update preview model
        content_creation_update_preview_from_block(g_context.preview_entity, g_context.current_block);
    }
}

void content_creation_set_block_color(Vec3 color) {
    if (g_context.current_block) {
        g_context.current_block->color = color;
        
        // Update preview color
        content_creation_update_preview_from_block(g_context.preview_entity, g_context.current_block);
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
        content_creation_update_preview_from_block(g_context.preview_entity, g_context.current_block);
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
    if (!file_path) {
        LOG_ERROR("Invalid file path for block definition");
        return NULL;
    }
    
    // Load and parse JSON file
    JsonValue* json_root = json_parse_file(file_path, NULL);
    if (!json_root) {
        LOG_ERROR("Failed to parse JSON file: %s", file_path);
        return content_creation_create_block_definition();
    }
    
    // Create block definition
    BlockDefinition* block = content_creation_create_block_definition();
    if (!block) {
        json_free(json_root);
        return NULL;
    }
    
    // Parse block properties from JSON
    content_creation_parse_block_from_json(block, json_root);
    
    json_free(json_root);
    LOG_INFO("Loaded block definition from: %s", file_path);
    return block;
}

// MARK: - Item Creation Tools

void content_creation_start_item_creation(void) {
    content_creation_reset_context();
    
    g_context.current_type = CONTENT_TYPE_ITEM;
    g_context.is_editing = true;
    g_context.current_item = content_creation_create_item_definition();
    
    // Create preview entity with default item mesh
    g_context.preview_entity = content_creation_create_preview_entity(CONTENT_TYPE_ITEM);
    
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
    g_context.preview_entity = content_creation_create_preview_entity(CONTENT_TYPE_ITEM);
    content_creation_update_preview_from_item(g_context.preview_entity, item);
    
    g_context.preview_visible = true;
    LOG_INFO("Started editing item: %s", item->name);
}

// ... (rest of the code remains the same)
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
        content_creation_update_preview_from_item(g_context.preview_entity, g_context.current_item);
    }
}

void content_creation_set_item_model(const char* model_path) {
    if (g_context.current_item && model_path) {
        strncpy(g_context.current_item->model_path, model_path, sizeof(g_context.current_item->model_path) - 1);
        
        // Update preview model
        content_creation_update_preview_from_item(g_context.preview_entity, g_context.current_item);
    }
}

void content_creation_set_item_scale(f32 scale) {
    if (g_context.current_item) {
        g_context.current_item->scale = scale;
        
        // Update preview scale
        content_creation_update_preview_from_item(g_context.preview_entity, g_context.current_item);
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
    if (!file_path) {
        LOG_ERROR("Invalid file path for item definition");
        return NULL;
    }
    
    // Load and parse JSON file
    JsonValue* json_root = json_parse_file(file_path, NULL);
    if (!json_root) {
        LOG_ERROR("Failed to parse JSON file: %s", file_path);
        return content_creation_create_item_definition();
    }
    
    // Create item definition
    ItemDefinition* item = content_creation_create_item_definition();
    if (!item) {
        json_free(json_root);
        return NULL;
    }
    
    // Parse item properties from JSON
    content_creation_parse_item_from_json(item, json_root);
    
    json_free(json_root);
    LOG_INFO("Loaded item definition from: %s", file_path);
    return item;
}

// MARK: - Mob Creation Tools

void content_creation_start_mob_creation(void) {
    content_creation_reset_context();
    
    g_context.current_type = CONTENT_TYPE_MOB;
    g_context.is_editing = true;
    g_context.current_mob = content_creation_create_mob_definition();
    
    // Create preview entity with default mob mesh
    g_context.preview_entity = content_creation_create_preview_entity(CONTENT_TYPE_MOB);
    
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
    g_context.preview_entity = content_creation_create_preview_entity(CONTENT_TYPE_MOB);
    content_creation_update_preview_from_mob(g_context.preview_entity, mob);
    
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
        content_creation_update_preview_from_mob(g_context.preview_entity, g_context.current_mob);
    }
}

void content_creation_set_mob_texture(const char* texture_path) {
    if (g_context.current_mob && texture_path) {
        strncpy(g_context.current_mob->texture_path, texture_path, sizeof(g_context.current_mob->texture_path) - 1);
        
        // Update preview texture
        content_creation_update_preview_from_mob(g_context.preview_entity, g_context.current_mob);
    }
}

void content_creation_set_mob_scale(Vec3 scale) {
    if (g_context.current_mob) {
        g_context.current_mob->scale = scale;
        
        // Update preview scale
        content_creation_update_preview_from_mob(g_context.preview_entity, g_context.current_mob);
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
    if (!file_path) {
        LOG_ERROR("Invalid file path for mob definition");
        return NULL;
    }
    
    // Load and parse JSON file
    JsonValue* json_root = json_parse_file(file_path, NULL);
    if (!json_root) {
        LOG_ERROR("Failed to parse JSON file: %s", file_path);
        return content_creation_create_mob_definition();
    }
    
    // Create mob definition
    MobDefinition* mob = content_creation_create_mob_definition();
    if (!mob) {
        json_free(json_root);
        return NULL;
    }
    
    // Parse mob properties from JSON
    content_creation_parse_mob_from_json(mob, json_root);
    
    json_free(json_root);
    LOG_INFO("Loaded mob definition from: %s", file_path);
    return mob;
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
    content_creation_destroy_preview_entity();
    
    memset(&g_context, 0, sizeof(ContentCreationContext));
    g_context.grid_size = 1.0f;
    g_context.grid_snapping_enabled = true;
    g_context.collision_preview_enabled = true;
}
