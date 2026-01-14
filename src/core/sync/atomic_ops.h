#pragma once

#include <common.h>
#include <core/types.h>
#include <stdatomic.h>

/**
 * =================================================================================================
 *                              ATOMIC OPERATIONS
 * =================================================================================================
 *
 * Purpose: Wrapper and convenience functions for atomic operations across different types.
 * Provides a consistent interface for lock-free synchronization primitives.
 */

/* ===== ATOMIC INTEGER OPERATIONS ===== */

/**
 * Atomically increment a 32-bit integer
 */
i32 atomic_increment_i32(_Atomic(i32) *value);

/**
 * Atomically decrement a 32-bit integer
 */
i32 atomic_decrement_i32(_Atomic(i32) *value);

/**
 * Atomically add to a 32-bit integer
 */
i32 atomic_add_i32(_Atomic(i32) *value, i32 delta);

/**
 * Atomically load a 32-bit integer
 */
i32 atomic_load_i32(_Atomic(i32) *value);

/**
 * Atomically store a 32-bit integer
 */
void atomic_store_i32(_Atomic(i32) *value, i32 newval);

/**
 * Atomically exchange a 32-bit integer and return old value
 */
i32 atomic_exchange_i32(_Atomic(i32) *value, i32 newval);

/**
 * Atomically compare and swap a 32-bit integer
 */
bool atomic_compare_exchange_i32(_Atomic(i32) *value, i32 *expected, i32 newval);

/* ===== ATOMIC POINTER OPERATIONS ===== */

/**
 * Atomically load a pointer
 */
void* atomic_load_ptr(_Atomic(void*) *ptr);

/**
 * Atomically store a pointer
 */
void atomic_store_ptr(_Atomic(void*) *ptr, void *newval);

/**
 * Atomically exchange a pointer
 */
void* atomic_exchange_ptr(_Atomic(void*) *ptr, void *newval);

/**
 * Atomically compare and swap a pointer
 */
bool atomic_compare_exchange_ptr(_Atomic(void*) *ptr, void **expected, void *newval);

/* ===== MEMORY ORDERING ===== */

/**
 * Full memory barrier (fence)
 */
void atomic_fence_acq_rel(void);

/**
 * Release memory barrier
 */
void atomic_fence_release(void);

/**
 * Acquire memory barrier
 */
void atomic_fence_acquire(void);

