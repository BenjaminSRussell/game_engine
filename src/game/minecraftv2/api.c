// src/core/game_api.c
//
// High-level game API implementation
//
#include <core/engine_core.h>
#include <core/game_api.h>
#include <core/logger.h>
#include <core/scene.h>
#include <math/mat4.h>
#include <renderer/camera.h>
#include <stdlib.h>
#include <string.h>

// Initialize game API
bool game_api_init(GameAPI *api, struct EngineCore *engine) {
  if (!api || !engine) {
    LOG_ERROR("Invalid parameters for game_api_init");
    return false;
  }

  memset(api, 0, sizeof(GameAPI));
  api->engine = engine;
  api->initialized = true;

  LOG_INFO("Game API initialized");
  return true;
}

void game_api_shutdown(GameAPI *api) {
  if (!api || !api->initialized) {
    return;
  }

  if (api->active_scene) {
    scene_shutdown(api->active_scene);
    free(api->active_scene);
    api->active_scene = NULL;
  }

  api->initialized = false;
  LOG_INFO("Game API shut down");
}

// Scene management
Scene *game_api_create_scene(GameAPI *api, const char *name) {
  if (!api || !api->initialized || !name) {
    return NULL;
  }

  Scene *scene = (Scene *)calloc(1, sizeof(Scene));
  if (!scene) {
    return NULL;
  }

  if (!scene_init(scene, api->engine, name)) {
    free(scene);
    return NULL;
  }

  return scene;
}

void game_api_set_active_scene(GameAPI *api, Scene *scene) {
  if (!api || !api->initialized) {
    return;
  }

  if (api->active_scene) {
    scene_set_active(api->active_scene, false);
  }

  api->active_scene = scene;
  if (scene) {
    scene_set_active(scene, true);
  }
}

Scene *game_api_get_active_scene(GameAPI *api) {
  return api ? api->active_scene : NULL;
}

// GameObject creation
GameObject *game_api_create_sprite(GameAPI *api, Vec3 position, Vec2 size,
                                   u32 texture_id, u32 layer) {
  if (!api || !api->initialized || !api->active_scene) {
    return NULL;
  }

  GameObject *obj = scene_create_game_object(api->active_scene, "sprite");
  if (obj && obj->node) {
    scene_node_set_position(obj->node, position);
    obj->node->layer = layer;
    // Store size and texture_id in user_data or component
  }

  return obj;
}

GameObject *game_api_create_mesh(GameAPI *api, Vec3 position, void *mesh_data,
                                 u32 texture_id) {
  if (!api || !api->initialized || !api->active_scene) {
    return NULL;
  }

  GameObject *obj = scene_create_game_object(api->active_scene, "mesh");
  if (obj && obj->node) {
    scene_node_set_position(obj->node, position);
    obj->user_data = mesh_data;
  }

  return obj;
}

GameObject *game_api_create_entity(GameAPI *api, const char *name) {
  if (!api || !api->initialized || !api->active_scene) {
    return NULL;
  }

  return scene_create_game_object(api->active_scene, name);
}

// GameObject manipulation
void game_api_set_position(GameObject *obj, Vec3 position) {
  if (obj && obj->node) {
    scene_node_set_position(obj->node, position);
  }
}

void game_api_set_rotation(GameObject *obj, Vec3 rotation) {
  if (obj && obj->node) {
    scene_node_set_rotation(obj->node, rotation);
  }
}

void game_api_set_scale(GameObject *obj, Vec3 scale) {
  if (obj && obj->node) {
    scene_node_set_scale(obj->node, scale);
  }
}

Vec3 game_api_get_position(GameObject *obj) {
  return obj && obj->node ? scene_node_get_position(obj->node) : vec3(0, 0, 0);
}

Vec3 game_api_get_rotation(GameObject *obj) {
  return obj && obj->node ? scene_node_get_rotation(obj->node) : vec3(0, 0, 0);
}

Vec3 game_api_get_scale(GameObject *obj) {
  return obj && obj->node ? scene_node_get_scale(obj->node) : vec3(1, 1, 1);
}

// 2.5D specific helpers
void game_api_setup_isometric_camera(GameAPI *api, Vec3 position, f32 tile_size,
                                     f32 height) {
  if (!api || !api->engine || !api->engine->camera) {
    return;
  }

  Camera *camera = api->engine->camera;
  camera->position = position;
  // Setup isometric view (would need camera controller)
  camera->yaw = -45.0f;
  camera->pitch = -30.0f;
}

void game_api_setup_ortho_camera(GameAPI *api, Vec3 position, f32 zoom) {
  if (!api || !api->engine || !api->engine->camera) {
    return;
  }

  Camera *camera = api->engine->camera;
  camera->position = position;
  camera->pitch = -90.0f; // Look down
}

// Input helpers (stubs - would need input system integration)
bool game_api_is_key_pressed(GameAPI *api, u32 key) {
  (void)api;
  (void)key;
  return false;
}

bool game_api_is_mouse_button_pressed(GameAPI *api, u32 button) {
  (void)api;
  (void)button;
  return false;
}

Vec2 game_api_get_mouse_position(GameAPI *api) {
  (void)api;
  return vec2(0, 0);
}

// Rendering helpers
void game_api_set_layer_depth(GameAPI *api, u32 layer, f32 depth) {
  if (api && api->active_scene) {
    scene_set_layer_depth(api->active_scene, layer, depth);
  }
}

f32 game_api_get_layer_depth(GameAPI *api, u32 layer) {
  if (api && api->active_scene) {
    return scene_get_layer_depth(api->active_scene, layer);
  }
  return 0.0f;
}

// Utility functions
GameObject *game_api_find_object(GameAPI *api, const char *name) {
  if (api && api->active_scene) {
    return scene_find_game_object(api->active_scene, name);
  }
  return NULL;
}

void game_api_destroy_object(GameAPI *api, GameObject *obj) {
  if (api && api->active_scene && obj) {
    scene_destroy_game_object(api->active_scene, obj);
  }
}

// Quick start helpers
static GameAPI g_game_api;
static EngineCore g_engine;

static void game_api_update_wrapper(EngineCore *engine, f32 delta_time) {
  if (g_game_api.on_update) {
    g_game_api.on_update(&g_game_api, delta_time);
  }
}

static void game_api_render_wrapper(EngineCore *engine) {
  if (g_game_api.active_scene) {
    scene_update(g_game_api.active_scene, g_game_api.engine->delta_time);
    scene_render(g_game_api.active_scene);
  }

  if (g_game_api.on_render) {
    g_game_api.on_render(&g_game_api);
  }
}

static void game_api_init_wrapper(EngineCore *engine) {
  if (!game_api_init(&g_game_api, engine)) {
    LOG_ERROR("Failed to initialize game API");
    return;
  }

  if (g_game_api.on_init) {
    g_game_api.on_init(&g_game_api);
  }
}

static void game_api_shutdown_wrapper(EngineCore *engine) {
  if (g_game_api.on_shutdown) {
    g_game_api.on_shutdown(&g_game_api);
  }

  game_api_shutdown(&g_game_api);
}

bool game_api_run_2_5d_game(const Game2_5DConfig *config) {
  if (!config) {
    LOG_ERROR("Invalid game config");
    return false;
  }

  EngineCoreConfig engine_config =
      engine_config_create_2_5d(config->width, config->height);
  if (config->title) {
    engine_config.window_title = config->title;
  }

  if (!engine_core_init(&g_engine, &engine_config)) {
    LOG_ERROR("Failed to initialize engine");
    return false;
  }

  // Setup game API
  if (!game_api_init(&g_game_api, &g_engine)) {
    engine_core_shutdown(&g_engine);
    return false;
  }

  // Set callbacks
  g_game_api.on_init = config->on_init;
  g_game_api.on_update = config->on_update;
  g_game_api.on_render = config->on_render;
  g_game_api.on_shutdown = config->on_shutdown;

  engine_set_init_callback(&g_engine, game_api_init_wrapper);
  engine_set_update_callback(&g_engine, game_api_update_wrapper);
  engine_set_render_callback(&g_engine, game_api_render_wrapper);
  engine_set_shutdown_callback(&g_engine, game_api_shutdown_wrapper);

  // Call init
  game_api_init_wrapper(&g_engine);

  // Main loop
  while (g_engine.running) {
    engine_core_update(&g_engine);
    engine_core_render(&g_engine);
  }

  // Cleanup
  engine_core_shutdown(&g_engine);

  return true;
}
