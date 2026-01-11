/**
 * @file swift_bridge.c
 * @brief Swift bridge implementation for engine integration
 */

#include "swift_bridge.h"
#include "core/logger.h"
#include "core/engine.h"
#include "core/types.h"
#include "math/math.h"
#include "rendering/renderer.h"
#include "audio/audio_system.h"
// #include "physics/physics_world.h" // Disabled due to missing header
// #include "assets/asset_manager.h" // Disabled due to missing header
// #include "input/input_manager.h" // Disabled due to missing header
#include "core/memory.h"
#include <time.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

// Global engine state
static bool g_engine_initialized = false;
static bool g_engine_running = false;
static SwiftEventCallback g_event_callback = NULL;
static SwiftEngineConfig g_current_config = {0};

// Internal helper functions
static uint64_t get_timestamp(void);
static void dispatch_event(const SwiftEvent* event);
static void update_performance_stats(void);

// Engine lifecycle management
int engine_swift_init(const char* config_path) {
    if (g_engine_initialized) {
        LOG_WARN("Engine already initialized");
        return 0;
    }
    
    LOG_INFO("Initializing engine from Swift bridge");
    
    // Load configuration if provided
    if (config_path) {
        if (engine_swift_load_config(config_path, &g_current_config) != 0) {
            LOG_WARN("Failed to load config, using defaults");
            // Set default configuration
            g_current_config.window_width = 1920;
            g_current_config.window_height = 1080;
            g_current_config.fullscreen = false;
            g_current_config.vsync = true;
            g_current_config.msaa_samples = 4;
            g_current_config.master_volume = 1.0f;
            g_current_config.music_volume = 0.8f;
            g_current_config.sfx_volume = 1.0f;
            g_current_config.enable_physics = true;
            g_current_config.enable_audio = true;
            strcpy(g_current_config.log_level, "INFO");
        }
    }
    
    // Initialize core engine systems
    if (engine_init() != 0) {
        LOG_ERROR("Failed to initialize core engine");
        return -1;
    }
    
    // Initialize renderer
    RendererConfig renderer_config = {
        .window_width = g_current_config.window_width,
        .window_height = g_current_config.window_height,
        .fullscreen = g_current_config.fullscreen,
        .vsync = g_current_config.vsync,
        .msaa_samples = g_current_config.msaa_samples
    };
    
    if (renderer_init(&renderer_config) != 0) {
        LOG_ERROR("Failed to initialize renderer");
        engine_shutdown();
        return -1;
    }
    
    // Initialize audio system if enabled
    if (g_current_config.enable_audio) {
        if (audio_system_init() != 0) {
            LOG_WARN("Failed to initialize audio system");
        }
    }
    
    // Initialize physics system if enabled
    if (g_current_config.enable_physics) {
        if (physics_world_init() != 0) {
            LOG_WARN("Failed to initialize physics system");
        }
    }
    
    // Initialize asset manager
    if (asset_manager_init() != 0) {
        LOG_ERROR("Failed to initialize asset manager");
        engine_shutdown();
        renderer_shutdown();
        return -1;
    }
    
    // Initialize input system
    if (input_manager_init() != 0) {
        LOG_ERROR("Failed to initialize input manager");
        engine_shutdown();
        renderer_shutdown();
        asset_manager_shutdown();
        return -1;
    }
    
    g_engine_initialized = true;
    g_engine_running = true;
    
    LOG_INFO("Engine initialized successfully");
    return 0;
}

int engine_swift_shutdown(void) {
    if (!g_engine_initialized) {
        LOG_WARN("Engine not initialized");
        return 0;
    }
    
    LOG_INFO("Shutting down engine from Swift bridge");
    
    g_engine_running = false;
    
    // Shutdown systems in reverse order
    input_manager_shutdown();
    asset_manager_shutdown();
    
    if (g_current_config.enable_physics) {
        physics_world_shutdown();
    }
    
    if (g_current_config.enable_audio) {
        audio_system_shutdown();
    }
    
    renderer_shutdown();
    engine_shutdown();
    
    g_engine_initialized = false;
    g_event_callback = NULL;
    
    LOG_INFO("Engine shutdown complete");
    return 0;
}

int engine_swift_update(float delta_time) {
    if (!g_engine_initialized || !g_engine_running) {
        return -1;
    }
    
    // Update input system
    input_manager_update(delta_time);
    
    // Update physics system if enabled
    if (g_current_config.enable_physics) {
        physics_world_update(delta_time);
    }
    
    // Update audio system if enabled
    if (g_current_config.enable_audio) {
        audio_system_update(delta_time);
    }
    
    // Update engine core
    engine_update(delta_time);
    
    // Update performance stats
    update_performance_stats();
    
    return 0;
}

int engine_swift_render(void) {
    if (!g_engine_initialized || !g_engine_running) {
        return -1;
    }
    
    return renderer_render_frame();
}

int engine_swift_resize(uint32_t width, uint32_t height) {
    if (!g_engine_initialized) {
        return -1;
    }
    
    g_current_config.window_width = width;
    g_current_config.window_height = height;
    
    // Dispatch resize event
    SwiftEvent event = {
        .type = SWIFT_EVENT_WINDOW_RESIZE,
        .timestamp = get_timestamp(),
        .data.window_resize = {width, height}
    };
    dispatch_event(&event);
    
    return renderer_resize(width, height);
}

// Engine state queries
bool engine_swift_is_initialized(void) {
    return g_engine_initialized;
}

bool engine_swift_is_running(void) {
    return g_engine_running;
}

float engine_swift_get_fps(void) {
    return renderer_get_fps();
}

uint32_t engine_swift_get_frame_count(void) {
    return engine_get_frame_count();
}

float engine_swift_get_delta_time(void) {
    return engine_get_delta_time();
}

// Rendering and graphics
int engine_swift_set_clear_color(float r, float g, float b, float a) {
    if (!g_engine_initialized) return -1;
    return renderer_set_clear_color(r, g, b, a);
}

int engine_swift_enable_vsync(bool enabled) {
    if (!g_engine_initialized) return -1;
    g_current_config.vsync = enabled;
    return renderer_enable_vsync(enabled);
}

int engine_swift_set_msaa(uint32_t samples) {
    if (!g_engine_initialized) return -1;
    g_current_config.msaa_samples = samples;
    return renderer_set_msaa(samples);
}

int engine_swift_capture_frame(const char* output_path) {
    if (!g_engine_initialized || !output_path) return -1;
    return renderer_capture_frame(output_path);
}

// Camera system
int engine_swift_set_camera(const SwiftCamera* camera) {
    if (!g_engine_initialized || !camera) return -1;
    
    // Convert Swift camera to engine camera format
    Camera engine_camera = {
        .position = {camera->position[0], camera->position[1], camera->position[2]},
        .rotation = {camera->rotation[0], camera->rotation[1], camera->rotation[2]},
        .fov = camera->fov,
        .near_plane = camera->near_plane,
        .far_plane = camera->far_plane
    };
    
    return renderer_set_camera(&engine_camera);
}

int engine_swift_get_camera(SwiftCamera* out_camera) {
    if (!g_engine_initialized || !out_camera) return -1;
    
    Camera engine_camera;
    if (renderer_get_camera(&engine_camera) != 0) {
        return -1;
    }
    
    // Convert engine camera to Swift camera format
    out_camera->position[0] = engine_camera.position.x;
    out_camera->position[1] = engine_camera.position.y;
    out_camera->position[2] = engine_camera.position.z;
    out_camera->rotation[0] = engine_camera.rotation.x;
    out_camera->rotation[1] = engine_camera.rotation.y;
    out_camera->rotation[2] = engine_camera.rotation.z;
    out_camera->fov = engine_camera.fov;
    out_camera->near_plane = engine_camera.near_plane;
    out_camera->far_plane = engine_camera.far_plane;
    
    return 0;
}

int engine_swift_set_camera_mode(int mode) {
    if (!g_engine_initialized) return -1;
    return renderer_set_camera_mode(mode);
}

// Input system
int engine_swift_input_mouse_move(float x, float y) {
    if (!g_engine_initialized) return -1;
    return input_manager_mouse_move(x, y);
}

int engine_swift_input_mouse_click(int button, bool pressed) {
    if (!g_engine_initialized) return -1;
    return input_manager_mouse_click(button, pressed);
}

int engine_swift_input_mouse_scroll(float delta) {
    if (!g_engine_initialized) return -1;
    return input_manager_mouse_scroll(delta);
}

int engine_swift_input_key_press(int key, bool pressed) {
    if (!g_engine_initialized) return -1;
    return input_manager_key_press(key, pressed);
}

int engine_swift_get_mouse_state(SwiftMouseState* out_state) {
    if (!g_engine_initialized || !out_state) return -1;
    
    MouseState mouse_state;
    if (input_manager_get_mouse_state(&mouse_state) != 0) {
        return -1;
    }
    
    // Convert to Swift format
    out_state->x = mouse_state.x;
    out_state->y = mouse_state.y;
    out_state->dx = mouse_state.dx;
    out_state->dy = mouse_state.dy;
    out_state->scroll_delta = mouse_state.scroll_delta;
    
    for (int i = 0; i < 5; i++) {
        out_state->buttons[i] = mouse_state.buttons[i];
    }
    
    return 0;
}

int engine_swift_get_keyboard_state(SwiftKeyboardState* out_state) {
    if (!g_engine_initialized || !out_state) return -1;
    
    KeyboardState keyboard_state;
    if (input_manager_get_keyboard_state(&keyboard_state) != 0) {
        return -1;
    }
    
    // Convert to Swift format
    for (int i = 0; i < 256; i++) {
        out_state->keys[i] = keyboard_state.keys[i];
    }
    
    for (int i = 0; i < 16; i++) {
        out_state->special_keys[i] = keyboard_state.special_keys[i];
    }
    
    return 0;
}

// Asset management
int engine_swift_load_asset(const char* path, const char* type) {
    if (!g_engine_initialized || !path || !type) return -1;
    
    AssetType asset_type = asset_manager_get_type_from_string(type);
    if (asset_type == ASSET_TYPE_UNKNOWN) {
        LOG_ERROR("Unknown asset type: %s", type);
        return -1;
    }
    
    uint32_t asset_id = asset_manager_load(path, asset_type, path);
    if (asset_id == 0) {
        LOG_ERROR("Failed to load asset: %s", path);
        return -1;
    }
    
    // Dispatch asset loaded event
    SwiftEvent event = {
        .type = SWIFT_EVENT_ASSET_LOADED,
        .timestamp = get_timestamp(),
        .data.asset_event = {asset_id, path}
    };
    dispatch_event(&event);
    
    return asset_id;
}

int engine_swift_unload_asset(uint32_t asset_id) {
    if (!g_engine_initialized) return -1;
    return asset_manager_unload(asset_id);
}

int engine_swift_get_asset_info(uint32_t asset_id, SwiftAssetInfo* out_info) {
    if (!g_engine_initialized || !out_info) return -1;
    
    Asset* asset = asset_manager_get_asset(asset_id);
    if (!asset) {
        return -1;
    }
    
    // Convert to Swift format
    out_info->id = asset->id;
    strncpy(out_info->name, asset->name, sizeof(out_info->name) - 1);
    strncpy(out_info->type, asset_manager_get_type_string(asset->type), sizeof(out_info->type) - 1);
    strncpy(out_info->path, asset->file_path, sizeof(out_info->path) - 1);
    out_info->loaded = asset->loaded;
    out_info->load_progress = asset->load_progress;
    
    return 0;
}

int engine_swift_get_all_assets(SwiftAssetInfo* out_assets, uint32_t max_count, uint32_t* out_count) {
    if (!g_engine_initialized || !out_assets || !out_count) return -1;
    
    Asset* assets = asset_manager_get_all_assets();
    uint32_t total_count = asset_manager_get_asset_count();
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < total_count && count < max_count; i++) {
        if (assets[i].id != 0) {
            // Convert to Swift format
            out_assets[count].id = assets[i].id;
            strncpy(out_assets[count].name, assets[i].name, sizeof(out_assets[count].name) - 1);
            strncpy(out_assets[count].type, asset_manager_get_type_string(assets[i].type), sizeof(out_assets[count].type) - 1);
            strncpy(out_assets[count].path, assets[i].file_path, sizeof(out_assets[count].path) - 1);
            out_assets[count].loaded = assets[i].loaded;
            out_assets[count].load_progress = assets[i].load_progress;
            count++;
        }
    }
    
    *out_count = count;
    return 0;
}

// Scene management
int engine_swift_create_entity(const char* name, uint32_t* out_id) {
    if (!g_engine_initialized || !name || !out_id) return -1;
    
    uint32_t entity_id = engine_create_entity(name);
    if (entity_id == 0) {
        return -1;
    }
    
    *out_id = entity_id;
    return 0;
}

int engine_swift_destroy_entity(uint32_t entity_id) {
    if (!g_engine_initialized) return -1;
    return engine_destroy_entity(entity_id);
}

int engine_swift_set_entity_transform(uint32_t entity_id, const float* transform_matrix) {
    if (!g_engine_initialized || !transform_matrix) return -1;
    
    Transform transform;
    memcpy(&transform.matrix, transform_matrix, sizeof(float) * 16);
    
    return engine_set_entity_transform(entity_id, &transform);
}

int engine_swift_get_entity_transform(uint32_t entity_id, float* out_transform_matrix) {
    if (!g_engine_initialized || !out_transform_matrix) return -1;
    
    Transform transform;
    if (engine_get_entity_transform(entity_id, &transform) != 0) {
        return -1;
    }
    
    memcpy(out_transform_matrix, &transform.matrix, sizeof(float) * 16);
    return 0;
}

int engine_swift_set_entity_active(uint32_t entity_id, bool active) {
    if (!g_engine_initialized) return -1;
    return engine_set_entity_active(entity_id, active);
}

int engine_swift_set_entity_visible(uint32_t entity_id, bool visible) {
    if (!g_engine_initialized) return -1;
    return engine_set_entity_visible(entity_id, visible);
}

int engine_swift_get_all_entities(SwiftEntity* out_entities, uint32_t max_count, uint32_t* out_count) {
    if (!g_engine_initialized || !out_entities || !out_count) return -1;
    
    Entity* entities = engine_get_all_entities();
    uint32_t total_count = engine_get_entity_count();
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < total_count && count < max_count; i++) {
        if (entities[i].id != 0) {
            // Convert to Swift format
            out_entities[count].id = entities[i].id;
            strncpy(out_entities[count].name, entities[i].name, sizeof(out_entities[count].name) - 1);
            memcpy(&out_entities[count].transform, &entities[i].transform.matrix, sizeof(float) * 16);
            out_entities[count].active = entities[i].active;
            out_entities[count].visible = entities[i].visible;
            count++;
        }
    }
    
    *out_count = count;
    return 0;
}

// Physics system
int engine_swift_add_rigidbody(uint32_t entity_id, const SwiftRigidBody* body) {
    if (!g_engine_initialized || !body) return -1;
    
    RigidBody rigid_body = {
        .position = {body->position[0], body->position[1], body->position[2]},
        .velocity = {body->velocity[0], body->velocity[1], body->velocity[2]},
        .angular_velocity = {body->angular_velocity[0], body->angular_velocity[1], body->angular_velocity[2]},
        .mass = body->mass,
        .kinematic = body->kinematic,
        .sleeping = body->sleeping
    };
    
    return physics_world_add_rigidbody(entity_id, &rigid_body);
}

int engine_swift_remove_rigidbody(uint32_t entity_id) {
    if (!g_engine_initialized) return -1;
    return physics_world_remove_rigidbody(entity_id);
}

int engine_swift_get_rigidbody(uint32_t entity_id, SwiftRigidBody* out_body) {
    if (!g_engine_initialized || !out_body) return -1;
    
    RigidBody rigid_body;
    if (physics_world_get_rigidbody(entity_id, &rigid_body) != 0) {
        return -1;
    }
    
    // Convert to Swift format
    out_body->position[0] = rigid_body.position.x;
    out_body->position[1] = rigid_body.position.y;
    out_body->position[2] = rigid_body.position.z;
    out_body->velocity[0] = rigid_body.velocity.x;
    out_body->velocity[1] = rigid_body.velocity.y;
    out_body->velocity[2] = rigid_body.velocity.z;
    out_body->angular_velocity[0] = rigid_body.angular_velocity.x;
    out_body->angular_velocity[1] = rigid_body.angular_velocity.y;
    out_body->angular_velocity[2] = rigid_body.angular_velocity.z;
    out_body->mass = rigid_body.mass;
    out_body->kinematic = rigid_body.kinematic;
    out_body->sleeping = rigid_body.sleeping;
    
    return 0;
}

int engine_swift_set_gravity(float x, float y, float z) {
    if (!g_engine_initialized) return -1;
    return physics_world_set_gravity(x, y, z);
}

int engine_swift_set_physics_timestep(float timestep) {
    if (!g_engine_initialized) return -1;
    return physics_world_set_timestep(timestep);
}

// Audio system
int engine_swift_play_audio(const char* audio_path, uint32_t* out_id) {
    if (!g_engine_initialized || !audio_path || !out_id) return -1;
    
    uint32_t audio_id = audio_system_play(audio_path);
    if (audio_id == 0) {
        return -1;
    }
    
    *out_id = audio_id;
    return 0;
}

int engine_swift_stop_audio(uint32_t audio_id) {
    if (!g_engine_initialized) return -1;
    return audio_system_stop(audio_id);
}

int engine_swift_pause_audio(uint32_t audio_id) {
    if (!g_engine_initialized) return -1;
    return audio_system_pause(audio_id);
}

int engine_swift_resume_audio(uint32_t audio_id) {
    if (!g_engine_initialized) return -1;
    return audio_system_resume(audio_id);
}

int engine_swift_set_audio_volume(uint32_t audio_id, float volume) {
    if (!g_engine_initialized) return -1;
    return audio_system_set_volume(audio_id, volume);
}

int engine_swift_set_audio_pitch(uint32_t audio_id, float pitch) {
    if (!g_engine_initialized) return -1;
    return audio_system_set_pitch(audio_id, pitch);
}

int engine_swift_set_audio_position(uint32_t audio_id, float x, float y, float z) {
    if (!g_engine_initialized) return -1;
    return audio_system_set_position(audio_id, x, y, z);
}

int engine_swift_get_audio_sources(SwiftAudioSource* out_sources, uint32_t max_count, uint32_t* out_count) {
    if (!g_engine_initialized || !out_sources || !out_count) return -1;
    
    AudioSource* sources = audio_system_get_all_sources();
    uint32_t total_count = audio_system_get_source_count();
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < total_count && count < max_count; i++) {
        if (sources[i].id != 0) {
            // Convert to Swift format
            out_sources[count].id = sources[i].id;
            strncpy(out_sources[count].name, sources[i].name, sizeof(out_sources[count].name) - 1);
            strncpy(out_sources[count].path, sources[i].file_path, sizeof(out_sources[count].path) - 1);
            out_sources[count].playing = sources[i].playing;
            out_sources[count].looping = sources[i].looping;
            out_sources[count].volume = sources[i].volume;
            out_sources[count].pitch = sources[i].pitch;
            out_sources[count].position = sources[i].position;
            count++;
        }
    }
    
    *out_count = count;
    return 0;
}

// Debug and profiling
int engine_swift_get_performance_stats(SwiftPerformanceStats* out_stats) {
    if (!g_engine_initialized || !out_stats) return -1;
    
    PerformanceStats stats;
    if (renderer_get_performance_stats(&stats) != 0) {
        return -1;
    }
    
    // Convert to Swift format
    out_stats->fps = stats.fps;
    out_stats->frame_time = stats.frame_time;
    out_stats->cpu_time = stats.cpu_time;
    out_stats->gpu_time = stats.gpu_time;
    out_stats->draw_calls = stats.draw_calls;
    out_stats->triangles = stats.triangles;
    out_stats->vertices = stats.vertices;
    out_stats->memory_usage = stats.memory_usage;
    
    return 0;
}

int engine_swift_enable_profiling(bool enabled) {
    if (!g_engine_initialized) return -1;
    return renderer_enable_profiling(enabled);
}

int engine_swift_take_screenshot(const char* output_path) {
    if (!g_engine_initialized || !output_path) return -1;
    return renderer_take_screenshot(output_path);
}

int engine_swift_start_profiling_region(const char* region_name) {
    if (!g_engine_initialized || !region_name) return -1;
    return renderer_start_profiling_region(region_name);
}

int engine_swift_end_profiling_region(const char* region_name) {
    if (!g_engine_initialized || !region_name) return -1;
    return renderer_end_profiling_region(region_name);
}

// Configuration and settings
int engine_swift_load_config(const char* config_path, SwiftEngineConfig* out_config) {
    if (!config_path || !out_config) return -1;
    
    // This would load JSON config file
    // For now, return defaults
    memset(out_config, 0, sizeof(SwiftEngineConfig));
    out_config->window_width = 1920;
    out_config->window_height = 1080;
    out_config->fullscreen = false;
    out_config->vsync = true;
    out_config->msaa_samples = 4;
    out_config->master_volume = 1.0f;
    out_config->music_volume = 0.8f;
    out_config->sfx_volume = 1.0f;
    out_config->enable_physics = true;
    out_config->enable_audio = true;
    strcpy(out_config->log_level, "INFO");
    
    return 0;
}

int engine_swift_save_config(const char* config_path, const SwiftEngineConfig* config) {
    if (!config_path || !config) return -1;
    
    // This would save JSON config file
    // For now, just return success
    return 0;
}

int engine_swift_apply_config(const SwiftEngineConfig* config) {
    if (!g_engine_initialized || !config) return -1;
    
    // Apply configuration to engine systems
    g_current_config = *config;
    
    // Apply renderer settings
    renderer_resize(config->window_width, config->window_height);
    renderer_enable_vsync(config->vsync);
    renderer_set_msaa(config->msaa_samples);
    
    // Apply audio settings
    if (g_current_config.enable_audio) {
        audio_system_set_master_volume(config->master_volume);
        audio_system_set_music_volume(config->music_volume);
        audio_system_set_sfx_volume(config->sfx_volume);
    }
    
    return 0;
}

int engine_swift_get_config(SwiftEngineConfig* out_config) {
    if (!out_config) return -1;
    *out_config = g_current_config;
    return 0;
}

// Event system
int engine_swift_set_event_callback(SwiftEventCallback callback) {
    g_event_callback = callback;
    return 0;
}

int engine_swift_poll_events(SwiftEvent* out_events, uint32_t max_count, uint32_t* out_count) {
    if (!out_events || !out_count) return -1;
    
    // This would poll events from engine systems
    // For now, return no events
    *out_count = 0;
    return 0;
}

// Memory management
int engine_swift_get_memory_stats(SwiftMemoryStats* out_stats) {
    if (!out_stats) return -1;
    
    MemoryStats stats;
    if (memory_get_stats(&stats) != 0) {
        return -1;
    }
    
    // Convert to Swift format
    out_stats->total_allocated = stats.total_allocated;
    out_stats->total_freed = stats.total_freed;
    out_stats->current_usage = stats.current_usage;
    out_stats->peak_usage = stats.peak_usage;
    out_stats->allocation_count = stats.allocation_count;
    out_stats->deallocation_count = stats.deallocation_count;
    
    return 0;
}

int engine_swift_enable_memory_tracking(bool enabled) {
    return memory_enable_tracking(enabled);
}

int engine_swift_force_garbage_collection(void) {
    return memory_force_gc();
}

// Utility functions
const char* engine_swift_get_version(void) {
    return engine_get_version();
}

const char* engine_swift_get_build_info(void) {
    return engine_get_build_info();
}

int engine_swift_get_platform_info(char* platform, size_t platform_size) {
    if (!platform) return -1;
    
    const char* platform_str = engine_get_platform();
    strncpy(platform, platform_str, platform_size - 1);
    platform[platform_size - 1] = '\0';
    
    return 0;
}

uint64_t engine_swift_get_time(void) {
    return get_timestamp();
}

float engine_swift_get_time_float(void) {
    return (float)get_timestamp() / 1000.0f;
}

// Internal helper functions
static uint64_t get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

static void dispatch_event(const SwiftEvent* event) {
    if (g_event_callback) {
        g_event_callback(event);
    }
}

static void update_performance_stats(void) {
    // This would update internal performance tracking
    // For now, let the renderer handle it
}
