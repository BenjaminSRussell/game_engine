#include "Public/Profiler.h"
#include <stdlib.h>

struct ProfilerSession {
  int id;
};
struct CPUProfiler {
  int id;
};
struct GPUProfiler {
  int id;
};
struct MemoryProfiler {
  int id;
};

static Profiler g_profiler = {0};

ProfilerSession *ProfilerSession_Create(void) {
  return malloc(sizeof(ProfilerSession));
}
void ProfilerSession_Destroy(ProfilerSession *s) { free(s); }
void ProfilerSession_BeginFrame(ProfilerSession *s) { (void)s; }
void ProfilerSession_EndFrame(ProfilerSession *s) { (void)s; }

CPUProfiler *CPUProfiler_Create(void) { return malloc(sizeof(CPUProfiler)); }
void CPUProfiler_Destroy(CPUProfiler *p) { free(p); }
void CPUProfiler_BeginFrame(CPUProfiler *p) { (void)p; }
void CPUProfiler_EndFrame(CPUProfiler *p) { (void)p; }

GPUProfiler *GPUProfiler_Create(void) { return malloc(sizeof(GPUProfiler)); }
void GPUProfiler_Destroy(GPUProfiler *p) { free(p); }
void GPUProfiler_BeginFrame(GPUProfiler *p) { (void)p; }
void GPUProfiler_EndFrame(GPUProfiler *p) { (void)p; }

MemoryProfiler *MemoryProfiler_Create(void) {
  return malloc(sizeof(MemoryProfiler));
}
void MemoryProfiler_Destroy(MemoryProfiler *p) { free(p); }

bool Profiler_Init(void) {
  g_profiler.session = ProfilerSession_Create();
  g_profiler.cpu_profiler = CPUProfiler_Create();
  g_profiler.memory_profiler = MemoryProfiler_Create();
  g_profiler.gpu_profiler = GPUProfiler_Create();

  return true;
}

void Profiler_BeginFrame(void) {
  ProfilerSession_BeginFrame(g_profiler.session);
  CPUProfiler_BeginFrame(g_profiler.cpu_profiler);
  // GPUProfiler is optional check?
  GPUProfiler_BeginFrame(g_profiler.gpu_profiler);
}

void Profiler_EndFrame(void) {
  GPUProfiler_EndFrame(g_profiler.gpu_profiler);
  CPUProfiler_EndFrame(g_profiler.cpu_profiler);
  ProfilerSession_EndFrame(g_profiler.session);
}

void Profiler_Shutdown(void) {
  GPUProfiler_Destroy(g_profiler.gpu_profiler);
  MemoryProfiler_Destroy(g_profiler.memory_profiler);
  CPUProfiler_Destroy(g_profiler.cpu_profiler);
  ProfilerSession_Destroy(g_profiler.session);
}
