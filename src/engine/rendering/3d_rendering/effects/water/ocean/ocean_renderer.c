/*
 * ocean_renderer.c
 * Ocean surface rendering
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#include "ocean_renderer.h"
#include "../../../../include/math/math.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "../../backend/metal/metal/mtl_texture.h"
#include "../../backend/metal/metal/mtl_buffer.h"
#include "../../backend/metal/metal/mtl_pipeline.h"
#include "../../backend/metal/metal/mtl_device.h"
#include "../../backend/metal/metal/mtl_command.h"
#include "fft_waves.h"

extern metal_device_t* backend_get_metal_device(void);
extern void* backend_get_current_render_command_encoder(void);

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define WATER_OCEAN_RENDERER_MAX_COUNT 4096
#define WATER_OCEAN_RENDERER_DEFAULT_CAPACITY 256

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ocean_renderer_data {
    Vec3 ocean_color;
    float roughness;
    float metallic;
    float subsurface_scattering;
    float wave_height_scale;
    float chopness;
    bool enable_caustics;
    bool enable_foam;
    
    metal_render_pipeline_t* pipeline;
    metal_buffer_t* vertex_buffer;
    metal_buffer_t* index_buffer;
    uint32_t index_count;
    
    // Linked wave simulation
    water_fft_waves_handle_t wave_handle;
} ocean_renderer_data_t;

typedef struct water_ocean_renderer_internal {
    uint32_t id;
    uint32_t flags;
    ocean_renderer_data_t* data;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} water_ocean_renderer_internal_t;

typedef struct water_ocean_renderer_context {
    water_ocean_renderer_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} water_ocean_renderer_context_t;

static water_ocean_renderer_context_t g_ocean_renderer_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool water_ocean_renderer_validate(const water_ocean_renderer_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->data) return false;
    return true;
}

static void water_ocean_renderer_cleanup_internal(water_ocean_renderer_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int water_ocean_renderer_init(void) {
    if (g_ocean_renderer_ctx.initialized) {
        return 0;
    }

    g_ocean_renderer_ctx.capacity = WATER_OCEAN_RENDERER_DEFAULT_CAPACITY;
    g_ocean_renderer_ctx.items = calloc(g_ocean_renderer_ctx.capacity, sizeof(water_ocean_renderer_internal_t));
    if (!g_ocean_renderer_ctx.items) {
        return -1;
    }

    g_ocean_renderer_ctx.count = 0;
    g_ocean_renderer_ctx.initialized = true;

    return 0;
}

void water_ocean_renderer_shutdown(void) {
    if (!g_ocean_renderer_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_ocean_renderer_ctx.count; i++) {
        water_ocean_renderer_cleanup_internal(&g_ocean_renderer_ctx.items[i]);
    }

    free(g_ocean_renderer_ctx.items);
    g_ocean_renderer_ctx.items = NULL;
    g_ocean_renderer_ctx.count = 0;
    g_ocean_renderer_ctx.capacity = 0;
    g_ocean_renderer_ctx.initialized = false;
}

int water_ocean_renderer_create(water_ocean_renderer_handle_t* out_handle, const water_ocean_renderer_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_ocean_renderer_ctx.initialized) {
        return -2;
    }

    if (g_ocean_renderer_ctx.count >= g_ocean_renderer_ctx.capacity) {
        uint32_t new_capacity = g_ocean_renderer_ctx.capacity * 2;
        water_ocean_renderer_internal_t* new_items = realloc(g_ocean_renderer_ctx.items, new_capacity * sizeof(water_ocean_renderer_internal_t));
        if (!new_items) return -3;
        
        memset(new_items + g_ocean_renderer_ctx.capacity, 0, (new_capacity - g_ocean_renderer_ctx.capacity) * sizeof(water_ocean_renderer_internal_t));
        g_ocean_renderer_ctx.items = new_items;
        g_ocean_renderer_ctx.capacity = new_capacity;
    }

    uint32_t index = g_ocean_renderer_ctx.count++;
    water_ocean_renderer_internal_t* item = &g_ocean_renderer_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = calloc(1, sizeof(ocean_renderer_data_t));
    if (!item->data) {
        g_ocean_renderer_ctx.count--;
        return -4;
    }

    // Default parameters
    item->data->ocean_color = (Vec3){0.0f, 0.1f, 0.3f};
    item->data->roughness = 0.2f;
    item->data->metallic = 0.0f;
    item->data->subsurface_scattering = 0.5f;
    item->data->wave_height_scale = 1.0f;
    item->data->chopness = 1.0f;
    item->data->enable_caustics = true;
    item->data->enable_foam = true;
    
    // Initialize mesh (simple grid)
    metal_device_t* device = backend_get_metal_device();
    
    // Create simple plane grid for testing 100x100
    // Real implementation would use a better mesh generator or LOD system
    uint32_t grid_size = 256;
    uint32_t vertex_count = grid_size * grid_size;
    uint32_t index_count = (grid_size - 1) * (grid_size - 1) * 6;
    
    // Buffers... (Simplified for this task)
    // We will assume helper `geometry_create_plane_buffers` exists or just allocate.
    // Allocating dummy buffers for completeness of structure
    metal_buffer_desc_t v_desc = { .size = vertex_count * sizeof(float) * 5, .storage_mode = METAL_STORAGE_SHARED, .usage = METAL_BUFFER_USAGE_VERTEX }; // Pos3 + UI2
    item->data->vertex_buffer = metal_buffer_create(device, &v_desc);
    
    metal_buffer_desc_t i_desc = { .size = index_count * sizeof(uint32_t), .storage_mode = METAL_STORAGE_SHARED, .usage = METAL_BUFFER_USAGE_INDEX };
    item->data->index_buffer = metal_buffer_create(device, &i_desc);
    item->data->index_count = index_count;

    // Load pipeline
    metal_shader_library_t* lib = metal_load_shader_library(device->device, "src/engine/rendering/3d_rendering/effects/water/water/ocean/ocean.metal");
    if (lib) {
        metal_render_pipeline_desc_t pipe_desc = {
            .vertex_function = metal_get_function(lib, "ocean_vertex"),
            .fragment_function = metal_get_function(lib, "ocean_fragment"),
            .color_format = METAL_PIXEL_FORMAT_BGRA8_UNORM, // Default swapchain format
            .depth_format = METAL_PIXEL_FORMAT_DEPTH32_FLOAT,
            // .vertex_descriptor = ... (need to set up)
        };
        // Setup vertex descriptor
        metal_vertex_descriptor_init(&pipe_desc.vertex_descriptor);
        metal_vertex_descriptor_add_attribute(&pipe_desc.vertex_descriptor, 0, METAL_VERTEX_FORMAT_FLOAT3, 0, 0); // Pos
        metal_vertex_descriptor_add_attribute(&pipe_desc.vertex_descriptor, 1, METAL_VERTEX_FORMAT_FLOAT2, 12, 0); // UV
        metal_vertex_descriptor_set_layout(&pipe_desc.vertex_descriptor, 0, 20, 0); // Stride 20

        item->data->pipeline = metal_create_render_pipeline(device->device, &pipe_desc);
        metal_destroy_shader_library(lib);
    }

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void water_ocean_renderer_destroy(water_ocean_renderer_handle_t handle) {
    if (handle.id >= g_ocean_renderer_ctx.count) {
        return;
    }

    water_ocean_renderer_cleanup_internal(&g_ocean_renderer_ctx.items[handle.id]);
}

int water_ocean_renderer_update(water_ocean_renderer_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_ocean_renderer_ctx.count) {
        return -1;
    }

    water_ocean_renderer_internal_t* item = &g_ocean_renderer_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    if (data && size == sizeof(ocean_renderer_data_t)) {
        memcpy(item->data, data, size);
        item->dirty = false;
        item->frame_updated++;
    }

    return 0;
}

bool water_ocean_renderer_is_valid(water_ocean_renderer_handle_t handle) {
    if (handle.id >= g_ocean_renderer_ctx.count) {
        return false;
    }
    return g_ocean_renderer_ctx.items[handle.id].initialized;
}

int water_ocean_renderer_get_info(water_ocean_renderer_handle_t handle, water_ocean_renderer_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_ocean_renderer_ctx.count) {
        return -2;
    }

    const water_ocean_renderer_internal_t* item = &g_ocean_renderer_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void water_ocean_renderer_mark_dirty(water_ocean_renderer_handle_t handle) {
    if (handle.id < g_ocean_renderer_ctx.count) {
        g_ocean_renderer_ctx.items[handle.id].dirty = true;
    }
}

int water_ocean_renderer_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_ocean_renderer_ctx.count; i++) {
        water_ocean_renderer_internal_t* item = &g_ocean_renderer_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t water_ocean_renderer_get_count(void) {
    return g_ocean_renderer_ctx.count;
}

size_t water_ocean_renderer_get_memory_usage(void) {
    size_t total = sizeof(g_ocean_renderer_ctx);
    total += g_ocean_renderer_ctx.capacity * sizeof(water_ocean_renderer_internal_t);

    for (uint32_t i = 0; i < g_ocean_renderer_ctx.count; i++) {
        if (g_ocean_renderer_ctx.items[i].data) {
            total += sizeof(ocean_renderer_data_t);
        }
    }

    return total;
}

// Public render function
void water_ocean_renderer_draw(water_ocean_renderer_handle_t handle, water_fft_waves_handle_t wave_handle, void* camera_ptr) {
    if (handle.id >= g_ocean_renderer_ctx.count) return;
    water_ocean_renderer_internal_t* item = &g_ocean_renderer_ctx.items[handle.id];
    if (!item->initialized || !item->data || !item->data->pipeline) return;

    id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)backend_get_current_render_command_encoder();
    if (!encoder) return;

    [encoder setRenderPipelineState:item->data->pipeline->state];
    [encoder setVertexBuffer:(__bridge id<MTLBuffer>)item->data->vertex_buffer->buffer offset:0 atIndex:0];
    
    // Get wave resources
    void *disp_ptr = NULL, *deriv_ptr = NULL;
    water_fft_waves_get_resources(wave_handle, &disp_ptr, &deriv_ptr);
    
    if (disp_ptr) [encoder setVertexTexture:(__bridge id<MTLTexture>)disp_ptr atIndex:0];
    if (deriv_ptr) [encoder setFragmentTexture:(__bridge id<MTLTexture>)deriv_ptr atIndex:0];
    
    // Set uniforms (camera, params) - simplified
    // OceanParams structure must match shader buffer(2) in vertex
    // Camera params buffer(1)
    
    [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                        indexCount:item->data->index_count
                         indexType:MTLIndexTypeUInt32
                       indexBuffer:(__bridge id<MTLBuffer>)item->data->index_buffer->buffer
                 indexBufferOffset:0];
}

void water_ocean_renderer_debug_print(void) {
    // Debug printing implementation
}
