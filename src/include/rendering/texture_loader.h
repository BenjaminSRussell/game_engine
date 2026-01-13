#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include "engine/include/common.h"
#include <include/rendering/vulkan.h>
#include "include/core/resource/vfs/vfs.h"

// Load atlas map JSON (texture definitions)
bool texture_load_atlas_map(VFS *vfs, const char *path);

// Get UV coordinates for a texture ID
bool texture_get_uv(int texture_id, float *u_min, float *v_min, float *u_max,
                    float *v_max);

// Load texture atlas image setup
bool texture_load_atlas(VulkanRenderer *renderer, VFS *vfs,
                        const char *atlas_path);

// Create texture sampler
bool texture_create_sampler(VulkanRenderer *renderer);

// Setup descriptor sets
bool texture_setup_descriptors(VulkanRenderer *renderer);

// Get tile name (debug)
const char *texture_get_name(int texture_id);

// Validate loaded map
bool texture_validate_atlas_map(void);

#endif // TEXTURE_LOADER_H
