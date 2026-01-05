/*
 * hzb_test.h
 * HZB occlusion test
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_HZB_TEST_H
#define CULLING_HZB_TEST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_hzb_test_handle {
    uint32_t id;
} culling_hzb_test_handle_t;

typedef struct culling_hzb_test_desc {
    uint32_t flags;
    void* user_data;
} culling_hzb_test_desc_t;

typedef struct culling_hzb_test_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_hzb_test_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_hzb_test_init(void);
void culling_hzb_test_shutdown(void);

/* Lifecycle */
int culling_hzb_test_create(culling_hzb_test_handle_t* out_handle, const culling_hzb_test_desc_t* desc);
void culling_hzb_test_destroy(culling_hzb_test_handle_t handle);

/* Operations */
int culling_hzb_test_update(culling_hzb_test_handle_t handle, const void* data, size_t size);
bool culling_hzb_test_is_valid(culling_hzb_test_handle_t handle);
int culling_hzb_test_get_info(culling_hzb_test_handle_t handle, culling_hzb_test_info_t* out_info);
void culling_hzb_test_mark_dirty(culling_hzb_test_handle_t handle);
int culling_hzb_test_process_pending(void);

/* Statistics */
uint32_t culling_hzb_test_get_count(void);
size_t culling_hzb_test_get_memory_usage(void);
void culling_hzb_test_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_HZB_TEST_H */
