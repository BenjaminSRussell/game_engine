/*
 * mtl_command_graph.c
 * Manual command buffer dependency tracking implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "backend/metal/mtl_command_graph.h"
#include <stdlib.h>
#include <string.h>

#if defined(__OBJC__)
#import <Metal/Metal.h>
#define TO_CMD_BUF(x) ((__bridge id<MTLCommandBuffer>)(x))
#endif

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct dependency_entry {
    mtl_command_buffer_t dependent;
    mtl_command_buffer_t dependency;
    mtl_dependency_type_t type;
    bool satisfied;
} dependency_entry_t;

struct mtl_command_dependency {
    dependency_entry_t* entries;
    unsigned int max_dependencies;
    unsigned int count;
};

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

mtl_command_dependency_t metal_command_dependency_create(unsigned int max_dependencies) {
    mtl_command_dependency_t tracker = malloc(sizeof(struct mtl_command_dependency));
    if (!tracker) return NULL;
    
    tracker->entries = calloc(max_dependencies, sizeof(dependency_entry_t));
    if (!tracker->entries) {
        free(tracker);
        return NULL;
    }
    
    tracker->max_dependencies = max_dependencies;
    tracker->count = 0;
    
    return tracker;
}

void metal_command_dependency_destroy(mtl_command_dependency_t tracker) {
    if (tracker) {
        free(tracker->entries);
        free(tracker);
    }
}

bool metal_command_dependency_add(mtl_command_dependency_t tracker,
                                   mtl_command_buffer_t dependent,
                                   mtl_command_buffer_t dependency,
                                   mtl_dependency_type_t type) {
    if (!tracker || !dependent || !dependency) return false;
    if (tracker->count >= tracker->max_dependencies) return false;
    
    dependency_entry_t* entry = &tracker->entries[tracker->count++];
    entry->dependent = dependent;
    entry->dependency = dependency;
    entry->type = type;
    entry->satisfied = false;
    
    return true;
}

bool metal_command_dependency_can_submit(mtl_command_dependency_t tracker,
                                         mtl_command_buffer_t buffer) {
    if (!tracker || !buffer) return true; // No tracker = no dependencies
    
    // Check all dependencies for this buffer
    for (unsigned int i = 0; i < tracker->count; i++) {
        dependency_entry_t* entry = &tracker->entries[i];
        if (entry->dependent == buffer && !entry->satisfied) {
            return false; // Unsatisfied dependency
        }
    }
    
    return true;
}

void metal_command_dependency_mark_completed(mtl_command_dependency_t tracker,
                                             mtl_command_buffer_t buffer) {
    if (!tracker || !buffer) return;
    
    // Mark all dependencies on this buffer as satisfied
    for (unsigned int i = 0; i < tracker->count; i++) {
        dependency_entry_t* entry = &tracker->entries[i];
        if (entry->dependency == buffer) {
            entry->satisfied = true;
        }
    }
}

void metal_command_dependency_wait(mtl_command_dependency_t tracker,
                                   mtl_command_buffer_t buffer) {
#if defined(__OBJC__)
    if (!tracker || !buffer) return;
    
    // Wait for all dependencies
    for (unsigned int i = 0; i < tracker->count; i++) {
        dependency_entry_t* entry = &tracker->entries[i];
        if (entry->dependent == buffer && !entry->satisfied) {
            // Wait for dependency to complete
            id<MTLCommandBuffer> dep_buf = TO_CMD_BUF(entry->dependency);
            [dep_buf waitUntilCompleted];
            entry->satisfied = true;
        }
    }
#endif
}

void metal_command_dependency_reset(mtl_command_dependency_t tracker) {
    if (!tracker) return;
    tracker->count = 0;
    memset(tracker->entries, 0, tracker->max_dependencies * sizeof(dependency_entry_t));
}
