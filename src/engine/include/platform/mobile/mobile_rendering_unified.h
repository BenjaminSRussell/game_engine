// include/platform/mobile/mobile_rendering_unified.h
//
// Purpose: Unified mobile rendering system consolidating mobile-specific optimizations
// This replaces multiple disparate mobile rendering APIs with a single interface

#ifndef MOBILE_RENDERING_UNIFIED_H
#define MOBILE_RENDERING_UNIFIED_H

#include "common.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "rendering/camera.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// UNIFIED MOBILE RENDERING CONFIGURATION
// ============================================================================

typedef struct {
    // Performance settings
    bool enable_dynamic_resolution;
    bool enable_adaptive_quality;
    bool enable_occlusion_culling;
    bool enable_distance_fog;
    float target_frame_rate;
    float min_frame_rate;
    float max_frame_rate;
    
    // Quality settings
    bool enable_low_quality_mode;
    bool enable_texture_compression;
    bool enable_vertex_compression;
    float lod_bias;
    uint32_t max_lod_levels;
    
    // Memory settings
    uint32_t texture_memory_budget_mb;
    uint32_t vertex_buffer_budget_mb;
    bool enable_texture_streaming;
    float texture_streaming_distance;
    
    // Mobile-specific optimizations
    bool enable_mobile_shaders;
    bool enable_half_precision;
    bool enable_instanced_rendering;
    bool enable_batch_rendering;
    uint32_t batch_size;
} MobileRenderingConfig;

// ============================================================================
// MOBILE RENDERING TYPES
// ============================================================================

typedef enum {
    QUALITY_AUTO = 0,
    QUALITY_LOW,
    QUALITY_MEDIUM,
    QUALITY_HIGH,
    QUALITY_ULTRA,
    QUALITY_COUNT
} QualityLevel;

typedef enum {
    ADAPTIVE_QUALITY_DISABLED = 0,
    ADAPTIVE_QUALITY_CONSERVATIVE,
    ADAPTIVE_QUALITY_BALANCED,
    ADAPTIVE_QUALITY_AGGRESSIVE
} AdaptiveQualityMode;

typedef enum {
    LOD_LEVEL_AUTO = 0,
    LOD_LEVEL_LOW,
    LOD_LEVEL_MEDIUM,
    LOD_LEVEL_HIGH,
    LOD_LEVEL_ULTRA
} LODLevel;

typedef struct {
    uint32_t width;
    uint32_t height;
    float scale_factor;
    bool is_retina;
    bool is_tablet;
    float pixel_density;
} ScreenInfo;

typedef struct {
    QualityLevel current_quality;
    float current_frame_time;
    float average_frame_time;
    uint32_t frame_drop_count;
    bool quality_changed;
    float quality_change_timer;
} AdaptiveQualityManager;

typedef struct {
    uint32_t texture_id;
    uint32_t width;
    uint32_t height;
    uint32_t mip_levels;
    bool is_compressed;
    bool is_streaming;
    float streaming_priority;
    float distance_to_viewer;
    uint32_t last_access_frame;
    bool is_resident;
} MobileTextureInfo;

typedef struct {
    uint32_t vertex_count;
    uint32_t triangle_count;
    uint32_t draw_calls;
    uint32_t batch_count;
    float culling_ratio;
    float lod_ratio;
    uint32_t texture_swaps;
    float gpu_memory_usage_mb;
} MobileRenderingStats;

// ============================================================================
// UNIFIED MOBILE RENDERING SYSTEM
// ============================================================================

typedef struct MobileRenderingSystem {
    // Configuration and state
    MobileRenderingConfig config;
    bool is_initialized;
    
    // Screen information
    ScreenInfo screen_info;
    
    // Adaptive quality management
    AdaptiveQualityManager quality_manager;
    
    // LOD management
    LODLevel current_lod_level;
    float lod_distances[5];
    bool lod_enabled;
    
    // Texture management
    MobileTextureInfo* texture_cache;
    uint32_t max_textures;
    uint32_t active_textures;
    uint32_t texture_memory_usage;
    
    // Batch rendering
    void* batch_vertices;
    uint32_t max_batch_vertices;
    uint32_t current_batch_count;
    uint32_t batch_vertex_count;
    
    // Culling systems
    void* occlusion_culler;
    void* distance_culler;
    bool culling_enabled;
    
    // Performance monitoring
    MobileRenderingStats stats;
    float frame_time_history[60];
    uint32_t frame_history_index;
    
    // Platform-specific data
    void* platform_data;
} MobileRenderingSystem;

// ============================================================================
// MAIN MOBILE RENDERING API
// ============================================================================

// System management
MobileRenderingSystem* mobile_rendering_create(const MobileRenderingConfig* config);
void mobile_rendering_destroy(MobileRenderingSystem* renderer);
bool mobile_rendering_is_initialized(MobileRenderingSystem* renderer);
void mobile_rendering_update(MobileRenderingSystem* renderer, float delta_time);

// ============================================================================
// SCREEN AND DISPLAY API
// ============================================================================

void mobile_rendering_screen_init(MobileRenderingSystem* renderer);
const ScreenInfo* mobile_rendering_get_screen_info(MobileRenderingSystem* renderer);
void mobile_rendering_update_screen_info(MobileRenderingSystem* renderer);
bool mobile_rendering_is_retina_display(MobileRenderingSystem* renderer);
bool mobile_rendering_is_tablet_device(MobileRenderingSystem* renderer);
float mobile_rendering_get_pixel_density(MobileRenderingSystem* renderer);
Vec2 mobile_rendering_get_screen_size(MobileRenderingSystem* renderer);

// ============================================================================
// ADAPTIVE QUALITY API
// ============================================================================

void mobile_rendering_adaptive_quality_init(MobileRenderingSystem* renderer);
void mobile_rendering_set_quality_mode(MobileRenderingSystem* renderer, QualityLevel quality);
QualityLevel mobile_rendering_get_quality_level(MobileRenderingSystem* renderer);
void mobile_rendering_set_adaptive_mode(MobileRenderingSystem* renderer, AdaptiveQualityMode mode);
AdaptiveQualityMode mobile_rendering_get_adaptive_mode(MobileRenderingSystem* renderer);
void mobile_rendering_enable_dynamic_quality(MobileRenderingSystem* renderer, bool enable);
bool mobile_rendering_is_dynamic_quality_enabled(MobileRenderingSystem* renderer);

// ============================================================================
// LOD MANAGEMENT API
// ============================================================================

void mobile_rendering_lod_init(MobileRenderingSystem* renderer);
void mobile_rendering_set_lod_level(MobileRenderingSystem* renderer, LODLevel level);
LODLevel mobile_rendering_get_lod_level(MobileRenderingSystem* renderer);
void mobile_rendering_set_lod_distances(MobileRenderingSystem* renderer, const float* distances);
void mobile_rendering_enable_lod(MobileRenderingSystem* renderer, bool enable);
bool mobile_rendering_is_lod_enabled(MobileRenderingSystem* renderer);
float mobile_rendering_calculate_lod_distance(MobileRenderingSystem* renderer, float distance_to_viewer);

// ============================================================================
// TEXTURE MANAGEMENT API
// ============================================================================

void mobile_rendering_textures_init(MobileRenderingSystem* renderer);
uint32_t mobile_rendering_load_texture(MobileRenderingSystem* renderer, const char* filename);
void mobile_rendering_unload_texture(MobileRenderingSystem* renderer, uint32_t texture_id);
void mobile_rendering_stream_texture(MobileRenderingSystem* renderer, uint32_t texture_id, float priority);
void mobile_rendering_evict_texture(MobileRenderingSystem* renderer, uint32_t texture_id);
const MobileTextureInfo* mobile_rendering_get_texture_info(MobileRenderingSystem* renderer, uint32_t texture_id);
void mobile_rendering_update_texture_priority(MobileRenderingSystem* renderer, uint32_t texture_id, float priority);
uint32_t mobile_rendering_get_texture_memory_usage(MobileRenderingSystem* renderer);
void mobile_rendering_set_texture_budget(MobileRenderingSystem* renderer, uint32_t budget_mb);

// ============================================================================
// BATCH RENDERING API
// ============================================================================

void mobile_rendering_batch_init(MobileRenderingSystem* renderer);
void mobile_rendering_begin_batch(MobileRenderingSystem* renderer);
void mobile_rendering_end_batch(MobileRenderingSystem* renderer);
void mobile_rendering_add_to_batch(MobileRenderingSystem* renderer, const void* vertices, uint32_t vertex_count);
void mobile_rendering_flush_batch(MobileRenderingSystem* renderer);
void mobile_rendering_set_batch_size(MobileRenderingSystem* renderer, uint32_t size);
uint32_t mobile_rendering_get_batch_count(MobileRenderingSystem* renderer);
void mobile_rendering_enable_instancing(MobileRenderingSystem* renderer, bool enable);

// ============================================================================
// CULLING API
// ============================================================================

void mobile_rendering_culling_init(MobileRenderingSystem* renderer);
void mobile_rendering_enable_occlusion_culling(MobileRenderingSystem* renderer, bool enable);
void mobile_rendering_enable_distance_culling(MobileRenderingSystem* renderer, bool enable);
bool mobile_rendering_is_occlusion_culling_enabled(MobileRenderingSystem* renderer);
bool mobile_rendering_is_distance_culling_enabled(MobileRenderingSystem* renderer);
void mobile_rendering_update_culling_distances(MobileRenderingSystem* renderer, float near_distance, float far_distance);
bool mobile_rendering_is_visible(MobileRenderingSystem* renderer, const Vec3* position, float radius);

// ============================================================================
// PERFORMANCE MONITORING API
// ============================================================================

void mobile_rendering_performance_init(MobileRenderingSystem* renderer);
const MobileRenderingStats* mobile_rendering_get_stats(MobileRenderingSystem* renderer);
void mobile_rendering_reset_stats(MobileRenderingSystem* renderer);
float mobile_rendering_get_average_frame_time(MobileRenderingSystem* renderer);
float mobile_rendering_get_current_fps(MobileRenderingSystem* renderer);
uint32_t mobile_rendering_get_frame_drops(MobileRenderingSystem* renderer);
void mobile_rendering_enable_performance_monitoring(MobileRenderingSystem* renderer, bool enable);

// ============================================================================
// MOBILE SHADER OPTIMIZATIONS
// ============================================================================

void mobile_rendering_mobile_shaders_init(MobileRenderingSystem* renderer);
void mobile_rendering_enable_half_precision(MobileRenderingSystem* renderer, bool enable);
void mobile_rendering_enable_vertex_compression(MobileRenderingSystem* renderer, bool enable);
void mobile_rendering_enable_texture_compression(MobileRenderingSystem* renderer, bool enable);
bool mobile_rendering_is_half_precision_enabled(MobileRenderingSystem* renderer);
bool mobile_rendering_is_vertex_compression_enabled(MobileRenderingSystem* renderer);
bool mobile_rendering_is_texture_compression_enabled(MobileRenderingSystem* renderer);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Configuration
MobileRenderingConfig mobile_rendering_create_default_config(void);
bool mobile_rendering_validate_config(const MobileRenderingConfig* config);

// Platform detection
typedef enum {
    PLATFORM_MOBILE_IOS = 0,
    PLATFORM_MOBILE_ANDROID,
    PLATFORM_MOBILE_UNKNOWN
} MobilePlatform;

MobilePlatform mobile_rendering_get_platform(void);
bool mobile_rendering_supports_metal(void);
bool mobile_rendering_supports_vulkan(void);
bool mobile_rendering_supports_opengl_es3(void);

// Memory management
void mobile_rendering_optimize_memory_usage(MobileRenderingSystem* renderer);
void mobile_rendering_garbage_collect_textures(MobileRenderingSystem* renderer);
void mobile_rendering_compact_vertex_buffers(MobileRenderingSystem* renderer);

// Quality presets
void mobile_rendering_apply_quality_preset(MobileRenderingSystem* renderer, QualityLevel quality);
void mobile_rendering_apply_battery_saver_mode(MobileRenderingSystem* renderer);
void mobile_rendering_apply_performance_mode(MobileRenderingSystem* renderer);

#ifdef __cplusplus
}
#endif

#endif // MOBILE_RENDERING_UNIFIED_H
