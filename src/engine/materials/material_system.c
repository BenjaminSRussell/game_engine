#include "materials/material_system.h"
#include "materials/material_instance.h"
#include "include/core/memory.h"
#include "include/core/hashmap.h"
#include "include/core/threading.h"
#include "include/core/logger.h"
#include <string.h>

// Internal state
static struct {
    HashMap *masters;   // Map<Name, MaterialMaster*>
    HashMap *instances; // Map<Name, MaterialInstance*>
    MaterialMaster *default_pbr;
    bool initialized;
    Mutex *mutex;
} state;

bool material_system_init(void) {
    if (state.initialized) return true;

    state.masters = hashmap_create(32, sizeof(char*), sizeof(MaterialMaster*), hash_string, equals_string);
    state.instances = hashmap_create(1024, sizeof(char*), sizeof(MaterialInstance*), hash_string, equals_string);
    state.mutex = mutex_create();

    // Mark as initialized so registration works
    state.initialized = true;

    // Create default PBR master
    state.default_pbr = material_master_create("PBR_Default", "assets/shaders/pbr_standard");
    if (state.default_pbr) {
        // Add standard PBR slots
        TextureID default_tex = {0};
        material_master_add_texture(state.default_pbr, "albedoMap", default_tex);
        material_master_add_texture(state.default_pbr, "normalMap", default_tex);
        material_master_add_texture(state.default_pbr, "metallicRoughnessMap", default_tex);
        material_master_add_texture(state.default_pbr, "emissiveMap", default_tex);
        material_master_add_texture(state.default_pbr, "aoMap", default_tex);

        f32 default_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        material_master_add_param(state.default_pbr, "baseColor", MATERIAL_PARAM_COLOR, default_color);

        f32 default_val = 0.5f;
        material_master_add_param(state.default_pbr, "roughness", MATERIAL_PARAM_FLOAT, &default_val);
        material_master_add_param(state.default_pbr, "metallic", MATERIAL_PARAM_FLOAT, &default_val);

        // Register it
        if (!material_system_register_master(state.default_pbr)) {
            LOG_ERROR("Failed to register default PBR master!");
        }
    } else {
        LOG_ERROR("Failed to create default PBR master material!");
    }

    LOG_INFO("Material system initialized");
    return true;
}

void material_system_shutdown(void) {
    if (!state.initialized) return;

    LOG_INFO("Shutting down material system");
    mutex_lock(state.mutex);

    hashmap_destroy(state.masters);
    hashmap_destroy(state.instances);

    mutex_unlock(state.mutex);
    mutex_destroy(state.mutex);

    state.initialized = false;
    LOG_INFO("Material system shutdown complete");
}

bool material_system_register_master(MaterialMaster *master) {
    if (!state.initialized || !master) return false;

    LOG_INFO("Registering master: %s (%p)", master->name, master);

    mutex_lock(state.mutex);
    // Check if exists
    if (hashmap_get_string(state.masters, master->name)) {
        LOG_WARN("Material master '%s' already exists", master->name);
        mutex_unlock(state.mutex);
        return false;
    }

    hashmap_insert_string(state.masters, master->name, &master);

    // Verification
    MaterialMaster **check = (MaterialMaster**)hashmap_get_string(state.masters, master->name);
    if (check && *check == master) {
        LOG_INFO("Master registered successfully");
    } else {
        LOG_ERROR("Master registration verification failed! Get returned %p", check);
    }

    mutex_unlock(state.mutex);
    return true;
}

MaterialInstance *material_system_create_instance(const char *master_name, const char *instance_name) {
    if (!state.initialized) return NULL;

    LOG_INFO("Creating instance '%s' from master '%s'", instance_name, master_name);

    mutex_lock(state.mutex);

    MaterialMaster **master_ptr = (MaterialMaster**)hashmap_get_string(state.masters, master_name);
    if (!master_ptr) {
        LOG_ERROR("Material master '%s' not found", master_name);
        mutex_unlock(state.mutex);
        return NULL;
    }

    if (hashmap_get_string(state.instances, instance_name)) {
        LOG_WARN("Material instance '%s' already exists", instance_name);
        mutex_unlock(state.mutex);
        return NULL;
    }

    MaterialInstance *instance = material_instance_create(*master_ptr, instance_name);

    if (instance) {
        hashmap_insert_string(state.instances, instance_name, &instance);
        LOG_INFO("Instance inserted into hashmap");
    }

    mutex_unlock(state.mutex);
    return instance;
}

MaterialMaster *material_system_get_master(const char *name) {
    if (!state.initialized) return NULL;
    mutex_lock(state.mutex);
    MaterialMaster **ptr = (MaterialMaster**)hashmap_get_string(state.masters, name);
    mutex_unlock(state.mutex);
    return ptr ? *ptr : NULL;
}

MaterialInstance *material_system_get_instance(const char *name) {
    if (!state.initialized) return NULL;
    mutex_lock(state.mutex);
    MaterialInstance **ptr = (MaterialInstance**)hashmap_get_string(state.instances, name);
    mutex_unlock(state.mutex);
    return ptr ? *ptr : NULL;
}

void material_system_destroy_instance(MaterialInstance *instance) {
    if (!state.initialized || !instance) return;

    LOG_INFO("Destroying instance: %s", instance->name);
    mutex_lock(state.mutex);

    const char *key = instance->name;
    if (hashmap_get_string(state.instances, key)) {
        LOG_INFO("Removing from hashmap");
        hashmap_remove(state.instances, &key);
    } else {
        LOG_WARN("Instance not found in hashmap");
    }

    LOG_INFO("Freeing instance memory");
    material_instance_destroy(instance);

    mutex_unlock(state.mutex);
    LOG_INFO("Instance destroyed");
}

MaterialMaster *material_system_get_default_pbr(void) {
    return state.default_pbr;
}
