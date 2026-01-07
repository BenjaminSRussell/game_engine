/*
 * lod_memory.h
 * LOD memory management and budgeting
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_LOD_MEMORY_H
#define GEOMETRY_LOD_MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "lod_generator.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

// Memory budget configuration
typedef struct lod_memory_budget {
    size_t total_budget_bytes;      // Total memory budget for LODs
    size_t cpu_budget_bytes;        // CPU memory budget
    size_t gpu_budget_bytes;        // GPU memory budget
    float eviction_threshold;       // Threshold to trigger eviction [0-1]
    bool allow_over_budget;         // Allow temporary over-budget
} lod_memory_budget_t;

// Memory usage statistics
typedef struct lod_memory_stats {
    size_t total_allocated;         // Total allocated
    size_t cpu_allocated;           // CPU allocated
    size_t gpu_allocated;           // GPU allocated
    size_t lod_chain_count;         // Number of LOD chains loaded
    size_t total_lod_meshes;        // Total LOD mesh count
    float budget_usage_percent;     // Budget usage percentage
    uint32_t eviction_count;        // Number of evictions
} lod_memory_stats_t;

// LOD memory pool
typedef struct lod_memory_pool {
    void* memory_block;
    size_t block_size;
    size_t used_bytes;
    uint32_t allocation_count;
    bool is_gpu_pool;
} lod_memory_pool_t;

// Memory tracking entry
typedef struct lod_memory_entry {
    uint32_t chain_id;
    size_t cpu_size;
    size_t gpu_size;
    uint64_t last_access_frame;
    float priority;
    bool pinned;                    // Cannot be evicted
} lod_memory_entry_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lod_memory_init(const lod_memory_budget_t* budget);
void lod_memory_shutdown(void);

/* Budget Management */
void lod_memory_set_budget(const lod_memory_budget_t* budget);
lod_memory_budget_t lod_memory_get_budget(void);
lod_memory_stats_t lod_memory_get_stats(void);

/* Memory Allocation */
void* lod_memory_alloc(size_t size, bool is_gpu);
void lod_memory_free(void* ptr, bool is_gpu);

/* LOD Chain Tracking */
int lod_memory_register_chain(uint32_t chain_id, const lod_chain_t* chain);
void lod_memory_unregister_chain(uint32_t chain_id);
void lod_memory_update_access(uint32_t chain_id, uint64_t frame_number);

/* Memory Estimation */
size_t lod_memory_estimate_chain_size(const lod_chain_t* chain);
size_t lod_memory_estimate_mesh_size(const mesh_data_t* mesh);

/* Eviction */
bool lod_memory_is_over_budget(void);
int lod_memory_evict_lru(size_t bytes_needed);
void lod_memory_set_chain_priority(uint32_t chain_id, float priority);
void lod_memory_pin_chain(uint32_t chain_id, bool pinned);

/* Debug */
void lod_memory_print_stats(void);
void lod_memory_validate(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_LOD_MEMORY_H */
