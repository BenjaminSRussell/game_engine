/**
 * @file physics_asset_loading.h
 * @brief Physics asset loading system
 *
 * Provides comprehensive asset loading for physics systems including
 * collision shapes, materials, constraints, and physics scenes from
 * various file formats with caching and async loading support.
 */

#ifndef PHYSICS_ASSET_LOADING_H
#define PHYSICS_ASSET_LOADING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Asset Types
// ========================================

typedef enum PhysicsAssetType {
    PHYSICS_ASSET_COLLISION_SHAPE,      // Collision shape asset
    PHYSICS_ASSET_MATERIAL,            // Material asset
    PHYSICS_ASSET_CONSTRAINT,          // Constraint asset
    PHYSICS_ASSET_RIGID_BODY,          // Rigid body asset
    PHYSICS_ASSET_SOFT_BODY,           // Soft body asset
    PHYSICS_ASSET_CLOTH,               // Cloth asset
    PHYSICS_ASSET_FLUID,               // Fluid asset
    PHYSICS_ASSET_TERRAIN,             // Terrain asset
    PHYSICS_ASSET_SCENE,               // Physics scene asset
    PHYSICS_ASSET_DESTRUCTION,         // Destruction asset
    PHYSICS_ASSET_PARTICLE_SYSTEM,     // Particle system asset
    PHYSICS_ASSET_FORCE_FIELD,         // Force field asset
    PHYSICS_ASSET_CUSTOM = 1000        // Custom asset type
} PhysicsAssetType;

// ========================================
// File Formats
// ========================================

typedef enum AssetFileFormat {
    ASSET_FORMAT_AUTO,                 // Auto-detect format
    ASSET_FORMAT_OBJ,                  // Wavefront OBJ
    ASSET_FORMAT_FBX,                  // Autodesk FBX
    ASSET_FORMAT_COLLADA,              // COLLADA (.dae)
    ASSET_FORMAT_GLTF,                 // glTF 2.0
    ASSET_FORMAT_PLY,                  // Stanford PLY
    ASSET_FORMAT_STL,                  // STL (3D printing)
    ASSET_FORMAT_PHYSX,                // PhysX XML
    ASSET_FORMAT_BULLET,               // Bullet Physics
    ASSET_FORMAT_HAVOK,                // Havok Physics
    ASSET_FORMAT_CUSTOM,               // Custom format
    ASSET_FORMAT_JSON,                 // JSON format
    ASSET_FORMAT_BINARY,               // Binary format
    ASSET_FORMAT_XML                   // XML format
} AssetFileFormat;

// ========================================
// Loading Flags
// ========================================

typedef enum AssetLoadingFlags {
    ASSET_FLAG_NONE = 0x00,
    ASSET_FLAG_ASYNC = 0x01,          // Asynchronous loading
    ASSET_FLAG_CACHE = 0x02,           // Use cache
    ASSET_FLAG_COMPRESSED = 0x04,     // Compressed asset
    ASSET_FLAG_STREAMING = 0x08,      // Streaming asset
    ASSET_FLAG_PRELOAD = 0x10,        // Preload asset
    ASSET_FLAG_LAZY = 0x20,           // Lazy loading
    ASSET_FLAG_SHARED = 0x40,         // Shared asset
    ASSET_FLAG_VOLATILE = 0x80        // Volatile asset
} AssetLoadingFlags;

// ========================================
// Asset Status
// ========================================

typedef enum AssetStatus {
    ASSET_STATUS_UNLOADED,            // Asset not loaded
    ASSET_STATUS_LOADING,             // Asset loading
    ASSET_STATUS_LOADED,              // Asset loaded
    ASSET_STATUS_ERROR,               // Loading error
    ASSET_STATUS_CACHED,              // Asset cached
    ASSET_STATUS_UNLOADING,           // Asset unloading
    ASSET_STATUS_DESTROYED            // Asset destroyed
} AssetStatus;

// ========================================
// Asset Info
// ========================================

typedef struct AssetInfo {
    char name[256];                   // Asset name
    char file_path[512];              // File path
    char description[512];            // Description
    PhysicsAssetType type;            // Asset type
    AssetFileFormat format;           // File format
    uint32_t flags;                   // Loading flags
    AssetStatus status;               // Asset status
    uint64_t file_size;               // File size
    uint64_t memory_size;              // Memory size
    uint64_t load_time;               // Load time in microseconds
    uint64_t last_access;             // Last access time
    uint32_t reference_count;         // Reference count
    bool persistent;                  // Persistent asset
    bool compressed;                  // Compressed asset
    bool cached;                      // Cached asset
    void *user_data;                 // User data pointer
} AssetInfo;

// ========================================
// Collision Shape Asset
// ========================================

typedef struct CollisionShapeAsset {
    uint32_t shape_type;              // Shape type
    float scale[3];                   // Shape scale
    float margin;                     // Collision margin
    bool is_convex;                   // Is convex shape
    int vertex_count;                 // Vertex count
    int triangle_count;               // Triangle count
    float *vertices;                  // Vertex data
    uint32_t *indices;                // Index data
    float *normals;                  // Normal data
    float *uvs;                      // UV data
    float bounds[6];                  // Bounding box
    float volume;                     // Shape volume
    float surface_area;               // Surface area
    float mass;                       // Shape mass
    float center_of_mass[3];          // Center of mass
    float inertia[9];                 // Inertia tensor
} CollisionShapeAsset;

// ========================================
// Material Asset
// ========================================

typedef struct MaterialAsset {
    char name[128];                  // Material name
    float density;                   // Density
    float friction;                  // Static friction
    float dynamic_friction;          // Dynamic friction
    float restitution;               // Restitution (bounciness)
    float rolling_friction;          // Rolling friction
    float spinning_friction;         // Spinning friction
    float thickness;                 // Material thickness
    float damping;                   // Linear damping
    float angular_damping;           // Angular damping
    float stiffness;                 // Material stiffness
    float toughness;                 // Material toughness
    float hardness;                  // Material hardness
    bool is_trigger;                 // Is trigger material
    bool is_sensor;                  // Is sensor material
    bool is_destructible;             // Is destructible
    float fracture_threshold;         // Fracture threshold
    char texture_path[256];          // Texture path
    char normal_map_path[256];        // Normal map path
    char material_properties[1024];   // Additional properties
} MaterialAsset;

// ========================================
// Rigid Body Asset
// ========================================

typedef struct RigidBodyAsset {
    char name[128];                  // Body name
    uint32_t shape_asset_id;         // Shape asset ID
    uint32_t material_asset_id;      // Material asset ID
    float mass;                       // Body mass
    float inertia[9];                 // Inertia tensor
    float center_of_mass[3];          // Center of mass
    float position[3];                // Initial position
    float rotation[4];                // Initial rotation (quaternion)
    float linear_velocity[3];         // Initial linear velocity
    float angular_velocity[3];        // Initial angular velocity
    bool is_static;                  // Is static body
    bool is_kinematic;                // Is kinematic body
    bool is_sleeping;                 // Is sleeping initially
    float sleep_threshold;            // Sleep threshold
    float ccd_radius;                 // CCD radius
    bool ccd_enabled;                 // CCD enabled
    char collision_group[64];         // Collision group
    char collision_mask[64];          // Collision mask
} RigidBodyAsset;

// ========================================
// Soft Body Asset
// ========================================

typedef struct SoftBodyAsset {
    char name[128];                  // Body name
    int vertex_count;                 // Vertex count
    int triangle_count;               // Triangle count
    int edge_count;                   // Edge count
    float *vertices;                  // Vertex positions
    float *normals;                  // Vertex normals
    uint32_t *triangles;             // Triangle indices
    uint32_t *edges;                  // Edge indices
    float *masses;                    // Vertex masses
    float *velocities;                // Vertex velocities
    float *rest_positions;           // Rest positions
    float material_stiffness;         // Material stiffness
    float material_damping;           // Material damping
    float friction_coefficient;       // Friction coefficient
    float thickness;                  // Body thickness
    bool self_collision;              // Self collision enabled
    int solver_iterations;           // Solver iterations
    float bending_constraints;        // Bending constraint strength
    float volume_conservation;       // Volume conservation factor
} SoftBodyAsset;

// ========================================
// Cloth Asset
// ========================================

typedef struct ClothAsset {
    char name[128];                  // Cloth name
    int width_segments;              // Width segments
    int height_segments;             // Height segments
    float width_size;                // Cloth width
    float height_size;               // Cloth height
    float *vertices;                  // Vertex positions
    float *normals;                  // Vertex normals
    float *uvs;                      // UV coordinates
    uint32_t *triangles;             // Triangle indices
    float *masses;                    // Vertex masses
    float structural_stiffness;       // Structural stiffness
    float shear_stiffness;            // Shear stiffness
    float bend_stiffness;             // Bend stiffness
    float friction_coefficient;       // Friction coefficient
    float thickness;                  // Cloth thickness
    float damping;                   // Damping coefficient
    bool self_collision;              // Self collision enabled
    int solver_iterations;           // Solver iterations
    float time_step;                  // Time step
    bool *pinned_vertices;            // Pinned vertices
    int pinned_count;                 // Number of pinned vertices
} ClothAsset;

// ========================================
// Physics Scene Asset
// ========================================

typedef struct PhysicsSceneAsset {
    char name[128];                  // Scene name
    char description[512];            // Scene description
    float gravity[3];                // Scene gravity
    float time_step;                 // Time step
    int max_substeps;                // Maximum substeps
    bool enable_sleeping;            // Enable sleeping
    float sleep_threshold;           // Sleep threshold
    bool enable_collision_detection;  // Enable collision detection
    bool enable_collision_response;    // Enable collision response
    bool enable_continuous_collision; // Enable continuous collision
    float ccd_safety_margin;          // CCD safety margin
    
    // Scene objects
    uint32_t *rigid_body_ids;        // Rigid body asset IDs
    int rigid_body_count;            // Number of rigid bodies
    uint32_t *soft_body_ids;         // Soft body asset IDs
    int soft_body_count;             // Number of soft bodies
    uint32_t *cloth_ids;             // Cloth asset IDs
    int cloth_count;                 // Number of cloths
    uint32_t *constraint_ids;         // Constraint asset IDs
    int constraint_count;             // Number of constraints
    
    // Scene bounds
    float scene_bounds[6];            // Scene bounding box
    float terrain_height;            // Terrain height
    float water_level;               // Water level
    
} PhysicsSceneAsset;

// ========================================
// Asset Loader
// ========================================

typedef struct AssetLoader {
    char name[64];                   // Loader name
    char file_extension[16];         // File extension
    AssetFileFormat format;           // Supported format
    PhysicsAssetType *supported_types; // Supported asset types
    int type_count;                  // Number of supported types
    
    // Loading functions
    bool (*can_load)(const char *file_path, AssetFileFormat format);
    bool (*load_asset)(const char *file_path, PhysicsAssetType type, 
                      AssetFileFormat format, void **asset_data, AssetInfo *info);
    bool (*save_asset)(const char *file_path, PhysicsAssetType type,
                      AssetFileFormat format, const void *asset_data);
    bool (*unload_asset)(void *asset_data, PhysicsAssetType type);
    bool (*validate_asset)(const void *asset_data, PhysicsAssetType type);
    
    // Utility functions
    bool (*get_asset_info)(const char *file_path, AssetInfo *info);
    size_t (*estimate_memory_usage)(const char *file_path, PhysicsAssetType type);
    bool (*optimize_asset)(void *asset_data, PhysicsAssetType type);
    
} AssetLoader;

// ========================================
// Asset Cache
// ========================================

typedef struct AssetCache {
    AssetInfo *cached_assets;         // Cached asset info
    int cache_size;                   // Cache size
    int cache_capacity;               // Cache capacity
    uint64_t max_memory_usage;        // Maximum memory usage
    uint64_t current_memory_usage;    // Current memory usage
    float cache_hit_rate;             // Cache hit rate
    uint64_t total_requests;          // Total requests
    uint64_t cache_hits;              // Cache hits
    bool lru_enabled;                 // LRU eviction enabled
    uint64_t last_cleanup_time;        // Last cleanup time
} AssetCache;

// ========================================
// Asset Loading System
// ========================================

typedef struct PhysicsAssetLoadingSystem {
    // Asset loaders
    AssetLoader *loaders;              // Loader array
    int loader_count;                 // Number of loaders
    int loader_capacity;              // Loader capacity
    
    // Asset registry
    AssetInfo *assets;                // Asset registry
    int asset_count;                  // Number of assets
    int asset_capacity;               // Asset capacity
    
    // Asset cache
    AssetCache cache;                  // Asset cache
    
    // Loading queue
    uint32_t *loading_queue;          // Loading queue
    int queue_size;                   // Queue size
    int queue_capacity;               // Queue capacity
    int queue_head;                   // Queue head
    int queue_tail;                   // Queue tail
    
    // Async loading
    bool async_loading_enabled;        // Async loading enabled
    int max_concurrent_loads;         // Maximum concurrent loads
    int active_loads;                // Active loads count
    uint64_t next_asset_id;           // Next asset ID
    
    // Statistics
    uint64_t total_loads;             // Total loads
    uint64_t successful_loads;        // Successful loads
    uint64_t failed_loads;            // Failed loads
    uint64_t total_load_time;          // Total load time
    float average_load_time;          // Average load time
    size_t total_memory_usage;        // Total memory usage
    
    // Configuration
    bool cache_enabled;               // Cache enabled
    bool compression_enabled;         // Compression enabled
    bool streaming_enabled;           // Streaming enabled
    bool validation_enabled;          // Validation enabled
    char asset_root_path[512];        // Asset root path
    char cache_path[512];             // Cache path
    
    // User data
    void *user_data;                 // User data pointer
    
} PhysicsAssetLoadingSystem;

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create physics asset loading system
 * @param max_assets Maximum assets
 * @param max_loaders Maximum loaders
 * @param cache_size Cache size
 * @return Asset loading system or NULL on failure
 */
PhysicsAssetLoadingSystem* physics_asset_system_create(int max_assets, int max_loaders, int cache_size);

/**
 * Destroy physics asset loading system
 * @param system Asset loading system
 */
void physics_asset_system_destroy(PhysicsAssetLoadingSystem *system);

// ========================================
// Loader Management
// ========================================

/**
 * Register asset loader
 * @param system Asset loading system
 * @param loader Asset loader
 * @return True if successful
 */
bool physics_asset_register_loader(PhysicsAssetLoadingSystem *system, const AssetLoader *loader);

/**
 * Unregister asset loader
 * @param system Asset loading system
 * @param loader_name Loader name
 * @return True if successful
 */
bool physics_asset_unregister_loader(PhysicsAssetLoadingSystem *system, const char *loader_name);

/**
 * Get loader for file format
 * @param system Asset loading system
 * @param format File format
 * @return Loader or NULL if not found
 */
AssetLoader* physics_asset_get_loader(PhysicsAssetLoadingSystem *system, AssetFileFormat format);

/**
 * Get loader for file extension
 * @param system Asset loading system
 * @param file_extension File extension
 * @return Loader or NULL if not found
 */
AssetLoader* physics_asset_get_loader_by_extension(PhysicsAssetLoadingSystem *system, const char *file_extension);

// ========================================
// Asset Loading
// ========================================

/**
 * Load physics asset
 * @param system Asset loading system
 * @param file_path File path
 * @param type Asset type
 * @param format File format (AUTO to auto-detect)
 * @param flags Loading flags
 * @return Asset ID or 0 on failure
 */
uint32_t physics_asset_load(PhysicsAssetLoadingSystem *system, const char *file_path,
                           PhysicsAssetType type, AssetFileFormat format, uint32_t flags);

/**
 * Load asset asynchronously
 * @param system Asset loading system
 * @param file_path File path
 * @param type Asset type
 * @param format File format
 * @param flags Loading flags
 * @return Asset ID or 0 on failure
 */
uint32_t physics_asset_load_async(PhysicsAssetLoadingSystem *system, const char *file_path,
                                  PhysicsAssetType type, AssetFileFormat format, uint32_t flags);

/**
 * Unload physics asset
 * @param system Asset loading system
 * @param asset_id Asset ID
 * @return True if successful
 */
bool physics_asset_unload(PhysicsAssetLoadingSystem *system, uint32_t asset_id);

/**
 * Reload physics asset
 * @param system Asset loading system
 * @param asset_id Asset ID
 * @return True if successful
 */
bool physics_asset_reload(PhysicsAssetLoadingSystem *system, uint32_t asset_id);

/**
 * Get loaded asset data
 * @param system Asset loading system
 * @param asset_id Asset ID
 * @return Asset data or NULL if not found
 */
void* physics_asset_get_data(PhysicsAssetLoadingSystem *system, uint32_t asset_id);

/**
 * Get asset info
 * @param system Asset loading system
 * @param asset_id Asset ID
 * @return Asset info or NULL if not found
 */
AssetInfo* physics_asset_get_info(PhysicsAssetLoadingSystem *system, uint32_t asset_id);

// ========================================
// Asset Management
// ========================================

/**
 * Find asset by name
 * @param system Asset loading system
 * @param name Asset name
 * @return Asset ID or 0 if not found
 */
uint32_t physics_asset_find_by_name(PhysicsAssetLoadingSystem *system, const char *name);

/**
 * Find assets by type
 * @param system Asset loading system
 * @param type Asset type
 * @param asset_ids Output asset IDs
 * @param max_assets Maximum assets to return
 * @return Number of assets found
 */
int physics_asset_find_by_type(PhysicsAssetLoadingSystem *system, PhysicsAssetType type,
                                 uint32_t *asset_ids, int max_assets);

/**
 * Get asset reference count
 * @param system Asset loading system
 * @param asset_id Asset ID
 * @return Reference count
 */
uint32_t physics_asset_get_reference_count(PhysicsAssetLoadingSystem *system, uint32_t asset_id);

/**
 * Add asset reference
 * @param system Asset loading system
 * @param asset_id Asset ID
 * @return True if successful
 */
bool physics_asset_add_reference(PhysicsAssetLoadingSystem *system, uint32_t asset_id);

/**
 * Remove asset reference
 * @param system Asset loading system
 * @param asset_id Asset ID
 * @return True if successful
 */
bool physics_asset_remove_reference(PhysicsAssetLoadingSystem *system, uint32_t asset_id);

// ========================================
// Cache Management
// ========================================

/**
 * Enable/disable asset cache
 * @param system Asset loading system
 * @param enabled Enable cache
 */
void physics_asset_set_cache_enabled(PhysicsAssetLoadingSystem *system, bool enabled);

/**
 * Set cache size
 * @param system Asset loading system
 * @param cache_size Cache size
 */
void physics_asset_set_cache_size(PhysicsAssetLoadingSystem *system, int cache_size);

/**
 * Set maximum cache memory
 * @param system Asset loading system
 * @param max_memory Maximum memory in bytes
 */
void physics_asset_set_cache_memory_limit(PhysicsAssetLoadingSystem *system, uint64_t max_memory);

/**
 * Clear asset cache
 * @param system Asset loading system
 */
void physics_asset_clear_cache(PhysicsAssetLoadingSystem *system);

/**
 * Get cache statistics
 * @param system Asset loading system
 * @param cache_size Cache size
 * @param cache_hit_rate Cache hit rate
 * @param memory_usage Memory usage
 */
void physics_asset_get_cache_stats(PhysicsAssetLoadingSystem *system, int *cache_size,
                                   float *cache_hit_rate, uint64_t *memory_usage);

// ========================================
// Async Loading
// ========================================

/**
 * Process async loading queue
 * @param system Asset loading system
 * @param max_time Maximum processing time in milliseconds
 * @return Number of assets processed
 */
int physics_asset_process_async_queue(PhysicsAssetLoadingSystem *system, float max_time);

/**
 * Check if asset is loaded
 * @param system Asset loading system
 * @param asset_id Asset ID
 * @return True if loaded
 */
bool physics_asset_is_loaded(PhysicsAssetLoadingSystem *system, uint32_t asset_id);

/**
 * Wait for asset to load
 * @param system Asset loading system
 * @param asset_id Asset ID
 * @param timeout_ms Timeout in milliseconds
 * @return True if loaded
 */
bool physics_asset_wait_for_load(PhysicsAssetLoadingSystem *system, uint32_t asset_id, int timeout_ms);

// ========================================
// Batch Operations
// ========================================

/**
 * Load multiple assets
 * @param system Asset loading system
 * @param file_paths Array of file paths
 * @param types Array of asset types
 * @param count Number of assets
 * @param flags Loading flags
 * @param asset_ids Output asset IDs
 * @return Number of assets loaded
 */
int physics_asset_load_batch(PhysicsAssetLoadingSystem *system, const char **file_paths,
                             PhysicsAssetType *types, int count, uint32_t flags,
                             uint32_t *asset_ids);

/**
 * Unload multiple assets
 * @param system Asset loading system
 * @param asset_ids Array of asset IDs
 * @param count Number of assets
 * @return Number of assets unloaded
 */
int physics_asset_unload_batch(PhysicsAssetLoadingSystem *system, const uint32_t *asset_ids, int count);

// ========================================
// Asset Validation
// ========================================

/**
 * Validate asset
 * @param system Asset loading system
 * @param asset_id Asset ID
 * @return True if valid
 */
bool physics_asset_validate(PhysicsAssetLoadingSystem *system, uint32_t asset_id);

/**
 * Validate all assets
 * @param system Asset loading system
 * @param invalid_assets Output invalid asset IDs
 * @param max_assets Maximum assets to return
 * @return Number of invalid assets
 */
int physics_asset_validate_all(PhysicsAssetLoadingSystem *system, uint32_t *invalid_assets, int max_assets);

// ========================================
// Statistics
// ========================================

/**
 * Get loading statistics
 * @param system Asset loading system
 * @param total_loads Total loads
 * @param successful_loads Successful loads
 * @param failed_loads Failed loads
 * @param average_load_time Average load time
 * @param total_memory_usage Total memory usage
 */
void physics_asset_get_statistics(PhysicsAssetLoadingSystem *system,
                                   uint64_t *total_loads, uint64_t *successful_loads,
                                   uint64_t *failed_loads, float *average_load_time,
                                   size_t *total_memory_usage);

/**
 * Reset statistics
 * @param system Asset loading system
 */
void physics_asset_reset_statistics(PhysicsAssetLoadingSystem *system);

// ========================================
// Configuration
// ========================================

/**
 * Set asset root path
 * @param system Asset loading system
 * @param path Root path
 */
void physics_asset_set_root_path(PhysicsAssetLoadingSystem *system, const char *path);

/**
 * Set cache path
 * @param system Asset loading system
 * @param path Cache path
 */
void physics_asset_set_cache_path(PhysicsAssetLoadingSystem *system, const char *path);

/**
 * Enable/disable compression
 * @param system Asset loading system
 * @param enabled Enable compression
 */
void physics_asset_set_compression_enabled(PhysicsAssetLoadingSystem *system, bool enabled);

/**
 * Set maximum concurrent loads
 * @param system Asset loading system
 * @param max_loads Maximum concurrent loads
 */
void physics_asset_set_max_concurrent_loads(PhysicsAssetLoadingSystem *system, int max_loads);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_ASSET_LOADING_H */
