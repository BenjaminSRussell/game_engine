#ifndef ULTIMATE_ENGINE_JOB_SYSTEM_H
#define ULTIMATE_ENGINE_JOB_SYSTEM_H

#include "../../Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Job Priority Levels
typedef enum JobPriority {
  JOB_PRIORITY_LOW,
  JOB_PRIORITY_NORMAL,
  JOB_PRIORITY_HIGH
} JobPriority;

// Job Handle (opaque pointer)
typedef void *JobHandle;

// Job Function Signature
typedef void (*JobFunction)(void *user_data);

// Lifecycle
VF_API bool JobSystem_Init(void);
VF_API void JobSystem_Shutdown(void);

// Job Scheduling
VF_API JobHandle JobSystem_Schedule(JobFunction function, void *user_data,
                                    JobPriority priority);
VF_API void JobSystem_Wait(JobHandle handle);

// Job Dependencies (for future expansion)
VF_API void JobSystem_AddDependency(JobHandle job, JobHandle dependency);

#ifdef __cplusplus
}
#endif

#endif // ULTIMATE_ENGINE_JOB_SYSTEM_H
