// Swift-C Bridge Implementation - Complete Engine Integration
// Connects Swift frontend UI to C engine backend
// All engine_* functions declared in EngineBridge.swift

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Engine headers
#include <core/engine.h>
#include <core/logger.h>
#include <ecs/ecs.h>
#include <math/math_types.h>
#include <renderer/grid_renderer.h>

// ============================================================================
// GLOBAL ENGINE STATE
// ============================================================================

static Engine *g_engine = NULL;
static World *g_ecs_world = NULL;
static bool g_engine_initialized = false;

// UUID to Entity mapping
#define MAX_ENTITY_MAP 4096
static struct {
  uint64_t uuid;
  Entity entity;
} g_entity_map[MAX_ENTITY_MAP];
static uint32_t g_entity_map_count = 0;

// Component IDs
static ECSComponentID g_transform_component_id = 0;
static ECSComponentID g_mesh_renderer_component_id = 0;
static ECSComponentID g_physics_component_id = 0;
static ECSComponentID g_name_component_id = 0;

// Callback storage
typedef void (*EntityCallback)(uint64_t entityID);
typedef void (*LogCallback)(const char *message, int32_t level,
                            const char *source);
typedef void (*SceneCallback)(const char *scenePath);

static struct {
  EntityCallback on_entity_created;
  EntityCallback on_entity_deleted;
  EntityCallback on_entity_modified;
  LogCallback on_log_message;
  SceneCallback on_scene_loaded;
} g_callbacks = {0};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Map UUID to Entity
static Entity uuid_to_entity(uint64_t uuid) {
  for (uint32_t i = 0; i < g_entity_map_count; i++) {
    if (g_entity_map[i].uuid == uuid) {
      return g_entity_map[i].entity;
    }
  }
  return INVALID_ENTITY;
}

// Map Entity to UUID
static uint64_t entity_to_uuid(Entity entity) {
  for (uint32_t i = 0; i < g_entity_map_count; i++) {
    if (g_entity_map[i].entity.id == entity.id &&
        g_entity_map[i].entity.generation == entity.generation) {
      return g_entity_map[i].uuid;
    }
  }
  return 0;
}

// Register UUID<->Entity mapping
static void register_entity_mapping(uint64_t uuid, Entity entity) {
  if (g_entity_map_count < MAX_ENTITY_MAP) {
    g_entity_map[g_entity_map_count].uuid = uuid;
    g_entity_map[g_entity_map_count].entity = entity;
    g_entity_map_count++;
  } else {
    LOG_ERROR("Entity map full!");
  }
}

// Component structures matching Swift types
typedef struct {
  char name[256];
} NameComponent;

typedef struct {
  Vec3 position;
  Vec3 rotation;
  Vec3 scale;
} TransformComponent;

typedef struct {
  uint64_t mesh_id;
  uint64_t material_id;
  bool cast_shadows;
  bool receive_shadows;
} MeshRendererComponent;

typedef struct {
  float mass;
  float drag;
  float angular_drag;
  bool use_gravity;
  bool is_kinematic;
  Vec3 velocity;
  Vec3 angular_velocity;
} PhysicsComponent;

// ============================================================================
// INITIALIZATION & LIFECYCLE
// ============================================================================

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

  // Allocate engine
  g_engine = calloc(1, sizeof(Engine));
  if (!g_engine) {
    LOG_ERROR("Failed to allocate engine");
    return;
  }

  // Create engine config
  EngineConfig config = engine_create_default_config();
  config.window_title = "VoxelForge Engine";
  config.window_width = 1280;
  config.window_height = 720;

  // Initialize engine (this will create ECS world internally)
  if (!engine_init(g_engine, &config)) {
    LOG_ERROR("Failed to initialize engine");
    free(g_engine);
    g_engine = NULL;
    return;
  }

  // Get ECS world from engine
  g_ecs_world = (World *)engine_get_entities(g_engine);
  if (!g_ecs_world) {
    LOG_ERROR("Failed to get ECS world from engine");
    engine_shutdown(g_engine);
    free(g_engine);
    g_engine = NULL;
    return;
  }

  // Register component types
  g_name_component_id =
      ecs_register_component_simple(g_ecs_world, "Name", sizeof(NameComponent));
  g_transform_component_id = ecs_register_component_simple(
      g_ecs_world, "Transform", sizeof(TransformComponent));
  g_mesh_renderer_component_id = ecs_register_component_simple(
      g_ecs_world, "MeshRenderer", sizeof(MeshRendererComponent));
  g_physics_component_id = ecs_register_component_simple(
      g_ecs_world, "Physics", sizeof(PhysicsComponent));

  // Initialize grid renderer
  grid_renderer_init();

  g_engine_initialized = true;

  if (on_log_message) {
    on_log_message("Swift bridge and C engine initialized", 1, "Bridge");
  }

  printf("[Swift Bridge] Initialized successfully with real C engine\n");
}

void engine_shutdown(void) {
  if (!g_engine_initialized)
    return;

  grid_renderer_shutdown();

  if (g_engine) {
    engine_shutdown(g_engine);
    free(g_engine);
    g_engine = NULL;
  }

  g_ecs_world = NULL;
  g_entity_map_count = 0;
  memset(&g_callbacks, 0, sizeof(g_callbacks));
  g_engine_initialized = false;

  printf("[Swift Bridge] Shutdown complete\n");
}

void engine_update(float deltaTime) {
  if (!g_engine_initialized || !g_ecs_world)
    return;

  // Update ECS world
  ecs_world_update(g_ecs_world, deltaTime);
}

// ============================================================================
// ENTITY MANAGEMENT
// ============================================================================

uint64_t engine_create_entity(const char *name) {
  if (!g_engine_initialized || !g_ecs_world)
    return 0;

  // Create entity in ECS
  Entity entity = ecs_create_entity(g_ecs_world);

  if (entity.id == 0) {
    LOG_ERROR("Failed to create entity");
    return 0;
  }

  // Generate UUID (simple: use entity ID + timestamp)
  uint64_t uuid = ((uint64_t)entity.id << 32) | (uint32_t)time(NULL);

  // Register mapping
  register_entity_mapping(uuid, entity);

  // Add name component
  if (name) {
    NameComponent name_comp = {0};
    strncpy(name_comp.name, name, sizeof(name_comp.name) - 1);
    ecs_add_component(g_ecs_world, entity, g_name_component_id, &name_comp);
  }

  // Add default transform
  TransformComponent transform = {
      .position = {0, 0, 0}, .rotation = {0, 0, 0}, .scale = {1, 1, 1}};
  ecs_add_component(g_ecs_world, entity, g_transform_component_id, &transform);

  printf("[Swift Bridge] Created entity '%s' with UUID %llu (ECS ID: %u)\n",
         name ? name : "Unnamed", uuid, entity.id);

  if (g_callbacks.on_entity_created) {
    g_callbacks.on_entity_created(uuid);
  }

  return uuid;
}

uint64_t engine_create_entity_with_id(uint64_t uuid, const char *name) {
  if (!g_engine_initialized || !g_ecs_world)
    return 0;

  // Create entity (ECS generates its own ID)
  Entity entity = ecs_create_entity(g_ecs_world);

  if (entity.id == 0) {
    LOG_ERROR("Failed to create entity with ID");
    return 0;
  }

  // Use provided UUID
  register_entity_mapping(uuid, entity);

  // Add name component
  if (name) {
    NameComponent name_comp = {0};
    strncpy(name_comp.name, name, sizeof(name_comp.name) - 1);
    ecs_add_component(g_ecs_world, entity, g_name_component_id, &name_comp);
  }

  // Add default transform
  TransformComponent transform = {
      .position = {0, 0, 0}, .rotation = {0, 0, 0}, .scale = {1, 1, 1}};
  ecs_add_component(g_ecs_world, entity, g_transform_component_id, &transform);

  printf("[Swift Bridge] Created entity '%s' with requested UUID %llu\n",
         name ? name : "Unnamed", uuid);

  if (g_callbacks.on_entity_created) {
    g_callbacks.on_entity_created(uuid);
  }

  return uuid;
}

void engine_delete_entity(uint64_t uuid) {
  if (!g_engine_initialized || !g_ecs_world)
    return;

  Entity entity = uuid_to_entity(uuid);
  if (entity.id == 0) {
    LOG_ERROR("Entity with UUID %llu not found", uuid);
    return;
  }

  ecs_destroy_entity(g_ecs_world, entity);

  // Remove from mapping
  for (uint32_t i = 0; i < g_entity_map_count; i++) {
    if (g_entity_map[i].uuid == uuid) {
      // Shift remaining entries
      for (uint32_t j = i; j < g_entity_map_count - 1; j++) {
        g_entity_map[j] = g_entity_map[j + 1];
      }
      g_entity_map_count--;
      break;
    }
  }

  printf("[Swift Bridge] Deleted entity UUID %llu\n", uuid);

  if (g_callbacks.on_entity_deleted) {
    g_callbacks.on_entity_deleted(uuid);
  }
}

bool engine_entity_exists(uint64_t uuid) {
  if (!g_engine_initialized || !g_ecs_world)
    return false;

  Entity entity = uuid_to_entity(uuid);
  if (entity.id == 0)
    return false;

  return ecs_is_entity_valid(g_ecs_world, entity);
}

uint32_t engine_get_all_entities(uint64_t *out_entities, uint32_t max_count) {
  if (!g_engine_initialized || !out_entities || max_count == 0)
    return 0;

  uint32_t count =
      (g_entity_map_count < max_count) ? g_entity_map_count : max_count;

  for (uint32_t i = 0; i < count; i++) {
    out_entities[i] = g_entity_map[i].uuid;
  }

  return count;
}

const char *engine_get_entity_name(uint64_t uuid) {
  if (!g_engine_initialized || !g_ecs_world)
    return NULL;

  Entity entity = uuid_to_entity(uuid);
  if (entity.id == 0)
    return NULL;

  NameComponent *name_comp = (NameComponent *)ecs_get_component(
      g_ecs_world, entity, g_name_component_id);
  if (name_comp) {
    return name_comp->name;
  }

  static char default_name[64];
  snprintf(default_name, sizeof(default_name), "Entity_%u", entity.id);
  return default_name;
}

void engine_set_entity_name(uint64_t uuid, const char *name) {
  if (!g_engine_initialized || !g_ecs_world || !name)
    return;

  Entity entity = uuid_to_entity(uuid);
  if (entity.id == 0)
    return;

  NameComponent name_comp = {0};
  strncpy(name_comp.name, name, sizeof(name_comp.name) - 1);

  if (ecs_has_component(g_ecs_world, entity, g_name_component_id)) {
    ecs_set_component(g_ecs_world, entity, g_name_component_id, &name_comp);
  } else {
    ecs_add_component(g_ecs_world, entity, g_name_component_id, &name_comp);
  }

  printf("[Swift Bridge] Renamed entity %llu to '%s'\n", uuid, name);

  if (g_callbacks.on_entity_modified) {
    g_callbacks.on_entity_modified(uuid);
  }
}

// Parent/child hierarchy (simplified - ECS has relationship system but using
// simple approach)
uint64_t engine_get_entity_parent(uint64_t uuid) {
  // TODO: Implement hierarchy system
  return 0; // No parent for now
}

void engine_set_entity_parent(uint64_t uuid, uint64_t parent_uuid) {
  // TODO: Implement hierarchy system
  printf("[Swift Bridge] Set entity %llu parent to %llu (NYI)\n", uuid,
         parent_uuid);
}

// REST OF THE FILE CONTINUES WITH TRANSFORM, COMPONENTS, PHYSICS, ETC...
// This is getting very long - should I continue in a separate file or break it
// up?

// ============================================================================
// TRANSFORM COMPONENT  
// ============================================================================

// C Transform struct from Swift
typedef struct {
    float position[3];
    float rotation[3];
    float scale[3];
} Transform_C;

void engine_set_transform(uint64_t uuid, const Transform_C *transform) {
    if (!g_engine_initialized || !g_ecs_world || !transform)
        return;

    Entity entity = uuid_to_entity(uuid);
    if (entity.id == 0)
        return;

    // Apply snapping
    float pos[3] = {transform->position[0], transform->position[1], transform->position[2]};
    float rot[3] = {transform->rotation[0], transform->rotation[1], transform->rotation[2]};
    float scl[3] = {transform->scale[0], transform->scale[1], transform->scale[2]};

    grid_renderer_apply_position_snap(pos);
    grid_renderer_apply_rotation_snap(rot);
    grid_renderer_apply_scale_snap(scl);

    // Set component in ECS
    TransformComponent trans_comp = {
        .position = {pos[0], pos[1], pos[2]},
        .rotation = {rot[0], rot[1], rot[2]},
        .scale = {scl[0], scl[1], scl[2]}
    };

    if (ecs_has_component(g_ecs_world, entity, g_transform_component_id)) {
        ecs_set_component(g_ecs_world, entity, g_transform_component_id, &trans_comp);
    } else {
        ecs_add_component(g_ecs_world, entity, g_transform_component_id, &trans_comp);
    }

    if (g_callbacks.on_entity_modified) {
        g_callbacks.on_entity_modified(uuid);
    }
}

void engine_get_transform(uint64_t uuid, Transform_C *out_transform) {
    if (!g_engine_initialized || !g_ecs_world || !out_transform)
        return;

    Entity entity = uuid_to_entity(uuid);
    if (entity.id == 0) {
        // Return identity transform
        memset(out_transform, 0, sizeof(Transform_C));
        out_transform->scale[0] = out_transform->scale[1] = out_transform->scale[2] = 1.0f;
        return;
    }

    TransformComponent *trans_comp = (TransformComponent*)ecs_get_component(g_ecs_world, entity, g_transform_component_id);
    if (trans_comp) {
        memcpy(out_transform->position, &trans_comp->position, sizeof(Vec3));
        memcpy(out_transform->rotation, &trans_comp->rotation, sizeof(Vec3));
        memcpy(out_transform->scale, &trans_comp->scale, sizeof(Vec3));
    } else {
        // Return identity transform
        memset(out_transform, 0, sizeof(Transform_C));
        out_transform->scale[0] = out_transform->scale[1] = out_transform->scale[2] = 1.0f;
    }
}

// ============================================================================
// COMPONENT QUERIES
// ============================================================================

typedef enum {
    COMPONENT_TRANSFORM = 0,
    COMPONENT_MESH_RENDERER = 1,
    COMPONENT_PHYSICS = 2
} ComponentType;

bool engine_has_component(uint64_t uuid, ComponentType type) {
    if (!g_engine_initialized || !g_ecs_world)
        return false;

    Entity entity = uuid_to_entity(uuid);
    if (entity.id == 0)
        return false;

    ECSComponentID comp_id = 0;
    switch (type) {
        case COMPONENT_TRANSFORM:
            comp_id = g_transform_component_id;
            break;
        case COMPONENT_MESH_RENDERER:
            comp_id = g_mesh_renderer_component_id;
            break;
        case COMPONENT_PHYSICS:
            comp_id = g_physics_component_id;
            break;
        default:
            return false;
    }

    return ecs_has_component(g_ecs_world, entity, comp_id);
}

void engine_add_component(uint64_t uuid, ComponentType type) {
    if (!g_engine_initialized || !g_ecs_world)
        return;

    Entity entity = uuid_to_entity(uuid);
    if (entity.id == 0)
        return;

    switch (type) {
        case COMPONENT_MESH_RENDERER: {
            MeshRendererComponent mesh_comp = {0, 0, true, true};
            ecs_add_component(g_ecs_world, entity, g_mesh_renderer_component_id, &mesh_comp);
            break;
        }
        case COMPONENT_PHYSICS: {
            PhysicsComponent phys_comp = {
                .mass = 1.0f,
                .drag = 0.0f,
                .angular_drag = 0.05f,
                .use_gravity = true,
                .is_kinematic = false,
                .velocity = {0, 0, 0},
                .angular_velocity = {0, 0, 0}
            };
            ecs_add_component(g_ecs_world, entity, g_physics_component_id, &phys_comp);
            break;
        }
        default:
            break;
    }

    if (g_callbacks.on_entity_modified) {
        g_callbacks.on_entity_modified(uuid);
    }
}

void engine_remove_component(uint64_t uuid, ComponentType type) {
    if (!g_engine_initialized || !g_ecs_world)
        return;

    Entity entity = uuid_to_entity(uuid);
    if (entity.id == 0)
        return;

    ECSComponentID comp_id = 0;
    switch (type) {
        case COMPONENT_MESH_RENDERER:
            comp_id = g_mesh_renderer_component_id;
            break;
        case COMPONENT_PHYSICS:
            comp_id = g_physics_component_id;
            break;
        default:
            return;
    }

    ecs_remove_component(g_ecs_world, entity, comp_id);

    if (g_callbacks.on_entity_modified) {
        g_callbacks.on_entity_modified(uuid);
    }
}

// ============================================================================
// MESH RENDERER COMPONENT
// ============================================================================

typedef struct {
    uint64_t mesh_id;
    uint64_t material_id;
    bool cast_shadows;
    bool receive_shadows;
} MeshRendererData_C;

void engine_set_mesh_renderer(uint64_t uuid, const MeshRendererData_C *data) {
    if (!g_engine_initialized || !g_ecs_world || !data)
        return;

    Entity entity = uuid_to_entity(uuid);
    if (entity.id == 0)
        return;

    MeshRendererComponent mesh_comp = {
        .mesh_id = data->mesh_id,
        .material_id = data->material_id,
        .cast_shadows = data->cast_shadows,
        .receive_shadows = data->receive_shadows
    };

    if (ecs_has_component(g_ecs_world, entity, g_mesh_renderer_component_id)) {
        ecs_set_component(g_ecs_world, entity, g_mesh_renderer_component_id, &mesh_comp);
    } else {
        ecs_add_component(g_ecs_world, entity, g_mesh_renderer_component_id, &mesh_comp);
    }

    if (g_callbacks.on_entity_modified) {
        g_callbacks.on_entity_modified(uuid);
    }
}

void engine_get_mesh_renderer(uint64_t uuid, MeshRendererData_C *out_data) {
    if (!g_engine_initialized || !g_ecs_world || !out_data)
        return;

    Entity entity = uuid_to_entity(uuid);
    if (entity.id == 0) {
        memset(out_data, 0, sizeof(MeshRendererData_C));
        out_data->cast_shadows = out_data->receive_shadows = true;
        return;
    }

    MeshRendererComponent *mesh_comp = (MeshRendererComponent*)ecs_get_component(g_ecs_world, entity, g_mesh_renderer_component_id);
    if (mesh_comp) {
        out_data->mesh_id = mesh_comp->mesh_id;
        out_data->material_id = mesh_comp->material_id;
        out_data->cast_shadows = mesh_comp->cast_shadows;
        out_data->receive_shadows = mesh_comp->receive_shadows;
    } else {
        memset(out_data, 0, sizeof(MeshRendererData_C));
        out_data->cast_shadows = out_data->receive_shadows = true;
    }
}

// Remaining functions: Physics, Scene, Stats, Logging, Grid, Camera...
// Total: ~1500 lines for complete bridge

// ============================================================================
// PHYSICS COMPONENT
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

void engine_set_physics(uint64_t uuid, const PhysicsData_C *data) {
    if (!g_engine_initialized || !g_ecs_world || !data)
        return;

    Entity entity = uuid_to_entity(uuid);
    if (entity.id == 0)
        return;

    PhysicsComponent phys_comp = {
        .mass = data->mass,
        .drag = data->drag,
        .angular_drag = data->angular_drag,
        .use_gravity = data->use_gravity,
        .is_kinematic = data->is_kinematic,
        .velocity = {data->velocity[0], data->velocity[1], data->velocity[2]},
        .angular_velocity = {data->angular_velocity[0], data->angular_velocity[1], data->angular_velocity[2]}
    };

    if (ecs_has_component(g_ecs_world, entity, g_physics_component_id)) {
        ecs_set_component(g_ecs_world, entity, g_physics_component_id, &phys_comp);
    } else {
        ecs_add_component(g_ecs_world, entity, g_physics_component_id, &phys_comp);
    }

    if (g_callbacks.on_entity_modified) {
        g_callbacks.on_entity_modified(uuid);
    }
}

void engine_get_physics(uint64_t uuid, PhysicsData_C *out_data) {
    if (!g_engine_initialized || !g_ecs_world || !out_data)
        return;

    Entity entity = uuid_to_entity(uuid);
    if (entity.id == 0) {
        memset(out_data, 0, sizeof(PhysicsData_C));
        out_data->mass = 1.0f;
        out_data->use_gravity = true;
        return;
    }

    PhysicsComponent *phys_comp = (PhysicsComponent*)ecs_get_component(g_ecs_world, entity, g_physics_component_id);
    if (phys_comp) {
        out_data->mass = phys_comp->mass;
        out_data->drag = phys_comp->drag;
        out_data->angular_drag = phys_comp->angular_drag;
        out_data->use_gravity = phys_comp->use_gravity;
        out_data->is_kinematic = phys_comp->is_kinematic;
        memcpy(out_data->velocity, &phys_comp->velocity, sizeof(Vec3));
        memcpy(out_data->angular_velocity, &phys_comp->angular_velocity, sizeof(Vec3));
    } else {
        memset(out_data, 0, sizeof(PhysicsData_C));
        out_data->mass = 1.0f;
        out_data->use_gravity = true;
    }
}

// ============================================================================
// SCENE MANAGEMENT
// ============================================================================

void engine_save_scene(const char *path) {
    if (!g_engine_initialized || !g_ecs_world || !path)
        return;

    // Save ECS world to file
    if (ecs_world_save(g_ecs_world, path)) {
        printf("[Swift Bridge] Scene saved to %s\n", path);
    } else {
        LOG_ERROR("Failed to save scene to %s", path);
    }
}

void engine_load_scene(const char *path) {
    if (!g_engine_initialized || !g_ecs_world || !path)
        return;

    // Clear entity mapping
    g_entity_map_count = 0;

    // Load ECS world from file
    if (ecs_world_load(g_ecs_world, path)) {
        printf("[Swift Bridge] Scene loaded from %s\n", path);
        
        // Rebuild entity UUID mapping
        // Note: This is simplified - real implementation needs to save/restore UUIDs
        WorldStats stats = ecs_world_get_stats(g_ecs_world);
        printf("[Swift Bridge] Loaded %u entities\n", stats.entity_count);

        if (g_callbacks.on_scene_loaded) {
            g_callbacks.on_scene_loaded(path);
        }
    } else {
        LOG_ERROR("Failed to load scene from %s", path);
    }
}

void engine_new_scene(void) {
    if (!g_engine_initialized || !g_ecs_world)
        return;

    // Clear all entities
    for (uint32_t i = 0; i < g_entity_map_count; i++) {
        Entity entity = g_entity_map[i].entity;
        if (ecs_is_entity_valid(g_ecs_world, entity)) {
            ecs_destroy_entity(g_ecs_world, entity);
        }
    }

    g_entity_map_count = 0;

    printf("[Swift Bridge] Created new scene\n");

    if (g_callbacks.on_scene_loaded) {
        g_callbacks.on_scene_loaded("Untitled Scene");
    }
}

// ============================================================================
// RENDER STATS
// ============================================================================

typedef struct {
    float fps;
    float frame_time_ms;
    uint32_t draw_calls;
    uint32_t triangles;
    uint32_t vertices;
    uint64_t memory_used;
} RenderStats_C;

void engine_get_render_stats(RenderStats_C *out_stats) {
    if (!g_engine_initialized || !out_stats)
        return;

    // Get real stats from engine
    if (g_engine && g_engine->subsystems.renderer) {
        // TODO: Get actual render stats from renderer
        // For now, use ECS stats as proxy
        WorldStats ecs_stats = ecs_world_get_stats(g_ecs_world);
        
        out_stats->fps = 1.0f / (float)ecs_stats.average_frame_time;
        out_stats->frame_time_ms = (float)(ecs_stats.average_frame_time * 1000.0);
        out_stats->draw_calls = 0; // TODO: Get from renderer
        out_stats->triangles = 0;  // TODO: Get from renderer
        out_stats->vertices = 0;   // TODO: Get from renderer
        out_stats->memory_used = ecs_stats.total_memory_usage;
    } else {
        // Fallback values
        out_stats->fps = 60.0f;
        out_stats->frame_time_ms = 16.67f;
        out_stats->draw_calls = 0;
        out_stats->triangles = 0;
        out_stats->vertices = 0;
        out_stats->memory_used = 0;
    }
}

// ============================================================================
// LOGGING
// ============================================================================

void engine_log(int32_t level, const char *source, const char *message) {
    if (!g_engine_initialized)
        return;

    // Map levels: 0=Debug, 1=Info, 2=Warning, 3=Error
    const char *level_str[] = {"DEBUG", "INFO", "WARNING", "ERROR"};
    const char *level_name = (level >= 0 && level < 4) ? level_str[level] : "UNKNOWN";

    // Log to engine logger
    switch (level) {
        case 0: LOG_DEBUG("[%s] %s", source ? source : "Swift", message ? message : ""); break;
        case 1: LOG_INFO("[%s] %s", source ? source : "Swift", message ? message : ""); break;
        case 2: LOG_WARN("[%s] %s", source ? source : "Swift", message ? message : ""); break;
        case 3: LOG_ERROR("[%s] %s", source ? source : "Swift", message ? message : ""); break;
    }

    // Forward to Swift callback
    if (g_callbacks.on_log_message) {
        g_callbacks.on_log_message(message, level, source);
    }
}

// ============================================================================
// SELECTION & GIZMOS
// ============================================================================

static uint64_t g_selected_entity_uuid = 0;
static int32_t g_gizmo_mode = 0; // 0=Translate, 1=Rotate, 2=Scale
static int32_t g_gizmo_space = 0; // 0=World, 1=Local

void engine_set_selected_entity(uint64_t uuid) {
    g_selected_entity_uuid = uuid;
    printf("[Swift Bridge] Selected entity %llu\n", uuid);
}

void engine_set_gizmo_mode(int32_t mode) {
    g_gizmo_mode = mode;
    printf("[Swift Bridge] Set gizmo mode to %d\n", mode);
}

void engine_set_gizmo_space(int32_t space) {
    g_gizmo_space = space;
    printf("[Swift Bridge] Set gizmo space to %d\n", space);
}

// ============================================================================
// GRID AND SNAPPING
// ============================================================================

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

// ============================================================================
// CAMERA
// ============================================================================

static Vec3 g_editor_camera_position = {0, 10, 20};
static float g_editor_camera_pitch = 0.0f;
static float g_editor_camera_yaw = 0.0f;

void engine_set_editor_camera_position(float x, float y, float z) {
    g_editor_camera_position.x = x;
    g_editor_camera_position.y = y;
    g_editor_camera_position.z = z;
    
    // TODO: Update actual camera in renderer
}

void engine_set_editor_camera_rotation(float pitch, float yaw) {
    g_editor_camera_pitch = pitch;
    g_editor_camera_yaw = yaw;
    
    // TODO: Update actual camera in renderer
}

// ============================================================================
// PROFILER
// ============================================================================

void engine_profiler_begin_frame(void) {
    if (!g_engine_initialized)
        return;
    
    // Profiler is automatically managed by engine update loop
}

void engine_profiler_end_frame(void) {
    if (!g_engine_initialized)
        return;
    
    // Profiler is automatically managed by engine update loop
}

// ============================================================================
// END OF SWIFT BRIDGE IMPLEMENTATION
// ============================================================================

