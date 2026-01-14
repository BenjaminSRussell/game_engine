// Rendering Path Manager
// Manages enabling/disabling different rendering paths and features

#ifndef RENDERING_PATH_MANAGER_H
#define RENDERING_PATH_MANAGER_H

#include "core/types.h"
#include "rendering/core/renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

// Rendering path types
typedef enum {
    RENDERING_PATH_FORWARD = 0x0001,
    RENDERING_PATH_DEFERRED = 0x0002,
    RENDERING_PATH_RAYTRACED = 0x0004,
    RENDERING_PATH_VOXEL = 0x0008,
    RENDERING_PATH_SPRITE_3D = 0x0010,
    RENDERING_PATH_COMPUTE = 0x0020,
    RENDERING_PATH_ALL = 0xFFFF
} RenderingPathFlags;

// Rendering features
typedef enum {
    RENDERING_FEATURE_RAYTRACING = 0x0001,
    RENDERING_FEATURE_VRS = 0x0002,
    RENDERING_FEATURE_TAA = 0x0004,
    RENDERING_FEATURE_SSAO = 0x0008,
    RENDERING_FEATURE_SSR = 0x0010,
    RENDERING_FEATURE_BLOOM = 0x0020,
    RENDERING_FEATURE_FOG = 0x0040,
    RENDERING_FEATURE_SHADOWS = 0x0080,
    RENDERING_FEATURE_POST_PROCESSING = 0x0100,
    RENDERING_FEATURE_GPU_PARTICLES = 0x0200,
    RENDERING_FEATURE_SIMD_MATH = 0x0400,
    RENDERING_FEATURE_ALL = 0xFFFF
} RenderingFeatureFlags;

// Rendering path configuration
typedef struct {
    RenderingPathFlags enabled_paths;
    RenderingFeatureFlags enabled_features;
    RenderingPathFlags fallback_paths;
    RenderingFeatureFlags disabled_features;
} RenderingPathConfig;

// Rendering path manager
typedef struct RenderingPathManager {
    RenderingPathConfig config;
    
    // Hardware capabilities
    bool supports_raytracing;
    bool supports_vrs;
    bool supports_compute;
    bool supports_simd_avx2;
    bool supports_simd_avx512;
    
    // Current active path
    RenderingPathFlags current_path;
    
    // Performance metrics
    f32 forward_render_time;
    f32 deferred_render_time;
    f32 raytraced_render_time;
    f32 voxel_render_time;
    f32 sprite_3d_render_time;
    
    bool initialized;
} RenderingPathManager;

// Create rendering path manager
RenderingPathManager *rendering_path_manager_create(void);

// Destroy rendering path manager
void rendering_path_manager_destroy(RenderingPathManager *manager);

// Initialize hardware capabilities
void rendering_path_manager_detect_hardware(RenderingPathManager *manager);

// Enable/disable rendering paths
void rendering_path_manager_enable_path(RenderingPathManager *manager, RenderingPathFlags paths);
void rendering_path_manager_disable_path(RenderingPathManager *manager, RenderingPathFlags paths);
bool rendering_path_manager_is_path_enabled(RenderingPathManager *manager, RenderingPathFlags path);

// Enable/disable rendering features
void rendering_path_manager_enable_feature(RenderingPathManager *manager, RenderingFeatureFlags features);
void rendering_path_manager_disable_feature(RenderingPathManager *manager, RenderingFeatureFlags features);
bool rendering_path_manager_is_feature_enabled(RenderingPathManager *manager, RenderingFeatureFlags feature);

// Get optimal rendering path for current hardware
RenderingPathFlags rendering_path_manager_get_optimal_path(RenderingPathManager *manager);

// Set rendering path based on quality/performance settings
void rendering_path_manager_set_quality_level(RenderingPathManager *manager, u32 quality_level); // 0-5

// Auto-select best rendering path
void rendering_path_manager_auto_select(RenderingPathManager *manager);

// Update performance metrics
void rendering_path_manager_update_metrics(RenderingPathManager *manager, RenderingPathFlags path, f32 render_time);

// Get configuration
RenderingPathConfig rendering_path_manager_get_config(RenderingPathManager *manager);

// Apply configuration to renderer
void rendering_path_manager_apply_config(RenderingPathManager *manager, IRenderer *renderer);

// Check if feature is supported by hardware
bool rendering_path_manager_is_feature_supported(RenderingPathManager *manager, RenderingFeatureFlags feature);

// Get fallback path if primary path fails
RenderingPathFlags rendering_path_manager_get_fallback_path(RenderingPathManager *manager, RenderingPathFlags primary_path);

#ifdef __cplusplus
}
#endif

#endif // RENDERING_PATH_MANAGER_H
