#ifndef MTL_DEVICE_CAPS_H
#define MTL_DEVICE_CAPS_H

#include <stdbool.h>
#import <Metal/Metal.h>

typedef struct {
    bool has_ray_tracing;
    bool has_mesh_shaders;
    // Add more capability flags here
} MTLDeviceCaps;

void mtl_device_caps_query(id<MTLDevice> device, MTLDeviceCaps* caps);

#endif // MTL_DEVICE_CAPS_H
