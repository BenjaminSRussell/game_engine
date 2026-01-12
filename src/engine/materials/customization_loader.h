/*
 * Material Customization Loader Header
 */

#ifndef MATERIAL_CUSTOMIZATION_LOADER_H
#define MATERIAL_CUSTOMIZATION_LOADER_H

#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize material system
void material_customization_init(void);

// Shutdown material system
void material_customization_shutdown(void);

// Create default materials
void material_create_default(void);

// Create custom material
int material_create_custom(const char* name, Vec3 albedo, Vec3 normal, 
                      Vec3 metallic, Vec3 roughness, Vec3 emissive,
                      float transparency, int texture_id, int shader_id);

// Get material by ID
void* material_get(int material_id);

// Get material by name
void* material_get_by_name(const char* name);

// Update material properties
void material_update_properties(int material_id, Vec3 albedo, Vec3 normal,
                           Vec3 metallic, Vec3 roughness, Vec3 emissive,
                           float transparency);

// Remove material
void material_remove(int material_id);

// Get all materials
void* material_get_all(int* count);

// Get material count
int material_get_count(void);

// Save materials to file
int material_save_to_file(const char* filename);

// Load materials from file
int material_load_from_file(const char* filename);

#ifdef __cplusplus
}
#endif

#endif // MATERIAL_CUSTOMIZATION_LOADER_H
