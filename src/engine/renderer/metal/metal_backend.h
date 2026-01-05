#ifndef METAL_BACKEND_H
#define METAL_BACKEND_H

#include <renderer/renderer.h>

// Metal Renderer Type
typedef struct MetalRenderer {
    IRenderer interface; // Base interface
    void* device;        // MTLDevice (id<MTLDevice>)
    void* command_queue; // MTLCommandQueue (id<MTLCommandQueue>)
    void* metal_layer;   // CAMetalLayer
    u32 current_frame_index;
} MetalRenderer;

// Creation
IRenderer* metal_renderer_create(void);

// C-Bridge functions for Metal Device (implemented in metal_device.m)
bool metal_device_init(void);
void metal_device_shutdown(void);
void* metal_device_get(void);
void* metal_device_get_command_queue(void);
void metal_device_set_layer(void* layer);

#endif // METAL_BACKEND_H
