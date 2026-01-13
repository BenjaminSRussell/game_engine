/*
 * swapchain_example.m
 * Example usage of Metal swapchain with display detection and HDR
 *
 * This example demonstrates the complete integration of all Agent 1.6
 * components.
 */

#import "mtl_device.h"
#import "mtl_display.h"
#import "mtl_hdr.h"
#import "mtl_swapchain.h"
#import <Cocoa/Cocoa.h>

void swapchain_usage_example(NSView *view) {
  // 1. Create Metal device
  metal_device_t *device = metal_device_create_system_default();
  if (!device) {
    NSLog(@"Failed to create Metal device");
    return;
  }

  // 2. Query display capabilities
  metal_display_info_t *display_info =
      metal_display_get_info(NULL); // NULL = main screen
  if (display_info) {
    // Check display capabilities
    bool has_hdr = metal_display_supports_hdr(display_info);
    bool has_vrr = metal_display_supports_vrr(display_info);
    double max_fps = metal_display_get_max_refresh_rate(display_info);
    float edr_headroom = metal_display_get_edr_headroom(display_info);

    NSLog(@"Display Capabilities:");
    NSLog(@"  HDR Supported: %@", has_hdr ? @"YES" : @"NO");
    NSLog(@"  VRR Supported: %@", has_vrr ? @"YES" : @"NO");
    NSLog(@"  Max Refresh Rate: %.0f Hz", max_fps);
    NSLog(@"  EDR Headroom: %.2f", edr_headroom);

    // 3. Determine optimal pixel format
    MTLPixelFormat format = metal_hdr_recommend_format(has_hdr, false);
    NSLog(@"Recommended Format: %s", metal_hdr_get_format_name(format));

    // 4. Create swapchain with HDR if supported
    metal_swapchain_t *swapchain =
        metal_swapchain_create(device, (__bridge void *)view, has_hdr);

    if (swapchain) {
      // 5. Configure present mode
      if (has_vrr) {
        // Use adaptive mode on ProMotion displays
        metal_swapchain_set_present_mode(swapchain,
                                         METAL_PRESENT_MODE_ADAPTIVE);
      } else {
        // Use standard vsync
        metal_swapchain_set_present_mode(swapchain, METAL_PRESENT_MODE_VSYNC);
      }

      // Set drawable timeout to prevent hangs
      metal_swapchain_set_drawable_timeout(swapchain, 1.0); // 1 second

      // 6. Render loop example
      for (int frame = 0; frame < 10; frame++) {
        // Begin frame timing
        metal_swapchain_begin_frame(swapchain);

        // Acquire drawable
        id<MTLTexture> drawable_texture =
            metal_swapchain_get_texture(swapchain);
        if (!drawable_texture) {
          NSLog(@"Warning: Failed to acquire drawable (frame %d)", frame);
          continue;
        }

        // Create command buffer
        id<MTLCommandBuffer> cmd =
            (__bridge id<MTLCommandBuffer>)metal_create_command_buffer(device);

        // Render to drawable_texture
        if (drawable_texture) {
            // Create render pass descriptor
            MTLRenderPassDescriptor *renderPassDesc = [MTLRenderPassDescriptor renderPassDescriptor];
            MTLRenderPassColorAttachmentDescriptor *colorAttachment = renderPassDesc.colorAttachments[0];
            
            colorAttachment.texture = drawable_texture;
            colorAttachment.loadAction = MTLLoadActionClear;
            colorAttachment.storeAction = MTLStoreActionStore;
            colorAttachment.clearColor = MTLClearColorMake(0.1, 0.2, 0.3, 1.0);
            
            // Create command encoder
            id<MTLRenderCommandEncoder> renderEncoder = [cmd renderCommandEncoderWithDescriptor:renderPassDesc];
            
            // Set viewport
            MTLViewport viewport = {
                0.0, 0.0,
                (double)drawable_texture.width,
                (double)drawable_texture.height,
                0.0, 1.0
            };
            [renderEncoder setViewport:viewport];
            
            // Simple triangle rendering
            struct {
                float position[4];
                float color[4];
            } vertices[] = {
                {{ 0.0,  0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}},  // Top - Red
                {{-0.5, -0.5, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}},  // Bottom Left - Green
                {{ 0.5, -0.5, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}}   // Bottom Right - Blue
            };
            
            // Create vertex buffer (simplified - in real code this would be cached)
            id<MTLBuffer> vertexBuffer = [device newBufferWithBytes:vertices
                                                                         length:sizeof(vertices)
                                                                        options:MTLResourceStorageModeShared];
            
            // Set vertex buffer and draw
            [renderEncoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
            [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
            
            // End encoding
            [renderEncoder endEncoding];
        }

        // Present drawable
        metal_swapchain_present(swapchain, cmd);
        [cmd commit];

        // End frame timing
        metal_swapchain_end_frame(swapchain);

        // Query statistics every 60 frames
        if (frame % 60 == 0) {
          metal_swapchain_stats_t stats;
          metal_swapchain_get_statistics(swapchain, &stats);

          double fps = 1000.0 / stats.avg_frame_time_ms;
          NSLog(@"Frame Stats:");
          NSLog(@"  FPS: %.1f", fps);
          NSLog(@"  Avg Frame Time: %.2f ms", stats.avg_frame_time_ms);
          NSLog(@"  Dropped Frames: %llu", stats.dropped_frames);
          NSLog(@"  Drawable Acquire Time: %.2f ms",
                stats.drawable_acquire_time_ms);
        }
      }

      // 7. Cleanup
      metal_swapchain_destroy(swapchain);
    }

    metal_display_info_destroy(display_info);
  }

  metal_device_destroy(device);
}

void hdr_color_conversion_example(void) {
  // Example: Convert SRGB colors to linear for rendering
  float srgb_color[3] = {0.5f, 0.5f, 0.5f}; // Mid gray in SRGB

  NSLog(@"SRGB Color: (%.3f, %.3f, %.3f)", srgb_color[0], srgb_color[1],
        srgb_color[2]);

  metal_hdr_convert_srgb_to_linear(srgb_color);
  NSLog(@"Linear Color: (%.3f, %.3f, %.3f)", srgb_color[0], srgb_color[1],
        srgb_color[2]);

  // Example: HDR tone mapping
  float hdr_color[3] = {2.5f, 1.8f, 1.2f}; // HDR values > 1.0
  NSLog(@"HDR Color (before): (%.2f, %.2f, %.2f)", hdr_color[0], hdr_color[1],
        hdr_color[2]);

  metal_hdr_tonemap_reinhard(hdr_color, 2.0f);
  NSLog(@"HDR Color (after Reinhard): (%.2f, %.2f, %.2f)", hdr_color[0],
        hdr_color[1], hdr_color[2]);

  // Calculate luminance
  float luma = metal_hdr_calculate_luminance(hdr_color);
  NSLog(@"Luminance: %.3f", luma);
}

int main(int argc, const char *argv[]) {
  @autoreleasepool {
    NSLog(@"=== Metal Swapchain Example ===");

    // Display HDR utilities example
    hdr_color_conversion_example();

    NSLog(@"\n=== Example Complete ===");
    NSLog(@"Note: Full swapchain example requires an NSView instance");
  }
  return 0;
}
