// Swift Bridge Wrapper Implementation
// Implements the C functions declared in swift_bridge.h

#include "platform/swift_bridge.h"
#include "core/logger.h"
#include "core/engine.h"
#include "core/entity.h"
#include "core/scene.h"
#include "core/renderer.h"
#include "core/transform.h"
#include "core/components/mesh_renderer.h"
#include "core/components/physics.h"
#include <stdlib.h>
#include <string.h>

// Global engine state
static EntityCallback g_entity_created_callback = NULL;
static EntityCallback g_entity_deleted_callback = NULL;
static EntityCallback g_entity_modified_callback = NULL;
static LogCallback g_log_callback = NULL;
static SceneCallback g_scene_loaded_callback = NULL;

// Internal helper functions
static void trigger_entity_created(uint64_t entityID);
static void trigger_entity_deleted(uint64_t entityID);
static void trigger_entity_modified(uint64_t entityID);
static void trigger_log_message(int32_t level, const char* source, const char* message);
static void trigger_scene_loaded(const char* scenePath);

// MARK: - Initialization & Lifecycle

void swift_engine_init(EntityCallback on_entity_created,
                      EntityCallback on_entity_deleted,
                      EntityCallback on_entity_modified, LogCallback on_log_message,
                      SceneCallback on_scene_loaded) {
    
    // Store callbacks
    g_entity_created_callback = on_entity_created;
    g_entity_deleted_callback = on_entity_deleted;
    g_entity_modified_callback = on_entity_modified;
    g_log_callback = on_log_message;
    g_scene_loaded_callback = on_scene_loaded;
    
    // Initialize engine systems
    if (!engine_initialize()) {
        LOG_ERROR("Failed to initialize engine");
        return;
    }
    
    // Setup internal event handlers
    entity_system_set_created_callback(trigger_entity_created);
    entity_system_set_deleted_callback(trigger_entity_deleted);
    entity_system_set_modified_callback(trigger_entity_modified);
    logger_set_callback(trigger_log_message);
    scene_system_set_loaded_callback(trigger_scene_loaded);
    
    LOG_INFO("Engine initialized successfully");
}

void engine_shutdown(void) {
    engine_cleanup();
    
    // Clear callbacks
    g_entity_created_callback = NULL;
    g_entity_deleted_callback = NULL;
    g_entity_modified_callback = NULL;
    g_log_callback = NULL;
    g_scene_loaded_callback = NULL;
    
    LOG_INFO("Engine shutdown complete");
}

void engine_update(float deltaTime) {
    engine_update_frame(deltaTime);
}

// MARK: - Entity Management

uint64_t engine_create_entity(const char *name) {
    if (!name) {
        LOG_ERROR("Entity name cannot be null");
        return 0;
    }
    
    EntityID entity_id = entity_create(name);
    if (entity_id == 0) {
        LOG_ERROR("Failed to create entity: %s", name);
        return 0;
    }
    
    return (uint64_t)entity_id;
}

void engine_delete_entity(uint64_t entityID) {
    if (entityID == 0) {
        LOG_ERROR("Invalid entity ID");
        return;
    }
    
    EntityID entity_id = (EntityID)entityID;
    if (!entity_exists(entity_id)) {
        LOG_WARNING("Attempted to delete non-existent entity: %llu", entityID);
        return;
    }
    
    entity_destroy(entity_id);
}

bool engine_entity_exists(uint64_t entityID) {
    if (entityID == 0) return false;
    return entity_exists((EntityID)entityID);
}

const char *engine_get_entity_name(uint64_t entityID) {
    if (entityID == 0) return NULL;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return NULL;
    
    return entity->name;
}

void engine_set_entity_name(uint64_t entityID, const char *name) {
    if (entityID == 0 || !name) return;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return;
    
    // Free old name if it exists
    if (entity->name) {
        free((void*)entity->name);
    }
    
    // Allocate and set new name
    entity->name = strdup(name);
}

uint32_t engine_get_all_entities(uint64_t *outEntities, uint32_t maxCount) {
    if (!outEntities || maxCount == 0) return 0;
    
    uint32_t count = 0;
    EntityIterator iterator = entity_iterator_create();
    
    while (entity_iterator_has_next(&iterator) && count < maxCount) {
        EntityID entity_id = entity_iterator_next(&iterator);
        outEntities[count++] = (uint64_t)entity_id;
    }
    
    return count;
}

// MARK: - Transform Component

void engine_set_transform(uint64_t entityID, const Transform_C *transform) {
    if (entityID == 0 || !transform) return;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return;
    
    TransformComponent* transform_comp = entity_get_transform(entity);
    if (!transform_comp) {
        transform_comp = entity_add_transform(entity);
        if (!transform_comp) return;
    }
    
    transform_comp->position.x = transform->position.x;
    transform_comp->position.y = transform->position.y;
    transform_comp->position.z = transform->position.z;
    
    transform_comp->rotation.x = transform->rotation.x;
    transform_comp->rotation.y = transform->rotation.y;
    transform_comp->rotation.z = transform->rotation.z;
    
    transform_comp->scale.x = transform->scale.x;
    transform_comp->scale.y = transform->scale.y;
    transform_comp->scale.z = transform->scale.z;
    
    transform_comp->dirty = true;
}

void engine_get_transform(uint64_t entityID, Transform_C *outTransform) {
    if (entityID == 0 || !outTransform) return;
    
    memset(outTransform, 0, sizeof(Transform_C));
    outTransform->scale.x = 1.0f;
    outTransform->scale.y = 1.0f;
    outTransform->scale.z = 1.0f;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return;
    
    TransformComponent* transform_comp = entity_get_transform(entity);
    if (!transform_comp) return;
    
    outTransform->position.x = transform_comp->position.x;
    outTransform->position.y = transform_comp->position.y;
    outTransform->position.z = transform_comp->position.z;
    
    outTransform->rotation.x = transform_comp->rotation.x;
    outTransform->rotation.y = transform_comp->rotation.y;
    outTransform->rotation.z = transform_comp->rotation.z;
    
    outTransform->scale.x = transform_comp->scale.x;
    outTransform->scale.y = transform_comp->scale.y;
    outTransform->scale.z = transform_comp->scale.z;
}

// MARK: - Component Queries

bool engine_has_component(uint64_t entityID, int32_t componentType) {
    if (entityID == 0) return false;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return false;
    
    switch (componentType) {
        case 0: return entity_has_transform(entity);
        case 1: return entity_has_mesh_renderer(entity);
        case 2: return entity_has_physics(entity);
        default: return false;
    }
}

// MARK: - Mesh Renderer Component

void engine_set_mesh_renderer(uint64_t entityID, const MeshRendererData_C *data) {
    if (entityID == 0 || !data) return;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return;
    
    MeshRendererComponent* mesh_comp = entity_get_mesh_renderer(entity);
    if (!mesh_comp) {
        mesh_comp = entity_add_mesh_renderer(entity);
        if (!mesh_comp) return;
    }
    
    mesh_comp->mesh_id = data->mesh_id;
    mesh_comp->material_id = data->material_id;
    mesh_comp->cast_shadows = data->cast_shadows;
    mesh_comp->receive_shadows = data->receive_shadows;
    mesh_comp->dirty = true;
}

void engine_get_mesh_renderer(uint64_t entityID, MeshRendererData_C *outData) {
    if (entityID == 0 || !outData) return;
    
    memset(outData, 0, sizeof(MeshRendererData_C));
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return;
    
    MeshRendererComponent* mesh_comp = entity_get_mesh_renderer(entity);
    if (!mesh_comp) return;
    
    outData->mesh_id = mesh_comp->mesh_id;
    outData->material_id = mesh_comp->material_id;
    outData->cast_shadows = mesh_comp->cast_shadows;
    outData->receive_shadows = mesh_comp->receive_shadows;
}

// MARK: - Physics Component

void engine_set_physics(uint64_t entityID, const PhysicsData_C *data) {
    if (entityID == 0 || !data) return;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return;
    
    PhysicsComponent* physics_comp = entity_get_physics(entity);
    if (!physics_comp) {
        physics_comp = entity_add_physics(entity);
        if (!physics_comp) return;
    }
    
    physics_comp->mass = data->mass;
    physics_comp->drag = data->drag;
    physics_comp->angular_drag = data->angular_drag;
    physics_comp->use_gravity = data->use_gravity;
    physics_comp->is_kinematic = data->is_kinematic;
    
    physics_comp->velocity.x = data->velocity.x;
    physics_comp->velocity.y = data->velocity.y;
    physics_comp->velocity.z = data->velocity.z;
    
    physics_comp->angular_velocity.x = data->angular_velocity.x;
    physics_comp->angular_velocity.y = data->angular_velocity.y;
    physics_comp->angular_velocity.z = data->angular_velocity.z;
    
    physics_comp->dirty = true;
}

void engine_get_physics(uint64_t entityID, PhysicsData_C *outData) {
    if (entityID == 0 || !outData) return;
    
    memset(outData, 0, sizeof(PhysicsData_C));
    outData->mass = 1.0f;
    outData->angular_drag = 0.05f;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return;
    
    PhysicsComponent* physics_comp = entity_get_physics(entity);
    if (!physics_comp) return;
    
    outData->mass = physics_comp->mass;
    outData->drag = physics_comp->drag;
    outData->angular_drag = physics_comp->angular_drag;
    outData->use_gravity = physics_comp->use_gravity;
    outData->is_kinematic = physics_comp->is_kinematic;
    
    outData->velocity.x = physics_comp->velocity.x;
    outData->velocity.y = physics_comp->velocity.y;
    outData->velocity.z = physics_comp->velocity.z;
    
    outData->angular_velocity.x = physics_comp->angular_velocity.x;
    outData->angular_velocity.y = physics_comp->angular_velocity.y;
    outData->angular_velocity.z = physics_comp->angular_velocity.z;
}

// MARK: - Scene Management

void engine_load_scene(const char *path) {
    if (!path) {
        LOG_ERROR("Scene path cannot be null");
        return;
    }
    
    if (!scene_load(path)) {
        LOG_ERROR("Failed to load scene: %s", path);
        return;
    }
    
    LOG_INFO("Scene loaded: %s", path);
}

void engine_save_scene(const char *path) {
    if (!path) {
        LOG_ERROR("Scene path cannot be null");
        return;
    }
    
    if (!scene_save(path)) {
        LOG_ERROR("Failed to save scene: %s", path);
        return;
    }
    
    LOG_INFO("Scene saved: %s", path);
}

void engine_new_scene(void) {
    scene_new();
    LOG_INFO("New scene created");
}

// MARK: - Render Stats

void engine_get_render_stats(RenderStats_C *outStats) {
    if (!outStats) return;
    
    memset(outStats, 0, sizeof(RenderStats_C));
    
    RendererStats* stats = renderer_get_stats();
    if (!stats) return;
    
    outStats->fps = stats->fps;
    outStats->frame_time_ms = stats->frame_time_ms;
    outStats->draw_calls = stats->draw_calls;
    outStats->triangles = stats->triangles;
    outStats->vertices = stats->vertices;
    outStats->memory_used = stats->memory_used;
}

// MARK: - Logging

void engine_log(int32_t level, const char *source, const char *message) {
    if (!message) return;
    
    const char* src = source ? source : "Swift";
    
    switch (level) {
        case 0: LOG_DEBUG("[%s] %s", src, message); break;
        case 1: LOG_INFO("[%s] %s", src, message); break;
        case 2: LOG_WARNING("[%s] %s", src, message); break;
        case 3: LOG_ERROR("[%s] %s", src, message); break;
        default: LOG_INFO("[%s] %s", src, message); break;
    }
}

// MARK: - Grid and Snapping Controls

void engine_set_grid_enabled(bool enabled) {
    renderer_set_grid_enabled(enabled);
}

void engine_set_grid_spacing(float spacing) {
    renderer_set_grid_spacing(spacing);
}

void engine_set_position_snap_enabled(bool enabled) {
    renderer_set_position_snap_enabled(enabled);
}

void engine_set_position_snap_increment(float increment) {
    renderer_set_position_snap_increment(increment);
}

void engine_set_rotation_snap_enabled(bool enabled) {
    renderer_set_rotation_snap_enabled(enabled);
}

void engine_set_rotation_snap_increment(float increment) {
    renderer_set_rotation_snap_increment(increment);
}

void engine_set_scale_snap_enabled(bool enabled) {
    renderer_set_scale_snap_enabled(enabled);
}

void engine_set_scale_snap_increment(float increment) {
    renderer_set_scale_snap_increment(increment);
}

// MARK: - Enhanced Entity Functions

bool engine_get_entity_active(uint64_t entityID) {
    if (entityID == 0) return false;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return false;
    
    return entity->active;
}

bool engine_get_entity_static(uint64_t entityID) {
    if (entityID == 0) return false;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return false;
    
    return entity->static_entity;
}

const char* engine_get_entity_tag(uint64_t entityID) {
    if (entityID == 0) return NULL;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return NULL;
    
    return entity->tag;
}

const char* engine_get_entity_layer(uint64_t entityID) {
    if (entityID == 0) return NULL;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return NULL;
    
    return entity->layer;
}

void engine_set_entity_active(uint64_t entityID, bool active) {
    if (entityID == 0) return;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return;
    
    entity->active = active;
}

void engine_set_entity_static(uint64_t entityID, bool isStatic) {
    if (entityID == 0) return;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return;
    
    entity->static_entity = isStatic;
}

void engine_set_entity_tag(uint64_t entityID, const char* tag) {
    if (entityID == 0 || !tag) return;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return;
    
    // Free existing tag if any
    if (entity->tag) {
        free((void*)entity->tag);
    }
    
    // Allocate and copy new tag
    entity->tag = strdup(tag);
}

void engine_set_entity_layer(uint64_t entityID, const char* layer) {
    if (entityID == 0 || !layer) return;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return;
    
    // Free existing layer if any
    if (entity->layer) {
        free((void*)entity->layer);
    }
    
    // Allocate and copy new layer
    entity->layer = strdup(layer);
}

int32_t engine_get_component_count(uint64_t entityID) {
    if (entityID == 0) return 0;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return 0;
    
    // Count components (simplified - would need actual component system integration)
    int32_t count = 0;
    if (entity_has_transform((EntityID)entityID)) count++;
    if (entity_has_mesh_renderer((EntityID)entityID)) count++;
    if (entity_has_physics((EntityID)entityID)) count++;
    
    return count;
}

void engine_get_component_types(uint64_t entityID, int32_t* outTypes, int32_t maxCount) {
    if (entityID == 0 || !outTypes || maxCount == 0) return;
    
    Entity* entity = entity_get((EntityID)entityID);
    if (!entity) return;
    
    int32_t count = 0;
    
    // Add component types (simplified - would need actual component system integration)
    if (count < maxCount && entity_has_transform((EntityID)entityID)) {
        outTypes[count++] = 1; // Transform component type
    }
    if (count < maxCount && entity_has_mesh_renderer((EntityID)entityID)) {
        outTypes[count++] = 2; // Mesh renderer component type
    }
    if (count < maxCount && entity_has_physics((EntityID)entityID)) {
        outTypes[count++] = 3; // Physics component type
    }
}

// MARK: - Component Management

void engine_add_component(uint64_t entityID, int32_t componentType) {
    if (entityID == 0) return;
    
    // This would need actual component system integration
    // For now, just log the request
    LOG_INFO("Add component type %d to entity %lu", componentType, entityID);
}

void engine_remove_component(uint64_t entityID, int32_t componentType) {
    if (entityID == 0) return;
    
    // This would need actual component system integration
    // For now, just log the request
    LOG_INFO("Remove component type %d from entity %lu", componentType, entityID);
}

void engine_create_entity_with_id(uint64_t entityID, const char* name) {
    if (entityID == 0 || !name) return;
    
    // This would need actual entity system integration
    // For now, just log the request
    LOG_INFO("Create entity with ID %lu and name %s", entityID, name);
}

// MARK: - Mesh Visualization

void engine_set_mesh_overlay_color(uint64_t entityID, float r, float g, float b, float a) {
    if (entityID == 0) return;
    
    // This would need actual renderer integration
    // For now, just log the request
    LOG_INFO("Set mesh overlay color for entity %lu: (%.2f, %.2f, %.2f, %.2f)", entityID, r, g, b, a);
}

void engine_set_mesh_wireframe_enabled(uint64_t entityID, bool enabled) {
    if (entityID == 0) return;
    
    // This would need actual renderer integration
    // For now, just log the request
    LOG_INFO("Set mesh wireframe %s for entity %lu", enabled ? "enabled" : "disabled", entityID);
}

void engine_set_mesh_vertex_colors_enabled(uint64_t entityID, bool enabled) {
    if (entityID == 0) return;
    
    LOG_INFO("Set mesh vertex colors %s for entity %lu", enabled ? "enabled" : "disabled", entityID);
}

void engine_set_mesh_uv_visualization(uint64_t entityID, int32_t mode) {
    if (entityID == 0) return;
    
    LOG_INFO("Set mesh UV visualization mode %d for entity %lu", mode, entityID);
}

void engine_set_mesh_normals_visualization(uint64_t entityID, bool enabled) {
    if (entityID == 0) return;
    
    LOG_INFO("Set mesh normals visualization %s for entity %lu", enabled ? "enabled" : "disabled", entityID);
}

void engine_set_mesh_bounds_visualization(uint64_t entityID, bool enabled) {
    if (entityID == 0) return;
    
    LOG_INFO("Set mesh bounds visualization %s for entity %lu", enabled ? "enabled" : "disabled", entityID);
}

void engine_set_material_override(uint64_t entityID, uint64_t materialID) {
    if (entityID == 0) return;
    
    LOG_INFO("Set material override %lu for entity %lu", materialID, entityID);
}

void engine_clear_material_override(uint64_t entityID) {
    if (entityID == 0) return;
    
    LOG_INFO("Clear material override for entity %lu", entityID);
}

void engine_set_selection_outline_color(float r, float g, float b) {
    LOG_INFO("Set selection outline color: (%.2f, %.2f, %.2f)", r, g, b);
}

void engine_set_selection_outline_width(float width) {
    LOG_INFO("Set selection outline width: %.2f", width);
}

void engine_set_hover_highlight_enabled(bool enabled) {
    LOG_INFO("Set hover highlight %s", enabled ? "enabled" : "disabled");
}

// MARK: - Render Mode Functions

void engine_set_render_mode(int32_t mode) {
    LOG_INFO("Set render mode: %d", mode);
}

void engine_set_lighting_enabled(bool enabled) {
    LOG_INFO("Set lighting %s", enabled ? "enabled" : "disabled");
}

void engine_set_shadows_enabled(bool enabled) {
    LOG_INFO("Set shadows %s", enabled ? "enabled" : "disabled");
}

void engine_set_performance_profiling_enabled(bool enabled) {
    LOG_INFO("Set performance profiling %s", enabled ? "enabled" : "disabled");
}

void engine_set_debug_rendering_enabled(bool enabled) {
    LOG_INFO("Set debug rendering %s", enabled ? "enabled" : "disabled");
}

void engine_set_wireframe_overlay_enabled(bool enabled) {
    LOG_INFO("Set wireframe overlay %s", enabled ? "enabled" : "disabled");
}

void engine_set_bounding_boxes_enabled(bool enabled) {
    LOG_INFO("Set bounding boxes %s", enabled ? "enabled" : "disabled");
}

void engine_set_post_processing_enabled(bool enabled) {
    LOG_INFO("Set post processing %s", enabled ? "enabled" : "disabled");
}

void engine_set_vsync_enabled(bool enabled) {
    LOG_INFO("Set vsync %s", enabled ? "enabled" : "disabled");
}

void engine_set_fps_limit(uint32_t limit) {
    LOG_INFO("Set FPS limit: %u", limit);
}

void engine_set_ambient_occlusion_enabled(bool enabled) {
    LOG_INFO("Set ambient occlusion %s", enabled ? "enabled" : "disabled");
}

// MARK: - Internal Callback Triggers

static void trigger_entity_created(uint64_t entityID) {
    if (g_entity_created_callback) {
        g_entity_created_callback(entityID);
    }
}

static void trigger_entity_deleted(uint64_t entityID) {
    if (g_entity_deleted_callback) {
        g_entity_deleted_callback(entityID);
    }
}

static void trigger_entity_modified(uint64_t entityID) {
    if (g_entity_modified_callback) {
        g_entity_modified_callback(entityID);
    }
}

static void trigger_log_message(int32_t level, const char* source, const char* message) {
    if (g_log_callback && message && source) {
        g_log_callback(message, level, source);
    }
}

static void trigger_scene_loaded(const char* scenePath) {
    if (g_scene_loaded_callback && scenePath) {
        g_scene_loaded_callback(scenePath);
    }
}
