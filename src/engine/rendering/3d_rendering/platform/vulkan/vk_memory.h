/*
 * vk_memory.h
 * Vulkan memory
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_VK_MEMORY_H
#define PLATFORM_VK_MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct platform_vk_memory_handle {
    uint32_t id;
} platform_vk_memory_handle_t;

typedef struct platform_vk_memory_desc {
    uint32_t flags;
    void* user_data;
} platform_vk_memory_desc_t;

typedef struct platform_vk_memory_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} platform_vk_memory_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int platform_vk_memory_init(void);
void platform_vk_memory_shutdown(void);

/* Lifecycle */
int platform_vk_memory_create(platform_vk_memory_handle_t* out_handle, const platform_vk_memory_desc_t* desc);
void platform_vk_memory_destroy(platform_vk_memory_handle_t handle);

/* Operations */
int platform_vk_memory_update(platform_vk_memory_handle_t handle, const void* data, size_t size);
bool platform_vk_memory_is_valid(platform_vk_memory_handle_t handle);
int platform_vk_memory_get_info(platform_vk_memory_handle_t handle, platform_vk_memory_info_t* out_info);
void platform_vk_memory_mark_dirty(platform_vk_memory_handle_t handle);
int platform_vk_memory_process_pending(void);

/* Statistics */
uint32_t platform_vk_memory_get_count(void);
size_t platform_vk_memory_get_memory_usage(void);
void platform_vk_memory_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_VK_MEMORY_H */
