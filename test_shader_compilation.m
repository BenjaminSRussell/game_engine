#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include <stdio.h>

// Test the specific shader loading approach used in the fixed voxel renderer
int test_shader_loading_fix() {
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            printf("❌ No Metal device found\n");
            return 0;
        }
        
        printf("Testing shader loading fix...\n");
        
        // Test 1: Broken approach (what was causing the issue)
        printf("\n1. Testing BROKEN approach (newLibraryWithURL):\n");
        NSString *shaderPath = @"src/engine/rendering/shaders/voxel.metal";
        NSURL *url = [NSURL fileURLWithPath:shaderPath];
        NSError *error = nil;
        
        id<MTLLibrary> brokenLib = [device newLibraryWithURL:url error:&error];
        if (brokenLib) {
            printf("   ❌ Unexpectedly succeeded\n");
            [brokenLib release];
        } else {
            printf("   ✅ Failed as expected: %s\n", [[error localizedDescription] UTF8String]);
        }
        
        // Test 2: Fixed approach (what we implemented)
        printf("\n2. Testing FIXED approach (newLibraryWithSource):\n");
        NSString *shaderSource = [NSString stringWithContentsOfFile:shaderPath
                                                            encoding:NSUTF8StringEncoding
                                                               error:&error];
        if (!shaderSource) {
            printf("   ❌ Failed to read shader file: %s\n", [[error localizedDescription] UTF8String]);
            return 0;
        }
        
        id<MTLLibrary> fixedLib = [device newLibraryWithSource:shaderSource
                                                       options:nil
                                                         error:&error];
        if (!fixedLib) {
            printf("   ❌ Compilation failed: %s\n", [[error localizedDescription] UTF8String]);
            return 0;
        }
        
        printf("   ✅ Success! Shader compiled\n");
        printf("   Functions found: %lu\n", (unsigned long)[[fixedLib functionNames] count]);
        
        // Test specific voxel functions
        id<MTLFunction> vertexFunc = [fixedLib newFunctionWithName:@"voxel_vertex_main"];
        id<MTLFunction> fragmentFunc = [fixedLib newFunctionWithName:@"voxel_fragment_main"];
        
        if (vertexFunc && fragmentFunc) {
            printf("   ✅ Both voxel shader functions loaded!\n");
            [vertexFunc release];
            [fragmentFunc release];
        } else {
            printf("   ❌ Failed to load voxel functions\n");
            printf("      Vertex: %s\n", vertexFunc ? "✅" : "❌");
            printf("      Fragment: %s\n", fragmentFunc ? "✅" : "❌");
            [fixedLib release];
            return 0;
        }
        
        [fixedLib release];
        return 1;
    }
}

int main() {
    printf("=== Shader Loading Fix Verification ===\n");
    
    int success = test_shader_loading_fix();
    
    if (success) {
        printf("\n🎉 SHADER LOADING FIX VERIFICATION PASSED!\n");
        printf("The voxel renderer should now work correctly.\n");
    } else {
        printf("\n❌ SHADER LOADING FIX VERIFICATION FAILED!\n");
    }
    
    return success ? 0 : 1;
}
