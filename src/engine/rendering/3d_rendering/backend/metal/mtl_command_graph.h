/*
 * mtl_command_graph.h
 * Manual command buffer dependency tracking (workaround for lack of render graph)
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_COMMAND_GRAPH_H
#define PLATFORM_MTL_COMMAND_GRAPH_H

#include "mtl_command_buffer.h"
#include "mtl_sync_primitives.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct mtl_command_dependency* mtl_command_dependency_t;

typedef enum mtl_dependency_type {
    MTL_DEPENDENCY_TYPE_EXECUTION = 0,  // Wait for command to complete
    MTL_DEPENDENCY_TYPE_MEMORY = 1      // Memory barrier needed
} mtl_dependency_type_t;

/* ============================================================================
 * COMMAND DEPENDENCY API
 * ============================================================================ */

/**
 * Creates a command dependency tracker.
 * @param max_dependencies Maximum number of dependencies to track.
 * @return The dependency tracker.
 */
mtl_command_dependency_t metal_command_dependency_create(unsigned int max_dependencies);

/**
 * Destroys a command dependency tracker.
 * @param tracker The tracker to destroy.
 */
void metal_command_dependency_destroy(mtl_command_dependency_t tracker);

/**
 * Adds a dependency between two command buffers.
 * @param tracker The dependency tracker.
 * @param dependent The command buffer that depends on another.
 * @param dependency The command buffer that must complete first.
 * @param type The type of dependency.
 * @return true if added successfully.
 */
bool metal_command_dependency_add(mtl_command_dependency_t tracker, 
                                   mtl_command_buffer_t dependent,
                                   mtl_command_buffer_t dependency,
                                   mtl_dependency_type_t type);

/**
 * Checks if a command buffer can be submitted (all dependencies met).
 * @param tracker The dependency tracker.
 * @param buffer The command buffer to check.
 * @return true if all dependencies are satisfied.
 */
bool metal_command_dependency_can_submit(mtl_command_dependency_t tracker,
                                         mtl_command_buffer_t buffer);

/**
 * Marks a command buffer as completed (updates dependency graph).
 * @param tracker The dependency tracker.
 * @param buffer The completed command buffer.
 */
void metal_command_dependency_mark_completed(mtl_command_dependency_t tracker,
                                             mtl_command_buffer_t buffer);

/**
 * Waits for all dependencies of a command buffer to complete.
 * @param tracker The dependency tracker.
 * @param buffer The command buffer.
 */
void metal_command_dependency_wait(mtl_command_dependency_t tracker,
                                   mtl_command_buffer_t buffer);

/**
 * Clears all tracked dependencies.
 * @param tracker The dependency tracker.
 */
void metal_command_dependency_reset(mtl_command_dependency_t tracker);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_COMMAND_GRAPH_H */
