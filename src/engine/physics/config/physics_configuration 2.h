/**
 * @file physics_configuration.h
 * @brief Physics configuration system
 *
 * Provides comprehensive configuration management for physics systems
 * including runtime settings, presets, validation, and persistence.
 */

#ifndef PHYSICS_CONFIGURATION_H
#define PHYSICS_CONFIGURATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Configuration Categories
// ========================================

typedef enum ConfigCategory {
    CONFIG_CATEGORY_SIMULATION,      // Simulation settings
    CONFIG_CATEGORY_COLLISION,        // Collision detection settings
    CONFIG_CATEGORY_SOLVER,           // Constraint solver settings
    CONFIG_CATEGORY_OPTIMIZATION,     // Performance optimization settings
    CONFIG_CATEGORY_DEBUGGING,        // Debug and profiling settings
    CONFIG_CATEGORY_RENDERING,        // Visualization settings
    CONFIG_CATEGORY_NETWORKING,       // Network synchronization settings
    CONFIG_CATEGORY_SERIALIZATION,    // Serialization settings
    CONFIG_CATEGORY_ALL               // All categories
} ConfigCategory;

// ========================================
// Configuration Value Types
// ========================================

typedef enum ConfigValueType {
    CONFIG_TYPE_BOOL,                 // Boolean value
    CONFIG_TYPE_INT,                  // Integer value
    CONFIG_TYPE_FLOAT,                // Float value
    CONFIG_TYPE_STRING,               // String value
    CONFIG_TYPE_VEC2,                 // 2D vector
    CONFIG_TYPE_VEC3,                 // 3D vector
    CONFIG_TYPE_VEC4,                 // 4D vector
    CONFIG_TYPE_COLOR,                // RGBA color
    CONFIG_TYPE_ENUM                  // Enum value
} ConfigValueType;

// ========================================
// Configuration Entry
// ========================================

typedef struct ConfigEntry {
    char key[64];                    // Configuration key
    char name[128];                  // Human-readable name
    char description[256];            // Description
    ConfigCategory category;          // Configuration category
    ConfigValueType type;             // Value type
    bool read_only;                  // Read-only flag
    bool requires_restart;            // Requires restart to apply
    float min_value;                 // Minimum value (for numeric types)
    float max_value;                 // Maximum value (for numeric types)
    float default_value;             // Default value (for numeric types)
    union {
        bool bool_value;
        int int_value;
        float float_value;
        char string_value[256];
        float vec2_value[2];
        float vec3_value[3];
        float vec4_value[4];
        float color_value[4];
        int enum_value;
    } value;
    union {
        bool bool_default;
        int int_default;
        float float_default;
        char string_default[256];
        float vec2_default[2];
        float vec3_default[3];
        float vec4_default[4];
        float color_default[4];
        int enum_default;
    } default_value;
    char enum_options[128];          // Enum options (comma-separated)
} ConfigEntry;

// ========================================
// Configuration Preset
// ========================================

typedef struct ConfigPreset {
    char name[64];                   // Preset name
    char description[256];            // Preset description
    char version[16];                // Version string
    ConfigEntry *entries;            // Configuration entries
    int entry_count;                 // Number of entries
    bool is_default;                 // Is default preset
    bool is_readonly;                // Is read-only preset
} ConfigPreset;

// ========================================
// Configuration Manager
// ========================================

typedef struct ConfigManager {
    ConfigEntry *entries;             // Configuration entries
    int entry_count;                 // Number of entries
    int entry_capacity;              // Entry array capacity
    
    ConfigPreset *presets;            // Configuration presets
    int preset_count;                 // Number of presets
    int preset_capacity;              // Preset array capacity
    
    ConfigPreset *current_preset;     // Current active preset
    char current_preset_name[64];     // Current preset name
    
    bool auto_save;                   // Auto-save changes
    bool validate_values;             // Validate values on set
    bool log_changes;                 // Log configuration changes
    char config_file_path[256];       // Configuration file path
    
    // Callbacks
    void (*value_changed_callback)(const char *key, const ConfigEntry *entry, void *user_data);
    void (*preset_loaded_callback)(const char *preset_name, void *user_data);
    void (*validation_failed_callback)(const char *key, const char *error, void *user_data);
    void *callback_user_data;
    
} ConfigManager;

// ========================================
// Simulation Configuration
// ========================================

typedef struct SimulationConfig {
    float gravity[3];                // Gravity vector
    float time_step;                 // Fixed time step
    float max_time_step;             // Maximum time step
    int velocity_iterations;          // Velocity solver iterations
    int position_iterations;          // Position solver iterations
    bool enable_sleeping;            // Enable sleeping objects
    float sleep_threshold;           // Sleep threshold
    float wake_up_threshold;          // Wake up threshold
    bool enable_warm_starting;        // Enable warm starting
    bool enable_adaptive_time_step;    // Enable adaptive time stepping
    float adaptive_time_scale;        // Adaptive time scale
    int max_substeps;                // Maximum substeps
    bool enable_continuous_collision; // Enable continuous collision detection
    float ccd_safety_margin;          // CCD safety margin
    int ccd_max_iterations;          // CCD maximum iterations
} SimulationConfig;

// ========================================
// Collision Configuration
// ========================================

typedef struct CollisionConfig {
    bool enable_collision_detection;  // Enable collision detection
    bool enable_collision_response;    // Enable collision response
    bool enable_broadphase;           // Enable broadphase
    bool enable_narrowphase;          // Enable narrowphase
    float collision_margin;           // Collision margin
    float contact_offset;             // Contact offset
    float allowed_penetration;        // Allowed penetration
    float bias_factor;                // Position correction bias
    int max_contacts_per_object;      // Maximum contacts per object
    int max_contact_pairs;            // Maximum contact pairs
    bool enable_contact_caching;       // Enable contact caching
    float contact_cache_ttl;          // Contact cache time-to-live
    bool enable_ray_casting;          // Enable ray casting
    bool enable_shape_casting;         // Enable shape casting
    float max_cast_distance;           // Maximum cast distance
    int max_cast_results;             // Maximum cast results
} CollisionConfig;

// ========================================
// Solver Configuration
// ========================================

typedef struct SolverConfig {
    int solver_type;                 // Solver type (0=Sequential Impulse, 1=XPBD)
    int max_solver_iterations;        // Maximum solver iterations
    float solver_epsilon;             // Solver convergence epsilon
    bool enable_constraint_warm_start; // Enable constraint warm starting
    float warm_start_factor;          // Warm start factor
    bool enable_position_correction;   // Enable position correction
    float position_correction_factor;  // Position correction factor
    bool enable_velocity_correction;   // Enable velocity correction
    float velocity_correction_factor;  // Velocity correction factor
    float min_constraint_distance;    // Minimum constraint distance
    float max_constraint_distance;    // Maximum constraint distance
    bool enable_constraint_breaking;   // Enable constraint breaking
    float break_force_threshold;       // Break force threshold
    bool enable_joint_limits;         // Enable joint limits
    float joint_limit_stiffness;      // Joint limit stiffness
    float joint_limit_damping;        // Joint limit damping
} SolverConfig;

// ========================================
// Optimization Configuration
// ========================================

typedef struct OptimizationConfig {
    bool enable_multi_threading;     // Enable multi-threading
    int worker_thread_count;          // Number of worker threads
    int batch_size;                   // Batch size for processing
    bool enable_simd;                 // Enable SIMD optimizations
    bool enable_memory_pooling;       // Enable memory pooling
    size_t memory_pool_size;          // Memory pool size
    bool enable_spatial_partitioning; // Enable spatial partitioning
    float spatial_grid_size;          // Spatial grid cell size
    bool enable_level_of_detail;      // Enable level of detail
    float lod_distance_threshold;     // LOD distance threshold
    bool enable_caching;              // Enable result caching
    int cache_size;                   // Cache size
    float cache_ttl;                  // Cache time-to-live
    bool enable_adaptive_optimization; // Enable adaptive optimization
    float performance_threshold;      // Performance threshold
    int adaptation_interval;          // Adaptation interval
} OptimizationConfig;

// ========================================
// Debug Configuration
// ========================================

typedef struct DebugConfig {
    bool enable_debug_draw;           // Enable debug drawing
    bool enable_profiling;            // Enable profiling
    bool enable_validation;           // Enable validation
    bool enable_logging;              // Enable logging
    int log_level;                    // Log level (0=Error, 1=Warning, 2=Info, 3=Debug)
    bool enable_memory_tracking;       // Enable memory tracking
    bool enable_performance_monitoring; // Enable performance monitoring
    bool enable_statistics;           // Enable statistics collection
    float statistics_update_interval;  // Statistics update interval
    bool enable_debug_visualization;   // Enable debug visualization
    int visualization_flags;          // Visualization flags
    bool enable_collision_debug;      // Enable collision debugging
    bool enable_solver_debug;         // Enable solver debugging
    bool enable_integration_debug;     // Enable integration debugging
} DebugConfig;

// ========================================
// Rendering Configuration
// ========================================

typedef struct RenderingConfig {
    bool enable_physics_rendering;    // Enable physics rendering
    bool render_collision_shapes;      // Render collision shapes
    bool render_bounding_volumes;      // Render bounding volumes
    bool render_velocity_vectors;      // Render velocity vectors
    bool render_force_vectors;         // Render force vectors
    bool render_contact_points;        // Render contact points
    bool render_constraints;           // Render constraints
    bool render_sleeping_objects;      // Render sleeping objects
    float line_width;                 // Line width for wireframe
    float point_size;                 // Point size
    float alpha;                       // Global alpha
    bool enable_transparency;          // Enable transparency
    bool enable_depth_test;            // Enable depth testing
    bool enable_culling;               // Enable face culling
    int max_render_distance;           // Maximum render distance
    bool enable_frustum_culling;       // Enable frustum culling
    bool enable_distance_culling;      // Enable distance culling
} RenderingConfig;

// ========================================
// Network Configuration
// ========================================

typedef struct NetworkConfig {
    bool enable_networking;           // Enable networking
    bool enable_authoritative_server; // Enable authoritative server
    bool enable_client_prediction;     // Enable client prediction
    bool enable_lag_compensation;      // Enable lag compensation
    float network_tick_rate;          // Network tick rate
    float interpolation_delay;        // Interpolation delay
    float extrapolation_limit;         // Extrapolation limit
    bool enable_delta_compression;     // Enable delta compression
    int max_network_objects;          // Maximum network objects
    float position_tolerance;         // Position tolerance
    float rotation_tolerance;         // Rotation tolerance
    bool enable_reconciliation;       // Enable reconciliation
    float reconciliation_threshold;    // Reconciliation threshold
} NetworkConfig;

// ========================================
// Serialization Configuration
// ========================================

typedef struct SerializationConfig {
    bool enable_serialization;        // Enable serialization
    int serialization_format;         // Serialization format
    bool enable_compression;          // Enable compression
    int compression_level;            // Compression level
    bool enable_encryption;            // Enable encryption
    char encryption_key[64];          // Encryption key
    bool enable_checksum;              // Enable checksum
    bool enable_versioning;            // Enable versioning
    bool enable_streaming;             // Enable streaming
    int stream_buffer_size;            // Stream buffer size
    bool enable_delta_serialization;   // Enable delta serialization
    float delta_threshold;            // Delta threshold
    int max_serialization_size;        // Maximum serialization size
} SerializationConfig;

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create configuration manager
 * @return Configuration manager or NULL on failure
 */
ConfigManager* config_manager_create(void);

/**
 * Destroy configuration manager
 * @param manager Configuration manager
 */
void config_manager_destroy(ConfigManager *manager);

// ========================================
// Entry Management
// ========================================

/**
 * Register configuration entry
 * @param manager Configuration manager
 * @param entry Configuration entry
 * @return True if successful
 */
bool config_manager_register_entry(ConfigManager *manager, const ConfigEntry *entry);

/**
 * Unregister configuration entry
 * @param manager Configuration manager
 * @param key Entry key
 * @return True if successful
 */
bool config_manager_unregister_entry(ConfigManager *manager, const char *key);

/**
 * Get configuration entry
 * @param manager Configuration manager
 * @param key Entry key
 * @return Entry pointer or NULL if not found
 */
ConfigEntry* config_manager_get_entry(ConfigManager *manager, const char *key);

/**
 * Find entries by category
 * @param manager Configuration manager
 * @param category Configuration category
 * @param entries Output entries array
 * @param max_entries Maximum entries to return
 * @return Number of entries found
 */
int config_manager_find_entries_by_category(ConfigManager *manager, ConfigCategory category,
                                           ConfigEntry **entries, int max_entries);

/**
 * Get all configuration entries
 * @param manager Configuration manager
 * @param entries Output entries array
 * @param max_entries Maximum entries to return
 * @return Number of entries
 */
int config_manager_get_all_entries(ConfigManager *manager, ConfigEntry **entries, int max_entries);

// ========================================
// Value Access
// ========================================

/**
 * Set boolean value
 * @param manager Configuration manager
 * @param key Entry key
 * @param value Boolean value
 * @return True if successful
 */
bool config_manager_set_bool(ConfigManager *manager, const char *key, bool value);

/**
 * Get boolean value
 * @param manager Configuration manager
 * @param key Entry key
 * @param default_value Default value if not found
 * @return Boolean value
 */
bool config_manager_get_bool(ConfigManager *manager, const char *key, bool default_value);

/**
 * Set integer value
 * @param manager Configuration manager
 * @param key Entry key
 * @param value Integer value
 * @return True if successful
 */
bool config_manager_set_int(ConfigManager *manager, const char *key, int value);

/**
 * Get integer value
 * @param manager Configuration manager
 * @param key Entry key
 * @param default_value Default value if not found
 * @return Integer value
 */
int config_manager_get_int(ConfigManager *manager, const char *key, int default_value);

/**
 * Set float value
 * @param manager Configuration manager
 * @param key Entry key
 * @param value Float value
 * @return True if successful
 */
bool config_manager_set_float(ConfigManager *manager, const char *key, float value);

/**
 * Get float value
 * @param manager Configuration manager
 * @param key Entry key
 * @param default_value Default value if not found
 * @return Float value
 */
float config_manager_get_float(ConfigManager *manager, const char *key, float default_value);

/**
 * Set string value
 * @param manager Configuration manager
 * @param key Entry key
 * @param value String value
 * @return True if successful
 */
bool config_manager_set_string(ConfigManager *manager, const char *key, const char *value);

/**
 * Get string value
 * @param manager Configuration manager
 * @param key Entry key
 * @param default_value Default value if not found
 * @return String value
 */
const char* config_manager_get_string(ConfigManager *manager, const char *key, const char *default_value);

/**
 * Set vector value
 * @param manager Configuration manager
 * @param key Entry key
 * @param value Vector value
 * @param components Number of components (2, 3, or 4)
 * @return True if successful
 */
bool config_manager_set_vector(ConfigManager *manager, const char *key, const float *value, int components);

/**
 * Get vector value
 * @param manager Configuration manager
 * @param key Entry key
 * @param value Output vector
 * @param components Number of components (2, 3, or 4)
 * @param default_value Default value if not found
 * @return True if successful
 */
bool config_manager_get_vector(ConfigManager *manager, const char *key, float *value, int components, const float *default_value);

// ========================================
// Preset Management
// ========================================

/**
 * Create configuration preset
 * @param manager Configuration manager
 * @param name Preset name
 * @param description Preset description
 * @return Preset pointer or NULL on failure
 */
ConfigPreset* config_manager_create_preset(ConfigManager *manager, const char *name, const char *description);

/**
 * Delete configuration preset
 * @param manager Configuration manager
 * @param name Preset name
 * @return True if successful
 */
bool config_manager_delete_preset(ConfigManager *manager, const char *name);

/**
 * Get configuration preset
 * @param manager Configuration manager
 * @param name Preset name
 * @return Preset pointer or NULL if not found
 */
ConfigPreset* config_manager_get_preset(ConfigManager *manager, const char *name);

/**
 * Load configuration preset
 * @param manager Configuration manager
 * @param name Preset name
 * @return True if successful
 */
bool config_manager_load_preset(ConfigManager *manager, const char *name);

/**
 * Save current configuration as preset
 * @param manager Configuration manager
 * @param name Preset name
 * @param description Preset description
 * @return True if successful
 */
bool config_manager_save_preset(ConfigManager *manager, const char *name, const char *description);

/**
 * Get all preset names
 * @param manager Configuration manager
 * @param names Output names array
 * @param max_names Maximum names to return
 * @return Number of presets
 */
int config_manager_get_preset_names(ConfigManager *manager, char **names, int max_names);

/**
 * Get current preset name
 * @param manager Configuration manager
 * @return Current preset name
 */
const char* config_manager_get_current_preset(ConfigManager *manager);

// ========================================
// Built-in Presets
// ========================================

/**
 * Load default preset
 * @param manager Configuration manager
 * @return True if successful
 */
bool config_manager_load_default_preset(ConfigManager *manager);

/**
 * Load high performance preset
 * @param manager Configuration manager
 * @return True if successful
 */
bool config_manager_load_high_performance_preset(ConfigManager *manager);

/**
 * Load high quality preset
 * @param manager Configuration manager
 * @return True if successful
 */
bool config_manager_load_high_quality_preset(ConfigManager *manager);

/**
 * Load mobile preset
 * @param manager Configuration manager
 * @return True if successful
 */
bool config_manager_load_mobile_preset(ConfigManager *manager);

/**
 * Load debug preset
 * @param manager Configuration manager
 * @return True if successful
 */
bool config_manager_load_debug_preset(ConfigManager *manager);

// ========================================
// File Operations
// ========================================

/**
 * Load configuration from file
 * @param manager Configuration manager
 * @param filename Configuration file path
 * @return True if successful
 */
bool config_manager_load_from_file(ConfigManager *manager, const char *filename);

/**
 * Save configuration to file
 * @param manager Configuration manager
 * @param filename Configuration file path
 * @return True if successful
 */
bool config_manager_save_to_file(ConfigManager *manager, const char *filename);

/**
 * Load preset from file
 * @param manager Configuration manager
 * @param filename Preset file path
 * @param preset_name Preset name
 * @return True if successful
 */
bool config_manager_load_preset_from_file(ConfigManager *manager, const char *filename, const char *preset_name);

/**
 * Save preset to file
 * @param manager Configuration manager
 * @param filename Preset file path
 * @param preset_name Preset name
 * @return True if successful
 */
bool config_manager_save_preset_to_file(ConfigManager *manager, const char *filename, const char *preset_name);

// ========================================
// Validation
// ========================================

/**
 * Validate configuration entry
 * @param entry Configuration entry
 * @param error Output error message
 * @param error_size Error message buffer size
 * @return True if valid
 */
bool config_validate_entry(const ConfigEntry *entry, char *error, size_t error_size);

/**
 * Validate all configuration entries
 * @param manager Configuration manager
 * @param errors Output error messages
 * @param max_errors Maximum errors to return
 * @return Number of validation errors
 */
int config_manager_validate_all(ConfigManager *manager, char **errors, int max_errors);

/**
 * Enable/disable validation
 * @param manager Configuration manager
 * @param enabled Enable validation
 */
void config_manager_set_validation_enabled(ConfigManager *manager, bool enabled);

// ========================================
// Callbacks
// ========================================

/**
 * Set value changed callback
 * @param manager Configuration manager
 * @param callback Callback function
 * @param user_data User data pointer
 */
void config_manager_set_value_changed_callback(ConfigManager *manager,
                                               void (*callback)(const char*, const ConfigEntry*, void*),
                                               void *user_data);

/**
 * Set preset loaded callback
 * @param manager Configuration manager
 * @param callback Callback function
 * @param user_data User data pointer
 */
void config_manager_set_preset_loaded_callback(ConfigManager *manager,
                                               void (*callback)(const char*, void*),
                                               void *user_data);

/**
 * Set validation failed callback
 * @param manager Configuration manager
 * @param callback Callback function
 * @param user_data User data pointer
 */
void config_manager_set_validation_failed_callback(ConfigManager *manager,
                                                  void (*callback)(const char*, const char*, void*),
                                                  void *user_data);

// ========================================
// Utility Functions
// ========================================

/**
 * Reset all values to defaults
 * @param manager Configuration manager
 */
void config_manager_reset_to_defaults(ConfigManager *manager);

/**
 * Reset category values to defaults
 * @param manager Configuration manager
 * @param category Configuration category
 */
void config_manager_reset_category_to_defaults(ConfigManager *manager, ConfigCategory category);

/**
 * Get configuration statistics
 * @param manager Configuration manager
 * @param total_entries Total number of entries
 * @param modified_entries Number of modified entries
 * @param preset_count Number of presets
 */
void config_manager_get_stats(ConfigManager *manager, int *total_entries, int *modified_entries, int *preset_count);

/**
 * Enable/disable auto-save
 * @param manager Configuration manager
 * @param enabled Enable auto-save
 */
void config_manager_set_auto_save(ConfigManager *manager, bool enabled);

/**
 * Set configuration file path
 * @param manager Configuration manager
 * @param path File path
 */
void config_manager_set_config_file_path(ConfigManager *manager, const char *path);

/**
 * Get configuration file path
 * @param manager Configuration manager
 * @return File path
 */
const char* config_manager_get_config_file_path(ConfigManager *manager);

/**
 * Validate configuration manager
 * @param manager Configuration manager
 * @return True if valid
 */
bool config_manager_validate(ConfigManager *manager);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_CONFIGURATION_H */
