/*
 * mtl_transfer.c
 * Metal data transfer manager implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#import <Metal/Metal.h>
#include "mtl_transfer.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

#define MAX_PENDING_TRANSFERS 64
#define STAGING_BUFFER_SIZE (16 * 1024 * 1024)  // 16MB

typedef struct pending_transfer {
    metal_transfer_completion_t callback;
    void* user_data;
    bool active;
} pending_transfer_t;

struct metal_transfer_manager {
    metal_device_t* device;
    
    // Dedicated transfer queue
    id<MTLCommandQueue> transfer_queue;
    
    // Staging buffer pool
    id<MTLBuffer> staging_buffer;
    size_t staging_buffer_offset;
    
    // Pending transfers
    pending_transfer_t pending_transfers[MAX_PENDING_TRANSFERS];
    uint32_t pending_count;
    
    // Statistics
    metal_transfer_stats_t stats;
    
    // Current command buffer
    id<MTLCommandBuffer> current_cmd_buffer;
    id<MTLBlitCommandEncoder> current_blit_encoder;
};

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static inline size_t align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static void ensure_command_buffer(metal_transfer_manager_t* mgr) {
    if (!mgr->current_cmd_buffer) {
        mgr->current_cmd_buffer = [mgr->transfer_queue commandBuffer];
        [mgr->current_cmd_buffer setLabel:@"Transfer Command Buffer"];
    }
}

static void ensure_blit_encoder(metal_transfer_manager_t* mgr) {
    ensure_command_buffer(mgr);
    if (!mgr->current_blit_encoder) {
        mgr->current_blit_encoder = [mgr->current_cmd_buffer blitCommandEncoder];
        [mgr->current_blit_encoder setLabel:@"Transfer Blit Encoder"];
    }
}

static void* allocate_staging(metal_transfer_manager_t* mgr, size_t size) {
    if (!mgr->staging_buffer) {
        mgr->staging_buffer = [mgr->device->device newBufferWithLength:STAGING_BUFFER_SIZE
                                                               options:MTLResourceStorageModeShared];
        [mgr->staging_buffer setLabel:@"Transfer Staging Buffer"];
        mgr->staging_buffer_offset = 0;
    }
    
    // Align to 256 bytes
    size_t aligned_size = align_size(size, 256);
    
    // Check if we need to flush and reset
    if (mgr->staging_buffer_offset + aligned_size > STAGING_BUFFER_SIZE) {
        metal_transfer_flush(mgr);
        metal_transfer_wait_idle(mgr);
        mgr->staging_buffer_offset = 0;
    }
    
    void* ptr = (uint8_t*)[mgr->staging_buffer contents] + mgr->staging_buffer_offset;
    size_t offset = mgr->staging_buffer_offset;
    mgr->staging_buffer_offset += aligned_size;
    
    mgr->stats.staging_buffer_used = mgr->staging_buffer_offset;
    
    return ptr;
}

static size_t get_staging_offset(metal_transfer_manager_t* mgr, void* ptr) {
    return (uint8_t*)ptr - (uint8_t*)[mgr->staging_buffer contents];
}

/* ============================================================================
 * TRANSFER MANAGER LIFECYCLE
 * ============================================================================ */

metal_transfer_manager_t* metal_transfer_manager_create(metal_device_t* device) {
    if (!device) {
        return NULL;
    }
    
    metal_transfer_manager_t* mgr = (metal_transfer_manager_t*)calloc(1, sizeof(metal_transfer_manager_t));
    if (!mgr) {
        return NULL;
    }
    
    mgr->device = device;
    
    // Create dedicated transfer queue with lower priority
    mgr->transfer_queue = [device->device newCommandQueue];
    [mgr->transfer_queue setLabel:@"Transfer Queue"];
    
    mgr->stats.staging_buffer_size = STAGING_BUFFER_SIZE;
    
    return mgr;
}

void metal_transfer_manager_destroy(metal_transfer_manager_t* mgr) {
    if (!mgr) {
        return;
    }
    
    // Wait for all pending transfers
    metal_transfer_wait_idle(mgr);
    
    // Release resources
    if (mgr->current_blit_encoder) {
        [mgr->current_blit_encoder endEncoding];
        mgr->current_blit_encoder = nil;
    }
    
    if (mgr->current_cmd_buffer) {
        mgr->current_cmd_buffer = nil;
    }
    
    mgr->staging_buffer = nil;
    mgr->transfer_queue = nil;
    
    free(mgr);
}

metal_transfer_stats_t metal_transfer_manager_get_stats(metal_transfer_manager_t* mgr) {
    if (mgr) {
        return mgr->stats;
    }
    metal_transfer_stats_t empty = {0};
    return empty;
}

/* ============================================================================
 * BUFFER TRANSFERS
 * ============================================================================ */

void metal_transfer_upload_buffer_staged(metal_transfer_manager_t* mgr,
                                         metal_buffer_t* dst,
                                         const void* data,
                                         size_t size,
                                         size_t offset,
                                         metal_transfer_completion_t callback,
                                         void* user_data) {
    if (!mgr || !dst || !data || size == 0) {
        if (callback) callback(user_data);
        return;
    }
    
    // For shared buffers, direct copy
    if (dst->storage_mode == METAL_STORAGE_SHARED) {
        memcpy((uint8_t*)dst->mapped_ptr + offset, data, size);
        if (callback) callback(user_data);
        return;
    }
    
    // Allocate staging space
    void* staging_ptr = allocate_staging(mgr, size);
    memcpy(staging_ptr, data, size);
    size_t staging_offset = get_staging_offset(mgr, staging_ptr);
    
    // Encode blit
    ensure_blit_encoder(mgr);
    [mgr->current_blit_encoder copyFromBuffer:mgr->staging_buffer
                                 sourceOffset:staging_offset
                                     toBuffer:dst->buffer
                            destinationOffset:offset
                                         size:size];
    
    mgr->stats.total_transfers++;
    mgr->stats.total_bytes_transferred += size;
    mgr->stats.active_transfers++;
    
    // Schedule callback
    if (callback && mgr->current_cmd_buffer) {
        [mgr->current_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
            callback(user_data);
            mgr->stats.active_transfers--;
        }];
    }
}

void metal_transfer_copy_buffer(metal_transfer_manager_t* mgr,
                               metal_buffer_t* src,
                               metal_buffer_t* dst,
                               size_t src_offset,
                               size_t dst_offset,
                               size_t size,
                               metal_transfer_completion_t callback,
                               void* user_data) {
    if (!mgr || !src || !dst || size == 0) {
        if (callback) callback(user_data);
        return;
    }
    
    ensure_blit_encoder(mgr);
    [mgr->current_blit_encoder copyFromBuffer:src->buffer
                                 sourceOffset:src_offset
                                     toBuffer:dst->buffer
                            destinationOffset:dst_offset
                                         size:size];
    
    mgr->stats.total_transfers++;
    mgr->stats.total_bytes_transferred += size;
    mgr->stats.active_transfers++;
    
    if (callback && mgr->current_cmd_buffer) {
        [mgr->current_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
            callback(user_data);
            mgr->stats.active_transfers--;
        }];
    }
}

/* ============================================================================
 * TEXTURE TRANSFERS
 * ============================================================================ */

void metal_transfer_upload_texture_staged(metal_transfer_manager_t* mgr,
                                          metal_texture_t* dst,
                                          const void* data,
                                          const metal_texture_region_t* region,
                                          metal_transfer_completion_t callback,
                                          void* user_data) {
    if (!mgr || !dst || !data) {
        if (callback) callback(user_data);
        return;
    }
    
    // Calculate size
    uint32_t width = region ? region->width : dst->width;
    uint32_t height = region ? region->height : dst->height;
    uint32_t depth = region ? region->depth : (dst->type == METAL_TEXTURE_TYPE_3D ? dst->depth : 1);
    
    size_t bytes_per_pixel = metal_pixel_format_bytes_per_pixel(dst->format);
    size_t bytes_per_row = width * bytes_per_pixel;
    size_t bytes_per_image = bytes_per_row * height;
    size_t total_size = bytes_per_image * depth;
    
    // Copy to staging
    void* staging_ptr = allocate_staging(mgr, total_size);
    memcpy(staging_ptr, data, total_size);
    size_t staging_offset = get_staging_offset(mgr, staging_ptr);
    
    // Create Metal region
    MTLRegion mtl_region;
    if (region) {
        mtl_region = MTLRegionMake3D(region->x, region->y, region->z,
                                     region->width, region->height, region->depth);
    } else {
        mtl_region = MTLRegionMake3D(0, 0, 0, dst->width, dst->height, depth);
    }
    
    uint32_t mip_level = region ? region->mip_level : 0;
    uint32_t array_slice = region ? region->array_slice : 0;
    
    // Encode blit
    ensure_blit_encoder(mgr);
    [mgr->current_blit_encoder copyFromBuffer:mgr->staging_buffer
                                 sourceOffset:staging_offset
                            sourceBytesPerRow:bytes_per_row
                          sourceBytesPerImage:bytes_per_image
                                   sourceSize:mtl_region.size
                                    toTexture:dst->texture
                             destinationSlice:array_slice
                             destinationLevel:mip_level
                            destinationOrigin:mtl_region.origin];
    
    mgr->stats.total_transfers++;
    mgr->stats.total_bytes_transferred += total_size;
    mgr->stats.active_transfers++;
    
    if (callback && mgr->current_cmd_buffer) {
        [mgr->current_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
            callback(user_data);
            mgr->stats.active_transfers--;
        }];
    }
}

void metal_transfer_blit_texture(metal_transfer_manager_t* mgr,
                                 metal_texture_t* src,
                                 metal_texture_t* dst,
                                 const metal_texture_region_t* src_region,
                                 const metal_texture_region_t* dst_region,
                                 metal_transfer_completion_t callback,
                                 void* user_data) {
    if (!mgr || !src || !dst) {
        if (callback) callback(user_data);
        return;
    }
    
    // Create Metal regions
    MTLRegion mtl_src_region;
    if (src_region) {
        mtl_src_region = MTLRegionMake3D(src_region->x, src_region->y, src_region->z,
                                         src_region->width, src_region->height, src_region->depth);
    } else {
        mtl_src_region = MTLRegionMake3D(0, 0, 0, src->width, src->height, 1);
    }
    
    MTLOrigin dst_origin;
    if (dst_region) {
        dst_origin = MTLOriginMake(dst_region->x, dst_region->y, dst_region->z);
    } else {
        dst_origin = MTLOriginMake(0, 0, 0);
    }
    
    uint32_t src_mip = src_region ? src_region->mip_level : 0;
    uint32_t src_slice = src_region ? src_region->array_slice : 0;
    uint32_t dst_mip = dst_region ? dst_region->mip_level : 0;
    uint32_t dst_slice = dst_region ? dst_region->array_slice : 0;
    
    ensure_blit_encoder(mgr);
    [mgr->current_blit_encoder copyFromTexture:src->texture
                                   sourceSlice:src_slice
                                   sourceLevel:src_mip
                                  sourceOrigin:mtl_src_region.origin
                                    sourceSize:mtl_src_region.size
                                     toTexture:dst->texture
                              destinationSlice:dst_slice
                              destinationLevel:dst_mip
                             destinationOrigin:dst_origin];
    
    mgr->stats.total_transfers++;
    mgr->stats.active_transfers++;
    
    if (callback && mgr->current_cmd_buffer) {
        [mgr->current_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
            callback(user_data);
            mgr->stats.active_transfers--;
        }];
    }
}

void metal_transfer_buffer_to_texture(metal_transfer_manager_t* mgr,
                                      metal_buffer_t* src,
                                      metal_texture_t* dst,
                                      size_t src_offset,
                                      const metal_texture_region_t* dst_region,
                                      size_t bytes_per_row,
                                      metal_transfer_completion_t callback,
                                      void* user_data) {
    if (!mgr || !src || !dst) {
        if (callback) callback(user_data);
        return;
    }
    
    MTLRegion mtl_region;
    MTLOrigin dst_origin;
    uint32_t mip_level = 0;
    uint32_t array_slice = 0;
    
    if (dst_region) {
        mtl_region = MTLRegionMake3D(0, 0, 0, dst_region->width, dst_region->height, dst_region->depth);
        dst_origin = MTLOriginMake(dst_region->x, dst_region->y, dst_region->z);
        mip_level = dst_region->mip_level;
        array_slice = dst_region->array_slice;
    } else {
        mtl_region = MTLRegionMake3D(0, 0, 0, dst->width, dst->height, 1);
        dst_origin = MTLOriginMake(0, 0, 0);
    }
    
    size_t bytes_per_image = bytes_per_row * mtl_region.size.height;
    
    ensure_blit_encoder(mgr);
    [mgr->current_blit_encoder copyFromBuffer:src->buffer
                                 sourceOffset:src_offset
                            sourceBytesPerRow:bytes_per_row
                          sourceBytesPerImage:bytes_per_image
                                   sourceSize:mtl_region.size
                                    toTexture:dst->texture
                             destinationSlice:array_slice
                             destinationLevel:mip_level
                            destinationOrigin:dst_origin];
    
    mgr->stats.total_transfers++;
    mgr->stats.active_transfers++;
    
    if (callback && mgr->current_cmd_buffer) {
        [mgr->current_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
            callback(user_data);
            mgr->stats.active_transfers--;
        }];
    }
}

void metal_transfer_texture_to_buffer(metal_transfer_manager_t* mgr,
                                      metal_texture_t* src,
                                      metal_buffer_t* dst,
                                      const metal_texture_region_t* src_region,
                                      size_t dst_offset,
                                      size_t bytes_per_row,
                                      metal_transfer_completion_t callback,
                                      void* user_data) {
    if (!mgr || !src || !dst) {
        if (callback) callback(user_data);
        return;
    }
    
    MTLRegion mtl_region;
    MTLOrigin src_origin;
    uint32_t mip_level = 0;
    uint32_t array_slice = 0;
    
    if (src_region) {
        mtl_region = MTLRegionMake3D(0, 0, 0, src_region->width, src_region->height, src_region->depth);
        src_origin = MTLOriginMake(src_region->x, src_region->y, src_region->z);
        mip_level = src_region->mip_level;
        array_slice = src_region->array_slice;
    } else {
        mtl_region = MTLRegionMake3D(0, 0, 0, src->width, src->height, 1);
        src_origin = MTLOriginMake(0, 0, 0);
    }
    
    size_t bytes_per_image = bytes_per_row * mtl_region.size.height;
    
    ensure_blit_encoder(mgr);
    [mgr->current_blit_encoder copyFromTexture:src->texture
                                   sourceSlice:array_slice
                                   sourceLevel:mip_level
                                  sourceOrigin:src_origin
                                    sourceSize:mtl_region.size
                                      toBuffer:dst->buffer
                             destinationOffset:dst_offset
                        destinationBytesPerRow:bytes_per_row
                      destinationBytesPerImage:bytes_per_image];
    
    mgr->stats.total_transfers++;
    mgr->stats.active_transfers++;
    
    if (callback && mgr->current_cmd_buffer) {
        [mgr->current_cmd_buffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
            callback(user_data);
            mgr->stats.active_transfers--;
        }];
    }
}

/* ============================================================================
 * BATCH OPERATIONS
 * ============================================================================ */

void metal_transfer_flush(metal_transfer_manager_t* mgr) {
    if (!mgr) {
        return;
    }
    
    if (mgr->current_blit_encoder) {
        [mgr->current_blit_encoder endEncoding];
        mgr->current_blit_encoder = nil;
    }
    
    if (mgr->current_cmd_buffer) {
        [mgr->current_cmd_buffer commit];
        mgr->current_cmd_buffer = nil;
    }
}

void metal_transfer_wait_idle(metal_transfer_manager_t* mgr) {
    if (!mgr) {
        return;
    }
    
    // Flush pending
    metal_transfer_flush(mgr);
    
    // Create fence command buffer and wait
    id<MTLCommandBuffer> fence = [mgr->transfer_queue commandBuffer];
    [fence commit];
    [fence waitUntilCompleted];
}

/* End of mtl_transfer.c */
