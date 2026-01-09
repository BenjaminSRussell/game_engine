/*
 * mtl_hazard_tracking.h
 * Comprehensive resource hazard detection and dependency tracking
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MTL_HAZARD_TRACKING_H
#define MTL_HAZARD_TRACKING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
// Forward declarations
// typedef struct metal_device metal_device_t;
// typedef struct metal_command_buffer metal_command_buffer_t;
typedef struct metal_fence metal_fence_t;

#include "mtl_device.h"

// ============================================================================
// Resource Dependency Graph
// ============================================================================

/**
 * Resource access descriptor.
 */
typedef struct metal_resource_access_desc {
    void* resource;                        // Resource pointer (buffer or texture)
    uint32_t access_flags;                 // Read/write flags
    uint32_t pipeline_stages;              // Pipeline stages accessing resource
    uint64_t frame_index;                  // Frame index of access
    uint32_t pass_index;                   // Render/compute pass index
} metal_resource_access_desc_t;

/**
 * Resource dependency node in the graph.
 */
typedef struct metal_resource_dependency {
    metal_resource_access_desc_t access;
    struct metal_resource_dependency* next; // Linked list
    metal_fence_t* fence;                  // Fence for this dependency
    bool barrier_inserted;                 // Whether barrier was inserted
} metal_resource_dependency_t;

/**
 * Dependency graph for tracking resource accesses.
 */
typedef struct metal_dependency_graph {
    metal_resource_dependency_t** resources; // Hash table of resources
    uint32_t capacity;                     // Hash table capacity
    uint32_t count;                        // Number of tracked resources
    uint64_t current_frame;                // Current frame index
} metal_dependency_graph_t;

/**
 * Create a dependency graph.
 * 
 * @param initial_capacity Initial hash table size
 * @return Dependency graph or NULL on failure
 */
metal_dependency_graph_t* metal_dependency_graph_create(uint32_t initial_capacity);

/**
 * Record a resource access in the dependency graph.
 * 
 * @param graph Dependency graph
 * @param access Resource access descriptor
 * @return true if access was recorded successfully
 */
bool metal_dependency_graph_record_access(metal_dependency_graph_t* graph,
                                          const metal_resource_access_desc_t* access);

/**
 * Check for hazards between a new access and existing accesses.
 * 
 * @param graph Dependency graph
 * @param access New resource access
 * @param out_hazards Output array of hazardous dependencies (can be NULL)
 * @param max_hazards Maximum hazards to return
 * @param out_count Output number of hazards found
 * @return true if hazards detected
 */
bool metal_dependency_graph_check_hazards(metal_dependency_graph_t* graph,
                                          const metal_resource_access_desc_t* access,
                                          metal_resource_dependency_t** out_hazards,
                                          uint32_t max_hazards,
                                          uint32_t* out_count);

/**
 * Advance to next frame, clearing old dependencies.
 * 
 * @param graph Dependency graph
 * @param frame_index New frame index
 */
void metal_dependency_graph_advance_frame(metal_dependency_graph_t* graph, uint64_t frame_index);

/**
 * Clear all dependencies.
 * 
 * @param graph Dependency graph
 */
void metal_dependency_graph_clear(metal_dependency_graph_t* graph);

/**
 * Destroy dependency graph.
 * 
 * @param graph Dependency graph to destroy
 */
void metal_dependency_graph_destroy(metal_dependency_graph_t* graph);

// ============================================================================
// Automatic Barrier Insertion
// ============================================================================

/**
 * Barrier insertion policy.
 */
typedef enum metal_barrier_policy {
    METAL_BARRIER_POLICY_CONSERVATIVE,     // Insert barriers for all hazards
    METAL_BARRIER_POLICY_OPPORTUNISTIC,    // Minimize barriers where safe
    METAL_BARRIER_POLICY_MANUAL            // No automatic insertion
} metal_barrier_policy_t;

/**
 * Barrier manager for automatic barrier insertion.
 */
typedef struct metal_barrier_manager {
    metal_dependency_graph_t* graph;
    metal_barrier_policy_t policy;
    id device;                             // MTLDevice for fence creation
    
    // Statistics
   uint32_t barriers_inserted;
    uint32_t hazards_detected;
    uint32_t false_positives;              // Barriers that weren't needed
} metal_barrier_manager_t;

/**
 * Create a barrier manager.
 * 
 * @param device Metal device (id<MTLDevice>)
 * @param policy Barrier insertion policy
 * @return Barrier manager or NULL on failure
 */
#ifdef __OBJC__
metal_barrier_manager_t* metal_barrier_manager_create(id<MTLDevice> device, metal_barrier_policy_t policy);
#else
metal_barrier_manager_t* metal_barrier_manager_create(id device, metal_barrier_policy_t policy);
#endif

/**
 * Record a resource access and insert barriers if needed.
 * 
 * @param manager Barrier manager
 * @param access Resource access descriptor
 * @param cmd_buffer Command buffer to insert barriers into
 * @return Number of barriers inserted
 */
uint32_t metal_barrier_manager_access_resource(metal_barrier_manager_t* manager,
                                               const metal_resource_access_desc_t* access,
                                               mtl_command_buffer_t* cmd_buffer);

/**
 * Insert explicit barrier for a resource.
 * 
 * @param manager Barrier manager
 * @param resource Resource pointer
 * @param cmd_buffer Command buffer
 * @param stages Pipeline stages
 */
void metal_barrier_manager_insert_barrier(metal_barrier_manager_t* manager,
                                          void* resource,
                                          mtl_command_buffer_t* cmd_buffer,
                                          uint32_t stages);

/**
 * Get barrier statistics.
 * 
 * @param manager Barrier manager
 * @param out_inserted Output number of barriers inserted
 * @param out_hazards Output number of hazards detected
 */
void metal_barrier_manager_get_stats(metal_barrier_manager_t* manager,
                                     uint32_t* out_inserted,
                                     uint32_t* out_hazards);

/**
 * Reset statistics.
 * 
 * @param manager Barrier manager
 */
void metal_barrier_manager_reset_stats(metal_barrier_manager_t* manager);

/**
 * Destroy barrier manager.
 * 
 * @param manager Barrier manager to destroy
 */
void metal_barrier_manager_destroy(metal_barrier_manager_t* manager);

// ============================================================================
// Multi-Pass Resource Tracking
// ============================================================================

/**
 * Render pass resource tracker.
 */
typedef struct metal_pass_resource_tracker {
    metal_dependency_graph_t* graph;
    uint32_t current_pass;
    uint32_t max_passes;
} metal_pass_resource_tracker_t;

/**
 * Create pass resource tracker.
 * 
 * @param max_passes Maximum number of passes to track
 * @return Pass tracker or NULL on failure
 */
metal_pass_resource_tracker_t* metal_pass_tracker_create(uint32_t max_passes);

/**
 * Begin a new render/compute pass.
 * 
 * @param tracker Pass tracker
 * @return Pass index
 */
uint32_t metal_pass_tracker_begin_pass(metal_pass_resource_tracker_t* tracker);

/**
 * End current pass.
 * 
 * @param tracker Pass tracker
 */
void metal_pass_tracker_end_pass(metal_pass_resource_tracker_t* tracker);

/**
 * Record resource access in current pass.
 * 
 * @param tracker Pass tracker
 * @param access Resource access descriptor
 */
void metal_pass_tracker_record_access(metal_pass_resource_tracker_t* tracker,
                                      const metal_resource_access_desc_t* access);

/**
 * Destroy pass tracker.
 * 
 * @param tracker Pass tracker to destroy
 */
void metal_pass_tracker_destroy(metal_pass_resource_tracker_t* tracker);

// ============================================================================
// Cache Coherence Tracking
// ============================================================================

/**
 * Cache coherence state.
 */
typedef enum metal_coherence_state {
    METAL_COHERENCE_INVALID,               // Cache invalid
    METAL_COHERENCE_SHARED,                // Shared read-only
    METAL_COHERENCE_EXCLUSIVE,             // Exclusive read-write
    METAL_COHERENCE_MODIFIED               // Modified, needs flush
} metal_coherence_state_t;

/**
 * Cache coherence tracker.
 */
typedef struct metal_coherence_tracker {
    void* resource;
    metal_coherence_state_t state;
    uint32_t last_writer_stage;            // Pipeline stage that last wrote
    bool needs_flush;                      // Needs cache flush
} metal_coherence_tracker_t;

/**
 * Update coherence state for a resource access.
 * 
 * @param tracker Coherence tracker
 * @param access_type Read/write access type
 * @param stage Pipeline stage
 * @return true if cache flush needed
 */
bool metal_coherence_update(metal_coherence_tracker_t* tracker,
                            uint32_t access_type,
                            uint32_t stage);

/**
 * Check if cache flush is needed.
 * 
 * @param tracker Coherence tracker
 * @return true if flush needed
 */
bool metal_coherence_needs_flush(const metal_coherence_tracker_t* tracker);

/**
 * Reset coherence state.
 * 
 * @param tracker Coherence tracker
 */
void metal_coherence_reset(metal_coherence_tracker_t* tracker);

// ============================================================================
// Hazard Analytics & Reporting
// ============================================================================

/**
 * Hazard report for debugging.
 */
typedef struct metal_hazard_report {
    uint32_t total_hazards;
    uint32_t raw_hazards;                  // Read-after-write
    uint32_t war_hazards;                  // Write-after-read
    uint32_t waw_hazards;                  // Write-after-write
    uint32_t barriers_inserted;
    uint32_t false_dependencies;
    double average_hazard_distance;        // Average distance between hazards (passes)
} metal_hazard_report_t;

/**
 * Generate hazard analytics report.
 * 
 * @param manager Barrier manager
 * @param report Output report structure
 */
void metal_generate_hazard_report(metal_barrier_manager_t* manager, metal_hazard_report_t* report);

/**
 * Print hazard report to console (debug).
 * 
 * @param report Hazard report
 */
void metal_print_hazard_report(const metal_hazard_report_t* report);

// ============================================================================
// Access Flags
// ============================================================================

#define METAL_ACCESS_READ              (1 << 0)
#define METAL_ACCESS_WRITE             (1 << 1)
#define METAL_ACCESS_READ_WRITE        (METAL_ACCESS_READ | METAL_ACCESS_WRITE)

#ifdef __cplusplus
}
#endif

#endif // MTL_HAZARD_TRACKING_H
