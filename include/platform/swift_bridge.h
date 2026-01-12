// Swift Bridge Header
// Public API for Swift-C bridge layer

#ifndef SWIFT_BRIDGE_H
#define SWIFT_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Callback types
typedef void (*EntityCallback)(uint64_t entityID);
typedef void (*LogCallback)(const char *message, int32_t level,
                            const char *source);
typedef void (*SceneCallback)(const char *scenePath);

// Transform data
typedef struct {
  float x, y, z;
} Vec3_C;

typedef struct {
  Vec3_C position;
  Vec3_C rotation;
  Vec3_C scale;
} Transform_C;

// Component data
typedef struct {
  uint64_t mesh_id;
  uint64_t material_id;
  bool cast_shadows;
  bool receive_shadows;
} MeshRendererData_C;

typedef struct {
  float mass;
  float drag;
  float angular_drag;
  bool use_gravity;
  bool is_kinematic;
  Vec3_C velocity;
  Vec3_C angular_velocity;
} PhysicsData_C;

typedef struct {
  float fps;
  float frame_time_ms;
  uint32_t draw_calls;
  uint32_t triangles;
  uint32_t vertices;
  uint64_t memory_used;
} RenderStats_C;

// Initialization & Lifecycle
void swift_engine_init(EntityCallback on_entity_created,
                      EntityCallback on_entity_deleted,
                      EntityCallback on_entity_modified, LogCallback on_log_message,
                      SceneCallback on_scene_loaded);
void engine_shutdown(void);
void engine_update(float deltaTime);

// Entity Management
uint64_t engine_create_entity(const char *name);
void engine_delete_entity(uint64_t entityID);
bool engine_entity_exists(uint64_t entityID);
const char *engine_get_entity_name(uint64_t entityID);
void engine_set_entity_name(uint64_t entityID, const char *name);
uint32_t engine_get_all_entities(uint64_t *outEntities, uint32_t maxCount);

// Transform Component
void engine_set_transform(uint64_t entityID, const Transform_C *transform);
void engine_get_transform(uint64_t entityID, Transform_C *outTransform);

// Component Queries
bool engine_has_component(uint64_t entityID, int32_t componentType);

// Mesh Renderer Component
void engine_set_mesh_renderer(uint64_t entityID,
                              const MeshRendererData_C *data);
void engine_get_mesh_renderer(uint64_t entityID, MeshRendererData_C *outData);

// Physics Component
void engine_set_physics(uint64_t entityID, const PhysicsData_C *data);
void engine_get_physics(uint64_t entityID, PhysicsData_C *outData);

// Scene Management
void engine_load_scene(const char *path);
void engine_save_scene(const char *path);
void engine_new_scene(void);

// Render Stats
void engine_get_render_stats(RenderStats_C *outStats);

// Logging
void engine_log(int32_t level, const char *source, const char *message);

// Grid and Snapping Controls
void engine_set_grid_enabled(bool enabled);
void engine_set_grid_spacing(float spacing);
void engine_set_position_snap_enabled(bool enabled);
void engine_set_position_snap_increment(float increment);
void engine_set_rotation_snap_enabled(bool enabled);
void engine_set_rotation_snap_increment(float increment);
void engine_set_scale_snap_enabled(bool enabled);
void engine_set_scale_snap_increment(float increment);

// Enhanced Entity Functions
bool engine_get_entity_active(uint64_t entityID);
bool engine_get_entity_static(uint64_t entityID);
const char* engine_get_entity_tag(uint64_t entityID);
const char* engine_get_entity_layer(uint64_t entityID);
void engine_set_entity_active(uint64_t entityID, bool active);
void engine_set_entity_static(uint64_t entityID, bool isStatic);
void engine_set_entity_tag(uint64_t entityID, const char* tag);
void engine_set_entity_layer(uint64_t entityID, const char* layer);
int32_t engine_get_component_count(uint64_t entityID);
void engine_get_component_types(uint64_t entityID, int32_t* outTypes, int32_t maxCount);

// Component Management
void engine_add_component(uint64_t entityID, int32_t componentType);
void engine_remove_component(uint64_t entityID, int32_t componentType);
void engine_create_entity_with_id(uint64_t entityID, const char* name);

// Mesh Visualization
void engine_set_mesh_overlay_color(uint64_t entityID, float r, float g, float b, float a);
void engine_set_mesh_wireframe_enabled(uint64_t entityID, bool enabled);
void engine_set_mesh_vertex_colors_enabled(uint64_t entityID, bool enabled);
void engine_set_mesh_uv_visualization(uint64_t entityID, int32_t mode);
void engine_set_mesh_normals_visualization(uint64_t entityID, bool enabled);
void engine_set_mesh_bounds_visualization(uint64_t entityID, bool enabled);
void engine_set_material_override(uint64_t entityID, uint64_t materialID);
void engine_clear_material_override(uint64_t entityID);

// Selection and Highlighting
void engine_set_selection_outline_color(float r, float g, float b);
void engine_set_selection_outline_width(float width);
void engine_set_hover_highlight_enabled(bool enabled);

// Render Mode Functions
void engine_set_render_mode(int32_t mode);
void engine_set_lighting_enabled(bool enabled);
void engine_set_shadows_enabled(bool enabled);
void engine_set_post_processing_enabled(bool enabled);
void engine_set_vsync_enabled(bool enabled);
void engine_set_fps_limit(uint32_t limit);
void engine_set_ambient_occlusion_enabled(bool enabled);
void engine_set_debug_rendering_enabled(bool enabled);
void engine_set_wireframe_overlay_enabled(bool enabled);
void engine_set_bounding_boxes_enabled(bool enabled);
void engine_set_performance_profiling_enabled(bool enabled);

// Advanced Entity Management
uint64_t engine_find_entity_by_name(const char* name);
bool engine_entity_has_component(uint64_t entityID, int32_t componentType);
void engine_entity_get_transform(uint64_t entityID, float* position, float* rotation, float* scale);
void engine_entity_set_transform(uint64_t entityID, const float* position, const float* rotation, const float* scale);

// Scene Management
void engine_load_scene(const char* scenePath);
void engine_save_scene(const char* scenePath);
void engine_unload_current_scene(void);

#ifdef __cplusplus
}
#endif

#endif // SWIFT_BRIDGE_H
