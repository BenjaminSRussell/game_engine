#include "assets/resources/resource_management/render_resource.h"
#include "assets/resources/resource_management/resource_pool.h"
#include "assets/resources/resource_management/resource_lifetime.h"
#include <stdlib.h>
#include <string.h>

/*
 * Internal structures to hold data in the pools
 */
typedef struct {
    size_t size;
    uint32_t usage;
    uint32_t memory_flags;
    void* api_handle;
    void* mapped_ptr;
} buffer_internal_t;

typedef struct {
    uint32_t width, height;
    ImageFormat format;
    uint32_t usage;
    void* api_handle;
} image_internal_t;

struct RenderResourceManager {
    resource_pool_t* buffer_pool;
    resource_pool_t* image_pool;
    
    // Stats tracking
    size_t total_allocated;
    uint32_t resource_count;
};

RenderResourceManager* render_resource_manager_create(uint32_t max_resources) {
    RenderResourceManager* mgr = (RenderResourceManager*)calloc(1, sizeof(RenderResourceManager));
    if (!mgr) return NULL;
    
    resource_pool_desc_t buf_desc = { sizeof(buffer_internal_t), max_resources, "BufferPool" };
    mgr->buffer_pool = resource_pool_create(&buf_desc);
    
    resource_pool_desc_t img_desc = { sizeof(image_internal_t), max_resources, "ImagePool" };
    mgr->image_pool = resource_pool_create(&img_desc);
    
    if (!mgr->buffer_pool || !mgr->image_pool) {
        render_resource_manager_destroy(mgr);
        return NULL;
    }
    
    return mgr;
}

void render_resource_manager_destroy(RenderResourceManager* manager) {
    if (!manager) return;
    
    if (manager->buffer_pool) resource_pool_destroy(manager->buffer_pool);
    if (manager->image_pool) resource_pool_destroy(manager->image_pool);
    
    free(manager);
}

buffer_handle_t render_buffer_create(RenderResourceManager* manager,
                                     size_t size,
                                     uint32_t usage,
                                     uint32_t memory_flags) {
    if (!manager) return buffer_handle_invalid();
    
    resource_handle_t raw_handle = resource_pool_alloc(manager->buffer_pool);
    if (resource_handle_is_null(raw_handle)) return buffer_handle_invalid();
    
    buffer_internal_t* data = (buffer_internal_t*)resource_pool_get(manager->buffer_pool, raw_handle);
    data->size = size;
    data->usage = usage;
    data->memory_flags = memory_flags;
    data->api_handle = NULL; // TODO: Allocate GPU buffer
    
    manager->total_allocated += size;
    manager->resource_count++;
    
    return (buffer_handle_t){ raw_handle };
}

static void deferred_buffer_destroy(void* ctx, resource_handle_t handle) {
    RenderResourceManager* mgr = (RenderResourceManager*)ctx;
    buffer_internal_t* data = (buffer_internal_t*)resource_pool_get(mgr->buffer_pool, handle);
    if (data) {
        // TODO: vkDestroyBuffer(data->api_handle)
        mgr->total_allocated -= data->size;
        mgr->resource_count--;
        resource_pool_free(mgr->buffer_pool, handle);
    }
}

void render_buffer_destroy(RenderResourceManager* manager, buffer_handle_t handle) {
    if (!manager || handle.id.handle == 0) return;
    
    resource_handle_t raw = handle.id;
    // Process destruction through the lifetime system to ensure safety
    resource_lifetime_defer_free(raw, deferred_buffer_destroy, manager);
}

void* render_buffer_map(RenderResourceManager* manager, buffer_handle_t handle) {
    if (!manager) return NULL;
    resource_handle_t raw = handle.id;
    buffer_internal_t* data = (buffer_internal_t*)resource_pool_get(manager->buffer_pool, raw);
    if (!data) return NULL;
    
    // TODO: Actually map memory
    return data->mapped_ptr;
}

void render_buffer_unmap(RenderResourceManager* manager, buffer_handle_t handle) {
    // TODO: Actually unmap memory
}

texture_handle_t render_image_create(RenderResourceManager* manager,
                                     uint32_t width,
                                     uint32_t height,
                                     ImageFormat format,
                                     uint32_t usage) {
    if (!manager) return texture_handle_invalid();
    
    resource_handle_t raw_handle = resource_pool_alloc(manager->image_pool);
    if (resource_handle_is_null(raw_handle)) return texture_handle_invalid();
    
    image_internal_t* data = (image_internal_t*)resource_pool_get(manager->image_pool, raw_handle);
    data->width = width;
    data->height = height;
    data->format = format;
    data->usage = usage;
    data->api_handle = NULL; // TODO: Allocate GPU image
    
    manager->resource_count++;
    
    return (texture_handle_t){ raw_handle };
}

static void deferred_image_destroy(void* ctx, resource_handle_t handle) {
    RenderResourceManager* mgr = (RenderResourceManager*)ctx;
    image_internal_t* data = (image_internal_t*)resource_pool_get(mgr->image_pool, handle);
    if (data) {
        // TODO: vkDestroyImage(data->api_handle)
        mgr->resource_count--;
        resource_pool_free(mgr->image_pool, handle);
    }
}

void render_image_destroy(RenderResourceManager* manager, texture_handle_t handle) {
    if (!manager || handle.id.handle == 0) return;
    
    resource_handle_t raw = handle.id;
    resource_lifetime_defer_free(raw, deferred_image_destroy, manager);
}

void render_resource_manager_get_stats(RenderResourceManager* manager,
                                       size_t* total_allocated,
                                       size_t* total_used,
                                       uint32_t* resource_count) {
    if (!manager) return;
    if (total_allocated) *total_allocated = manager->total_allocated;
    if (total_used) *total_used = manager->total_allocated; // Simplified
    if (resource_count) *resource_count = manager->resource_count;
}
