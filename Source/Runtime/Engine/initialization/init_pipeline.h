#ifndef INIT_PIPELINE_H
#define INIT_PIPELINE_H

#include <stdbool.h>
#include <stdint.h>

// Initialization error codes
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

// Public initialization interface
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

// Cleanup and completion
void init_cleanup_on_error(InitError error);
void init_complete_all_systems(void);

#endif // INIT_PIPELINE_H
