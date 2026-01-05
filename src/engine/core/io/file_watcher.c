#include "core/io/file_watcher.h"
#include <stdlib.h>

void file_watcher_init() {}

void file_watcher_add_path(const char *path) {}

void file_watcher_remove_path(const char *path) {}

int file_watcher_check_changes(void *changed_files) {
    // Return number of changed files
    return 0;
}

void file_watcher_shutdown() {}
