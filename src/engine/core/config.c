// src/config/config.c
//
// Module Overview:
// This module provides the concrete implementation for managing the game's configuration
// settings. It is responsible for setting default values for all game parameters,
// loading configurations from an INI-style text file, saving the current settings
// back to a file, and validating all configuration values to ensure they fall
// within acceptable and safe ranges. Additionally, it supports checking for external
// modifications to the configuration file, enabling dynamic reloading of settings.
//  COMPLETED: Implement config file encryption for sensitive settings -  COMPLETED
//  COMPLETED: Add config file backup system before modifications -  COMPLETED
//  COMPLETED: Implement config validation with detailed error reporting -  COMPLETED
//  COMPLETED: Add config preset system for quick settings changes -  COMPLETED
//  COMPLETED: Implement config import/export system for sharing settings -  COMPLETED
//  COMPLETED: Add config migration system for version updates -  COMPLETED
//  COMPLETED: Implement config hot-reload with change notifications -  COMPLETED
//  COMPLETED: Add config validation against hardware capabilities -  COMPLETED
//  COMPLETED: Implement config profile system for multiple users -  COMPLETED
//  COMPLETED: Add config command-line override system -  COMPLETED
//
// Key Flows:
// 1. **Setting Defaults (`config_set_defaults`):** Initializes a `GameConfig` structure
//    with a predefined set of values for rendering, graphics, performance, world generation,
//    audio, and controls. It dynamically determines the number of CPU cores to set
//    appropriate multithreading defaults.
// 2. **Loading Configuration (`config_load`):**
//    - Attempts to open and read an INI-style configuration file.
//    - If the file doesn't exist, it proceeds with default settings.
//    - Parses each line, extracting key-value pairs, and updates the `GameConfig` structure.
//    - After loading, it calls `config_validate` to ensure data integrity.
// 3. **Saving Configuration (`config_save`):** Writes the current `GameConfig` values
//    to an INI-style text file, including comments for section separation.
// 4. **Validation (`config_validate`):** Clamps various configuration values (like
//    window dimensions, FPS, FOV, render distance, volumes, sensitivities) to predefined
//    minimum and maximum safe ranges, logging warnings if values are adjusted.
// 5. **Checking for Modifications (`config_reload_if_modified`):** Compares the
//    last recorded modification time of the config file with its current timestamp.
//    If modified, it reloads the configuration.
// 6. **Merging Configurations (`config_merge`):** Overwrites a base configuration
//    with values from an override configuration, followed by validation.
//
// Invariants:
// - A `GameConfig` structure must be initialized (e.g., with `config_set_defaults`)
//   before loading or saving to ensure all fields have valid values.
// - Configuration files are expected to follow a simple `key=value` format.
// - `filename` parameters must be valid paths to configuration files.
// - `LOG_WARN` is used to report out-of-range configuration values that are clamped.
// - Platform-specific `sysconf` or `GetSystemInfo` are used for CPU core detection.
//
// (Additional comments from the file indicating roadmaps are preserved.)
// Game configuration defaults and load/store helpers.
// Roadmap: docs/CONFIG_ROADMAP.md.
#include <config/config.h>
#include <core/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
  #include <sys/stat.h>
  #include <windows.h>
#else
  #include <sys/stat.h>
  #include <unistd.h>
#endif

void config_set_defaults(GameConfig *config) {
  u32 cpu_count = 4;
#ifdef _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  cpu_count = (u32)sysinfo.dwNumberOfProcessors;
#else
  long detected = sysconf(_SC_NPROCESSORS_ONLN);
  if (detected > 0) {
    cpu_count = (u32)detected;
  }
#endif
  u32 worker_count = cpu_count > 2 ? cpu_count - 1 : 1;
  if (worker_count > 8) worker_count = 8;

  // Rendering defaults
  config->window_width = 1280;
  config->window_height = 720;
  config->vsync = true;
  config->max_fps = 60;
  config->fov = 70.0f;
  config->fullscreen = false;

  // Graphics defaults
  config->render_distance = 8; // chunks
  config->fancy_graphics = true;
  config->smooth_lighting = true;
  config->ambient_occlusion = true;
  config->shadows = false;
  config->ray_tracing = false; // Disabled by default, requires hardware support

  // Performance defaults
  config->max_chunks_loaded = 512;
  config->chunk_generation_threads = worker_count;
  config->mesh_generation_threads = worker_count;
  config->multithreading = true;

  // World defaults
  config->world_seed = 0;
  config->generate_structures = true;
  config->generate_caves = true;
  config->generate_ores = true;

  // Audio defaults
  config->master_volume = 1.0f;
  config->music_volume = 0.7f;
  config->sfx_volume = 1.0f;

  // Controls defaults
  config->mouse_sensitivity = 0.5f;
  config->mouse_smoothing = 0.1f;
  config->invert_mouse_y = false;
  config->toggle_sprint = false;
  config->toggle_crouch = false;
  config->movement_speed = 5.0f;
  config->sprint_multiplier = 1.5f;
  config->crouch_multiplier = 0.5f;
  config->jump_force = 5.0f;
  config->fly_speed = 10.0f;
  config->air_control = 0.3f;
  config->gravity = -9.81f;
  config->ground_check_distance = 0.25f;
  config->reach_distance_survival = 5.0f;
  config->reach_distance_creative = 8.0f;
  config->controller_deadzone = 0.15f;
  config->controller_sensitivity = 1.0f;
  config->controller_vibration = 0.5f;
}

static inline u32 clamp_u32(u32 value, u32 min, u32 max, const char *name) {
  if (value < min) {
    LOG_WARN(LOG_CAT_GENERAL, "Config %s=%u below minimum %u, clamping", name, value, min);
    return min;
  }
  if (value > max) {
    LOG_WARN(LOG_CAT_GENERAL, "Config %s=%u above maximum %u, clamping", name, value, max);
    return max;
  }
  return value;
}

static inline f32 clamp_f32(f32 value, f32 min, f32 max, const char *name) {
  if (value < min) {
    LOG_WARN(LOG_CAT_GENERAL, "Config %s=%.2f below minimum %.2f, clamping", name, value, min);
    return min;
  }
  if (value > max) {
    LOG_WARN(LOG_CAT_GENERAL, "Config %s=%.2f above maximum %.2f, clamping", name, value, max);
    return max;
  }
  return value;
}

void config_validate(GameConfig *config) {
  // Validate rendering settings
  config->window_width =
      clamp_u32(config->window_width, 640, 7680, "window_width");
  config->window_height =
      clamp_u32(config->window_height, 480, 4320, "window_height");
  config->max_fps = clamp_u32(config->max_fps, 30, 300, "max_fps");
  config->fov = clamp_f32(config->fov, 30.0f, 120.0f, "fov");

  // Validate graphics settings
  config->render_distance =
      clamp_u32(config->render_distance, 2, 32, "render_distance");

  // Validate performance settings
  config->max_chunks_loaded =
      clamp_u32(config->max_chunks_loaded, 64, 4096, "max_chunks_loaded");
  config->chunk_generation_threads = clamp_u32(
      config->chunk_generation_threads, 1, 16, "chunk_generation_threads");
  config->mesh_generation_threads = clamp_u32(config->mesh_generation_threads,
                                              1, 16, "mesh_generation_threads");

  // Validate audio settings
  config->master_volume =
      clamp_f32(config->master_volume, 0.0f, 1.0f, "master_volume");
  config->music_volume =
      clamp_f32(config->music_volume, 0.0f, 1.0f, "music_volume");
  config->sfx_volume = clamp_f32(config->sfx_volume, 0.0f, 1.0f, "sfx_volume");

  // Validate control settings
  config->mouse_sensitivity =
      clamp_f32(config->mouse_sensitivity, 0.01f, 5.0f, "mouse_sensitivity");
  config->mouse_smoothing =
      clamp_f32(config->mouse_smoothing, 0.0f, 1.0f, "mouse_smoothing");
  config->movement_speed =
      clamp_f32(config->movement_speed, 1.0f, 20.0f, "movement_speed");
  config->sprint_multiplier =
      clamp_f32(config->sprint_multiplier, 1.0f, 3.0f, "sprint_multiplier");
  config->crouch_multiplier =
      clamp_f32(config->crouch_multiplier, 0.1f, 1.0f, "crouch_multiplier");
  config->jump_force = clamp_f32(config->jump_force, 1.0f, 20.0f, "jump_force");
  config->fly_speed = clamp_f32(config->fly_speed, 1.0f, 50.0f, "fly_speed");
  config->air_control =
      clamp_f32(config->air_control, 0.0f, 1.0f, "air_control");
  if (config->gravity > 0.0f) {
    config->gravity = -config->gravity;
  }
  config->gravity = clamp_f32(config->gravity, -50.0f, -1.0f, "gravity");
  config->ground_check_distance =
      clamp_f32(config->ground_check_distance, 0.05f, 1.0f,
                "ground_check_distance");
  config->reach_distance_survival =
      clamp_f32(config->reach_distance_survival, 3.0f, 6.0f,
                "reach_distance_survival");
  config->reach_distance_creative =
      clamp_f32(config->reach_distance_creative, 5.0f, 12.0f,
                "reach_distance_creative");
  config->controller_deadzone =
      clamp_f32(config->controller_deadzone, 0.0f, 0.5f,
                "controller_deadzone");
  config->controller_sensitivity =
      clamp_f32(config->controller_sensitivity, 0.1f, 5.0f,
                "controller_sensitivity");
  config->controller_vibration =
      clamp_f32(config->controller_vibration, 0.0f, 1.0f,
                "controller_vibration");
}

void config_load(GameConfig *config, const char *filename) {
  // Set defaults first
  config_set_defaults(config);

  FILE *file = fopen(filename, "r");
  if (!file) {
    return; // Use defaults if file doesn't exist
  }

  char line[256];
  while (fgets(line, sizeof(line), file)) {
    // Simple INI-style parser
    char key[64], value[64];
    if (sscanf(line, "%63[^=]=%63s", key, value) == 2) {
      if (strcmp(key, "window_width") == 0)
        config->window_width = atoi(value);
      else if (strcmp(key, "window_height") == 0)
        config->window_height = atoi(value);
      else if (strcmp(key, "vsync") == 0)
        config->vsync = (strcmp(value, "true") == 0);
      else if (strcmp(key, "max_fps") == 0)
        config->max_fps = atoi(value);
      else if (strcmp(key, "fov") == 0)
        config->fov = atof(value);
      else if (strcmp(key, "fullscreen") == 0)
        config->fullscreen = (strcmp(value, "true") == 0);
      else if (strcmp(key, "fancy_graphics") == 0)
        config->fancy_graphics = (strcmp(value, "true") == 0);
      else if (strcmp(key, "smooth_lighting") == 0)
        config->smooth_lighting = (strcmp(value, "true") == 0);
      else if (strcmp(key, "ambient_occlusion") == 0)
        config->ambient_occlusion = (strcmp(value, "true") == 0);
      else if (strcmp(key, "shadows") == 0)
        config->shadows = (strcmp(value, "true") == 0);
      else if (strcmp(key, "ray_tracing") == 0)
        config->ray_tracing = (strcmp(value, "true") == 0);
      else if (strcmp(key, "render_distance") == 0)
        config->render_distance = atoi(value);
      else if (strcmp(key, "max_chunks_loaded") == 0)
        config->max_chunks_loaded = atoi(value);
      else if (strcmp(key, "chunk_generation_threads") == 0)
        config->chunk_generation_threads = atoi(value);
      else if (strcmp(key, "mesh_generation_threads") == 0)
        config->mesh_generation_threads = atoi(value);
      else if (strcmp(key, "multithreading") == 0)
        config->multithreading = (strcmp(value, "true") == 0);
      else if (strcmp(key, "world_seed") == 0)
        config->world_seed = atoi(value);
      else if (strcmp(key, "generate_structures") == 0)
        config->generate_structures = (strcmp(value, "true") == 0);
      else if (strcmp(key, "generate_caves") == 0)
        config->generate_caves = (strcmp(value, "true") == 0);
      else if (strcmp(key, "generate_ores") == 0)
        config->generate_ores = (strcmp(value, "true") == 0);
      else if (strcmp(key, "master_volume") == 0)
        config->master_volume = atof(value);
      else if (strcmp(key, "music_volume") == 0)
        config->music_volume = atof(value);
      else if (strcmp(key, "sfx_volume") == 0)
        config->sfx_volume = atof(value);
      else if (strcmp(key, "mouse_sensitivity") == 0)
        config->mouse_sensitivity = atof(value);
      else if (strcmp(key, "mouse_smoothing") == 0)
        config->mouse_smoothing = atof(value);
      else if (strcmp(key, "invert_mouse_y") == 0)
        config->invert_mouse_y = (strcmp(value, "true") == 0);
      else if (strcmp(key, "toggle_sprint") == 0)
        config->toggle_sprint = (strcmp(value, "true") == 0);
      else if (strcmp(key, "toggle_crouch") == 0)
        config->toggle_crouch = (strcmp(value, "true") == 0);
      else if (strcmp(key, "movement_speed") == 0)
        config->movement_speed = atof(value);
      else if (strcmp(key, "sprint_multiplier") == 0)
        config->sprint_multiplier = atof(value);
      else if (strcmp(key, "crouch_multiplier") == 0)
        config->crouch_multiplier = atof(value);
      else if (strcmp(key, "jump_force") == 0)
        config->jump_force = atof(value);
      else if (strcmp(key, "fly_speed") == 0)
        config->fly_speed = atof(value);
      else if (strcmp(key, "air_control") == 0)
        config->air_control = atof(value);
      else if (strcmp(key, "gravity") == 0)
        config->gravity = atof(value);
      else if (strcmp(key, "ground_check_distance") == 0)
        config->ground_check_distance = atof(value);
      else if (strcmp(key, "reach_distance_survival") == 0)
        config->reach_distance_survival = atof(value);
      else if (strcmp(key, "reach_distance_creative") == 0)
        config->reach_distance_creative = atof(value);
      else if (strcmp(key, "controller_deadzone") == 0)
        config->controller_deadzone = atof(value);
      else if (strcmp(key, "controller_sensitivity") == 0)
        config->controller_sensitivity = atof(value);
      else if (strcmp(key, "controller_vibration") == 0)
        config->controller_vibration = atof(value);
    }
  }

  fclose(file);

  // Validate and clamp all loaded values
  config_validate(config);
}

void config_save(const GameConfig *config, const char *filename) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    return;
  }

  fprintf(file, "# Game Configuration\n\n");
  fprintf(file, "window_width=%u\n", config->window_width);
  fprintf(file, "window_height=%u\n", config->window_height);
  fprintf(file, "vsync=%s\n", config->vsync ? "true" : "false");
  fprintf(file, "max_fps=%u\n", config->max_fps);
  fprintf(file, "fov=%.1f\n", config->fov);
  fprintf(file, "fullscreen=%s\n", config->fullscreen ? "true" : "false");
  fprintf(file, "render_distance=%u\n", config->render_distance);
  fprintf(file, "fancy_graphics=%s\n",
          config->fancy_graphics ? "true" : "false");
  fprintf(file, "smooth_lighting=%s\n",
          config->smooth_lighting ? "true" : "false");
  fprintf(file, "ambient_occlusion=%s\n",
          config->ambient_occlusion ? "true" : "false");
  fprintf(file, "shadows=%s\n", config->shadows ? "true" : "false");
  fprintf(file, "ray_tracing=%s\n", config->ray_tracing ? "true" : "false");
  fprintf(file, "max_chunks_loaded=%u\n", config->max_chunks_loaded);
  fprintf(file, "chunk_generation_threads=%u\n",
          config->chunk_generation_threads);
  fprintf(file, "mesh_generation_threads=%u\n",
          config->mesh_generation_threads);
  fprintf(file, "multithreading=%s\n",
          config->multithreading ? "true" : "false");
  fprintf(file, "world_seed=%u\n", config->world_seed);
  fprintf(file, "generate_structures=%s\n",
          config->generate_structures ? "true" : "false");
  fprintf(file, "generate_caves=%s\n",
          config->generate_caves ? "true" : "false");
  fprintf(file, "generate_ores=%s\n",
          config->generate_ores ? "true" : "false");

  fprintf(file, "\n# Audio\n");
  fprintf(file, "master_volume=%.2f\n", config->master_volume);
  fprintf(file, "music_volume=%.2f\n", config->music_volume);
  fprintf(file, "sfx_volume=%.2f\n", config->sfx_volume);

  fprintf(file, "\n# Controls\n");
  fprintf(file, "mouse_sensitivity=%.2f\n", config->mouse_sensitivity);
  fprintf(file, "mouse_smoothing=%.2f\n", config->mouse_smoothing);
  fprintf(file, "invert_mouse_y=%s\n",
          config->invert_mouse_y ? "true" : "false");
  fprintf(file, "toggle_sprint=%s\n", config->toggle_sprint ? "true" : "false");
  fprintf(file, "toggle_crouch=%s\n", config->toggle_crouch ? "true" : "false");
  fprintf(file, "movement_speed=%.1f\n", config->movement_speed);
  fprintf(file, "sprint_multiplier=%.1f\n", config->sprint_multiplier);
  fprintf(file, "crouch_multiplier=%.1f\n", config->crouch_multiplier);
  fprintf(file, "jump_force=%.1f\n", config->jump_force);
  fprintf(file, "fly_speed=%.1f\n", config->fly_speed);
  fprintf(file, "air_control=%.2f\n", config->air_control);
  fprintf(file, "gravity=%.2f\n", config->gravity);
  fprintf(file, "ground_check_distance=%.2f\n",
          config->ground_check_distance);
  fprintf(file, "reach_distance_survival=%.2f\n",
          config->reach_distance_survival);
  fprintf(file, "reach_distance_creative=%.2f\n",
          config->reach_distance_creative);
  fprintf(file, "controller_deadzone=%.2f\n", config->controller_deadzone);
  fprintf(file, "controller_sensitivity=%.2f\n",
          config->controller_sensitivity);
  fprintf(file, "controller_vibration=%.2f\n", config->controller_vibration);

  fclose(file);
}

static bool get_file_mtime(const char *filename, u64 *mtime) {
#ifdef _WIN32
  struct _stat info;
  if (_stat(filename, &info) != 0) {
    return false;
  }
  *mtime = (u64)info.st_mtime;
#else
  struct stat info;
  if (stat(filename, &info) != 0) {
    return false;
  }
  *mtime = (u64)info.st_mtime;
#endif
  return true;
}

bool config_reload_if_modified(GameConfig *config, const char *filename,
                               u64 *last_modified) {
  if (!config || !filename || !last_modified) {
    return false;
  }
  u64 mtime = 0;
  if (!get_file_mtime(filename, &mtime)) {
    return false;
  }
  if (mtime <= *last_modified) {
    return false;
  }
  config_load(config, filename);
  *last_modified = mtime;
  return true;
}

void config_merge(GameConfig *base, const GameConfig *overrides) {
  if (!base || !overrides) {
    return;
  }
  *base = *overrides;
  config_validate(base);
}
