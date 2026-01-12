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
}

void engine_shutdown(void) {
}

void engine_update(float delta_time) {
    // Stub implementation
}

// ============================================================================
// Entity Management
// ============================================================================

uint64_t engine_create_entity(const char* name) {
    static uint64_t next_id = 1;
    return next_id++;
}

void engine_delete_entity(uint64_t entity_id) {
}

void engine_set_transform(uint64_t entity_id, TransformData transform) {
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
}

void engine_remove_component(uint64_t entity_id, int component_type) {
}

// ============================================================================
// Scene Management
// ============================================================================

void engine_new_scene(void) {
}

void engine_load_scene(const char* path) {
}

void engine_save_scene(const char* path) {
}

// ============================================================================
// Physics
// ============================================================================

void engine_set_physics(uint64_t entity_id, PhysicsData* physics) {
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
}
