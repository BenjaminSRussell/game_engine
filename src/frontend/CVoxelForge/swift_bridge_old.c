#include "voxelforge_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Engine Includes
#include "engine/include/core/logger.h"
#include "core/types.h"
#include "core/engine.h"
#include "core/gamestate.h"
#include "renderer/renderer.h"
#include "renderer/core/scene.h"
#include "scene/scene_manager.h"
#include "entity/entity.h"
#include "entity/component_manager.h"
#include "input/input.h"

// ============================================================================
// Core Engine Lifecycle - Use actual engine API
// ============================================================================

static Engine *g_engine = NULL;

bool engine_init(Engine *engine, const EngineConfig *config) { 
    LOG_INFO("[Bridge] Initializing Engine...");
    
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

void engine_shutdown(Engine *engine) {
    LOG_INFO("[Bridge] Shutting down Engine...");
    g_engine = NULL;
}

void swiftbridge_update(float delta_time) {
    if (g_engine) {
        // Update engine systems
        // This would call the actual engine update
        (void)delta_time;
    }
}
    config.window_height = 720;
    config.window_title = "VoxelForge Studio";
    config.headless = false; // We want rendering, but maybe not a window? 
                             // If Swift hosts the window, we might need a special init.
                             // For now, assuming standard engine init.
                             
    if (!engine_initialize(config)) {
        LOG_ERROR("[Bridge] Failed to initialize engine");
        return;
    }
    
    // Initialize Editor Context if needed
    // editor_init();
    
    LOG_INFO("[Bridge] Engine Initialized");
}

void engine_shutdown(void) { 
    LOG_INFO("[Bridge] Shutting down Engine...");
    engine_shutdown_subsystems();
}

void swiftbridge_update(float delta_time) {
    // Forward update to engine
    engine_update(delta_time);
}

// ============================================================================
// Render Stats
// ============================================================================

void engine_get_render_stats(RenderStats_C *stats) {
    if (stats) {
        // Fetch real stats from renderer
        RendererStats rstats = renderer_get_stats();
        stats->fps = 1.0f / 0.016f; // TODO: Get actual frame time
        stats->frame_time_ms = 16.67f;
        stats->draw_calls = rstats.draw_calls;
        stats->triangles = rstats.triangles;
        stats->vertices = rstats.vertices;
        stats->memory_used = 0; // TODO: Track memory
    }
}

// ============================================================================
// Entity Management
// ============================================================================

uint64_t engine_create_entity(const char *name) {
    Entity entity = entity_create(name);
    return (uint64_t)entity.id;
}

uint64_t engine_create_entity_with_id(uint64_t id, const char *name) {
    // This function might not exist in core yet, falling back to create
    // entity_create_with_id(id, name);
    Entity entity = entity_create(name); // TODO: Support ID forcing
    return (uint64_t)entity.id;
}

void engine_delete_entity(uint64_t entity_id) {
    entity_destroy((EntityID)entity_id);
}

const char *engine_get_entity_name(uint64_t entity_id) {
    // Warning: returning pointer to internal memory or static buffer
    // entity_get_name returns a pointer to the component data string
    return entity_get_name((EntityID)entity_id);
}

void engine_set_entity_name(uint64_t entity_id, const char *name) {
    entity_set_name((EntityID)entity_id, name);
}

uint64_t engine_get_entity_parent(uint64_t entity_id) { 
    return (uint64_t)entity_get_parent((EntityID)entity_id); 
}

void engine_set_entity_parent(uint64_t entity_id, uint64_t parent_id) {
    entity_set_parent((EntityID)entity_id, (EntityID)parent_id);
}

// ============================================================================
// Transform
// ============================================================================

void engine_get_transform(uint64_t entity_id, Transform_C *t) {
    if (!t) return;
    
    TransformComponent* tc = component_get_transform((EntityID)entity_id);
    if (tc) {
        memcpy(t->position, tc->position, sizeof(float)*3);
        // Quat to Euler conversion needed here if tc stores quaternion
        // Assuming TC stores position/rotation/scale for now or we convert
        // For simplicity reusing memory layout if matches, else copy:
        memcpy(t->rotation, tc->rotation, sizeof(float)*3); 
        memcpy(t->scale, tc->scale, sizeof(float)*3);
    }
}

void engine_set_transform(uint64_t entity_id, const Transform_C *t) {
    if (!t) return;
    
    TransformComponent* tc = component_get_transform((EntityID)entity_id);
    if (tc) {
         memcpy(tc->position, t->position, sizeof(float)*3);
         memcpy(tc->rotation, t->rotation, sizeof(float)*3);
         memcpy(tc->scale, t->scale, sizeof(float)*3);
         
         // Mark dirty
         transform_mark_dirty((EntityID)entity_id);
    }
}

// ============================================================================
// Components
// ============================================================================

bool engine_has_component(uint64_t entity_id, int32_t type) { 
    return entity_has_component((EntityID)entity_id, (ComponentType)type); 
}

void engine_add_component(uint64_t entity_id, int32_t type) {
    entity_add_component((EntityID)entity_id, (ComponentType)type);
}

void engine_remove_component(uint64_t entity_id, int32_t type) {
    entity_remove_component((EntityID)entity_id, (ComponentType)type);
}

// ============================================================================
// Mesh Renderer
// ============================================================================

void engine_set_mesh_renderer(uint64_t entity_id, const MeshRendererData_C *d) {
    if (!d) return;
    MeshComponent* mc = component_get_mesh((EntityID)entity_id);
    if (!mc) {
        mc = (MeshComponent*)entity_add_component((EntityID)entity_id, COMPONENT_MESH_RENDERER);
    }
    
    if (mc) {
        mc->mesh_resource_id = d->mesh_id;
        mc->material_resource_id = d->material_id;
        mc->cast_shadows = d->cast_shadows;
        mc->receive_shadows = d->receive_shadows;
    }
}

void engine_get_mesh_renderer(uint64_t entity_id, MeshRendererData_C *d) {
    if (!d) return;
    MeshComponent* mc = component_get_mesh((EntityID)entity_id);
    if (mc) {
        d->mesh_id = mc->mesh_resource_id;
        d->material_id = mc->material_resource_id;
        d->cast_shadows = mc->cast_shadows;
        d->receive_shadows = mc->receive_shadows;
    }
}

// ============================================================================
// Scene
// ============================================================================

void engine_save_scene(const char *path) {
    scene_save(path);
}

void engine_load_scene(const char *path) {
    scene_load(path);
}

void engine_new_scene(void) { 
    scene_clear();
}

// ============================================================================
// Logging
// ============================================================================

void engine_log(int32_t level, const char *source, const char *msg) {
    // Bridge log from Swift back to C Logger
    switch(level) {
        case 0: LOG_DEBUG("[%s] %s", source, msg); break;
        case 1: LOG_INFO("[%s] %s", source, msg); break;
        case 2: LOG_WARN("[%s] %s", source, msg); break;
        case 3: LOG_ERROR("[%s] %s", source, msg); break;
        default: LOG_INFO("[%s] %s", source, msg); break;
    }
}

// ============================================================================
// Selection, Gizmos, Camera (Stubs for now, need Editor Context integration)
// ============================================================================

void engine_set_selected_entity(uint64_t entity_id) {
    // editor_context_select_entity(entity_id);
}

void engine_set_gizmo_mode(int32_t mode) {}
void engine_set_gizmo_space(int32_t space) {}

// ... Rest of functions remain stubs or map to editor_context ...

// Component Introspection (Stubbed for now)
int32_t engine_get_component_count(uint64_t entity_id) { return 0; }
int32_t engine_get_component_types(uint64_t entity_id, const char **out_types, int32_t max_count) { return 0; }

// Entity Metadata Linkage
bool engine_get_entity_active(uint64_t entity_id) { return true; }
void engine_set_entity_active(uint64_t entity_id, bool active) {}
const char *engine_get_entity_layer(uint64_t entity_id) { return "Default"; }
void engine_set_entity_layer(uint64_t entity_id, const char *layer) {}
bool engine_get_entity_static(uint64_t entity_id) { return false; }
void engine_set_entity_static(uint64_t entity_id, bool is_static) {}
const char *engine_get_entity_tag(uint64_t entity_id) { return "Untagged"; }
void engine_set_entity_tag(uint64_t entity_id, const char *tag) {}

// Hierarchy
int32_t engine_get_entity_children(uint64_t entity_id, uint64_t *out_children, int32_t max_count) { return 0; }
int32_t engine_get_root_entities(uint64_t *out_entities, int32_t max_count) { return 0; }

// Visualization Stubs
void engine_set_mesh_overlay_color(uint64_t entity_id, float r, float g, float b, float a) {}
void engine_set_mesh_wireframe_enabled(uint64_t entity_id, bool enabled) {}
void engine_set_mesh_vertex_colors_enabled(uint64_t entity_id, bool enabled) {}
void engine_set_mesh_uv_visualization(uint64_t entity_id, int32_t mode) {}
void engine_set_mesh_normals_visualization(uint64_t entity_id, bool enabled) {}
void engine_set_mesh_bounds_visualization(uint64_t entity_id, bool enabled) {}
void engine_set_material_override(uint64_t entity_id, uint64_t material_id) {}
void engine_clear_material_override(uint64_t entity_id) {}
void engine_set_selection_outline_color(float r, float g, float b) {}
void engine_set_selection_outline_width(float width) {}
void engine_set_hover_highlight_enabled(bool enabled) {}
void engine_set_render_mode(int32_t mode) {}
void engine_set_lighting_enabled(bool enabled) {}
void engine_set_shadows_enabled(bool enabled) {}
void engine_set_ambient_occlusion_enabled(bool enabled) {}
void engine_set_position_snap_enabled(bool e) {}
void engine_set_position_snap_increment(float i) {}
void engine_set_rotation_snap_enabled(bool e) {}
void engine_set_rotation_snap_increment(float i) {}
void engine_set_scale_snap_enabled(bool e) {}
void engine_set_scale_snap_increment(float i) {}
void engine_set_editor_camera_position(float x, float y, float z) {}
void engine_set_editor_camera_rotation(float p, float y) {}
void engine_profiler_begin_frame(void) {}
void engine_profiler_end_frame(void) {}
void engine_set_grid_enabled(bool e) {}
void engine_set_grid_spacing(float s) {}

