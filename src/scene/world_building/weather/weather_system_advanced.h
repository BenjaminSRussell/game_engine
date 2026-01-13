/**
 * =================================================================================================
 *                              WEATHER SYSTEM - ADVANCED FEATURES
 *                                  Agent: AGENT_WORLD_1
 * =================================================================================================
 *
 * Enterprise-grade weather system with rendering integration, serialization,
 * and professional-grade atmospheric effects.
 *
 * =================================================================================================
 */

#ifndef WEATHER_SYSTEM_ADVANCED_H
#define WEATHER_SYSTEM_ADVANCED_H

#include "weather_system.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =================================================================================================
 *                                    RENDERING INTEGRATION
 * =================================================================================================
 */

/**
 * Initialize weather rendering system
 * Compiles specialized shaders and creates required textures
 * Returns: true on success, false on failure
 */
bool weather_rendering_init(void);

/**
 * Apply weather effects to rendering pipeline
 * This should be called during the main render loop
 * Parameters:
 *   manager - Weather manager with current state
 *   view_matrix - Current view matrix
 *   projection_matrix - Current projection matrix  
 *   camera_position - Current camera position in world space
 */
void weather_manager_apply_to_rendering(WeatherManager* manager, 
                                       const float* view_matrix,
                                       const float* projection_matrix,
                                       const float* camera_position);

/**
 * Shutdown weather rendering system
 * Frees all GPU resources and shaders
 */
void weather_rendering_shutdown(void);

/**
 * Set weather rendering quality level
 * Parameters:
 *   quality - Quality multiplier (0.1 = lowest, 1.0 = highest)
 */
void weather_rendering_set_quality(float quality);

/**
 * Enable/disable weather rendering
 */
void weather_rendering_set_enabled(bool enabled);

/**
 * Get weather rendering performance metrics
 */
void weather_rendering_get_metrics(float* frame_time_ms, uint32_t* triangles, uint32_t* draw_calls);

/**
 * Generate procedural noise textures for weather effects
 * Called automatically during initialization, but can be called manually for updates
 */
void weather_rendering_generate_noise_textures(void);

/* =================================================================================================
 *                                    SERIALIZATION
 * =================================================================================================
 */

/**
 * Serialize weather manager to JSON format
 * Parameters:
 *   manager - Weather manager to serialize
 *   buffer - Output buffer for JSON data
 *   buffer_size - Size of output buffer
 * Returns: true on success, false on failure
 */
bool weather_manager_serialize(const WeatherManager* manager, char* buffer, size_t buffer_size);

/**
 * Deserialize weather manager from JSON format
 * Parameters:
 *   manager - Weather manager to populate (must be initialized first)
 *   json_data - JSON data to deserialize
 * Returns: true on success, false on failure
 */
bool weather_manager_deserialize(WeatherManager* manager, const char* json_data);

/**
 * Serialize weather manager to binary format
 * More compact and faster than JSON, includes CRC32 validation
 * Parameters:
 *   manager - Weather manager to serialize
 *   buffer - Output buffer for binary data
 *   size - Input: buffer size, Output: actual data size
 * Returns: true on success, false on failure
 */
bool weather_manager_serialize_binary(const WeatherManager* manager, uint8_t* buffer, size_t* size);

/**
 * Deserialize weather manager from binary format
 * Parameters:
 *   manager - Weather manager to populate (must be initialized first)
 *   buffer - Binary data to deserialize
 *   size - Size of binary data
 * Returns: true on success, false on failure
 */
bool weather_manager_deserialize_binary(WeatherManager* manager, const uint8_t* buffer, size_t size);

/**
 * Save weather state to file
 * Automatically detects format from file extension (.json or .bin)
 */
bool weather_manager_save_to_file(const WeatherManager* manager, const char* file_path);

/**
 * Load weather state from file
 * Automatically detects format from file extension (.json or .bin)
 */
bool weather_manager_load_from_file(WeatherManager* manager, const char* file_path);

/* =================================================================================================
 *                                    ADVANCED WEATHER FEATURES
 * =================================================================================================
 */

/**
 * Weather zone management for localized weather effects
 */
typedef struct WeatherZoneManager {
    WeatherZone* zones;
    uint32_t zone_count;
    uint32_t zone_capacity;
    
    // Spatial indexing for efficient zone queries
    void* spatial_index; // Implementation-specific
    
    // Zone blending settings
    float global_blend_factor;
    bool enable_smooth_blending;
} WeatherZoneManager;

/**
 * Advanced weather simulation with meteorological accuracy
 */
typedef struct WeatherSimulation {
    // Atmospheric pressure systems
    float pressure_map[64][64];      // 2D pressure field
    float temperature_map[64][64];   // 2D temperature field
    float humidity_map[64][64];      // 2D humidity field
    
    // Wind simulation
    float wind_field_u[64][64];      // U-component of wind
    float wind_field_v[64][64];      // V-component of wind
    
    // Simulation parameters
    float time_step;
    float diffusion_rate;
    float coriolis_parameter;
    
    // Weather front tracking
    uint32_t front_count;
    struct WeatherFront {
        float position[2];
        float intensity;
        float direction;
        float speed;
    } fronts[16];
} WeatherSimulation;

/**
 * Initialize advanced weather zone management
 */
bool weather_zone_manager_init(WeatherZoneManager* manager, uint32_t initial_capacity);

/**
 * Shutdown weather zone management
 */
void weather_zone_manager_shutdown(WeatherZoneManager* manager);

/**
 * Add a weather zone
 */
bool weather_zone_manager_add_zone(WeatherZoneManager* manager, const WeatherZone* zone);

/**
 * Remove a weather zone by ID
 */
bool weather_zone_manager_remove_zone(WeatherZoneManager* manager, uint32_t zone_id);

/**
 * Get weather parameters at a specific position considering all zones
 */
bool weather_zone_manager_get_at_position(const WeatherZoneManager* manager,
                                         const float* position,
                                         WeatherParameters* out_params);

/**
 * Initialize advanced weather simulation
 */
bool weather_simulation_init(WeatherSimulation* simulation);

/**
 * Update weather simulation
 */
void weather_simulation_update(WeatherSimulation* simulation, float delta_time);

/**
 * Convert simulation state to weather parameters
 */
void weather_simulation_to_parameters(const WeatherSimulation* simulation,
                                     float world_x, float world_z,
                                     WeatherParameters* out_params);

/* =================================================================================================
 *                                    PERFORMANCE MONITORING
 * =================================================================================================
 */

/**
 * Weather system performance metrics
 */
typedef struct WeatherPerformanceMetrics {
    // Timing metrics
    float update_time_ms;
    float rendering_time_ms;
    float simulation_time_ms;
    
    // Memory usage
    size_t total_memory_used;
    size_t peak_memory_used;
    
    // Rendering statistics
    uint32_t triangles_rendered;
    uint32_t draw_calls_per_frame;
    uint32_t texture_uploads;
    
    // Simulation statistics
    uint32_t active_particles;
    uint32_t weather_zones_processed;
    uint32_t lightning_bolts_active;
    
    // Frame rate impact
    float frame_time_impact_ms;
    float gpu_time_ms;
    float cpu_time_ms;
} WeatherPerformanceMetrics;

/**
 * Get comprehensive weather system performance metrics
 */
void weather_get_performance_metrics(WeatherPerformanceMetrics* metrics);

/**
 * Reset performance metrics
 */
void weather_reset_performance_metrics(void);

/**
 * Enable/disable performance monitoring
 */
void weather_set_performance_monitoring(bool enabled);

/* =================================================================================================
 *                                    ERROR HANDLING AND LOGGING
 * =================================================================================================
 */

/**
 * Weather system error codes
 */
typedef enum WeatherError {
    WEATHER_ERROR_NONE = 0,
    WEATHER_ERROR_INVALID_PARAMETER,
    WEATHER_ERROR_OUT_OF_MEMORY,
    WEATHER_ERROR_RENDERING_FAILURE,
    WEATHER_ERROR_SERIALIZATION_FAILURE,
    WEATHER_ERROR_FILE_IO_FAILURE,
    WEATHER_ERROR_VERSION_MISMATCH,
    WEATHER_ERROR_CORRUPTED_DATA,
    WEATHER_ERROR_GPU_RESOURCE_FAILURE,
    WEATHER_ERROR_SHADER_COMPILATION_FAILURE
} WeatherError;

/**
 * Get last weather system error
 */
WeatherError weather_get_last_error(void);

/**
 * Get error description string
 */
const char* weather_get_error_string(WeatherError error);

/**
 * Set weather system logging level
 */
void weather_set_log_level(int level); // 0=off, 1=errors, 2=warnings, 3=info, 4=debug

/**
 * Weather system logging callback
 */
typedef void (*WeatherLogCallback)(int level, const char* message);
void weather_set_log_callback(WeatherLogCallback callback);

/* =================================================================================================
 *                                    CONFIGURATION MANAGEMENT
 * =================================================================================================
 */

/**
 * Weather system configuration
 */
typedef struct WeatherSystemConfig {
    // Rendering settings
    bool enable_atmospheric_scattering;
    bool enable_volumetric_clouds;
    bool enable_precipitation_particles;
    bool enable_lightning_effects;
    bool enable_screen_space_reflections;
    
    // Quality settings
    int cloud_ray_march_steps;
    int precipitation_max_particles;
    float atmospheric_scattering_quality;
    float volumetric_cloud_quality;
    
    // Performance settings
    bool enable_gpu_instancing;
    bool enable_frustum_culling;
    bool enable_occlusion_culling;
    bool enable_lod_system;
    
    // Memory settings
    size_t max_memory_usage;
    size_t texture_cache_size;
    uint32_t max_weather_zones;
    
    // Debug settings
    bool enable_wireframe_mode;
    bool enable_debug_visualization;
    bool enable_performance_profiling;
} WeatherSystemConfig;

/**
 * Get default weather system configuration
 */
void weather_config_get_defaults(WeatherSystemConfig* config);

/**
 * Apply weather system configuration
 */
bool weather_config_apply(const WeatherSystemConfig* config);

/**
 * Get current weather system configuration
 */
void weather_config_get_current(WeatherSystemConfig* config);

/**
 * Save weather configuration to file
 */
bool weather_config_save_to_file(const WeatherSystemConfig* config, const char* file_path);

/**
 * Load weather configuration from file
 */
bool weather_config_load_from_file(WeatherSystemConfig* config, const char* file_path);

/* =================================================================================================
 *                                    UTILITY FUNCTIONS
 * =================================================================================================
 */

/**
 * Get weather system version information
 */
void weather_get_version(int* major, int* minor, int* patch);

/**
 * Check if weather system is properly initialized
 */
bool weather_is_initialized(void);

/**
 * Get weather system memory usage
 */
size_t weather_get_memory_usage(void);

/**
 * Force garbage collection of weather resources
 */
void weather_garbage_collect(void);

/**
 * Get weather system capabilities
 */
typedef struct WeatherCapabilities {
    bool supports_volumetric_rendering;
    bool supports_gpu_compute;
    bool supports_ray_tracing;
    bool supports_variable_rate_shading;
    uint32_t max_texture_size;
    uint32_t max_compute_workgroup_size;
} WeatherCapabilities;

void weather_get_capabilities(WeatherCapabilities* caps);

#ifdef __cplusplus
}
#endif

#endif // WEATHER_SYSTEM_ADVANCED_H