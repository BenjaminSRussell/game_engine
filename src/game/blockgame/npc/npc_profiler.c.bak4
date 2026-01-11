// Basic profiler for NPC system (Milestone 5).
#include <npc/npc.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *name;
  u64 start_cycles;
  u64 total_cycles;
  u32 call_count;
} ProfileEntry;

static ProfileEntry g_profile_entries[32];
static u32 g_profile_count = 0;

#if defined(__x86_64__) || defined(_M_X64)
static inline u64 read_cycles(void) {
  u32 hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((u64)hi << 32) | lo;
}
#else
static inline u64 read_cycles(void) { return 0; }
#endif

void npc_profile_begin(const char *name) {
  for (u32 i = 0; i < g_profile_count; i++) {
    if (g_profile_entries[i].name == name) {
      g_profile_entries[i].start_cycles = read_cycles();
      return;
    }
  }
  if (g_profile_count < 32) {
    ProfileEntry *e = &g_profile_entries[g_profile_count++];
    e->name = name;
    e->start_cycles = read_cycles();
    e->total_cycles = 0;
    e->call_count = 0;
  }
}

void npc_profile_end(const char *name) {
  u64 end = read_cycles();
  for (u32 i = 0; i < g_profile_count; i++) {
    if (g_profile_entries[i].name == name) {
      g_profile_entries[i].total_cycles +=
          end - g_profile_entries[i].start_cycles;
      g_profile_entries[i].call_count++;
      break;
    }
  }
}

void npc_profile_dump(void) {
  for (u32 i = 0; i < g_profile_count; i++) {
    ProfileEntry *e = &g_profile_entries[i];
    if (e->call_count > 0) {
      LOG_INFO("[PROFILE] %s: %llu cycles avg (%u calls)", e->name,
               e->total_cycles / e->call_count, e->call_count);
    }
  }
}
