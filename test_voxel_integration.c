#include <stdio.h>
#include <stdlib.h>

// Include the engine headers
#include "src/engine/rendering/voxel_renderer.h"
#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_shader_library.h"

int main() {
    printf("Testing voxel renderer with shader fixes...\n");
    
    // Create Metal device
    metal_device_t *device = metal_device_create();
    if (!device) {
        printf("❌ Failed to create Metal device\n");
        return 1;
    }
    
    printf("✅ Metal device created\n");
    
    // Create voxel renderer
    VoxelRenderer *renderer = voxel_renderer_create(device);
    if (!renderer) {
        printf("❌ Failed to create voxel renderer\n");
        metal_device_destroy(device);
        return 1;
    }
    
    printf("✅ Voxel renderer created successfully\n");
    printf("🎉 Shader loading fix is working!\n");
    
    // Cleanup
    voxel_renderer_destroy(renderer);
    metal_device_destroy(device);
    
    return 0;
}
