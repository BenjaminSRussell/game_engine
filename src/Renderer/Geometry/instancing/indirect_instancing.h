/*
 * indirect_instancing.h
 * Indirect draw instancing management
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_INDIRECT_INSTANCING_H
#define GEOMETRY_INDIRECT_INSTANCING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math/mat4.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_indirect_instancing_handle {
    uint32_t id;
} geometry_indirect_instancing_handle_t;

typedef struct geometry_indirect_instancing_desc {
    uint32_t max_draws;
    uint32_t flags;
    void* user_data;
} geometry_indirect_instancing_desc_t;

// Matches VkDrawIndexedIndirectCommand
typedef struct {
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t  vertexOffset;
    uint32_t firstInstance;
} IndirectDrawCommand;

// Structure for compute shader culling/output
// This is effectively the same as above but might be used in SSBO
typedef IndirectDrawCommand GPUIndirectCommand;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_indirect_instancing_init(void);
void geometry_indirect_instancing_shutdown(void);

/* Lifecycle */
int geometry_indirect_instancing_create(geometry_indirect_instancing_handle_t* out_handle, const geometry_indirect_instancing_desc_t* desc);
void geometry_indirect_instancing_destroy(geometry_indirect_instancing_handle_t handle);

/* Operations */

// Add a draw command to the CPU-side list, to be uploaded or processed
int geometry_indirect_instancing_add_command(geometry_indirect_instancing_handle_t handle, 
                                             const IndirectDrawCommand* command);

// Reset the command count for the next frame
void geometry_indirect_instancing_reset(geometry_indirect_instancing_handle_t handle);

// Upload commands to GPU buffer (for non-culling drawing or initial state)
int geometry_indirect_instancing_upload(geometry_indirect_instancing_handle_t handle);

/* Access */
uint32_t geometry_indirect_instancing_get_buffer_id(geometry_indirect_instancing_handle_t handle);
uint32_t geometry_indirect_instancing_get_count_buffer_id(geometry_indirect_instancing_handle_t handle); // For MDI with count buffer
uint32_t geometry_indirect_instancing_get_draw_count(geometry_indirect_instancing_handle_t handle);

// Get actual Vulkan buffer handles (for rendering)
VkBuffer geometry_indirect_instancing_get_cmd_buffer(geometry_indirect_instancing_handle_t handle);
VkBuffer geometry_indirect_instancing_get_count_buffer(geometry_indirect_instancing_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_INDIRECT_INSTANCING_H */
