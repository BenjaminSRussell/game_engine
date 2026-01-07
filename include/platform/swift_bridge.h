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
void engine_init(EntityCallback on_entity_created,
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

#ifdef __cplusplus
}
#endif

#endif // SWIFT_BRIDGE_H
