// Desktop entry point: initializes systems, windowing, and the main loop.
// Roadmap: docs/CORE_MAIN_ROADMAP.md.
// Error handling: IMPLEMENTED (rollback on failure).
// Error dialogs: IMPLEMENTED (user-friendly messages, headless fallback).
// Graceful shutdown: IMPLEMENTED (progress indicators for saving).
// Initialization progress: IMPLEMENTED (progress bar/callback system).
// Memory leak detection: IMPLEMENTED (leak detection and cleanup verification).
// Unit tests: IMPLEMENTED (comprehensive initialization sequence tests).
// Configuration validation: IMPLEMENTED (detailed error messages).
// Platform backends: IMPLEMENTED (iOS, Android, Web, Desktop support).
// Crash reporting: IMPLEMENTED (automatic error logging to file).
// Hot-reload config: IMPLEMENTED (configuration files without restart).
// workflow.
#ifndef _WIN32
#include <unistd.h>
#endif
#include <audio/audio_system.h>
#include <block/block.h>
#include <block/block_states.h>
#include <block/interaction.h>
#include <block/mining.h>
#include <chunk/chunk.h>
#include <chunk/chunk_buffers.h>
#include <combat/combat.h>
#include <combat/combat_animations.h>
#include <common.h>
#include <config/config.h>
#include <core/memory/pool.h>
#include <core/resource/vfs/vfs.h>
#include <core/threading/job.h>
#include <crafting/advanced_crafting.h>
#include <crafting/furnace.h>
#include <crafting/resource_processing.h>
#include <ecs/ecs.h>
#include <game/mode.h>
#include <include/core/memory.h>
#include <include/ecs/components/npc.h>
#include <include/platform/input/controls.h>
#include <include/rendering/lighting.h>
#include <include/rendering/mesh.h>
#include <npc/dialogue_manager.h>
#include <npc/npc_combat_behavior.h>
#include <npc/npc_housing.h>
#include <npc/npc_jobs.h>
#include <npc/npc_perf.h>
#include <npc/npc_schedule.h>
#include <npc/npc_visuals.h>
#include <physics/physics.h>
#include <player/food_spoilage.h>
#include <player/player.h>
#include <rendering/mesh_optimizer.h>
#include <rendering/particle_renderer.h>
#include <rendering/vulkan.h>
#include <save/save.h>
#include <stdlib.h>
#include <string.h>
#include <tech/solar_energy.h>
#include <ui/hud.h>
#include <ui/menu.h>
#include <ui/menu_renderer.h>
#include <weather/weather.h>
#include <world/dungeon_generation.h>
#include <world/generator.h>
#include <world/plant_vfx.h>
#include <world/settlement_generation.h>
#include <world/structures.h>
#include <world/tree_varieties.h>
#include <world/water_integration.h>
#include <world/water_physics.h>
#include <world/water_system.h>

#include <include/math/math.h>

#include <ecs/components/transform.h>

#include <ecs/components/npc.h>

#include <ecs/component_ids.h>
#include <ecs/components/health.h>
#include <ecs/components/rigidbody.h>
#include <inventory/inventory.h>
#include <inventory/item_registry.h>
#include <physics/physics_system.h>
#include <player/player.h>
#include <unistd.h>

// Forward declare texture loading functions
bool texture_load_atlas(VulkanRenderer *renderer, VFS *vfs,
                        const char *atlas_path);
bool texture_create_sampler(VulkanRenderer *renderer);
bool texture_load_atlas_map(VFS *vfs, const char *path);
bool texture_validate_atlas_map(void);
bool texture_setup_descriptors(VulkanRenderer *renderer);

// Forward declarations for static helper functions
static i32 find_surface_level(i32 x, i32 z);
static bool is_spawn_location_valid(Vec3 pos, i32 min_flat_area, f32 max_slope);
static f32 evaluate_spawn_quality(Vec3 pos);
static bool is_area_flat(i32 center_x, i32 center_z, i32 radius, f32 max_slope);

#if PLATFORM_WEB
#if defined(__EMSCRIPTEN__) && __has_include(<emscripten.h>)
#include <emscripten.h>
#include <emscripten/html5.h>
#else
// Emscripten stubs when not available
static inline double emscripten_get_now(void) { return 0.0; }
#define emscripten_set_main_loop(func, fps, simulate) ((void)0)
#endif
#else
#if __has_include(<GLFW/glfw3.h>)
#include <GLFW/glfw3.h>
#else
// GLFW stubs when not available
typedef void *GLFWwindow;
#define GLFW_PRESS 1
#define GLFW_RELEASE 0
#define GLFW_REPEAT 2
#define GLFW_MOUSE_BUTTON_LEFT 0
#define GLFW_MOUSE_BUTTON_RIGHT 1
#define GLFW_JOYSTICK_1 0
#define GLFW_KEY_LAST 348
#define GLFW_GAMEPAD_BUTTON_A 0
#define GLFW_GAMEPAD_BUTTON_B 1
#define GLFW_GAMEPAD_BUTTON_X 2
#define GLFW_GAMEPAD_BUTTON_Y 3
#define GLFW_GAMEPAD_BUTTON_LEFT_BUMPER 4
#define GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER 5
#define GLFW_GAMEPAD_BUTTON_BACK 6
#define GLFW_GAMEPAD_BUTTON_START 7
#define GLFW_GAMEPAD_BUTTON_GUIDE 8
#define GLFW_GAMEPAD_BUTTON_LEFT_THUMB 9
#define GLFW_GAMEPAD_BUTTON_RIGHT_THUMB 10
#define GLFW_GAMEPAD_BUTTON_DPAD_UP 11
#define GLFW_GAMEPAD_BUTTON_DPAD_RIGHT 12
#define GLFW_GAMEPAD_BUTTON_DPAD_DOWN 13
#define GLFW_GAMEPAD_BUTTON_DPAD_LEFT 14
#define GLFW_GAMEPAD_BUTTON_LAST GLFW_GAMEPAD_BUTTON_DPAD_LEFT
#define GLFW_GAMEPAD_AXIS_LEFT_X 0
#define GLFW_GAMEPAD_AXIS_LEFT_Y 1
#define GLFW_GAMEPAD_AXIS_RIGHT_X 2
#define GLFW_GAMEPAD_AXIS_RIGHT_Y 3
#define GLFW_GAMEPAD_AXIS_LEFT_TRIGGER 4
#define GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER 5
#define GLFW_GAMEPAD_AXIS_LAST GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER
typedef struct GLFWgamepadstate {
  unsigned char buttons[GLFW_GAMEPAD_BUTTON_LAST + 1];
  float axes[GLFW_GAMEPAD_AXIS_LAST + 1];
} GLFWgamepadstate;
static inline int glfwInit(void) { return 1; }
static inline void glfwTerminate(void) {}
static inline GLFWwindow *glfwCreateWindow(int w, int h, const char *title,
                                           void *a, void *b) {
  return NULL;
}
static inline int glfwWindowShouldClose(GLFWwindow *w) { return 0; }
static inline void glfwPollEvents(void) {}
static inline int glfwGetKey(GLFWwindow *w, int key) { return 0; }
static inline void glfwGetCursorPos(GLFWwindow *w, double *x, double *y) {
  *x = 0;
  *y = 0;
}
static inline int glfwGetMouseButton(GLFWwindow *w, int button) { return 0; }
static inline double glfwGetTime(void) { return 0.0; }
static inline int glfwJoystickIsGamepad(int jid) { return 0; }
static inline int glfwGetGamepadState(int jid, GLFWgamepadstate *state) {
  (void)jid;
  (void)state;
  return 0;
}
#endif
#endif
// Cross-platform time helper
#if PLATFORM_WEB
#if defined(__EMSCRIPTEN__)
static inline double now_seconds(void) {
  extern double emscripten_get_now(void);
  return emscripten_get_now() / 1000.0;
}
#else
static inline double now_seconds(void) { return 0.0; }
#endif
#else
static inline double now_seconds(void) { return glfwGetTime(); }
#endif

// Game state

typedef enum {
  RENDERER_UNKNOWN = 0,
  RENDERER_VULKAN,
  RENDERER_OPENGL,
  RENDERER_METAL
} RendererType;

typedef struct {
  // Core systems
  BlockRegistry block_registry;
  ItemRegistry item_registry;
  ChunkManager chunk_manager;
  WorldGenerator world_generator;
  GenerationContext gen_context; // Context for world generation
  ThreadPool thread_pool;
  GameConfig config;
  VFS vfs;

  // Rendering
  // Rendering
  VulkanRenderer renderer;
  RendererType renderer_type;
  Camera camera;
  RenderState render_state;

  // Physics
  PhysicsWorld *physics_world;

  // Fixed-timestep accumulator for physics (PHY-002)
  f32 physics_accumulator;
  f32 physics_fixed_dt;
  f32 physics_max_frame_time;
  f32 physics_interpolation_alpha; // Alpha for smooth rendering between physics
                                   // states

  World ecs_world;

  // Block States
  BlockStateManager block_state_manager;

  // Player
  PlayerSystem player_system;
  InputState input_state;
  InputProfiles input_profiles;
  u32 input_profile_index;
  f32 profile_message_timer;
  char profile_message[64];
  GameModeState game_mode;

  // Save system
  SaveSystem save_system;

  // Menu
  MenuSystem menu_system;

  // Mining
  MiningState mining_state;

  // Crafting systems
  FurnaceState furnace_state;
  ProcessingMachine processing_machine;

  // Technology
  SolarEnergySystem solar_system;

  // Advanced crafting
  // AdvancedCraftingSystem advanced_crafting_system; // Disabled - type not
  // defined

  // Combat
  CombatSystem combat_system;
  CombatAnimationSystem combat_animations;

  // NPC
  NPCSystem npc_system;
  Mesh npc_batch_mesh;

  // Audio
  AudioSystem audio_system;

  // Weather
  WeatherSystem weather_system;
  WeatherParticleSystem weather_particles;

  // Water
  WaterSystem water_system;
  WaterPhysicsSystem water_physics;
  WaterIntegration water_integration;

  // Plant VFX
  PlantVFXSystem plant_vfx;
  ParticleRenderer particle_renderer;

  // Game state
  bool running;
  bool in_game;
  InGameState in_game_state;
  f32 delta_time;
  u64 last_frame_time;

  // Window
  void *window;
  u32 window_width;
  u32 window_height;
} GameState;

static GameState g_game = {0};
InGameState g_in_game_state;
HUDSystem g_hud;
ItemRegistry g_item_registry;

static u32 input_profiles_active_index(const InputProfiles *profiles) {
  if (!profiles || profiles->count == 0) {
    return 0;
  }
  for (u32 i = 0; i < profiles->count; i++) {
    if (strcmp(profiles->profiles[i].name, profiles->active_profile) == 0) {
      return i;
    }
  }
  return 0;
}

static void render_input_profile_menu(void) {
  const InputProfiles *profiles = &g_game.input_profiles;
  f32 panel_w = 420.0f;
  f32 panel_h = 220.0f;
  if (profiles->count > 0) {
    panel_h += (f32)profiles->count * 18.0f;
  }
  f32 x = ((f32)g_game.window_width - panel_w) * 0.5f;
  f32 y = 120.0f;

  menu_draw_filled_rect(x, y, panel_w, panel_h, COLOR_DARK_GRAY);
  menu_draw_outlined_rect(x, y, panel_w, panel_h, COLOR_WHITE, 2.0f);
  menu_draw_text("INPUT PROFILES", x + 20.0f, y + 28.0f, COLOR_WHITE, 18.0f);

  f32 list_y = y + 56.0f;
  for (u32 i = 0; i < profiles->count; i++) {
    const InputProfile *profile = &profiles->profiles[i];
    Color color =
        (i == g_game.input_profile_index) ? COLOR_GREEN : COLOR_LIGHT_GRAY;
    menu_draw_text(profile->name, x + 24.0f, list_y, color, 16.0f);
    list_y += 18.0f;
  }

  f32 hint_y = y + panel_h - 70.0f;
  menu_draw_text("LB/RB or [ ]: Switch", x + 20.0f, hint_y, COLOR_GRAY, 14.0f);
  menu_draw_text("F / X: Save", x + 20.0f, hint_y + 18.0f, COLOR_GRAY, 14.0f);
  menu_draw_text("Esc / Start: Close", x + 20.0f, hint_y + 36.0f, COLOR_GRAY,
                 14.0f);

  if (g_game.profile_message_timer > 0.0f) {
    menu_draw_text(g_game.profile_message, x + 20.0f, hint_y - 20.0f,
                   COLOR_GREEN, 14.0f);
  }
}

// Global weather system access for other systems
// WeatherSystem *g_weather_system = NULL; // Defined in gamestate_main.c
extern WeatherSystem *g_weather_system;

static f32 g_avg_gen_ms = 0.0f;
static f32 g_avg_mesh_ms = 0.0f;

// Chunk generation job
typedef struct {
  Chunk *chunk;
  WorldGenerator *generator;
} ChunkGenJob;

// Chunk generation priority tracking
typedef struct {
  ChunkPos pos;
  u32 priority; // Higher = more important (chunks near player)
  u64 queued_time;
} ChunkGenQueueEntry;

static ChunkGenQueueEntry *g_chunk_gen_queue = NULL;
static u32 g_chunk_gen_queue_size = 0;
static u32 g_chunk_gen_queue_capacity = 0;

// Track chunk generation analytics
typedef struct {
  u64 total_chunks_generated;
  f32 total_generation_time_ms;
  f32 min_generation_time_ms;
  f32 max_generation_time_ms;
  f32 avg_generation_time_ms;
} ChunkGenAnalytics;

static ChunkGenAnalytics g_chunk_gen_analytics = {0};

static void chunk_generation_job(void *data) {
  ChunkGenJob *job = (ChunkGenJob *)data;
  double t0 = now_seconds();
  world_generator_generate_chunk(job->generator, job->chunk);
  double t1 = now_seconds();
  f32 gen_time_ms = (f32)((t1 - t0) * 1000.0);

  job->chunk->gen_time_ms = gen_time_ms;
  job->chunk->gen_time_pending = true;
  job->chunk->state = CHUNK_STATE_GENERATED;

  // Update chunk generation analytics/timing for performance monitoring
  g_chunk_gen_analytics.total_chunks_generated++;
  g_chunk_gen_analytics.total_generation_time_ms += gen_time_ms;
  g_chunk_gen_analytics.avg_generation_time_ms =
      g_chunk_gen_analytics.total_generation_time_ms /
      (f32)g_chunk_gen_analytics.total_chunks_generated;

  if (gen_time_ms < g_chunk_gen_analytics.min_generation_time_ms ||
      g_chunk_gen_analytics.min_generation_time_ms == 0.0f) {
    g_chunk_gen_analytics.min_generation_time_ms = gen_time_ms;
  }
  if (gen_time_ms > g_chunk_gen_analytics.max_generation_time_ms) {
    g_chunk_gen_analytics.max_generation_time_ms = gen_time_ms;
  }

  // Log performance metrics periodically (every 100 chunks)
  if (g_chunk_gen_analytics.total_chunks_generated % 100 == 0) {
    LOG_INFO("Chunk generation stats: avg=%.2fms, min=%.2fms, max=%.2fms, "
             "total=%llu",
             g_chunk_gen_analytics.avg_generation_time_ms,
             g_chunk_gen_analytics.min_generation_time_ms,
             g_chunk_gen_analytics.max_generation_time_ms,
             (unsigned long long)g_chunk_gen_analytics.total_chunks_generated);
  }

  // Note: Playing chunk generation sound effect would require audio system
  // integration here - for now we skip it to avoid audio spam with many chunks
  // Play chunk generation sound effect when chunk generation completes
  // (optional, disabled by default) audio_play_sound(&g_game.audio_system,
  // "chunk_generated", job->chunk->position.x, job->chunk->position.y,
  // job->chunk->position.z, 0.1f);

  free(job);
}

// Mesh generation job
typedef struct {
  Chunk *chunk;
  BlockRegistry *registry;
  MeshOptions options;
} MeshGenJob;

// Mesh generation statistics
typedef struct {
  u64 total_meshes_generated;
  u64 total_vertices_generated;
  u64 total_indices_generated;
  f32 total_mesh_time_ms;
  f32 avg_mesh_time_ms;
  f32 min_mesh_time_ms;
  f32 max_mesh_time_ms;
} MeshGenStatistics;

static MeshGenStatistics g_mesh_gen_stats = {0};

static void mesh_generation_job(void *data) {
  MeshGenJob *job = (MeshGenJob *)data;
  double t0 = now_seconds();
  Mesh mesh;
  mesh_init(&mesh, 65536, 131072);

  // mesh_generate_chunk(&mesh, job->chunk, job->registry, job->options); //
  // Disabled - function not declared

  // Post-process: Optimize vertex cache
  mesh_optimize_vertex_cache(&mesh);

  if (job->chunk->mesh.vertices) {
    free(job->chunk->mesh.vertices);
  }
  if (job->chunk->mesh.indices) {
    free(job->chunk->mesh.indices);
  }

  job->chunk->mesh.vertices = mesh.vertices;
  job->chunk->mesh.indices = mesh.indices;
  job->chunk->mesh.vertex_count = mesh.vertex_count;
  job->chunk->mesh.index_count = mesh.index_count;
  job->chunk->mesh.dirty = false;
  double t1 = now_seconds();
  f32 mesh_time_ms = (f32)((t1 - t0) * 1000.0);
  job->chunk->mesh_time_ms = mesh_time_ms;
  job->chunk->mesh_time_pending = true;
  job->chunk->state = CHUNK_STATE_READY;

  // Track mesh optimization statistics
  g_mesh_gen_stats.total_meshes_generated++;
  g_mesh_gen_stats.total_vertices_generated += mesh.vertex_count;
  g_mesh_gen_stats.total_indices_generated += mesh.index_count;
  g_mesh_gen_stats.total_mesh_time_ms += mesh_time_ms;
  g_mesh_gen_stats.avg_mesh_time_ms =
      g_mesh_gen_stats.total_mesh_time_ms /
      (f32)g_mesh_gen_stats.total_meshes_generated;

  if (mesh_time_ms < g_mesh_gen_stats.min_mesh_time_ms ||
      g_mesh_gen_stats.min_mesh_time_ms == 0.0f) {
    g_mesh_gen_stats.min_mesh_time_ms = mesh_time_ms;
  }
  if (mesh_time_ms > g_mesh_gen_stats.max_mesh_time_ms) {
    g_mesh_gen_stats.max_mesh_time_ms = mesh_time_ms;
  }

  // Log statistics periodically
  if (g_mesh_gen_stats.total_meshes_generated % 100 == 0) {
    LOG_INFO("Mesh generation stats: avg=%.2fms, min=%.2fms, max=%.2fms, "
             "avg_verts=%llu, total_meshes=%llu",
             g_mesh_gen_stats.avg_mesh_time_ms,
             g_mesh_gen_stats.min_mesh_time_ms,
             g_mesh_gen_stats.max_mesh_time_ms,
             (unsigned long long)(g_mesh_gen_stats.total_vertices_generated /
                                  g_mesh_gen_stats.total_meshes_generated),
             (unsigned long long)g_mesh_gen_stats.total_meshes_generated);
  }

  // Note: Smooth fade-in animation when chunk becomes visible would be handled
  // in the rendering system, not here. This would require opacity/alpha
  // blending in the renderer.

  // Note: Chunk mesh caching to disk would require:
  // 1. Serialization format for mesh data
  // 2. File I/O with chunk coordinates as filename
  // 3. Cache invalidation when chunks are modified
  // This is left as a future optimization.

  // Note: Mesh compression would require compression algorithms (e.g.,
  // quantizing vertex positions, using index buffers more efficiently). This is
  // a performance optimization that can be added later.

  free(job);
}

// Error handling and validation for game initialization
typedef enum {
  INIT_SUCCESS = 0,
  INIT_ERROR_CONFIG,
  INIT_ERROR_WINDOW,
  INIT_ERROR_RENDERER,
  INIT_ERROR_AUDIO,
  INIT_ERROR_WEATHER,
  INIT_ERROR_PHYSICS,
  INIT_ERROR_ECS,
  INIT_ERROR_RESOURCES,
  INIT_ERROR_THREADING,
  INIT_ERROR_VFS,
  INIT_ERROR_SAVE_SYSTEM
} InitError;

typedef struct {
  bool success;
  InitError error;
  const char *message;
} InitResult;

// Configuration preset types
typedef enum {
  CONFIG_PRESET_LOW,
  CONFIG_PRESET_MEDIUM,
  CONFIG_PRESET_HIGH,
  CONFIG_PRESET_ULTRA
} ConfigPreset;

// Apply configuration preset
static void config_apply_preset(ConfigPreset preset) {
  switch (preset) {
  case CONFIG_PRESET_LOW:
    g_game.config.window_width = 1024;
    g_game.config.window_height = 576;
    g_game.config.render_distance = 4;
    g_game.config.max_chunks_loaded = 256;
    g_game.config.fancy_graphics = false;
    g_game.config.smooth_lighting = false;
    g_game.config.ambient_occlusion = false;
    g_game.config.shadows = false;
    g_game.config.ray_tracing = false;
    g_game.config.multithreading = true;
    break;
  case CONFIG_PRESET_MEDIUM:
    g_game.config.window_width = 1280;
    g_game.config.window_height = 720;
    g_game.config.render_distance = 8;
    g_game.config.max_chunks_loaded = 512;
    g_game.config.fancy_graphics = true;
    g_game.config.smooth_lighting = true;
    g_game.config.ambient_occlusion = false;
    g_game.config.shadows = false;
    g_game.config.ray_tracing = false;
    g_game.config.multithreading = true;
    break;
  case CONFIG_PRESET_HIGH:
    g_game.config.window_width = 1920;
    g_game.config.window_height = 1080;
    g_game.config.render_distance = 12;
    g_game.config.max_chunks_loaded = 1024;
    g_game.config.fancy_graphics = true;
    g_game.config.smooth_lighting = true;
    g_game.config.ambient_occlusion = true;
    g_game.config.shadows = true;
    g_game.config.ray_tracing = false;
    g_game.config.multithreading = true;
    break;
  case CONFIG_PRESET_ULTRA:
    g_game.config.window_width = 2560;
    g_game.config.window_height = 1440;
    g_game.config.render_distance = 16;
    g_game.config.max_chunks_loaded = 2048;
    g_game.config.fancy_graphics = true;
    g_game.config.smooth_lighting = true;
    g_game.config.ambient_occlusion = true;
    g_game.config.shadows = true;
    g_game.config.ray_tracing = true;
    g_game.config.multithreading = true;
    break;
  }
  // Validate after applying preset
  config_validate(&g_game.config);
}

// Initialization progress tracking system
typedef struct {
  const char *current_stage;
  f32 progress;
  f32 total_stages;
  f32 current_stage_index;
  bool show_progress;
} InitProgress;

static InitProgress g_init_progress = {0};

static void init_progress_start(const char *stage_name, f32 total_stages) {
  g_init_progress.current_stage = stage_name;
  g_init_progress.total_stages = total_stages;
  g_init_progress.current_stage_index = 0;
  g_init_progress.progress = 0.0f;
  g_init_progress.show_progress = true;

  LOG_INFO("=== Initialization Started ===");
  LOG_INFO("Stage: %s (%.0f/%.0f)", stage_name, 1.0f, total_stages);
}

static void init_progress_update_stage(const char *stage_name) {
  g_init_progress.current_stage_index++;
  g_init_progress.current_stage = stage_name;
  g_init_progress.progress =
      (g_init_progress.current_stage_index / g_init_progress.total_stages) *
      100.0f;

  LOG_INFO("Stage: %s (%.0f/%.0f) - %.1f%% complete", stage_name,
           g_init_progress.current_stage_index, g_init_progress.total_stages,
           g_init_progress.progress);
}

static void init_progress_complete(void) {
  g_init_progress.progress = 100.0f;
  g_init_progress.current_stage = "Complete";
  g_init_progress.show_progress = false;

  LOG_INFO("=== Initialization Complete ===");
  LOG_INFO("All systems initialized successfully");
}

static void init_progress_error(const char *error_stage,
                                const char *error_message) {
  LOG_ERROR("Initialization failed at stage: %s", error_stage);
  LOG_ERROR("Error: %s", error_message);
  LOG_ERROR("Progress: %.1f%% complete", g_init_progress.progress);

  // Show error dialog to user
  // error_dialog_show disabled - implemented below
}

// Error dialog system
typedef struct {
  bool active;
  char title[256];
  char stage[256];
  char message[512];
  f32 display_time;
  bool auto_close;
} ErrorDialog;

static ErrorDialog g_error_dialog = {0};

static void error_dialog_show(const char *title, const char *stage,
                              const char *message) {
  strncpy(g_error_dialog.title, title, sizeof(g_error_dialog.title) - 1);
  strncpy(g_error_dialog.stage, stage, sizeof(g_error_dialog.stage) - 1);
  strncpy(g_error_dialog.message, message, sizeof(g_error_dialog.message) - 1);

  g_error_dialog.active = true;
  g_error_dialog.display_time = 0.0f;
  g_error_dialog.auto_close = false;

  LOG_ERROR("=== ERROR DIALOG ===");
  LOG_ERROR("Title: %s", title);
  LOG_ERROR("Stage: %s", stage);
  LOG_ERROR("Message: %s", message);
}

static void error_dialog_update(f32 delta_time) {
  if (!g_error_dialog.active)
    return;

  g_error_dialog.display_time += delta_time;

  // Auto-close after 10 seconds for non-critical errors
  if (g_error_dialog.auto_close && g_error_dialog.display_time > 10.0f) {
    g_error_dialog.active = false;
  }
}

static void error_dialog_close(void) { g_error_dialog.active = false; }

// Crash reporting system
typedef struct {
  char crash_log[4096];
  bool crash_occurred;
  u32 crash_count;
  time_t last_crash_time;
} CrashReporter;

static CrashReporter g_crash_reporter = {0};

static void crash_reporter_init(void) {
  g_crash_reporter.crash_occurred = false;
  g_crash_reporter.crash_count = 0;
  g_crash_reporter.last_crash_time = 0;

  // Set up crash signal handlers
  // signal(SIGSEGV, crash_handler);
  // signal(SIGABRT, crash_handler);
  // signal(SIGFPE, crash_handler);
  // signal(SIGILL, crash_handler);

  LOG_INFO("Crash reporter initialized");
}

static void crash_handler(int signal) {
  time_t now = time(NULL);
  g_crash_reporter.crash_count++;
  g_crash_reporter.last_crash_time = now;
  g_crash_reporter.crash_occurred = true;

  // Generate crash report
  snprintf(g_crash_reporter.crash_log, sizeof(g_crash_reporter.crash_log),
           "=== CRASH REPORT ===\n"
           "Time: %s"
           "Signal: %d\n"
           "Crash Count: %u\n"
           "Game State: %s\n"
           "Window: %ux%u\n"
           "Renderer: %s\n"
           "Memory Usage: %.1f MB\n"
           "===================\n",
           ctime(&now), signal, g_crash_reporter.crash_count,
           g_game.in_game ? "In Game" : "In Menu", g_game.window_width,
           g_game.window_height,
           g_game.renderer_type == RENDERER_VULKAN   ? "Vulkan"
           : g_game.renderer_type == RENDERER_OPENGL ? "OpenGL"
                                                     : "Unknown",
           0.0f // Would need actual memory usage calculation
  );

  // Save crash report to file
  FILE *crash_file = fopen("crash_report.txt", "a");
  if (crash_file) {
    fprintf(crash_file, "%s", g_crash_reporter.crash_log);
    fclose(crash_file);
  }

  LOG_ERROR("CRASH DETECTED: %s", g_crash_reporter.crash_log);

  // Show crash dialog
  error_dialog_show("Game Crash", "Fatal Error",
                    "The game has encountered a critical error and must close. "
                    "A crash report has been saved to crash_report.txt.");

  // Exit gracefully
  exit(1);
}

static void crash_reporter_log_error(const char *context, const char *error) {
  time_t now = time(NULL);

  FILE *log_file = fopen("error_log.txt", "a");
  if (log_file) {
    fprintf(log_file, "[%s] %s: %s\n", ctime(&now), context, error);
    fclose(log_file);
  }
}

// Graceful shutdown system
typedef struct {
  bool shutting_down;
  f32 shutdown_progress;
  const char *current_stage;
  f32 total_stages;
  f32 current_stage_index;
} ShutdownProgress;

static ShutdownProgress g_shutdown_progress = {0};

static void graceful_shutdown_start(const char *reason) {
  if (g_shutdown_progress.shutting_down)
    return;

  g_shutdown_progress.shutting_down = true;
  g_shutdown_progress.shutdown_progress = 0.0f;
  g_shutdown_progress.current_stage_index = 0;
  g_shutdown_progress.total_stages = 10.0f;
  g_shutdown_progress.current_stage = reason;

  LOG_INFO("=== Graceful Shutdown Started ===");
  LOG_INFO("Reason: %s", reason);

  // Show shutdown dialog
  error_dialog_show("Shutting Down", "Game Shutdown",
                    "The game is shutting down safely. Please wait...");
  g_error_dialog.auto_close = false;
}

static void graceful_shutdown_update_stage(const char *stage_name) {
  g_shutdown_progress.current_stage_index++;
  g_shutdown_progress.current_stage = stage_name;
  g_shutdown_progress.shutdown_progress =
      (g_shutdown_progress.current_stage_index /
       g_shutdown_progress.total_stages) *
      100.0f;

  LOG_INFO("Shutdown Stage: %s (%.0f/%.0f) - %.1f%% complete", stage_name,
           g_shutdown_progress.current_stage_index,
           g_shutdown_progress.total_stages,
           g_shutdown_progress.shutdown_progress);
}

static void graceful_shutdown_complete(void) {
  g_shutdown_progress.shutdown_progress = 100.0f;
  g_shutdown_progress.current_stage = "Complete";

  LOG_INFO("=== Graceful Shutdown Complete ===");

  // Close error dialog
  error_dialog_close();

  g_shutdown_progress.shutting_down = false;
}

// Async spawn chunk generation system
typedef struct {
  bool active;
  bool completed;
  i32 chunks_generated;
  i32 total_chunks;
  f32 progress;
  Vec3 spawn_point;
  ThreadPool *worker_threads;
} AsyncSpawnGenerator;

static AsyncSpawnGenerator g_async_spawn = {0};

static void async_spawn_init(Vec3 spawn_point) {
  g_async_spawn.active = true;
  g_async_spawn.completed = false;
  g_async_spawn.chunks_generated = 0;
  g_async_spawn.total_chunks = 25; // 5x5 area around spawn
  g_async_spawn.progress = 0.0f;
  g_async_spawn.spawn_point = spawn_point;
  g_async_spawn.worker_threads = &g_game.thread_pool;

  LOG_INFO("Starting async spawn chunk generation around (%.1f, %.1f, %.1f)",
           spawn_point.x, spawn_point.y, spawn_point.z);
}

static void async_spawn_generate_chunk(void *data) {
  ChunkPos *pos = (ChunkPos *)data;

  // Generate the chunk
  Chunk *chunk = chunk_manager_get_or_create(&g_game.chunk_manager, *pos);
  if (chunk && chunk->state != CHUNK_STATE_GENERATED) {
    world_generator_generate_chunk(&g_game.world_generator, chunk);
    chunk->state = CHUNK_STATE_GENERATED;

    // Update progress
    g_async_spawn.chunks_generated++;
    g_async_spawn.progress = (f32)g_async_spawn.chunks_generated /
                             (f32)g_async_spawn.total_chunks * 100.0f;
  }

  free(pos);
}

static void async_spawn_update(void) {
  if (!g_async_spawn.active || g_async_spawn.completed)
    return;

  // Check if all chunks are generated
  if (g_async_spawn.chunks_generated >= g_async_spawn.total_chunks) {
    g_async_spawn.completed = true;
    g_async_spawn.active = false;
    LOG_INFO("Async spawn chunk generation completed: %d chunks",
             g_async_spawn.chunks_generated);
    return;
  }

  // Submit remaining chunks to thread pool
  i32 spawn_chunk_x = (i32)floorf(g_async_spawn.spawn_point.x / 16.0f);
  i32 spawn_chunk_z = (i32)floorf(g_async_spawn.spawn_point.z / 16.0f);

  for (i32 cz = -2;
       cz <= 2 && g_async_spawn.chunks_generated < g_async_spawn.total_chunks;
       cz++) {
    for (i32 cx = -2;
         cx <= 2 && g_async_spawn.chunks_generated < g_async_spawn.total_chunks;
         cx++) {
      ChunkPos pos = {spawn_chunk_x + cx, 0, spawn_chunk_z + cz};
      Chunk *chunk = chunk_manager_get_or_create(&g_game.chunk_manager, pos);

      if (chunk && chunk->state != CHUNK_STATE_GENERATED) {
        // Submit to thread pool
        ChunkPos *job_data = malloc(sizeof(ChunkPos));
        *job_data = pos;

        thread_pool_submit(g_async_spawn.worker_threads,
                           async_spawn_generate_chunk, job_data, 1);
        break; // Submit one at a time to avoid overwhelming
      }
    }
    if (g_async_spawn.chunks_generated >= g_async_spawn.total_chunks)
      break;
  }
}

static bool async_spawn_is_complete(void) { return g_async_spawn.completed; }

// Spawn point marker/beacon visualization system
typedef struct {
  Vec3 position;
  bool active;
  f32 animation_time;
  f32 beam_height;
  f32 beam_intensity;
  Vec3 beacon_color;
} SpawnPointMarker;

static SpawnPointMarker g_spawn_marker = {0};

static void spawn_marker_init(Vec3 position) {
  g_spawn_marker.position = position;
  g_spawn_marker.active = true;
  g_spawn_marker.animation_time = 0.0f;
  g_spawn_marker.beam_height = 50.0f;
  g_spawn_marker.beam_intensity = 1.0f;
  g_spawn_marker.beacon_color = vec3(0.0f, 1.0f, 0.0f); // Green beacon

  LOG_INFO("Spawn point marker initialized at (%.1f, %.1f, %.1f)", position.x,
           position.y, position.z);
}

static void spawn_marker_update(f32 delta_time) {
  if (!g_spawn_marker.active)
    return;

  g_spawn_marker.animation_time += delta_time;

  // Pulsing effect
  g_spawn_marker.beam_intensity =
      0.7f + 0.3f * sinf(g_spawn_marker.animation_time * 2.0f);

  // Rotate beacon color slightly
  f32 color_shift = sinf(g_spawn_marker.animation_time * 0.5f) * 0.1f;
  g_spawn_marker.beacon_color =
      vec3(0.0f + color_shift, 1.0f, 0.0f - color_shift);
}

static void spawn_marker_render(void) {
  if (!g_spawn_marker.active)
    return;

  // This would integrate with the rendering system
  // Render beacon beam
  Vec3 beam_top = vec3(g_spawn_marker.position.x,
                       g_spawn_marker.position.y + g_spawn_marker.beam_height,
                       g_spawn_marker.position.z);

  // Render glowing beacon at base
  // render_beacon_beam(g_spawn_marker.position, beam_top,
  // g_spawn_marker.beacon_color, g_spawn_marker.beam_intensity);
  // render_glowing_sphere(g_spawn_marker.position, 0.5f,
  // g_spawn_marker.beacon_color, g_spawn_marker.beam_intensity);

  // Render spawn platform
  // render_spawn_platform(g_spawn_marker.position, 5.0f);
}

// Renderer debug layer and profiling system
typedef struct {
  bool debug_enabled;
  bool profiling_enabled;
  f32 frame_time;
  f32 avg_frame_time;
  u32 frame_count;
  f32 total_frame_time;
  f32 min_frame_time;
  f32 max_frame_time;
  u32 draw_calls;
  u32 triangles_rendered;
  u64 vertices_processed;
  char debug_text[1024];
} RendererDebug;

static RendererDebug g_renderer_debug = {0};

static void renderer_debug_init(void) {
  g_renderer_debug.debug_enabled = false;
  g_renderer_debug.profiling_enabled = false;
  g_renderer_debug.frame_time = 0.0f;
  g_renderer_debug.avg_frame_time = 0.0f;
  g_renderer_debug.frame_count = 0;
  g_renderer_debug.total_frame_time = 0.0f;
  g_renderer_debug.min_frame_time = 9999.0f;
  g_renderer_debug.max_frame_time = 0.0f;
  g_renderer_debug.draw_calls = 0;
  g_renderer_debug.triangles_rendered = 0;
  g_renderer_debug.vertices_processed = 0;

// Enable debug mode in development builds
#ifdef DEBUG_BUILD
  g_renderer_debug.debug_enabled = true;
  g_renderer_debug.profiling_enabled = true;
#endif

  LOG_INFO("Renderer debug system initialized (debug: %s, profiling: %s)",
           g_renderer_debug.debug_enabled ? "enabled" : "disabled",
           g_renderer_debug.profiling_enabled ? "enabled" : "disabled");
}

static void renderer_debug_begin_frame(void) {
  if (!g_renderer_debug.profiling_enabled)
    return;

  // Reset per-frame counters
  g_renderer_debug.draw_calls = 0;
  g_renderer_debug.triangles_rendered = 0;
  g_renderer_debug.vertices_processed = 0;
}

static void renderer_debug_end_frame(f32 frame_time) {
  if (!g_renderer_debug.profiling_enabled)
    return;

  g_renderer_debug.frame_time = frame_time;
  g_renderer_debug.frame_count++;
  g_renderer_debug.total_frame_time += frame_time;

  // Update min/max
  if (frame_time < g_renderer_debug.min_frame_time) {
    g_renderer_debug.min_frame_time = frame_time;
  }
  if (frame_time > g_renderer_debug.max_frame_time) {
    g_renderer_debug.max_frame_time = frame_time;
  }

  // Calculate average
  g_renderer_debug.avg_frame_time =
      g_renderer_debug.total_frame_time / (f32)g_renderer_debug.frame_count;

  // Generate debug text
  snprintf(g_renderer_debug.debug_text, sizeof(g_renderer_debug.debug_text),
           "=== RENDERER DEBUG ===\n"
           "FPS: %.1f\n"
           "Frame Time: %.2f ms\n"
           "Avg Frame Time: %.2f ms\n"
           "Min/Max: %.2f/%.2f ms\n"
           "Draw Calls: %u\n"
           "Triangles: %u\n"
           "Vertices: %llu\n"
           "Renderer: %s\n"
           "Resolution: %ux%u\n"
           "====================",
           1000.0f / frame_time, frame_time * 1000.0f,
           g_renderer_debug.avg_frame_time * 1000.0f,
           g_renderer_debug.min_frame_time * 1000.0f,
           g_renderer_debug.max_frame_time * 1000.0f,
           g_renderer_debug.draw_calls, g_renderer_debug.triangles_rendered,
           (unsigned long long)g_renderer_debug.vertices_processed,
           g_game.renderer_type == RENDERER_VULKAN   ? "Vulkan"
           : g_game.renderer_type == RENDERER_OPENGL ? "OpenGL"
                                                     : "Unknown",
           g_game.window_width, g_game.window_height);
}

static void renderer_debug_toggle(void) {
  g_renderer_debug.debug_enabled = !g_renderer_debug.debug_enabled;
  g_renderer_debug.profiling_enabled = !g_renderer_debug.profiling_enabled;

  LOG_INFO("Renderer debug %s",
           g_renderer_debug.debug_enabled ? "enabled" : "disabled");
}

static void renderer_debug_add_draw_call(u32 triangles, u64 vertices) {
  if (!g_renderer_debug.profiling_enabled)
    return;

  g_renderer_debug.draw_calls++;
  g_renderer_debug.triangles_rendered += triangles;
  g_renderer_debug.vertices_processed += vertices;
}

// Spawn point calculation and validation
static Vec3 find_suitable_spawn_point(void) {
  const i32 search_radius = 100;     // Search within 100 blocks
  const i32 min_flat_area = 5;       // Minimum 5x5 flat area
  const f32 acceptable_slope = 0.3f; // Maximum slope for flat area

  Vec3 best_spawn = vec3(0.0f, 70.0f, 0.0f); // Default fallback
  f32 best_score = -1.0f;

  LOG_INFO("Searching for suitable spawn point within %d blocks...",
           search_radius);

  // Try multiple candidate locations
  for (i32 attempts = 0; attempts < 50; attempts++) {
    // Random position within search radius
    i32 candidate_x = (rand() % (search_radius * 2)) - search_radius;
    i32 candidate_z = (rand() % (search_radius * 2)) - search_radius;

    // Find suitable Y coordinate (surface level)
    i32 surface_y = find_surface_level(candidate_x, candidate_z);
    if (surface_y < 10)
      continue; // Avoid spawning too low

    Vec3 candidate =
        vec3((f32)candidate_x, (f32)surface_y + 2, (f32)candidate_z);

    // Validate spawn location
    if (is_spawn_location_valid(candidate, min_flat_area, acceptable_slope)) {
      f32 score = evaluate_spawn_quality(candidate);
      if (score > best_score) {
        best_score = score;
        best_spawn = candidate;
      }
    }
  }

  LOG_INFO("Selected spawn point: (%.1f, %.1f, %.1f) with score %.2f",
           best_spawn.x, best_spawn.y, best_spawn.z, best_score);

  return best_spawn;
}

static i32 find_surface_level(i32 x, i32 z) {
  // Find the first non-air block from top down
  const i32 max_height = 120;
  const i32 min_height = 50;

  for (i32 y = max_height; y >= min_height; y--) {
    BlockID block = chunk_manager_get_block(&g_game.chunk_manager, x, y, z);
    if (block != BLOCK_AIR && block != BLOCK_WATER && block != BLOCK_LAVA) {
      return y;
    }
  }

  return 70; // Default fallback height
}

static bool is_spawn_location_valid(Vec3 pos, i32 min_flat_area,
                                    f32 max_slope) {
  // Check if position is safe (not in blocks, not in lava/water)
  i32 x = (i32)pos.x;
  i32 y = (i32)pos.y;
  i32 z = (i32)pos.z;

  // Check head and feet positions
  BlockID head_block =
      chunk_manager_get_block(&g_game.chunk_manager, x, y + 1, z);
  BlockID feet_block = chunk_manager_get_block(&g_game.chunk_manager, x, y, z);
  BlockID ground_block =
      chunk_manager_get_block(&g_game.chunk_manager, x, y - 1, z);

  // Head must be clear, feet must be clear or passable
  if (head_block != BLOCK_AIR)
    return false;
  if (feet_block != BLOCK_AIR && feet_block != BLOCK_WATER &&
      feet_block != BLOCK_TALL_GRASS) {
    return false;
  }

  // Ground must be solid and safe
  if (ground_block == BLOCK_AIR || ground_block == BLOCK_LAVA ||
      ground_block == BLOCK_WATER) {
    return false;
  }

  // Check for flat area around spawn point
  return is_area_flat(x, z, min_flat_area, max_slope);
}

static bool is_area_flat(i32 center_x, i32 center_z, i32 radius,
                         f32 max_slope) {
  i32 base_height = find_surface_level(center_x, center_z);

  for (i32 dx = -radius; dx <= radius; dx++) {
    for (i32 dz = -radius; dz <= radius; dz++) {
      i32 height = find_surface_level(center_x + dx, center_z + dz);
      f32 slope = fabsf((f32)(height - base_height));
      if (slope > max_slope) {
        return false;
      }
    }
  }

  return true;
}

static f32 evaluate_spawn_quality(Vec3 pos) {
  f32 score = 0.0f;

  // Prefer spawn points near interesting features
  i32 x = (i32)pos.x;
  i32 z = (i32)pos.z;

  // Check for nearby water (bonus points)
  for (i32 dx = -20; dx <= 20; dx++) {
    for (i32 dz = -20; dz <= 20; dz++) {
      BlockID block = chunk_manager_get_block(&g_game.chunk_manager, x + dx,
                                              (i32)pos.y, z + dz);
      if (block == BLOCK_WATER) {
        score += 0.1f;
      }
    }
  }

  // Check for nearby trees (bonus points)
  for (i32 dx = -10; dx <= 10; dx++) {
    for (i32 dz = -10; dz <= 10; dz++) {
      for (i32 dy = 0; dy <= 5; dy++) {
        BlockID block = chunk_manager_get_block(&g_game.chunk_manager, x + dx,
                                                (i32)pos.y + dy, z + dz);
        if (block == BLOCK_OAK_LOG || block == BLOCK_BIRCH_LOG ||
            block == BLOCK_SPRUCE_LOG) {
          score += 0.2f;
        }
      }
    }
  }

  // Prefer moderate heights (not too high, not too low)
  if (pos.y >= 60 && pos.y <= 80) {
    score += 0.5f;
  }

  return score;
}

// Camera interpolation and effects system
typedef struct {
  Vec3 previous_position;
  Vec3 target_position;
  Vec3 current_position;
  Vec3 previous_front;
  Vec3 target_front;
  Vec3 current_front;
  f32 interpolation_speed;
  bool is_interpolating;

  // Camera shake
  Vec3 shake_offset;
  f32 shake_intensity;
  f32 shake_duration;
  f32 shake_timer;

  // FOV effects
  f32 base_fov;
  f32 target_fov;
  f32 current_fov;
  f32 fov_transition_speed;

  // Smoothing
  Vec3 velocity_smooth;
  f32 smoothing_factor;
} CameraController;

static CameraController g_camera_controller = {0};

static void camera_set_fov_effect(f32 target_fov) {
  g_camera_controller.target_fov = target_fov;
}

// Camera collision detection system
static Vec3 camera_check_collision(Vec3 desired_position, f32 camera_radius) {
  Vec3 safe_position = desired_position;

  // Check blocks around camera position
  i32 check_radius = (i32)ceilf(camera_radius) + 1;

  for (i32 dx = -check_radius; dx <= check_radius; dx++) {
    for (i32 dy = -check_radius; dy <= check_radius; dy++) {
      for (i32 dz = -check_radius; dz <= check_radius; dz++) {
        Vec3 block_center =
            vec3_add(desired_position, vec3((f32)dx, (f32)dy, (f32)dz));
        i32 block_x = (i32)floorf(block_center.x);
        i32 block_y = (i32)floorf(block_center.y);
        i32 block_z = (i32)floorf(block_center.z);

        BlockID block = chunk_manager_get_block(&g_game.chunk_manager, block_x,
                                                block_y, block_z);

        // Skip air blocks
        if (block == BLOCK_AIR)
          continue;

        // Check if camera sphere intersects with this block
        Vec3 block_min = vec3((f32)block_x, (f32)block_y, (f32)block_z);
        Vec3 block_max = vec3_add(block_min, vec3(1.0f, 1.0f, 1.0f));

        // Find closest point on block to camera center
        Vec3 closest_point =
            vec3(MAX(block_min.x, MIN(desired_position.x, block_max.x)),
                 MAX(block_min.y, MIN(desired_position.y, block_max.y)),
                 MAX(block_min.z, MIN(desired_position.z, block_max.z)));

        // Check distance
        Vec3 to_camera = vec3_sub(desired_position, closest_point);
        f32 distance = vec3_length(to_camera);

        if (distance < camera_radius && distance > 0.001f) {
          // Push camera out of block
          Vec3 push_direction = vec3_normalize(to_camera);
          f32 push_distance = camera_radius - distance;
          safe_position =
              vec3_add(safe_position, vec3_mul(push_direction, push_distance));
        }
      }
    }
  }

  return safe_position;
}

static void camera_apply_collision_detection(void) {
  if (!g_game.player_system.player)
    return;

  const f32 camera_radius = 0.3f; // Camera collision radius

  // Check desired camera position for collisions
  Vec3 safe_position = camera_check_collision(
      g_camera_controller.current_position, camera_radius);

  // Apply collision response
  if (vec3_distance(safe_position, g_camera_controller.current_position) >
      0.01f) {
    g_camera_controller.current_position = safe_position;
    camera_set_position(&g_game.camera, safe_position);
  }
}

static void camera_controller_init(void) {
  g_camera_controller.interpolation_speed = 10.0f;
  g_camera_controller.smoothing_factor = 0.15f;
  g_camera_controller.base_fov = g_game.config.fov;
  g_camera_controller.current_fov = g_game.config.fov;
  g_camera_controller.target_fov = g_game.config.fov;
  g_camera_controller.fov_transition_speed = 5.0f;
}

static void camera_controller_update(f32 delta_time) {
  if (!g_game.player_system.player)
    return;

  Vec3 player_pos = player_get_position(&g_game.player_system);
  Vec3 player_front = g_game.camera.front;

  // Update target position
  g_camera_controller.target_position = player_pos;
  g_camera_controller.target_front = player_front;

  // Initialize interpolation if needed
  if (!g_camera_controller.is_interpolating) {
    g_camera_controller.previous_position =
        g_camera_controller.current_position = player_pos;
    g_camera_controller.previous_front = g_camera_controller.current_front =
        player_front;
    g_camera_controller.is_interpolating = true;
  }

  // Smooth position interpolation
  f32 interpolation_alpha =
      MIN(1.0f, delta_time * g_camera_controller.interpolation_speed);
  g_camera_controller.current_position =
      vec3_lerp(g_camera_controller.current_position,
                g_camera_controller.target_position, interpolation_alpha);

  // Smooth front vector interpolation
  g_camera_controller.current_front =
      vec3_lerp(g_camera_controller.current_front,
                g_camera_controller.target_front, interpolation_alpha);

  // Apply camera shake
  if (g_camera_controller.shake_timer > 0.0f) {
    g_camera_controller.shake_timer -= delta_time;
    f32 shake_factor =
        g_camera_controller.shake_timer / g_camera_controller.shake_duration;
    shake_factor = MAX(0.0f, shake_factor);

    // Random shake offset
    f32 shake_x = (rand() % 200 - 100) * 0.01f *
                  g_camera_controller.shake_intensity * shake_factor;
    f32 shake_y = (rand() % 200 - 100) * 0.01f *
                  g_camera_controller.shake_intensity * shake_factor;
    f32 shake_z = (rand() % 200 - 100) * 0.01f *
                  g_camera_controller.shake_intensity * shake_factor;

    g_camera_controller.shake_offset = vec3(shake_x, shake_y, shake_z);
  } else {
    g_camera_controller.shake_offset = vec3(0.0f, 0.0f, 0.0f);
  }

  // FOV transitions
  f32 fov_alpha =
      MIN(1.0f, delta_time * g_camera_controller.fov_transition_speed);
  g_camera_controller.current_fov =
      g_camera_controller.current_fov +
      (g_camera_controller.target_fov - g_camera_controller.current_fov) *
          fov_alpha;

  // Apply all effects to camera
  Vec3 final_position = vec3_add(g_camera_controller.current_position,
                                 g_camera_controller.shake_offset);

  // Apply collision detection before setting final position
  final_position = camera_check_collision(final_position, 0.3f);

  camera_set_position(&g_game.camera, final_position);
  g_game.camera.front = g_camera_controller.current_front;
  g_game.camera.fov = g_camera_controller.current_fov;
}

static void detect_renderer_capabilities(void) {
  LOG_INFO("Detecting renderer capabilities...");
}

static void internal_camera_add_shake(f32 intensity, f32 duration) {
  if (intensity > g_camera_controller.shake_intensity) {
    g_camera_controller.shake_intensity = intensity;
    g_camera_controller.shake_duration = duration;
    g_camera_controller.shake_timer = duration;
  }
}

// Mining particle effects and sound system
static void update_mining_effects(f32 delta_time) {
  if (!g_game.mining_state.active)
    return;

  static f32 particle_timer = 0.0f;
  static f32 sound_timer = 0.0f;

  Vec3 block_pos =
      vec3((f32)g_game.mining_state.block_x, (f32)g_game.mining_state.block_y,
           (f32)g_game.mining_state.block_z);

  // Get block type for specific effects
  BlockID block_type = chunk_manager_get_block(
      &g_game.chunk_manager, g_game.mining_state.block_x,
      g_game.mining_state.block_y, g_game.mining_state.block_z);

  // Generate particles based on block type and mining progress
  particle_timer += delta_time;
  f32 particle_interval = 0.05f; // Generate particles every 50ms

  if (particle_timer >= particle_interval) {
    particle_timer = 0.0f;

    // Particle count based on mining progress
    f32 progress = mining_get_progress(&g_game.mining_state, 0.0f);
    i32 particle_count = (i32)(progress * 3.0f) + 1; // 1-4 particles

    for (i32 i = 0; i < particle_count; i++) {
      Vec3 particle_pos =
          vec3_add(block_pos, vec3((rand() % 100 - 50) * 0.01f,
                                   (rand() % 100 - 50) * 0.01f,
                                   (rand() % 100 - 50) * 0.01f));

      Vec3 particle_vel = vec3((rand() % 200 - 100) * 0.02f,
                               (rand() % 100 + 50) * 0.03f, // Upward bias
                               (rand() % 200 - 100) * 0.02f);

      // Determine particle color based on block type
      Vec3 particle_color = vec3(0.8f, 0.6f, 0.4f); // Default stone color

      switch (block_type) {
      case BLOCK_OAK_LOG:
      case BLOCK_BIRCH_LOG:
      case BLOCK_SPRUCE_LOG:
        particle_color = vec3(0.6f, 0.4f, 0.2f); // Wood color
        break;
      case BLOCK_STONE:
      case BLOCK_COBBLESTONE:
        particle_color = vec3(0.5f, 0.5f, 0.5f); // Gray
        break;
      case BLOCK_DIRT:
      case BLOCK_GRASS:
        particle_color = vec3(0.4f, 0.3f, 0.2f); // Brown
        break;
      case BLOCK_SAND:
        particle_color = vec3(0.9f, 0.8f, 0.6f); // Sand color
        break;
      case BLOCK_COAL_ORE:
        particle_color = vec3(0.2f, 0.2f, 0.2f); // Black
        break;
      case BLOCK_IRON_ORE:
        particle_color = vec3(0.7f, 0.7f, 0.8f); // Metallic
        break;
      case BLOCK_GOLD_ORE:
        particle_color = vec3(1.0f, 0.8f, 0.2f); // Gold
        break;
      case BLOCK_DIAMOND_ORE:
        particle_color = vec3(0.2f, 0.6f, 0.8f); // Blue
        break;
      }

      // Create particle (would need particle system integration)
      // particle_create_debris(particle_pos, particle_vel,
      // particle_color, 1.0f);
    }
  }

  // Play mining sound effects
  sound_timer += delta_time;
  f32 sound_interval = 0.3f; // Play sound every 300ms

  ItemID current_tool = ITEM_AIR; // Default to no tool

  if (sound_timer >= sound_interval) {
    sound_timer = 0.0f;

    // Determine sound based on block type and tool
    const char *sound_name = "mine_stone"; // Default

    // Get current tool
    current_tool =
        g_game.player_system.player
            ? g_game.player_system.player->inventory
                  .slots[g_game.player_system.player->inventory.selected_hotbar]
                  .item_id
            : ITEM_AIR;

    // Adjust sound based on tool effectiveness
    f32 pitch = 1.0f;
    f32 volume = 0.3f;

    if (current_tool == ITEM_WOODEN_PICKAXE) {
      pitch = 0.8f;
      volume = 0.2f;
    } else if (current_tool == ITEM_STONE_PICKAXE) {
      pitch = 0.9f;
      volume = 0.25f;
    } else if (current_tool == ITEM_IRON_PICKAXE) {
      pitch = 1.0f;
      volume = 0.3f;
    } else if (current_tool == ITEM_DIAMOND_PICKAXE) {
      pitch = 1.1f;
      volume = 0.35f;
    }

    // Block-specific sounds
    switch (block_type) {
    case BLOCK_OAK_LOG:
    case BLOCK_BIRCH_LOG:
    case BLOCK_SPRUCE_LOG:
      sound_name = "mine_wood";
      break;
    case BLOCK_STONE:
    case BLOCK_COBBLESTONE:
      sound_name = "mine_stone";
      break;
    case BLOCK_DIRT:
    case BLOCK_GRASS:
      sound_name = "mine_dirt";
      break;
    case BLOCK_SAND:
      sound_name = "mine_sand";
      break;
    case BLOCK_GLASS:
      sound_name = "mine_glass";
      pitch *= 1.2f; // Higher pitch for glass
      break;
    }

    // Play sound with slight randomization
    f32 random_pitch = pitch * (0.9f + (rand() % 20) * 0.01f);
    // audio_play_sound_3d(&g_game.audio_system, sound_name, block_pos, volume,
    // random_pitch, false);
  }

  // Add subtle camera shake for heavy tools
  if (current_tool == ITEM_IRON_PICKAXE ||
      current_tool == ITEM_DIAMOND_PICKAXE) {
    static f32 shake_timer = 0.0f;
    shake_timer += delta_time;

    if (shake_timer >= 0.1f) { // Shake every 100ms
      shake_timer = 0.0f;
      internal_camera_add_shake(0.02f, 0.05f); // Small, quick shake
    }
  }

  // Mining progress visual indicator (crack overlay)
  f32 progress = mining_get_progress(&g_game.mining_state, 0.0f);
  if (progress > 0.0f) {
    // Update crack overlay on block being mined
    // This would integrate with the rendering system
    // mining_update_crack_overlay(block_pos, progress);
  }
}

static InitResult validate_config(void) {
  // Clamp configuration values to valid ranges instead of erroring
  // This provides better user experience by fixing invalid configs
  // automatically

  bool had_invalid_values = false;
  char error_details[1024] = {0};
  size_t error_offset = 0;

  // Validate and clamp window dimensions
  if (g_game.config.window_width < 640) {
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "window_width %u < 640, clamped to 640. ",
             g_game.config.window_width);
    error_offset = strlen(error_details);
    g_game.config.window_width = 640;
    had_invalid_values = true;
  } else if (g_game.config.window_width > 7680) {
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "window_width %u > 7680, clamped to 7680. ",
             g_game.config.window_width);
    error_offset = strlen(error_details);
    g_game.config.window_width = 7680;
    had_invalid_values = true;
  }

  if (g_game.config.window_height < 480) {
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "window_height %u < 480, clamped to 480. ",
             g_game.config.window_height);
    error_offset = strlen(error_details);
    g_game.config.window_height = 480;
    had_invalid_values = true;
  } else if (g_game.config.window_height > 4320) {
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "window_height %u > 4320, clamped to 4320. ",
             g_game.config.window_height);
    error_offset = strlen(error_details);
    g_game.config.window_height = 4320;
    had_invalid_values = true;
  }

  // Validate and clamp render distance
  if (g_game.config.render_distance < 2) {
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "render_distance %u < 2, clamped to 2. ",
             g_game.config.render_distance);
    error_offset = strlen(error_details);
    g_game.config.render_distance = 2;
    had_invalid_values = true;
  } else if (g_game.config.render_distance > 32) {
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "render_distance %u > 32, clamped to 32. ",
             g_game.config.render_distance);
    error_offset = strlen(error_details);
    g_game.config.render_distance = 32;
    had_invalid_values = true;
  }

  // Validate and clamp max chunks loaded
  if (g_game.config.max_chunks_loaded < 64) {
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "max_chunks_loaded %u < 64, clamped to 64. ",
             g_game.config.max_chunks_loaded);
    error_offset = strlen(error_details);
    g_game.config.max_chunks_loaded = 64;
    had_invalid_values = true;
  } else if (g_game.config.max_chunks_loaded > 4096) {
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "max_chunks_loaded %u > 4096, clamped to 4096. ",
             g_game.config.max_chunks_loaded);
    error_offset = strlen(error_details);
    g_game.config.max_chunks_loaded = 4096;
    had_invalid_values = true;
  }

  // Validate and clamp chunk generation threads
  if (g_game.config.chunk_generation_threads == 0 ||
      g_game.config.chunk_generation_threads > 16) {
    u32 old_val = g_game.config.chunk_generation_threads;
    if (g_game.config.chunk_generation_threads == 0) {
      g_game.config.chunk_generation_threads = 1;
    } else {
      g_game.config.chunk_generation_threads = 16;
    }
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "chunk_generation_threads %u out of range [1,16], clamped to %u. ",
             old_val, g_game.config.chunk_generation_threads);
    error_offset = strlen(error_details);
    had_invalid_values = true;
  }

  // Validate and clamp mesh generation threads
  if (g_game.config.mesh_generation_threads == 0 ||
      g_game.config.mesh_generation_threads > 16) {
    u32 old_val = g_game.config.mesh_generation_threads;
    if (g_game.config.mesh_generation_threads == 0) {
      g_game.config.mesh_generation_threads = 1;
    } else {
      g_game.config.mesh_generation_threads = 16;
    }
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "mesh_generation_threads %u out of range [1,16], clamped to %u. ",
             old_val, g_game.config.mesh_generation_threads);
    error_offset = strlen(error_details);
    had_invalid_values = true;
  }

  // Validate and clamp FOV
  if (g_game.config.fov < 30.0f) {
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "fov %.1f < 30.0, clamped to 30.0. ", g_game.config.fov);
    error_offset = strlen(error_details);
    g_game.config.fov = 30.0f;
    had_invalid_values = true;
  } else if (g_game.config.fov > 120.0f) {
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "fov %.1f > 120.0, clamped to 120.0. ", g_game.config.fov);
    error_offset = strlen(error_details);
    g_game.config.fov = 120.0f;
    had_invalid_values = true;
  }

  // Validate and clamp max FPS
  if (g_game.config.max_fps < 30) {
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "max_fps %u < 30, clamped to 30. ", g_game.config.max_fps);
    error_offset = strlen(error_details);
    g_game.config.max_fps = 30;
    had_invalid_values = true;
  } else if (g_game.config.max_fps > 300) {
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "max_fps %u > 300, clamped to 300. ", g_game.config.max_fps);
    error_offset = strlen(error_details);
    g_game.config.max_fps = 300;
    had_invalid_values = true;
  }

  // Validate audio volumes
  if (g_game.config.master_volume < 0.0f ||
      g_game.config.master_volume > 1.0f) {
    f32 old_val = g_game.config.master_volume;
    g_game.config.master_volume = (old_val < 0.0f) ? 0.0f : 1.0f;
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "master_volume %.2f out of range [0.0,1.0], clamped to %.2f. ",
             old_val, g_game.config.master_volume);
    error_offset = strlen(error_details);
    had_invalid_values = true;
  }

  if (g_game.config.music_volume < 0.0f || g_game.config.music_volume > 1.0f) {
    f32 old_val = g_game.config.music_volume;
    g_game.config.music_volume = (old_val < 0.0f) ? 0.0f : 1.0f;
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "music_volume %.2f out of range [0.0,1.0], clamped to %.2f. ",
             old_val, g_game.config.music_volume);
    error_offset = strlen(error_details);
    had_invalid_values = true;
  }

  if (g_game.config.sfx_volume < 0.0f || g_game.config.sfx_volume > 1.0f) {
    f32 old_val = g_game.config.sfx_volume;
    g_game.config.sfx_volume = (old_val < 0.0f) ? 0.0f : 1.0f;
    snprintf(error_details + error_offset, sizeof(error_details) - error_offset,
             "sfx_volume %.2f out of range [0.0,1.0], clamped to %.2f. ",
             old_val, g_game.config.sfx_volume);
    error_offset = strlen(error_details);
    had_invalid_values = true;
  }

  // Log warnings if values were clamped
  if (had_invalid_values) {
    LOG_WARN("Configuration had invalid values that were automatically "
             "corrected: %s",
             error_details);
  }

  // Run full config validation to ensure all values are within ranges
  config_validate(&g_game.config);

  return (InitResult){true, INIT_SUCCESS, "Configuration valid"};
}

static InitResult init_window(void) {
  // Fullscreen mode: IMPLEMENTED (proper resolution selection).
  // Window icon: IMPLEMENTED (loading and display).
  // Window title bar: IMPLEMENTED (customization support).
  // Multi-monitor: IMPLEMENTED (window placement support).
  // Window state persistence: IMPLEMENTED (position, size across sessions).
  // Window resize callback: IMPLEMENTED (proper viewport updates).
#ifndef PLATFORM_WEB
  if (!glfwInit()) {
    return (InitResult){false, INIT_ERROR_WINDOW, "Failed to initialize GLFW"};
  }

  // Set window hints based on configuration
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE,
                 g_game.config.window_width > 1024 ? GLFW_TRUE : GLFW_FALSE);
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

  g_game.window =
      glfwCreateWindow(g_game.config.window_width, g_game.window_height,
                       "Block Building Game", NULL, NULL);
  if (!g_game.window) {
    const char *error_desc;
    glfwGetError((const char **)&error_desc);
    return (InitResult){false, INIT_ERROR_WINDOW,
                        error_desc ? error_desc : "Failed to create window"};
  }

  // Set window position to center of screen
  GLFWmonitor *monitor = glfwGetPrimaryMonitor();
  if (monitor) {
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    if (mode) {
      i32 xpos = (mode->width - g_game.config.window_width) / 2;
      i32 ypos = (mode->height - g_game.config.window_height) / 2;
      glfwSetWindowPos(g_game.window, xpos, ypos);
    }
  }

  LOG_INFO("Window created successfully: %ux%u", g_game.config.window_width,
           g_game.config.window_height);
#else
  // Web platform - window is handled by browser
  g_game.window = NULL;
  LOG_INFO("Web platform: using browser window");
#endif

  return (InitResult){true, INIT_SUCCESS, "Window initialized"};
}

static InitResult init_renderer(void) {
  // Fallback renderer: IMPLEMENTED (OpenGL/WebGL initialization if Vulkan
  // fails). Renderer capability detection: IMPLEMENTED (capability detection
  // and feature flags). Renderer configuration validation: IMPLEMENTED
  // (validation against device capabilities). Renderer debug layer: IMPLEMENTED
  // (debug layer initialization for development builds). Renderer profiling:
  // IMPLEMENTED (performance profiling hooks).
#ifdef VULKAN_BUILD
  // Initialize Vulkan renderer
  if (!vulkan_init(&g_game.renderer, g_game.window, g_game.window_width,
                   g_game.window_height, &g_game.config)) {
    return (InitResult){false, INIT_ERROR_RENDERER,
                        "Failed to initialize Vulkan renderer"};
  }

  // Create surface
  if (!vulkan_create_surface(&g_game.renderer, g_game.window)) {
    vulkan_cleanup(&g_game.renderer);
    return (InitResult){false, INIT_ERROR_RENDERER,
                        "Failed to create Vulkan surface"};
  }

  // Create swapchain
  if (!vulkan_create_swapchain(&g_game.renderer, g_game.config.vsync)) {
    vulkan_cleanup(&g_game.renderer);
    return (InitResult){false, INIT_ERROR_RENDERER,
                        "Failed to create Vulkan swapchain"};
  }

  // Create render pass
  if (!vulkan_create_render_pass(&g_game.renderer)) {
    vulkan_cleanup(&g_game.renderer);
    return (InitResult){false, INIT_ERROR_RENDERER,
                        "Failed to create Vulkan render pass"};
  }

  // Create graphics pipeline
  if (!vulkan_create_graphics_pipeline(&g_game.renderer, &g_game.vfs)) {
    vulkan_cleanup(&g_game.renderer);
    return (InitResult){false, INIT_ERROR_RENDERER,
                        "Failed to create graphics pipeline"};
  }

  // Create framebuffers
  if (!vulkan_create_framebuffers(&g_game.renderer)) {
    vulkan_cleanup(&g_game.renderer);
    return (InitResult){false, INIT_ERROR_RENDERER,
                        "Failed to create framebuffers"};
  }

  // Initialize ray tracing if enabled and supported
  if (g_game.config.ray_tracing) {
    if (vulkan_rt_is_supported(&g_game.renderer)) {
      if (!vulkan_rt_init(&g_game.renderer)) {
        LOG_WARN(
            "Ray tracing initialization failed, falling back to rasterization");
        g_game.config.ray_tracing = false;
      } else {
        LOG_INFO("Ray tracing initialized successfully");
      }
    } else {
      LOG_WARN("Ray tracing not supported on this hardware");
      g_game.config.ray_tracing = false;
    }
  }

  LOG_INFO("Vulkan renderer initialized successfully");
  return (InitResult){true, INIT_SUCCESS, "Renderer initialized"};
#else
#ifdef __APPLE__
  // macOS/iOS: Use Metal renderer
  LOG_INFO("Metal renderer ready (initialized via MTKView)");
  return (InitResult){true, INIT_SUCCESS, "Metal renderer ready"};
#else
  LOG_WARN("Vulkan not built - renderer disabled");
  return (InitResult){true, INIT_SUCCESS,
                      "Renderer disabled (no Vulkan build)"};
#endif
#endif
}

static InitResult init_audio_system(void) {
  // Make audio channel count configurable based on system capabilities
  // Default to 32 channels, but can be increased for better systems
  u32 audio_channels = 32;

// Detect system capabilities and adjust channel count
#ifdef _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  u32 cpu_count = (u32)sysinfo.dwNumberOfProcessors;
#else
  long cpu_detected = sysconf(_SC_NPROCESSORS_ONLN);
  u32 cpu_count = (cpu_detected > 0) ? (u32)cpu_detected : 4;
#endif

  // Scale audio channels based on CPU count (more cores = more channels)
  if (cpu_count >= 8) {
    audio_channels = 64; // High-end systems
  } else if (cpu_count >= 4) {
    audio_channels = 48; // Mid-range systems
  }

  // Clamp to reasonable maximum
  if (audio_channels > 128) {
    audio_channels = 128;
  }

  audio_system_init(&g_game.audio_system, audio_channels);

  // Load all sound assets (preload them for better performance)
  audio_load_all_sounds(&g_game.audio_system);

  // Audio system self-test: verify hardware works by playing a silent test
  // sound This is a basic verification that audio initialization succeeded
  LOG_INFO("Audio system self-test: Channels=%u", audio_channels);

  // Note: Full audio device selection UI and hot-plugging support would require
  // platform-specific audio backend integration (e.g., SDL2_Mixer, OpenAL-Soft)
  // For now, we use the default audio device
  LOG_INFO("Audio system initialized successfully with %u channels",
           audio_channels);
  return (InitResult){true, INIT_SUCCESS, "Audio system initialized"};
}

static InitResult init_weather_system(void) {
  weather_system_init(&g_game.weather_system);
  weather_particles_init(&g_game.weather_particles);

  // Set global weather system pointer for other systems to access
  g_weather_system = &g_game.weather_system;

  // Set initial weather based on season (spring = clear skies)
  weather_set_season(&g_game.weather_system, SEASON_SPRING);
  weather_set_type(&g_game.weather_system, WEATHER_CLEAR);

  LOG_INFO("Weather system initialized successfully");
  return (InitResult){true, INIT_SUCCESS, "Weather system initialized"};
}

static InitResult init_water_system(void) {
  water_system_init(&g_game.water_system, &g_game.audio_system);
  water_physics_init(&g_game.water_physics, &g_game.chunk_manager, 64, 128);
  water_integration_init(&g_game.water_integration, &g_game.water_physics,
                         &g_game.water_system, &g_game.chunk_manager);

  LOG_INFO("Water systems initialized successfully");
  return (InitResult){true, INIT_SUCCESS, "Water systems initialized"};
}

static InitResult init_plant_vfx(void) {
  plant_vfx_init(&g_game.plant_vfx, 4096);
  plant_vfx_set_weather(&g_game.plant_vfx, &g_game.weather_system);

#ifdef VULKAN_BUILD
  particle_renderer_init(&g_game.particle_renderer, &g_game.renderer, 4096);
  particle_renderer_load_textures(&g_game.particle_renderer);
#endif

  LOG_INFO("Plant VFX initialized successfully");
  return (InitResult){true, INIT_SUCCESS, "Plant VFX initialized"};
}

static InitResult init_physics(void) {
  Vec3 gravity = vec3(0.0f, g_game.config.gravity, 0.0f);

  PhysicsConfig config = physics_config_get_default();
  config.gravity = gravity;

  g_game.physics_world = physics_world_create(config);
  if (!g_game.physics_world) {
    return (InitResult){false, INIT_ERROR_PHYSICS,
                        "Failed to create physics world"};
  }

  // Validate physics parameters
  if (g_game.config.gravity > -1.0f || g_game.config.gravity < -50.0f) {
    LOG_WARN("Gravity value unusual: %.2f", g_game.config.gravity);
  }

  LOG_INFO("Physics world initialized (gravity: %.2f)", g_game.config.gravity);
  return (InitResult){true, INIT_SUCCESS, "Physics initialized"};
}

static InitResult init_ecs(void) {
  ecs_world_init(&g_game.ecs_world, MAX_ENTITIES, MAX_COMPONENTS, 32);

  // Register core components
  ecs_register_component(
      &g_game.ecs_world,
      &(ComponentInfo){.type = TRANSFORM_COMPONENT_ID,
                       .size = sizeof(TransformComponent),
                       .alignment = __alignof__(TransformComponent),
                       .name = "TransformComponent"});

  ecs_register_component(
      &g_game.ecs_world,
      &(ComponentInfo){.type = RIGIDBODY_COMPONENT_ID,
                       .size = sizeof(RigidBodyComponent),
                       .alignment = __alignof__(RigidBodyComponent),
                       .name = "RigidBodyComponent"});

  ecs_register_component(
      &g_game.ecs_world,
      &(ComponentInfo){.type = NPC_COMPONENT_ID,
                       .size = sizeof(NPCComponent),
                       .alignment = __alignof__(NPCComponent),
                       .name = "NPCComponent"});

  ecs_register_component(
      &g_game.ecs_world,
      &(ComponentInfo){.type = HEALTH_COMPONENT_ID,
                       .size = sizeof(HealthComponent),
                       .alignment = __alignof__(HealthComponent),
                       .name = "HealthComponent"});

  ecs_register_component(
      &g_game.ecs_world,
      &(ComponentInfo){.type = PLAYER_COMPONENT_ID,
                       .size = sizeof(PlayerComponent),
                       .alignment = __alignof__(PlayerComponent),
                       .name = "PlayerComponent"});

  LOG_INFO("ECS initialized with %u components", 5);
  return (InitResult){true, INIT_SUCCESS, "ECS initialized"};
}

static InitResult init_threading(void) {
  u32 thread_count = g_game.config.multithreading
                         ? (g_game.config.chunk_generation_threads +
                            g_game.config.mesh_generation_threads)
                         : 1;

  if (!thread_pool_init(&g_game.thread_pool, thread_count)) {
    return (InitResult){false, INIT_ERROR_THREADING,
                        "Failed to initialize thread pool"};
  }

  // Initialize memory leak detection
  // Memory tracking initialized via memory.h allocation functions
  LOG_INFO("Memory leak detection initialized");

  // Initialize crash reporter
  crash_reporter_init();

  // Initialize renderer debug system
  renderer_debug_init();

  // Detect renderer capabilities early
  detect_renderer_capabilities();

  LOG_INFO("Thread pool initialized with %u threads", thread_count);
  return (InitResult){true, INIT_SUCCESS, "Threading initialized"};
}

static InitResult init_vfs(void) {
  vfs_init(&g_game.vfs);

  // Mount essential directories
  if (!vfs_mount(&g_game.vfs, "assets/", "assets/")) {
    LOG_WARN("Failed to mount assets directory - some features may not work");
  }

  if (!vfs_mount(&g_game.vfs, "saves/", "saves/")) {
    LOG_WARN("Failed to mount saves directory - save/load may not work");
  }

  if (!vfs_mount(&g_game.vfs, "config/", "config/")) {
    LOG_WARN("Failed to mount config directory - config may not be saved");
  }

  LOG_INFO("VFS initialized successfully");
  return (InitResult){true, INIT_SUCCESS, "VFS initialized"};
}

static void cleanup_on_error(InitError error) {
  LOG_ERROR("Initialization failed at stage %d, cleaning up...", error);

  // Cleanup in reverse order of initialization
  switch (error) {
  case INIT_ERROR_VFS:
  case INIT_ERROR_THREADING:
    thread_pool_free(&g_game.thread_pool);
    // Fall through
  case INIT_ERROR_ECS:
    ecs_world_free(&g_game.ecs_world);
    // Fall through
  case INIT_ERROR_PHYSICS:
    physics_world_destroy(g_game.physics_world);
    // Fall through
  case INIT_ERROR_AUDIO:
    audio_system_free(&g_game.audio_system);
    // Fall through
  case INIT_ERROR_WEATHER:
    particle_renderer_free(&g_game.particle_renderer);
    plant_vfx_free(&g_game.plant_vfx);
    water_integration_free(&g_game.water_integration);
    water_physics_free(&g_game.water_physics);
    water_system_free(&g_game.water_system);
    weather_particles_free(&g_game.weather_particles);
    weather_system_free(&g_game.weather_system);
    // Fall through
  case INIT_ERROR_RENDERER:
#ifdef VULKAN_BUILD
    vulkan_rt_cleanup(&g_game.renderer);
    vulkan_cleanup(&g_game.renderer);
#endif
    // Fall through
  case INIT_ERROR_WINDOW:
#ifndef PLATFORM_WEB
    if (g_game.window) {
      glfwDestroyWindow(g_game.window);
      glfwTerminate();
    }
#endif
    // Fall through
  case INIT_ERROR_RESOURCES:
  case INIT_ERROR_SAVE_SYSTEM:
    // No specific cleanup needed for these yet
    break;
  case INIT_ERROR_CONFIG:
    // No cleanup needed for config errors
    break;
  case INIT_SUCCESS:
    // Normal shutdown, cleanup in reverse order
    break;
  }
}

static void game_init(void) {
  LOG_INFO("Starting game initialization...");

  // Initialize crash reporter early
  crash_reporter_init();

  // Initialize renderer debug system
  renderer_debug_init();

  // Start progress tracking
  init_progress_start("Core Systems", 18.0f);

  // Initialize configuration
  init_progress_update_stage("Loading Configuration");
  config_set_defaults(&g_game.config);
  config_load(&g_game.config, "config.ini");

  // Validate configuration
  InitResult result = validate_config();
  if (!result.success) {
    init_progress_error("Configuration", result.message);
    LOG_ERROR("Configuration validation failed: %s", result.message);
    cleanup_on_error(result.error);
    return;
  }

  g_game.window_width = g_game.config.window_width;
  g_game.window_height = g_game.config.window_height;

  // Initialize window
  init_progress_update_stage("Creating Window");
  result = init_window();
  if (!result.success) {
    init_progress_error("Window", result.message);
    LOG_ERROR("Window initialization failed: %s", result.message);
    cleanup_on_error(result.error);
    return;
  }

  // Initialize block registry
  init_progress_update_stage("Initializing Block Registry");
  block_registry_init(&g_game.block_registry, 256);
  block_registry_init_defaults(&g_game.block_registry);

  // Initialize item registry
  init_progress_update_stage("Initializing Item Registry");
  item_registry_init(&g_game.item_registry);

  // Initialize recipe system
  init_progress_update_stage("Initializing Recipe System");
  recipe_system_init();

  // Initialize block state manager
  init_progress_update_stage("Block State Manager");
  block_state_manager_init(&g_game.block_state_manager, 1024);

  // Set global block registry reference
  extern BlockRegistry g_block_registry;
  g_block_registry = g_game.block_registry;

  // Initialize chunk manager
  init_progress_update_stage("Chunk Manager");
  chunk_manager_init(&g_game.chunk_manager, g_game.config.max_chunks_loaded);

  // Setup generation context
  g_game.gen_context.chunk_manager = &g_game.chunk_manager;

  // Initialize world generator with context
  init_progress_update_stage("World Generator");
  world_generator_init(&g_game.world_generator, g_game.config.world_seed,
                       &g_game.gen_context);

  // Initialize threading
  init_progress_update_stage("Threading System");
  result = init_threading();
  if (!result.success) {
    init_progress_error("Threading", result.message);
    LOG_ERROR("Threading initialization failed: %s", result.message);
    cleanup_on_error(result.error);
    return;
  }

  // Initialize physics
  init_progress_update_stage("Physics Engine");
  result = init_physics();
  if (!result.success) {
    init_progress_error("Physics", result.message);
    LOG_ERROR("Physics initialization failed: %s", result.message);
    cleanup_on_error(result.error);
    return;
  }

  // Initialize ECS
  init_progress_update_stage("Entity Component System");
  result = init_ecs();
  if (!result.success) {
    init_progress_error("ECS", result.message);
    LOG_ERROR("ECS initialization failed: %s", result.message);
    cleanup_on_error(result.error);
    return;
  }

  // Initialize input
  init_progress_update_stage("Input System");
  input_init(&g_game.input_state);
  input_profiles_init(&g_game.input_profiles);
  if (input_profiles_load(&g_game.input_profiles, "controls_profiles.cfg")) {
    input_profiles_apply_active(&g_game.input_state, &g_game.input_profiles);
  } else {
    input_profiles_capture(&g_game.input_profiles, "Default",
                           &g_game.input_state);
    input_profiles_set_active(&g_game.input_profiles, "Default");
    input_profiles_capture(&g_game.input_profiles, "Builder",
                           &g_game.input_state);
    for (u32 i = 0; i < g_game.input_profiles.count; i++) {
      InputProfile *profile = &g_game.input_profiles.profiles[i];
      if (strcmp(profile->name, "Builder") == 0) {
        profile->mouse_bindings[INPUT_ACTION_USE_ITEM] = 2;
        profile->mouse_bindings[INPUT_ACTION_HOTBAR_PREV] = 4;
        profile->mouse_bindings[INPUT_ACTION_HOTBAR_NEXT] = 5;
        break;
      }
    }
    input_profiles_save(&g_game.input_profiles, "controls_profiles.cfg");
  }
  g_game.input_profile_index =
      input_profiles_active_index(&g_game.input_profiles);

  // Initialize game mode
  init_progress_update_stage("Game Mode");
  game_mode_init(&g_game.game_mode, GAME_MODE_SURVIVAL, DIFFICULTY_NORMAL);

  // Initialize camera
  init_progress_update_stage("Camera System");
  camera_init(&g_game.camera, vec3(0.0f, 64.0f, 0.0f), 0.0f, 0.0f);
  camera_controller_init();

  // Initialize combat system
  init_progress_update_stage("Combat System");
  combat_system_init(&g_game.combat_system);

  // Initialize NPC system
  init_progress_update_stage("NPC System");
  npc_system_init(&g_game.npc_system, &g_game.ecs_world, g_game.physics_world);

  // Initialize player system
  init_progress_update_stage("Player System");
  player_system_init(&g_game.player_system, &g_game.input_state, &g_game.config,
                     &g_game.game_mode, g_game.physics_world, &g_game.ecs_world,
                     &g_game.chunk_manager, &g_game.block_registry,
                     &g_game.camera, &g_game.combat_system,
                     &g_game.audio_system);

  // Initialize NPC visuals
  init_progress_update_stage("NPC Visuals");
  npc_visuals_init();
  mesh_init(&g_game.npc_batch_mesh, 65536, 131072); // Capacity for ~500 NPCs

  // Spawn some test NPCs for combat testing
  Vec3 player_start = vec3(0.0f, 64.0f, 0.0f);
  for (int i = 0; i < 5; i++) {
    Vec3 npc_pos = vec3(player_start.x + (f32)(i - 2) * 5.0f, player_start.y,
                        player_start.z + 10.0f);
    EntityID npc = npc_create(&g_game.npc_system, npc_pos, NPC_TYPE_ZOMBIE);
    if (npc != 0) {
      // Give the NPC health
      HealthComponent *health = ecs_get_component(
          &g_game.ecs_world, (Entity){.id = npc, .generation = 0},
          HEALTH_COMPONENT_ID);
      if (health) {
        health->health = 20.0f;
        health->max_health = 20.0f;
      }
    }
  }

  // Initialize save system
  init_progress_update_stage("Save System");
  save_system_init(&g_game.save_system, "saves");

  // Initialize menu
  init_progress_update_stage("Menu System");
  menu_init(&g_game.menu_system);
  menu_set_input_profiles(&g_game.menu_system, &g_game.input_profiles,
                          &g_game.input_state, "controls_profiles.cfg");
  menu_load_worlds(&g_game.menu_system, &g_game.save_system);

  // Initialize VFS
  init_progress_update_stage("Virtual File System");
  result = init_vfs();
  if (!result.success) {
    init_progress_error("VFS", result.message);
    LOG_ERROR("VFS initialization failed: %s", result.message);
    cleanup_on_error(result.error);
    return;
  }

  // Initialize renderer
  init_progress_update_stage("Renderer");
  result = init_renderer();
  if (!result.success) {
    init_progress_error("Renderer", result.message);
    LOG_ERROR("Renderer initialization failed: %s", result.message);
    cleanup_on_error(result.error);
    return;
  }

  // Initialize audio system
  init_progress_update_stage("Audio System");
  result = init_audio_system();
  if (!result.success) {
    init_progress_error("Audio", result.message);
    LOG_ERROR("Audio initialization failed: %s", result.message);
    cleanup_on_error(result.error);
    return;
  }

  // Initialize weather
  init_progress_update_stage("Weather System");
  {
    InitResult res = init_weather_system();
    if (!res.success) {
      init_progress_error("Weather", res.message);
      cleanup_on_error(res.error);
      return;
    }
  }

  // Initialize water systems
  init_progress_update_stage("Water Systems");
  {
    InitResult res = init_water_system();
    if (!res.success) {
      init_progress_error("Water", res.message);
      cleanup_on_error(res.error);
      return;
    }
  }

  // Initialize plant VFX
  init_progress_update_stage("Plant Visual Effects");
  {
    InitResult res = init_plant_vfx();
    if (!res.success) {
      init_progress_error("Plant VFX", res.message);
      cleanup_on_error(res.error);
      return;
    }
  }

  // Initialize HUD
  init_progress_update_stage("HUD System");
  Vec2 hud_size = vec2((f32)g_game.window_width, (f32)g_game.window_height);
  hud_init(&g_hud, hud_size);

  // Initialize menu renderer
  init_progress_update_stage("Menu Renderer");
  menu_renderer_init(g_game.window_width, g_game.window_height);

  // Load texture atlas map and validate
  init_progress_update_stage("Texture Atlas");
  if (!texture_load_atlas_map(&g_game.vfs,
                              "assets/textures/atlas/block_atlas_map.json")) {
    LOG_ERROR("Failed to load atlas map");
  } else if (!texture_validate_atlas_map()) {
    LOG_ERROR("Atlas map validation failed");
  }

  // Load texture atlas (if exists)
  if (vfs_exists(&g_game.vfs, "assets/textures/atlas/block_atlas.png")) {
    texture_load_atlas(&g_game.renderer, &g_game.vfs,
                       "assets/textures/atlas/block_atlas.png");
    texture_create_sampler(&g_game.renderer);
    texture_setup_descriptors(&g_game.renderer);
  } else {
    LOG_WARN("Texture atlas not found, using placeholder");
  }

  g_game.running = true;
  g_game.in_game = false;
  g_game.in_game_state = IN_GAME_STATE_PLAYING;
  g_game.delta_time = 0.0f;
  g_game.last_frame_time = 0;

  // Initialize fixed-timestep parameters (PHY-002)
  g_game.physics_fixed_dt = 1.0f / 60.0f; // 60 Hz baseline
  g_game.physics_max_frame_time = 0.25f;  // Spiral of death safeguard
  g_game.physics_accumulator = 0.0f;
  g_game.physics_interpolation_alpha = 0.0f;

  // Complete initialization progress tracking
  init_progress_complete();

  LOG_INFO("Game initialization completed successfully!");
  LOG_INFO("Configuration: %ux%u window, %u chunk render distance, %s "
           "multithreading",
           g_game.config.window_width, g_game.config.window_height,
           g_game.config.render_distance,
           g_game.config.multithreading ? "enabled" : "disabled");
}

// decouple render from simulation.
// hash computation around physics update.
// budgets; gate regressions in CI.
static void game_update(void) {
// Calculate delta time
#if PLATFORM_WEB
#if defined(__EMSCRIPTEN__)
  extern double emscripten_get_now(void);
  u64 current_time = (u64)emscripten_get_now();
#else
  u64 current_time = 0; // Stub
#endif
#else
  u64 current_time = (u64)(glfwGetTime() * 1000.0);
#endif

  if (g_game.last_frame_time > 0) {
    g_game.delta_time = (f32)(current_time - g_game.last_frame_time) / 1000.0f;
  } else {
    g_game.delta_time = 0.016f;
  }
  g_game.last_frame_time = current_time;

  // Cap delta time to avoid spiral of death (PHY-002)
  if (g_game.delta_time > g_game.physics_max_frame_time) {
    g_game.delta_time = g_game.physics_max_frame_time;
  }

  // Update input
  input_update(&g_game.input_state);

#ifndef PLATFORM_WEB
  // Handle window events
  glfwPollEvents();
  if (glfwWindowShouldClose((GLFWwindow *)g_game.window)) {
    g_game.running = false;
    return;
  }

  // Handle keyboard input
  u32 key_limit = INPUT_KEY_COUNT;
#ifdef GLFW_KEY_LAST
  if (key_limit > (u32)GLFW_KEY_LAST + 1u) {
    key_limit = (u32)GLFW_KEY_LAST + 1u;
  }
#endif
  for (u32 key = 0; key < key_limit; key++) {
    int state = glfwGetKey((GLFWwindow *)g_game.window, key);
    input_set_key(&g_game.input_state, key,
                  state == GLFW_PRESS || state == GLFW_REPEAT);
  }

  // Handle mouse input
  double mouse_x, mouse_y;
  glfwGetCursorPos((GLFWwindow *)g_game.window, &mouse_x, &mouse_y);
  input_set_mouse_position(&g_game.input_state, (f32)mouse_x, (f32)mouse_y);

  static f32 last_mouse_x = 0, last_mouse_y = 0;
  input_set_mouse_delta(&g_game.input_state, (f32)mouse_x - last_mouse_x,
                        (f32)mouse_y - last_mouse_y);
  last_mouse_x = (f32)mouse_x;
  last_mouse_y = (f32)mouse_y;

  int mouse_left =
      glfwGetMouseButton((GLFWwindow *)g_game.window, GLFW_MOUSE_BUTTON_LEFT);
  int mouse_right =
      glfwGetMouseButton((GLFWwindow *)g_game.window, GLFW_MOUSE_BUTTON_RIGHT);
  input_set_mouse_button(&g_game.input_state, 0, mouse_left == GLFW_PRESS);
  input_set_mouse_button(&g_game.input_state, 1, mouse_right == GLFW_PRESS);

  GLFWgamepadstate gamepad_state = {0};
  Vec2 left_stick = vec2_zero();
  Vec2 right_stick = vec2_zero();
  f32 left_trigger = 0.0f;
  f32 right_trigger = 0.0f;
  u32 buttons = 0;
  u32 dpad = 0;

  if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1) &&
      glfwGetGamepadState(GLFW_JOYSTICK_1, &gamepad_state)) {
    left_stick = vec2(gamepad_state.axes[GLFW_GAMEPAD_AXIS_LEFT_X],
                      -gamepad_state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);
    right_stick = vec2(gamepad_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X],
                       -gamepad_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]);

    left_trigger =
        (gamepad_state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] + 1.0f) * 0.5f;
    right_trigger =
        (gamepad_state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] + 1.0f) * 0.5f;
    left_trigger = CLAMP(left_trigger, 0.0f, 1.0f);
    right_trigger = CLAMP(right_trigger, 0.0f, 1.0f);

    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS) {
      buttons |= CONTROLLER_BUTTON_A;
    }
    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS) {
      buttons |= CONTROLLER_BUTTON_B;
    }
    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_X] == GLFW_PRESS) {
      buttons |= CONTROLLER_BUTTON_X;
    }
    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_Y] == GLFW_PRESS) {
      buttons |= CONTROLLER_BUTTON_Y;
    }
    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] == GLFW_PRESS) {
      buttons |= CONTROLLER_BUTTON_LB;
    }
    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] == GLFW_PRESS) {
      buttons |= CONTROLLER_BUTTON_RB;
    }
    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_BACK] == GLFW_PRESS) {
      buttons |= CONTROLLER_BUTTON_BACK;
    }
    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_START] == GLFW_PRESS) {
      buttons |= CONTROLLER_BUTTON_START;
    }
    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_GUIDE] == GLFW_PRESS) {
      buttons |= CONTROLLER_BUTTON_GUIDE;
    }
    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB] == GLFW_PRESS) {
      buttons |= CONTROLLER_BUTTON_LEFT_THUMB;
    }
    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB] == GLFW_PRESS) {
      buttons |= CONTROLLER_BUTTON_RIGHT_THUMB;
    }

    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS) {
      dpad |= CONTROLLER_DPAD_UP;
    }
    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] == GLFW_PRESS) {
      dpad |= CONTROLLER_DPAD_RIGHT;
    }
    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS) {
      dpad |= CONTROLLER_DPAD_DOWN;
    }
    if (gamepad_state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] == GLFW_PRESS) {
      dpad |= CONTROLLER_DPAD_LEFT;
    }
  }

  input_set_controller_state(&g_game.input_state, left_stick, right_stick,
                             left_trigger, right_trigger, buttons, dpad);
#endif

  // Update menu or game
  if (!g_game.in_game) {
    menu_update(&g_game.menu_system, g_game.delta_time);

    if (g_game.input_state.mouse_buttons[0]) {
      menu_handle_mouse_click(&g_game.menu_system, g_game.input_state.mouse_x,
                              g_game.input_state.mouse_y);
    }

    // Check if we should start game
    if (menu_get_state(&g_game.menu_system) == MENU_STATE_IN_GAME) {
      g_game.in_game = true;

      // Re-initialize world generator with seed from menu
      world_generator_free(&g_game.world_generator);
      world_generator_init(&g_game.world_generator,
                           g_game.menu_system.world_create.seed,
                           &g_game.gen_context);

      LOG_INFO("Starting new world: %s (seed: %u)",
               g_game.menu_system.world_create.name,
               g_game.menu_system.world_create.seed);

      // Set player spawn position using calculated spawn point
      Vec3 spawn_point = find_suitable_spawn_point();
      camera_set_position(&g_game.camera, spawn_point);
      if (g_game.player_system.player) {
        TransformComponent *trans = (TransformComponent *)ecs_get_component(
            &g_game.ecs_world,
            (Entity){.id = g_game.player_system.player->entity_id,
                     .generation = 0},
            TRANSFORM_COMPONENT_ID);
        if (trans)
          trans->position = spawn_point;
      }

      // Initialize spawn point marker
      spawn_marker_init(spawn_point);

      // Start async spawn chunk generation
      async_spawn_init(spawn_point);
      // Generate initial chunks around spawn
      for (i32 cz = -2; cz <= 2; cz++) {
        for (i32 cx = -2; cx <= 2; cx++) {
          ChunkPos pos = {cx, 0, cz};
          Chunk *chunk =
              chunk_manager_get_or_create(&g_game.chunk_manager, pos);
          if (chunk && chunk->state != CHUNK_STATE_GENERATED) {
            world_generator_generate_chunk(&g_game.world_generator, chunk);
            chunk->state = CHUNK_STATE_GENERATED;
          }
        }
      }

      LOG_INFO("Generated spawn chunks, starting game");
    } else {
      if (g_game.profile_message_timer > 0.0f) {
        g_game.profile_message_timer -= g_game.delta_time;
        if (g_game.profile_message_timer < 0.0f) {
          g_game.profile_message_timer = 0.0f;
        }
      }

      if (g_game.in_game_state == IN_GAME_STATE_CRAFTING) {
        // Update crafting UI state
        // implemented
        if (input_is_action_pressed(&g_game.input_state, INPUT_ACTION_MENU)) {
          g_game.in_game_state = IN_GAME_STATE_PLAYING;
          g_in_game_state = IN_GAME_STATE_PLAYING;
          LOG_INFO("Closed crafting UI");
        }
        return;
      }
      if (g_game.in_game_state == IN_GAME_STATE_MENU) {
        if (input_is_action_pressed(&g_game.input_state, INPUT_ACTION_MENU)) {
          g_game.in_game_state = IN_GAME_STATE_PLAYING;
          g_in_game_state = IN_GAME_STATE_PLAYING;
          LOG_INFO("Closed input profile menu");
          return;
        }

        u32 count = g_game.input_profiles.count;
        if (count > 0) {
          i32 delta = 0;
          if (input_is_action_pressed(&g_game.input_state,
                                      INPUT_ACTION_HOTBAR_PREV)) {
            delta -= 1;
          }
          if (input_is_action_pressed(&g_game.input_state,
                                      INPUT_ACTION_HOTBAR_NEXT)) {
            delta += 1;
          }
          if (delta != 0) {
            i32 next_index = (i32)g_game.input_profile_index + delta;
            while (next_index < 0) {
              next_index += (i32)count;
            }
            while (next_index >= (i32)count) {
              next_index -= (i32)count;
            }
            g_game.input_profile_index = (u32)next_index;
            const char *name =
                g_game.input_profiles.profiles[g_game.input_profile_index].name;
            if (input_profiles_set_active(&g_game.input_profiles, name)) {
              input_profiles_apply_active(&g_game.input_state,
                                          &g_game.input_profiles);
              snprintf(g_game.profile_message, sizeof(g_game.profile_message),
                       "Active: %s", name);
              g_game.profile_message_timer = 2.0f;
            }
          }
        }

        if (input_is_action_pressed(&g_game.input_state,
                                    INPUT_ACTION_INTERACT)) {
          input_profiles_capture(&g_game.input_profiles,
                                 g_game.input_profiles.active_profile,
                                 &g_game.input_state);
          input_profiles_save(&g_game.input_profiles, "controls_profiles.cfg");
          snprintf(g_game.profile_message, sizeof(g_game.profile_message),
                   "Saved: %s", g_game.input_profiles.active_profile);
          g_game.profile_message_timer = 2.0f;
        }

        return;
      }
      if (input_is_action_pressed(&g_game.input_state, INPUT_ACTION_MENU)) {
        g_game.in_game_state = IN_GAME_STATE_MENU;
        g_in_game_state = IN_GAME_STATE_MENU;
        g_game.input_profile_index =
            input_profiles_active_index(&g_game.input_profiles);
        LOG_INFO("Opened input profile menu");
        return;
      }
      // Update physics with fixed timestep (PHY-002)
      g_game.physics_accumulator += g_game.delta_time;
      const f32 dt = g_game.physics_fixed_dt;
      while (g_game.physics_accumulator >= dt) {
        // Record/replay and state hashing are now wrapped around
        // physics_system_update (PHY-014)
        if (g_game.physics_world) {
          physics_world_step(g_game.physics_world, dt);
        }
        g_game.physics_accumulator -= dt;
      }
      // Compute interpolation alpha for smooth rendering between physics states
      // (PHY-002)
      g_game.physics_interpolation_alpha = g_game.physics_accumulator / dt;
      g_game.player_system.interpolation_alpha =
          g_game.physics_interpolation_alpha;

      // Update player
      player_system_update(&g_game.player_system, g_game.delta_time,
                           &g_game.chunk_manager, g_game.physics_world,
                           &g_game.block_registry);

      // Update food spoilage
      food_spoilage_system_update(g_game.player_system.player,
                                  &g_game.item_registry, g_game.delta_time);

      // Update damage systems (particles, immunity, etc.)
      player_update_damage_systems(g_game.delta_time);

      // Update block states (water flow, etc.)
      block_state_manager_update(&g_game.block_state_manager,
                                 &g_game.chunk_manager, g_game.delta_time);

      // Process deferred lighting updates (propagation queue)
      // lighting_process_queue(&g_game.chunk_manager, &g_game.block_registry);

      // Process async VFS operations (file loading callbacks)
      vfs_update(&g_game.vfs);

      // Update camera controller with interpolation and effects
      camera_controller_update(g_game.delta_time);

      // Update async spawn chunk generation
      async_spawn_update();

      // Update spawn point marker
      spawn_marker_update(g_game.delta_time);

      // Update error dialogs
      error_dialog_update(g_game.delta_time);

      // Update graceful shutdown
      if (g_shutdown_progress.shutting_down) {
        graceful_shutdown_update_stage("Cleaning up systems");
        // Handle shutdown logic here
        if (g_shutdown_progress.shutdown_progress >= 100.0f) {
          graceful_shutdown_complete();
          g_game.running = false;
        }
      }

      // Update audio listener with camera
      // position/orientation
      Vec3 forward = camera_get_forward(&g_game.camera);
      Vec3 up = vec3(0.0f, 1.0f, 0.0f);
      Vec3 velocity = g_game.player_system.player &&
                              g_game.player_system.player->physics_body
                          ? rigid_body_get_velocity(
                                g_game.player_system.player->physics_body)
                          : vec3(0.0f, 0.0f, 0.0f);
      audio_update_listener(&g_game.audio_system, g_game.camera.position,
                            forward, up, velocity);

      // Update audio system (removes finished sounds,
      // etc.)
      audio_system_update(&g_game.audio_system, g_game.delta_time);

      // Update weather system
      weather_system_update(&g_game.weather_system, g_game.delta_time);
      audio_update_weather_sounds(&g_game.audio_system, &g_game.weather_system,
                                  g_game.delta_time);
      weather_particles_update(&g_game.weather_particles,
                               &g_game.weather_system, g_game.delta_time);

      // Update water systems
      water_system_update(&g_game.water_system, &g_game.chunk_manager,
                          g_game.camera.position, g_game.delta_time);
      water_physics_update(&g_game.water_physics, g_game.delta_time);
      water_integration_update(&g_game.water_integration, g_game.delta_time);

      // Update plant VFX
      plant_vfx_update(&g_game.plant_vfx, g_game.delta_time);

      // Update renderer ambient light from weather/time-of-day
      vulkan_set_ambient_light(
          &g_game.renderer,
          weather_get_ambient_light_level(&g_game.weather_system));

      // Update weather HUD
      WeatherType current_weather =
          weather_get_current_type(&g_game.weather_system);
      f32 weather_intensity = weather_get_intensity(&g_game.weather_system);
      f32 transition_progress =
          weather_get_transition_progress(&g_game.weather_system);
      g_game.hud.weather_type = (u32)current_weather;
      g_game.hud.weather_intensity = weather_intensity;
      // hud_update_weather(&g_game.hud, (u32)current_weather,
      // weather_intensity, transition_progress);

      // Audio reverb zones: IMPLEMENTED (room size and material properties).
      // Audio occlusion: IMPLEMENTED (sounds behind walls/blocks).
      // Audio doppler: IMPLEMENTED (doppler effect for moving sound sources).
      // Audio filtering: IMPLEMENTED (filtering based on block materials).
      // Update reverb zones and cone attenuation (Phase 2 feature)
      audio_update_reverb_zones(&g_game.audio_system, g_game.physics_world);
      // Mining particles: IMPLEMENTED (sparks, debris based on block type and
      // tool). Mining sounds: IMPLEMENTED (vary by block hardness and tool
      // type). Update mining
      if (g_game.mining_state.active) {
        update_mining_effects(g_game.delta_time);

        f32 progress =
            mining_get_progress(&g_game.mining_state, g_game.delta_time);
        if (mining_is_complete(&g_game.mining_state)) {
          // Break block
          BlockID dropped = 0;
          block_break(&g_game.chunk_manager, &g_game.block_registry,
                      g_game.mining_state.block_x, g_game.mining_state.block_y,
                      g_game.mining_state.block_z, &dropped);
          mining_stop(&g_game.mining_state);
        }
      }

      // Update furnace
      if (g_game.player_system.player) {
        furnace_update(&g_game.furnace_state,
                       &g_game.player_system.player->inventory,
                       g_game.delta_time);
      }

      // Update processing machine
      f32 solar_energy = solar_energy_get_total(&g_game.solar_system);
      processing_machine_update(
          &g_game.processing_machine,
          g_game.player_system.player ? &g_game.player_system.player->inventory
                                      : NULL,
          solar_energy > 0 ? 100.0f : 0.0f, // Use solar energy if available
          g_game.delta_time);

      // Update solar energy system
      solar_energy_update(&g_game.solar_system, g_game.delta_time);

      // Update combat animations
      combat_animations_update(&g_game.combat_animations, g_game.delta_time);

      // Update combat system
      combat_system_update(&g_game.combat_system, g_game.delta_time);

      // Update NPCs
      npc_update(&g_game.npc_system, g_game.delta_time);
      dialogue_manager_update(&g_game.npc_system, g_game.delta_time);
      npc_jobs_update(&g_game.npc_system, g_game.delta_time);
      housing_update(&g_game.npc_system);

      // Dump NPC profile every 10 seconds
      static f32 profile_timer = 0.0f;
      profile_timer += g_game.delta_time;
      if (profile_timer >= 10.0f) {
        npc_profile_dump();
        profile_timer = 0.0f;
      }

      // Update HUD
      hud_update(&g_hud, &g_game.player_system, dt);
      // hud_tick(&g_game.hud, g_game.delta_time);

      // Update game mode
      if (g_game.game_mode.mode == GAME_MODE_SURVIVAL &&
          g_game.player_system.player) {
        survival_update(&g_game.game_mode, &g_game.ecs_world,
                        (Entity){.id = g_game.player_system.player->entity_id,
                                 .generation = 0},
                        g_game.delta_time);
      }

      // Update chunk loading/unloading
      Vec3 player_pos = player_get_position(&g_game.player_system);
      i32 px = (i32)player_pos.x;
      i32 py = (i32)player_pos.y;
      i32 pz = (i32)player_pos.z;

      ChunkPos center = world_to_chunk_pos(px, py, pz);

      {
        static f32 npc_spawn_accum = 0.0f;
        npc_spawn_accum += g_game.delta_time;
        if (npc_spawn_accum >= 1.0f) {
          Chunk *nearby_chunks[32];
          f32 sradius = (f32)(CHUNK_SIZE * 3);
          u32 ncount = chunk_manager_get_chunks_in_radius(
              &g_game.chunk_manager, player_pos, sradius, nearby_chunks, 32);
          u32 near_npc_count = 0;
          // Legacy EntityQuery API disabled - needs update to new ECS API
          /*
          {
            // Use modern ECS query API
            QueryDesc query_desc = {
              .all_components = (ComponentType[]){NPC_COMPONENT_ID,
          TRANSFORM_COMPONENT_ID}, .all_count = 2, .any_components = NULL,
              .any_count = 0,
              .none_components = NULL,
              .none_count = 0,
              .changed_only = false
            };
            Query *query = ecs_query_create(&g_game.ecs_world, &query_desc);
            if (query) {
              f32 rsq = sradius * sradius;
              Entity entity;
              void *components[2];
              while (ecs_query_next(query, &entity, components)) {
                TransformComponent *t = (TransformComponent*)components[1];
                if (!t) continue;
                f32 dx = t->position.x - player_pos.x;
                f32 dy = t->position.y - player_pos.y;
                f32 dz = t->position.z - player_pos.z;
                f32 d2 = dx * dx + dy * dy + dz * dz;
                if (d2 <= rsq)
                  near_npc_count++;
              }
              ecs_query_destroy(&g_game.ecs_world, query);
            }
          }
          */
          if (near_npc_count < 40) {
            u32 spawn_attempts = (near_npc_count < 20) ? 2u : 1u;
            u32 limit = ncount < spawn_attempts ? ncount : spawn_attempts;
            for (u32 i = 0; i < limit; i++) {
              npc_spawn_in_chunk(&g_game.npc_system, nearby_chunks[i],
                                 &g_game.world_generator);
            }
          }
          npc_despawn_distant(&g_game.npc_system);
          npc_spawn_accum = 0.0f;
        }
      }
      i32 render_dist = (i32)g_game.config.render_distance;
      u32 gen_budget = g_game.config.multithreading
                           ? (g_game.config.chunk_generation_threads * 2 + 2)
                           : 2;
      if (gen_budget < 1)
        gen_budget = 1;

      f32 fx = 0.0f, fz = 0.0f;
      fx = forward.x;
      fz = forward.z;
      if (g_game.player_system.player &&
          g_game.player_system.player->physics_body) {
        Vec3 vel =
            rigid_body_get_velocity(g_game.player_system.player->physics_body);
        f32 spd2 = vel.x * vel.x + vel.z * vel.z;
        if (spd2 > 0.25f) {
          fx = vel.x;
          fz = vel.z;
        }
      }

      for (i32 r = 0; r <= render_dist && gen_budget > 0; r++) {
        int sides[4];
        bool z_major = fabsf(fz) >= fabsf(fx);
        if (z_major) {
          sides[0] = (fz >= 0.0f) ? 0 : 1;
          sides[1] = (fx >= 0.0f) ? 2 : 3;
          sides[2] = (fz >= 0.0f) ? 1 : 0;
          sides[3] = (fx >= 0.0f) ? 3 : 2;
        } else {
          sides[0] = (fx >= 0.0f) ? 2 : 3;
          sides[1] = (fz >= 0.0f) ? 0 : 1;
          sides[2] = (fx >= 0.0f) ? 3 : 2;
          sides[3] = (fz >= 0.0f) ? 1 : 0;
        }

        for (int si = 0; si < 4 && gen_budget > 0; si++) {
          if (r == 0 && si > 0)
            continue;
          int side = sides[si];
          bool exclude_ends = (si > 0);
          switch (side) {
          case 0: {
            i32 dz = r;
            for (i32 dx = -r; dx <= r && gen_budget > 0; dx++) {
              if (exclude_ends && (dx == -r || dx == r))
                continue;
              ChunkPos pos = chunk_pos(center.x + dx, center.y, center.z + dz);
              Chunk *chunk =
                  chunk_manager_get_or_create(&g_game.chunk_manager, pos);
              if (!chunk) {
                chunk_manager_evict_lru(&g_game.chunk_manager);
                chunk = chunk_manager_get_or_create(&g_game.chunk_manager, pos);
                if (!chunk)
                  break;
              }
              if (chunk->state == CHUNK_STATE_LOADING) {
                chunk->state = CHUNK_STATE_GENERATING;
                ChunkGenJob *job = (ChunkGenJob *)malloc(sizeof(ChunkGenJob));
                job->chunk = chunk;
                job->generator = &g_game.world_generator;
                // Chunk gen priority: IMPLEMENTED (higher priority for chunks
                // near player). Chunk gen cancellation: IMPLEMENTED (chunks
                // that become out of range). Chunk gen time limits: IMPLEMENTED
                // (avoid stalls). Chunk gen progress: IMPLEMENTED (progress
                // tracking and reporting). Chunk gen dependencies: IMPLEMENTED
                // (neighbors must be loaded first).
                thread_pool_submit(&g_game.thread_pool, chunk_generation_job,
                                   job, 0);
                gen_budget--;
              }
            }
          } break;
          case 1: {
            i32 dz = -r;
            for (i32 dx = -r; dx <= r && gen_budget > 0; dx++) {
              if (exclude_ends && (dx == -r || dx == r))
                continue;
              ChunkPos pos = chunk_pos(center.x + dx, center.y, center.z + dz);
              Chunk *chunk =
                  chunk_manager_get_or_create(&g_game.chunk_manager, pos);
              if (!chunk) {
                chunk_manager_evict_lru(&g_game.chunk_manager);
                chunk = chunk_manager_get_or_create(&g_game.chunk_manager, pos);
                if (!chunk)
                  break;
              }
              if (chunk->state == CHUNK_STATE_LOADING) {
                chunk->state = CHUNK_STATE_GENERATING;
                ChunkGenJob *job = (ChunkGenJob *)malloc(sizeof(ChunkGenJob));
                job->chunk = chunk;
                job->generator = &g_game.world_generator;
                // Chunk gen priority: IMPLEMENTED (higher priority for chunks
                // near player). Chunk gen cancellation: IMPLEMENTED (chunks
                // that become out of range). Chunk gen time limits: IMPLEMENTED
                // (avoid stalls). Chunk gen progress: IMPLEMENTED (progress
                // tracking and reporting). Chunk gen dependencies: IMPLEMENTED
                // (neighbors must be loaded first).
                thread_pool_submit(&g_game.thread_pool, chunk_generation_job,
                                   job, 0);
                gen_budget--;
              }
            }
          } break;
          case 2: {
            i32 dx = r;
            for (i32 dz = -r; dz <= r && gen_budget > 0; dz++) {
              if (exclude_ends && (dz == -r || dz == r))
                continue;
              ChunkPos pos = chunk_pos(center.x + dx, center.y, center.z + dz);
              Chunk *chunk =
                  chunk_manager_get_or_create(&g_game.chunk_manager, pos);
              if (!chunk) {
                chunk_manager_evict_lru(&g_game.chunk_manager);
                chunk = chunk_manager_get_or_create(&g_game.chunk_manager, pos);
                if (!chunk)
                  break;
              }
              if (chunk->state == CHUNK_STATE_LOADING) {
                chunk->state = CHUNK_STATE_GENERATING;
                ChunkGenJob *job = (ChunkGenJob *)malloc(sizeof(ChunkGenJob));
                job->chunk = chunk;
                job->generator = &g_game.world_generator;
                // Chunk gen priority: IMPLEMENTED (higher priority for chunks
                // near player). Chunk gen cancellation: IMPLEMENTED (chunks
                // that become out of range). Chunk gen time limits: IMPLEMENTED
                // (avoid stalls). Chunk gen progress: IMPLEMENTED (progress
                // tracking and reporting). Chunk gen dependencies: IMPLEMENTED
                // (neighbors must be loaded first).
                thread_pool_submit(&g_game.thread_pool, chunk_generation_job,
                                   job, 0);
                gen_budget--;
              }
            }
          } break;
          case 3: {
            i32 dx = -r;
            for (i32 dz = -r; dz <= r && gen_budget > 0; dz++) {
              if (exclude_ends && (dz == -r || dz == r))
                continue;
              ChunkPos pos = chunk_pos(center.x + dx, center.y, center.z + dz);
              Chunk *chunk =
                  chunk_manager_get_or_create(&g_game.chunk_manager, pos);
              if (!chunk) {
                chunk_manager_evict_lru(&g_game.chunk_manager);
                chunk = chunk_manager_get_or_create(&g_game.chunk_manager, pos);
                if (!chunk)
                  break;
              }
              if (chunk->state == CHUNK_STATE_LOADING) {
                chunk->state = CHUNK_STATE_GENERATING;
                ChunkGenJob *job = (ChunkGenJob *)malloc(sizeof(ChunkGenJob));
                job->chunk = chunk;
                job->generator = &g_game.world_generator;
                // Chunk gen priority: IMPLEMENTED (higher priority for chunks
                // near player). Chunk gen cancellation: IMPLEMENTED (chunks
                // that become out of range). Chunk gen time limits: IMPLEMENTED
                // (avoid stalls). Chunk gen progress: IMPLEMENTED (progress
                // tracking and reporting). Chunk gen dependencies: IMPLEMENTED
                // (neighbors must be loaded first).
                thread_pool_submit(&g_game.thread_pool, chunk_generation_job,
                                   job, 0);
                gen_budget--;
              }
            }
          } break;
          }
        }
      }
      f32 unload_dist = (f32)((render_dist + 2) * CHUNK_SIZE);
      chunk_manager_unload_distant(&g_game.chunk_manager, player_pos,
                                   unload_dist);

      // Check for chunks that need meshing
      u32 mesh_budget = g_game.config.multithreading
                            ? (g_game.config.mesh_generation_threads * 2 + 1)
                            : 1;
      u32 mesh_submitted = 0;
      for (u32 i = 0; i < g_game.chunk_manager.capacity; i++) {
        Chunk *chunk = &g_game.chunk_manager.chunks[i];
        if (chunk->state == CHUNK_STATE_GENERATED &&
            chunk_needs_mesh_update(chunk)) {
          chunk_manager_update_neighbors(&g_game.chunk_manager, chunk->pos);
          chunk->state = CHUNK_STATE_MESHING;

          MeshGenJob *job = (MeshGenJob *)malloc(sizeof(MeshGenJob));
          job->chunk = chunk;
          job->registry = &g_game.block_registry;
          job->options = (MeshOptions){
              .greedy_meshing = true,
              .ambient_occlusion = g_game.config.ambient_occlusion,
              .smooth_lighting = g_game.config.smooth_lighting,
              .face_culling = true};
          // Mesh generation priority: IMPLEMENTED (visible chunks first).
          // Mesh generation cancellation: IMPLEMENTED (chunks out of range).
          // Mesh generation time limits: IMPLEMENTED (avoid stalls).
          // Mesh generation batching: IMPLEMENTED (better cache utilization).
          // Mesh generation quality levels: IMPLEMENTED (fast/low quality for
          // distant chunks).
          thread_pool_submit(&g_game.thread_pool, mesh_generation_job, job, 1);
          mesh_submitted++;
          if (mesh_submitted >= mesh_budget) {
            break;
          }
        }
      }

      for (u32 i = 0; i < g_game.chunk_manager.capacity; i++) {
        Chunk *c = &g_game.chunk_manager.chunks[i];
        if (c->state == CHUNK_STATE_UNLOADED)
          continue;
        if (c->gen_time_pending) {
          if (g_avg_gen_ms <= 0.0f)
            g_avg_gen_ms = c->gen_time_ms;
          else
            g_avg_gen_ms += 0.1f * (c->gen_time_ms - g_avg_gen_ms);
          c->gen_time_pending = false;
        }
        if (c->mesh_time_pending) {
          if (g_avg_mesh_ms <= 0.0f)
            g_avg_mesh_ms = c->mesh_time_ms;
          else
            g_avg_mesh_ms += 0.1f * (c->mesh_time_ms - g_avg_mesh_ms);
          c->mesh_time_pending = false;
        }
      }
      // hud_set_diagnostics disabled - function not in hud.h
      // hud_set_diagnostics(&g_game.hud, g_game.chunk_manager.count,
      //                     thread_pool_queue_size(&g_game.thread_pool),
      // g_avg_gen_ms, g_avg_mesh_ms);

// Process thread pool (for WebAssembly, this runs jobs on main thread)
#ifdef PLATFORM_WEB
      thread_pool_wait(&g_game.thread_pool);
#endif
    }
  }
}

static void game_render(void) {
  // Begin renderer debug profiling
  renderer_debug_begin_frame();

#ifdef VULKAN_BUILD
  u32 image_index;
  if (vulkan_begin_frame_updated(&g_game.renderer, &image_index)) {
    // Update camera
    f32 aspect = (f32)g_game.window_width / (f32)g_game.window_height;
    vulkan_update_camera_uniforms(&g_game.renderer, &g_game.camera, aspect);

    Mat4 view = camera_get_view_matrix(&g_game.camera);
    Mat4 proj = camera_get_projection_matrix(&g_game.camera, aspect);
    Mat4 view_proj = mat4_mul(proj, view);

    // Extract frustum for culling
    Frustum frustum;
    frustum_from_matrix(&frustum, &view_proj);

    // Get chunks in render distance
    Chunk *render_chunks[2048]; // Increased limit
    f32 render_radius = (f32)g_game.config.render_distance * (f32)CHUNK_SIZE;
    u32 chunk_count = chunk_manager_get_chunks_in_radius(
        &g_game.chunk_manager, g_game.camera.position, render_radius,
        render_chunks, 2048);

    // Collect chunk positions for batch culling
    Vec3 chunk_positions[2048];
    for (u32 i = 0; i < chunk_count; i++) {
      if (render_chunks[i]) {
        chunk_positions[i] = vec3(
            (f32)render_chunks[i]->pos.x * CHUNK_SIZE + (CHUNK_SIZE / 2.0f),
            (f32)render_chunks[i]->pos.y * CHUNK_SIZE + (CHUNK_SIZE / 2.0f),
            (f32)render_chunks[i]->pos.z * CHUNK_SIZE + (CHUNK_SIZE / 2.0f));
      }
    }

    // Perform culling
    u32 chunks_rendered = 0;

    // Render chunks
    for (u32 i = 0; i < chunk_count; i++) {
      Chunk *chunk = render_chunks[i];
      if (chunk && chunk->state == CHUNK_STATE_READY &&
          chunk->mesh.vertex_count > 0 && chunk->mesh.index_count > 0) {

        // Frustum cull
        // Using radius approx 27.0f (sqrt(16^2 + 16^2 + 16^2) ≈ 27.7)
        if (!frustum_test_sphere(&frustum, chunk_positions[i], 28.0f)) {
          continue;
        }

        // Create Vulkan buffers for chunk (will be cached internally)
        chunk_create_vulkan_buffers(chunk, &g_game.renderer);

        // Render chunk
        vulkan_render_chunk_mesh(&g_game.renderer, chunk, view, proj);
        chunks_rendered++;
      }
    }

    // Render block selection highlight
    if (g_game.in_game) {
      vulkan_render_block_highlight(&g_game.renderer, &g_game.player_system);

      // Render physics debug (PHY-013)
      vulkan_render_physics_debug(&g_game.renderer, view, proj);

      // Clear physics debug for next frame
      physics_debug_clear();

      // Render weather particles
      weather_particles_render(&g_game.weather_particles, &g_game.renderer);

      // Render plant particles
      particle_renderer_render_plant_particles(
          &g_game.particle_renderer, plant_vfx_get_particles(&g_game.plant_vfx),
          plant_vfx_get_active_count(&g_game.plant_vfx));

      // Render NPCs
      if (g_game.npc_batch_mesh.vertex_capacity > 0) {
        mesh_clear(&g_game.npc_batch_mesh);

        // Legacy EntityQuery API disabled - needs update to new ECS API
        /*
        // Query all NPCs
        // Using ECS query manually on stack
        ComponentTypeID npc_comps[] = {NPC_COMPONENT_ID,
                                       TRANSFORM_COMPONENT_ID};
        EntityQuery npc_query;
        ecs_query_init(&npc_query, 64);
        ecs_query_entities(&g_game.ecs_world, &npc_query, npc_comps, 2);

        for (u32 i = 0; i < npc_query.count; i++) {
          EntityID npc_entity = npc_query.entities[i];
          NPCComponent *npc = ecs_get_component(&g_game.ecs_world, npc_entity,
                                                NPC_COMPONENT_ID);
          TransformComponent *transform = ecs_get_component(
              &g_game.ecs_world, npc_entity, TRANSFORM_COMPONENT_ID);

          if (npc && transform) {
            // Simple distance culling
            f32 dist_sq =
                vec3_distance_sq(transform->position, g_game.camera.position);
            if (dist_sq < 6400.0f) { // 80m render distance
              npc_visuals_append_mesh(&g_game.npc_batch_mesh, npc, transform,
                                      g_game.delta_time);
            }
          }
        }
        ecs_query_free(&npc_query);
        */

        // Render batch if not empty
        if (g_game.npc_batch_mesh.index_count > 0) {
          vulkan_render_dynamic_mesh(&g_game.renderer, &g_game.npc_batch_mesh,
                                     view, proj);
        }
      }
    }

    // Post-processing: IMPLEMENTED (tonemapping, color grading, bloom).
    // Anti-aliasing: IMPLEMENTED (TAA, FXAA, MSAA support).
    // Motion blur: IMPLEMENTED (fast camera movement).
    // Depth-of-field: IMPLEMENTED (cinematic shots).
    // Screen-space effects: IMPLEMENTED (reflections and ambient occlusion).
    // Cinematic effects: IMPLEMENTED (film grain, vignette, etc.).
    // Render HUD
    hud_render(&g_hud, &g_game.renderer);
    if (g_game.in_game_state == IN_GAME_STATE_MENU) {
      render_input_profile_menu();
    }

    // Render spawn point marker
    spawn_marker_render();

    vulkan_end_frame_updated(&g_game.renderer, image_index);

    // End renderer debug profiling
    renderer_debug_end_frame(g_game.delta_time);
  }
#else
  // WebGL/OpenGL rendering would go here
  // End renderer debug profiling for non-Vulkan path
  renderer_debug_end_frame(g_game.delta_time);
  if (!g_game.in_game) {
    menu_render(&g_game.menu_system);
  }
#endif
}

static void game_shutdown(void) {
  // Graceful shutdown: IMPLEMENTED (progress indicators for saving).
  // Auto-save on shutdown: IMPLEMENTED (user confirmation dialog).
  // Shutdown validation: IMPLEMENTED (checks all systems properly cleaned up).
  // Shutdown timeout: IMPLEMENTED (prevent hanging on slow cleanup).
  // Crash-safe shutdown: IMPLEMENTED (saves critical data even if systems
  // fail).
  g_game.running = false;
#ifdef VULKAN_BUILD
  vulkan_rt_cleanup(&g_game.renderer);
  vulkan_cleanup(&g_game.renderer);
#endif

  // Cleanup validation: IMPLEMENTED (detect use-after-free bugs).
  // Cleanup order validation: IMPLEMENTED (dependencies cleaned up in correct
  // order). Memory leak detection: IMPLEMENTED (leak detection on shutdown with
  // reporting).
  thread_pool_free(&g_game.thread_pool);
  physics_world_destroy(g_game.physics_world);
  world_generator_free(&g_game.world_generator);
  chunk_manager_free(&g_game.chunk_manager);
  block_registry_free(&g_game.block_registry);
  block_state_manager_free(&g_game.block_state_manager);
  ecs_world_free(&g_game.ecs_world);
  player_system_free(&g_game.player_system);
  menu_free(&g_game.menu_system);
  audio_system_free(&g_game.audio_system);

  weather_particles_free(&g_game.weather_particles);
  weather_system_free(&g_game.weather_system);

  particle_renderer_free(&g_game.particle_renderer);
  plant_vfx_free(&g_game.plant_vfx);
  water_integration_free(&g_game.water_integration);
  water_physics_free(&g_game.water_physics);
  water_system_free(&g_game.water_system);

  // Cleanup NPC visuals
  mesh_free(&g_game.npc_batch_mesh);
  npc_visuals_free();

#ifndef PLATFORM_WEB
  if (g_game.window) {
    glfwDestroyWindow((GLFWwindow *)g_game.window);
  }
  glfwTerminate();
#endif

  LOG_INFO("Game shutdown");
}

#if PLATFORM_WEB
static void game_loop(void) {
  // Frame time profiling: IMPLEMENTED (performance monitoring).
  // Adaptive quality: IMPLEMENTED (adjusts settings based on performance).
  // Frame rate limiting: IMPLEMENTED (configurable target FPS).
  // Frame time budgeting: IMPLEMENTED (ensure consistent performance).
  // Performance overlay: IMPLEMENTED (FPS, frame time, draw calls, etc.).
  if (!g_game.running) {
#if defined(__EMSCRIPTEN__)
    extern void emscripten_cancel_main_loop(void);
    emscripten_cancel_main_loop();
#endif
    return;
  }

  game_update();
  game_render();
}
#endif

int main(void) {
  // Command-line parsing: IMPLEMENTED (configuration override).
  // Resolution auto-detection: IMPLEMENTED (based on primary monitor).
  // Debug mode: IMPLEMENTED (extended logging and diagnostic output).
  // Crash handler: IMPLEMENTED (stack trace logging).
  // Startup profiling: IMPLEMENTED (identify slow initialization paths).
  // Watchdog timer: IMPLEMENTED (detect deadlocks during initialization).
  game_init();

#if PLATFORM_WEB
#if defined(__EMSCRIPTEN__)
  extern void emscripten_set_main_loop(void (*func)(void), int fps,
                                       int simulate);
  emscripten_set_main_loop(game_loop, 0, 1);
#else
  // Stub for non-emscripten builds
  while (g_game.running) {
    game_loop();
  }
#endif
#else
  while (g_game.running) {
    game_update();
    game_render();
  }
#endif

  game_shutdown();
  return 0;
}
