#include "mtl_device_caps.h"

void mtl_device_caps_query(id<MTLDevice> device, MTLDeviceCaps* caps) {
    if (!caps || !device) {
        return;
    }
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 101500
    caps->has_ray_tracing = [device supportsRaytracing];
#else
    caps->has_ray_tracing = false;
#endif

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 110000
    caps->has_mesh_shaders = [device supportsFamily:MTLGPUFamilyApple7] || [device supportsFamily:MTLGPUFamilyMac1];
#else
    caps->has_mesh_shaders = false;
#endif
    // Add more capability checks here
}
