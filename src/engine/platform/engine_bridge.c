// engine_bridge.c - C implementation of Swift-C bridge
#include "engine_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// MARK: - Global State

static EngineCallbacks g_callbacks = {0};
static bool g_initialized = false;

// Placeholder entity storage (replace with actual ECS)
#define MAX_ENTITIES 10000
static struct {
  uint64_t id;
  char name[256];
  Transform transform;
  bool has_mesh_renderer;
  MeshRendererData mesh_renderer;
  bool has_physics;
  PhysicsData physics;
  bool active;
} g_entities[MAX_ENTITIES];

static uint32_t g_entity_count = 0;
static uint64_t g_next_entity_id = 1;

// MARK: - Initialization

void engine_bridge_init(EngineCallbacks callbacks) {
  g_callbacks = callbacks;
  g_initialized = true;

  // Initialize entity storage
  memset(g_entities, 0, sizeof(g_entities));
  g_entity_count = 0;

  if (g_callbacks.on_log_message) {
    g_callbacks.on_log_message("Engine bridge initialized", LOG_LEVEL_INFO,
                               "EngineBridge");
  }
}

void engine_bridge_shutdown(void) {
  g_initialized = false;

  if (g_callbacks.on_log_message) {
    g_callbacks.on_log_message("Engine bridge shutdown", LOG_LEVEL_INFO,
                               "EngineBridge");
  }
}

void engine_update(float delta_time) {
  if (!g_initialized)
    return;

  // Update physics, rendering, etc.
  // This would integrate with the actual engine systems
}

// MARK: - Entity Management

uint64_t engine_create_entity(const char *name) {
  if (g_entity_count >= MAX_ENTITIES) {
    if (g_callbacks.on_log_message) {
      g_callbacks.on_log_message("Max entities reached", LOG_LEVEL_ERROR,
                                 "EngineBridge");
    }
    return 0;
  }

  uint64_t entity_id = g_next_entity_id++;
  uint32_t index = g_entity_count++;

  g_entities[index].id = entity_id;
  strncpy(g_entities[index].name, name, sizeof(g_entities[index].name) - 1);
  g_entities[index].active = true;

  // Default transform
  g_entities[index].transform.position[0] = 0.0f;
  g_entities[index].transform.position[1] = 0.0f;
  g_entities[index].transform.position[2] = 0.0f;
  g_entities[index].transform.rotation[0] = 0.0f;
  g_entities[index].transform.rotation[1] = 0.0f;
  g_entities[index].transform.rotation[2] = 0.0f;
  g_entities[index].transform.scale[0] = 1.0f;
  g_entities[index].transform.scale[1] = 1.0f;
  g_entities[index].transform.scale[2] = 1.0f;

  if (g_callbacks.on_entity_created) {
    g_callbacks.on_entity_created(entity_id);
  }

  return entity_id;
}

void engine_delete_entity(uint64_t entity_id) {
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].id == entity_id && g_entities[i].active) {
      g_entities[i].active = false;

      if (g_callbacks.on_entity_deleted) {
        g_callbacks.on_entity_deleted(entity_id);
      }
      return;
    }
  }
}

bool engine_entity_exists(uint64_t entity_id) {
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].id == entity_id && g_entities[i].active) {
      return true;
    }
  }
  return false;
}

const char *engine_get_entity_name(uint64_t entity_id) {
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].id == entity_id && g_entities[i].active) {
      return g_entities[i].name;
    }
  }
  return NULL;
}

void engine_set_entity_name(uint64_t entity_id, const char *name) {
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].id == entity_id && g_entities[i].active) {
      strncpy(g_entities[i].name, name, sizeof(g_entities[i].name) - 1);

      if (g_callbacks.on_entity_modified) {
        g_callbacks.on_entity_modified(entity_id);
      }
      return;
    }
  }
}

uint32_t engine_get_all_entities(uint64_t *out_entities, uint32_t max_count) {
  uint32_t count = 0;
  for (uint32_t i = 0; i < g_entity_count && count < max_count; i++) {
    if (g_entities[i].active) {
      out_entities[count++] = g_entities[i].id;
    }
  }
  return count;
}

// MARK: - Transform Component

void engine_set_transform(uint64_t entity_id, const Transform *transform) {
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].id == entity_id && g_entities[i].active) {
      memcpy(&g_entities[i].transform, transform, sizeof(Transform));

      if (g_callbacks.on_entity_modified) {
        g_callbacks.on_entity_modified(entity_id);
      }
      return;
    }
  }
}

void engine_get_transform(uint64_t entity_id, Transform *out_transform) {
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].id == entity_id && g_entities[i].active) {
      memcpy(out_transform, &g_entities[i].transform, sizeof(Transform));
      return;
    }
  }
}

// MARK: - Component Management

bool engine_has_component(uint64_t entity_id, ComponentType type) {
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].id == entity_id && g_entities[i].active) {
      switch (type) {
      case COMPONENT_TRANSFORM:
        return true; // All entities have transform
      case COMPONENT_MESH_RENDERER:
        return g_entities[i].has_mesh_renderer;
      case COMPONENT_PHYSICS:
        return g_entities[i].has_physics;
      default:
        return false;
      }
    }
  }
  return false;
}

void engine_add_component(uint64_t entity_id, ComponentType type,
                          const void *data, uint32_t data_size) {
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].id == entity_id && g_entities[i].active) {
      switch (type) {
      case COMPONENT_MESH_RENDERER:
        g_entities[i].has_mesh_renderer = true;
        if (data) {
          memcpy(&g_entities[i].mesh_renderer, data, sizeof(MeshRendererData));
        }
        break;
      case COMPONENT_PHYSICS:
        g_entities[i].has_physics = true;
        if (data) {
          memcpy(&g_entities[i].physics, data, sizeof(PhysicsData));
        }
        break;
      default:
        break;
      }

      if (g_callbacks.on_entity_modified) {
        g_callbacks.on_entity_modified(entity_id);
      }
      return;
    }
  }
}

void engine_remove_component(uint64_t entity_id, ComponentType type) {
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].id == entity_id && g_entities[i].active) {
      switch (type) {
      case COMPONENT_MESH_RENDERER:
        g_entities[i].has_mesh_renderer = false;
        break;
      case COMPONENT_PHYSICS:
        g_entities[i].has_physics = false;
        break;
      default:
        break;
      }

      if (g_callbacks.on_entity_modified) {
        g_callbacks.on_entity_modified(entity_id);
      }
      return;
    }
  }
}

// MARK: - Mesh Renderer Component

void engine_set_mesh_renderer(uint64_t entity_id,
                              const MeshRendererData *data) {
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].id == entity_id && g_entities[i].active) {
      g_entities[i].has_mesh_renderer = true;
      memcpy(&g_entities[i].mesh_renderer, data, sizeof(MeshRendererData));

      if (g_callbacks.on_entity_modified) {
        g_callbacks.on_entity_modified(entity_id);
      }
      return;
    }
  }
}

void engine_get_mesh_renderer(uint64_t entity_id, MeshRendererData *out_data) {
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].id == entity_id && g_entities[i].active &&
        g_entities[i].has_mesh_renderer) {
      memcpy(out_data, &g_entities[i].mesh_renderer, sizeof(MeshRendererData));
      return;
    }
  }
}

// MARK: - Physics Component

void engine_set_physics(uint64_t entity_id, const PhysicsData *data) {
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].id == entity_id && g_entities[i].active) {
      g_entities[i].has_physics = true;
      memcpy(&g_entities[i].physics, data, sizeof(PhysicsData));

      if (g_callbacks.on_entity_modified) {
        g_callbacks.on_entity_modified(entity_id);
      }
      return;
    }
  }
}

void engine_bridge_get_physics(uint64_t entity_id, PhysicsData *out_data) {
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].id == entity_id && g_entities[i].active &&
        g_entities[i].has_physics) {
      memcpy(out_data, &g_entities[i].physics, sizeof(PhysicsData));
      return;
    }
  }
}

// MARK: - Scene Management

void engine_load_scene(const char *path) {
  if (g_callbacks.on_log_message) {
    char msg[512];
    snprintf(msg, sizeof(msg), "Loading scene: %s", path);
    g_callbacks.on_log_message(msg, LOG_LEVEL_INFO, "SceneManager");
  }

  // TODO: Actual scene loading

  if (g_callbacks.on_scene_loaded) {
    g_callbacks.on_scene_loaded(path);
  }
}

void engine_save_scene(const char *path) {
  if (g_callbacks.on_log_message) {
    char msg[512];
    snprintf(msg, sizeof(msg), "Saving scene: %s", path);
    g_callbacks.on_log_message(msg, LOG_LEVEL_INFO, "SceneManager");
  }

  // TODO: Actual scene saving
}

void engine_new_scene(void) {
  // Clear all entities
  for (uint32_t i = 0; i < g_entity_count; i++) {
    if (g_entities[i].active) {
      engine_delete_entity(g_entities[i].id);
    }
  }

  g_entity_count = 0;

  if (g_callbacks.on_log_message) {
    g_callbacks.on_log_message("New scene created", LOG_LEVEL_INFO,
                               "SceneManager");
  }
}

// MARK: - Rendering

void engine_get_render_stats(RenderStats *out_stats) {
  // Placeholder stats
  out_stats->fps = 60.0f;
  out_stats->frame_time_ms = 16.67f;
  out_stats->draw_calls = 124;
  out_stats->triangles = 45200;
  out_stats->vertices = 67800;
  out_stats->memory_used = 256 * 1024 * 1024; // 256MB
}

// MARK: - Logging

void engine_log(LogLevel level, const char *source, const char *message) {
  if (g_callbacks.on_log_message) {
    g_callbacks.on_log_message(message, level, source);
  }
}
