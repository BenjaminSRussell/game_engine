#ifndef GAME_CONTEXT_H
#define GAME_CONTEXT_H

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct IRenderer IRenderer;
typedef struct PhysicsEngine PhysicsEngine;
typedef struct ECSWorld ECSWorld;
typedef struct InputManager InputManager;
typedef struct AudioSystem AudioSystem;
typedef struct VFS VFS;

// Global game state structure
typedef struct {
    // Rendering
    IRenderer *renderer;
    bool renderer_ready;

    // Physics
    PhysicsEngine *physics;
    bool physics_ready;

    // Entity Component System
    ECSWorld *ecs;
    bool ecs_ready;

    // Input
    InputManager *input;
    bool input_ready;

    // Audio
    AudioSystem *audio;
    bool audio_ready;

    // Virtual File System
    VFS *vfs;
    bool vfs_ready;

    // State flags
    bool running;
    bool should_exit;
    bool is_paused;
    bool initialized;

    // Timing
    double delta_time;
    double accumulated_time;
    double fixed_timestep;
    double frame_time;

    // Frame info
    uint64_t frame_count;
    double fps;
    double target_fps;
} GameState;

// Global game context (defined in game_context.c)
extern GameState *g_game_state;

// Game lifecycle
bool game_initialize(void);
void game_shutdown(void);

// Game loop
void game_update(double delta_time);
void game_render(void);
void game_loop_iteration(void);

// Game state queries
bool game_is_running(void);
bool game_is_initialized(void);
void game_set_running(bool running);
GameState* game_get_context(void);

// Subsystem access
IRenderer* game_get_renderer(void);
PhysicsEngine* game_get_physics(void);
ECSWorld* game_get_ecs(void);
InputManager* game_get_input(void);
AudioSystem* game_get_audio(void);
VFS* game_get_vfs(void);

// Frame timing
double game_get_delta_time(void);
double game_get_fps(void);
uint64_t game_get_frame_count(void);

#endif // GAME_CONTEXT_H
