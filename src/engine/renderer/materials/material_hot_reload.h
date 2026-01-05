// Material Hot-Reload System
// Watches .mat files and auto-reloads on changes

#ifndef MATERIAL_HOT_RELOAD_H
#define MATERIAL_HOT_RELOAD_H

#include "material_instance.h"
#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// File watcher for materials
typedef struct MaterialHotReloader {
    MaterialMaster **masters;
    u32 master_count;
    MaterialInstance **instances;
    u32 instance_count;
    
    // Platform-specific file watcher
    void *platform_watcher;
    bool is_watching;
} MaterialHotReloader;

// Create hot-reload system
MaterialHotReloader *material_hot_reload_create(void);

// Destroy hot-reload system
void material_hot_reload_destroy(MaterialHotReloader *reloader);

// Start watching materials directory
void material_hot_reload_start(MaterialHotReloader *reloader, const char *materials_dir);

// Stop watching
void material_hot_reload_stop(MaterialHotReloader *reloader);

// Poll for file changes (call each frame)
void material_hot_reload_update(MaterialHotReloader *reloader);

// Register material for hot-reload
void material_hot_reload_register(MaterialHotReloader *reloader, MaterialInstance *instance);

// Unregister material
void material_hot_reload_unregister(MaterialHotReloader *reloader, MaterialInstance *instance);

#ifdef __cplusplus
}
#endif

#endif // MATERIAL_HOT_RELOAD_H
