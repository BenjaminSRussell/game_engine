/**
 * @file thread_api.h
 * @brief Public threading API
 */

#ifndef VOXELFORGE_THREAD_API_H
#define VOXELFORGE_THREAD_API_H

#include "Core/Public/Threading/thread_types.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Thread
// ============================================================================

VF_API VF_Thread *vf_thread_create(VF_ThreadFn fn, void *user_data,
                                   const char *name);
VF_API void vf_thread_destroy(VF_Thread *thread);
VF_API void vf_thread_join(VF_Thread *thread);
VF_API void vf_thread_detach(VF_Thread *thread);
VF_API b8 vf_thread_is_running(VF_Thread *thread);
VF_API void vf_thread_set_priority(VF_Thread *thread,
                                   VF_ThreadPriority priority);
VF_API void vf_thread_set_affinity(VF_Thread *thread, u32 core_mask);

VF_API u32 vf_thread_get_id(void);
VF_API u32 vf_thread_hardware_concurrency(void);
VF_API void vf_thread_sleep(u32 milliseconds);
VF_API void vf_thread_yield(void);

// ============================================================================
// Mutex
// ============================================================================

VF_API VF_Mutex *vf_mutex_create(void);
VF_API void vf_mutex_destroy(VF_Mutex *mutex);
VF_API void vf_mutex_lock(VF_Mutex *mutex);
VF_API b8 vf_mutex_try_lock(VF_Mutex *mutex);
VF_API void vf_mutex_unlock(VF_Mutex *mutex);

// ============================================================================
// Read-Write Lock
// ============================================================================

VF_API VF_RWLock *vf_rwlock_create(void);
VF_API void vf_rwlock_destroy(VF_RWLock *rwlock);
VF_API void vf_rwlock_read_lock(VF_RWLock *rwlock);
VF_API void vf_rwlock_read_unlock(VF_RWLock *rwlock);
VF_API void vf_rwlock_write_lock(VF_RWLock *rwlock);
VF_API void vf_rwlock_write_unlock(VF_RWLock *rwlock);

// ============================================================================
// Semaphore
// ============================================================================

VF_API VF_Semaphore *vf_semaphore_create(u32 initial_count);
VF_API void vf_semaphore_destroy(VF_Semaphore *sem);
VF_API void vf_semaphore_wait(VF_Semaphore *sem);
VF_API b8 vf_semaphore_try_wait(VF_Semaphore *sem);
VF_API void vf_semaphore_signal(VF_Semaphore *sem);

// ============================================================================
// Condition Variable
// ============================================================================

VF_API VF_CondVar *vf_condvar_create(void);
VF_API void vf_condvar_destroy(VF_CondVar *cv);
VF_API void vf_condvar_wait(VF_CondVar *cv, VF_Mutex *mutex);
VF_API b8 vf_condvar_wait_timeout(VF_CondVar *cv, VF_Mutex *mutex, u32 ms);
VF_API void vf_condvar_signal(VF_CondVar *cv);
VF_API void vf_condvar_broadcast(VF_CondVar *cv);

// ============================================================================
// Atomics
// ============================================================================

VF_API i32 vf_atomic_load_i32(volatile i32 *ptr);
VF_API void vf_atomic_store_i32(volatile i32 *ptr, i32 value);
VF_API i32 vf_atomic_add_i32(volatile i32 *ptr, i32 value);
VF_API i32 vf_atomic_sub_i32(volatile i32 *ptr, i32 value);
VF_API i32 vf_atomic_exchange_i32(volatile i32 *ptr, i32 value);
VF_API b8 vf_atomic_compare_exchange_i32(volatile i32 *ptr, i32 *expected,
                                         i32 desired);

VF_API i64 vf_atomic_load_i64(volatile i64 *ptr);
VF_API void vf_atomic_store_i64(volatile i64 *ptr, i64 value);
VF_API i64 vf_atomic_add_i64(volatile i64 *ptr, i64 value);

VF_API void *vf_atomic_load_ptr(void *volatile *ptr);
VF_API void vf_atomic_store_ptr(void *volatile *ptr, void *value);
VF_API void *vf_atomic_exchange_ptr(void *volatile *ptr, void *value);

// ============================================================================
// Thread Pool
// ============================================================================

VF_API VF_ThreadPool *vf_threadpool_create(u32 thread_count);
VF_API void vf_threadpool_destroy(VF_ThreadPool *pool);
VF_API void vf_threadpool_submit(VF_ThreadPool *pool, VF_JobDesc *job);
VF_API void vf_threadpool_wait(VF_ThreadPool *pool);
VF_API u32 vf_threadpool_get_thread_count(VF_ThreadPool *pool);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_THREAD_API_H
