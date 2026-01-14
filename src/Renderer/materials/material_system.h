#ifndef MATERIAL_SYSTEM_H
#define MATERIAL_SYSTEM_H

#include "rendering/materials/material_instance.h"
#include "include/core/types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the material system
bool material_system_init(void);

// Shutdown the material system
void material_system_shutdown(void);

// Register a master material (takes ownership)
bool material_system_register_master(MaterialMaster *master);

// Create and register a new instance from a master name
MaterialInstance *material_system_create_instance(const char *master_name, const char *instance_name);

// Get a master material by name
MaterialMaster *material_system_get_master(const char *name);

// Get a material instance by name
MaterialInstance *material_system_get_instance(const char *name);

// Destroy a specific instance (and remove from registry)
void material_system_destroy_instance(MaterialInstance *instance);

// Get Default PBR Master (convenience)
MaterialMaster *material_system_get_default_pbr(void);

#ifdef __cplusplus
}
#endif

#endif // MATERIAL_SYSTEM_H
