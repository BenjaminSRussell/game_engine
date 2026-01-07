/*
 * multi_draw_indirect.c
 * Multi-draw indirect rendering with Metal ICB implementation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "multi_draw_indirect.h"
#include "../../backend/metal/mtl_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

typedef struct rendering_multi_draw_indirect_internal {
    uint32_t id;
    uint32_t flags;
    indirect_command_buffer_t* icb;
    metal_buffer_t* argument_buffer;
    uint32_t max_draw_count;
    uint32_t current_draw_count;
    bool initialized;
    bool dirty;
    bool uses_icb;
} rendering_multi_draw_indirect_internal_t;

typedef struct rendering_multi_draw_indirect_context {
    rendering_multi_draw_indirect_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
    
    // Global statistics
    uint32_t total_draw_calls;
    uint32_t total_instances;
    uint32_t total_batches;
} rendering_multi_draw_indirect_context_t;

static rendering_multi_draw_indirect_context_t g_multi_draw_indirect_ctx = {0};

/* ============================================================================
 * INDIRECT COMMAND BUFFER (Metal ICB)
 * ============================================================================ */

#ifdef __OBJC__

indirect_command_buffer_t* indirect_command_buffer_create(
    metal_device_t* device,
    const indirect_command_buffer_desc_t* desc) {
    
    if (!device || !desc || desc->max_command_count == 0) {
        return NULL;
    }
    
    @autoreleasepool {
        // Get Metal device
        id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
        if (!mtlDevice) {
            fprintf(stderr, "[ICB] Invalid Metal device\n");
            return NULL;
        }
        
        // Allocate ICB structure
        indirect_command_buffer_t* icb = (indirect_command_buffer_t*)calloc(
            1, sizeof(indirect_command_buffer_t));
        if (!icb) {
            return NULL;
        }
        
        // Create ICB descriptor
        MTLIndirectCommandBufferDescriptor* icbDesc = [MTLIndirectCommandBufferDescriptor new];
        icbDesc.commandTypes = MTLIndirectCommandTypeDraw;
        icbDesc.inheritBuffers = desc->inherit_buffers;
        icbDesc.inheritPipelineState = desc->inherit_pipeline_state;
        icbDesc.maxVertexBufferBindCount = 8;
        icbDesc.maxFragmentBufferBindCount = 8;
        
        // Create Metal ICB
        id<MTLIndirectCommandBuffer> metalICB = [mtlDevice 
            newIndirectCommandBufferWithDescriptor:icbDesc
            maxCommandCount:desc->max_command_count
            options:0];
        
        if (!metalICB) {
            fprintf(stderr, "[ICB] Failed to create Metal ICB\n");
            free(icb);
            return NULL;
        }
        
        // Store ICB (bridged retain)
        icb->icb = (__bridge_retained MTLIndirectCommandBuffer*)metalICB;
        icb->max_commands = desc->max_command_count;
        icb->command_count = 0;
        icb->supports_indexed = desc->supports_indexed;
        icb->supports_non_indexed = desc->supports_non_indexed;
        
        if (desc->label) {
            strncpy(icb->label, desc->label, sizeof(icb->label) - 1);
            metalICB.label = [NSString stringWithUTF8String:desc->label];
        }
        
        // Create argument buffer for indirect args
        metal_buffer_desc_t buffer_desc = {0};
        buffer_desc.size = desc->max_command_count * sizeof(indirect_draw_indexed_args_t);
        buffer_desc.storage_mode = METAL_STORAGE_SHARED;
        buffer_desc.usage = METAL_BUFFER_USAGE_STORAGE;
        buffer_desc.label = "ICB_ArgumentBuffer";
        
        icb->argument_buffer = metal_buffer_create(device, &buffer_desc);
        
        printf("[ICB] Created '%s': %u max commands\n", icb->label, icb->max_commands);
        
        return icb;
    }
}

void indirect_command_buffer_destroy(indirect_command_buffer_t* icb) {
    if (!icb) {
        return;
    }
    
    @autoreleasepool {
        if (icb->icb) {
            CFRelease(icb->icb);
            icb->icb = NULL;
        }
        
        if (icb->argument_buffer) {
            metal_buffer_destroy(icb->argument_buffer);
        }
        
        printf("[ICB] Destroyed '%s'\n", icb->label);
        free(icb);
    }
}

void indirect_command_buffer_reset(indirect_command_buffer_t* icb) {
    if (!icb) {
        return;
    }
    
    @autoreleasepool {
        id<MTLIndirectCommandBuffer> metalICB = (__bridge id<MTLIndirectCommandBuffer>)icb->icb;
        if (metalICB) {
            [metalICB resetWithRange:NSMakeRange(0, icb->max_commands)];
        }
        
        icb->command_count = 0;
        icb->total_draws = 0;
        icb->total_instances = 0;
    }
}

void indirect_command_buffer_set_count(indirect_command_buffer_t* icb, uint32_t count) {
    if (icb && count <= icb->max_commands) {
        icb->command_count = count;
    }
}

void indirect_command_buffer_execute(
    indirect_command_buffer_t* icb,
    MTLRenderCommandEncoder* encoder,
    MTLCommandBuffer* command_buffer) {
    
    if (!icb || !encoder || icb->command_count == 0) {
        return;
    }
    
    @autoreleasepool {
        id<MTLRenderCommandEncoder> mtlEncoder = (__bridge id<MTLRenderCommandEncoder>)encoder;
        id<MTLIndirectCommandBuffer> metalICB = (__bridge id<MTLIndirectCommandBuffer>)icb->icb;
        
        if (mtlEncoder && metalICB) {
            // Execute indirect command buffer
            [mtlEncoder executeCommandsInBuffer:metalICB
                                      withRange:NSMakeRange(0, icb->command_count)];
            
            printf("[ICB] Executed %u commands\n", icb->command_count);
        }
    }
}

MTLIndirectCommandBuffer* indirect_command_buffer_get_metal_icb(
    indirect_command_buffer_t* icb) {
    
    return icb ? icb->icb : NULL;
}

#else // !__OBJC__

// Stub implementations for non-Objective-C builds
indirect_command_buffer_t* indirect_command_buffer_create(
    metal_device_t* device,
    const indirect_command_buffer_desc_t* desc) {
    fprintf(stderr, "[ICB] Metal ICB requires Objective-C compilation\n");
    return NULL;
}

void indirect_command_buffer_destroy(indirect_command_buffer_t* icb) {}
void indirect_command_buffer_reset(indirect_command_buffer_t* icb) {}
void indirect_command_buffer_set_count(indirect_command_buffer_t* icb, uint32_t count) {}
void indirect_command_buffer_execute(
    indirect_command_buffer_t* icb,
    MTLRenderCommandEncoder* encoder,
    MTLCommandBuffer* command_buffer) {}
MTLIndirectCommandBuffer* indirect_command_buffer_get_metal_icb(
    indirect_command_buffer_t* icb) { return NULL; }

#endif // __OBJC__

/* ============================================================================
 * MULTI-DRAW INDIRECT
 * ============================================================================ */

int rendering_multi_draw_indirect_init(void) {
    if (g_multi_draw_indirect_ctx.initialized) {
        return 0;
    }

    g_multi_draw_indirect_ctx.capacity = 256;
    g_multi_draw_indirect_ctx.items = calloc(
        g_multi_draw_indirect_ctx.capacity, 
        sizeof(rendering_multi_draw_indirect_internal_t));
    
    if (!g_multi_draw_indirect_ctx.items) {
        return -1;
    }

    g_multi_draw_indirect_ctx.count = 0;
    g_multi_draw_indirect_ctx.initialized = true;

    printf("[Multi-Draw Indirect] System initialized\n");
    return 0;
}

void rendering_multi_draw_indirect_shutdown(void) {
    if (!g_multi_draw_indirect_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_multi_draw_indirect_ctx.count; i++) {
        rendering_multi_draw_indirect_internal_t* item = &g_multi_draw_indirect_ctx.items[i];
        if (item->icb) {
            indirect_command_buffer_destroy(item->icb);
        }
        if (item->argument_buffer) {
            metal_buffer_destroy(item->argument_buffer);
        }
    }

    free(g_multi_draw_indirect_ctx.items);
    
    printf("[Multi-Draw Indirect] System shutdown\n");
    memset(&g_multi_draw_indirect_ctx, 0, sizeof(rendering_multi_draw_indirect_context_t));
}

int rendering_multi_draw_indirect_create(
    rendering_multi_draw_indirect_handle_t* out_handle, 
    const rendering_multi_draw_indirect_desc_t* desc) {
    
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_multi_draw_indirect_ctx.initialized) {
        return -2;
    }

    if (g_multi_draw_indirect_ctx.count >= g_multi_draw_indirect_ctx.capacity) {
        uint32_t new_capacity = g_multi_draw_indirect_ctx.capacity * 2;
        rendering_multi_draw_indirect_internal_t* new_items = realloc(
            g_multi_draw_indirect_ctx.items, 
            new_capacity * sizeof(rendering_multi_draw_indirect_internal_t));
        
        if (!new_items) {
            return -3;
        }
        
        g_multi_draw_indirect_ctx.items = new_items;
        g_multi_draw_indirect_ctx.capacity = new_capacity;
    }

    uint32_t index = g_multi_draw_indirect_ctx.count++;
    rendering_multi_draw_indirect_internal_t* item = &g_multi_draw_indirect_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->max_draw_count = desc->max_draw_count;
    item->uses_icb = desc->use_indirect_command_buffer;
    item->initialized = true;
    item->dirty = true;
    item->icb = NULL;
    item->argument_buffer = NULL;

    out_handle->id = index;
    
    printf("[Multi-Draw Indirect] Created handle %u (max draws: %u, ICB: %s)\n",
           index, desc->max_draw_count, desc->use_indirect_command_buffer ? "yes" : "no");
    
    return 0;
}

void rendering_multi_draw_indirect_destroy(
    rendering_multi_draw_indirect_handle_t handle) {
    
    if (handle.id >= g_multi_draw_indirect_ctx.count) {
        return;
    }

    rendering_multi_draw_indirect_internal_t* item = &g_multi_draw_indirect_ctx.items[handle.id];
    
    if (item->icb) {
        indirect_command_buffer_destroy(item->icb);
        item->icb = NULL;
    }
    
    if (item->argument_buffer) {
        metal_buffer_destroy(item->argument_buffer);
        item->argument_buffer = NULL;
    }
    
    item->initialized = false;
}

int rendering_multi_draw_indirect_execute(
    rendering_multi_draw_indirect_handle_t handle,
    MTLRenderCommandEncoder* encoder,
    MTLCommandBuffer* command_buffer) {
    
    if (handle.id >= g_multi_draw_indirect_ctx.count) {
        return -1;
    }

    rendering_multi_draw_indirect_internal_t* item = &g_multi_draw_indirect_ctx.items[handle.id];
    
    if (!item->initialized) {
        return -2;
    }
    
    if (item->uses_icb && item->icb) {
        indirect_command_buffer_execute(item->icb, encoder, command_buffer);
        
        // Update statistics
        g_multi_draw_indirect_ctx.total_draw_calls += item->icb->command_count;
        g_multi_draw_indirect_ctx.total_instances += item->icb->total_instances;
        g_multi_draw_indirect_ctx.total_batches++;
        
        return 0;
    }
    
    return -3;
}

bool rendering_multi_draw_indirect_is_valid(
    rendering_multi_draw_indirect_handle_t handle) {
    
    if (handle.id >= g_multi_draw_indirect_ctx.count) {
        return false;
    }
    
    return g_multi_draw_indirect_ctx.items[handle.id].initialized;
}

int rendering_multi_draw_indirect_get_info(
    rendering_multi_draw_indirect_handle_t handle, 
    rendering_multi_draw_indirect_info_t* out_info) {
    
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_multi_draw_indirect_ctx.count) {
        return -2;
    }

    const rendering_multi_draw_indirect_internal_t* item = 
        &g_multi_draw_indirect_ctx.items[handle.id];
    
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->max_draw_count = item->max_draw_count;
    out_info->current_draw_count = item->icb ? item->icb->command_count : 0;
    out_info->initialized = item->initialized;
    out_info->uses_icb = item->uses_icb;

    return 0;
}

uint32_t rendering_multi_draw_indirect_get_count(void) {
    return g_multi_draw_indirect_ctx.count;
}

size_t rendering_multi_draw_indirect_get_memory_usage(void) {
    size_t total = sizeof(rendering_multi_draw_indirect_context_t);
    total += g_multi_draw_indirect_ctx.capacity * 
             sizeof(rendering_multi_draw_indirect_internal_t);
    return total;
}

multi_draw_stats_t rendering_multi_draw_indirect_get_stats(void) {
    multi_draw_stats_t stats = {0};
    
    stats.total_draw_calls = g_multi_draw_indirect_ctx.total_draw_calls;
    stats.total_instances = g_multi_draw_indirect_ctx.total_instances;
    stats.batches_executed = g_multi_draw_indirect_ctx.total_batches;
    
    // Calculate draw call reduction (compare to individual draws)
    if (stats.total_instances > 0) {
        float individual_draws = (float)stats.total_instances;
        float actual_draws = (float)stats.total_draw_calls;
        stats.draw_call_reduction = (1.0f - (actual_draws / individual_draws)) * 100.0f;
    }
    
    return stats;
}

void rendering_multi_draw_indirect_debug_print(void) {
    if (!g_multi_draw_indirect_ctx.initialized) {
        return;
    }
    
    printf("=== Multi-Draw Indirect Status ===\n");
    printf("  Count: %u / %u\n", 
           g_multi_draw_indirect_ctx.count, 
           g_multi_draw_indirect_ctx.capacity);
    printf("  Total Draw Calls: %u\n", g_multi_draw_indirect_ctx.total_draw_calls);
    printf("  Total Instances: %u\n", g_multi_draw_indirect_ctx.total_instances);
    printf("  Batches: %u\n", g_multi_draw_indirect_ctx.total_batches);
    
    multi_draw_stats_t stats = rendering_multi_draw_indirect_get_stats();
    printf("  Draw Call Reduction: %.1f%%\n", stats.draw_call_reduction);
}
