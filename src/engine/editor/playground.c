// playground.h does not exist, so we define types here
#include <core/logger.h>
#include <core/memory.h>
#include <core/time_system.h>
#include <ecs/ecs.h>
#include <math/quat.h>
#include <math/vec3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations of types
typedef struct Scene Scene;
typedef struct PhysicsWorld PhysicsWorld;
typedef struct InputContext {
  int dummy;
} InputContext;
typedef struct Transform {
  Vec3 position;
  Quat rotation;
  Vec3 scale;
} Transform;

// Playground session state
typedef struct {
  bool is_active;
  bool is_paused;
  bool is_initialized;

  // Scene management
  Scene *editor_scene;
  Scene *game_scene;
  Scene *backup_scene;

  // Player state
  Entity player_entity;
  Vec3 player_start_position;
  Quat player_start_rotation;

  // Input management
  InputContext editor_input_context;
  InputContext game_input_context;

  // Physics simulation
  PhysicsWorld *physics_world;
  f32 simulation_time;
  f32 time_scale;

  // Session metadata
  u64 session_start_time;
  u64 session_duration;
  u32 frame_count;

  // Performance tracking
  f32 average_fps;
  f32 frame_time_accumulator;
  u32 performance_sample_count;
} PlaygroundSession;

static PlaygroundSession g_session = {0};

// Forward declarations
static void playground_clone_editor_scene(void);
static void playground_switch_input_context(void);
static void playground_initialize_physics(void);
static void playground_spawn_player_pawn(void);
static void playground_restore_editor_state(void);
static void playground_update_simulation(f32 delta_time);
static void playground_handle_game_input(void);
static void playground_update_performance_stats(f32 delta_time);

// Helper function stubs
static Scene *get_current_editor_scene(void) { return NULL; }
static InputContext input_get_current_context(void) {
  return (InputContext){0};
}
static InputContext input_create_game_context(void) {
  return (InputContext){0};
}
static void input_set_context(InputContext context) { (void)context; }
static void input_capture_mouse(bool capture) { (void)capture; }
static bool input_is_mouse_captured(void) { return false; }
static PhysicsWorld *physics_create_world(void) { return NULL; }
static void physics_destroy_world(PhysicsWorld *world) { (void)world; }
static void physics_set_gravity(PhysicsWorld *world, Vec3 gravity) {
  (void)world;
  (void)gravity;
}
static void physics_set_time_step(PhysicsWorld *world, f32 time_step) {
  (void)world;
  (void)time_step;
}
static void physics_step_simulation(PhysicsWorld *world, f32 delta_time) {
  (void)world;
  (void)delta_time;
}

// Public API
void Playground_StartSession(void) {
  printf("Playground: Starting play-in-editor session...\n");

  if (g_session.is_active) {
    printf("WARNING: Playground session already active\n");
    return;
  }

  playground_clone_editor_scene();
  playground_switch_input_context();
  playground_initialize_physics();
  playground_spawn_player_pawn();

  g_session.is_active = true;
  g_session.is_paused = false;
  g_session.simulation_time = 0.0f;
  g_session.time_scale = 1.0f;
  g_session.session_start_time = (u64)(time_get_high_res_time() * 1000000000.0);
  g_session.frame_count = 0;
  g_session.average_fps = 60.0f;
  g_session.frame_time_accumulator = 0.0f;
  g_session.performance_sample_count = 0;

  printf("Playground: Session started successfully\n");
}

void Playground_StopSession(void) {
  printf("Playground: Stopping play-in-editor session...\n");

  if (!g_session.is_active) {
    printf("WARNING: No active playground session to stop\n");
    return;
  }

  g_session.session_duration = (u64)(time_get_high_res_time() * 1000000000.0) -
                               g_session.session_start_time;

  playground_restore_editor_state();

  if (g_session.physics_world) {
    physics_destroy_world(g_session.physics_world);
    g_session.physics_world = NULL;
  }

  g_session.game_scene = NULL;
  memset(&g_session, 0, sizeof(PlaygroundSession));

  printf("Playground: Session stopped\n");
}

void Playground_PauseSession(void) {
  if (!g_session.is_active || g_session.is_paused)
    return;
  g_session.is_paused = true;
  printf("Playground: Session paused\n");
}

void Playground_ResumeSession(void) {
  if (!g_session.is_active || !g_session.is_paused)
    return;
  g_session.is_paused = false;
  printf("Playground: Session resumed\n");
}

void Playground_UpdateSession(f32 delta_time) {
  if (!g_session.is_active || g_session.is_paused)
    return;

  playground_update_simulation(delta_time);
  playground_handle_game_input();
  playground_update_performance_stats(delta_time);

  g_session.simulation_time += delta_time * g_session.time_scale;
  g_session.frame_count++;
}

bool Playground_IsSessionActive(void) { return g_session.is_active; }

bool Playground_IsSessionPaused(void) { return g_session.is_paused; }

Scene *Playground_GetGameScene(void) { return g_session.game_scene; }

Entity Playground_GetPlayerEntity(void) { return g_session.player_entity; }

void Playground_SetTimeScale(f32 scale) {
  g_session.time_scale = (scale > 0.0f) ? scale : 0.0f;
  printf("Playground: Time scale set to %.2f\n", g_session.time_scale);
}

f32 Playground_GetTimeScale(void) { return g_session.time_scale; }

f32 Playground_GetSimulationTime(void) { return g_session.simulation_time; }

void Playground_GetSessionStats(u64 *duration, u32 *frame_count,
                                f32 *average_fps) {
  if (duration)
    *duration = g_session.session_duration;
  if (frame_count)
    *frame_count = g_session.frame_count;
  if (average_fps)
    *average_fps = g_session.average_fps;
}

// Implementation functions
static void playground_clone_editor_scene(void) {
  printf("  Cloning editor scene to game scene...\n");
  g_session.editor_scene = get_current_editor_scene();
  if (!g_session.editor_scene) {
    LOG_ERROR("No editor scene available for cloning");
  }
}

static void playground_switch_input_context(void) {
  printf("  Switching input context from editor to game...\n");
  g_session.editor_input_context = input_get_current_context();
  g_session.game_input_context = input_create_game_context();
  input_set_context(g_session.game_input_context);
  input_capture_mouse(true);
}

static void playground_initialize_physics(void) {
  printf("  Initializing physics engine for game scene...\n");
  g_session.physics_world = physics_create_world();
  if (g_session.physics_world) {
    physics_set_gravity(g_session.physics_world, (Vec3){0, -9.81f, 0});
    physics_set_time_step(g_session.physics_world, 1.0f / 60.0f);
  }
}

static void playground_spawn_player_pawn(void) {
  printf("  Spawning player pawn...\n");
  g_session.player_start_position = (Vec3){0, 2, 0};
  g_session.player_start_rotation = (Quat){0, 0, 0, 1};
  g_session.player_entity = (Entity){0};
}

static void playground_restore_editor_state(void) {
  printf("  Restoring editor state...\n");
  input_set_context(g_session.editor_input_context);
  input_capture_mouse(false);
}

static void playground_update_simulation(f32 delta_time) {
  if (g_session.physics_world) {
    physics_step_simulation(g_session.physics_world,
                            delta_time * g_session.time_scale);
  }
}

static void playground_handle_game_input(void) {
  // Stub
}

static void playground_update_performance_stats(f32 delta_time) {
  g_session.frame_time_accumulator += delta_time;
  g_session.performance_sample_count++;

  if (g_session.performance_sample_count >= 60) {
    f32 average_frame_time =
        g_session.frame_time_accumulator / g_session.performance_sample_count;
    g_session.average_fps = 1.0f / average_frame_time;
    g_session.frame_time_accumulator = 0.0f;
    g_session.performance_sample_count = 0;
  }
}
