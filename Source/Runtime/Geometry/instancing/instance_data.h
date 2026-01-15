/*
 * instance_data.h
 * Instance data management for GPU-driven rendering
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_INSTANCE_DATA_H
#define GEOMETRY_INSTANCE_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef struct metal_device metal_device_t;
typedef struct metal_buffer metal_buffer_t;

/* ============================================================================
 * TYPES & STRUCTURES
 * ============================================================================ */

// Per-instance transform data (4x4 matrix)
typedef struct instance_transform {
    float matrix[16];  // Column-major 4x4 transform matrix
} instance_transform_t;

// Per-instance material parameter overrides
typedef struct instance_material_params {
    float base_color[4];     // RGBA color tint
    float roughness;         // Roughness override [0-1]
    float metallic;          // Metallic override [0-1]
    float emission;          // Emission intensity
    uint32_t texture_index;  // Texture array index (for bindless)
    uint32_t padding[3];     // Align to 16 bytes
} instance_material_params_t;

// Per-instance visibility and culling flags
typedef struct instance_visibility_flags {
    uint32_t visible : 1;           // Visibility flag
    uint32_t cast_shadow : 1;       // Shadow casting
    uint32_t receive_shadow : 1;    // Shadow receiving
    uint32_t culling_enabled : 1;   // Enable GPU culling
    uint32_t lod_level : 3;         // Current LOD level (0-7)
    uint32_t user_flags : 25;       // User-defined flags
} instance_visibility_flags_t;

// Complete instance data (tightly packed for GPU)
typedef struct instance_data {
    instance_transform_t transform;
    instance_material_params_t material_params;
    instance_visibility_flags_t visibility_flags;
    uint32_t mesh_id;               // Mesh identifier
    uint32_t material_id;           // Material identifier
    uint32_t custom_data[2];        // User-defined data
} instance_data_t;

// Instance buffer descriptor
typedef struct instance_buffer_desc {
    uint32_t max_instances;         // Maximum instance count
    uint32_t initial_count;         // Initial instance count
    const instance_data_t* initial_data; // Initial data (optional)
    bool allow_resize;              // Allow dynamic resizing
    bool gpu_writable;              // Allow GPU writes (for culling)
    const char* label;              // Debug label
} instance_buffer_desc_t;

// Instance buffer handle
typedef struct instance_buffer {
    metal_buffer_t* buffer;         // Metal buffer
    uint32_t max_instances;         // Capacity
    uint32_t instance_count;        // Current count
    size_t instance_stride;         // Size per instance
    bool allow_resize;
    bool gpu_writable;
    
    // Statistics
    uint32_t update_count;          // Number of updates
    size_t total_bytes;             // Total buffer size
    
    // Debug
    char label[64];
} instance_buffer_t;

// Instance pool descriptor
typedef struct instance_pool_desc {
    uint32_t initial_capacity;      // Initial pool capacity
    uint32_t growth_factor;         // Growth multiplier (e.g., 2)
    size_t alignment;               // Memory alignment
    const char* label;
} instance_pool_desc_t;

// Instance pool for managing allocations
typedef struct instance_pool {
    void* memory;                   // Pool memory
    uint32_t capacity;              // Total capacity
    uint32_t allocated;             // Currently allocated
    uint32_t growth_factor;
    size_t alignment;
    
    // Free list for reuse
    uint32_t* free_list;
    uint32_t free_count;
    
    // Statistics
    uint32_t peak_allocated;
    uint32_t total_allocations;
    
    char label[64];
} instance_pool_t;

// Instance buffer statistics
typedef struct instance_buffer_stats {
    uint32_t total_buffers;
    uint32_t total_instances;
    size_t total_memory_bytes;
    size_t peak_memory_bytes;
    uint32_t total_updates;
} instance_buffer_stats_t;

/* ============================================================================
 * API - INSTANCE BUFFER MANAGEMENT
 * ============================================================================ */

/**
 * Initialize instance data system
 * Must be called before any other instance functions
 */
int instance_data_init(void);

/**
 * Shutdown instance data system
 * Frees all resources
 */
void instance_data_shutdown(void);

/**
 * Create an instance buffer
 * @param device Metal device
 * @param desc Buffer descriptor
 * @return Instance buffer or NULL on failure
 */
instance_buffer_t* instance_buffer_create(metal_device_t* device, 
                                          const instance_buffer_desc_t* desc);

/**
 * Destroy an instance buffer
 */
void instance_buffer_destroy(instance_buffer_t* buffer);

/**
 * Update instance data
 * @param buffer Instance buffer
 * @param instances Array of instance data
 * @param count Number of instances
 * @param offset Starting offset
 * @return 0 on success, -1 on failure
 */
int instance_buffer_update(instance_buffer_t* buffer,
                          const instance_data_t* instances,
                          uint32_t count,
                          uint32_t offset);

/**
 * Update a single instance
 */
int instance_buffer_update_single(instance_buffer_t* buffer,
                                  const instance_data_t* instance,
                                  uint32_t index);

/**
 * Resize instance buffer (if allowed)
 * @param buffer Instance buffer
 * @param new_capacity New capacity
 * @return 0 on success, -1 on failure
 */
int instance_buffer_resize(instance_buffer_t* buffer, uint32_t new_capacity);

/**
 * Get instance count
 */
uint32_t instance_buffer_get_count(const instance_buffer_t* buffer);

/**
 * Set instance count (for dynamic instance management)
 */
void instance_buffer_set_count(instance_buffer_t* buffer, uint32_t count);

/**
 * Get Metal buffer for GPU binding
 */
metal_buffer_t* instance_buffer_get_metal_buffer(instance_buffer_t* buffer);

/**
 * Validate instance buffer state
 */
bool instance_buffer_validate(const instance_buffer_t* buffer);

/* ============================================================================
 * API - INSTANCE POOL MANAGEMENT
 * ============================================================================ */

/**
 * Create an instance pool
 */
instance_pool_t* instance_pool_create(const instance_pool_desc_t* desc);

/**
 * Destroy an instance pool
 */
void instance_pool_destroy(instance_pool_t* pool);

/**
 * Allocate instance slots from pool
 * @param pool Instance pool
 * @param count Number of instances to allocate
 * @param out_index Output index of first allocated instance
 * @return 0 on success, -1 on failure
 */
int instance_pool_alloc(instance_pool_t* pool, uint32_t count, uint32_t* out_index);

/**
 * Free instance slots back to pool
 */
int instance_pool_free(instance_pool_t* pool, uint32_t index, uint32_t count);

/**
 * Reset pool (free all allocations)
 */
void instance_pool_reset(instance_pool_t* pool);

/**
 * Check if pool can allocate N instances
 */
bool instance_pool_can_allocate(const instance_pool_t* pool, uint32_t count);

/* ============================================================================
 * API - UTILITIES & STATISTICS
 * ============================================================================ */

/**
 * Get global instance buffer statistics
 */
instance_buffer_stats_t instance_buffer_get_stats(void);

/**
 * Calculate memory size for N instances
 */
size_t instance_data_calculate_size(uint32_t instance_count);

/**
 * Validate instance data structure
 */
bool instance_data_validate(const instance_data_t* instance);

/**
 * Debug print instance buffer info
 */
void instance_buffer_debug_print(const instance_buffer_t* buffer);

/**
 * Debug print instance pool info
 */
void instance_pool_debug_print(const instance_pool_t* pool);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_INSTANCE_DATA_H */
