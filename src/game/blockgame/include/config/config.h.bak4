// include/config/config.h
//
// Purpose: Defines the `GameConfig` structure which encapsulates all configurable
// settings for the game, ranging from rendering and graphics options to performance,
// world generation, audio, and control sensitivities. It also declares functions
// for managing the lifecycle of these configurations (loading from/saving to file,
// setting defaults, and validation).
//
// Public APIs:
// - `GameConfig`: A comprehensive structure containing fields for various game settings.
//   Fields cover rendering (`window_width`, `vsync`, `fov`), graphics (`render_distance`, `shadows`),
//   performance (`max_chunks_loaded`, `multithreading`), world generation (`world_seed`, `generate_structures`),
//   audio volumes, and control parameters (`mouse_sensitivity`, `movement_speed`, `gravity`).
// - `config_load`: Loads game configuration from a specified file into a `GameConfig` structure.
// - `config_save`: Saves the current `GameConfig` to a specified file.
// - `config_set_defaults`: Initializes a `GameConfig` structure with default values.
// - `config_validate`: Ensures that all configuration values are within acceptable and safe ranges.
// - `config_reload_if_modified`: Reloads configuration from a file if it has been modified externally.
// - `config_merge`: Merges override settings into a base configuration.
//
// Ownership: `GameConfig` instances are typically owned by the main game state.
// The functions operate on these instances, modifying their internal state based
// on file I/O or default values.
//
// Invariants:
// - A `GameConfig` structure must be initialized (e.g., with `config_set_defaults`)
//   before loading or saving to prevent undefined behavior.
// - Configuration files are expected to be in a compatible format for loading.
// - `config_validate` should be called after loading or setting defaults to ensure
//   all values are sane and do not cause issues.
//
#ifndef CONFIG_H
#define CONFIG_H


#include "../game_common.h"

// Game configuration
typedef struct GameConfig {
  // Rendering
  u32 window_width;
  u32 window_height;
  bool vsync;
  u32 max_fps;
  f32 fov;
  bool fullscreen;

  // Graphics
  u32 render_distance; // In chunks
  bool fancy_graphics;
  bool smooth_lighting;
  bool ambient_occlusion;
  bool shadows;
  bool ray_tracing; // Enable Vulkan ray tracing (if supported)

  // Performance
  u32 max_chunks_loaded;
  u32 chunk_generation_threads;
  u32 mesh_generation_threads;
  bool multithreading;

  // World
  u32 world_seed;
  bool generate_structures;
  bool generate_caves;
  bool generate_ores;

  // Audio
  f32 master_volume; // 0..1
  f32 music_volume;  // 0..1
  f32 sfx_volume;    // 0..1

  // Controls
  f32 mouse_sensitivity;
  f32 mouse_smoothing;
  bool invert_mouse_y;
  bool toggle_sprint;
  bool toggle_crouch;
  f32 movement_speed;
  f32 sprint_multiplier;
  f32 crouch_multiplier;
  f32 jump_force;
  f32 fly_speed;
  f32 air_control;
  f32 gravity;
  f32 ground_check_distance;
  f32 reach_distance_survival;
  f32 reach_distance_creative;

  // Controller
  f32 controller_deadzone;
  f32 controller_sensitivity;
  f32 controller_vibration;
} GameConfig;

// Load/save configuration
void config_load(GameConfig *config, const char *filename);
void config_save(const GameConfig *config, const char *filename);
void config_set_defaults(GameConfig *config);

// Validate and clamp config values to safe ranges
void config_validate(GameConfig *config);
bool config_reload_if_modified(GameConfig *config, const char *filename,
                               u64 *last_modified);
void config_merge(GameConfig *base, const GameConfig *overrides);

#endif // CONFIG_H
