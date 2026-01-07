#ifndef RESOURCE_POOL_H
#define RESOURCE_POOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "resource_handle.h"
#include <stddef.h>

/**
 * @brief Generic resource pool container
 * Manages an array of data with versioned handles.
 */
typedef struct resource_pool_t resource_pool_t;

/**
 * @brief Configuration for resource pool
 */
typedef struct {
    uint32_t element_size;      // Size of each element in bytes
    uint32_t capacity;          // Max number of elements
    const char* name;           // Debug name
} resource_pool_desc_t;

/**
 * @brief Initialize a new resource pool
 */
resource_pool_t* resource_pool_create(const resource_pool_desc_t* desc);

/**
 * @brief Destroy a resource pool and free memory
 */
void resource_pool_destroy(resource_pool_t* pool);

/**
 * @brief Allocate a new slot in the pool
 * @return Handle to the allocated slot, or invalid handle if full
 */
resource_handle_t resource_pool_alloc(resource_pool_t* pool);

/**
 * @brief Free a slot in the pool
 * @return True if successful, false if handle was invalid
 */
bool resource_pool_free(resource_pool_t* pool, resource_handle_t handle);

/**
 * @brief Get pointer to data at handle
 * @return Pointer or NULL if handle is invalid/stale
 */
void* resource_pool_get(resource_pool_t* pool, resource_handle_t handle);

/**
 * @brief Get pointer to data at raw index (internal use)
 * @warning Does not check generation! Unsafe.
 */
void* resource_pool_get_raw(resource_pool_t* pool, uint32_t index);

/**
 * @brief Clear all resources (invalidate all handles)
 */
void resource_pool_clear(resource_pool_t* pool);

/**
 * @brief Get usage statistics
 */
uint32_t resource_pool_count(resource_pool_t* pool);
uint32_t resource_pool_capacity(resource_pool_t* pool);

#ifdef __cplusplus
}
#endif

#endif // RESOURCE_POOL_H
