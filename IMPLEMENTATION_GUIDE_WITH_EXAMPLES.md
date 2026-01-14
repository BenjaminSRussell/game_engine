# Refactoring Implementation Guide with Code Examples

**Purpose:** Step-by-step guide to execute the refactoring plan with actual code snippets

---

## Step 1: Create Core Module Headers

### 1.1 platform_bootstrap.h

**File Location:** `src/engine/core/platform/platform_bootstrap.h`

```c
#ifndef PLATFORM_BOOTSTRAP_H
#define PLATFORM_BOOTSTRAP_H

#include <stdbool.h>
#include <stdint.h>

// Platform-specific window types
typedef void *GLFWwindow;

// Gamepad state structure
typedef struct {
    float axes[6];
    unsigned char buttons[15];
    int button_count;
    int axis_count;
} GLFWgamepadstate;

// GLFW key codes
#define GLFW_PRESS 1
#define GLFW_RELEASE 0
#define GLFW_REPEAT 2
#define GLFW_MOUSE_BUTTON_LEFT 0
#define GLFW_MOUSE_BUTTON_RIGHT 1
#define GLFW_JOYSTICK_1 0
#define GLFW_KEY_LAST 348

// Gamepad buttons
#define GLFW_GAMEPAD_BUTTON_A 0
#define GLFW_GAMEPAD_BUTTON_B 1
#define GLFW_GAMEPAD_BUTTON_X 2
#define GLFW_GAMEPAD_BUTTON_Y 3
// ... more buttons

// Gamepad axes
#define GLFW_GAMEPAD_AXIS_LEFT_X 0
#define GLFW_GAMEPAD_AXIS_LEFT_Y 1
#define GLFW_GAMEPAD_AXIS_RIGHT_X 2
#define GLFW_GAMEPAD_AXIS_RIGHT_Y 3
#define GLFW_GAMEPAD_AXIS_LEFT_TRIGGER 4
#define GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER 5

// Platform initialization/shutdown
int platform_init(void);
void platform_shutdown(void);

// Timing
double platform_get_time_seconds(void);

// Window management
GLFWwindow *platform_create_window(int width, int height, const char *title,
                                   void *monitor, GLFWwindow *share);
int platform_window_should_close(GLFWwindow *window);
void platform_close_window(GLFWwindow *window);
void platform_poll_events(void);
void platform_destroy_window(GLFWwindow *window);

// Keyboard input
int platform_get_key(GLFWwindow *window, int key);

// Mouse input
int platform_get_mouse_button(GLFWwindow *window, int button);
void platform_get_cursor_position(GLFWwindow *window, double *x, double *y);

// Gamepad input
int platform_is_joystick_gamepad(int joystick);
int platform_get_gamepad_state(int joystick, GLFWgamepadstate *state);

#endif // PLATFORM_BOOTSTRAP_H
```

### 1.2 init_pipeline.h

**File Location:** `src/engine/core/initialization/init_pipeline.h`

```c
#ifndef INIT_PIPELINE_H
#define INIT_PIPELINE_H

#include <stdbool.h>
#include <stdint.h>

// Initialization result codes
typedef enum {
    INIT_ERROR_NONE = 0,
    INIT_ERROR_CONFIG = 1,
    INIT_ERROR_WINDOW = 2,
    INIT_ERROR_RENDERER = 3,
    INIT_ERROR_VULKAN = 4,
    INIT_ERROR_GRAPHICS_PIPELINE = 5,
    INIT_ERROR_FRAMEBUFFERS = 6,
    INIT_ERROR_RAY_TRACING = 7,
    INIT_ERROR_RENDERER_BACKEND = 8,
    INIT_ERROR_AUDIO = 9,
    INIT_ERROR_WEATHER = 10,
    INIT_ERROR_WATER = 11,
    INIT_ERROR_PLANT_VFX = 12,
    INIT_ERROR_PHYSICS = 13,
    INIT_ERROR_ECS = 14,
    INIT_ERROR_THREADING = 15,
    INIT_ERROR_VFS = 16,
    INIT_ERROR_UNKNOWN = 99
} InitError;

// Initialization result structure
typedef struct {
    bool success;
    const char *error_message;
    InitError error_type;
    int error_code;
} InitResult;

// Initialization stages (for progress reporting)
typedef enum {
    INIT_STAGE_VALIDATE_CONFIG = 1,
    INIT_STAGE_CREATE_WINDOW = 2,
    INIT_STAGE_INIT_RENDERER = 3,
    INIT_STAGE_INIT_VULKAN = 4,
    INIT_STAGE_INIT_FRAMEBUFFERS = 5,
    INIT_STAGE_INIT_RAY_TRACING = 6,
    INIT_STAGE_INIT_BACKEND = 7,
    INIT_STAGE_INIT_AUDIO = 8,
    INIT_STAGE_INIT_WEATHER = 9,
    INIT_STAGE_INIT_WATER = 10,
    INIT_STAGE_INIT_PLANT_VFX = 11,
    INIT_STAGE_INIT_PHYSICS = 12,
    INIT_STAGE_INIT_ECS = 13,
    INIT_STAGE_INIT_THREADING = 14,
    INIT_STAGE_INIT_VFS = 15,
    INIT_STAGE_COMPLETE = 16
} InitStage;

// Public interface
InitResult init_validate_config(void);
InitResult init_create_window(void);
InitResult init_renderer_system(void);
InitResult init_vulkan_graphics_pipeline(void);
InitResult init_vulkan_framebuffers(void);
InitResult init_ray_tracing_system(void);
InitResult init_renderer_backend(void);
InitResult init_audio_system(void);
InitResult init_weather_system(void);
InitResult init_water_system(void);
InitResult init_plant_vfx_system(void);
InitResult init_physics_system(void);
InitResult init_ecs_system(void);
InitResult init_threading_system(void);
InitResult init_vfs_system(void);

// Cleanup on error
void init_cleanup_on_error(InitError error);
void init_complete_all_systems(void);

#endif // INIT_PIPELINE_H
```

### 1.3 game_context.h

**File Location:** `src/engine/core/game_context.h`

```c
#ifndef GAME_CONTEXT_H
#define GAME_CONTEXT_H

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct IRenderer IRenderer;
typedef struct PhysicsEngine PhysicsEngine;
typedef struct ECSWorld ECSWorld;
typedef struct InputManager InputManager;

// Global game state
typedef struct {
    // Rendering
    IRenderer *renderer;
    bool renderer_ready;

    // Physics
    PhysicsEngine *physics;
    bool physics_ready;

    // ECS
    ECSWorld *ecs;
    bool ecs_ready;

    // Input
    InputManager *input;

    // State flags
    bool running;
    bool should_exit;
    bool is_paused;

    // Timing
    double delta_time;
    double accumulated_time;
    double fixed_timestep;

    // Frame info
    uint64_t frame_count;
    double fps;
} GameState;

// Global game context
extern GameState *g_game_state;

// Game lifecycle
bool game_initialize(void);
void game_shutdown(void);

// Game loop
void game_update(double delta_time);
void game_render(void);

// Game state queries
bool game_is_running(void);
void game_set_running(bool running);
GameState* game_get_context(void);

// Subsystem access
IRenderer* game_get_renderer(void);
PhysicsEngine* game_get_physics(void);
ECSWorld* game_get_ecs(void);

#endif // GAME_CONTEXT_H
```

---

## Step 2: Implement Core Modules

### 2.1 platform_bootstrap.c Implementation Example

**File Location:** `src/engine/core/platform/platform_bootstrap.c`

```c
#include "platform_bootstrap.h"
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

// Platform-specific time implementation
double platform_get_time_seconds(void) {
#ifdef _WIN32
    static LARGE_INTEGER frequency = {0};
    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
    }

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / frequency.QuadPart;

#elif defined(__APPLE__)
    static mach_timebase_info_data_t timebase = {0};
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }

    uint64_t now = mach_absolute_time();
    return (double)(now * timebase.numer) / (timebase.denom * 1e9);

#else  // Linux
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
#endif
}

// Platform initialization
int platform_init(void) {
    // Platform-specific initialization
    return 1; // success
}

void platform_shutdown(void) {
    // Platform-specific cleanup
}

// Window management stubs (expand based on platform)
GLFWwindow *platform_create_window(int width, int height, const char *title,
                                   void *monitor, GLFWwindow *share) {
    // Implementation depends on platform
    return NULL;  // Placeholder
}

int platform_window_should_close(GLFWwindow *window) {
    return 0;  // Placeholder
}

void platform_poll_events(void) {
    // Event polling implementation
}

int platform_get_key(GLFWwindow *window, int key) {
    return 0;  // Placeholder
}

int platform_get_mouse_button(GLFWwindow *window, int button) {
    return 0;  // Placeholder
}

void platform_get_cursor_position(GLFWwindow *window, double *x, double *y) {
    *x = 0.0;
    *y = 0.0;
}

int platform_is_joystick_gamepad(int joystick) {
    return 0;
}

int platform_get_gamepad_state(int joystick, GLFWgamepadstate *state) {
    return 0;
}
```

### 2.2 init_pipeline.c Implementation Skeleton

**File Location:** `src/engine/core/initialization/init_pipeline.c`

```c
#include "init_pipeline.h"
#include "../platform/platform_bootstrap.h"
#include <stdio.h>
#include <string.h>

// Private initialization functions
static InitResult validate_and_load_config(void);
static void log_init_error(InitError error, const char *message);

// Main initialization pipeline
InitResult init_validate_config(void) {
    InitResult result = {.success = false, .error_type = INIT_ERROR_CONFIG};

    // Load and validate configuration
    if (!validate_and_load_config().success) {
        result.error_message = "Failed to validate configuration";
        return result;
    }

    result.success = true;
    return result;
}

InitResult init_create_window(void) {
    InitResult result = {.success = false, .error_type = INIT_ERROR_WINDOW};

    if (!platform_init()) {
        result.error_message = "Failed to initialize platform";
        return result;
    }

    // Window creation will be coordinated at game_context level
    result.success = true;
    return result;
}

InitResult init_renderer_system(void) {
    InitResult result = {.success = false, .error_type = INIT_ERROR_RENDERER};

    // Renderer initialization
    // This would call actual renderer setup
    result.success = true;
    return result;
}

// ... implement other init functions ...

void init_cleanup_on_error(InitError error) {
    log_init_error(error, "Cleaning up on initialization error");

    // Cleanup in reverse order of initialization
    // This ensures proper resource deallocation
}

void init_complete_all_systems(void) {
    // Mark all systems as ready
    printf("All engine systems initialized successfully\n");
}

// Private helper functions
static InitResult validate_and_load_config(void) {
    InitResult result = {.success = true, .error_type = INIT_ERROR_NONE};
    // Implementation
    return result;
}

static void log_init_error(InitError error, const char *message) {
    fprintf(stderr, "INIT ERROR [%d]: %s\n", error, message);
}
```

---

## Step 3: Update CMakeLists.txt

### Before:
```cmake
add_executable(minecraft_v2
    src/engine/core/monolithic_main.c
    # ... other sources
)
```

### After:
```cmake
# Core module sources
set(CORE_SOURCES
    src/engine/core/platform/platform_bootstrap.c
    src/engine/core/initialization/init_pipeline.c
    src/engine/core/input_management.c
    src/engine/core/chunk_generation.c
    src/engine/core/config_management.c
    src/engine/core/progress_tracking.c
    src/engine/core/spawn_generation.c
    src/engine/core/game_context.c
)

# Asset IO sources
set(ASSET_IO_SOURCES
    src/engine/assets/io/asset_io_base.c
    src/engine/assets/io/bundling/bundle_manager.c
    src/engine/assets/io/bundling/bundle_processor.c
    # ... other asset files
)

add_executable(minecraft_v2
    ${CORE_SOURCES}
    ${ASSET_IO_SOURCES}
    # ... other sources
)

# Include directories
target_include_directories(minecraft_v2 PRIVATE
    src/engine/core
    src/engine/core/platform
    src/engine/core/initialization
    src/engine/assets/io
    # ... other includes
)
```

---

## Step 4: Function Mapping Template

**Use this to track function extraction:**

```markdown
# Function Extraction Tracking

## Platform Bootstrap Module
- [x] Extract emscripten_get_now() → platform_bootstrap.c:45
- [x] Extract glfwInit() → platform_bootstrap.c:60
- [x] Extract platform_get_time_seconds() → platform_bootstrap.c:100
- [ ] Extract glfwCreateWindow() → platform_bootstrap.c:LINE
- [ ] Extract glfwGetKey() → platform_bootstrap.c:LINE

## Init Pipeline Module
- [ ] Extract validate_config() → init_pipeline.c:LINE
- [ ] Extract init_window() → init_pipeline.c:LINE
- [ ] Extract init_renderer() → init_pipeline.c:LINE

## Game Context Module
- [ ] Extract game_init() → game_context.c:LINE
- [ ] Extract game_update() → game_context.c:LINE
- [ ] Extract game_render() → game_context.c:LINE
- [ ] Extract game_shutdown() → game_context.c:LINE
```

---

## Step 5: Verification Checklist

### Compilation
- [ ] All new `.h` files have include guards
- [ ] All new `.c` files include their headers
- [ ] No circular dependencies
- [ ] CMakeLists.txt includes all new files
- [ ] `cmake` configuration succeeds
- [ ] `make` compilation succeeds with no warnings
- [ ] No undefined references at link time

### Functionality
- [ ] Game initializes without errors
- [ ] All subsystems report ready
- [ ] Main game loop runs
- [ ] Renderer displays correctly
- [ ] Input works as before
- [ ] No memory leaks detected
- [ ] Performance comparable to before

### Code Quality
- [ ] All functions properly declared in headers
- [ ] All structs in `.h` files
- [ ] Consistent naming conventions
- [ ] Comments explain complex logic
- [ ] No orphaned functions
- [ ] No unused includes

---

## Quick Reference: Where Functions Go

| Function | Module | File |
|----------|--------|------|
| emscripten_get_now() | Platform | platform_bootstrap.c |
| glfwInit() | Platform | platform_bootstrap.c |
| glfwCreateWindow() | Platform | platform_bootstrap.c |
| glfwPollEvents() | Platform | platform_bootstrap.c |
| glfwGetKey() | Platform | platform_bootstrap.c |
| validate_config() | Pipeline | init_pipeline.c |
| init_window() | Pipeline | init_pipeline.c |
| init_renderer() | Pipeline | init_pipeline.c |
| init_physics() | Pipeline | init_pipeline.c |
| init_audio_system() | Pipeline | init_pipeline.c |
| chunk_generation_job() | Chunks | chunk_generation.c |
| find_suitable_spawn_point() | Spawn | spawn_generation.c |
| game_init() | Context | game_context.c |
| game_update() | Context | game_context.c |
| game_render() | Context | game_context.c |
| game_shutdown() | Context | game_context.c |

---

## What This Achieves

✓ **Clear Separation of Concerns:** Platform, initialization, game state are separate
✓ **Proper Headers:** All public interfaces defined in `.h` files
✓ **Centralized Helpers:** asset_io_base.c provides common functionality
✓ **Future Development:** Easy to add new initialization stages
✓ **Testability:** Each module can be tested independently
✓ **Maintainability:** Clear organization makes code easier to navigate
✓ **Scalability:** Adding new features doesn't bloat any single file

