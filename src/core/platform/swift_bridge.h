#ifndef SWIFT_BRIDGE_H
#define SWIFT_BRIDGE_H

#include <core/types.h>
#include "engine/include/math/math_all.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Engine lifecycle management
int engine_swift_init(const char* config_path);
int engine_swift_shutdown(void);
int engine_swift_update(float delta_time);
int engine_swift_render(void);
int engine_swift_resize(uint32_t width, uint32_t height);

// Engine state queries
bool engine_swift_is_initialized(void);
bool engine_swift_is_running(void);
float engine_swift_get_fps(void);
uint32_t engine_swift_get_frame_count(void);
float engine_swift_get_delta_time(void);

// Rendering and graphics
int engine_swift_set_clear_color(float r, float g, float b, float a);
int engine_swift_enable_vsync(bool enabled);
int engine_swift_set_msaa(uint32_t samples);
int engine_swift_capture_frame(const char* output_path);

// Camera system
typedef struct {
    float position[3];
    float rotation[3];
    float fov;
    float near_plane;
    float far_plane;
} SwiftCamera;

int engine_swift_set_camera(const SwiftCamera* camera);
int engine_swift_get_camera(SwiftCamera* out_camera);
int engine_swift_set_camera_mode(int mode); // 0=first_person, 1=third_person, 2=free

// Input system
typedef struct {
    float x, y;
    float dx, dy;
    bool buttons[5]; // 0=left, 1=right, 2=middle, 3=x1, 4=x2
    float scroll_delta;
} SwiftMouseState;

typedef struct {
    bool keys[256]; // Standard keyboard keys
    bool special_keys[16]; // Special keys (shift, ctrl, alt, etc.)
} SwiftKeyboardState;

int engine_swift_input_mouse_move(float x, float y);
int engine_swift_input_mouse_click(int button, bool pressed);
int engine_swift_input_mouse_scroll(float delta);
int engine_swift_input_key_press(int key, bool pressed);
int engine_swift_get_mouse_state(SwiftMouseState* out_state);
int engine_swift_get_keyboard_state(SwiftKeyboardState* out_state);

// Asset management
typedef struct {
    uint32_t id;
    char name[256];
    char type[64];
    char path[512];
    bool loaded;
    float load_progress;
} SwiftAssetInfo;

int engine_swift_load_asset(const char* path, const char* type);
int engine_swift_unload_asset(uint32_t asset_id);
int engine_swift_get_asset_info(uint32_t asset_id, SwiftAssetInfo* out_info);
int engine_swift_get_all_assets(SwiftAssetInfo* out_assets, uint32_t max_count, uint32_t* out_count);

// Scene management
typedef struct {
    uint32_t id;
    char name[256];
    float transform[16]; // 4x4 matrix
    bool active;
    bool visible;
} SwiftEntity;

int engine_swift_create_entity(const char* name, uint32_t* out_id);
int engine_swift_destroy_entity(uint32_t entity_id);
int engine_swift_set_entity_transform(uint32_t entity_id, const float* transform_matrix);
int engine_swift_get_entity_transform(uint32_t entity_id, float* out_transform_matrix);
int engine_swift_set_entity_active(uint32_t entity_id, bool active);
int engine_swift_set_entity_visible(uint32_t entity_id, bool visible);
int engine_swift_get_all_entities(SwiftEntity* out_entities, uint32_t max_count, uint32_t* out_count);

// Physics system
typedef struct {
    float position[3];
    float velocity[3];
    float angular_velocity[3];
    float mass;
    bool kinematic;
    bool sleeping;
} SwiftRigidBody;

int engine_swift_add_rigidbody(uint32_t entity_id, const SwiftRigidBody* body);
int engine_swift_remove_rigidbody(uint32_t entity_id);
int engine_swift_get_rigidbody(uint32_t entity_id, SwiftRigidBody* out_body);
int engine_swift_set_gravity(float x, float y, float z);
int engine_swift_set_physics_timestep(float timestep);

// Audio system
typedef struct {
    uint32_t id;
    char name[256];
    char path[512];
    bool playing;
    bool looping;
    float volume;
    float pitch;
    Vec3 position;
} SwiftAudioSource;

int engine_swift_play_audio(const char* audio_path, uint32_t* out_id);
int engine_swift_stop_audio(uint32_t audio_id);
int engine_swift_pause_audio(uint32_t audio_id);
int engine_swift_resume_audio(uint32_t audio_id);
int engine_swift_set_audio_volume(uint32_t audio_id, float volume);
int engine_swift_set_audio_pitch(uint32_t audio_id, float pitch);
int engine_swift_set_audio_position(uint32_t audio_id, float x, float y, float z);
int engine_swift_get_audio_sources(SwiftAudioSource* out_sources, uint32_t max_count, uint32_t* out_count);

// Debug and profiling
typedef struct {
    float fps;
    float frame_time;
    float cpu_time;
    float gpu_time;
    uint32_t draw_calls;
    uint32_t triangles;
    uint32_t vertices;
    float memory_usage;
} SwiftPerformanceStats;

int engine_swift_get_performance_stats(SwiftPerformanceStats* out_stats);
int engine_swift_enable_profiling(bool enabled);
int engine_swift_take_screenshot(const char* output_path);
int engine_swift_start_profiling_region(const char* region_name);
int engine_swift_end_profiling_region(const char* region_name);

// Configuration and settings
typedef struct {
    uint32_t window_width;
    uint32_t window_height;
    bool fullscreen;
    bool vsync;
    uint32_t msaa_samples;
    float master_volume;
    float music_volume;
    float sfx_volume;
    bool enable_physics;
    bool enable_audio;
    char log_level[16];
} SwiftEngineConfig;

int engine_swift_load_config(const char* config_path, SwiftEngineConfig* out_config);
int engine_swift_save_config(const char* config_path, const SwiftEngineConfig* config);
int engine_swift_apply_config(const SwiftEngineConfig* config);
int engine_swift_get_config(SwiftEngineConfig* out_config);

// Event system
typedef enum {
    SWIFT_EVENT_NONE = 0,
    SWIFT_EVENT_WINDOW_CLOSE,
    SWIFT_EVENT_WINDOW_RESIZE,
    SWIFT_EVENT_KEY_PRESS,
    SWIFT_EVENT_KEY_RELEASE,
    SWIFT_EVENT_MOUSE_PRESS,
    SWIFT_EVENT_MOUSE_RELEASE,
    SWIFT_EVENT_MOUSE_MOVE,
    SWIFT_EVENT_MOUSE_SCROLL,
    SWIFT_EVENT_ASSET_LOADED,
    SWIFT_EVENT_ASSET_FAILED,
    SWIFT_EVENT_SCENE_LOADED,
    SWIFT_EVENT_ERROR,
    SWIFT_EVENT_COUNT
} SwiftEventType;

typedef struct {
    SwiftEventType type;
    uint64_t timestamp;
    union {
        struct { uint32_t width, height; } window_resize;
        struct { int key; } key_event;
        struct { int button; float x, y; } mouse_event;
        struct { uint32_t asset_id; const char* path; } asset_event;
        struct { const char* message; } error_event;
    } data;
} SwiftEvent;

typedef void (*SwiftEventCallback)(const SwiftEvent* event);

int engine_swift_set_event_callback(SwiftEventCallback callback);
int engine_swift_poll_events(SwiftEvent* out_events, uint32_t max_count, uint32_t* out_count);

// Memory management
typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    uint32_t allocation_count;
    uint32_t deallocation_count;
} SwiftMemoryStats;

int engine_swift_get_memory_stats(SwiftMemoryStats* out_stats);
int engine_swift_enable_memory_tracking(bool enabled);
int engine_swift_force_garbage_collection(void);

// Utility functions
const char* engine_swift_get_version(void);
const char* engine_swift_get_build_info(void);
int engine_swift_get_platform_info(char* platform, size_t platform_size);
uint64_t engine_swift_get_time(void);
float engine_swift_get_time_float(void);

#ifdef __cplusplus
}
#endif

#endif // SWIFT_BRIDGE_H
