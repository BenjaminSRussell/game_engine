#ifndef ULTIMATE_ENGINE_PLATFORM_H
#define ULTIMATE_ENGINE_PLATFORM_H

#include <stdbool.h>

// Platform Data
typedef struct PlatformData {
  void *handle;
  int width;
  int height;
  const char *app_name;
} PlatformData;

// Lifecycle
bool Platform_Init(void);
void Platform_Shutdown(void);
void Platform_Update(float delta_time);
bool Platform_RequestedExit(void);

// Internal Backend Functions
bool PlatformWindows_Init(void);
void PlatformWindows_Update(float dt);
void PlatformWindows_Shutdown(void);

bool PlatformMacos_Init(void);
void PlatformMacos_Update(float dt);
void PlatformMacos_Shutdown(void);

bool PlatformLinux_Init(void);
void PlatformLinux_Update(float dt);
void PlatformLinux_Shutdown(void);

#endif // ULTIMATE_ENGINE_PLATFORM_H
