#include <core/file_watcher.h>
#include <core/logger.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

typedef struct FileEntry {
  char path[512];
  u64 last_modified;
  struct FileEntry *next;
} FileEntry;

struct FileWatcher {
  char watch_path[512];
  FileEntry *entries;
  FileEventCallback callback;
  void *user_data;
  bool is_watching;
};

FileWatcher *file_watcher_create(const char *watch_path) {
  FileWatcher *watcher = (FileWatcher *)malloc(sizeof(FileWatcher));
  if (!watcher)
    return NULL;

  strncpy(watcher->watch_path, watch_path, 511);
  watcher->entries = NULL;
  watcher->callback = NULL;
  watcher->user_data = NULL;
  watcher->is_watching = false;

  LOG_DEBUG("File watcher created for path: %s", watch_path);
  return watcher;
}

static void free_entries(FileEntry *entry) {
  while (entry) {
    FileEntry *next = entry->next;
    free(entry);
    entry = next;
  }
}

void file_watcher_destroy(FileWatcher *watcher) {
  if (!watcher)
    return;
  free_entries(watcher->entries);
  free(watcher);
}

void file_watcher_add_callback(FileWatcher *watcher, FileEventCallback callback,
                               void *user_data) {
  if (!watcher)
    return;
  watcher->callback = callback;
  watcher->user_data = user_data;
}

static u64 get_file_mod_time(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    return (u64)st.st_mtime;
  }
  return 0;
}

static void scan_directory(FileWatcher *watcher, const char *dir_path,
                           bool initial) {
  DIR *dir = opendir(dir_path);
  if (!dir)
    return;

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char full_path[512];
    snprintf(full_path, 512, "%s/%s", dir_path, entry->d_name);

    struct stat st;
    if (stat(full_path, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        scan_directory(watcher, full_path, initial);
      } else if (S_ISREG(st.st_mode)) {
        u64 mod_time = (u64)st.st_mtime;

        // Find entry
        FileEntry *fe = watcher->entries;
        FileEntry *prev = NULL;
        bool found = false;
        while (fe) {
          if (strcmp(fe->path, full_path) == 0) {
            found = true;
            if (!initial && mod_time > fe->last_modified) {
              fe->last_modified = mod_time;
              if (watcher->callback) {
                FileEvent ev;
                strncpy(ev.path, full_path, 511);
                ev.event = FILE_EVENT_MODIFIED;
                ev.timestamp = mod_time;
                watcher->callback(&ev, watcher->user_data);
              }
            }
            break;
          }
          prev = fe;
          fe = fe->next;
        }

        if (!found) {
          FileEntry *new_entry = (FileEntry *)malloc(sizeof(FileEntry));
          strncpy(new_entry->path, full_path, 511);
          new_entry->last_modified = mod_time;
          new_entry->next = watcher->entries;
          watcher->entries = new_entry;

          if (!initial && watcher->callback) {
            FileEvent ev;
            strncpy(ev.path, full_path, 511);
            ev.event = FILE_EVENT_CREATED;
            ev.timestamp = mod_time;
            watcher->callback(&ev, watcher->user_data);
          }
        }
      }
    }
  }
  closedir(dir);
}

bool file_watcher_start(FileWatcher *watcher) {
  if (!watcher)
    return false;
  // Initial scan to populate entries
  scan_directory(watcher, watcher->watch_path, true);
  watcher->is_watching = true;
  LOG_INFO("File watcher started for: %s", watcher->watch_path);
  return true;
}

void file_watcher_stop(FileWatcher *watcher) {
  if (watcher)
    watcher->is_watching = false;
}

void file_watcher_poll(FileWatcher *watcher) {
  if (!watcher || !watcher->is_watching)
    return;

  // In a real high-performance engine, we'd poll every X frames or use OS
  // events. For now, this simple polling scan works for dev-time hot-reloading.
  scan_directory(watcher, watcher->watch_path, false);
}
