#include "rendering/texture_loader.h"
#include <core/logger.h>
#include <stdlib.h>

// Stub implementations to fix linker errors
// These should ideally be implemented using the actual Vulkan/Metal backend

bool texture_create_sampler(VulkanRenderer *renderer) {
    LOG_INFO("Stub: texture_create_sampler");
    return true;
}

bool texture_load_atlas(VulkanRenderer *renderer, VFS *vfs, const char *atlas_path) {
    LOG_INFO("Stub: texture_load_atlas");
    return true;
}

bool texture_load_atlas_map(VFS *vfs, const char *path) {
    LOG_INFO("Stub: texture_load_atlas_map");
    return true;
}

bool texture_setup_descriptors(VulkanRenderer *renderer) {
    LOG_INFO("Stub: texture_setup_descriptors");
    return true;
}
