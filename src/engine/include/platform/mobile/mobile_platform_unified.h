// include/platform/mobile/mobile_platform_unified.h
//
// Purpose: Unified mobile platform system consolidating iOS and Android support
// This replaces multiple disparate mobile platform APIs with a single interface

#ifndef MOBILE_PLATFORM_UNIFIED_H
#define MOBILE_PLATFORM_UNIFIED_H

#include "common.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// UNIFIED MOBILE PLATFORM CONFIGURATION
// ============================================================================

typedef struct {
    // Device capabilities
    bool enable_touch_input;
    bool enable_accelerometer;
    bool enable_gyroscope;
    bool enable_magnetometer;
    bool enable_gps;
    bool enable_camera;
    bool enable_microphone;
    bool enable_vibration;
    
    // Performance settings
    bool enable_background_processing;
    bool enable_multithreading;
    uint32_t max_worker_threads;
    float target_frame_rate;
    bool enable_adaptive_performance;
    
    // Memory settings
    uint32_t texture_memory_limit_mb;
    uint32_t audio_buffer_limit_mb;
    bool enable_memory_compression;
    
    // Platform-specific settings
    bool enable_ios_specific_features;
    bool enable_android_specific_features;
    bool enable_cross_platform_abstractions;
} MobilePlatformConfig;

// ============================================================================
// MOBILE PLATFORM TYPES
// ============================================================================

typedef enum {
    MOBILE_PLATFORM_IOS = 0,
    MOBILE_PLATFORM_ANDROID,
    MOBILE_PLATFORM_UNKNOWN
} MobilePlatformType;

typedef enum {
    DEVICE_TYPE_PHONE = 0,
    DEVICE_TYPE_TABLET,
    DEVICE_TYPE_TV,
    DEVICE_TYPE_UNKNOWN
} DeviceType;

typedef enum {
    BATTERY_LEVEL_UNKNOWN = -1,
    BATTERY_LEVEL_CRITICAL = 0,
    BATTERY_LEVEL_LOW = 25,
    BATTERY_LEVEL_MEDIUM = 50,
    BATTERY_LEVEL_HIGH = 75,
    BATTERY_LEVEL_FULL = 100
} BatteryLevel;

typedef enum {
    NETWORK_TYPE_NONE = 0,
    NETWORK_TYPE_WIFI,
    NETWORK_TYPE_CELLULAR,
    NETWORK_TYPE_ETHERNET,
    NETWORK_TYPE_BLUETOOTH
} NetworkType;

typedef enum {
    APP_STATE_FOREGROUND = 0,
    APP_STATE_BACKGROUND,
    APP_STATE_SUSPENDED,
    APP_STATE_TERMINATED
} ApplicationState;

typedef struct {
    char manufacturer[64];
    char model[64];
    char os_version[32];
    char os_name[32];
    uint32_t os_version_major;
    uint32_t os_version_minor;
    uint32_t os_version_patch;
    DeviceType device_type;
    bool is_tablet;
    bool is_retina;
    float screen_scale;
    Vec2 screen_resolution;
    float pixel_density;
    uint32_t total_memory_mb;
    uint32_t available_memory_mb;
    uint32_t cpu_cores;
    char gpu_name[64];
    bool supports_metal;
    bool supports_vulkan;
    bool supports_opengl_es3;
} DeviceInfo;

typedef struct {
    BatteryLevel level;
    bool is_charging;
    bool is_power_connected;
    float battery_percentage;
    int32_t battery_temperature; // Celsius
    uint32_t estimated_time_remaining_minutes;
} BatteryInfo;

typedef struct {
    NetworkType type;
    bool is_connected;
    char ssid[64];
    int32_t signal_strength; // dBm
    float connection_speed_mbps;
    uint32_t data_usage_mb;
    bool is_roaming;
} NetworkInfo;

typedef struct {
    ApplicationState state;
    bool is_active;
    bool is_visible;
    bool has_focus;
    double state_change_time;
    uint32_t background_time_seconds;
} ApplicationStatus;

typedef struct {
    Vec2 position;
    float accuracy_meters;
    float altitude_meters;
    float speed_mps;
    double timestamp;
    bool is_available;
} LocationInfo;

// ============================================================================
// UNIFIED MOBILE PLATFORM SYSTEM
// ============================================================================

typedef struct MobilePlatformSystem {
    // Configuration and state
    MobilePlatformConfig config;
    bool is_initialized;
    
    // Platform information
    MobilePlatformType platform_type;
    DeviceInfo device_info;
    BatteryInfo battery_info;
    NetworkInfo network_info;
    ApplicationStatus app_status;
    LocationInfo location_info;
    
    // Event callbacks
    void (*battery_callback)(const BatteryInfo* battery);
    void (*network_callback)(const NetworkInfo* network);
    void (*app_state_callback)(ApplicationState old_state, ApplicationState new_state);
    void (*location_callback)(const LocationInfo* location);
    void (*memory_warning_callback)(uint32_t available_memory_mb);
    void (*low_memory_callback)(void);
    
    // Performance monitoring
    float cpu_usage_percentage;
    uint32_t memory_usage_mb;
    float gpu_usage_percentage;
    float thermal_state; // 0.0 = normal, 1.0 = critical
    bool thermal_throttling_active;
    
    // Platform-specific data
    void* platform_data;
} MobilePlatformSystem;

// ============================================================================
// MAIN MOBILE PLATFORM API
// ============================================================================

// System management
MobilePlatformSystem* mobile_platform_create(const MobilePlatformConfig* config);
void mobile_platform_destroy(MobilePlatformSystem* platform);
bool mobile_platform_is_initialized(MobilePlatformSystem* platform);
void mobile_platform_update(MobilePlatformSystem* platform, float delta_time);

// ============================================================================
// DEVICE INFORMATION API
// ============================================================================

void mobile_platform_device_init(MobilePlatformSystem* platform);
const DeviceInfo* mobile_platform_get_device_info(MobilePlatformSystem* platform);
MobilePlatformType mobile_platform_get_platform_type(MobilePlatformSystem* platform);
DeviceType mobile_platform_get_device_type(MobilePlatformSystem* platform);
bool mobile_platform_is_tablet(MobilePlatformSystem* platform);
bool mobile_platform_is_retina_display(MobilePlatformSystem* platform);
float mobile_platform_get_screen_scale(MobilePlatformSystem* platform);
Vec2 mobile_platform_get_screen_resolution(MobilePlatformSystem* platform);
float mobile_platform_get_pixel_density(MobilePlatformSystem* platform);
uint32_t mobile_platform_get_total_memory(MobilePlatformSystem* platform);
uint32_t mobile_platform_get_available_memory(MobilePlatformSystem* platform);
uint32_t mobile_platform_get_cpu_cores(MobilePlatformSystem* platform);

// ============================================================================
// BATTERY MONITORING API
// ============================================================================

void mobile_platform_battery_init(MobilePlatformSystem* platform);
const BatteryInfo* mobile_platform_get_battery_info(MobilePlatformSystem* platform);
BatteryLevel mobile_platform_get_battery_level(MobilePlatformSystem* platform);
bool mobile_platform_is_charging(MobilePlatformSystem* platform);
bool mobile_platform_is_power_connected(MobilePlatformSystem* platform);
float mobile_platform_get_battery_percentage(MobilePlatformSystem* platform);
int32_t mobile_platform_get_battery_temperature(MobilePlatformSystem* platform);
uint32_t mobile_platform_get_estimated_battery_time(MobilePlatformSystem* platform);

// ============================================================================
// NETWORK MONITORING API
// ============================================================================

void mobile_platform_network_init(MobilePlatformSystem* platform);
const NetworkInfo* mobile_platform_get_network_info(MobilePlatformSystem* platform);
NetworkType mobile_platform_get_network_type(MobilePlatformSystem* platform);
bool mobile_platform_is_network_connected(MobilePlatformSystem* platform);
bool mobile_platform_is_wifi_connected(MobilePlatformSystem* platform);
bool mobile_platform_is_cellular_connected(MobilePlatformSystem* platform);
const char* mobile_platform_get_ssid(MobilePlatformSystem* platform);
int32_t mobile_platform_get_signal_strength(MobilePlatformSystem* platform);
float mobile_platform_get_connection_speed(MobilePlatformSystem* platform);
uint32_t mobile_platform_get_data_usage(MobilePlatformSystem* platform);

// ============================================================================
// APPLICATION STATE API
// ============================================================================

void mobile_platform_app_state_init(MobilePlatformSystem* platform);
const ApplicationStatus* mobile_platform_get_app_status(MobilePlatformSystem* platform);
ApplicationState mobile_platform_get_app_state(MobilePlatformSystem* platform);
bool mobile_platform_is_app_active(MobilePlatformSystem* platform);
bool mobile_platform_is_app_visible(MobilePlatformSystem* platform);
bool mobile_platform_has_app_focus(MobilePlatformSystem* platform);
uint32_t mobile_platform_get_background_time(MobilePlatformSystem* platform);

// ============================================================================
// LOCATION SERVICES API
// ============================================================================

void mobile_platform_location_init(MobilePlatformSystem* platform);
const LocationInfo* mobile_platform_get_location(MobilePlatformSystem* platform);
bool mobile_platform_is_location_available(MobilePlatformSystem* platform);
void mobile_platform_request_location_update(MobilePlatformSystem* platform);
void mobile_platform_enable_location_tracking(MobilePlatformSystem* platform, bool enable);

// ============================================================================
// PERFORMANCE MONITORING API
// ============================================================================

void mobile_platform_performance_init(MobilePlatformSystem* platform);
float mobile_platform_get_cpu_usage(MobilePlatformSystem* platform);
uint32_t mobile_platform_get_memory_usage(MobilePlatformSystem* platform);
float mobile_platform_get_gpu_usage(MobilePlatformSystem* platform);
float mobile_platform_get_thermal_state(MobilePlatformSystem* platform);
bool mobile_platform_is_thermal_throttling(MobilePlatformSystem* platform);
void mobile_platform_enable_performance_monitoring(MobilePlatformSystem* platform, bool enable);

// ============================================================================
// MEMORY MANAGEMENT API
// ============================================================================

void mobile_platform_memory_init(MobilePlatformSystem* platform);
void mobile_platform_register_memory_warning_callback(MobilePlatformSystem* platform, 
                                                    void (*callback)(uint32_t available_memory_mb));
void mobile_platform_register_low_memory_callback(MobilePlatformSystem* platform, 
                                                void (*callback)(void));
void mobile_platform_trigger_memory_cleanup(MobilePlatformSystem* platform);
void mobile_platform_set_memory_limit(MobilePlatformSystem* platform, uint32_t limit_mb);
uint32_t mobile_platform_get_memory_limit(MobilePlatformSystem* platform);

// ============================================================================
// BACKGROUND PROCESSING API
// ============================================================================

void mobile_platform_background_init(MobilePlatformSystem* platform);
void mobile_platform_enable_background_processing(MobilePlatformSystem* platform, bool enable);
bool mobile_platform_is_background_processing_enabled(MobilePlatformSystem* platform);
void mobile_platform_schedule_background_task(MobilePlatformSystem* platform, 
                                          void (*task)(void* data), void* data, 
                                          float delay_seconds);
void mobile_platform_cancel_background_task(MobilePlatformSystem* platform, uint32_t task_id);

// ============================================================================
// CALLBACK REGISTRATION API
// ============================================================================

void mobile_platform_set_battery_callback(MobilePlatformSystem* platform, 
                                      void (*callback)(const BatteryInfo* battery));
void mobile_platform_set_network_callback(MobilePlatformSystem* platform, 
                                      void (*callback)(const NetworkInfo* network));
void mobile_platform_set_app_state_callback(MobilePlatformSystem* platform, 
                                         void (*callback)(ApplicationState old_state, ApplicationState new_state));
void mobile_platform_set_location_callback(MobilePlatformSystem* platform, 
                                       void (*callback)(const LocationInfo* location));
void mobile_platform_set_memory_warning_callback(MobilePlatformSystem* platform, 
                                            void (*callback)(uint32_t available_memory_mb));
void mobile_platform_set_low_memory_callback(MobilePlatformSystem* platform, 
                                          void (*callback)(void));

// ============================================================================
// PLATFORM-SPECIFIC FEATURES
// ============================================================================

// iOS-specific features
void mobile_platform_enable_ios_features(MobilePlatformSystem* platform);
void mobile_platform_request_review_prompt(MobilePlatformSystem* platform);
void mobile_platform_show_app_store_rating(MobilePlatformSystem* platform);
void mobile_platform_enable_icloud_sync(MobilePlatformSystem* platform, bool enable);
void mobile_platform_enable_game_center(MobilePlatformSystem* platform, bool enable);
void mobile_platform_enable_in_app_purchases(MobilePlatformSystem* platform, bool enable);

// Android-specific features
void mobile_platform_enable_android_features(MobilePlatformSystem* platform);
void mobile_platform_request_permissions(MobilePlatformSystem* platform);
void mobile_platform_show_google_play_rating(MobilePlatformSystem* platform);
void mobile_platform_enable_google_play_services(MobilePlatformSystem* platform, bool enable);
void mobile_platform_enable_google_play_games(MobilePlatformSystem* platform, bool enable);
void mobile_platform_enable_billing(MobilePlatformSystem* platform, bool enable);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Configuration
MobilePlatformConfig mobile_platform_create_default_config(void);
bool mobile_platform_validate_config(const MobilePlatformConfig* config);

// Platform detection
MobilePlatformType mobile_platform_detect_platform(void);
bool mobile_platform_supports_feature(MobilePlatformSystem* platform, const char* feature_name);
const char* mobile_platform_get_platform_string(MobilePlatformSystem* platform);

// Device capabilities
bool mobile_platform_supports_vulkan(MobilePlatformSystem* platform);
bool mobile_platform_supports_metal(MobilePlatformSystem* platform);
bool mobile_platform_supports_opengl_es3(MobilePlatformSystem* platform);
bool mobile_platform_supports_multithreading(MobilePlatformSystem* platform);
uint32_t mobile_platform_get_max_threads(MobilePlatformSystem* platform);

// Performance optimization
void mobile_platform_optimize_for_device(MobilePlatformSystem* platform);
void mobile_platform_apply_performance_profile(MobilePlatformSystem* platform, const char* profile_name);
void mobile_platform_enable_adaptive_performance(MobilePlatformSystem* platform, bool enable);

// Error handling
typedef enum {
    MOBILE_PLATFORM_ERROR_NONE = 0,
    MOBILE_PLATFORM_ERROR_NOT_INITIALIZED,
    MOBILE_PLATFORM_ERROR_INVALID_CONFIG,
    MOBILE_PLATFORM_ERROR_PLATFORM_NOT_SUPPORTED,
    MOBILE_PLATFORM_ERROR_FEATURE_NOT_AVAILABLE,
    MOBILE_PLATFORM_ERROR_PERMISSION_DENIED,
    MOBILE_PLATFORM_ERROR_OUT_OF_MEMORY,
    MOBILE_PLATFORM_ERROR_SYSTEM_ERROR
} MobilePlatformError;

const char* mobile_platform_get_error_string(MobilePlatformError error);

#ifdef __cplusplus
}
#endif

#endif // MOBILE_PLATFORM_UNIFIED_H
