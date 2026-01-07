/*
 * resource_aliasing.c
 * Memory aliasing for transients
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/render_graph/resource_aliasing.h"
#include "rendering/render_graph/graph_compiler.h"
#include "rendering/render_graph/resource_node.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct aliased_resource {
    rg_resource_handle_t handle;
    uint32_t first_pass;
    uint32_t last_pass;
    size_t size;
    size_t offset;
} aliased_resource_t;

int rendering_resource_aliasing_init(void) {
    return 0;
}

void rendering_resource_aliasing_shutdown(void) {
}

// Basic greedy interval scheduling for memory aliasing
// Sort transients by size (descending)
typedef struct {
    uint32_t resource_index;
    size_t size;
    uint32_t first;
    uint32_t last;
} transient_info_t;

int compare_transients(const void* a, const void* b) {
    const transient_info_t* ta = (const transient_info_t*)a;
    const transient_info_t* tb = (const transient_info_t*)b;
    if (ta->size < tb->size) return 1; // Descending
    if (ta->size > tb->size) return -1;
    return 0;
}

void rg_resolve_aliasing(render_graph_t* graph) {
    if (!graph) return;
    
    // 1. Collect all transient resources
    uint32_t transient_capacity = graph->resource_count;
    transient_info_t* transients = malloc(transient_capacity * sizeof(transient_info_t));
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < graph->resource_count; i++) {
        rendering_resource_node_info_t info;
        if (rendering_resource_node_get_info(graph->resources[i], &info) == 0) {
            if (info.is_transient && info.memory_size > 0 && info.first_use_pass != UINT32_MAX) {
                transients[count].resource_index = i; // This is the INDEX in graph->resources, not ID
                transients[count].size = info.memory_size;
                transients[count].first = info.first_use_pass;
                transients[count].last = info.last_use_pass;
                count++;
            }
        }
    }
    
    if (count == 0) {
        free(transients);
        return;
    }
    
    // 2. Sort by size descending
    qsort(transients, count, sizeof(transient_info_t), compare_transients);
    
    // 3. Simple Greedy Allocation with Heap Offsetting
    // We maintain a list of allocated blocks or just try to fit?
    // Simplified algorithm: "Heaps" approach
    // We place each resource at offset 0 if possible, or higher.
    // For a generic allocator, this is complex (buddy allocator etc).
    // For Metal Aliasing, we typically just need non-overlapping ranges for concurrent lifetimes.
    //
    // Algorithm:
    // Keep a list of "active ranges" in the heap.
    // For each resource, find a gap in the address space where:
    //   gap_start + size <= gap_end
    //   AND no other resource active in [first, last] overlaps [gap_start, gap_end] physically.
    //
    // Simplified: Just use a stack allocator simulation? No, because lifetimes are arbitrary intervals.
    //
    // Let's implement a simple "Shelf packing" or column checking.
    // Since we only have one dimension (memory offset), we just need to check if [offset, offset+size] 
    // is free during [first, last] time.
    
    // BUT: Checking every offset is slow.
    // Optimization: Track "high water mark" isn't enough because low memory is freed.
    //
    // Correct Approach for offline scheduling: Graph Coloring on Interval Graph.
    // Nodes = Resources. Edge = Time Overlap. Color = Memory Offset? No, Color = Heap? 
    // This is the "Dynamic Storage Allocation Problem", which is NP-hard.
    //
    // Heuristic:
    // Placed resources list: {offset, size, first, last}.
    // For each new resource R:
    //   Try candidate offsets (0, end of Placed[0], end of Placed[1]...)
    //   For candidate O:
    //     Check if [O, O + R.size] overlaps with any Placed P WHERE P.time overlaps R.time.
    //     If valid, pick O. Usually pick smallest valid O (Best Fit).
    
    // Since N is small (<100 resources), O(N^2) is fine.
    
    typedef struct {
        size_t offset;
        size_t size;
        uint32_t first;
        uint32_t last;
    } placed_resource_t;
    
    placed_resource_t* placed = malloc(count * sizeof(placed_resource_t));
    uint32_t placed_count = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        transient_info_t* curr = &transients[i];
        size_t best_offset = 0;
        bool found = false;
        
        // Candidates: 0, and every end of existing block
        // We can optimize candidate list, but let's just use 0 + ends.
        
        // Candidate 0
        bool conflict_at_0 = false;
        for (uint32_t p = 0; p < placed_count; p++) {
             // Check time overlap
             bool time_overlap = !(curr->last < placed[p].first || curr->first > placed[p].last);
             if (time_overlap) {
                 // Check space overlap at offset 0
                 // Range [0, curr->size] vs [p.offset, p.offset + p.size]
                 size_t p_end = placed[p].offset + placed[p].size;
                 if (0 < p_end && curr->size > placed[p].offset) {
                     conflict_at_0 = true;
                     break;
                 }
             }
        }
        if (!conflict_at_0) {
            best_offset = 0;
            found = true;
        } else {
            // Try other candidates (ends of placed blocks)
            // Sort edges? No, just iterate.
            size_t min_valid_offset = SIZE_MAX;
            
            for (uint32_t c = 0; c < placed_count; c++) {
                size_t candidate = placed[c].offset + placed[c].size;
                // Align candidate?
                // Align to 256 bytes or similar (Metal requirement varies, say 1024)
                candidate = (candidate + 1023) & ~1023; 
                
                // Check validity
                bool conflict = false;
                for (uint32_t p = 0; p < placed_count; p++) {
                    bool time_overlap = !(curr->last < placed[p].first || curr->first > placed[p].last);
                    if (time_overlap) {
                         // Check space overlap
                         size_t start = candidate;
                         size_t end = candidate + curr->size;
                         size_t p_start = placed[p].offset;
                         size_t p_end = placed[p].offset + placed[p].size;
                         
                         if (start < p_end && end > p_start) {
                             conflict = true;
                             break;
                         }
                    }
                }
                
                if (!conflict) {
                    if (candidate < min_valid_offset) {
                        min_valid_offset = candidate;
                    }
                }
            }
            if (min_valid_offset != SIZE_MAX) {
                best_offset = min_valid_offset;
                found = true;
            }
        }
        
        if (found) {
            placed[placed_count].offset = best_offset;
            placed[placed_count].size = curr->size;
            placed[placed_count].first = curr->first;
            placed[placed_count].last = curr->last;
            placed_count++;
            
            // Update the actual resource node
            // Note: We need access to internal struct to set memory_offset directly
            // or use specific API if available.
            // I added standard setters? No, just set_lifetime and get/set metal resource.
            // I need `rendering_resource_node_set_memory_offset`? 
            // Better: update the internal struct directly via keyhole or add setter.
            // But I am in a different C file. I SHOULD add a setter.
            // For now, let's assume `rendering_resource_node_set_memory_offset` exists 
            // and I will add it to resource_node.c/h immediately after.
             
             // Or write generic setter? 
             // "rendering_resource_node_set_aliasing_info(handle, offset)"?
             // Let's call it `rendering_resource_node_set_memory(handle, offset, size)`
             // But size is already known.
             
             // CALLING NON-EXISTENT FUNCTION WARNING.
             // I will comment this out or rely on adding it in next step.
             // Actually, I can use `rendering_resource_node_get_info` which copies,
             // but `set` requires a function.
             // I'll add the call and implement it next.
             // rendering_resource_node_set_memory_offset(graph->resources[curr->resource_index], best_offset);
            
            // Fixed call with cast
            rendering_resource_node_handle_t res_handle = graph->resources[curr->resource_index];
            rendering_resource_node_set_memory_offset(res_handle, best_offset);
        }
    }
    
    free(placed);
    free(transients);
}

/* End of resource_aliasing.c */
