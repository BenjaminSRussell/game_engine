// engine_bridge.h - C header for Swift-C bridge
#ifndef ENGINE_BRIDGE_H
#define ENGINE_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// MARK: - Callback Types

typedef void (*EntityCreatedCallback)(uint64_t entity_id);
typedef void (*EntityDeletedCallback)(uint64_t entity_id);
typedef void (*EntityModifiedCallback)(uint64_t entity_id);
typedef void (*LogMessageCallback)(const char *message, int level,
                                   const char *source);
typedef void (*SceneLoadedCallback)(const char *scene_path);

typedef struct {
  EntityCreatedCallback on_entity_created;
  EntityDeletedCallback on_entity_deleted;
  EntityModifiedCallback on_entity_modified;
  LogMessageCallback on_log_message;
  SceneLoadedCallback on_scene_loaded;
} EngineCallbacks;

// MARK: - Initialization

void engine_init(EngineCallbacks callbacks);
void engine_shutdown(void);
void engine_update(float delta_time);

// MARK: - Entity Management

uint64_t engine_create_entity(const char *name);
void engine_delete_entity(uint64_t entity_id);
bool engine_entity_exists(uint64_t entity_id);
const char *engine_get_entity_name(uint64_t entity_id);
void engine_set_entity_name(uint64_t entity_id, const char *name);

// Get all entities (returns count, fills array)
uint32_t engine_get_all_entities(uint64_t *out_entities, uint32_t max_count);

// MARK: - Transform Component

typedef struct {
  float position[3]; // x, y, z
  float rotation[3]; // pitch, yaw, roll (degrees)
  float scale[3];    // x, y, z
} Transform;

void engine_set_transform(uint64_t entity_id, const Transform *transform);
void engine_get_transform(uint64_t entity_id, Transform *out_transform);

// MARK: - Component Management

typedef enum {
  COMPONENT_TRANSFORM = 0,
  COMPONENT_MESH_RENDERER = 1,
  COMPONENT_PHYSICS = 2,
  COMPONENT_CAMERA = 3,
  COMPONENT_LIGHT = 4,
  COMPONENT_AUDIO_SOURCE = 5,
  COMPONENT_SCRIPT = 6
} ComponentType;

bool engine_has_component(uint64_t entity_id, ComponentType type);
void engine_add_component(uint64_t entity_id, ComponentType type,
                          const void *data, uint32_t data_size);
void engine_remove_component(uint64_t entity_id, ComponentType type);
void *engine_get_component(uint64_t entity_id, ComponentType type);

// MARK: - Mesh Renderer Component

typedef struct {
  uint64_t mesh_id;
  uint64_t material_id;
  bool cast_shadows;
  bool receive_shadows;
} MeshRendererData;

void engine_set_mesh_renderer(uint64_t entity_id, const MeshRendererData *data);
void engine_get_mesh_renderer(uint64_t entity_id, MeshRendererData *out_data);

// MARK: - Physics Component

typedef struct {
  float mass;
  float drag;
  float angular_drag;
  bool use_gravity;
  bool is_kinematic;
  float velocity[3];
  float angular_velocity[3];
} PhysicsData;

void engine_set_physics(uint64_t entity_id, const PhysicsData *data);
void engine_get_physics(uint64_t entity_id, PhysicsData *out_data);

// MARK: - Scene Management

void engine_load_scene(const char *path);
void engine_save_scene(const char *path);
void engine_new_scene(void);
const char *engine_get_current_scene_path(void);

// MARK: - Camera

void engine_set_camera_position(float x, float y, float z);
void engine_set_camera_rotation(float pitch, float yaw, float roll);
void engine_set_camera_fov(float fov);
void engine_get_camera_position(float *out_position);
void engine_get_camera_rotation(float *out_rotation);

// MARK: - Rendering

void *engine_get_render_target(void);
void engine_set_viewport_size(uint32_t width, uint32_t height);

typedef struct {
  float fps;
  float frame_time_ms;
  uint32_t draw_calls;
  uint32_t triangles;
  uint32_t vertices;
  uint64_t memory_used;
} RenderStats;

void engine_get_render_stats(RenderStats *out_stats);

// MARK: - Asset Management

typedef enum {
  ASSET_TEXTURE = 0,
  ASSET_MODEL = 1,
  ASSET_MATERIAL = 2,
  ASSET_AUDIO = 3,
  ASSET_SCRIPT = 4,
  ASSET_SCENE = 5
} AssetType;

uint64_t engine_import_asset(const char *path, AssetType type);
void engine_delete_asset(uint64_t asset_id);
const char *engine_get_asset_path(uint64_t asset_id);
AssetType engine_get_asset_type(uint64_t asset_id);

// Get all assets of a type
uint32_t engine_get_assets_by_type(AssetType type, uint64_t *out_assets,
                                   uint32_t max_count);

// MARK: - Logging

typedef enum {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO = 1,
  LOG_LEVEL_WARNING = 2,
  LOG_LEVEL_ERROR = 3
} LogLevel;

void engine_log(LogLevel level, const char *source, const char *message);

// MARK: - Project Management

typedef struct {
  char name[256];
  char path[1024];
  uint32_t target_platform;
  uint32_t rendering_api;
} ProjectSettings;

void engine_load_project(const char *path);
void engine_save_project(void);
void engine_get_project_settings(ProjectSettings *out_settings);
void engine_set_project_settings(const ProjectSettings *settings);

// MARK: - Build System

typedef enum {
  BUILD_CONFIG_DEBUG = 0,
  BUILD_CONFIG_RELEASE = 1,
  BUILD_CONFIG_SHIPPING = 2
} BuildConfiguration;

typedef void (*BuildProgressCallback)(float progress, const char *status);

void engine_build_project(BuildConfiguration config,
                          BuildProgressCallback callback);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_BRIDGE_H
