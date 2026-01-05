
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

// Simple stub for async Metal IO
// This would hook into MTLLoader or similar in a real engine

void metal_io_init(void) {
  // Initialize IO queues
}

void metal_io_shutdown(void) {
  // Shutdown queues
}

void metal_io_load_texture_async(const char *path,
                                 void (*callback)(void *texture,
                                                  void *userdata),
                                 void *userdata) {
  // In a real implementation:
  // 1. Dispatch background loading of file
  // 2. Create MTLTexture via buffer
  // 3. Dispatch callback on main/render thread

  // For now, no-op or synchronous stub could reduce complexity
}
