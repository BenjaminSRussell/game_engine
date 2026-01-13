#pragma once

#include "engine/include/common.h"
#include <core/types.h>
#include <core/threading/fiber_context.h>

/**
 * =================================================================================================
 *                                  FIBER SYSTEM
 * =================================================================================================
 *
 * Purpose: High-level fiber management system built on top of fiber contexts.
 * Provides fiber scheduling, pooling, and synchronization primitives.
 */

typedef struct FiberScheduler FiberScheduler;
typedef FiberFn FiberFunction;

/* ===== FIBER SCHEDULER API ===== */

/**
 * Initialize the fiber scheduler
 */
void fiber_system_init(u32 fiber_pool_size);

/**
 * Shutdown the fiber scheduler
 */
void fiber_system_shutdown(void);

/**
 * Get the fiber scheduler instance
 */
FiberScheduler* fiber_system_get_scheduler(void);

/**
 * Schedule a fiber function for execution
 */
FiberContext* fiber_system_spawn(FiberFunction func, void *arg);

/**
 * Execute pending fibers until completion or yield
 */
void fiber_system_process(void);

/**
 * Wait for a fiber to complete
 */
void fiber_system_wait(FiberContext *fiber);

/**
 * Check if all fibers have completed
 */
bool fiber_system_is_idle(void);

/**
 * Get number of active fibers
 */
u32 fiber_system_get_active_count(void);

