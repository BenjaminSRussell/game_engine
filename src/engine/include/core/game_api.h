// include/core/game_api.h
//
// Purpose: High-level API for creating 2.5D and 3D games efficiently.
// This provides a simplified interface on top of the engine core, making
// it easy to create games without dealing with low-level engine details.
//
// Public APIs:
// - Simple game object creation and management
// - 2.5D sprite rendering helpers
// - Scene management shortcuts
// - Input handling helpers
//
// Ownership: Game API manages resources through the engine core
//
// Invariants:
// - Engine must be initialized before using game API
//
#ifndef GAME_API_H
#define GAME_API_H

#include <common.h>
#include "include/core/scene.h"
#include <core/engine_core.h>
#include <math/vec2.h>
#include <math/vec3.h>

// Forward declarations
struct EngineCore;

// Game API context
typedef struct GameAPI {
  struct EngineCore *engine;
  Scene *active_scene;
  bool initialized;

  // Callbacks
  void (*on_init)(struct GameAPI *api);
  void (*on_update)(struct GameAPI *api, f32 delta_time);
  void (*on_render)(struct GameAPI *api);
  void (*on_shutdown)(struct GameAPI *api);
} GameAPI;

// Initialize game API
bool game_api_init(GameAPI *api, struct EngineCore *engine);
void game_api_shutdown(GameAPI *api);

// Scene management
Scene *game_api_create_scene(GameAPI *api, const char *name);
void game_api_set_active_scene(GameAPI *api, Scene *scene);
Scene *game_api_get_active_scene(GameAPI *api);

// GameObject creation (high-level)
GameObject *game_api_create_sprite(GameAPI *api, Vec3 position, Vec2 size,
                                   u32 texture_id, u32 layer);
GameObject *game_api_create_mesh(GameAPI *api, Vec3 position, void *mesh_data,
                                 u32 texture_id);
GameObject *game_api_create_entity(GameAPI *api, const char *name);

// GameObject manipulation
void game_api_set_position(GameObject *obj, Vec3 position);
void game_api_set_rotation(GameObject *obj, Vec3 rotation);
void game_api_set_scale(GameObject *obj, Vec3 scale);
Vec3 game_api_get_position(GameObject *obj);
Vec3 game_api_get_rotation(GameObject *obj);
Vec3 game_api_get_scale(GameObject *obj);

// 2.5D specific helpers
void game_api_setup_isometric_camera(GameAPI *api, Vec3 position, f32 tile_size,
                                     f32 height);
void game_api_setup_ortho_camera(GameAPI *api, Vec3 position, f32 zoom);

// Input helpers (would need input system integration)
bool game_api_is_key_pressed(GameAPI *api, u32 key);
bool game_api_is_mouse_button_pressed(GameAPI *api, u32 button);
Vec2 game_api_get_mouse_position(GameAPI *api);

// Rendering helpers
void game_api_set_layer_depth(GameAPI *api, u32 layer, f32 depth);
f32 game_api_get_layer_depth(GameAPI *api, u32 layer);

// Utility functions
GameObject *game_api_find_object(GameAPI *api, const char *name);
void game_api_destroy_object(GameAPI *api, GameObject *obj);

// Quick start helpers for common game types
typedef struct {
  u32 width;
  u32 height;
  const char *title;
  void (*on_init)(GameAPI *api);
  void (*on_update)(GameAPI *api, f32 delta_time);
  void (*on_render)(GameAPI *api);
  void (*on_shutdown)(GameAPI *api);
} Game2_5DConfig;

bool game_api_run_2_5d_game(const Game2_5DConfig *config);

#endif // GAME_API_H
