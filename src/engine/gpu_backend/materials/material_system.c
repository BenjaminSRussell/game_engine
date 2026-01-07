#include "material_system.h"
#include "../../include/core/logger.h"
#include "material_defaults.h" // For default materials
#include <stdlib.h>
#include <string.h>

#define MAX_MASTERS 64
#define MAX_INSTANCES 1024

typedef struct {
    MaterialMaster *masters[MAX_MASTERS];
    u32 master_count;
    
    MaterialInstance *instances[MAX_INSTANCES];
    u32 instance_count;
    
    bool initialized;
} MaterialSystemState;

static MaterialSystemState g_material_system = {0};

bool material_system_init(void) {
    if (g_material_system.initialized) return true;
    
    g_material_system.master_count = 0;
    g_material_system.instance_count = 0;
    g_material_system.initialized = true;
    
    LOG_INFO("Material System initialized");
    
    // Create default masters immediately
    MaterialMaster *pbr = material_create_default_pbr();
    if (pbr) material_system_register_master(pbr);
    
    MaterialMaster *trans = material_create_transparent();
    if (trans) material_system_register_master(trans);
    
    MaterialMaster *emissive = material_create_emissive();
    if (emissive) material_system_register_master(emissive);
    
    MaterialMaster *foliage = material_create_foliage();
    if (foliage) material_system_register_master(foliage);
    
    return true;
}

void material_system_shutdown(void) {
    if (!g_material_system.initialized) return;
    
    // Destroy instances
    for (u32 i = 0; i < g_material_system.instance_count; i++) {
        if (g_material_system.instances[i]) {
            material_instance_destroy(g_material_system.instances[i]);
        }
    }
    
    // Destroy masters
    for (u32 i = 0; i < g_material_system.master_count; i++) {
        if (g_material_system.masters[i]) {
            material_master_destroy(g_material_system.masters[i]);
        }
    }
    
    g_material_system.master_count = 0;
    g_material_system.instance_count = 0;
    g_material_system.initialized = false;
    
    LOG_INFO("Material System shutdown");
}

bool material_system_register_master(MaterialMaster *master) {
    if (!g_material_system.initialized) return false;
    if (!master) return false;
    
    if (g_material_system.master_count >= MAX_MASTERS) {
        LOG_ERROR("Material System: Max masters reached");
        return false;
    }
    
    // Check duplicate name
    if (material_system_get_master(master->name)) {
        LOG_WARN("Material System: Master '%s' already exists", master->name);
        return false;
    }
    
    g_material_system.masters[g_material_system.master_count++] = master;
    LOG_INFO("Registered master material: %s", master->name);
    return true;
}

MaterialMaster *material_system_get_master(const char *name) {
    if (!name) return NULL;
    
    for (u32 i = 0; i < g_material_system.master_count; i++) {
        if (strcmp(g_material_system.masters[i]->name, name) == 0) {
            return g_material_system.masters[i];
        }
    }
    return NULL;
}

MaterialInstance *material_system_create_instance(const char *master_name, const char *instance_name) {
    if (!g_material_system.initialized) return NULL;
    
    MaterialMaster *master = material_system_get_master(master_name);
    if (!master) {
        LOG_ERROR("Material System: Master '%s' not found", master_name);
        return NULL;
    }
    
    if (g_material_system.instance_count >= MAX_INSTANCES) {
        LOG_ERROR("Material System: Max instances reached");
        return NULL;
    }
    
    MaterialInstance *instance = material_instance_create(master, instance_name);
    if (instance) {
        g_material_system.instances[g_material_system.instance_count++] = instance;
    }
    
    return instance;
}

MaterialInstance *material_system_get_instance(const char *name) {
    if (!name) return NULL;
    
    for (u32 i = 0; i < g_material_system.instance_count; i++) {
        if (strcmp(g_material_system.instances[i]->name, name) == 0) {
            return g_material_system.instances[i];
        }
    }
    return NULL;
}

void material_system_destroy_instance(MaterialInstance *instance) {
    if (!instance) return;
    
    // Find and remove
    for (u32 i = 0; i < g_material_system.instance_count; i++) {
        if (g_material_system.instances[i] == instance) {
            // Swap with last
            g_material_system.instances[i] = g_material_system.instances[g_material_system.instance_count - 1];
            g_material_system.instance_count--;
            
            material_instance_destroy(instance);
            return;
        }
    }
}

MaterialMaster *material_system_get_default_pbr(void) {
    return material_system_get_master("M_PBR_Standard");
}
