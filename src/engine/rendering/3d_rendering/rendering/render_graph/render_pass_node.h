/*
 * render_pass_node.h
 * Render graph pass nodes
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_RENDER_PASS_NODE_H
#define RENDERING_RENDER_PASS_NODE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef uint32_t rg_resource_handle_t;
#define RG_INVALID_RESOURCE (rg_resource_handle_t)0xFFFFFFFF

typedef enum rendering_render_pass_type {
    RENDERING_PASS_TYPE_GRAPHICS,
    RENDERING_PASS_TYPE_COMPUTE,
    RENDERING_PASS_TYPE_ASYNC_COMPUTE,
    RENDERING_PASS_TYPE_TRANSFER
} rendering_render_pass_type_t;

typedef struct rendering_render_pass_node_handle {
    uint32_t id;
} rendering_render_pass_node_handle_t;

typedef void (*rendering_render_pass_execute_fn)(void* cmd, void* user_data);

typedef struct rendering_render_pass_node_desc {
    const char* name;
    rendering_render_pass_type_t type;
    
    rg_resource_handle_t color_outputs[8];
    uint32_t color_output_count;
    rg_resource_handle_t depth_output;
    bool resolve_depth;
    
    rg_resource_handle_t texture_inputs[16];
    uint32_t texture_input_count;
    
    rg_resource_handle_t storage_outputs[8];
    uint32_t storage_output_count;
    
    rendering_render_pass_execute_fn execute;
    void* user_data;
    uint32_t flags;
} rendering_render_pass_node_desc_t;

typedef struct rendering_render_pass_node_info {
    uint32_t id;
    const char* name;
    rendering_render_pass_type_t type;
    uint32_t flags;
    bool initialized;
} rendering_render_pass_node_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_render_pass_node_init(void);
void rendering_render_pass_node_shutdown(void);

/* Lifecycle */
int rendering_render_pass_node_create(rendering_render_pass_node_handle_t* out_handle, const rendering_render_pass_node_desc_t* desc);
void rendering_render_pass_node_destroy(rendering_render_pass_node_handle_t handle);

/* Operations */
int rendering_render_pass_node_update(rendering_render_pass_node_handle_t handle, const void* data, size_t size);
bool rendering_render_pass_node_is_valid(rendering_render_pass_node_handle_t handle);
int rendering_render_pass_node_get_info(rendering_render_pass_node_handle_t handle, rendering_render_pass_node_info_t* out_info);
void rendering_render_pass_node_mark_dirty(rendering_render_pass_node_handle_t handle);
int rendering_render_pass_node_process_pending(void);
void rendering_render_pass_node_execute(rendering_render_pass_node_handle_t handle, void* cmd);

/* Statistics */
uint32_t rendering_render_pass_node_get_count(void);
size_t rendering_render_pass_node_get_memory_usage(void);
void rendering_render_pass_node_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_RENDER_PASS_NODE_H */
