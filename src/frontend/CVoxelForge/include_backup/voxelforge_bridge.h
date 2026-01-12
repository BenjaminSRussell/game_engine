#ifndef VOXELFORGE_SWIFT_BRIDGE_H
#define VOXELFORGE_SWIFT_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Types and Callbacks
// ============================================================================

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
} TransformData;

typedef struct {
    int type;
    void* data;
} ComponentData;

typedef struct {
    void (*on_entity_created)(uint64_t entity_id);
    void (*on_entity_deleted)(uint64_t entity_id);
    void (*on_entity_modified)(uint64_t entity_id);
    void (*on_log_message)(const char* message, int level, const char* source);
    void (*on_scene_loaded)(const char* scene_path);
} EngineCallbacks;

typedef struct {
    float fps;
    float frame_time_ms;
    uint32_t draw_calls;
    uint32_t triangles;
    uint32_t vertices;
    uint64_t memory_used;
} RenderStats;

typedef struct {
    float mass;
    float drag;
    int is_kinematic;
} PhysicsData;

// ============================================================================
// Engine Lifecycle
// ============================================================================

void engine_init(EngineCallbacks callbacks);
void engine_shutdown(void);
void engine_update(float delta_time);

// ============================================================================
// Entity Management
// ============================================================================

uint64_t engine_create_entity(const char *name);
uint64_t engine_create_entity_with_id(uint64_t id, const char *name);
void engine_delete_entity(uint64_t entity_id);
const char *engine_get_entity_name(uint64_t entity_id);
void engine_set_entity_name(uint64_t entity_id, const char *name);
uint64_t engine_get_entity_parent(uint64_t entity_id);
void engine_set_entity_parent(uint64_t entity_id, uint64_t parent_id);

// ============================================================================
// Components
// ============================================================================

typedef enum {
  COMPONENT_TRANSFORM = 0,
  COMPONENT_MESH_RENDERER = 1,
  COMPONENT_PHYSICS = 2
} ComponentType;

bool engine_has_component(uint64_t entity_id, int32_t type);
void engine_add_component(uint64_t entity_id, int32_t type);
void engine_remove_component(uint64_t entity_id, int32_t type);

// Component Introspection (Phase 1)
int32_t engine_get_component_count(uint64_t entity_id);
int32_t engine_get_component_types(uint64_t entity_id, const char **out_types,
                                   int32_t max_count);

// Entity Metadata (Phase 2)
bool engine_get_entity_active(uint64_t entity_id);
void engine_set_entity_active(uint64_t entity_id, bool active);
const char *engine_get_entity_layer(uint64_t entity_id);
void engine_set_entity_layer(uint64_t entity_id, const char *layer);
bool engine_get_entity_static(uint64_t entity_id);
void engine_set_entity_static(uint64_t entity_id, bool is_static);
const char *engine_get_entity_tag(uint64_t entity_id);
void engine_set_entity_tag(uint64_t entity_id, const char *tag);
void engine_set_transform(uint64_t entity_id, TransformData transform);
TransformData engine_get_transform(uint64_t entity_id);

// Scene Hierarchy (Phase 3)
int32_t engine_get_entity_children(uint64_t entity_id, uint64_t *out_children,
                                   int32_t max_count);
int32_t engine_get_root_entities(uint64_t *out_entities, int32_t max_count);

// ============================================================================
// Mesh Renderer
// ============================================================================

typedef struct {
  uint64_t mesh_id;
  uint64_t material_id;
  bool cast_shadows;
  bool receive_shadows;
} MeshRendererData_C;

void engine_set_mesh_renderer(uint64_t entity_id,
                              const MeshRendererData_C *data);
void engine_get_mesh_renderer(uint64_t entity_id, MeshRendererData_C *data);

// ============================================================================
// Physics
// ============================================================================

typedef struct {
  float mass;
  float drag;
  float angular_drag;
  bool use_gravity;
  bool is_kinematic;
  float velocity[3];
  float angular_velocity[3];
} PhysicsData_C;

void engine_set_physics(uint64_t entity_id, const PhysicsData_C *data);
void engine_get_physics(uint64_t entity_id, PhysicsData_C *data);

// ============================================================================
// Scene Management
// ============================================================================

void engine_save_scene(const char *path);
void engine_load_scene(const char *path);
void engine_new_scene(void);

// ============================================================================
// Logging
// ============================================================================

void engine_log(int32_t level, const char *source, const char *message);

// ============================================================================
// Selection & Gizmos
// ============================================================================

void engine_set_selected_entity(uint64_t entity_id);
void engine_set_gizmo_mode(int32_t mode);
void engine_set_gizmo_space(int32_t space);

// ============================================================================
// Snapping
// ============================================================================

void engine_set_position_snap_enabled(bool enabled);
void engine_set_position_snap_increment(float increment);
void engine_set_rotation_snap_enabled(bool enabled);
void engine_set_rotation_snap_increment(float increment);
void engine_set_scale_snap_enabled(bool enabled);
void engine_set_scale_snap_increment(float increment);

// ============================================================================
// Camera
// ============================================================================

void engine_set_editor_camera_position(float x, float y, float z);
void engine_set_editor_camera_rotation(float pitch, float yaw);

// ============================================================================
// Profiler
// ============================================================================

void engine_profiler_begin_frame(void);
void engine_profiler_end_frame(void);

// ============================================================================
// Grid & Snapping
// ============================================================================

void engine_set_grid_enabled(bool enabled);
void engine_set_grid_spacing(float spacing);

// ============================================================================
// Mesh Overlay & Visualization
// ============================================================================

void engine_set_mesh_overlay_color(uint64_t entity_id, float r, float g,
                                   float b, float a);
void engine_set_mesh_wireframe_enabled(uint64_t entity_id, bool enabled);
void engine_set_mesh_vertex_colors_enabled(uint64_t entity_id, bool enabled);
void engine_set_mesh_uv_visualization(uint64_t entity_id, int32_t mode);
void engine_set_mesh_normals_visualization(uint64_t entity_id, bool enabled);
void engine_set_mesh_bounds_visualization(uint64_t entity_id, bool enabled);
void engine_set_material_override(uint64_t entity_id, uint64_t material_id);
void engine_clear_material_override(uint64_t entity_id);

void engine_set_selection_outline_color(float r, float g, float b);
void engine_set_selection_outline_width(float width);
void engine_set_hover_highlight_enabled(bool enabled);

void engine_set_render_mode(int32_t mode);
void engine_set_lighting_enabled(bool enabled);
void engine_set_shadows_enabled(bool enabled);
void engine_set_ambient_occlusion_enabled(bool enabled);

// ============================================================================
// Additional Functions for Frontend Bridge
// ============================================================================

void engine_new_scene(void);
void engine_load_scene(const char* path);
void engine_save_scene(const char* path);
void engine_add_component(uint64_t entity_id, ComponentData component);
void engine_remove_component(uint64_t entity_id, int component_type);
void engine_set_physics(uint64_t entity_id, PhysicsData* physics);
void engine_set_render_stats(RenderStats* stats);
void engine_log(const char* message, int level, const char* source);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_SWIFT_BRIDGE_H

// Swift Bridge Function Declarations
// These are the actual functions called from Swift

#ifdef __cplusplus
extern "C" {
#endif

// Note: These have different signatures from core engine functions
void swift_engine_init(void (*on_entity_created)(uint64_t),
                       void (*on_entity_deleted)(uint64_t),
                       void (*on_entity_modified)(uint64_t),
                       void (*on_log_message)(const char *, int32_t,
                                              const char *),
                       void (*on_scene_loaded)(const char *));
void swift_engine_shutdown(void);
void swift_engine_update(float delta_time);

#ifdef __cplusplus
}
#endif
