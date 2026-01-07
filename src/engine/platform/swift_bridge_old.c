// Swift-C Bridge Implementation
// Connects Swift frontend UI to C engine backend
// All engine_* functions declared in EngineBridge.swift

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Engine headers
#include <core/logger.h>
#include <renderer/grid_renderer.h>

// Temporary stubs until we find correct headers
typedef uint64_t EntityID;

// Callback types
typedef void (*EntityCallback)(uint64_t entityID);
typedef void (*LogCallback)(const char *message, int32_t level,
                            const char *source);
typedef void (*SceneCallback)(const char *scenePath);

// Callback storage
static struct {
  EntityCallback on_entity_created;
  EntityCallback on_entity_deleted;
  EntityCallback on_entity_modified;
  LogCallback on_log_message;
  SceneCallback on_scene_loaded;
} g_callbacks = {0};

// Global engine state
static bool g_engine_initialized = false;

// =============================================================================
// INITIALIZATION & LIFECYCLE
// =============================================================================

void engine_init(EntityCallback on_entity_created,
                 EntityCallback on_entity_deleted,
                 EntityCallback on_entity_modified, LogCallback on_log_message,
                 SceneCallback on_scene_loaded) {
  if (g_engine_initialized) {
    printf("[Swift Bridge] Warning: Engine already initialized\n");
    return;
  }

  // Store callbacks
  g_callbacks.on_entity_created = on_entity_created;
  g_callbacks.on_entity_deleted = on_entity_deleted;
  g_callbacks.on_entity_modified = on_entity_modified;
  g_callbacks.on_log_message = on_log_message;
  g_callbacks.on_scene_loaded = on_scene_loaded;

  // Initialize subsystems
  grid_renderer_init();

  g_engine_initialized = true;

  if (on_log_message) {
    on_log_message("Swift bridge initialized", 2, "Bridge"); // 2 = Info
  }

  printf("[Swift Bridge] Initialized successfully\n");
}

void engine_shutdown(void) {
  if (!g_engine_initialized)
    return;

  grid_renderer_shutdown();

  memset(&g_callbacks, 0, sizeof(g_callbacks));
  g_engine_initialized = false;

  printf("[Swift Bridge] Shutdown complete\n");
}

void engine_update(float deltaTime) {
  if (!g_engine_initialized)
    return;

  // Update subsystems
  // TODO: Call actual engine update
  (void)deltaTime;
}

// =============================================================================
// ENTITY MANAGEMENT
// =============================================================================

uint64_t engine_create_entity(const char *name) {
  if (!g_engine_initialized)
    return 0;

  // TODO: Call actual ECS system
  static uint64_t next_id = 1;
  uint64_t entityID = next_id++;

  printf("[Swift Bridge] Created entity '%s' with ID %llu\n", name, entityID);

  if (g_callbacks.on_entity_created) {
    g_callbacks.on_entity_created(entityID);
  }

  return entityID;
}

void engine_delete_entity(uint64_t entityID) {
  if (!g_engine_initialized)
    return;

  // TODO: Call actual ECS system
  printf("[Swift Bridge] Deleted entity %llu\n", entityID);

  if (g_callbacks.on_entity_deleted) {
    g_callbacks.on_entity_deleted(entityID);
  }
}

bool engine_entity_exists(uint64_t entityID) {
  if (!g_engine_initialized)
    return false;

  // TODO: Call actual ECS system
  // For now, assume entities 1-100 exist
  return (entityID > 0 && entityID < 100);
}

const char *engine_get_entity_name(uint64_t entityID) {
  if (!g_engine_initialized)
    return NULL;

  // TODO: Call actual ECS system
  static char name_buffer[256];
  snprintf(name_buffer, sizeof(name_buffer), "Entity_%llu", entityID);
  return name_buffer;
}

void engine_set_entity_name(uint64_t entityID, const char *name) {
  if (!g_engine_initialized || !name)
    return;

  // TODO: Call actual ECS system
  printf("[Swift Bridge] Renamed entity %llu to '%s'\n", entityID, name);

  if (g_callbacks.on_entity_modified) {
    g_callbacks.on_entity_modified(entityID);
  }
}

uint32_t engine_get_all_entities(uint64_t *outEntities, uint32_t maxCount) {
  if (!g_engine_initialized || !outEntities)
    return 0;

  // TODO: Call actual ECS system
  // Return some demo entities for now
  uint32_t count = (maxCount < 5) ? maxCount : 5;
  for (uint32_t i = 0; i < count; i++) {
    outEntities[i] = i + 1;
  }

  return count;
}

// =============================================================================
// TRANSFORM COMPONENT
// =============================================================================

typedef struct {
  float x, y, z;
} Vec3_C;

typedef struct {
  Vec3_C position;
  Vec3_C rotation;
  Vec3_C scale;
} Transform_C;

void engine_set_transform(uint64_t entityID, const Transform_C *transform) {
  if (!g_engine_initialized || !transform)
    return;

  // Apply snapping before setting
  float pos[3] = {transform->position.x, transform->position.y,
                  transform->position.z};
  float rot[3] = {transform->rotation.x, transform->rotation.y,
                  transform->rotation.z};
  float scl[3] = {transform->scale.x, transform->scale.y, transform->scale.z};

  grid_renderer_apply_position_snap(pos);
  grid_renderer_apply_rotation_snap(rot);
  grid_renderer_apply_scale_snap(scl);

  // TODO: Call actual ECS transform system with snapped values
  printf("[Swift Bridge] Set transform for entity %llu: pos(%.2f,%.2f,%.2f)\n",
         entityID, pos[0], pos[1], pos[2]);

  if (g_callbacks.on_entity_modified) {
    g_callbacks.on_entity_modified(entityID);
  }
}

void engine_get_transform(uint64_t entityID, Transform_C *outTransform) {
  if (!g_engine_initialized || !outTransform)
    return;

  // TODO: Call actual ECS transform system
  // Return demo transform
  outTransform->position = (Vec3_C){0.0f, 0.0f, 0.0f};
  outTransform->rotation = (Vec3_C){0.0f, 0.0f, 0.0f};
  outTransform->scale = (Vec3_C){1.0f, 1.0f, 1.0f};
}

// =============================================================================
// COMPONENT QUERIES
// =============================================================================

bool engine_has_component(uint64_t entityID, int32_t componentType) {
  if (!g_engine_initialized)
    return false;

  // TODO: Call actual ECS component system
  // For now, assume all entities have transform (type 0)
  return (componentType == 0);
}

// =============================================================================
// MESH RENDERER COMPONENT
// =============================================================================

typedef struct {
  uint64_t mesh_id;
  uint64_t material_id;
  bool cast_shadows;
  bool receive_shadows;
} MeshRendererData_C;

void engine_set_mesh_renderer(uint64_t entityID,
                              const MeshRendererData_C *data) {
  if (!g_engine_initialized || !data)
    return;

  // TODO: Call actual renderer system
  printf("[Swift Bridge] Set mesh renderer for entity %llu\n", entityID);

  if (g_callbacks.on_entity_modified) {
    g_callbacks.on_entity_modified(entityID);
  }
}

void engine_get_mesh_renderer(uint64_t entityID, MeshRendererData_C *outData) {
  if (!g_engine_initialized || !outData)
    return;

  // TODO: Call actual renderer system
  outData->mesh_id = 0;
  outData->material_id = 0;
  outData->cast_shadows = true;
  outData->receive_shadows = true;
}

// =============================================================================
// PHYSICS COMPONENT
// =============================================================================

typedef struct {
  float mass;
  float drag;
  float angular_drag;
  bool use_gravity;
  bool is_kinematic;
  Vec3_C velocity;
  Vec3_C angular_velocity;
} PhysicsData_C;

void engine_set_physics(uint64_t entityID, const PhysicsData_C *data) {
  if (!g_engine_initialized || !data)
    return;

  // TODO: Call actual physics system
  // Integrate with physics engine
  printf(
      "[Swift Bridge] Set physics for entity %llu: mass=%.2f, use_gravity=%d\n",
      entityID, data->mass, data->use_gravity);

  if (g_callbacks.on_entity_modified) {
    g_callbacks.on_entity_modified(entityID);
  }
}

void engine_get_physics(uint64_t entityID, PhysicsData_C *outData) {
  if (!g_engine_initialized || !outData)
    return;

  // TODO: Call actual physics system
  outData->mass = 1.0f;
  outData->drag = 0.0f;
  outData->angular_drag = 0.05f;
  outData->use_gravity = true;
  outData->is_kinematic = false;
  outData->velocity = (Vec3_C){0.0f, 0.0f, 0.0f};
  outData->angular_velocity = (Vec3_C){0.0f, 0.0f, 0.0f};
}

// =============================================================================
// SCENE MANAGEMENT
// =============================================================================

void engine_load_scene(const char *path) {
  if (!g_engine_initialized || !path)
    return;

  //  TODO: Call actual scene loader
  printf("[Swift Bridge] Loading scene: %s\n", path);

  if (g_callbacks.on_scene_loaded) {
    g_callbacks.on_scene_loaded(path);
  }
}

void engine_save_scene(const char *path) {
  if (!g_engine_initialized || !path)
    return;

  // TODO: Call actual scene saver
  printf("[Swift Bridge] Saving scene: %s\n", path);
}

void engine_new_scene(void) {
  if (!g_engine_initialized)
    return;

  // TODO: Clear all entities and reset scene
  printf("[Swift Bridge] Creating new scene\n");

  if (g_callbacks.on_scene_loaded) {
    g_callbacks.on_scene_loaded("Untitled Scene");
  }
}

// =============================================================================
// RENDER STATS
// =============================================================================

typedef struct {
  float fps;
  float frame_time_ms;
  uint32_t draw_calls;
  uint32_t triangles;
  uint32_t vertices;
  uint64_t memory_used;
} RenderStats_C;

void engine_get_render_stats(RenderStats_C *outStats) {
  if (!g_engine_initialized || !outStats)
    return;

  // TODO: Get actual render stats
  outStats->fps = 60.0f;
  outStats->frame_time_ms = 16.67f;
  outStats->draw_calls = 124;
  outStats->triangles = 45200;
  outStats->vertices = 22600;
  outStats->memory_used = 256 * 1024 * 1024; // 256 MB
}

// =============================================================================
// LOGGING
// =============================================================================

void engine_log(int32_t level, const char *source, const char *message) {
  if (!g_engine_initialized)
    return;

  const char *level_str[] = {"ERROR", "WARN", "INFO", "DEBUG"};
  const char *level_name =
      (level >= 0 && level < 4) ? level_str[level] : "UNKNOWN";

  printf("[%s][%s] %s\n", source ? source : "Engine", level_name,
         message ? message : "");

  if (g_callbacks.on_log_message) {
    g_callbacks.on_log_message(message, level, source);
  }
}

// =============================================================================
// GRID AND SNAPPING CONTROLS
// =============================================================================

void engine_set_grid_enabled(bool enabled) {
  if (!g_engine_initialized)
    return;
  grid_renderer_toggle_grid();
}

void engine_set_grid_spacing(float spacing) {
  if (!g_engine_initialized)
    return;
  grid_renderer_set_grid_spacing(spacing);
}

void engine_set_position_snap_enabled(bool enabled) {
  if (!g_engine_initialized)
    return;
  grid_renderer_toggle_position_snap();
}

void engine_set_position_snap_increment(float increment) {
  if (!g_engine_initialized)
    return;

  GridSettings settings;
  grid_renderer_get_settings(&settings);
  settings.snap_position_increment = increment;
  grid_renderer_set_settings(&settings);
}

void engine_set_rotation_snap_enabled(bool enabled) {
  if (!g_engine_initialized)
    return;
  grid_renderer_toggle_rotation_snap();
}

void engine_set_rotation_snap_increment(float increment) {
  if (!g_engine_initialized)
    return;

  GridSettings settings;
  grid_renderer_get_settings(&settings);
  settings.snap_rotation_increment = increment;
  grid_renderer_set_settings(&settings);
}

void engine_set_scale_snap_enabled(bool enabled) {
  if (!g_engine_initialized)
    return;
  grid_renderer_toggle_scale_snap();
}

void engine_set_scale_snap_increment(float increment) {
  if (!g_engine_initialized)
    return;

  GridSettings settings;
  grid_renderer_get_settings(&settings);
  settings.snap_scale_increment = increment;
  grid_renderer_set_settings(&settings);
}
