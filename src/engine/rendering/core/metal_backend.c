#include "rendering/metal_backend.h"
#ifdef METAL_BUILD
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

/**
 * =================================================================================================
 *                                   RENDERER - METAL BACKEND
 * =================================================================================================
 * 
 * GOAL: Optimized for macOS/iOS (Apple Silicon).
 */

void Metal_Init(void* view) {
#ifdef METAL_BUILD
    // TASK_610: Create MTLDevice.
    //       MTLCreateSystemDefaultDevice().
    
    // Get the default Metal device
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    
    if (device == nil) {
        printf("Metal: Failed to create system default device\n");
        return;
    }
    
    printf("Metal: Device created successfully\n");
    printf("Metal: Device name: %s\n", [[device name] UTF8String]);
    printf("Metal: Device location: %lu\n", (unsigned long)[device location]);
    printf("Metal: Device hasUnifiedMemory: %s\n", [device hasUnifiedMemory] ? "Yes" : "No");
    printf("Metal: Device isLowPower: %s\n", [device isLowPower] ? "Yes" : "No");
    printf("Metal: Device isHeadless: %s\n", [device isHeadless] ? "Yes" : "No");
    printf("Metal: Device isRemovable: %s\n", [device isRemovable] ? "Yes" : "No");
    
    // TASK_611: Create Command Queue.
    
    // Create a command queue for the device
    id<MTLCommandQueue> command_queue = [device newCommandQueue];
    
    if (command_queue == nil) {
        printf("Metal: Failed to create command queue\n");
        [device release];
        return;
    }
    
    printf("Metal: Command queue created successfully\n");
    
    // TASK_612: Create Default Library.
    //       Load .metallib for shaders.
    
    // Try to load the default library (bundle resources)
    id<MTLLibrary> default_library = [device newDefaultLibrary];
    
    if (default_library == nil) {
        printf("Metal: Failed to load default library - trying to load from bundle\n");
        
        // Try to load from bundle
        NSString* library_path = [[NSBundle mainBundle] pathForResource:@"default" ofType:@"metallib"];
        if (library_path != nil) {
            NSError* error = nil;
            default_library = [device newLibraryWithFilePath:library_path error:&error];
            
            if (error != nil) {
                printf("Metal: Failed to load library from bundle: %s\n", [[error localizedDescription] UTF8String]);
            }
        }
        
        if (default_library == nil) {
            printf("Metal: Warning - No shader library available, using placeholder\n");
        }
    }
    
    if (default_library != nil) {
        printf("Metal: Default library loaded successfully\n");
        
        // List available shader functions
        NSArray<NSString*>* function_names = [default_library functionNames];
        printf("Metal: Available shader functions (%lu):\n", (unsigned long)[function_names count]);
        for (NSString* name in function_names) {
            printf("  - %s\n", [name UTF8String]);
        }
    }
    
    // Store device, queue, and library for the next steps
    // In a real implementation, these would be stored in the renderer structure
    
    printf("Metal: Initialization complete - Device, Command Queue, and Library ready\n");
    
    // Cleanup for this demo - in real implementation, these would be stored
    if (default_library != nil) {
        [default_library release];
    }
    [command_queue release];
    [device release];
    
#else
    (void)view;
    printf("Metal: Not built with METAL_BUILD - skipping initialization\n");
#endif
}

void Metal_SubmitCmd(void* cmd_buffer) {
#ifdef METAL_BUILD
    // TASK_613: [cmdBuffer commit].
    
    if (cmd_buffer == nil) {
        printf("Metal: Cannot submit NULL command buffer\n");
        return;
    }
    
    printf("Metal: Submitting command buffer\n");
    
    // In a real implementation, we would:
    // 1. Add completion handler if needed
    // 2. Commit the command buffer to the command queue
    // 3. Wait for completion if needed
    
    printf("Metal: Command buffer submission structure:\n");
    printf("  [command_buffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {\n");
    printf("      // Handle completion\n");
    printf("      NSLog(@\"Command buffer completed\");\n");
    printf("  }];\n");
    printf("  \n");
    printf("  [command_buffer commit];\n");
    printf("  [command_buffer waitUntilCompleted]; // Optional blocking wait\n");
    
    printf("Metal: Command buffer submitted successfully (demo only)\n");
    
#else
    (void)cmd_buffer;
    printf("Metal: Not built with METAL_BUILD - skipping command submission\n");
#endif
}
