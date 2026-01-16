#ifndef ULTIMATE_ENGINE_PROFILER_H
#define ULTIMATE_ENGINE_PROFILER_H

#include "../../Core/Public/core_types.h"
#include <stdbool.h>

typedef struct ProfilerSession ProfilerSession;
typedef struct CPUProfiler CPUProfiler;
typedef struct GPUProfiler GPUProfiler;
typedef struct MemoryProfiler MemoryProfiler;

typedef struct Profiler {
  ProfilerSession *session;
  CPUProfiler *cpu_profiler;
  GPUProfiler *gpu_profiler;
  MemoryProfiler *memory_profiler;
} Profiler;

// Lifecycle
bool Profiler_Init(void);
void Profiler_Shutdown(void);
void Profiler_BeginFrame(void);
void Profiler_EndFrame(void);

// Subsystem Stubs
ProfilerSession *ProfilerSession_Create(void);
void ProfilerSession_Destroy(ProfilerSession *s);
void ProfilerSession_BeginFrame(ProfilerSession *s);
void ProfilerSession_EndFrame(ProfilerSession *s);

CPUProfiler *CPUProfiler_Create(void);
void CPUProfiler_Destroy(CPUProfiler *p);
void CPUProfiler_BeginFrame(CPUProfiler *p);
void CPUProfiler_EndFrame(CPUProfiler *p);

GPUProfiler *GPUProfiler_Create(void);
void GPUProfiler_Destroy(GPUProfiler *p);
void GPUProfiler_BeginFrame(GPUProfiler *p);
void GPUProfiler_EndFrame(GPUProfiler *p);

MemoryProfiler *MemoryProfiler_Create(void);
void MemoryProfiler_Destroy(MemoryProfiler *p);

#endif // ULTIMATE_ENGINE_PROFILER_H
