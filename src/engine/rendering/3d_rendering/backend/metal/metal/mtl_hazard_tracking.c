/*
 * mtl_hazard_tracking.c
 * Comprehensive resource hazard detection and dependency tracking implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_hazard_tracking.h"
#include "mtl_sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Hash Table Helpers
// ============================================================================

static uint32_t hash_pointer(void* ptr, uint32_t capacity) {
    uintptr_t addr = (uintptr_t)ptr;
    // Simple hash function
    return (uint32_t)((addr >> 3) % capacity);
}

// ============================================================================
// Dependency Graph Implementation
// ============================================================================

metal_dependency_graph_t* metal_dependency_graph_create(uint32_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 128;
    
    metal_dependency_graph_t* graph = (metal_dependency_graph_t*)calloc(1, sizeof(metal_dependency_graph_t));
    if (!graph) return NULL;
    
    graph->resources = (metal_resource_dependency_t**)calloc(initial_capacity, sizeof(metal_resource_dependency_t*));
    if (!graph->resources) {
        free(graph);
        return NULL;
    }
    
    graph->capacity = initial_capacity;
    return graph;
}

bool metal_dependency_graph_record_access(metal_dependency_graph_t* graph,
                                          const metal_resource_access_desc_t* access) {
    if (!graph || !access || !access->resource) return false;
    
    uint32_t index = hash_pointer(access->resource, graph->capacity);
    
    // Create new dependency node
    metal_resource_dependency_t* dep = (metal_resource_dependency_t*)calloc(1, sizeof(metal_resource_dependency_t));
    if (!dep) return false;
    
    dep->access = *access;
    dep->next = graph->resources[index];
    graph->resources[index] = dep;
    graph->count++;
    
    return true;
}

bool metal_dependency_graph_check_hazards(metal_dependency_graph_t* graph,
                                          const metal_resource_access_desc_t* access,
                                          metal_resource_dependency_t** out_hazards,
                                          uint32_t max_hazards,
                                          uint32_t* out_count) {
    if (!graph || !access || !access->resource) return false;
    
    uint32_t index = hash_pointer(access->resource, graph->capacity);
    uint32_t hazard_count = 0;
    bool has_hazards = false;
    
    // Check all accesses for this resource
    metal_resource_dependency_t* dep = graph->resources[index];
    while (dep) {
        // Only check if same resource
        if (dep->access.resource == access->resource) {
            // Check for hazards
            bool is_write_access = (access->access_flags & METAL_ACCESS_WRITE) != 0;
            bool was_write_access = (dep->access.access_flags & METAL_ACCESS_WRITE) != 0;
            bool is_read_access = (access->access_flags & METAL_ACCESS_READ) != 0;
            
            // RAW, WAR, or WAW hazard
            if ((was_write_access && is_read_access) ||   // RAW
                (was_write_access && is_write_access) ||   // WAW
                (!was_write_access && is_write_access)) {  // WAR
                
                has_hazards = true;
                
                if (out_hazards && hazard_count < max_hazards) {
                    out_hazards[hazard_count++] = dep;
                }
            }
        }
        dep = dep->next;
    }
    
    if (out_count) *out_count = hazard_count;
    return has_hazards;
}

void metal_dependency_graph_advance_frame(metal_dependency_graph_t* graph, uint64_t frame_index) {
    if (!graph) return;
    
    graph->current_frame = frame_index;
    
    // Clean up old dependencies (older than 3 frames)
    for (uint32_t i = 0; i < graph->capacity; i++) {
        metal_resource_dependency_t** current = &graph->resources[i];
        
        while (*current) {
            metal_resource_dependency_t* dep = *current;
            
            if (frame_index - dep->access.frame_index > 3) {
                *current = dep->next;
                
                if (dep->fence) {
                    metal_fence_destroy(dep->fence);
                }
                free(dep);
                graph->count--;
            } else {
                current = &(*current)->next;
            }
        }
    }
}

void metal_dependency_graph_clear(metal_dependency_graph_t* graph) {
    if (!graph) return;
    
    for (uint32_t i = 0; i < graph->capacity; i++) {
        metal_resource_dependency_t* dep = graph->resources[i];
        
        while (dep) {
            metal_resource_dependency_t* next = dep->next;
            
            if (dep->fence) {
                metal_fence_destroy(dep->fence);
            }
            free(dep);
            dep = next;
        }
        
        graph->resources[i] = NULL;
    }
    
    graph->count = 0;
}

void metal_dependency_graph_destroy(metal_dependency_graph_t* graph) {
    if (!graph) return;
    
    metal_dependency_graph_clear(graph);
    
    if (graph->resources) {
        free(graph->resources);
    }
    
    free(graph);
}

// ============================================================================
// Barrier Manager Implementation
// ============================================================================

metal_barrier_manager_t* metal_barrier_manager_create(id device, metal_barrier_policy_t policy) {
    if (!device) return NULL;
    
    metal_barrier_manager_t* manager = (metal_barrier_manager_t*)calloc(1, sizeof(metal_barrier_manager_t));
    if (!manager) return NULL;
    
    manager->graph = metal_dependency_graph_create(256);
    if (!manager->graph) {
        free(manager);
        return NULL;
    }
    
    manager->policy = policy;
    manager->device = device;
    
    return manager;
}

uint32_t metal_barrier_manager_access_resource(metal_barrier_manager_t* manager,
                                               const metal_resource_access_desc_t* access,
                                               metal_command_buffer_t* cmd_buffer) {
    if (!manager || !access || !cmd_buffer) return 0;
    
    uint32_t barriers_count = 0;
    
    // Check for hazards
    metal_resource_dependency_t* hazards[16];
    uint32_t hazard_count = 0;
    
    bool has_hazards = metal_dependency_graph_check_hazards(
        manager->graph, access, hazards, 16, &hazard_count);
    
    if (has_hazards) {
        manager->hazards_detected++;
        
        // Insert barriers based on policy
        if (manager->policy == METAL_BARRIER_POLICY_CONSERVATIVE ||
            manager->policy == METAL_BARRIER_POLICY_OPPORTUNISTIC) {
            
            // Insert barrier
            metal_insert_texture_barrier(cmd_buffer, access->resource, access->pipeline_stages);
            barriers_count++;
            manager->barriers_inserted++;
        }
    }
    
    // Record this access
    metal_dependency_graph_record_access(manager->graph, access);
    
    return barriers_count;
}

void metal_barrier_manager_insert_barrier(metal_barrier_manager_t* manager,
                                          void* resource,
                                          metal_command_buffer_t* cmd_buffer,
                                          uint32_t stages) {
    if (!manager || !resource || !cmd_buffer) return;
    
    metal_insert_texture_barrier(cmd_buffer, resource, stages);
    manager->barriers_inserted++;
}

void metal_barrier_manager_get_stats(metal_barrier_manager_t* manager,
                                     uint32_t* out_inserted,
                                     uint32_t* out_hazards) {
    if (!manager) return;
    
    if (out_inserted) *out_inserted = manager->barriers_inserted;
    if (out_hazards) *out_hazards = manager->hazards_detected;
}

void metal_barrier_manager_reset_stats(metal_barrier_manager_t* manager) {
    if (!manager) return;
    
    manager->barriers_inserted = 0;
    manager->hazards_detected = 0;
    manager->false_positives = 0;
}

void metal_barrier_manager_destroy(metal_barrier_manager_t* manager) {
    if (!manager) return;
    
    if (manager->graph) {
        metal_dependency_graph_destroy(manager->graph);
    }
    
    free(manager);
}

// ============================================================================
// Pass Resource Tracker Implementation
// ============================================================================

metal_pass_resource_tracker_t* metal_pass_tracker_create(uint32_t max_passes) {
    if (max_passes == 0) max_passes = 32;
    
    metal_pass_resource_tracker_t* tracker = 
        (metal_pass_resource_tracker_t*)calloc(1, sizeof(metal_pass_resource_tracker_t));
    
    if (!tracker) return NULL;
    
    tracker->graph = metal_dependency_graph_create(128);
    if (!tracker->graph) {
        free(tracker);
        return NULL;
    }
    
    tracker->max_passes = max_passes;
    return tracker;
}

uint32_t metal_pass_tracker_begin_pass(metal_pass_resource_tracker_t* tracker) {
    if (!tracker) return 0;
    
    tracker->current_pass++;
    return tracker->current_pass;
}

void metal_pass_tracker_end_pass(metal_pass_resource_tracker_t* tracker) {
    // No-op for now, could add pass finalization logic
}

void metal_pass_tracker_record_access(metal_pass_resource_tracker_t* tracker,
                                      const metal_resource_access_desc_t* access) {
    if (!tracker || !access) return;
    
    metal_resource_access_desc_t modified_access = *access;
    modified_access.pass_index = tracker->current_pass;
    
    metal_dependency_graph_record_access(tracker->graph, &modified_access);
}

void metal_pass_tracker_destroy(metal_pass_resource_tracker_t* tracker) {
    if (!tracker) return;
    
    if (tracker->graph) {
        metal_dependency_graph_destroy(tracker->graph);
    }
    
    free(tracker);
}

// ============================================================================
// Cache Coherence Implementation
// ============================================================================

bool metal_coherence_update(metal_coherence_tracker_t* tracker,
                            uint32_t access_type,
                            uint32_t stage) {
    if (!tracker) return false;
    
    bool needs_flush = false;
    
    if (access_type & METAL_ACCESS_WRITE) {
        // Write access
        if (tracker->state == METAL_COHERENCE_SHARED ||
            tracker->state == METAL_COHERENCE_EXCLUSIVE) {
            // Transition to modified, need flush if was shared
            needs_flush = (tracker->state == METAL_COHERENCE_SHARED);
        }
        
        tracker->state = METAL_COHERENCE_MODIFIED;
        tracker->last_writer_stage = stage;
        tracker->needs_flush = true;
    } else {
        // Read access
        if (tracker->state == METAL_COHERENCE_MODIFIED) {
            // Need to flush before read
            needs_flush = true;
            tracker->state = METAL_COHERENCE_EXCLUSIVE;
        } else if (tracker->state == METAL_COHERENCE_INVALID) {
            tracker->state = METAL_COHERENCE_SHARED;
        }
    }
    
    return needs_flush;
}

bool metal_coherence_needs_flush(const metal_coherence_tracker_t* tracker) {
    if (!tracker) return false;
    return tracker->needs_flush;
}

void metal_coherence_reset(metal_coherence_tracker_t* tracker) {
    if (!tracker) return;
    
    tracker->state = METAL_COHERENCE_INVALID;
    tracker->last_writer_stage = 0;
    tracker->needs_flush = false;
}

// ============================================================================
// Hazard Analytics Implementation
// ============================================================================

void metal_generate_hazard_report(metal_barrier_manager_t* manager, metal_hazard_report_t* report) {
    if (!manager || !report) return;
    
    memset(report, 0, sizeof(metal_hazard_report_t));
    
    report->total_hazards = manager->hazards_detected;
    report->barriers_inserted = manager->barriers_inserted;
    report->false_dependencies = manager->false_positives;
    
    // Detailed hazard type analysis would require tracking hazard types
    // For now, provide basic stats
    report->raw_hazards = manager->hazards_detected / 3;  // Rough estimate
    report->war_hazards = manager->hazards_detected / 3;
    report->waw_hazards = manager->hazards_detected / 3;
}

void metal_print_hazard_report(const metal_hazard_report_t* report) {
    if (!report) return;
    
    printf("=== Metal Hazard Analytics Report ===\n");
    printf("Total Hazards:        %u\n", report->total_hazards);
    printf("  - RAW Hazards:      %u\n", report->raw_hazards);
    printf("  - WAR Hazards:      %u\n", report->war_hazards);
    printf("  - WAW Hazards:      %u\n", report->waw_hazards);
    printf("Barriers Inserted:    %u\n", report->barriers_inserted);
    printf("False Dependencies:   %u\n", report->false_dependencies);
    printf("Avg Hazard Distance:  %.2f passes\n", report->average_hazard_distance);
    printf("===================================\n");
}
