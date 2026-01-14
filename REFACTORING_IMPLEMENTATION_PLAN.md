# Comprehensive Engine Refactoring & Consolidation Plan

**Status:** READY FOR IMPLEMENTATION
**Date:** January 13, 2026
**Priority:** HIGH - Enables future development and maintenance

---

## Executive Summary

Your codebase has 2 monolithic core files (3,510 + 3,283 lines = 6,793 lines) plus fragmented asset management. This plan breaks them into focused, single-responsibility modules with:

- ✓ Proper header file organization
- ✓ Centralized helper functions
- ✓ Clear module boundaries
- ✓ Future-proof architecture
- ✓ Proper linking and dependencies

---

## PHASE 1: Core Engine Refactoring (monolithic_main.c)

### Current State
**File:** `src/engine/core/monolithic_main.c` (3,510 lines)
**Issues:** Handles 18+ distinct responsibilities
**Solution:** Break into 7 focused modules

### Module Breakdown

#### Module 1: Platform Bootstrap (350 lines)
**File:** `src/engine/core/platform/platform_bootstrap.c`
**Header:** `src/engine/core/platform/platform_bootstrap.h`

**Responsibility:** Cross-platform initialization stubs
**Contains:**
- `emscripten_get_now()` - Web timing
- `glfwInit()`, `glfwTerminate()` - Window management stubs
- `glfwCreateWindow()`, `glfwWindowShouldClose()` - Window lifecycle
- `glfwPollEvents()`, `glfwGetKey()` - Input polling
- `glfwGetCursorPos()`, `glfwGetMouseButton()` - Mouse input
- `glfwGetTime()` - Timing
- `now_seconds()` - Platform-specific timer
- Platform-specific #ifdefs for different backends

**Functions to Extract:**
```c
static inline double emscripten_get_now(void)
static inline int glfwInit(void)
static inline void glfwTerminate(void)
static inline GLFWwindow *glfwCreateWindow(...)
static inline int glfwWindowShouldClose(GLFWwindow *w)
static inline void glfwPollEvents(void)
static inline int glfwGetKey(GLFWwindow *w, int key)
static inline void glfwGetCursorPos(GLFWwindow *w, double *x, double *y)
static inline int glfwGetMouseButton(GLFWwindow *w, int button)
static inline double glfwGetTime(void)
static inline int glfwJoystickIsGamepad(int jid)
static inline int glfwGetGamepadState(int jid, GLFWgamepadstate *state)
static inline double now_seconds(void)
```

**Header Content:**
```c
#ifndef PLATFORM_BOOTSTRAP_H
#define PLATFORM_BOOTSTRAP_H

typedef void *GLFWwindow;
typedef struct { /* gamepad state */ } GLFWgamepadstate;

// Platform initialization
int platform_init(void);
void platform_shutdown(void);
double platform_get_time_seconds(void);
// Input polling functions
int platform_get_key(GLFWwindow *w, int key);
int platform_get_mouse_button(GLFWwindow *w, int button);
// Window functions
GLFWwindow *platform_create_window(int w, int h, const char *title);
int platform_window_should_close(GLFWwindow *w);

#endif
```

---

#### Module 2: Initialization Pipeline (650 lines)
**File:** `src/engine/core/initialization/init_pipeline.c`
**Header:** `src/engine/core/initialization/init_pipeline.h`

**Responsibility:** Coordinated system initialization
**Contains:**
- `validate_config()` - Configuration validation
- `init_window()` - Window creation
- `init_renderer()` - Renderer setup
- `init_vulkan_graphics_pipeline()` - Vulkan graphics
- `init_vulkan_framebuffers()` - Framebuffer creation
- `init_ray_tracing()` - Ray tracing setup
- `init_renderer_backend()` - Backend selection
- `init_audio_system()` - Audio initialization
- `init_weather_system()` - Weather system
- `init_water_system()` - Water simulation
- `init_plant_vfx()` - Plant visual effects
- `init_physics()` - Physics engine
- `init_ecs()` - Entity component system
- `init_threading()` - Job system
- `init_vfs()` - Virtual file system
- `cleanup_on_error()` - Error cleanup sequence

**InitResult Struct:**
```c
typedef struct {
    bool success;
    const char *error_message;
    int error_code;
    InitError error_type;
} InitResult;

typedef enum {
    INIT_ERROR_NONE,
    INIT_ERROR_CONFIG,
    INIT_ERROR_WINDOW,
    INIT_ERROR_RENDERER,
    INIT_ERROR_VULKAN,
    INIT_ERROR_PHYSICS,
    INIT_ERROR_AUDIO,
    // ... etc
} InitError;
```

**Header Content:**
```c
#ifndef INIT_PIPELINE_H
#define INIT_PIPELINE_H

typedef struct {
    bool success;
    const char *error_message;
    InitError error_type;
} InitResult;

typedef enum { INIT_ERROR_NONE, /* ... */ } InitError;

// Initialization stages
InitResult init_validate_config(void);
InitResult init_window(void);
InitResult init_renderer(void);
InitResult init_physics(void);
InitResult init_audio(void);
InitResult init_threading(void);
// Cleanup
void init_cleanup_on_error(InitError error);

#endif
```

---

#### Module 3: Input Management (250 lines)
**File:** `src/engine/core/input_management.c`
**Header:** `src/engine/core/input_management.h`

**Responsibility:** Input device and profile management
**Contains:**
- `render_input_profile_menu()` - Input UI rendering
- `input_profiles_active_index()` - Profile queries
- Input configuration/validation
- Profile switching logic
- Gamepad state management

**Functions to Extract:**
```c
static u32 input_profiles_active_index(const InputProfiles *profiles)
static void render_input_profile_menu(void)
```

---

#### Module 4: Chunk Generation (400 lines)
**File:** `src/engine/core/chunk_generation.c`
**Header:** `src/engine/core/chunk_generation.h`

**Responsibility:** Chunk generation job scheduling
**Contains:**
- `chunk_generation_job()` - Job function
- `mesh_generation_job()` - Mesh generation
- `ChunkGenQueueEntry` struct
- `ChunkGenAnalytics` struct
- Queue management
- Analytics tracking

**Structs:**
```c
typedef struct {
    // chunk data
} ChunkGenQueueEntry;

typedef struct {
    f32 avg_gen_ms;
    f32 avg_mesh_ms;
    u32 total_chunks_generated;
} ChunkGenAnalytics;
```

---

#### Module 5: Configuration Management (200 lines)
**File:** `src/engine/core/config_management.c`
**Header:** `src/engine/core/config_management.h`

**Responsibility:** Configuration presets and validation
**Contains:**
- `config_apply_preset()` - Preset application
- `ConfigPreset` enum
- Configuration validation
- Preset definitions

---

#### Module 6: Progress & Error Tracking (300 lines)
**File:** `src/engine/core/progress_tracking.c`
**Header:** `src/engine/core/progress_tracking.h`

**Responsibility:** UI feedback during initialization
**Contains:**
- `init_progress_start()` - Progress start
- `init_progress_update_stage()` - Stage updates
- `init_progress_complete()` - Completion
- `error_dialog_show()` - Error UI
- `error_dialog_update()` - Error animation
- `crash_reporter_log_error()` - Crash logging
- `graceful_shutdown_start()` - Shutdown feedback

**Structs:**
```c
typedef struct {
    const char *stage_name;
    f32 progress;
    f32 total_stages;
} InitProgress;

typedef struct {
    bool active;
    const char *title;
    const char *message;
    f32 time_displayed;
} ErrorDialog;
```

---

#### Module 7: Spawn Point Generation (300 lines)
**File:** `src/engine/core/spawn_generation.c`
**Header:** `src/engine/core/spawn_generation.h`

**Responsibility:** Player spawn location finding
**Contains:**
- `find_suitable_spawn_point()` - Spawn search
- `find_surface_level()` - Terrain scanning
- `is_spawn_location_valid()` - Location validation
- `is_area_flat()` - Flatness checking
- `evaluate_spawn_quality()` - Quality scoring
- `async_spawn_init()` - Async generation
- `spawn_marker_init()`, `spawn_marker_update()`, `spawn_marker_render()` - Marker rendering

---

### New Module 8: Core Context (Game State) (250 lines)
**File:** `src/engine/core/game_context.c`
**Header:** `src/engine/core/game_context.h`

**Responsibility:** Global game state and lifecycle
**Contains:**
- `GameState` struct (moved from monolithic)
- `game_init()` - Full initialization sequence
- `game_update()` - Main update logic
- `game_render()` - Rendering coordination
- `game_shutdown()` - Cleanup
- `game_loop()` - Main loop wrapper

**Struct:**
```c
typedef struct {
    // global state
    IRenderer *renderer;
    PhysicsEngine *physics;
    ECSWorld *ecs;
    // ... other systems
} GameState;
```

---

## PHASE 2: Asset Management Consolidation

### Current Problem
Asset IO files use inconsistent naming: `manager_01.c`, `processor_04.c`, `renderer_03.c`

### Solution: Consolidate & Rename

**Current Structure:**
```
src/engine/assets/io/
├── bundling/
│   ├── manager_01.c        → bundle_manager.c
│   ├── processor_04.c      → bundle_processor.c
│   ├── renderer_03.c       → bundle_renderer.c
│
├── caching/
│   ├── manager_01.c        → cache_manager.c
│   ├── processor_04.c      → cache_processor.c
│   ├── renderer_03.c       → cache_renderer.c
│
├── export/
│   ├── manager_01.c        → export_manager.c
│   ├── processor_04.c      → export_processor.c
│   ├── renderer_03.c       → export_renderer.c
│
├── scene/
│   ├── manager_01.c        → scene_manager.c
│   ├── processor_04.c      → scene_processor.c
│   ├── renderer_03.c       → scene_renderer.c
```

**Pattern:** Each subsystem (bundling, caching, export, scene) has:
- Manager: Lifecycle and state management
- Processor: Data processing and transformation
- Renderer: Output/serialization

### Create: Asset IO Base Library

**File:** `src/engine/assets/io/asset_io_base.c`
**Header:** `src/engine/assets/io/asset_io_base.h`

**Purpose:** Centralized helper functions for all asset IO modules

**Contains:**
```c
// Memory management
void* asset_io_alloc(size_t size, const char *context);
void asset_io_free(void *ptr);

// Error handling
typedef struct {
    int error_code;
    const char *message;
    const char *context;
} AssetIOError;

AssetIOError asset_io_error_create(int code, const char *msg);
void asset_io_error_log(AssetIOError err);

// Common structures
typedef struct {
    u64 offset;
    u64 size;
    u32 checksum;
} AssetIOHeader;

// Validation
bool asset_io_validate_header(AssetIOHeader *header);
bool asset_io_validate_file(const char *path);

// Progress tracking
typedef void (*AssetIOProgressCallback)(f32 progress, const char *status);
void asset_io_set_progress_callback(AssetIOProgressCallback cb);
void asset_io_report_progress(f32 progress, const char *status);
```

---

## PHASE 3: Verify Linking & Dependencies

### Create Dependency Map

**File:** `src/engine/core/CORE_DEPENDENCIES.md`

```
monolithic_main.c (DEPRECATED - being removed)
├── Depends on: platform_bootstrap
├── Depends on: init_pipeline
├── Depends on: input_management
├── Depends on: chunk_generation
├── Depends on: config_management
├── Depends on: progress_tracking
├── Depends on: spawn_generation
└── Depends on: game_context

game_context.c (NEW ENTRY POINT)
├── Depends on: init_pipeline
├── Depends on: physics
├── Depends on: renderer
├── Depends on: ecs
└── Depends on: all subsystems

init_pipeline.c
├── Calls: platform_bootstrap functions
├── Calls: init_renderer()
├── Calls: init_physics()
├── Calls: init_audio()
└── Calls: init_threading()
```

### Update CMakeLists.txt

**Change from:**
```cmake
add_executable(minecraft_v2
    src/engine/core/monolithic_main.c
)
```

**Change to:**
```cmake
add_executable(minecraft_v2
    # Core modules
    src/engine/core/game_context.c
    src/engine/core/initialization/init_pipeline.c
    src/engine/core/platform/platform_bootstrap.c
    src/engine/core/input_management.c
    src/engine/core/chunk_generation.c
    src/engine/core/config_management.c
    src/engine/core/progress_tracking.c
    src/engine/core/spawn_generation.c

    # Asset IO base
    src/engine/assets/io/asset_io_base.c

    # ... rest of build
)
```

---

## PHASE 4: Function Extraction Checklist

### monolithic_main.c Functions to Extract

**Platform Bootstrap (9 functions)**
- [ ] `emscripten_get_now()` → platform_bootstrap.c
- [ ] `glfwInit()` → platform_bootstrap.c
- [ ] `glfwCreateWindow()` → platform_bootstrap.c
- [ ] `glfwGetKey()` → platform_bootstrap.c
- [ ] `glfwGetCursorPos()` → platform_bootstrap.c
- [ ] `glfwGetMouseButton()` → platform_bootstrap.c
- [ ] `glfwGetTime()` → platform_bootstrap.c
- [ ] `glfwJoystickIsGamepad()` → platform_bootstrap.c
- [ ] `glfwGetGamepadState()` → platform_bootstrap.c

**Input Management (2 functions)**
- [ ] `input_profiles_active_index()` → input_management.c
- [ ] `render_input_profile_menu()` → input_management.c

**Chunk Generation (2 functions)**
- [ ] `chunk_generation_job()` → chunk_generation.c
- [ ] `mesh_generation_job()` → chunk_generation.c

**Configuration (1 function)**
- [ ] `config_apply_preset()` → config_management.c

**Spawn Generation (5 functions)**
- [ ] `find_surface_level()` → spawn_generation.c
- [ ] `is_spawn_location_valid()` → spawn_generation.c
- [ ] `is_area_flat()` → spawn_generation.c
- [ ] `evaluate_spawn_quality()` → spawn_generation.c
- [ ] `find_suitable_spawn_point()` → spawn_generation.c

**Initialization Pipeline (15+ functions)**
- [ ] `validate_config()` → init_pipeline.c
- [ ] `init_window()` → init_pipeline.c
- [ ] `init_renderer()` → init_pipeline.c
- [ ] `init_vulkan_graphics_pipeline()` → init_pipeline.c
- [ ] `init_vulkan_framebuffers()` → init_pipeline.c
- [ ] `init_ray_tracing()` → init_pipeline.c
- [ ] `init_renderer_backend()` → init_pipeline.c
- [ ] `init_audio_system()` → init_pipeline.c
- [ ] `init_weather_system()` → init_pipeline.c
- [ ] `init_water_system()` → init_pipeline.c
- [ ] `init_plant_vfx()` → init_pipeline.c
- [ ] `init_physics()` → init_pipeline.c
- [ ] `init_ecs()` → init_pipeline.c
- [ ] `init_threading()` → init_pipeline.c
- [ ] `init_vfs()` → init_pipeline.c
- [ ] `cleanup_on_error()` → init_pipeline.c

**Progress & Error Tracking (10+ functions)**
- [ ] `init_progress_start()` → progress_tracking.c
- [ ] `init_progress_update_stage()` → progress_tracking.c
- [ ] `init_progress_complete()` → progress_tracking.c
- [ ] `error_dialog_show()` → progress_tracking.c
- [ ] `error_dialog_update()` → progress_tracking.c
- [ ] `error_dialog_close()` → progress_tracking.c
- [ ] `crash_reporter_init()` → progress_tracking.c
- [ ] `crash_handler()` → progress_tracking.c
- [ ] `crash_reporter_log_error()` → progress_tracking.c
- [ ] `graceful_shutdown_start()` → progress_tracking.c

**Game Context (4 functions)**
- [ ] `game_init()` → game_context.c
- [ ] `game_update()` → game_context.c
- [ ] `game_render()` → game_context.c
- [ ] `game_shutdown()` → game_context.c

---

## Implementation Order

### Step 1: Create Header Files (1 day)
1. Create all `.h` files in proper locations
2. Define all structs, enums, function signatures
3. Add include guards and necessary #includes

### Step 2: Create Implementation Files (2 days)
1. Create `.c` files in folders
2. Move extracted functions
3. Update #includes
4. Update struct definitions

### Step 3: Update Linking (1 day)
1. Update CMakeLists.txt
2. Add new source files to build
3. Remove monolithic_main.c from build

### Step 4: Compile & Test (1 day)
1. Run `cmake` and `make`
2. Fix compilation errors
3. Run basic functionality tests
4. Commit changes

### Step 5: Delete Obsolete Files (1 day)
1. Verify no references to monolithic_main.c
2. Delete monolithic_main.c
3. Commit deletion
4. Test build

**Total Time:** 5-6 days with proper verification

---

## Benefits of This Refactoring

✓ **Modularity:** Each file has single responsibility (< 700 lines max)
✓ **Maintainability:** Clear function grouping by purpose
✓ **Testability:** Each module can be unit tested independently
✓ **Scalability:** Easy to add new initialization stages
✓ **Clarity:** Developers know where to find specific functionality
✓ **Future-proof:** Easy to add new platforms, renderers, physics engines

---

## Success Criteria

- [ ] All 7-8 new modules created with proper headers
- [ ] All functions extracted from monolithic_main.c
- [ ] Build succeeds without warnings
- [ ] All modules properly linked
- [ ] CMakeLists.txt updated
- [ ] Game runs identically to before
- [ ] monolithic_main.c deleted
- [ ] Code compiles cleanly on all platforms

