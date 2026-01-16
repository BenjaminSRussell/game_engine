#include "../../Memory/Public/Memory.h"
#include "../Public/ECS.h"
#include <string.h>

#define MAX_SYSTEMS 128

typedef void (*SystemFunction)(float dt);

typedef struct SystemEntry {
  char name[64];
  SystemFunction update_func;
  SystemFunction render_func;
  bool active;
  int order;
} SystemEntry;

typedef struct SystemManager {
  SystemEntry systems[MAX_SYSTEMS];
  u32 system_count;
} SystemManager;

static SystemManager g_sys_mgr = {0};

void SystemManager_Init(void) {
  Memory_Zero(&g_sys_mgr, sizeof(SystemManager));
}

void SystemManager_Register(const char *name, SystemFunction update,
                            SystemFunction render, int order) {
  if (g_sys_mgr.system_count >= MAX_SYSTEMS)
    return;

  SystemEntry *entry = &g_sys_mgr.systems[g_sys_mgr.system_count++];
  strncpy(entry->name, name, 63);
  entry->update_func = update;
  entry->render_func = render;
  entry->active = true;
  entry->order = order;

  // Simple sort by order (bubble sort for simplicity on registration)
  // Full impl would use qsort or stable sort
  for (u32 i = 0; i < g_sys_mgr.system_count - 1; i++) {
    for (u32 j = 0; j < g_sys_mgr.system_count - i - 1; j++) {
      if (g_sys_mgr.systems[j].order > g_sys_mgr.systems[j + 1].order) {
        SystemEntry temp = g_sys_mgr.systems[j];
        g_sys_mgr.systems[j] = g_sys_mgr.systems[j + 1];
        g_sys_mgr.systems[j + 1] = temp;
      }
    }
  }
}

void SystemManager_Execute(float dt) {
  for (u32 i = 0; i < g_sys_mgr.system_count; i++) {
    if (g_sys_mgr.systems[i].active && g_sys_mgr.systems[i].update_func) {
      g_sys_mgr.systems[i].update_func(dt);
    }
  }
}

void SystemManager_ExecuteRender(void) {
  for (u32 i = 0; i < g_sys_mgr.system_count; i++) {
    if (g_sys_mgr.systems[i].active && g_sys_mgr.systems[i].render_func) {
      g_sys_mgr.systems[i].render_func(0.0f); // Render doesn't need DT usually
    }
  }
}
