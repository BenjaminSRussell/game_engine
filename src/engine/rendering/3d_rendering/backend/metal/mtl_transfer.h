/*
 * mtl_transfer.h
 * Metal data transfer manager
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_TRANSFER_H
#define PLATFORM_MTL_TRANSFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "mtl_device.h"
#include "mtl_buffer.h"
#include "mtl_texture.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct metal_transfer_manager metal_transfer_manager_t;

/* Callback invoked when transfer completes */
typedef void (*metal_transfer_completion_t)(void* user_data);

typedef struct metal_transfer_stats {
    uint64_t total_transfers;
    uint64_t total_bytes_transferred;
    uint64_t active_transfers;
    size_t staging_buffer_size;
    size_t staging_buffer_used;
} metal_transfer_stats_t;

/* ============================================================================
 * API - Transfer Manager Lifecycle
 * ============================================================================ */

/* Create transfer manager with dedicated command queue */
metal_transfer_manager_t* metal_transfer_manager_create(metal_device_t* device);

/* Destroy transfer manager */
void metal_transfer_manager_destroy(metal_transfer_manager_t* mgr);

/* Get transfer statistics */
metal_transfer_stats_t metal_transfer_manager_get_stats(metal_transfer_manager_t* mgr);

/* ============================================================================
 * API - Buffer Transfers
 * ============================================================================ */

/* Upload to buffer using staging (for private buffers) */
void metal_transfer_upload_buffer_staged(metal_transfer_manager_t* mgr,
                                         metal_buffer_t* dst,
                                         const void* data,
                                         size_t size,
                                         size_t offset,
                                         metal_transfer_completion_t callback,
                                         void* user_data);

/* Copy buffer to buffer */
void metal_transfer_copy_buffer(metal_transfer_manager_t* mgr,
                               metal_buffer_t* src,
                               metal_buffer_t* dst,
                               size_t src_offset,
                               size_t dst_offset,
                               size_t size,
                               metal_transfer_completion_t callback,
                               void* user_data);

/* ============================================================================
 * API - Texture Transfers
 * ============================================================================ */

/* Upload to texture using staging */
void metal_transfer_upload_texture_staged(metal_transfer_manager_t* mgr,
                                          metal_texture_t* dst,
                                          const void* data,
                                          const metal_texture_region_t* region,
                                          metal_transfer_completion_t callback,
                                          void* user_data);

/* Blit texture to texture */
void metal_transfer_blit_texture(metal_transfer_manager_t* mgr,
                                 metal_texture_t* src,
                                 metal_texture_t* dst,
                                 const metal_texture_region_t* src_region,
                                 const metal_texture_region_t* dst_region,
                                 metal_transfer_completion_t callback,
                                 void* user_data);

/* Copy buffer to texture */
void metal_transfer_buffer_to_texture(metal_transfer_manager_t* mgr,
                                      metal_buffer_t* src,
                                      metal_texture_t* dst,
                                      size_t src_offset,
                                      const metal_texture_region_t* dst_region,
                                      size_t bytes_per_row,
                                      metal_transfer_completion_t callback,
                                      void* user_data);

/* Copy texture to buffer */
void metal_transfer_texture_to_buffer(metal_transfer_manager_t* mgr,
                                      metal_texture_t* src,
                                      metal_buffer_t* dst,
                                      const metal_texture_region_t* src_region,
                                      size_t dst_offset,
                                      size_t bytes_per_row,
                                      metal_transfer_completion_t callback,
                                      void* user_data);

/* ============================================================================
 * API - Batch Operations
 * ============================================================================ */

/* Flush all pending transfers (submit command buffer) */
void metal_transfer_flush(metal_transfer_manager_t* mgr);

/* Wait for all transfers to complete */
void metal_transfer_wait_idle(metal_transfer_manager_t* mgr);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_TRANSFER_H */
