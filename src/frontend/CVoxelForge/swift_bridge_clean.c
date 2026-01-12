#include "voxelforge_bridge_clean.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Engine Lifecycle - Minimal Bridge Implementation
// ============================================================================

static Engine *g_engine = NULL;

bool bridge_init_engine(Engine *engine, const EngineConfig *config) { 
    printf("[Bridge] Initializing Engine...\n");
    
    // Store engine reference for bridge use
    g_engine = engine;
    
    // Call actual engine init
    if (engine && config) {
        // This would call the real engine_init function
        // For now, return success
        return true;
    }
    return false;
}

void bridge_shutdown_engine(Engine *engine) {
    printf("[Bridge] Shutting down Engine...\n");
    g_engine = NULL;
}

void bridge_update(float delta_time) {
    if (g_engine) {
        // Update engine systems
        // This would call the actual engine update
        (void)delta_time;
    }
}

// ============================================================================
// Render Stats
// ============================================================================

void bridge_get_render_stats(BridgeRenderStats *stats) {
    if (stats) {
        stats->fps = 60.0f;
        stats->frame_time_ms = 16.67f;
        stats->draw_calls = 1000;
        stats->triangles = 500000;
        stats->vertices = 250000;
        stats->memory_used = 128 * 1024 * 1024; // 128MB
    }
}

// ============================================================================
// Scene Management
// ============================================================================

void bridge_create_scene(const char *name) {
    printf("[Bridge] Creating scene: %s\n", name ? name : "unnamed");
}

void bridge_load_scene(const char *path) {
    printf("[Bridge] Loading scene: %s\n", path ? path : "no path");
}

void bridge_save_scene(const char *path) {
    printf("[Bridge] Saving scene: %s\n", path ? path : "no path");
}

// ============================================================================
// Entity Management
// ============================================================================

uint64_t bridge_create_entity(void) {
    static uint64_t next_id = 1;
    return next_id++;
}

void bridge_destroy_entity(uint64_t entity_id) {
    printf("[Bridge] Destroying entity: %llu\n", entity_id);
}

// ============================================================================
// Physics Interface
// ============================================================================

void bridge_get_physics(uint64_t entity_id, BridgePhysicsData *data) {
    if (data) {
        data->position.x = 0.0f;
        data->position.y = 0.0f;
        data->position.z = 0.0f;
        data->velocity.x = 0.0f;
        data->velocity.y = 0.0f;
        data->velocity.z = 0.0f;
        data->mass = 1.0f;
    }
    (void)entity_id;
}

// ============================================================================
// Material System
// ============================================================================

void bridge_create_material(const char *name) {
    printf("[Bridge] Creating material: %s\n", name ? name : "unnamed");
}

void bridge_set_material_property(const char *property, float value) {
    printf("[Bridge] Setting material property %s to %f\n", property ? property : "unknown", value);
}

// ============================================================================
// Asset Management
// ============================================================================

void bridge_load_asset(const char *path) {
    printf("[Bridge] Loading asset: %s\n", path ? path : "no path");
}

void bridge_unload_asset(const char *path) {
    printf("[Bridge] Unloading asset: %s\n", path ? path : "no path");
}

// ============================================================================
// Debug Interface
// ============================================================================

void bridge_enable_debug_mode(bool enabled) {
    printf("[Bridge] Debug mode %s\n", enabled ? "enabled" : "disabled");
}

void bridge_toggle_wireframe(void) {
    printf("[Bridge] Toggling wireframe mode\n");
}

// ============================================================================
// Hot Reload
// ============================================================================

void bridge_enable_hot_reload(bool enabled) {
    printf("[Bridge] Hot reload %s\n", enabled ? "enabled" : "disabled");
}

void bridge_reload_shaders(void) {
    printf("[Bridge] Reloading shaders\n");
}

// ============================================================================
// Logging
// ============================================================================

void bridge_log(int32_t level, const char *source, const char *message) {
    printf("[%s] %s: %s\n", source ? source : "Bridge", 
           level == 0 ? "INFO" : level == 1 ? "WARN" : "ERROR", 
           message ? message : "");
}
