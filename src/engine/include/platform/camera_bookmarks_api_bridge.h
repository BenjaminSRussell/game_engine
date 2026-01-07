// Camera Bookmarks API Bridge
// Exposes camera bookmarking system to VoxelForgeStudio

#ifndef CAMERA_BOOKMARKS_API_BRIDGE_H
#define CAMERA_BOOKMARKS_API_BRIDGE_H

#include "../common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Camera Bookmarks API
// ============================================================================

/// Save the current camera state as a bookmark
/// @param name Name of the bookmark
/// @param index Optional index (hotkey slot 0-9), use -1 for no hotkey
void camera_bookmark_save(const char *name, int32_t index);

/// Restore a camera bookmark
/// @param name Name of the bookmark to restore
void camera_bookmark_restore(const char *name);

/// Restore a camera bookmark by index
/// @param index Index (hotkey slot)
void camera_bookmark_restore_by_index(int32_t index);

/// Delete a bookmark
/// @param name Name of bookmark
void camera_bookmark_delete(const char *name);

/// Get number of bookmarks
uint32_t camera_bookmark_get_count(void);

/// Get list of bookmark names
/// @param names Output buffer for names
/// @param max_count Maximum number of bookmarks
/// @return Number written
uint32_t camera_bookmark_get_names(char (*names)[64], uint32_t max_count);

#ifdef __cplusplus
}
#endif

#endif // CAMERA_BOOKMARKS_API_BRIDGE_H
