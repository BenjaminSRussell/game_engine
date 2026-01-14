// Thumbnail Generator API Bridge
// Exposes asset thumbnail generation to VoxelForgeStudio

#ifndef THUMBNAIL_GENERATOR_API_BRIDGE_H
#define THUMBNAIL_GENERATOR_API_BRIDGE_H

#include <common.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Thumbnail Generator API
// ============================================================================

/// Request thumbnail generation for an asset
/// @param asset_path Path to the asset
/// @param width Desired width
/// @param height Desired height
/// @return Request ID
uint64_t thumbnail_request(const char *asset_path, uint32_t width,
                           uint32_t height);

/// Check if a thumbnail is ready
bool thumbnail_is_ready(uint64_t request_id);

/// Get thumbnail data (RGBA)
/// @param request_id Request ID
/// @param buffer Output buffer (must be allocated by caller)
/// @param buffer_size Size of buffer
/// @return true if successful
bool thumbnail_get_data(uint64_t request_id, void *buffer,
                        uint32_t buffer_size);

/// Cancel a thumbnail request
void thumbnail_cancel(uint64_t request_id);

/// Clear thumbnail cache
void thumbnail_clear_cache(void);

/// Get total pending requests
uint32_t thumbnail_get_pending_count(void);

#ifdef __cplusplus
}
#endif

#endif // THUMBNAIL_GENERATOR_API_BRIDGE_H
