// Camera Bookmarks API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/camera_bookmarks_api_bridge.h"
#include <stdlib.h>
#include <string.h>

// Mock implementation state
#define MAX_BOOKMARKS 64
typedef struct {
  char name[64];
  int32_t index;
  // float position[3];
  // float rotation[3];
} Bookmark;

static Bookmark g_bookmarks[MAX_BOOKMARKS];
static uint32_t g_count = 0;

void camera_bookmark_save(const char *name, int32_t index) {
  if (!name)
    return;

  // Check if updating existing
  for (uint32_t i = 0; i < g_count; i++) {
    if (strcmp(g_bookmarks[i].name, name) == 0) {
      g_bookmarks[i].index = index;
      // Capture current camera state here
      LOG_INFO("Updated camera bookmark: %s", name);
      return;
    }
  }

  if (g_count < MAX_BOOKMARKS) {
    strncpy(g_bookmarks[g_count].name, name, 63);
    g_bookmarks[g_count].name[63] = '\0';
    g_bookmarks[g_count].index = index;
    // Capture current camera state here
    g_count++;
    LOG_INFO("Saved camera bookmark: %s", name);
  }
}

void camera_bookmark_restore(const char *name) {
  if (!name)
    return;
  LOG_INFO("Restoring camera bookmark: %s", name);
  // Apply camera state
}

void camera_bookmark_restore_by_index(int32_t index) {
  for (uint32_t i = 0; i < g_count; i++) {
    if (g_bookmarks[i].index == index) {
      camera_bookmark_restore(g_bookmarks[i].name);
      return;
    }
  }
}

void camera_bookmark_delete(const char *name) {
  if (!name)
    return;

  for (uint32_t i = 0; i < g_count; i++) {
    if (strcmp(g_bookmarks[i].name, name) == 0) {
      // Remove by swapping with last
      g_bookmarks[i] = g_bookmarks[--g_count];
      LOG_INFO("Deleted camera bookmark: %s", name);
      return;
    }
  }
}

uint32_t camera_bookmark_get_count(void) { return g_count; }

uint32_t camera_bookmark_get_names(char (*names)[64], uint32_t max_count) {
  if (!names || max_count == 0)
    return 0;

  uint32_t count = g_count < max_count ? g_count : max_count;
  for (uint32_t i = 0; i < count; i++) {
    strncpy(names[i], g_bookmarks[i].name, 64);
  }
  return count;
}
