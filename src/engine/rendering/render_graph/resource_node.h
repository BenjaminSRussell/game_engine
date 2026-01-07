/*
 * resource_node.h
 * Render graph resources
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_RESOURCE_NODE_H
#define RENDERING_RESOURCE_NODE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum rendering_resource_type {
    RENDERING_RESOURCE_TYPE_TEXTURE,
    RENDERING_RESOURCE_TYPE_BUFFER
} rendering_resource_type_t;

typedef struct rendering_resource_node_handle {
    uint32_t id;
} rendering_resource_node_handle_t;

typedef struct rendering_resource_node_desc {
    const char* name;
    rendering_resource_type_t type;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t format; // Cast to appropriate format enum (MTLPixelFormat for textures)
    uint32_t usage;  // MTLTextureUsage or buffer usage flags
    uint32_t flags;
    void* user_data;
    bool is_transient;  // If true, resource can be aliased
    size_t buffer_size; // For buffers, size in bytes
} rendering_resource_node_desc_t;

typedef struct rendering_resource_node_info {
    uint32_t id;
    const char* name;
    rendering_resource_type_t type;
    uint32_t flags;
    bool initialized;
    bool is_transient;
    
    // Lifetime tracking for aliasing
    uint32_t first_use_pass;
    uint32_t last_use_pass;
    
    // Memory size for aliasing
    size_t memory_size;
    size_t memory_offset;  // Offset in aliased heap
    
    // Metal resource handles (void* for C compatibility, cast to id<MTLTexture> or id<MTLBuffer>)
    void* metal_resource;
} rendering_resource_node_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_resource_node_init(void);
void rendering_resource_node_shutdown(void);

/* Lifecycle */
int rendering_resource_node_create(rendering_resource_node_handle_t* out_handle, const rendering_resource_node_desc_t* desc);
void rendering_resource_node_destroy(rendering_resource_node_handle_t handle);

/* Operations */
int rendering_resource_node_update(rendering_resource_node_handle_t handle, const void* data, size_t size);
bool rendering_resource_node_is_valid(rendering_resource_node_handle_t handle);
int rendering_resource_node_get_info(rendering_resource_node_handle_t handle, rendering_resource_node_info_t* out_info);
void rendering_resource_node_mark_dirty(rendering_resource_node_handle_t handle);
int rendering_resource_node_process_pending(void);

/* Statistics */
uint32_t rendering_resource_node_get_count(void);
size_t rendering_resource_node_get_memory_usage(void);
void rendering_resource_node_debug_print(void);

/* Resource Lifetime Management */
void rendering_resource_node_set_lifetime(rendering_resource_node_handle_t handle, uint32_t first_pass, uint32_t last_pass);
void* rendering_resource_node_get_metal_resource(rendering_resource_node_handle_t handle);
void rendering_resource_node_set_metal_resource(rendering_resource_node_handle_t handle, void* metal_resource);
void rendering_resource_node_set_memory_offset(rendering_resource_node_handle_t handle, size_t offset);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_RESOURCE_NODE_H */
