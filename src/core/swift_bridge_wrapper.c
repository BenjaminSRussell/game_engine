// Simplified Swift Bridge Wrapper Implementation
// Implements basic C functions declared in swift_bridge.h

#include "platform/swift_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple logger stub for testing
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

// Global engine state
static EntityCallback g_entity_created_callback = NULL;
static EntityCallback g_entity_deleted_callback = NULL;
static EntityCallback g_entity_modified_callback = NULL;
static LogCallback g_log_callback = NULL;
static SceneCallback g_scene_loaded_callback = NULL;

// MARK: - Internal Callback Handlers

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

static void trigger_log_message(const char *message, int32_t level, const char *source) {
    if (g_log_callback) {
        g_log_callback(message, level, source);
    }
}

static void trigger_scene_loaded(const char *scenePath) {
    if (g_scene_loaded_callback) {
        g_scene_loaded_callback(scenePath);
    }
}

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
    
    LOG_INFO("Engine initialized successfully");
}

void swift_engine_shutdown(void) {
    LOG_INFO("Swift engine shutdown called");
}

void swift_engine_update(float deltaTime) {
    LOG_INFO("Swift engine update: %.3f", deltaTime);
}

// MARK: - Entity Management

uint64_t engine_create_entity(const char *name) {
    if (!name) {
        LOG_ERROR("Entity name cannot be null");
        return 0;
    }
    
    static uint32_t next_entity_id = 1;
    uint32_t entity_id = next_entity_id++;
    
    LOG_INFO("Created entity %u with name '%s'", entity_id, name);
    return (uint64_t)entity_id;
}

void engine_delete_entity(uint64_t entityID) {
    if (entityID == 0) return;
    
    LOG_INFO("Deleted entity %llu", entityID);
}

bool engine_entity_exists(uint64_t entityID) {
    if (entityID == 0) return false;
    
    // Simple stub - assume entities with ID > 0 exist
    return true;
}

const char *engine_get_entity_name(uint64_t entityID) {
    if (entityID == 0) return NULL;
    
    // Return a static name for now
    static char name_buffer[256];
    snprintf(name_buffer, sizeof(name_buffer), "Entity_%llu", entityID);
    return name_buffer;
}

void engine_set_entity_name(uint64_t entityID, const char *name) {
    if (entityID == 0 || !name) return;
    
    LOG_INFO("Set entity %llu name to '%s'", entityID, name);
}

uint32_t engine_get_all_entities(uint64_t *outEntities, uint32_t maxCount) {
    if (!outEntities || maxCount == 0) return 0;
    
    // Return some test entities
    uint32_t count = (maxCount < 3) ? maxCount : 3;
    for (uint32_t i = 0; i < count; i++) {
        outEntities[i] = i + 1;
    }
    
    LOG_INFO("Getting %u entities", count);
    return count;
}

bool engine_has_component(uint64_t entityID, int32_t componentType) {
    if (entityID == 0) return false;
    
    // Simple stub - assume all entities have basic components
    return componentType >= 0 && componentType <= 2;
}

// MARK: - Transform Component

void engine_set_transform(uint64_t entityID, const Transform_C *transform) {
    if (entityID == 0 || !transform) return;
    
    LOG_INFO("Setting transform for entity %llu", entityID);
}

void engine_get_transform(uint64_t entityID, Transform_C *outTransform) {
    if (entityID == 0 || !outTransform) return;
    
    // Return default transform
    outTransform->position.x = 0.0f; outTransform->position.y = 0.0f; outTransform->position.z = 0.0f;
    outTransform->rotation.x = 0.0f; outTransform->rotation.y = 0.0f; outTransform->rotation.z = 0.0f;
    outTransform->scale.x = 1.0f; outTransform->scale.y = 1.0f; outTransform->scale.z = 1.0f;
    
    LOG_DEBUG("Getting transform for entity %llu", entityID);
}

// MARK: - Mesh Renderer Component

void engine_set_mesh_renderer(uint64_t entityID, const MeshRendererData_C *data) {
    if (entityID == 0 || !data) return;
    
    LOG_INFO("Setting mesh renderer for entity %llu", entityID);
}

void engine_get_mesh_renderer(uint64_t entityID, MeshRendererData_C *outData) {
    if (entityID == 0 || !outData) return;
    
    memset(outData, 0, sizeof(MeshRendererData_C));
    LOG_DEBUG("Getting mesh renderer for entity %llu", entityID);
}

// MARK: - Physics Component

void engine_set_physics(uint64_t entityID, const PhysicsData_C *data) {
    if (entityID == 0 || !data) return;
    
    LOG_INFO("Setting physics for entity %llu", entityID);
}

void swift_engine_get_physics_data(uint64_t entityID, PhysicsData_C *outData) {
    if (!outData) {
        LOG_ERROR("Physics data output parameter cannot be null");
        return;
    }
    
    // Initialize with default values
    outData->velocity.x = 0.0f;
    outData->velocity.y = 0.0f;
    outData->velocity.z = 0.0f;
    outData->angular_velocity.x = 0.0f;
    outData->angular_velocity.y = 0.0f;
    outData->angular_velocity.z = 0.0f;
    outData->mass = 1.0f;
    
    LOG_DEBUG("Getting physics data for entity %llu", entityID);
}

// MARK: - Scene Management

void engine_load_scene(const char *path) {
    if (!path) return;
    
    LOG_INFO("Loading scene from '%s'", path);
}

void engine_save_scene(const char *path) {
    if (!path) return;
    
    LOG_INFO("Saving scene to '%s'", path);
}

void engine_new_scene(void) {
    LOG_INFO("Creating new scene");
}

// MARK: - Render Stats

void engine_get_render_stats(RenderStats_C *outStats) {
    if (!outStats) return;
    
    memset(outStats, 0, sizeof(RenderStats_C));
    outStats->frame_time_ms = 16.67f; // 60 FPS
    outStats->draw_calls = 100;
    outStats->triangles = 50000;
    outStats->vertices = 25000;
    outStats->memory_used = 1024 * 1024; // 1MB
    
    LOG_DEBUG("Getting render stats");
}

// MARK: - Logging

void engine_log_message(int level, const char *message) {
    if (!message) return;
    
    switch (level) {
        case 0: LOG_INFO("%s", message); break;
        case 1: LOG_DEBUG("%s", message); break;
        case 2: LOG_ERROR("%s", message); break;
        default: LOG_INFO("%s", message); break;
    }
}
