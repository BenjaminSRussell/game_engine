// src/engine/examples/prototype/main.c
//
// Purpose: Minimal working prototype demonstrating the game engine
//
#include <core/asset_manager.h>
#include <core/logger.h>
#include <core/engine.h>
#include <core/game_module.h>
#include <input/input.h>
#include <math.h>
#include <physics/physics.h>
#include <renderer/renderer.h>
#include <stdio.h>
#include <stdlib.h> // Added
#include <tools/hot_reload.h>
#include <tools/profiler.h>

// -----------------------------------------------------------------------------
// Game Module Implementation
// -----------------------------------------------------------------------------

// Prototype Game Module Callbacks
static struct {
  u32 frame_count;
  RigidBody *player_body;
  RigidBody *floor_body;
  RigidBody *spheres[10];
  u32 sphere_count;
  Shader *shader;
} g_prototype_state;

static void on_test_reload(const char *path, void *user_data) {
  (void)user_data;
  LOG_INFO("[HotReload] File changed: %s", path);
}

bool prototype_init(GameModule *module, Engine *engine) {
  (void)module;
  LOG_INFO("[prototype_init] Prototype Game Module Initialized");
  g_prototype_state.frame_count = 0;

  // Register hot reload callback for verification
  hot_reload_register_callback("txt", on_test_reload, NULL);

  // Asset Loading
  AssetManager *assets = engine_get_assets(engine);
  if (assets) {
    LOG_INFO("[prototype_init] Loading assets from manifest...");
    asset_manager_preload(assets, "assets/manifest.json");

    Asset *vs = asset_manager_get(assets, "shader_basic_vs");
    Asset *fs = asset_manager_get(assets, "shader_basic_fs");

    if (vs && fs) {
      LOG_INFO("[prototype_init] Shaders loaded. Creating program...");
      g_prototype_state.shader = renderer_create_shader(
          engine_get_renderer(engine), (const char *)vs->data,
          (const char *)fs->data);
    } else {
      LOG_WARN("[prototype_init] Failed to load shaders");
    }
  }

  PhysicsWorld *phys = engine_get_physics(engine);
  if (!phys) {
    LOG_ERROR("Physics world not available");
    return false;
  }

  // 1. Create Floor
  g_prototype_state.floor_body =
      rigid_body_create(BODY_TYPE_STATIC, vec3(0, -2, 0));
  Collider *floor_col = collider_create_box(vec3(50, 1, 50));
  rigid_body_attach_collider(g_prototype_state.floor_body, floor_col);
  physics_world_add_body(phys, g_prototype_state.floor_body);

  // 2. Create Player
  g_prototype_state.player_body =
      rigid_body_create(BODY_TYPE_DYNAMIC, vec3(0, 5, 0));
  Collider *player_col = collider_create_sphere(0.8f);
  rigid_body_attach_collider(g_prototype_state.player_body, player_col);
  rigid_body_set_friction(g_prototype_state.player_body, 1.0f);
  physics_world_add_body(phys, g_prototype_state.player_body);

  // 3. Create some falling spheres
  g_prototype_state.sphere_count = 5;
  for (u32 i = 0; i < g_prototype_state.sphere_count; i++) {
    g_prototype_state.spheres[i] = rigid_body_create(
        BODY_TYPE_DYNAMIC, vec3(i * 0.5f, 10 + i * 3, i * 0.5f));
    Collider *sphere_col = collider_create_sphere(1.0f);
    rigid_body_attach_collider(g_prototype_state.spheres[i], sphere_col);
    rigid_body_set_restitution(g_prototype_state.spheres[i], 0.5f);
    rigid_body_set_friction(g_prototype_state.spheres[i], 0.3f);
    physics_world_add_body(phys, g_prototype_state.spheres[i]);
  }

  return true;
}

void prototype_shutdown(GameModule *module) {
  (void)module;
  LOG_INFO("[prototype_shutdown] Prototype Game Module Shutdown");
  profiler_print_summary();
}

void prototype_update(GameModule *module, Engine *engine, f32 delta_time) {
  PROFILE_SCOPE("Prototype_Update");
  (void)module;

  PhysicsWorld *phys = engine_get_physics(engine);
  if (phys) {
    physics_world_step(phys, delta_time);
  }

  g_prototype_state.frame_count++;

  // Log positions occasionally to verify physics is working
  if (g_prototype_state.frame_count % 60 == 0) {
    Vec3 p0 = rigid_body_get_position(g_prototype_state.spheres[0]);
    LOG_INFO("[prototype_update] Sphere 0 Position: (%.2f, %.2f, %.2f)", p0.x,
             p0.y, p0.z);

    // Also print profiler summary every 60 frames for verification
    profiler_print_summary();
  }
}

void prototype_handle_input(GameModule *module, Engine *engine) {
  PROFILE_SCOPE("Prototype_Input");
  (void)module;
  InputState *input = (InputState *)engine_get_input(engine);
  if (!input)
    return;

  if (!g_prototype_state.player_body)
    return;

  f32 speed = 5.0f;
  Vec3 impulse = vec3_zero();

  if (input_is_action_held(input, INPUT_ACTION_MOVE_FORWARD))
    impulse.z -= 1.0f;
  if (input_is_action_held(input, INPUT_ACTION_MOVE_BACKWARD))
    impulse.z += 1.0f;
  if (input_is_action_held(input, INPUT_ACTION_MOVE_LEFT))
    impulse.x -= 1.0f;
  if (input_is_action_held(input, INPUT_ACTION_MOVE_RIGHT))
    impulse.x += 1.0f;

  if (vec3_length(impulse) > 0) {
    impulse = vec3_normalize(impulse);
    impulse = vec3_mul(impulse, speed);
    rigid_body_add_force(g_prototype_state.player_body, impulse);
  }

  if (input_is_action_pressed(input, INPUT_ACTION_JUMP)) {
    rigid_body_add_impulse(g_prototype_state.player_body, vec3(0, 5.0f, 0));
  }
}

void prototype_render(GameModule *module, Engine *engine) {
  PROFILE_SCOPE("Prototype_Render");
  (void)module;

  Renderer *renderer = engine_get_renderer(engine);
  if (g_prototype_state.shader) {
    renderer_bind_shader(renderer, g_prototype_state.shader);
  }

  renderer_draw_test_triangle(renderer);
}

// -----------------------------------------------------------------------------
// Main Entry Point
// -----------------------------------------------------------------------------

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf("=================================\n");
  printf("  Game Engine Prototype Demo\n");
  printf("  (Now using unified Engine API)\n");
  printf("=================================\n\n");

  // Define Game Module with new comprehensive API
  static GameModule module = {
      .info =
          {
              .name = "Prototype",
              .version = "0.1.0",
              .author = "Engine Team",
              .description = "Basic prototype game module",
              .capabilities = GAME_MODULE_CAP_3D_RENDERING,
              .state = GAME_MODULE_STATE_UNLOADED,
          },
      .initialize = prototype_init,
      .shutdown = prototype_shutdown,
      .update = prototype_update,
      .handle_input = prototype_handle_input,
      .render = prototype_render,
  };

  // Create Engine Config
  EngineConfig config = engine_create_default_config();
  config.window_width = 1280;
  config.window_height = 720;
  config.window_title = "Engine Prototype (OpenGL Backend)";
  config.log_level = "DEBUG"; // Ensure we see init logs

  // Create and Initialize Engine
  Engine engine; // Stack allocation for now
  if (!engine_init(&engine, &config)) {
    LOG_FATAL("Failed to initialize engine");
    return EXIT_FAILURE;
  }

  // Attach Game Module
  engine_set_game_module(&engine, &module);

  // Run Engine
  engine_run(&engine, &module);

  // Cleanup
  engine_shutdown(&engine);

  return EXIT_SUCCESS;
}
