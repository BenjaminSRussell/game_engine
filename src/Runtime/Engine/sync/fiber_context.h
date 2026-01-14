#pragma once

#include <common.h>
#include <core/types.h>
#include <stddef.h>

/**
 * =================================================================================================
 *                                  FIBER CONTEXT
 * =================================================================================================
 *
 * Purpose: Low-level fiber (lightweight coroutine) context management.
 * Provides fiber creation, context switching, and stack management.
 */

typedef struct FiberContext FiberContext;

/**
 * Fiber function signature
 */
typedef void (*FiberFunction)(void *arg);

/* ===== FIBER API ===== */

/**
 * Create a new fiber with specified stack size
 */
FiberContext* fiber_create(FiberFunction func, void *arg, size_t stack_size);

/**
 * Destroy a fiber and free resources
 */
void fiber_destroy(FiberContext *fiber);

/**
 * Switch execution to another fiber
 */
void fiber_switch(FiberContext *from, FiberContext *to);

/**
 * Yield control back to caller
 */
void fiber_yield(void);

/**
 * Resume a suspended fiber
 */
void fiber_resume(FiberContext *fiber);

/**
 * Check if fiber has completed execution
 */
bool fiber_is_complete(FiberContext *fiber);

/**
 * Get currently executing fiber
 */
FiberContext* fiber_current(void);

/**
 * Get fiber's user data pointer
 */
void* fiber_get_user_data(FiberContext *fiber);

/**
 * Set fiber's user data pointer
 */
void fiber_set_user_data(FiberContext *fiber, void *data);

#endif /* FIBER_CONTEXT_H */
