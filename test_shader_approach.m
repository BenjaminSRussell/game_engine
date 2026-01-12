#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include <stdio.h>

// Simulate the shader library loading approach from the engine
id<MTLLibrary> load_shader_library_correctly(id<MTLDevice> device, const char *path) {
    NSString *nsPath = [NSString stringWithUTF8String:path];
    NSError *error = nil;
    
    // Load source and compile at runtime (this is what the shader library manager does)
    NSString *source = [NSString stringWithContentsOfFile:nsPath
                                                 encoding:NSUTF8StringEncoding
                                                    error:&error];
    if (source) {
        id<MTLLibrary> library = [device newLibraryWithSource:source
                                                       options:nil
                                                         error:&error];
        if (error) {
            NSLog(@"Shader compilation error: %@", error);
        }
        return library;
    }
    return nil;
}

// Simulate the old broken approach
id<MTLLibrary> load_shader_library_broken(id<MTLDevice> device, const char *path) {
    NSString *nsPath = [NSString stringWithUTF8String:path];
    NSURL *url = [NSURL fileURLWithPath:nsPath];
    NSError *error = nil;
    
    // This tries to load .metal as compiled library - will fail
    id<MTLLibrary> library = [device newLibraryWithURL:url error:&error];
    if (error) {
        NSLog(@"Failed to load shader library from %@: %@", nsPath, error);
    }
    return library;
}

int main() {
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            printf("No Metal device found\n");
            return 1;
        }
        
        const char *shaderPath = "src/engine/rendering/shaders/voxel.metal";
        
        printf("Testing shader loading approaches...\n\n");
        
        // Test broken approach
        printf("1. Testing BROKEN approach (newLibraryWithURL):\n");
        id<MTLLibrary> brokenLib = load_shader_library_broken(device, shaderPath);
        if (brokenLib) {
            printf("   Unexpectedly succeeded\n");
        } else {
            printf("   Failed as expected (this was the problem)\n");
        }
        
        printf("\n2. Testing FIXED approach (newLibraryWithSource):\n");
        id<MTLLibrary> fixedLib = load_shader_library_correctly(device, shaderPath);
        if (fixedLib) {
            printf("   Success! Shader loaded and compiled\n");
            printf("   Functions: %lu\n", (unsigned long)[[fixedLib functionNames] count]);
            for (NSString *funcName in [fixedLib functionNames]) {
                printf("     - %s\n", [funcName UTF8String]);
            }
        } else {
            printf("   Failed\n");
        }
        
        printf("\nConclusion: The shader loading fix should resolve the issue!\n");
    }
    
    return 0;
}
