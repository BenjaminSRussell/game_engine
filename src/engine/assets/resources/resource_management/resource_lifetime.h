#ifndef RESOURCE_LIFETIME_H
#define RESOURCE_LIFETIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "assets/resources/resource_management/resource_handle.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Function pointer type for destroying a specific resource type
 */
typedef void (*resource_destroy_fn)(void* ctx, resource_handle_t handle);

/**
 * @brief Initialize the resource lifetime system
 * @param frame_latency Number of frames to wait before deleting (usually 2 or 3)
 */
void resource_lifetime_init(uint32_t frame_latency);

/**
 * @brief Shutdown and force delete all pending resources
 */
void resource_lifetime_shutdown(void);

/**
 * @brief Advance the frame counter and process deletion queue
 * Should be called once per frame, usually at start or end
 */
void resource_lifetime_next_frame(void);

/**
 * @brief Queue a resource for deferred deletion
 * @param handle Handle to the resource to delete
 * @param destroy_fn Function to call to actually free the resource
 * @param ctx Context pointer to pass to destroy_fn (e.g., the pool or manager)
 */
void resource_lifetime_defer_free(resource_handle_t handle, resource_destroy_fn destroy_fn, void* ctx);

#ifdef __cplusplus
}
#endif

#endif // RESOURCE_LIFETIME_H
