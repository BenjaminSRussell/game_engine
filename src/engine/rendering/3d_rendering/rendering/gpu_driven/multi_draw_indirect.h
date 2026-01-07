/*
 * multi_draw_indirect.h
 * Multi-draw indirect rendering with Metal ICB support
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_MULTI_DRAW_INDIRECT_H
#define RENDERING_MULTI_DRAW_INDIRECT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef struct metal_device metal_device_t;
typedef struct metal_buffer metal_buffer_t;

#ifdef __OBJC__
@class MTLIndirectCommandBuffer;
@class MTLCommandBuffer;
@class MTLRenderCommandEncoder;
#else
typedef void MTLIndirectCommandBuffer;
typedef void MTLCommandBuffer;
typedef void MTLRenderCommandEncoder;
#endif

/* ============================================================================
 * TYPES & STRUCTURES
 * ============================================================================ */

// Metal indirect draw arguments (matches MTLDrawIndexedPrimitivesIndirectArguments)
typedef struct indirect_draw_indexed_args {
    uint32_t index_count;
    uint32_t instance_count;
    uint32_t index_start;
    int32_t  base_vertex;
    uint32_t base_instance;
} indirect_draw_indexed_args_t;

// Non-indexed variant (matches MTLDrawPrimitivesIndirectArguments)
typedef struct indirect_draw_args {
    uint32_t vertex_count;
    uint32_t instance_count;
    uint32_t vertex_start;
    uint32_t base_instance;
} indirect_draw_args_t;

// Indirect command buffer descriptor
typedef struct indirect_command_buffer_desc {
    uint32_t max_command_count;     // Maximum number of commands
    bool supports_indexed;          // Support indexed draws
    bool supports_non_indexed;      // Support non-indexed draws
    bool inherit_pipeline_state;    // Inherit PSO from encoder
    bool inherit_buffers;           // Inherit bound buffers
    const char* label;
} indirect_command_buffer_desc_t;

// Indirect command buffer
typedef struct indirect_command_buffer {
    MTLIndirectCommandBuffer* icb;  // Metal ICB
    metal_buffer_t* argument_buffer; // Argument buffer
    uint32_t max_commands;
    uint32_t command_count;         // Current command count
    bool supports_indexed;
    bool supports_non_indexed;
    
    // Statistics
    uint32_t total_draws;
    uint32_t total_instances;
    
    char label[64];
} indirect_command_buffer_t;

// Multi-draw indirect handle
typedef struct rendering_multi_draw_indirect_handle {
    uint32_t id;
} rendering_multi_draw_indirect_handle_t;

// Multi-draw indirect descriptor
typedef struct rendering_multi_draw_indirect_desc {
    uint32_t max_draw_count;        // Maximum concurrent draws
    bool use_indirect_command_buffer; // Use Metal ICB
    bool indexed;                   // Indexed vs non-indexed
    uint32_t flags;
    void* user_data;
} rendering_multi_draw_indirect_desc_t;

// Multi-draw indirect info
typedef struct rendering_multi_draw_indirect_info {
    uint32_t id;
    uint32_t max_draw_count;
    uint32_t current_draw_count;
    uint32_t flags;
    bool initialized;
    bool uses_icb;
} rendering_multi_draw_indirect_info_t;

// Multi-draw statistics
typedef struct multi_draw_stats {
    uint32_t total_draw_calls;      // Total draw calls issued
    uint32_t total_instances;       // Total instances rendered
    uint32_t batches_executed;      // Number of batches
    float draw_call_reduction;      // Percentage reduction vs individual draws
} multi_draw_stats_t;

/* ============================================================================
 * API - INDIRECT COMMAND BUFFER (Metal ICB)
 * ============================================================================ */

/**
 * Create a Metal indirect command buffer
 * @param device Metal device
 * @param desc ICB descriptor
 * @return ICB or NULL on failure
 */
indirect_command_buffer_t* indirect_command_buffer_create(
    metal_device_t* device,
    const indirect_command_buffer_desc_t* desc);

/**
 * Destroy indirect command buffer
 */
void indirect_command_buffer_destroy(indirect_command_buffer_t* icb);

/**
 * Reset indirect command buffer (clear all commands)
 */
void indirect_command_buffer_reset(indirect_command_buffer_t* icb);

/**
 * Set command count for execution
 */
void indirect_command_buffer_set_count(indirect_command_buffer_t* icb, uint32_t count);

/**
 * Execute indirect command buffer
 * @param icb Indirect command buffer
 * @param encoder Render command encoder
 * @param command_buffer Metal command buffer
 */
void indirect_command_buffer_execute(
    indirect_command_buffer_t* icb,
    MTLRenderCommandEncoder* encoder,
    MTLCommandBuffer* command_buffer);

/**
 * Get Metal ICB for GPU writing
 */
MTLIndirectCommandBuffer* indirect_command_buffer_get_metal_icb(
    indirect_command_buffer_t* icb);

/* ============================================================================
 * API - MULTI-DRAW INDIRECT
 * ============================================================================ */

/* Initialization */
int rendering_multi_draw_indirect_init(void);
void rendering_multi_draw_indirect_shutdown(void);

/* Lifecycle */
int rendering_multi_draw_indirect_create(
    rendering_multi_draw_indirect_handle_t* out_handle, 
    const rendering_multi_draw_indirect_desc_t* desc);
void rendering_multi_draw_indirect_destroy(
    rendering_multi_draw_indirect_handle_t handle);

/* Operations */
int rendering_multi_draw_indirect_update(
    rendering_multi_draw_indirect_handle_t handle, 
    const void* data, 
    size_t size);

bool rendering_multi_draw_indirect_is_valid(
    rendering_multi_draw_indirect_handle_t handle);

int rendering_multi_draw_indirect_get_info(
    rendering_multi_draw_indirect_handle_t handle, 
    rendering_multi_draw_indirect_info_t* out_info);

void rendering_multi_draw_indirect_mark_dirty(
    rendering_multi_draw_indirect_handle_t handle);

int rendering_multi_draw_indirect_process_pending(void);

/**
 * Execute multi-draw indirect
 */
int rendering_multi_draw_indirect_execute(
    rendering_multi_draw_indirect_handle_t handle,
    MTLRenderCommandEncoder* encoder,
    MTLCommandBuffer* command_buffer);

/* Statistics */
uint32_t rendering_multi_draw_indirect_get_count(void);
size_t rendering_multi_draw_indirect_get_memory_usage(void);
multi_draw_stats_t rendering_multi_draw_indirect_get_stats(void);
void rendering_multi_draw_indirect_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_MULTI_DRAW_INDIRECT_H */
