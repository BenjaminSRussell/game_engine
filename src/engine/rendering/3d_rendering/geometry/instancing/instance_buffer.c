/*
 * instance_buffer.c
 * Instance Buffer Management
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "instance_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define INSTANCE_BUFFER_SIZE (16 * 1024 * 1024) // 16MB ring buffer

typedef struct instance_buffer_ctx {
    void* gpu_buffer;
    uint32_t offset;
    uint32_t capacity;
    bool initialized;
} instance_buffer_ctx_t;

static instance_buffer_ctx_t g_inst_buf = {0};

int instance_buffer_init(void) {
    if (g_inst_buf.initialized) return 0;
    
    g_inst_buf.capacity = INSTANCE_BUFFER_SIZE;
    g_inst_buf.offset = 0;
    // RenderAPI_CreateBuffer(&g_inst_buf.gpu_buffer, SIZE, DYNAMIC);
    
    g_inst_buf.initialized = true;
    return 0;
}

/*
 * Allocates space in the ring buffer for `count` instances of `stride` bytes.
 * Returns the offset in the buffer.
 */
uint32_t instance_buffer_allocate(uint32_t count, uint32_t stride, const void* data) {
    if (!g_inst_buf.initialized) return 0;

    uint32_t size = count * stride;
    
    // Check wrap around
    if (g_inst_buf.offset + size > g_inst_buf.capacity) {
        g_inst_buf.offset = 0; // Reset to start (orphaning logic might be needed)
    }
    
    uint32_t current_offset = g_inst_buf.offset;
    
    // Map and Copy
    // void* mapped = RenderAPI_MapBuffer(g_inst_buf.gpu_buffer, current_offset, size);
    // if (mapped && data) memcpy(mapped, data, size);
    // RenderAPI_UnmapBuffer(g_inst_buf.gpu_buffer);
    
    g_inst_buf.offset += size;
    // Align offset to 256 bytes (common GPU alignment requirement)
    g_inst_buf.offset = (g_inst_buf.offset + 255) & ~255;
    
    return current_offset;
}

void instance_buffer_shutdown(void) {
    if (g_inst_buf.initialized) {
        // cleanup
        g_inst_buf.initialized = false;
    }
}
