// Editor/EditorInterop.m
// Implementation of the bridge
#import "include/editor/EditorInterop.h"
#import "include/backend/metal/mtl_integration.h"
#import "include/core/engine.h"
#import "include/editor/level_editor.h"

// Global Engine State (Mock for now, would be the actual global engine pointer)
static MetalIntegratedRenderer *g_Renderer = NULL;
static LevelEditor *g_LevelEditor = NULL;

@implementation EngineBridge

+ (void)initializeEngineWithDevice:(id<MTLDevice>)device
                             width:(uint32_t)width
                            height:(uint32_t)height {
  // Initialize standard C engine systems
  g_Renderer =
      metal_integrated_create(device, width, height, METALFX_QUALITY_BALANCED);
  g_LevelEditor = level_editor_create();
  NSLog(@"[EngineBridge] Engine initialized via Swift Interop");
}

+ (void)shutdownEngine {
  if (g_Renderer)
    metal_integrated_destroy(g_Renderer);
  g_Renderer = NULL;
}

+ (void)update:(float)deltaTime {
  // Engine Loop Update
  if (g_LevelEditor)
    level_editor_update(g_LevelEditor, deltaTime);
}

+ (void)renderFrameWithCommandBuffer:(id<MTLCommandBuffer>)cmdBuffer
                       outputTexture:(id<MTLTexture>)texture {
  if (g_Renderer) {
    // Construct view/proj matrices from editor camera
    Mat4 view_proj = mat4_identity(); // Placeholder
    Vec3 camera_pos = {0, 0, 0};

    metal_integrated_render_frame(g_Renderer, cmdBuffer, &view_proj,
                                  &camera_pos, NULL, 0, texture);
  }
}

+ (void)editorSelectActor:(uint32_t)actorId {
  // Forward to Level Editor...
}

+ (void)editorMoveActor:(uint32_t)actorId position:(vector_float3)position {
  // Forward to ECS...
}

+ (void)editorSetViewMode:(int)mode {
  // Only implemented for debug views so far
}

+ (uint32_t)getEntityCount {
  return 10; // Placeholder
}

+ (NSString *)getEntityName:(uint32_t)entityId {
  return [NSString stringWithFormat:@"Entity_%d", entityId];
}

+ (vector_float3)getEntityPosition:(uint32_t)entityId {
  return (vector_float3){0, 0, 0};
}

@end
