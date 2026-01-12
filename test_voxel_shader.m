#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include <stdio.h>

int main() {
    @autoreleasepool {
        // Get the default device
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            printf("No Metal device found\n");
            return 1;
        }
        
        printf("Testing voxel shader loading...\n");
        
        // Try to load the voxel shader file
        NSString *shaderPath = @"src/engine/rendering/shaders/voxel.metal";
        NSString *shaderSource = [NSString stringWithContentsOfFile:shaderPath 
                                                            encoding:NSUTF8StringEncoding 
                                                               error:nil];
        
        if (!shaderSource) {
            printf("Failed to load voxel shader file: %s\n", [shaderPath UTF8String]);
            return 1;
        }
        
        printf("Loaded voxel shader file (%lu characters)\n", (unsigned long)[shaderSource length]);
        
        // Try to compile it
        NSError *error = nil;
        id<MTLLibrary> library = [device newLibraryWithSource:shaderSource options:nil error:&error];
        
        if (!library) {
            printf("Shader compilation failed: %s\n", [[error localizedDescription] UTF8String]);
            return 1;
        }
        
        printf("Voxel shader compilation successful!\n");
        printf("Functions found: %lu\n", (unsigned long)[[library functionNames] count]);
        
        for (NSString *functionName in [library functionNames]) {
            printf("  - %s\n", [functionName UTF8String]);
        }
        
        // Test specific function loading
        id<MTLFunction> vertexFunc = [library newFunctionWithName:@"voxel_vertex_main"];
        id<MTLFunction> fragmentFunc = [library newFunctionWithName:@"voxel_fragment_main"];
        
        if (vertexFunc && fragmentFunc) {
            printf("Both voxel vertex and fragment functions loaded successfully!\n");
        } else {
            printf("Failed to load voxel shader functions\n");
            printf("   Vertex function: %s\n", vertexFunc ? "SUCCESS" : "FAILED");
            printf("   Fragment function: %s\n", fragmentFunc ? "SUCCESS" : "FAILED");
            return 1;
        }
        
        printf("Voxel shader loading and compilation is working!\n");
    }
    
    return 0;
}
