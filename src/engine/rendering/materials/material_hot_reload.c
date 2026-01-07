// Material Hot-Reload - Implementation
#include "rendering/materials/material_hot_reload.h"
#include "rendering/materials/material_serialization.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

// Simple timestamp-based file watching (cross-platform)
typedef struct FileTimestamp {
    char filepath[256];
    time_t last_modified;
} FileTimestamp;

typedef struct SimpleFileWatcher {
    FileTimestamp *files;
    u32 file_count;
    u32 file_capacity;
    char watch_dir[256];
} SimpleFileWatcher;

MaterialHotReloader *material_hot_reload_create(void) {
    MaterialHotReloader *reloader = (MaterialHotReloader *)calloc(1, sizeof(MaterialHotReloader));
    if (!reloader) {
        LOG_ERROR("Failed to allocate material hot-reloader");
        return NULL;
    }
    
    reloader->master_count = 0;
    reloader->instance_count = 0;
    reloader->is_watching = false;
    
    return reloader;
}

void material_hot_reload_destroy(MaterialHotReloader *reloader) {
    if (!reloader) return;
    
    if (reloader->is_watching) {
        material_hot_reload_stop(reloader);
    }
    
    free(reloader->masters);
    free(reloader->instances);
    free(reloader);
}

static time_t get_file_mtime(const char *filepath) {
    struct stat st;
    if (stat(filepath, &st) == 0) {
        return st.st_mtime;
    }
    return 0;
}

static void scan_directory_for_mat_files(SimpleFileWatcher *watcher) {
    DIR *dir = opendir(watcher->watch_dir);
    if (!dir) {
        LOG_ERROR("Failed to open materials directory: %s", watcher->watch_dir);
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Check for .mat extension
        size_t len = strlen(entry->d_name);
        if (len > 4 && strcmp(entry->d_name + len - 4, ".mat") == 0) {
            char filepath[256];
            snprintf(filepath, sizeof(filepath), "%s/%s", watcher->watch_dir, entry->d_name);
            
            // Check if already tracked
            bool found = false;
            for (u32 i = 0; i < watcher->file_count; i++) {
                if (strcmp(watcher->files[i].filepath, filepath) == 0) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                // Add new file
                if (watcher->file_count >= watcher->file_capacity) {
                    watcher->file_capacity = watcher->file_capacity ? watcher->file_capacity * 2 : 16;
                    watcher->files = (FileTimestamp *)realloc(watcher->files,
                                                               watcher->file_capacity * sizeof(FileTimestamp));
                }
                
                FileTimestamp *ft = &watcher->files[watcher->file_count++];
                strncpy(ft->filepath, filepath, sizeof(ft->filepath) - 1);
                ft->last_modified = get_file_mtime(filepath);
                
                LOG_DEBUG("Watching material file: %s", filepath);
            }
        }
    }
    
    closedir(dir);
}

void material_hot_reload_start(MaterialHotReloader *reloader, const char *materials_dir) {
    if (!reloader || !materials_dir) return;
    
    SimpleFileWatcher *watcher = (SimpleFileWatcher *)calloc(1, sizeof(SimpleFileWatcher));
    strncpy(watcher->watch_dir, materials_dir, sizeof(watcher->watch_dir) - 1);
    watcher->file_capacity = 0;
    watcher->files = NULL;
    watcher->file_count = 0;
    
    // Initial scan
    scan_directory_for_mat_files(watcher);
    
    reloader->platform_watcher = watcher;
    reloader->is_watching = true;
    
    LOG_INFO("Material hot-reload started, watching: %s", materials_dir);
}

void material_hot_reload_stop(MaterialHotReloader *reloader) {
    if (!reloader || !reloader->is_watching) return;
    
    if (reloader->platform_watcher) {
        SimpleFileWatcher *watcher = (SimpleFileWatcher *)reloader->platform_watcher;
        free(watcher->files);
        free(watcher);
        reloader->platform_watcher = NULL;
    }
    
    reloader->is_watching = false;
    LOG_INFO("Material hot-reload stopped");
}

void material_hot_reload_update(MaterialHotReloader *reloader) {
    if (!reloader || !reloader->is_watching) return;
    
    SimpleFileWatcher *watcher = (SimpleFileWatcher *)reloader->platform_watcher;
    if (!watcher) return;
    
    // Rescan for new files periodically
    static u32 scan_counter = 0;
    if (++scan_counter > 300) {  // Every ~5 seconds at 60fps
        scan_directory_for_mat_files(watcher);
        scan_counter = 0;
    }
    
    // Check for modified files
    for (u32 i = 0; i < watcher->file_count; i++) {
        FileTimestamp *ft = &watcher->files[i];
        time_t current_mtime = get_file_mtime(ft->filepath);
        
        if (current_mtime > ft->last_modified) {
            LOG_INFO("Material file changed: %s", ft->filepath);
            
            // Find matching material instance to reload
            for (u32 j = 0; j < reloader->instance_count; j++) {
                MaterialInstance *instance = reloader->instances[j];
                
                // Check if this instance's file matches
                char expected_path[256];
                snprintf(expected_path, sizeof(expected_path), "%s/%s.mat",
                         watcher->watch_dir, instance->name);
                
                if (strcmp(ft->filepath, expected_path) == 0) {
                    LOG_INFO("Reloading material: %s", instance->name);
                    
                    // Reload from file
                    MaterialInstance *reloaded = material_load_from_file(ft->filepath, instance->parent);
                    if (reloaded) {
                        // Copy parameters from reloaded to instance
                        instance->override_count = reloaded->override_count;
                        free(instance->overrides);
                        instance->overrides = reloaded->overrides;
                        reloaded->overrides = NULL;  // Transfer ownership
                        
                        // Recompile shader variant
                        instance->needs_recompile = true;
                        material_instance_compile(instance);
                        
                        material_instance_destroy(reloaded);
                        
                        LOG_INFO("Material %s hot-reloaded successfully", instance->name);
                    }
                }
            }
            
            ft->last_modified = current_mtime;
        }
    }
}

void material_hot_reload_register(MaterialHotReloader *reloader, MaterialInstance *instance) {
    if (!reloader || !instance) return;
    
    // Add to tracked instances
    reloader->instance_count++;
    reloader->instances = (MaterialInstance **)realloc(reloader->instances,
                                                         reloader->instance_count * sizeof(MaterialInstance*));
    reloader->instances[reloader->instance_count - 1] = instance;
    
    LOG_DEBUG("Registered material %s for hot-reload", instance->name);
}

void material_hot_reload_unregister(MaterialHotReloader *reloader, MaterialInstance *instance) {
    if (!reloader || !instance) return;
    
    // Remove from tracked instances
    for (u32 i = 0; i < reloader->instance_count; i++) {
        if (reloader->instances[i] == instance) {
            // Shift remaining
            for (u32 j = i; j < reloader->instance_count - 1; j++) {
                reloader->instances[j] = reloader->instances[j + 1];
            }
            reloader->instance_count--;
            break;
        }
    }
}
