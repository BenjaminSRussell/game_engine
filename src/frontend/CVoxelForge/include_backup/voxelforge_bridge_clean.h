#ifndef VOXELFORGE_SWIFT_BRIDGE_H
#define VOXELFORGE_SWIFT_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Engine Lifecycle - Minimal Bridge API
// ============================================================================

typedef struct Engine Engine;
typedef struct EngineConfig EngineConfig;

bool bridge_init_engine(Engine *engine, const EngineConfig *config);
void bridge_shutdown_engine(Engine *engine);
void bridge_update(float delta_time);

// ============================================================================
// Render Stats
// ============================================================================

typedef struct {
    float fps;
    float frame_time_ms;
    uint32_t draw_calls;
    uint32_t triangles;
    uint32_t vertices;
    uint64_t memory_used;
} BridgeRenderStats;

void bridge_get_render_stats(BridgeRenderStats *stats);

// ============================================================================
// Scene Management
// ============================================================================

void bridge_create_scene(const char *name);
void bridge_load_scene(const char *path);
void bridge_save_scene(const char *path);

// ============================================================================
// Entity Management
// ============================================================================

uint64_t bridge_create_entity(void);
void bridge_destroy_entity(uint64_t entity_id);

// ============================================================================
// Physics Interface
// ============================================================================

typedef struct {
    float x, y, z;
} BridgeVec3;

typedef struct {
    BridgeVec3 position;
    BridgeVec3 velocity;
    float mass;
} BridgePhysicsData;

void bridge_get_physics(uint64_t entity_id, BridgePhysicsData *data);

// ============================================================================
// Material System
// ============================================================================

void bridge_create_material(const char *name);
void bridge_set_material_property(const char *property, float value);

// ============================================================================
// Asset Management
// ============================================================================

void bridge_load_asset(const char *path);
void bridge_unload_asset(const char *path);

// ============================================================================
// Debug Interface
// ============================================================================

void bridge_enable_debug_mode(bool enabled);
void bridge_toggle_wireframe(void);

// ============================================================================
// Hot Reload
// ============================================================================

void bridge_enable_hot_reload(bool enabled);
void bridge_reload_shaders(void);

// ============================================================================
// Logging
// ============================================================================

void bridge_log(int32_t level, const char *source, const char *message);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_SWIFT_BRIDGE_H
