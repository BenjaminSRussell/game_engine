/*
 * asset_hot_reload.c
 * Hot reload support implementation
 *
 * Part of the Asset System subsystem
 * Advanced 3D Rendering Engine
 */

#include "asset_hot_reload.h"
#include "core/common/memory/allocator.h"
#include "engine/include/core/logger.h"
#include "platform/file_system.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#define MAX_WATCHED_FILES 1024
#define MAX_PENDING_RELOADS 64
#define RELOAD_DEBOUNCE_MS 100

typedef struct file_watch_entry {
    char file_path[512];
    uint64_t last_modified;
    uint32_t asset_id;
    bool active;
    void (*reload_callback)(uint32_t asset_id, const char* file_path);
} file_watch_entry_t;

typedef struct pending_reload {
    uint32_t asset_id;
    char file_path[512];
    double timestamp;
} pending_reload_t;

typedef struct asset_hot_reload_system {
    file_watch_entry_t watched_files[MAX_WATCHED_FILES];
    uint32_t watched_count;
    
    pending_reload_t pending_reloads[MAX_PENDING_RELOADS];
    uint32_t pending_count;
    
#ifdef _WIN32
    HANDLE directory_handles[MAX_WATCHED_FILES];
    HANDLE completion_port;
#else
    int inotify_fd;
    int watch_descriptors[MAX_WATCHED_FILES];
#endif
    
    bool initialized;
    uint64_t total_reloads;
    size_t memory_usage;
} asset_hot_reload_system_t;

static asset_hot_reload_system_t g_hot_reload = {0};

/* ============================================================================
 * PLATFORM-SPECIFIC FILE WATCHING
 * ============================================================================ */

#ifdef _WIN32
static bool setup_file_watch_win32(const char* file_path, uint32_t watch_index) {
    char directory_path[512];
    strncpy(directory_path, file_path, sizeof(directory_path));
    
    // Extract directory from file path
    char* last_slash = strrchr(directory_path, '/');
    if (!last_slash) last_slash = strrchr(directory_path, '\\');
    if (last_slash) *last_slash = '\0';
    else strcpy(directory_path, ".");
    
    HANDLE dir_handle = CreateFileA(
        directory_path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );
    
    if (dir_handle == INVALID_HANDLE_VALUE) {
        log_error("Failed to create directory handle for %s", directory_path);
        return false;
    }
    
    g_hot_reload.directory_handles[watch_index] = dir_handle;
    
    // Associate with completion port
    if (!CreateIoCompletionPort(dir_handle, g_hot_reload.completion_port, (ULONG_PTR)watch_index, 0)) {
        log_error("Failed to associate directory handle with completion port");
        CloseHandle(dir_handle);
        return false;
    }
    
    return true;
}
#else
static bool setup_file_watch_inotify(const char* file_path, uint32_t watch_index) {
    int wd = inotify_add_watch(g_hot_reload.inotify_fd, file_path, IN_MODIFY | IN_CLOSE_WRITE);
    if (wd == -1) {
        log_error("Failed to add inotify watch for %s: %s", file_path, strerror(errno));
        return false;
    }
    
    g_hot_reload.watch_descriptors[watch_index] = wd;
    return true;
}
#endif

/* ============================================================================
 * CORE IMPLEMENTATION
 * ============================================================================ */

int asset_system_asset_hot_reload_init(void) {
    if (g_hot_reload.initialized) {
        return 0;  // Already initialized
    }
    
    memset(&g_hot_reload, 0, sizeof(g_hot_reload));
    
#ifdef _WIN32
    g_hot_reload.completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (!g_hot_reload.completion_port) {
        log_error("Failed to create IO completion port for hot reload");
        return -1;
    }
#else
    g_hot_reload.inotify_fd = inotify_init1(IN_NONBLOCK);
    if (g_hot_reload.inotify_fd == -1) {
        log_error("Failed to initialize inotify for hot reload: %s", strerror(errno));
        return -1;
    }
#endif
    
    g_hot_reload.initialized = true;
    g_hot_reload.memory_usage = sizeof(g_hot_reload);
    
    log_info("Asset hot reload system initialized");
    return 0;
}

void asset_system_asset_hot_reload_shutdown(void) {
    if (!g_hot_reload.initialized) {
        return;
    }
    
#ifdef _WIN32
    // Close directory handles
    for (uint32_t i = 0; i < g_hot_reload.watched_count; i++) {
        if (g_hot_reload.directory_handles[i] != INVALID_HANDLE_VALUE) {
            CloseHandle(g_hot_reload.directory_handles[i]);
        }
    }
    
    if (g_hot_reload.completion_port) {
        CloseHandle(g_hot_reload.completion_port);
    }
#else
    // Remove inotify watches
    for (uint32_t i = 0; i < g_hot_reload.watched_count; i++) {
        if (g_hot_reload.watch_descriptors[i] != -1) {
            inotify_rm_watch(g_hot_reload.inotify_fd, g_hot_reload.watch_descriptors[i]);
        }
    }
    
    if (g_hot_reload.inotify_fd != -1) {
        close(g_hot_reload.inotify_fd);
    }
#endif
    
    memset(&g_hot_reload, 0, sizeof(g_hot_reload));
    log_info("Asset hot reload system shutdown");
}

int asset_system_asset_hot_reload_create(asset_system_asset_hot_reload_handle_t* out_handle, 
                                        const asset_system_asset_hot_reload_desc_t* desc) {
    if (!g_hot_reload.initialized || !out_handle || !desc) {
        return -1;
    }
    
    if (g_hot_reload.watched_count >= MAX_WATCHED_FILES) {
        log_error("Maximum number of watched files reached");
        return -2;
    }
    
    uint32_t watch_index = g_hot_reload.watched_count++;
    file_watch_entry_t* entry = &g_hot_reload.watched_files[watch_index];
    
    // Extract file path from user data (assuming it's a string)
    const char* file_path = (const char*)desc->user_data;
    if (!file_path) {
        log_error("File path not provided in hot reload descriptor");
        return -3;
    }
    
    strncpy(entry->file_path, file_path, sizeof(entry->file_path) - 1);
    entry->file_path[sizeof(entry->file_path) - 1] = '\0';
    
    // Get file modification time
    struct stat file_stat;
    if (stat(entry->file_path, &file_stat) == 0) {
        entry->last_modified = file_stat.st_mtime;
    } else {
        entry->last_modified = 0;
    }
    
    entry->asset_id = watch_index;  // Use watch index as asset ID for now
    entry->active = true;
    entry->reload_callback = NULL;  // Would be set based on asset type
    
    // Setup platform-specific file watching
#ifdef _WIN32
    if (!setup_file_watch_win32(entry->file_path, watch_index)) {
        g_hot_reload.watched_count--;
        return -4;
    }
#else
    if (!setup_file_watch_inotify(entry->file_path, watch_index)) {
        g_hot_reload.watched_count--;
        return -4;
    }
#endif
    
    out_handle->id = watch_index;
    g_hot_reload.memory_usage += sizeof(file_watch_entry_t);
    
    log_debug("Added hot reload watch for %s (ID: %u)", entry->file_path, watch_index);
    return 0;
}

void asset_system_asset_hot_reload_destroy(asset_system_asset_hot_reload_handle_t handle) {
    if (!g_hot_reload.initialized || handle.id >= MAX_WATCHED_FILES) {
        return;
    }
    
    file_watch_entry_t* entry = &g_hot_reload.watched_files[handle.id];
    if (!entry->active) {
        return;
    }
    
    entry->active = false;
    
#ifdef _WIN32
    if (g_hot_reload.directory_handles[handle.id] != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hot_reload.directory_handles[handle.id]);
        g_hot_reload.directory_handles[handle.id] = INVALID_HANDLE_VALUE;
    }
#else
    if (g_hot_reload.watch_descriptors[handle.id] != -1) {
        inotify_rm_watch(g_hot_reload.inotify_fd, g_hot_reload.watch_descriptors[handle.id]);
        g_hot_reload.watch_descriptors[handle.id] = -1;
    }
#endif
    
    g_hot_reload.memory_usage -= sizeof(file_watch_entry_t);
    log_debug("Removed hot reload watch for %s", entry->file_path);
}

int asset_system_asset_hot_reload_update(asset_system_asset_hot_reload_handle_t handle, 
                                         const void* data, size_t size) {
    if (!g_hot_reload.initialized || handle.id >= MAX_WATCHED_FILES) {
        return -1;
    }
    
    file_watch_entry_t* entry = &g_hot_reload.watched_files[handle.id];
    if (!entry->active) {
        return -2;
    }
    
    // Update file modification time
    struct stat file_stat;
    if (stat(entry->file_path, &file_stat) == 0) {
        if (file_stat.st_mtime > entry->last_modified) {
            entry->last_modified = file_stat.st_mtime;
            
            // Add to pending reloads
            if (g_hot_reload.pending_count < MAX_PENDING_RELOADS) {
                pending_reload_t* pending = &g_hot_reload.pending_reloads[g_hot_reload.pending_count++];
                pending->asset_id = entry->asset_id;
                strncpy(pending->file_path, entry->file_path, sizeof(pending->file_path) - 1);
                pending->file_path[sizeof(pending->file_path) - 1] = '\0';
                pending->timestamp = clock() / (double)CLOCKS_PER_SEC;
                
                log_debug("File change detected: %s", entry->file_path);
            }
        }
    }
    
    return 0;
}

bool asset_system_asset_hot_reload_is_valid(asset_system_asset_hot_reload_handle_t handle) {
    if (!g_hot_reload.initialized || handle.id >= MAX_WATCHED_FILES) {
        return false;
    }
    
    return g_hot_reload.watched_files[handle.id].active;
}

int asset_system_asset_hot_reload_get_info(asset_system_asset_hot_reload_handle_t handle, 
                                          asset_system_asset_hot_reload_info_t* out_info) {
    if (!g_hot_reload.initialized || handle.id >= MAX_WATCHED_FILES || !out_info) {
        return -1;
    }
    
    file_watch_entry_t* entry = &g_hot_reload.watched_files[handle.id];
    if (!entry->active) {
        return -2;
    }
    
    out_info->id = entry->asset_id;
    out_info->flags = 0;  // Could store various flags
    out_info->initialized = true;
    
    return 0;
}

void asset_system_asset_hot_reload_mark_dirty(asset_system_asset_hot_reload_handle_t handle) {
    if (!g_hot_reload.initialized || handle.id >= MAX_WATCHED_FILES) {
        return;
    }
    
    file_watch_entry_t* entry = &g_hot_reload.watched_files[handle.id];
    if (!entry->active) {
        return;
    }
    
    // Add to pending reloads immediately
    if (g_hot_reload.pending_count < MAX_PENDING_RELOADS) {
        pending_reload_t* pending = &g_hot_reload.pending_reloads[g_hot_reload.pending_count++];
        pending->asset_id = entry->asset_id;
        strncpy(pending->file_path, entry->file_path, sizeof(pending->file_path) - 1);
        pending->file_path[sizeof(pending->file_path) - 1] = '\0';
        pending->timestamp = clock() / (double)CLOCKS_PER_SEC;
        
        log_debug("Marked asset %u as dirty for reload", entry->asset_id);
    }
}

int asset_system_asset_hot_reload_process_pending(void) {
    if (!g_hot_reload.initialized) {
        return -1;
    }
    
    if (g_hot_reload.pending_count == 0) {
        return 0;  // No pending reloads
    }
    
    double current_time = clock() / (double)CLOCKS_PER_SEC;
    uint32_t processed = 0;
    
    // Process pending reloads with debouncing
    for (uint32_t i = 0; i < g_hot_reload.pending_count; ) {
        pending_reload_t* pending = &g_hot_reload.pending_reloads[i];
        
        // Check if enough time has passed for debouncing
        if (current_time - pending->timestamp >= (RELOAD_DEBOUNCE_MS / 1000.0)) {
            // Find corresponding watch entry
            for (uint32_t j = 0; j < g_hot_reload.watched_count; j++) {
                file_watch_entry_t* entry = &g_hot_reload.watched_files[j];
                if (entry->active && entry->asset_id == pending->asset_id) {
                    if (entry->reload_callback) {
                        entry->reload_callback(entry->asset_id, pending->file_path);
                        g_hot_reload.total_reloads++;
                        processed++;
                        log_debug("Processed reload for asset %u: %s", entry->asset_id, pending->file_path);
                    }
                    break;
                }
            }
            
            // Remove from pending list
            memmove(&g_hot_reload.pending_reloads[i], 
                   &g_hot_reload.pending_reloads[i + 1],
                   (g_hot_reload.pending_count - i - 1) * sizeof(pending_reload_t));
            g_hot_reload.pending_count--;
        } else {
            i++;  // Skip this one for now
        }
    }
    
    return processed;
}

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

uint32_t asset_system_asset_hot_reload_get_count(void) {
    if (!g_hot_reload.initialized) {
        return 0;
    }
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < g_hot_reload.watched_count; i++) {
        if (g_hot_reload.watched_files[i].active) {
            count++;
        }
    }
    
    return count;
}

size_t asset_system_asset_hot_reload_get_memory_usage(void) {
    if (!g_hot_reload.initialized) {
        return 0;
    }
    
    return g_hot_reload.memory_usage;
}

void asset_system_asset_hot_reload_debug_print(void) {
    if (!g_hot_reload.initialized) {
        log_info("Hot reload system not initialized");
        return;
    }
    
    log_info("=== Asset Hot Reload System Debug Info ===");
    log_info("Watched files: %u / %d", asset_system_asset_hot_reload_get_count(), MAX_WATCHED_FILES);
    log_info("Pending reloads: %u / %d", g_hot_reload.pending_count, MAX_PENDING_RELOADS);
    log_info("Total reloads processed: %llu", (unsigned long long)g_hot_reload.total_reloads);
    log_info("Memory usage: %zu bytes", g_hot_reload.memory_usage);
    
    log_info("Active watches:");
    for (uint32_t i = 0; i < g_hot_reload.watched_count; i++) {
        file_watch_entry_t* entry = &g_hot_reload.watched_files[i];
        if (entry->active) {
            log_info("  [%u] Asset %u: %s", i, entry->asset_id, entry->file_path);
        }
    }
    log_info("==========================================");
}
