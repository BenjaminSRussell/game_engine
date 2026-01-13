/*
 * vk_sync.h
 * Vulkan synchronization
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_VK_SYNC_H
#define PLATFORM_VK_SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct platform_vk_sync_handle {
    uint32_t id;
} platform_vk_sync_handle_t;

typedef struct platform_vk_sync_desc {
    uint32_t flags;
    void* user_data;
} platform_vk_sync_desc_t;

typedef struct platform_vk_sync_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} platform_vk_sync_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int platform_vk_sync_init(void);
void platform_vk_sync_shutdown(void);

/* Lifecycle */
int platform_vk_sync_create(platform_vk_sync_handle_t* out_handle, const platform_vk_sync_desc_t* desc);
void platform_vk_sync_destroy(platform_vk_sync_handle_t handle);

/* Operations */
int platform_vk_sync_update(platform_vk_sync_handle_t handle, const void* data, size_t size);
bool platform_vk_sync_is_valid(platform_vk_sync_handle_t handle);
int platform_vk_sync_get_info(platform_vk_sync_handle_t handle, platform_vk_sync_info_t* out_info);
void platform_vk_sync_mark_dirty(platform_vk_sync_handle_t handle);
int platform_vk_sync_process_pending(void);

/* Statistics */
uint32_t platform_vk_sync_get_count(void);
size_t platform_vk_sync_get_memory_usage(void);
void platform_vk_sync_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_VK_SYNC_H */
