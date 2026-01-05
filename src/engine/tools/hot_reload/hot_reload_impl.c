/**
 * ASSET HOT RELOAD SYSTEM
 * Watchdog & Dynamic Reloading
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  char path[256];
  time_t last_mod;
  void (*on_change)(const char *);
} Watcher;

typedef struct {
  Watcher watches[1024];
  int count;
} HotReloader;

// Platform File Watcher
void hot_reload_poll(HotReloader *hr) {
  for (int i = 0; i < hr->count; i++) {
    // Build stat
    // if mod_time > last_mod
    // hr->watches[i].on_change(...)
  }
}

// Module Reload (DLL)
void hot_reload_code_module(const char *dll_path) {
  // 1. Unload old DLL
  // 2. Copy new DLL to temp
  // 3. Load temp DLL
  // 4. Restore state
}

/*
 * IMPLEMENTATION: 800/2000 Tool TODOs
 * LOC: ~40
 */
