// Engine stub implementations for frontend linking
// These provide minimal implementations to allow the frontend to build and test

#include "voxelforge_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Core Engine Functions
// ============================================================================

void engine_init(EngineCallbacks callbacks) {
    printf("[Engine Stub] Engine initialized with callbacks\n");
}

void engine_shutdown(void) {
    printf("[Engine Stub] Engine shutdown\n");
}

void engine_update(float delta_time) {
    // Stub implementation
}

// ============================================================================
// Entity Management
// ============================================================================

uint64_t engine_create_entity(const char* name) {
    static uint64_t next_id = 1;
    printf("[Engine Stub] Created entity '%s' with ID %llu\n", name, next_id);
    return next_id++;
}

void engine_delete_entity(uint64_t entity_id) {
    printf("[Engine Stub] Deleted entity %llu\n", entity_id);
}

void engine_set_transform(uint64_t entity_id, TransformData transform) {
    printf("[Engine Stub] Set transform for entity %llu\n", entity_id);
}

TransformData engine_get_transform(uint64_t entity_id) {
    TransformData transform = {0};
    transform.position = (Vec3){0, 0, 0};
    transform.rotation = (Vec3){0, 0, 0};
    transform.scale = (Vec3){1, 1, 1};
    return transform;
}

// ============================================================================
// Component Management
// ============================================================================

void engine_add_component(uint64_t entity_id, ComponentData component) {
    printf("[Engine Stub] Added component %d to entity %llu\n", component.type, entity_id);
}

void engine_remove_component(uint64_t entity_id, int component_type) {
    printf("[Engine Stub] Removed component %d from entity %llu\n", component_type, entity_id);
}

// ============================================================================
// Scene Management
// ============================================================================

void engine_new_scene(void) {
    printf("[Engine Stub] Created new scene\n");
}

void engine_load_scene(const char* path) {
    printf("[Engine Stub] Loaded scene from '%s'\n", path);
}

void engine_save_scene(const char* path) {
    printf("[Engine Stub] Saved scene to '%s'\n", path);
}

// ============================================================================
// Physics
// ============================================================================

void engine_set_physics(uint64_t entity_id, PhysicsData* physics) {
    printf("[Engine Stub] Set physics for entity %llu\n", entity_id);
}

// ============================================================================
// Rendering
// ============================================================================

void engine_set_render_stats(RenderStats* stats) {
    // Stub implementation
}

// ============================================================================
// Utility Functions
// ============================================================================

void engine_log(const char* message, int level, const char* source) {
    const char* level_names[] = {"DEBUG", "INFO", "WARNING", "ERROR"};
    printf("[%s] %s: %s\n", level_names[level], source, message);
}
