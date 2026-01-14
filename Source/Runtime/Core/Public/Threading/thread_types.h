/**
 * @file thread_types.h
 * @brief Threading subsystem type definitions
 */

#ifndef VOXELFORGE_THREAD_TYPES_H
#define VOXELFORGE_THREAD_TYPES_H

#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct VF_Thread VF_Thread;
typedef struct VF_Mutex VF_Mutex;
typedef struct VF_RWLock VF_RWLock;
typedef struct VF_Semaphore VF_Semaphore;
typedef struct VF_CondVar VF_CondVar;
typedef struct VF_ThreadPool VF_ThreadPool;

// ============================================================================
// Thread Function Type
// ============================================================================

typedef void (*VF_ThreadFn)(void *user_data);

// ============================================================================
// Thread Priority
// ============================================================================

typedef enum VF_ThreadPriority {
  VF_THREAD_PRIORITY_LOW = 0,
  VF_THREAD_PRIORITY_NORMAL = 1,
  VF_THREAD_PRIORITY_HIGH = 2,
  VF_THREAD_PRIORITY_CRITICAL = 3,
} VF_ThreadPriority;

// ============================================================================
// Thread State
// ============================================================================

typedef enum VF_ThreadState {
  VF_THREAD_STATE_CREATED = 0,
  VF_THREAD_STATE_RUNNING = 1,
  VF_THREAD_STATE_WAITING = 2,
  VF_THREAD_STATE_FINISHED = 3,
} VF_ThreadState;

// ============================================================================
// Job Types
// ============================================================================

typedef struct VF_Job VF_Job;
typedef void (*VF_JobFn)(void *data, u32 job_index, u32 thread_index);

typedef struct VF_JobDesc {
  VF_JobFn fn;
  void *data;
  u32 count;      // Number of jobs in batch
  u32 group_size; // Jobs per batch
} VF_JobDesc;

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_THREAD_TYPES_H
