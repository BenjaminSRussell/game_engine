// Mod API registration and dispatch.
// Roadmap: docs/MOD_API_ROADMAP.md.
// TODO: Implement mod API versioning system for compatibility.
// TODO: Add mod API validation system for invalid hooks.
// TODO: Implement mod API hot-reload system for development.
// TODO: Add mod API security system for sandboxing.
// TODO: Implement mod API event system for inter-mod communication.
// TODO: Add mod API documentation generation system.
// TODO: Implement mod API testing framework.
// TODO: Add mod API performance monitoring system.
// TODO: Implement mod API dependency resolution system.
// TODO: Add mod API error recovery system.
#include <modding/mod_api.h>
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <pthread.h>

ModRegistry *mod_registry_create(void) {
    ModRegistry *registry = (ModRegistry *)calloc(1, sizeof(ModRegistry));
    if (!registry) {
        LOG_ERROR("Failed to allocate mod registry");
        return NULL;
    }
    
    registry->mod_count = 0;
    for (u32 i = 0; i < MOD_HOOK_COUNT; i++) {
        registry->hook_counts[i] = 0;
    }

#ifndef PLATFORM_WEB
    if (pthread_mutex_init(&registry->mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize mod registry mutex");
        free(registry);
        return NULL;
    }
#endif
    
    LOG_INFO("Mod registry created");
    return registry;
}

void mod_registry_destroy(ModRegistry *registry) {
    if (!registry) return;

#ifndef PLATFORM_WEB
    pthread_mutex_lock(&registry->mutex);
#endif
    for (u32 i = 0; i < registry->mod_count; i++) {
        mod_registry_unload_mod(registry, i);
    }
#ifndef PLATFORM_WEB
    pthread_mutex_unlock(&registry->mutex);
    pthread_mutex_destroy(&registry->mutex);
#endif
    
    free(registry);
    LOG_INFO("Mod registry destroyed");
}
bool mod_registry_load_mod(ModRegistry *registry, const char *mod_path) {
    if (!registry || !mod_path) {
        LOG_ERROR("Invalid registry or mod path");
        return false;
    }

#ifndef PLATFORM_WEB
    pthread_mutex_lock(&registry->mutex);
#endif
    if (registry->mod_count >= MAX_MODS_LOADED) {
#ifndef PLATFORM_WEB
        pthread_mutex_unlock(&registry->mutex);
#endif
        LOG_ERROR("Max mods loaded (%d)", MAX_MODS_LOADED);
        return false;
    }
#ifndef PLATFORM_WEB
    pthread_mutex_unlock(&registry->mutex);
#endif
    
    // Load shared library
    void *handle = dlopen(mod_path, RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        LOG_ERROR("Failed to load mod: %s - %s", mod_path, dlerror());
        return false;
    }
    
    typedef ModInfo* (*GetModInfoFunc)(void);
    GetModInfoFunc get_mod_info = (GetModInfoFunc)dlsym(handle, "get_mod_info");
    if (!get_mod_info) {
        LOG_ERROR("Mod missing get_mod_info function: %s", mod_path);
        dlclose(handle);
        return false;
    }
    
    ModInfo *info = get_mod_info();
    if (!info) {
        LOG_ERROR("Mod returned null info: %s", mod_path);
        dlclose(handle);
        return false;
    }

    // Validate mod info structure
    if (!info->name || strlen(info->name) == 0 || strlen(info->name) > 255) {
        LOG_ERROR("Mod has invalid name: %s", mod_path);
        dlclose(handle);
        return false;
    }

    if (!info->version || strlen(info->version) == 0 || strlen(info->version) > 63) {
        LOG_ERROR("Mod %s has invalid version string", info->name);
        dlclose(handle);
        return false;
    }

    
#ifndef PLATFORM_WEB
    pthread_mutex_lock(&registry->mutex);
#endif
    Mod *mod = &registry->mods[registry->mod_count];
    mod_init(mod, info);
    mod->handle = handle;
    mod->loaded = true;

    if (info->on_init) {
        info->on_init(mod);
    }

    registry->mod_count++;
#ifndef PLATFORM_WEB
    pthread_mutex_unlock(&registry->mutex);
#endif

    LOG_INFO("Loaded mod: %s v%d.%d.%d", info->name, info->version_major, 
             info->version_minor, info->version_patch);
    
    return true;
}
bool mod_registry_unload_mod(ModRegistry *registry, u32 mod_id) {
    if (!registry || mod_id >= registry->mod_count) {
        return false;
    }

#ifndef PLATFORM_WEB
    pthread_mutex_lock(&registry->mutex);
#endif
    Mod *mod = &registry->mods[mod_id];

    if (mod->info.on_shutdown) {
        mod->info.on_shutdown(mod);
    }

    mod_shutdown(mod);

    if (mod->handle) {
        dlclose(mod->handle);
        mod->handle = NULL;
    }

    mod->loaded = false;
#ifndef PLATFORM_WEB
    pthread_mutex_unlock(&registry->mutex);
#endif
    LOG_INFO("Unloaded mod: %s", mod->info.name);

    return true;
}

Mod *mod_registry_get_mod(ModRegistry *registry, const char *name) {
    if (!registry || !name) return NULL;

#ifndef PLATFORM_WEB
    pthread_mutex_lock(&registry->mutex);
#endif
    for (u32 i = 0; i < registry->mod_count; i++) {
        if (strcmp(registry->mods[i].info.name, name) == 0) {
#ifndef PLATFORM_WEB
            pthread_mutex_unlock(&registry->mutex);
#endif
            return &registry->mods[i];
        }
    }
#ifndef PLATFORM_WEB
    pthread_mutex_unlock(&registry->mutex);
#endif
    return NULL;
}

void mod_registry_register_hook(ModRegistry *registry, ModHookType type, 
                                ModHookCallback callback, void *user_data) {
    if (!registry || !callback || type >= MOD_HOOK_COUNT) {
        return;
    }

#ifndef PLATFORM_WEB
    pthread_mutex_lock(&registry->mutex);
#endif
    if (registry->hook_counts[type] >= MAX_MOD_HOOKS) {
#ifndef PLATFORM_WEB
        pthread_mutex_unlock(&registry->mutex);
#endif
        LOG_WARN("Max hooks registered for type %d", type);
        return;
    }

    ModHook *hook = &registry->global_hooks[type][registry->hook_counts[type]];
    hook->type = type;
    hook->callback = callback;
    hook->user_data = user_data;

    registry->hook_counts[type]++;
#ifndef PLATFORM_WEB
    pthread_mutex_unlock(&registry->mutex);
#endif
}

void mod_registry_trigger_hook(ModRegistry *registry, ModHookType type, void *event_data) {
    if (!registry || type >= MOD_HOOK_COUNT) {
        return;
    }

    // Copy global hooks while holding lock, then invoke without holding lock
#ifndef PLATFORM_WEB
    pthread_mutex_lock(&registry->mutex);
#endif
    u32 global_count = registry->hook_counts[type];
    ModHook global_copy[MAX_MOD_HOOKS];
    if (global_count > 0) {
        memcpy(global_copy, registry->global_hooks[type], sizeof(ModHook) * global_count);
    }

    // Gather mod hooks into a local list so we can call them without the lock
    typedef struct { Mod *mod; ModHook hook; } LocalHook;
    LocalHook local_hooks[MAX_MODS_LOADED * MOD_HOOK_COUNT];
    u32 local_count = 0;

    for (u32 i = 0; i < registry->mod_count; i++) {
        Mod *mod = &registry->mods[i];
        if (!mod->loaded || !mod->enabled) continue;
        for (u32 j = 0; j < mod->hook_count; j++) {
            ModHook *h = &mod->hooks[j];
            if (h->type == type && h->callback) {
                local_hooks[local_count].mod = mod;
                local_hooks[local_count].hook = *h;
                local_count++;
            }
        }
    }
#ifndef PLATFORM_WEB
    pthread_mutex_unlock(&registry->mutex);
#endif

    for (u32 i = 0; i < global_count; i++) {
        ModHook *hook = &global_copy[i];
        if (hook->callback) {
            hook->callback(NULL, event_data);
        }
    }

    for (u32 i = 0; i < local_count; i++) {
        LocalHook *lh = &local_hooks[i];
        if (lh->hook.callback) {
            lh->hook.callback(lh->mod, event_data);
        }
    }
}

void mod_registry_update(ModRegistry *registry, f32 delta_time) {
    if (!registry) return;

#ifndef PLATFORM_WEB
    pthread_mutex_lock(&registry->mutex);
#endif
    Mod *mods_copy[MAX_MODS_LOADED];
    u32 count = registry->mod_count;
    for (u32 i = 0; i < count; i++) {
        mods_copy[i] = &registry->mods[i];
    }
#ifndef PLATFORM_WEB
    pthread_mutex_unlock(&registry->mutex);
#endif

    for (u32 i = 0; i < count; i++) {
        Mod *mod = mods_copy[i];
        if (!mod->loaded || !mod->enabled) continue;

        if (mod->info.on_update) {
            mod->info.on_update(mod, delta_time);
        }
    }
}

void mod_init(Mod *mod, const ModInfo *info) {
    if (!mod || !info) return;
    
    memcpy(&mod->info, info, sizeof(ModInfo));
    mod->hook_count = 0;
    mod->loaded = false;
    mod->enabled = true;
    mod->handle = NULL;
    mod->user_data = NULL;
    
    memset(mod->hooks, 0, sizeof(mod->hooks));
}

void mod_shutdown(Mod *mod) {
    if (!mod) return;
    
    mod->hook_count = 0;
    mod->enabled = false;
    mod->user_data = NULL;
}

void mod_register_hook(Mod *mod, ModHookType type, ModHookCallback callback, void *user_data) {
    if (!mod || !callback || type >= MOD_HOOK_COUNT) {
        return;
    }
    
    if (mod->hook_count >= MOD_HOOK_COUNT) {
        LOG_WARN("Max hooks for mod %s", mod->info.name);
        return;
    }
    
    ModHook *hook = &mod->hooks[mod->hook_count];
    hook->type = type;
    hook->callback = callback;
    hook->user_data = user_data;
    
    mod->hook_count++;
}
