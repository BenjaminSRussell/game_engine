// src/core/engine_core.c
//
// Implementation of the engine core abstraction layer
//
#include "../../include/engine/engine_core.h"
#include "../../include/core/logger.h"
#include "../../include/render/camera.h"
#include "../../include/render/vulkan.h"
#include "../../include/physics/physics.h"
#include "../../include/thread/job.h"
#include "../../include/vfs/vfs.h"
#include "../../include/audio/audio_system.h"
#include "../../include/config/config.h"
#include <stdlib.h>
#include <string.h>

// Default configuration
void engine_config_set_defaults(EngineConfig *config) {
    if (!config) return;
    
    config->window_width = 1280;
    config->window_height = 720;
    config->window_title = "Minecraft v2 Engine";
    config->rendering_mode = RENDERING_MODE_3D_VOXEL;
    config->enable_physics = true;
    config->enable_audio = true;
    config->enable_ray_tracing = false;
    config->max_entities = MAX_ENTITIES;
    config->max_components = MAX_COMPONENTS;
    config->physics_fixed_dt = 1.0f / 60.0f;
    config->physics_max_frame_time = 0.25f;
}

EngineConfig engine_config_create_2_5d(u32 width, u32 height) {
    EngineConfig config;
    engine_config_set_defaults(&config);
    config.window_width = width;
    config.window_height = height;
    config.rendering_mode = RENDERING_MODE_2_5D_ISOMETRIC;
    return config;
}

EngineConfig engine_config_create_3d(u32 width, u32 height) {
    EngineConfig config;
    engine_config_set_defaults(&config);
    config.window_width = width;
    config.window_height = height;
    config.rendering_mode = RENDERING_MODE_3D_VOXEL;
    return config;
}

EngineConfig engine_config_create_2d(u32 width, u32 height) {
    EngineConfig config;
    engine_config_set_defaults(&config);
    config.window_width = width;
    config.window_height = height;
    config.rendering_mode = RENDERING_MODE_2D_SPRITE;
    return config;
}

// Engine initialization
bool engine_core_init(EngineCore *engine, const EngineConfig *config) {
    if (!engine || !config) {
        LOG_ERROR("Invalid parameters for engine_core_init");
        return false;
    }
    
    memset(engine, 0, sizeof(EngineCore));
    engine->config = *config;
    engine->current_rendering_mode = config->rendering_mode;
    engine->window_width = config->window_width;
    engine->window_height = config->window_height;
    
    // Initialize ECS
    ecs_world_init(&engine->ecs_world, config->max_entities, 
                   config->max_components, 32);
    
    // Initialize physics if enabled
    if (config->enable_physics) {
        engine->physics_world = (struct PhysicsWorld *)malloc(sizeof(struct PhysicsWorld));
        if (engine->physics_world) {
            physics_world_init(engine->physics_world, vec3(0.0f, -9.81f, 0.0f));
        }
    }
    
    // Initialize thread pool
    engine->thread_pool = (struct ThreadPool *)malloc(sizeof(struct ThreadPool));
    if (engine->thread_pool) {
        thread_pool_init(engine->thread_pool, 4);
    }
    
    // Initialize VFS
    engine->vfs = (struct VFS *)malloc(sizeof(struct VFS));
    if (engine->vfs) {
        vfs_init(engine->vfs);
    }
    
    // Initialize audio if enabled
    if (config->enable_audio) {
        engine->audio_system = (struct AudioSystem *)malloc(sizeof(struct AudioSystem));
        // TODO: Add proper audio system initialization with device enumeration
        // TODO: Add audio format configuration (sample rate, bit depth, channels)
        // TODO: Add audio system unit tests for device management and playback
        // Audio initialization would go here
    }
    
    // Initialize camera
    engine->camera = (struct Camera *)malloc(sizeof(struct Camera));
    if (engine->camera) {
        camera_init(engine->camera, vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
    }
    
    // Initialize renderer based on mode
    RendererType renderer_type = RENDERER_TYPE_VOXEL;
    if (config->rendering_mode == RENDERING_MODE_2_5D_ISOMETRIC || 
        config->rendering_mode == RENDERING_MODE_2_5D_ORTHO) {
        renderer_type = RENDERER_TYPE_SPRITE_3D;
    }
    
    engine->renderer = renderer_create(renderer_type);
    if (!engine->renderer) {
        LOG_ERROR("Failed to create renderer");
        engine_core_shutdown(engine);
        return false;
    }
    
    // Initialize renderer with window (would need actual window handle)
    RendererInitParams renderer_params = {
        .window = engine->window,
        .width = config->window_width,
        .height = config->window_height,
        .type = renderer_type,
        .config = NULL // TODO: Add game config parameter support
    };
    
    // TODO: Add renderer validation and error handling unit tests
    // TODO: Add window handle integration tests
    // TODO: Add renderer mode switching integration tests
    
    if (!engine->renderer->init(engine->renderer, &renderer_params)) {
        LOG_ERROR("Failed to initialize renderer");
        engine_core_shutdown(engine);
        return false;
    }
    
    engine->initialized = true;
    engine->running = true;
    engine->last_frame_time = 0;
    engine->delta_time = 0.016f;
    
    LOG_INFO("Engine core initialized successfully");
    return true;
}

void engine_core_shutdown(EngineCore *engine) {
    if (!engine || !engine->initialized) {
        return;
    }
    
    // Call shutdown callback
    if (engine->on_shutdown) {
        engine->on_shutdown(engine);
    }
    
    // Cleanup renderer
    if (engine->renderer) {
        engine->renderer->cleanup(engine->renderer);
        renderer_destroy(engine->renderer);
        engine->renderer = NULL;
    }
    
    // Cleanup camera
    if (engine->camera) {
        free(engine->camera);
        engine->camera = NULL;
    }
    
    // Cleanup audio
    if (engine->audio_system) {
        free(engine->audio_system);
        engine->audio_system = NULL;
    }
    
    // Cleanup VFS
    if (engine->vfs) {
        vfs_shutdown(engine->vfs);
        free(engine->vfs);
        engine->vfs = NULL;
    }
    
    // Cleanup thread pool
    if (engine->thread_pool) {
        thread_pool_shutdown(engine->thread_pool);
        free(engine->thread_pool);
        engine->thread_pool = NULL;
    }
    
    // Cleanup physics
    if (engine->physics_world) {
        physics_world_free(engine->physics_world);
        free(engine->physics_world);
        engine->physics_world = NULL;
    }
    
    // Cleanup ECS
    ecs_world_free(&engine->ecs_world);
    
    engine->initialized = false;
    engine->running = false;
    
    LOG_INFO("Engine core shut down");
}

void engine_core_update(EngineCore *engine) {
    if (!engine || !engine->initialized || !engine->running) {
        return;
    }
    
    // Calculate delta time
    u64 current_time = 0;
#if PLATFORM_WEB
#if defined(__EMSCRIPTEN__)
    extern double emscripten_get_now(void);
    current_time = (u64)emscripten_get_now();
#else
    current_time = 0;
#endif
#else
    extern double glfwGetTime(void);
    current_time = (u64)(glfwGetTime() * 1000.0);
#endif
    
    if (engine->last_frame_time > 0) {
        engine->delta_time = (f32)(current_time - engine->last_frame_time) / 1000.0f;
    } else {
        engine->delta_time = 0.016f;
    }
    engine->last_frame_time = current_time;
    
    // Cap delta time
    if (engine->delta_time > engine->config.physics_max_frame_time) {
        engine->delta_time = engine->config.physics_max_frame_time;
    }
    
    // Update physics if enabled
    if (engine->physics_world && engine->config.enable_physics) {
        physics_system_update(engine->physics_world, &engine->ecs_world, 
                             engine->config.physics_fixed_dt);
    }
    
    // Update ECS systems
    ecs_update_systems(&engine->ecs_world, engine->delta_time);
    
    // Call update callback
    if (engine->on_update) {
        engine->on_update(engine, engine->delta_time);
    }
}

void engine_core_render(EngineCore *engine) {
    if (!engine || !engine->initialized || !engine->running) {
        return;
    }
    
    if (!engine->renderer) {
        return;
    }
    
    u32 image_index;
    if (engine->renderer->begin_frame(engine->renderer, &image_index)) {
        // Update camera uniforms
        f32 aspect = (f32)engine->window_width / (f32)engine->window_height;
        engine->renderer->update_camera_uniforms(engine->renderer, 
                                                  engine->camera, aspect);
        
        // Call render callback
        if (engine->on_render) {
            engine->on_render(engine);
        }
        
        engine->renderer->end_frame(engine->renderer, image_index);
    }
}

// Rendering mode switching
bool engine_set_rendering_mode(EngineCore *engine, RenderingMode mode) {
    if (!engine || !engine->initialized) {
        return false;
    }
    
    if (mode == engine->current_rendering_mode) {
        return true;
    }
    
    // For now, just update the mode
    // TODO: Implement proper renderer recreation for mode switching
    // TODO: Add resource cleanup and reallocation for mode changes
    // TODO: Add rendering mode switching unit tests
    // In a full implementation, we'd recreate the renderer
    engine->current_rendering_mode = mode;
    engine->config.rendering_mode = mode;
    
    LOG_INFO("Rendering mode changed to %d", mode);
    return true;
}

RenderingMode engine_get_rendering_mode(EngineCore *engine) {
    if (!engine) {
        return RENDERING_MODE_3D_VOXEL;
    }
    return engine->current_rendering_mode;
}

// Window management
void engine_set_window_size(EngineCore *engine, u32 width, u32 height) {
    if (!engine) return;
    
    engine->window_width = width;
    engine->window_height = height;
    
    if (engine->renderer) {
        engine->renderer->resize(engine->renderer, width, height);
    }
}

void engine_get_window_size(EngineCore *engine, u32 *width, u32 *height) {
    if (!engine || !width || !height) return;
    
    *width = engine->window_width;
    *height = engine->window_height;
}

// Callback registration
void engine_set_init_callback(EngineCore *engine, void (*callback)(EngineCore *)) {
    if (engine) engine->on_init = callback;
}

void engine_set_update_callback(EngineCore *engine, void (*callback)(EngineCore *, f32)) {
    if (engine) engine->on_update = callback;
}

void engine_set_render_callback(EngineCore *engine, void (*callback)(EngineCore *)) {
    if (engine) engine->on_render = callback;
}

void engine_set_shutdown_callback(EngineCore *engine, void (*callback)(EngineCore *)) {
    if (engine) engine->on_shutdown = callback;
}

// High-level game creation API
bool engine_run_game(const GameDefinition *game) {
    if (!game) {
        LOG_ERROR("Invalid game definition");
        return false;
    }
    
    // TODO: Add game definition validation unit tests
    // TODO: Add engine lifecycle integration tests
    // TODO: Add callback registration and execution tests
    
    EngineCore engine;
    if (!engine_core_init(&engine, &game->config)) {
        LOG_ERROR("Failed to initialize engine");
        return false;
    }
    
    // Set callbacks
    engine.on_init = game->game_init;
    engine.on_update = game->game_update;
    engine.on_render = game->game_render;
    engine.on_shutdown = game->game_shutdown;
    
    // Call init callback
    if (engine.on_init) {
        engine.on_init(&engine);
    }
    
    // TODO: Add main loop error handling and recovery tests
    // TODO: Add frame rate limiting and performance tests
    // Main loop
    while (engine.running) {
        engine_core_update(&engine);
        engine_core_render(&engine);
    }
    
    // Cleanup
    engine_core_shutdown(&engine);
    
    return true;
}

