#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include "include/common.h"

typedef enum {
  FILE_EVENT_CREATED,
  FILE_EVENT_MODIFIED,
  FILE_EVENT_DELETED
} FileEventType;

typedef struct {
  char path[512];
  FileEventType event;
  u64 timestamp;
} FileEvent;

typedef void (*FileEventCallback)(const FileEvent *event, void *user_data);

typedef struct FileWatcher FileWatcher;

// Create a file watcher for a directory
FileWatcher *file_watcher_create(const char *watch_path);

// Destroy file watcher and free resources
void file_watcher_destroy(FileWatcher *watcher);

// Register callback for file events
void file_watcher_add_callback(FileWatcher *watcher, FileEventCallback callback,
                               void *user_data);

// Poll for file system events (call regularly from main loop)
void file_watcher_poll(FileWatcher *watcher);

// Start watching (for platforms that need explicit start)
bool file_watcher_start(FileWatcher *watcher);

// Stop watching
void file_watcher_stop(FileWatcher *watcher);

#endif // FILE_WATCHER_H
